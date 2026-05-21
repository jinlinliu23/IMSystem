#include "cpp/clientfacade.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    ClientFacade clientFacade;

    QQmlApplicationEngine engine;
    // 向 QML 暴露 ClientFacade，RegisterPage 等可直接调用 registerUser()
    engine.rootContext()->setContextProperty(QStringLiteral("ClientFacade"), &clientFacade);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("client", "Main");

    return QCoreApplication::exec();
}
