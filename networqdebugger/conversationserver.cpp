#include "conversationserver.h"
#include <QTcpSocket>
#include <QSystemTrayIcon>
#include "conversation.h"
#include "conversationwidget.h"
#include <QMenu>
#include <QApplication>
#include <QMessageBox>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QtEndian>
#include <QTimer>

class AddressDialog : public QDialog {
public:
    AddressDialog(QWidget *parent = 0) : QDialog(parent) {
        QHBoxLayout *l = new QHBoxLayout;
        lab = new QLabel;
        lab->setText(tr("Address:"));
        l->addWidget(lab);
        le = new QLineEdit;
        //le->setText();
        l->addWidget(le);
        sp = new QSpinBox;
        sp->setRange(1, 65535);
        //sp->setValue(80);
        l->addWidget(sp);
        b = new QPushButton;
        connect(b, SIGNAL(clicked()), this, SLOT(accept()));
        b->setText(tr("OK"));
        l->addWidget(b);
        setLayout(l);
    }
    void setLabelText(QString txt) {
        lab->setText(txt);
    }
    void setButtonText(QString txt) {
        b->setText(txt);
    }
    void setHost(QString hst) {
        le->setText(hst);
    }
    QString host() const { return le->text(); }
    void setPort(int pt) {
        sp->setValue(pt);
    }
    int port() const {
        return sp->value();
    }

private:
    QLabel *lab;
    QPushButton *b;
    QLineEdit *le;
    QSpinBox *sp;
};

ConversationServer::ConversationServer(QObject *parent) :
    QTcpServer(parent)
{
    this->m = AskForRouteMode;
    port = -1;
    QIcon icon(":/internet-device-Vista-icon.png");
    trayicon = new QSystemTrayIcon(icon, this);
    QMenu *menu = new QMenu;
    QActionGroup *actionGroup = new QActionGroup(menu);
    modeActions[Socks5Mode] = actionGroup->addAction(tr("SOCKS5"));
    modeActions[DefaultRouteMode] = actionGroup->addAction(tr("Default route"));
    modeActions[AskForRouteMode] = actionGroup->addAction(tr("Ask for route"));
    foreach(QAction *act, modeActions.values()) {
        act->setCheckable(true);
    }

    modeActions[this->m]->setChecked(true);
    connect(actionGroup, SIGNAL(triggered(QAction*)), this, SLOT(modeActionTriggered(QAction*)));
    menu->addActions(actionGroup->actions());
    menu->addSeparator();
    menu->addAction(tr("Listen on..."), this, SLOT(changeListen()));
    menu->addAction(tr("Set default route..."), this, SLOT(setDefRoute()));
    menu->addSeparator();
    menu->addAction(tr("About..."), this, SLOT(about()));
    menu->addAction(tr("About Qt..."), qApp, SLOT(aboutQt()));
    menu->addAction(tr("Quit"), qApp, SLOT(quit()));
    trayicon->setContextMenu(menu);
    trayicon->show();
    QTimer::singleShot(1000, this, SLOT(initialize()));
}

void ConversationServer::initialize() {
    if(isListening()) {
        trayicon->showMessage(tr("NetworqDebugger"),
                              tr("Listening for connections on %1:%2").arg(serverAddress().toString())
                                                                      .arg(serverPort())
                             );
    }
}

