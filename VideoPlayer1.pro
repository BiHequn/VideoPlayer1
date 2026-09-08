QT       += core gui sql opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
DEFINES += SDL_MAIN_HANDLED

SOURCES += \
    PacketQueue.cpp \
    main.cpp \
    logindialog.cpp \
    onlinedialog.cpp \
    videodialog.cpp \
    videoplayer.cpp \
    network/networkclient.cpp

HEADERS += \
    PacketQueue.h \
    logindialog.h \
    onlinedialog.h \
    videodialog.h \
    videoplayer.h \
    network/networkclient.h

FORMS += \
    logindialog.ui \
    onlinedialog.ui \
    videodialog.ui

include(./opengl/opengl.pri)
INCLUDEPATH += ./opengl/

INCLUDEPATH += $$PWD/ffmpeg-4.2.2/include \
               $$PWD/SDL2-2.0.10/include \
               $$PWD/opengl/include \
               $$PWD/network

LIBS +=  $$PWD/ffmpeg-4.2.2/lib/avcodec.lib \
         $$PWD/ffmpeg-4.2.2/lib/avdevice.lib \
         $$PWD/ffmpeg-4.2.2/lib/avfilter.lib \
         $$PWD/ffmpeg-4.2.2/lib/avformat.lib \
         $$PWD/ffmpeg-4.2.2/lib/avutil.lib \
         $$PWD/ffmpeg-4.2.2/lib/postproc.lib \
         $$PWD/ffmpeg-4.2.2/lib/swresample.lib \
         $$PWD/ffmpeg-4.2.2/lib/swscale.lib \
         $$PWD/SDL2-2.0.10/lib/x86/SDL2.lib


qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
