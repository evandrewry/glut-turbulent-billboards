project(final)

cmake_minimum_required(VERSION 2.6.2 FATAL_ERROR)

option(BUILD_DEBUG "Turn on the debug mode" OFF)
#===================================================================

## Compiler
# set compiler flags for debug/release
if ( BUILD_DEBUG )
    add_definitions(-DDEBUG)
    set(CMAKE_BUILD_TYPE Debug)
else ()
    add_definitions(-Wno-deprecated)
    set(CMAKE_BUILD_TYPE Release)
endif ()

# package for opengl and glut
find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${GLUT_INCLUDE_DIR} ${GLEW_INCLUDE_DIR})

FILE( GLOB LIB lib/*.c lib/*.cpp )

FILE( GLOB PNGLITE pnglite/*.c )
FILE( GLOB OPENCTM openctm/*.c )
FILE( GLOB MAIN_CPP  src/*.cpp)
FILE( GLOB MAIN_H    src/*.hpp)
FILE( GLOB MAIN_GLSL assets/*.glsl )

ADD_DEFINITIONS( -DGLEW_STATIC -DOPENCTM_STATIC )

INCLUDE_DIRECTORIES(
    lib
    openctm
    pnglite
)

ADD_LIBRARY( ThirdParty
    ${LIB}
    ${OPENCTM}
    ${PNGLITE}
)


ADD_EXECUTABLE( final
    ${MAIN_GLSL}
    ${MAIN_CPP}
    ${MAIN_H}
)

TARGET_LINK_LIBRARIES( final ThirdParty 
    ${GLUT_LIBRARIES}  ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES})