void ConversationServer::incomingConnection(int handle) {
    QTcpSocket *sock = new QTcpSocket(this);
    sock->setSocketDescriptor(handle);
    QTcpSocket *remote = new QTcpSocket(this);
    QString thisHost = host;
    int thisPort = port;
    if(mode() == DefaultRouteMode && thisPort > 0) {
        // nothing to do here
    } else if(mode() == Socks5Mode) {
        // here comes a lousy SOCKS5 implementation...

        // prepare a "general error" response
        QByteArray generalError;
        generalError.append((char)0x05); // SOCKS5
        generalError.append((char)0x01); // general error

        sock->waitForReadyRead();
        QByteArray ba = sock->read(2);
        // 1st byte - SOCKS version
        // 2nd byte - Auth method count

        int authCnt = (int)ba.at(1);

        // we only serve SOCKS5
        if(ba.at(0)!=5) {
            sock->close();
            sock->deleteLater();
            delete remote;
            trayicon->showMessage(tr("NetworqDebugger"), tr("Wrong SOCKS version. Connection cancelled"));
            return;
        }
        while(sock->bytesAvailable() < authCnt)
            sock->waitForReadyRead();
        // ignore auth
        sock->read(authCnt);
        QByteArray response;
        response.append((char)0x05); // SOCKS version
        response.append((char)0x00); // requested auth method (0x00 means no auth)
        sock->write(response);
        sock->waitForBytesWritten();
        while(sock->bytesAvailable() < 4)
            sock->waitForReadyRead();
        ba = sock->read(4);
        // 1st byte - SOCKS version
        // 2nd byte - command (1 = open TCP stream)
        // 3rd byte - 0x00
        // 4th byte - address type (1 = IPv4, 3 = QDN, 4 = IPv6)
        if(ba.at(0)!=5) {
            sock->write(generalError);
            sock->waitForBytesWritten();
            sock->close();
            sock->deleteLater();
            delete remote;
            trayicon->showMessage(tr("NetworqDebugger"), tr("Wrong SOCKS version. Connection cancelled"));
            return;
        } else if(ba.at(1)!=1) {
            // only handle "open stream" requests
            sock->write(generalError);
            sock->waitForBytesWritten();
            sock->close();
            sock->deleteLater();
            delete remote;
            trayicon->showMessage(tr("NetworqDebugger"), tr("Unhandled SOCKS command. Connection cancelled"));
            return;
        } else if(ba.at(2)!=0) {
            sock->write(generalError);
            sock->waitForBytesWritten();
            sock->close();
            sock->deleteLater();
            delete remote;
            trayicon->showMessage(tr("NetworqDebugger"), tr("Broken SOCKS client implementation. Connection cancelled"));
            return;
        }
        // reuse old response object containing 0x05, 0x00
        // 1st byte = SOCKS version
        // 2nd byte = status (0x00 = success)
        // 3rd byte = 0x00
        // 4th and following bytes - repeat what the client sent, not sure that's correct,
        //                                                        it should contain the outgoing host:port pair
        response.append((char)0x00);
        response.append(ba.at(3));
        switch(ba.at(3)) {
        // IPv4
        case 1: {
                while(sock->bytesAvailable() < 4)
                    sock->waitForReadyRead();
                ba = sock->read(4);
                QHostAddress addr((quint32)ba.constData());
                thisHost = addr.toString();
                response.append(ba);
            };
            break;
        // domain name
        case 3: {
                while(sock->bytesAvailable() < 1)
                    sock->waitForReadyRead();
                ba = sock->read(1);
                int len = ba.at(0);
                response.append(ba);
                while(sock->bytesAvailable() < len)
                    sock->waitForReadyRead();
                ba = sock->read(len);
                thisHost = ba;
                response.append(ba);
            };
            break;
        default: {
                // deny IPv6 (yes, I'm lazy, it's just a few lines of code) and incorrect values
                sock->write(generalError);
                sock->waitForBytesWritten();
                sock->close();
                sock->deleteLater();
                delete remote;
                trayicon->showMessage(tr("NetworqDebugger"), tr("Unhandled SOCKS address type. Connection cancelled"));
                return;
            }
        }
        while(sock->bytesAvailable() < 2)
            sock->waitForReadyRead();
        ba = sock->read(2);
        // port in network order
        thisPort = qFromBigEndian<qint16>((uchar*)ba.constData());
        response.append(ba);
        sock->write(response); // notify success and proceed with remote connection

    } else if(mode() == AskForRouteMode) {
        AddressDialog dlg;
        dlg.setLabelText(tr("Remote:"));
        dlg.setButtonText(tr("Connect"));
        dlg.setHost(host);
        dlg.setPort(port == -1 ? 80 : port);
        dlg.setWindowTitle(tr("Enter remote host and port"));
        if(!dlg.exec() || dlg.host().isEmpty()){
            sock->close();
            sock->deleteLater();
            delete remote;
            trayicon->showMessage(tr("NetworqDebugger"), tr("Connection cancelled"));
            return;
        }
        thisHost = dlg.host();
        thisPort = dlg.port();
    } else {
        qFatal("Wrong mode");
    }
    remote->connectToHost(thisHost, thisPort);
    trayicon->showMessage(tr("NetworqDebugger"), tr("Connecting to %1:%2").arg(thisHost).arg(thisPort));
    Conversation *conversation = new Conversation(this);
    ConversationWidget *widget = new ConversationWidget;
    conversation->setWidget(widget);
    conversation->setLocalSocket(sock);
    conversation->setRemoteSocket(remote);
    widget->show();
    connect(conversation, SIGNAL(connectionClosed()), conversation, SLOT(deleteLater()));

}



