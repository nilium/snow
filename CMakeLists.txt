cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
include(ExternalProject)
include(FindPkgConfig)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

project(snow C CXX)
set(CMAKE_CXX_STANDARD 11)
set(THIRDPARTY_INSTALL_DIR "${CMAKE_SOURCE_DIR}/3p")

include(BuildAngelScript)
include(BuildEnet)
include(BuildFLTK)
include(BuildGLFW3)
include(BuildLua)
include(BuildPhysFS)
include(BuildSnowCommon)
include(BuildZeroMQ)

# Build snow proper
find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCE_FILES
        src/*.c src/*.cc src/*.cpp
        ext/*.c ext/*.cc ext/*.cpp
        )

if(APPLE)
        file(GLOB_RECURSE APPLE_SOURCES src/*.mm ext/*.mm)
        list(APPEND SOURCE_FILES ${APPLE_SOURCES})
endif(APPLE)

# Drop inline source files (such as template implementations)
list(FILTER SOURCE_FILES EXCLUDE REGEX /inline/)

include_directories(
        ext
        src
        "${SCOM_INCLUDE_DIR}"
        "${PHYSFS_INCLUDE_DIR}"
        "${LUA_INCLUDE_DIR}"
        "${ZMQ_INCLUDE_DIR}"
        "${ENET_INCLUDE_DIR}"
        "${GLFW_INCLUDE_DIR}"
        "${FL_INCLUDE_DIR}"
        "${AS_INCLUDE_DIR}"
)

if(APPLE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-fast-math")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ambiguous-ellipsis")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lc++ ")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreVideo")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework OpenGL")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework IOKit")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ObjC++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -headerpad_max_install_names")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch x86_64")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -arch x86_64")
endif(APPLE)

if(UNIX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffloat-store")
endif()

add_executable(snow ${SOURCE_FILES})
# add_executable(snow stub.c)
target_link_libraries(snow
        angelscript
        enet
        fltk
        glfw
        liblua
        libsnow-common
        physfs
        zeromq
        )

if(APPLE)
        set_property(TARGET snow PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/snow.app/Contents/MacOS")
endif(APPLE)

include_directories(
        ext
        "${SCOM_INCLUDE_DIR}"
)

add_executable(snowhash snowhash.cc ext/snow-ext/hash.cc ext/snow-ext/murmur3.c)
target_link_libraries(snowhash
        libsnow-common
)
set_property(TARGET snowhash PROPERTY RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
