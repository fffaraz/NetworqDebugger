#-------------------------------------------------
#
# Project created by QtCreator 2010-09-10T00:20:00
#
#-------------------------------------------------

QT       += core gui network widgets

TARGET = networqdebugger
TEMPLATE = app


SOURCES += main.cpp\
        conversationwidget.cpp \
    conversation.cpp \
    conversationserver.cpp

HEADERS  += conversationwidget.h \
    conversation.h \
    conversationserver.h

FORMS    += conversationwidget.ui

RESOURCES += \
    resources.qrc
