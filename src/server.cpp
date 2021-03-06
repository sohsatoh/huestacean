/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QtNetwork>
#include <QtCore>
#include <QSettings>

#include <QMessageBox>

#include "server.h"
#include "huestacean.h"

QString buttonTitle;
char* response;

Server::Server(QObject *parent)
        : QObject(parent)
{
        QSettings settings;
        int enableServer = settings.value("enableServer").toInt();

        if(enableServer) {
                QNetworkConfigurationManager manager;
                if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
                        // Get saved network configuration
                        QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
                        settings.beginGroup(QLatin1String("QtNetwork"));
                        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
                        settings.endGroup();

                        // If the saved network configuration is not currently discovered use the system default
                        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
                        if ((config.state() & QNetworkConfiguration::Discovered) !=
                            QNetworkConfiguration::Discovered) {
                                config = manager.defaultConfiguration();
                        }

                        networkSession = new QNetworkSession(config, this);
                        connect(networkSession, &QNetworkSession::opened, this, &Server::sessionOpened);

                        networkSession->open();
                } else {
                        sessionOpened();
                }

                connect(tcpServer, &QTcpServer::newConnection, this, &Server::connected);
                connect(tcpServer, &QTcpServer::serverError, this, &Server::error);
        }
}

void Server::sessionOpened()
{
        if (networkSession) {
                QNetworkConfiguration config = networkSession->configuration();
                QString id;
                if (config.type() == QNetworkConfiguration::UserChoice)
                        id = networkSession->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
                else
                        id = config.identifier();

                QSettings settings(QSettings::UserScope, QLatin1String("QtProject"));
                settings.beginGroup(QLatin1String("QtNetwork"));
                settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
                settings.endGroup();
        }

        //Read Settings
        QSettings settings;
        settings.beginGroup("Server");
        savedPortNum = settings.value("savedPortNum", "8989").toInt();
        settings.endGroup();

        tcpServer = new QTcpServer(this);
        if (!tcpServer->listen(QHostAddress::LocalHost, savedPortNum)) {
                return;
        }

        port = tcpServer->serverPort();

        QString ipAddress;
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        // use the first non-localhost IPv4 address
        for (int i = 0; i < ipAddressesList.size(); ++i) {
                if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                    ipAddressesList.at(i).toIPv4Address()) {
                        ipAddress = ipAddressesList.at(i).toString();
                        break;
                }
        }

        // if we did not find one, use IPv4 localhost
        if (ipAddress.isEmpty())
                ipAddress = QHostAddress(QHostAddress::LocalHost).toString();


}

//! [4]
void Server::connected()
{
        while (tcpServer->hasPendingConnections())
        {
                QTcpSocket *socket = tcpServer->nextPendingConnection();
                connect(socket, SIGNAL(readyRead()), SLOT(readyRead()));
                connect(socket, SIGNAL(disconnected()), SLOT(disconnected()));
        }
}

void Server::readyRead() {
        QMetaObject::invokeMethod(rootObject, "changeList");

        QTcpSocket *socket = static_cast<QTcpSocket*>(sender());
        QStringList tokens = QString(socket->readLine()).split(QRegExp("[ \r\n][ \r\n]*"));

        QObject *qmlObject = rootObject->findChild<QObject*>("syncButton");
        buttonTitle = qmlObject->property("text").toString();

        if ( tokens[1].contains("start", Qt::CaseInsensitive) != 0 ) {
                response =
                        "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Access-Control-Allow-Headers: Content-Type\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n"
                        "\r\n"
                        "<html><head><title>Fortune Server</title></head>"
                        "<body><p>Start sync...</p></body>"
                        "</html>"
                ;
                if (buttonTitle.contains("Start", Qt::CaseInsensitive) != 0 ) {
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
        else {
                response =
                        "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n"
                        "Access-Control-Allow-Origin: *\r\n"
                        "Access-Control-Allow-Headers: Content-Type\r\n"
                        "Content-Type: text/html; charset=UTF-8\r\n"
                        "\r\n"
                        "<html><head><title>Fortune Server</title></head>"
                        "<body><p>Stop sync...</p></body>"
                        "</html>"
                ;
                if ( buttonTitle.contains("Stop", Qt::CaseInsensitive) != 0 ) {
                        QEvent evtPress(QEvent::MouseButtonPress);
                        QEvent evtRelease(QEvent::MouseButtonRelease);

                        qmlObject->event(&evtPress);
                        qmlObject->event(&evtRelease);

                        qDebug() << "Stop Sync";
                }
                else {
                        qDebug() << "Skipped";
                }
        }

        socket->write(response);
        socket->disconnectFromHost();
}

void Server::setPort(int portNum) {
        //save portNum to setSavedPortNum
        QSettings settings;
        settings.beginGroup("Server");
        settings.setValue("savedPortNum",portNum);
        settings.endGroup();
        restart();
}

void Server::restart() {
        QSettings settings;
        settings.beginGroup("Server");
        savedPortNum = settings.value("savedPortNum", "8989").toInt();
        settings.endGroup();

        tcpServer->close();

        port = tcpServer->serverPort();

        tcpServer->listen(QHostAddress::LocalHost, savedPortNum);

        QString msg = "Restarted the Server";
        QMessageBox Msgbox;
        Msgbox.setText(msg);
        Msgbox.exec();
}

//Catch Error
void Server::error() {
        QString msg = tcpServer->errorString();

        QMessageBox Msgbox;
        Msgbox.setText(msg);
        Msgbox.exec();

        restart();
}
