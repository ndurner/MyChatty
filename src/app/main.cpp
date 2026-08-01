#include "ChatController.h"
#include "MarkdownRenderer.h"
#include "SettingsStore.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStringList>
#include <QTimer>

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
    bool verifyQmlLoad = false;
    const QStringList arguments = app.arguments();
    for (const QString &argument : arguments) {
        if (argument.startsWith("--ui-state=")) {
            initialUiState = argument.mid(QString("--ui-state=").size());
        } else if (argument == "--verify-qml-load") {
            verifyQmlLoad = true;
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
    if (verifyQmlLoad) {
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                         [&app](QObject *object, const QUrl &) {
                             if (object) {
                                 qInfo("MYCHATTY_QML_LOAD_OK");
                                 QTimer::singleShot(0, &app, &QCoreApplication::quit);
                             }
                         });
    }
    engine.loadFromModule("MyChatty", "Main");
    return app.exec();
}
