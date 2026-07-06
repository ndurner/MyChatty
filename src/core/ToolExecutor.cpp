#include "ToolExecutor.h"

#include "ConversationFileStore.h"

#include <QJsonObject>
#include <QJSEngine>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

namespace MyChatty {
namespace {

QJsonObject toolOk(const QString &prints)
{
    return QJsonObject{{"success", true}, {"prints", prints}};
}

QJsonObject toolError(const QString &message)
{
    return QJsonObject{{"success", false}, {"error", message}};
}

class ScriptHost : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void print(const QString &value)
    {
        m_output.append(value);
    }

    QString output() const
    {
        return m_output.join('\n') + (m_output.isEmpty() ? QString() : QStringLiteral("\n"));
    }

private:
    QStringList m_output;
};

} // namespace

ToolExecutor::ToolExecutor(SettingsStore *settings, QString conversationId)
    : m_settings(settings)
    , m_conversationId(std::move(conversationId))
{
}

QJsonObject ToolExecutor::execute(const QString &name, const QJsonObject &arguments) const
{
    if (name == QStringLiteral("eval_javascript")) {
        return evalJavaScript(arguments);
    }
    return toolError(QStringLiteral("Unknown tool '%1'").arg(name));
}

QJsonObject ToolExecutor::evalJavaScript(const QJsonObject &arguments) const
{
    Q_UNUSED(m_settings);
    const QString source = arguments.value("javascript_source_code").toString();
    if (source.trimmed().isEmpty()) {
        return toolError(QStringLiteral("javascript_source_code is required"));
    }

    QJSEngine engine;
    ScriptHost host;
    ConversationFileStore fs(m_conversationId.isEmpty() ? QStringLiteral("adhoc") : m_conversationId);
    engine.globalObject().setProperty(QStringLiteral("__host"), engine.newQObject(&host));
    engine.globalObject().setProperty(QStringLiteral("fs"), engine.newQObject(&fs));
    engine.evaluate(QStringLiteral(
        "this.print = function() {"
        "  __host.print(Array.prototype.map.call(arguments, function(value) {"
        "    return typeof value === 'string' ? value : JSON.stringify(value);"
        "  }).join(' '));"
        "};"
        "this.require = undefined;"
        "this.importScripts = undefined;"
    ));

    std::atomic_bool finished = false;
    std::mutex watchdogMutex;
    std::condition_variable watchdogCondition;
    std::thread watchdog([&engine, &finished, &watchdogMutex, &watchdogCondition]() {
        std::unique_lock<std::mutex> lock(watchdogMutex);
        const bool completed = watchdogCondition.wait_for(lock, std::chrono::seconds(2), [&finished]() {
            return finished.load();
        });
        if (!completed) {
            engine.setInterrupted(true);
        }
    });

    const QJSValue result = engine.evaluate(source, QStringLiteral("model-tool.js"));
    finished.store(true);
    watchdogCondition.notify_one();
    if (watchdog.joinable()) {
        watchdog.join();
    }
    if (result.isError()) {
        return toolError(QStringLiteral("%1:%2: %3")
                             .arg(result.property(QStringLiteral("fileName")).toString())
                             .arg(result.property(QStringLiteral("lineNumber")).toInt())
                             .arg(result.toString()));
    }
    QString prints = host.output();
    if (prints.isEmpty() && !result.isUndefined()) {
        prints = result.toString() + QStringLiteral("\n");
    }
    return toolOk(prints);
}

} // namespace MyChatty

#include "ToolExecutor.moc"
