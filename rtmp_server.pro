#-------------------------------------------------
#
# Project created by QtCreator 2015-07-28T10:55:28
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = rtmp_server
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    rtmp.cpp

HEADERS += \
    rtmp.hpp

QMAKE_CXXFLAGS += -std=c++11
QMAKE_LIBS += -lboost_system-mt -lws2_32 -lmswsock

#QMAKE_LIBDIR += D:\ToolKit\boost_1_57_0\lib
#QMAKE_INCDIR += D:\ToolKit\boost_1_57_0
#QMAKE_LIBS += -ID:/ToolKit/boost_1_57_0/lib -lboost_system-vc120-mt-1_57
