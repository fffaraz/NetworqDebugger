#ifndef CONVERSATION_H
#define CONVERSATION_H

#include <QObject>


class QTcpSocket;
class ConversationWidget;

class Conversation : public QObject
{
    Q_OBJECT
public:
    explicit Conversation(QObject *parent = 0);
    void setRemoteSocket(QTcpSocket*);
    void setLocalSocket(QTcpSocket*);
    void setWidget(ConversationWidget*);
signals:
    void connectionClosed();
public slots:

private slots:
    void remoteRead();
    void localRead();
    void remoteClosed();
    void localClosed();
private:
    QTcpSocket *remote;
    QTcpSocket *local;
    ConversationWidget *widget;
    bool closing;
};

#endif // CONVERSATION_H
