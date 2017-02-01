# Lua build
if(APPLE)
        set(LUA_PLAT macosx)
elseif(WIN32)
        set(LUA_PLAT mingw)
elseif(UNIX)
        set(LUA_PLAT generic)
endif()

set(LUA_RELEASE 5.3.3)
set(LUA_SHASUM a0341bc3d1415b814cc738b2ec01ae56045d64ef)
set(LUA_INSTALL_DIR "${THIRDPARTY_INSTALL_DIR}/lua")
set(LUA_INCLUDE_DIR "${LUA_INSTALL_DIR}/include")
set(LUA_LIB_DIR "${LUA_INSTALL_DIR}/lib")

ExternalProject_Add(lib-lua-${LUA_RELEASE}
        URL "https://www.lua.org/ftp/lua-${LUA_RELEASE}.tar.gz"
        URL_HASH SHA1=${LUA_SHASUM}
        BUILD_IN_SOURCE 1
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make ${LUA_PLAT}
        INSTALL_COMMAND make install "INSTALL_TOP=${LUA_INSTALL_DIR}"
        INSTALL_DIR "${LUA_INSTALL_DIR}"
        )

add_library(liblua STATIC IMPORTED)
set_property(TARGET liblua PROPERTY IMPORTED_LOCATION "${LUA_LIB_DIR}/liblua.a")
add_dependencies(liblua lib-lua-${LUA_RELEASE})
