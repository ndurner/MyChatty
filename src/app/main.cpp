#include "ChatController.h"
#include "MarkdownRenderer.h"
#include "SettingsStore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStringList>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("NilsDurner");
    QCoreApplication::setApplicationName("MyChatty");
    QQuickStyle::setStyle("Basic");

    qRegisterMetaType<MyChatty::ChatResult>("MyChatty::ChatResult");

    MyChatty::SettingsStore settings;
    MyChatty::ChatController chat(&settings);
    MyChatty::MarkdownRenderer markdown;
    QString initialUiState;
    const QStringList arguments = app.arguments();
    for (const QString &argument : arguments) {
        if (argument.startsWith("--ui-state=")) {
            initialUiState = argument.mid(QString("--ui-state=").size());
        }
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("settingsStore", &settings);
    engine.rootContext()->setContextProperty("chatController", &chat);
    engine.rootContext()->setContextProperty("markdownRenderer", &markdown);
    engine.rootContext()->setContextProperty("initialUiState", initialUiState);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.loadFromModule("MyChatty", "Main");
    return app.exec();
}
