#-------------------------------------------------
#
# Project created by QtCreator 2014-04-23T13:12:55
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = PortMidiTest
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp

INCLUDEPATH += /usr/local/include
LIBS += -L/usr/local/lib -lportmidi
