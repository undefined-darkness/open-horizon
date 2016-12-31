cmake_minimum_required(VERSION 2.8)

project(convert_locations)

set("root" ../)

add_subdirectory(${root}deps/nya-engine nya-engine)
include_directories(${root}deps/nya-engine)
include_directories(${root}deps/pugixml-1.4/src)
include_directories(${root}deps/zip/src)
include_directories(${root})

define_source_files(${root}convert_locations)
define_source_files(${root}containers)
define_source_files(${root}deps/pugixml-1.4/src)
list(APPEND src_files ${root}deps/zip/src/zip.c)
list(APPEND src_files ${root}renderer/mesh_ndxr.cpp)
list(APPEND src_files ${root}renderer/mesh_sm.cpp)
list(APPEND src_files ${root}renderer/texture_gim.cpp)

set(CMAKE_CXX_FLAGS "-std=c++0x -Wno-multichar")

add_executable(convert_locations ${src_files})

target_link_libraries(convert_locations nya_engine)

find_package(OpenGL REQUIRED)
if (NOT OPENGL_FOUND)
    message(ERROR " OpenGL not found!")
endif()
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
target_link_libraries(convert_locations ${OPENGL_LIBRARIES})

if (WIN32)
    include_directories(${root}deps/zlib-1.2.8)
    target_link_libraries(convert_locations ${root}deps/zlib-1.2.8/zlib.lib)
else ()
    find_package(ZLIB)
    if (NOT ZLIB_FOUND)
        message(ERROR " zlib not found!")
    endif()
    include_directories(${ZLIB_INCLUDE_DIRS})
    target_link_libraries(convert_locations ${ZLIB_LIBRARIES})
    target_link_libraries(convert_locations pthread)
endif()

