#libsnow-common
set(SCOM_INSTALL_DIR "${THIRDPARTY_INSTALL_DIR}/libsnow-common")
set(SCOM_INCLUDE_DIR "${SCOM_INSTALL_DIR}/include")
set(SCOM_LIB_DIR "${SCOM_INSTALL_DIR}/lib")

ExternalProject_Add("lib-libsnow-common"
        GIT_REPOSITORY "https://git.spiff.io/libsnow-common.git"
        INSTALL_DIR "${SCOM_INSTALL_DIR}"
        UPDATE_COMMAND ""
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND premake4 gmake
        BUILD_COMMAND make
        INSTALL_COMMAND premake4 "--prefix=${SCOM_INSTALL_DIR}" install
        )

add_library(libsnow-common STATIC IMPORTED)
set_property(TARGET libsnow-common PROPERTY IMPORTED_LOCATION "${SCOM_LIB_DIR}/libsnow-common.a")
add_dependencies(libsnow-common lib-libsnow-common)
