# ZeroMQ build
set(ZMQ_RELEASE 4.2.1)
set(ZMQ_INSTALL_DIR "${THIRDPARTY_INSTALL_DIR}/zeromq")
set(ZMQ_INCLUDE_DIR "${ZMQ_INSTALL_DIR}/include")
set(ZMQ_LIB_DIR "${ZMQ_INSTALL_DIR}/lib")

ExternalProject_Add(lib-zeromq-${ZMQ_RELEASE}
        GIT_REPOSITORY "https://github.com/zeromq/libzmq.git"
        GIT_TAG v${ZMQ_RELEASE}
        INSTALL_DIR "${ZMQ_INSTALL_DIR}"
        UPDATE_COMMAND ""

        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                "-DCMAKE_INSTALL_PREFIX=${ZMQ_INSTALL_DIR}"
                -DBUILD_SHARED_LIBS=OFF
                -DBUILD_STATIC_LIBS=ON
                -DBUILD_PACKAGING=OFF
                -DBUILD_TESTING=OFF
                -DBUILD_NC_TESTS=OFF
                -DZMQ_BUILD_FRAMEWORK=OFF
                -BUILD_CONFIG_TESTS=OFF
                -DINSTALL_HEADERS=ON
                -DWITH_DOC=OFF
        )

add_library(zeromq STATIC IMPORTED)
set_property(TARGET zeromq PROPERTY IMPORTED_LOCATION "${ZMQ_LIB_DIR}/libzmq-static.a")
add_dependencies(zeromq lib-zeromq-${ZMQ_RELEASE})
