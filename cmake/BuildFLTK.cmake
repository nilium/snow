# FLTK build (yep, FLTK)
set(FL_RELEASE 1.3.4)
set(FL_MD5SUM d7fcd27ab928648e1a1366dd2e273970)
set(FL_INSTALL_DIR "${THIRDPARTY_INSTALL_DIR}/fltk")
set(FL_INCLUDE_DIR "${FL_INSTALL_DIR}/include")
set(FL_LIB_DIR "${FL_INSTALL_DIR}/lib")

ExternalProject_Add(lib-fltk-${FL_RELEASE}
        URL "http://fltk.org/pub/fltk/${FL_RELEASE}/fltk-${FL_RELEASE}-1-source.tar.gz"
        URL_HASH MD5=${FL_MD5SUM}
        UPDATE_COMMAND ""
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ./configure "--prefix=${FL_INSTALL_DIR}" --disable-gl
        )

add_library(fltk STATIC IMPORTED)
set_property(TARGET fltk PROPERTY IMPORTED_LOCATION "${FL_LIB_DIR}/libfltk.a")
add_dependencies(fltk lib-fltk-${FL_RELEASE})
