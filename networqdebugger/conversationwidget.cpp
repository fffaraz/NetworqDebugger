#include "conversationwidget.h"
#include "ui_conversationwidget.h"
#include <QTextCursor>

ConversationWidget::ConversationWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ConversationWidget)
{
    ui->setupUi(this);
}

ConversationWidget::~ConversationWidget()
{
    delete ui;
}

void ConversationWidget::appendServer(const QString &txt, QTextCharFormat fmt) {
    QTextCursor cursor(ui->serverLog->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(txt, fmt);

}

void ConversationWidget::appendClient(const QString &txt, QTextCharFormat fmt) {
    QTextCursor cursor(ui->clientLog->document());
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(txt, fmt);
}

void ConversationWidget::clearServer() {
    ui->serverLog->clear();
}

void ConversationWidget::clearClient() {
    ui->clientLog->clear();
}
