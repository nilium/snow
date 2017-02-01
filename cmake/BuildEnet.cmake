# enet build
set(ENET_RELEASE 1.3)
set(ENET_INSTALL_DIR ${THIRDPARTY_INSTALL_DIR}/enet)
set(ENET_INCLUDE_DIR ${ENET_INSTALL_DIR}/include)
set(ENET_LIB_DIR ${ENET_INSTALL_DIR}/lib)

ExternalProject_Add(lib-enet-${ENET_RELEASE}
        GIT_REPOSITORY "https://github.com/lsalzman/enet.git"
        GIT_TAG f46fee0acc8e243b2b6910b09693f93c3aad775f
        INSTALL_DIR "${ENET_INSTALL_DIR}"
        BUILD_IN_SOURCE 1
        UPDATE_COMMAND ""
        CONFIGURE_COMMAND autoreconf -vfi && ./configure "--prefix=${ENET_INSTALL_DIR}"
        )

add_library(enet STATIC IMPORTED)
set_property(TARGET enet PROPERTY IMPORTED_LOCATION "${ENET_LIB_DIR}/libenet.a")
add_dependencies(enet lib-enet-${ENET_RELEASE})
