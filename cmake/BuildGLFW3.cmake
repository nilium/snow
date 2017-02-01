# GLFW build
set(GLFW_RELEASE 3.2.1)
set(GLFW_INSTALL_DIR ${THIRDPARTY_INSTALL_DIR}/glfw3)
set(GLFW_INCLUDE_DIR ${GLFW_INSTALL_DIR}/include)
set(GLFW_LIB_DIR ${GLFW_INSTALL_DIR}/lib)

ExternalProject_Add(lib-glfw-${GLFW_RELEASE}
        GIT_REPOSITORY "https://github.com/glfw/glfw.git"
        GIT_TAG ${GLFW_RELEASE}
        UPDATE_COMMAND ""
        INSTALL_DIR "${GLFW_INSTALL_DIR}"
        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                "-DCMAKE_INSTALL_PREFIX=${GLFW_INSTALL_DIR}"
                -DGLFW_BUILD_EXAMPLES=OFF
                -DGLFW_BUILD_TESTS=OFF
                -DGLFW_BUILD_DOCS=OFF
                -DGLFW_INSTALL=ON
        )

add_library(glfw STATIC IMPORTED)
set_property(TARGET glfw PROPERTY IMPORTED_LOCATION "${GLFW_LIB_DIR}/libglfw3.a")
add_dependencies(glfw lib-glfw-${GLFW_RELEASE})
