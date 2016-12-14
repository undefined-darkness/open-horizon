#
# open horizon -- undefined_darkness@outlook.com
#
#-------------------------------------------------

QT += core gui opengl widgets
win32: LIBS += -lopengl32 -lwinmm
macx: LIBS += -lz
win32: LIBS += ../deps/zlib-1.2.8/zlib.lib

macx: LIBS += ../deps/lua-5.2.4/liblua52.a
win32: LIBS += ../deps/lua-5.2.4/lua52.lib

QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -msse -msse2

macx: QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-reorder

TARGET = mission_editor
TEMPLATE = app

win32: PLATFORM_NAME = win32
macx: PLATFORM_NAME = mac

CONFIG(debug, debug | release): BIN_DIR_NAME = $${PLATFORM_NAME}_debug
CONFIG(release, debug | release): BIN_DIR_NAME = $${PLATFORM_NAME}_release

DEPS_PATH=$${_PRO_FILE_PWD_}/../deps/
include($${DEPS_PATH}/nya-engine/build/qt5/nya_sources.pri)

DESTDIR = ../bin/$${BIN_DIR_NAME}/
OBJECTS_DIR = ../obj/$${BIN_DIR_NAME}/
MOC_DIR = $${OBJECTS_DIR}/

INCLUDEPATH += ../
INCLUDEPATH += $${DEPS_PATH}/pugixml-1.4/src/
INCLUDEPATH += $${DEPS_PATH}/zip/src/
INCLUDEPATH += $${DEPS_PATH}/lua-5.2.4/include/
win32: INCLUDEPATH += $${DEPS_PATH}/zlib-1.2.8/

QMAKE_TARGET_BUNDLE_PREFIX = open-horizon

#-------------------------------------------------

SOURCES += \
    ../containers/fhm.cpp \
    ../containers/dpl.cpp \
    ../containers/qdf.cpp \
    main.cpp\
    main_window.cpp \
    scene_view.cpp \
    ../renderer/model.cpp \
    ../renderer/fhm_mesh.cpp \
    ../renderer/mesh_ndxr.cpp \
    ../renderer/location_params.cpp \
    ../renderer/location.cpp \
    ../renderer/mesh_obj.cpp \
    ../renderer/fhm_location.cpp \
    ../renderer/sky.cpp \
    ../renderer/shared.cpp \
    ../deps/pugixml-1.4/src/pugixml.cpp \
    ../phys/mesh.cpp \
    ../phys/physics.cpp \
    ../phys/plane_params.cpp \
    ../deps/zip/src/zip.c \
    ../deps/nya-engine/extensions/zip_resources_provider.cpp \
    ../util/script.cpp \
    ../util/platform_dialogs.cpp \
    ../util/location.cpp

HEADERS += \
    ../containers/fhm.h \
    ../containers/dpl.h \
    ../containers/qdf.h \
    ../containers/decrypt.h \
    ../containers/dpl_provider.h \
    ../containers/qdf_provider.h \
    main_window.h \
    scene_view.h \
    ../renderer/model.h \
    ../renderer/fhm_mesh.h \
    ../renderer/mesh_ndxr.h \
    ../renderer/location_params.h \
    ../renderer/fhm_location.h \
    ../renderer/location.h \
    ../renderer/mesh_obj.h \
    ../renderer/sky.h \
    ../renderer/shared.h \
    ../util/resources.h \
    ../util/config.h \
    ../util/location.h \
    ../util/util.h \
    ../util/params.h \
    ../util/zip.h \
    ../util/xml.h \
    ../util/script.h \
    ../deps/pugixml-1.4/src/pugiconfig.hpp \
    ../deps/pugixml-1.4/src/pugixml.hpp \
    ../phys/physics.h \
    ../phys/mesh.h \
    ../phys/plane_params.h \
    ../deps/zip/src/miniz.h \
    ../deps/zip/src/zip.h \
    ../deps/nya-engine/extensions/zip_resources_provider.h

#-------------------------------------------------
