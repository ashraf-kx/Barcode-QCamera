TEMPLATE = app
TARGET = camera

QT += multimedia multimediawidgets gui widgets

HEADERS = \
    camera.h \
    imagesettings.h

SOURCES = \
    main.cpp \
    camera.cpp \
    imagesettings.cpp

FORMS += \
    camera.ui \
    imagesettings.ui

target.path = $$[QT_INSTALL_EXAMPLES]/multimediawidgets/camera
INSTALLS += target


include(QZXing/QZXing.pri)

RESOURCES += \
    sounds.qrc

