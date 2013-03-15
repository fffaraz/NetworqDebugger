#include <QtGui/QApplication>

#include "conversationserver.h"
#include <QStringList>
#include <QMessageBox>
#include <QSettings>
#include <QSessionManager>
#include <QMetaEnum>

/*
TODO
- implement SOCKS4/SOCKS4a
- implement "bind" (only after a "connect"?)
- implement UDP ASSOCIATE
- handle connection errors (timeout)
- provide syntax highlighting (autodetect protocol?)
- some live configuration options
 */

class NetworqDebuggerApplication : public QApplication {
public:
    NetworqDebuggerApplication(int argc, char **argv) : QApplication(argc, argv) {
        QStringList args = arguments();
        QHostAddress addr = QHostAddress::Any;
        int port = 8080;

        if(isSessionRestored()) {

            QSettings settings;
            settings.beginGroup(sessionId());
            addr.setAddress(settings.value("address").toString());
            port = settings.value("port").toInt();

            int k = server.metaObject()->enumerator(0).keyToValue(
                        qPrintable(settings.value("mode").toString())
                    );
            if(k!=-1)
                server.setMode((ConversationServer::Mode)k);


            if(!settings.value("remote host").toString().isEmpty()) {
                server.setRemoteHost(settings.value("remote host").toString(),
                                     settings.value("remote port").toInt());
            }
            settings.endGroup();
            settings.remove(sessionId());
        } else {

            if(args.contains("-listen")) {
                int ind = args.indexOf("-listen");
                QRegExp rx("(.*):([0-9]+)");
                if(args.count() <= ind || !rx.exactMatch(args.at(ind+1))) {
                    QTextStream cerr(stderr);
                    cerr << tr("Invalid arguments") << endl;
                    exit(1);
                }
                addr = QHostAddress(rx.cap(1));
                port = rx.cap(2).toInt();
            }
            if(args.contains("-socks5"))
                server.setMode(ConversationServer::Socks5Mode);
            else if(args.contains("-remote")) {
                int ind = args.indexOf("-remote");
                QRegExp rx("(.*):([0-9]+)");
                if(args.count() <= ind || !rx.exactMatch(args.at(ind+1))) {
                    QTextStream cerr(stderr);
                    cerr << tr("Invalid arguments") << endl;
                    exit(1);
                }

                server.setMode(ConversationServer::DefaultRouteMode);
                server.setRemoteHost(rx.cap(1), rx.cap(2).toInt());
            }
        }
        if(!server.listen(addr, port)){
            QMessageBox::critical(0, tr("Error"),
                                  tr("Can't bind to %1:%2").arg(addr.toString()).arg(port));
            exit(1);
        }
        setQuitOnLastWindowClosed(false);
    }
    void saveState ( QSessionManager & manager ) {
        QSettings settings;
        settings.beginGroup(sessionId());
        settings.setValue("address", server.serverAddress().toString());
        settings.setValue("port", server.serverPort());
        settings.setValue("mode", server.metaObject()->enumerator(0).valueToKey(server.mode()));
        settings.setValue("remote host", server.remoteHost());
        settings.setValue("remote port", server.remotePort());
        settings.endGroup();
    }

private:
    ConversationServer server;
};

int main(int argc, char *argv[])
{
    QApplication::setOrganizationName("Qt Centre");
    QApplication::setApplicationName("NetworqDebugger");
    NetworqDebuggerApplication a(argc, argv);
    return a.exec();
}