void ConversationServer::setMode(Mode m) {
    if(this->m == m) return;
    this->m = m;
    modeActions[m]->setChecked(true);
}

ConversationServer::Mode ConversationServer::mode() const {
    return this->m;
}

void ConversationServer::setRemoteHost(const QString &host, int port) {
    this->host = host;
    this->port = port;
}

QString ConversationServer::remoteHost() const {
    return this->host;
}

int ConversationServer::remotePort() const {
    return this->port;
}

void ConversationServer::modeActionTriggered(QAction *act) {
    Mode mod = modeActions.key(act);
    if(mod==mode())
        return;
    setMode(mod);
    trayicon->showMessage(tr("NetworqDebugger"),
                          tr("Mode changed to '%1'").arg(act->text()), QSystemTrayIcon::Information, 3000);
}

void ConversationServer::about() {
    QString aboutText =
            tr("<html><body>\n"
               "<table><tr>\n"
               "<td><img src=\":/internet-device-Vista-icon.png\" width=\"128\"/></td>\n"
               "<td>\n"
               "<h1>NetworqDebugger</h1>\n"
               "<p>The application provides a network proxy for debugging network traffic\n"
               "by displaying the conversation between the client and a server in a window.<br/>\n"
               "Currently two major modes of operation are supported:\n"
               "<ul>\n"
               "<li><b>SOCKS5</b> - a SOCKS compliant network proxy</li>\n"
               "<li><b>transparent proxy</b> - a simple forwarding network proxy</li>\n"
               "</ul>\n"
               "The preferred mode of operation is SOCKS5.\n"
               "To use it, configure a <tt>QNetworkProxy</tt> in your application, i.e. in <tt>main()</tt>:\n"
               "<div style='font-size: larger; margin: 5mm;'><code><pre>\n"
               "QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, \n"
               "                    \"%1\", %2);\n"
               "QNetworkProxy::setApplicationProxy(proxy);"
               "</pre></code></div>\n"
               "For the other mode simply direct your client code to <tt>%1:%2</tt> instead of a remote server.\n"
               "</p>\n"
               "<p style=\"font-size: 8px;\">Application icon provided by <a href=\"http://www.devcom.com\">devcom.com</a></p>\n"
               "</td>\n"
               "<tr></table>\n"
               "</body></html>");
    QMessageBox::about(0, tr("About NetworqDebugger"), aboutText.arg(serverAddress()== QHostAddress::Any ? "localhost" : serverAddress().toString()).arg(serverPort()));
}

void ConversationServer::changeListen() {
    AddressDialog dlg;
    dlg.setWindowTitle(tr("Set new address binding"));
    dlg.setLabelText(tr("Listen on:"));
    dlg.setHost(serverAddress().toString());
    dlg.setPort(serverPort());
    if(dlg.exec()){
        QString hst = dlg.host();
        int prt = dlg.port();
        if(prt>0 && !hst.isEmpty()) {
            close();
            if(listen(QHostAddress(hst), prt)){
                trayicon->showMessage(tr("NetworqDebugger"),
                                       tr("Listening for connections on %1:%2").arg(serverAddress().toString())
                                                                               .arg(serverPort())
                                      );
            }
        }
    }
}

void ConversationServer::setDefRoute() {
    AddressDialog dlg;
    dlg.setWindowTitle(tr("Set new default route"));
    dlg.setLabelText(tr("Remote:"));
    dlg.setButtonText(tr("Set"));
    dlg.setHost(remoteHost());
    dlg.setPort(remotePort());
    if(dlg.exec()) {
        setRemoteHost(dlg.host(), dlg.port());
    }
}
