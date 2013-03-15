#include "conversation.h"
#include "conversationwidget.h"
#include <QTcpSocket>

Conversation::Conversation(QObject *parent) :
    QObject(parent)
{
    remote = local = 0;
    widget = 0;
    closing = false;
}

void Conversation::setRemoteSocket(QTcpSocket *sock) {
    if(remote) {
        disconnect(remote, SIGNAL(readyRead()), this, SLOT(remoteRead()));
        disconnect(remote, SIGNAL(disconnected()), this, SLOT(remoteClosed()));
    }
    remote = sock;
    if(remote) {
        connect(remote, SIGNAL(readyRead()), this, SLOT(remoteRead()));
        connect(remote, SIGNAL(disconnected()), this, SLOT(remoteClosed()));
        remoteRead();
    }
}

void Conversation::setLocalSocket(QTcpSocket *sock) {
    if(local) {
        disconnect(local, SIGNAL(readyRead()), this, SLOT(localRead()));
        disconnect(local, SIGNAL(disconnected()), this, SLOT(localClosed()));
    }
    local = sock;
    if(local) {
        connect(local, SIGNAL(readyRead()), this, SLOT(localRead()));
        connect(local, SIGNAL(disconnected()), this, SLOT(localClosed()));
        localRead();
    }
}

void Conversation::remoteRead() {
    QByteArray data = remote->readAll();
    if(data.isEmpty())
        return;
    if(widget) {
        QTextStream stream(data);
        widget->appendServer(stream.readAll());
    }
    if(local)
        local->write(data);

}

void Conversation::localRead() {
    QByteArray data = local->readAll();
    if(data.isEmpty())
        return;
    if(widget){
        QTextStream stream(data);
        widget->appendClient(stream.readAll());
    }
    if(remote)
        remote->write(data);
}

void Conversation::remoteClosed() {
    if(widget && !closing) {
        QTextCharFormat fmt;
        fmt.setForeground(Qt::red);
        widget->appendServer(tr("Connection closed"), fmt);
        closing = true;
    }
    if(local)
        local->disconnectFromHost();
    emit connectionClosed();
}

void Conversation::localClosed() {

    if(widget && !closing) {
        QTextCharFormat fmt;
        fmt.setForeground(Qt::red);
        widget->appendClient(tr("Connection closed"), fmt);
        closing = true;
    }
    if(remote)
        remote->disconnectFromHost();
    emit connectionClosed();
}

void Conversation::setWidget(ConversationWidget *wgt) {
    widget = wgt;
}
