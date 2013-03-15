#ifndef CONVERSATIONSERVER_H
#define CONVERSATIONSERVER_H

#include <QTcpServer>

class QSystemTrayIcon;
class QAction;
class ConversationServer : public QTcpServer
{
    Q_OBJECT
    Q_ENUMS(Mode);
public:
    enum Mode {
        Socks5Mode, DefaultRouteMode, AskForRouteMode
    };

    explicit ConversationServer(QObject *parent = 0);
    void setMode(Mode m);
    Mode mode() const;
    void setRemoteHost(const QString &host, int port);
    QString remoteHost() const;
    int remotePort() const;
signals:

public slots:
    void about();
protected:
    void incomingConnection(int handle);
private slots:
    void initialize();
    void modeActionTriggered(QAction*);
    void changeListen();
    void setDefRoute();
private:
    Mode m;
    QString host;
    int port;
    QSystemTrayIcon *trayicon;
    QMap<Mode, QAction*> modeActions;
};

#endif // CONVERSATIONSERVER_H
