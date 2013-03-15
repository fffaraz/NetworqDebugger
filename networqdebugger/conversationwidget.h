#ifndef CONVERSATIONWIDGET_H
#define CONVERSATIONWIDGET_H

#include <QWidget>
#include <QTextCharFormat>

namespace Ui {
    class ConversationWidget;
}

class ConversationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ConversationWidget(QWidget *parent = 0);
    ~ConversationWidget();
public slots:
    void appendServer(const QString &, QTextCharFormat fmt = QTextCharFormat());
    void appendClient(const QString &, QTextCharFormat fmt = QTextCharFormat());
    void clearServer();
    void clearClient();
private:
    Ui::ConversationWidget *ui;
};

#endif // CONVERSATIONWIDGET_H
