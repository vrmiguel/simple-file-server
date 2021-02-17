TEMPLATE = app
CONFIG += console
CONFIG += c11
CONFIG -= app_bundle
CONFIG -= qt



SOURCES += \
        main.c \
        request.c \
        utils.c

HEADERS += \
    config.h \
    request.h \
    utils.h
