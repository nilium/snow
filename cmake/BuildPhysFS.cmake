# PhysicsFS build
set(PHYSFS_INSTALL_DIR ${THIRDPARTY_INSTALL_DIR}/physfs)
set(PHYSFS_INCLUDE_DIR "${PHYSFS_INSTALL_DIR}/include")
set(PHYSFS_LIB_DIR "${PHYSFS_INSTALL_DIR}/lib")
set(PHYSFS_RELEASE_TAG default)
set(PHYSFS_RELEASE 2.1)

ExternalProject_Add(lib-physfs-${PHYSFS_RELEASE}
        HG_REPOSITORY https://hg.icculus.org/icculus/physfs/
        HG_TAG ${PHYSFS_RELEASE_TAG}
        INSTALL_DIR "${PHYSFS_INSTALL_DIR}"
        UPDATE_COMMAND ""
        CMAKE_ARGS "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
                "-DCMAKE_INSTALL_PREFIX=${PHYSFS_INSTALL_DIR}"
                -DPHYSFS_ARCHIVE_ZIP=ON
                -DPHYSFS_ARCHIVE_7Z=OFF
                -DPHYSFS_ARCHIVE_GRP=OFF
                -DPHYSFS_ARCHIVE_WAD=OFF
                -DPHYSFS_ARCHIVE_HOG=OFF
                -DPHYSFS_ARCHIVE_MVL=OFF
                -DPHYSFS_ARCHIVE_QPAK=OFF
                -DPHYSFS_HAVE_CDROM_SUPPORT=OFF
                -DPHYSFS_HAVE_THREAD_SUPPORT=ON
                -DPHYSFS_INTERNAL_ZLIB=OFF
                -DPHYSFS_BUILD_STATIC=ON
                -DPHYSFS_BUILD_SHARED=OFF
                -DPHYSFS_BUILD_WX_TEST=OFF
                -DPHYSFS_BUILD_TEST=OFF
                -Wno-dev
        )

find_package(ZLIB REQUIRED)

add_library(physfs STATIC IMPORTED)
set_property(TARGET physfs PROPERTY IMPORTED_LOCATION "${PHYSFS_LIB_DIR}/libphysfs.a")
add_dependencies(physfs lib-physfs-${PHYSFS_RELEASE})
