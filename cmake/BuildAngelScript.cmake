# AngelScript build
set(AS_RELEASE 2.29.2)
set(AS_SHASUM a983ea7e429f18672f9b6f927c98f580679ce19c)

ExternalProject_Add(lib-angelscript-${AS_RELEASE}
        URL "http://www.angelcode.com/angelscript/sdk/files/angelscript_${AS_RELEASE}.zip"
        URL_HASH SHA1=${AS_SHASUM}
        SOURCE_SUBDIR angelscript/projects/cmake
        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        )

ExternalProject_Get_Property(lib-angelscript-${AS_RELEASE} source_dir)
set(AS_INCLUDE_DIR "${source_dir}/angelscript/include")
set(AS_LIB_DIR "${source_dir}/angelscript/lib")

add_library(angelscript STATIC IMPORTED)
set_property(TARGET angelscript PROPERTY IMPORTED_LOCATION "${AS_LIB_DIR}/libAngelscript.a")
add_dependencies(angelscript lib-angelscript-${AS_RELEASE})
