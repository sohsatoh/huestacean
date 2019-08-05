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

#ifdef _WIN32
#include <Windows.h>
#endif

extern QQmlApplicationEngine* engine;
QQmlApplicationEngine* engine = nullptr;

QObject *qmlObject;

static QObject *huestacean_singleton_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
        Q_UNUSED(engine);
        Q_UNUSED(scriptEngine);

        Huestacean *huestacean = new Huestacean();
        return huestacean;
}


MyTcpServer::MyTcpServer(QObject *parent) :
    QObject(parent)
{
    server = new QTcpServer(this);

    // whenever a user connects, it will emit signal
    connect(server, SIGNAL(newConnection()),
            this, SLOT(onConnect()));

    connect(server, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(acceptError(QAbstractSocket::SocketError)));

    if(!server->listen(QHostAddress::LocalHost, 8989))
    {
        qDebug() << "Server could not start";
    }
    else
    {
        qDebug() << "Server started!";
    }
}

void MyTcpServer::onConnect()
{
    qDebug() << "CONNECTED";

    // need to grab the socket
    socket = server->nextPendingConnection();

    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "\r\n"
        "<html><head><title>XPG Server</title></head>"
        "<body><p>Start sync...</p></body>"
        "</html>"
    ;

    qDebug() << socket->write(response);

    socket->waitForBytesWritten();

    socket->close();
    // delete socket;

    QString buttonTitle = qmlObject->property("text").toString();

    if ( buttonTitle.contains("Start", Qt::CaseInsensitive) != 0 ) {
        QEvent evtPress(QEvent::MouseButtonPress);
        QEvent evtRelease(QEvent::MouseButtonRelease);

        qmlObject->event(&evtPress);
        qmlObject->event(&evtRelease);

        qDebug() << "Start Sync";
    }
    else {
        qDebug() << "Skipped";
    }
}

void MyTcpServer::acceptError(QAbstractSocket::SocketError socketError)
{
    QMessageBox msgBox;

    QString errorMessage = socket->errorString();

    msgBox.setText(errorMessage);
    msgBox.exec();

    socket->close();
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

        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setOrganizationName("BradyBrenot");
        QCoreApplication::setOrganizationDomain("bradybrenot.com");
        QCoreApplication::setApplicationName("Huestacean");

        QApplication app(argc, argv);

        qmlRegisterSingletonType<Huestacean>("Huestacean", 1, 0, "Huestacean", huestacean_singleton_provider);

        const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);

        QQmlApplicationEngine theEngine;

        ::engine = &theEngine;
        theEngine.rootContext()->setContextProperty("fixedFont", fixedFont);
        theEngine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
        if (theEngine.rootObjects().isEmpty())
                return -1;

        theEngine.rootContext()->setContextProperty("mainWindow", theEngine.rootObjects().first());

        QObject *rootObject = theEngine.rootObjects().first();
        qmlObject = rootObject->findChild<QObject*>("syncButton");

        MyTcpServer server;

        return app.exec();
}
