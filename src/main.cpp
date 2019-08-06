#include <QApplication>
#include <QQmlApplicationEngine>
#include <QFontDatabase>
#include <QQmlContext>
#include <QtHttpServer>

#include <QTcpSocket>
#include <QTcpServer>
#include <QDebug>
#include <QMessageBox>




#include "huestacean.h"
#include "entertainment.h"
#include "server.h"

#ifdef _WIN32
#include <Windows.h>
#endif

extern QQmlApplicationEngine* engine;
QQmlApplicationEngine* engine = nullptr;

QObject *rootObject;

static QObject *huestacean_singleton_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
        Q_UNUSED(engine);
        Q_UNUSED(scriptEngine);

        Huestacean *huestacean = new Huestacean();
        return huestacean;
}

static QObject *server_singleton_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
        Q_UNUSED(engine);
        Q_UNUSED(scriptEngine);

        Server *server = new Server();
        return server;
}


int main(int argc, char *argv[])
{
#ifdef _WIN32
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
                freopen("CONOUT$", "w", stdout);
                freopen("CONOUT$", "w", stderr);
        }
#endif

        //https://github.com/sqlitebrowser/sqlitebrowser/commit/73946400c32d1f7cfcd4672ab0ab3f563eb84f4e
        qputenv("QT_BEARER_POLL_TIMEOUT", QByteArray::number(INT_MAX));

        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QApplication::setOrganizationName("BradyBrenot");
        QApplication::setOrganizationDomain("bradybrenot.com");
        QApplication::setApplicationName("Huestacean");

        QApplication app(argc, argv);

        qmlRegisterSingletonType<Huestacean>("Huestacean", 1, 0, "Huestacean", huestacean_singleton_provider);
        qmlRegisterSingletonType<Server>("Server", 1, 0, "Server", server_singleton_provider);

        const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

        QQmlApplicationEngine theEngine;

        ::engine = &theEngine;
        theEngine.rootContext()->setContextProperty("fixedFont", fixedFont);
        theEngine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
        if (theEngine.rootObjects().isEmpty())
                return -1;

        theEngine.rootContext()->setContextProperty("mainWindow", theEngine.rootObjects().first());

        rootObject = theEngine.rootObjects().first();

        return app.exec();
}
