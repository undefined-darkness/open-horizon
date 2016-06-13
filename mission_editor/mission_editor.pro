#
# open horizon -- undefined_darkness@outlook.com
#
#-------------------------------------------------

QT += core gui opengl widgets
win32: LIBS += -lopengl32 -lwinmm

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-reorder

TARGET = mission_editor
TEMPLATE = app

CONFIG(debug, debug | release): BIN_DIR_NAME = $${TARGET}_debug
CONFIG(release, debug | release): BIN_DIR_NAME = $${TARGET}_release

DEPS_PATH=$${_PRO_FILE_PWD_}/../deps/
include($${DEPS_PATH}/nya-engine/build/qt5/nya_sources.pri)

DESTDIR = ../bin/$${BIN_DIR_NAME}/
OBJECTS_DIR = ../obj/$${BIN_DIR_NAME}/
MOC_DIR = $${OBJECTS_DIR}/

INCLUDEPATH += ../

QMAKE_TARGET_BUNDLE_PREFIX = open-horizon

#-------------------------------------------------

SOURCES += main.cpp\
        main_window.cpp \
    scene_view.cpp

HEADERS += main_window.h \
    scene_view.h

#-------------------------------------------------
