#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "RobotBridge.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("LeRobot Qt HMI"));

    RobotBridge bridge;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("robot"), &bridge);

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("LeRobotHmi", "Main");
    if (engine.rootObjects().isEmpty()) return -1;

    return app.exec();
}
