# ----------------------------------------------------------------------------
# package information
# add el7.x86_64
execute_process (
            COMMAND         ${CMAKE_MODULE_PATH}/rpm_kernel_release.sh
            RESULT_VARIABLE RV
            OUTPUT_VARIABLE CENTOS_VERSION
            )
if (RV EQUAL 0)
set (KERNEL_RELEASE ${CENTOS_VERSION})
else ()
set (KERNEL_RELEASE "unknown-arch")
endif ()

execute_process (
            COMMAND         ${CMAKE_MODULE_PATH}/git_checkin_count.sh
            RESULT_VARIABLE RV
            OUTPUT_VARIABLE GIT_CHECKIN_COUNT
            )
if (RV EQUAL 0)
else ()
set (GIT_CHECKIN_COUNT "1")
endif ()

set (PACKAGE_NAME        "${CMAKE_PROJECT_NAME}")
set (PACKAGE_VERSION     "0.4.0.${GIT_CHECKIN_COUNT}")
set (PACKAGE_STRING      "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME     "${PACKAGE_NAME}-${PACKAGE_VERSION}")
set (PACKAGE_BUGREPORT   "https://github.com/Qihoo360/evpp/issues")
set (PACKAGE_DESCRIPTION "evpp is a modern C++ network library for developing high performance network servers in TCP/UDP/HTTP protocols.")
set (PACKAGE_URL         "http://github.com/Qihoo360/evpp")

version_numbers (
  ${PACKAGE_VERSION}
    PACKAGE_VERSION_MAJOR
    PACKAGE_VERSION_MINOR
    PACKAGE_VERSION_PATCH
)
set (PACKAGE_SOVERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")

set (CPACK_PACKAGE_CONTACT       "https://github.com/Qihoo360/evpp")
set (CPACK_PACKAGE_ARCHITECTURE  "${KERNEL_RELEASE}")
set (CPACK_PACKAGE_VERSION_MAJOR "${PACKAGE_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${PACKAGE_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${PACKAGE_VERSION_PATCH}")
set (CPACK_PACKAGE_FILE_NAME     "${PACKAGE_NAME}-${PACKAGE_VERSION}-${CPACK_PACKAGE_ARCHITECTURE}")
set (CPACK_OUTPUT_FILE_PREFIX   packages)

#message(STATUS "XXXXXXXXXXXXXXX PACKAGE_SOVERSION=" ${PACKAGE_SOVERSION})
#message(STATUS "XXXXXXXXXXXXXXX PACKAGE_NAME=" ${PACKAGE_NAME})
#message(STATUS "XXXXXXXXXXXXXXX CPACK_PACKAGE_FILE_NAME=" ${CPACK_PACKAGE_FILE_NAME})

if (UNIX)
    set (CPACK_GENERATOR "RPM;TGZ;DEB")
    set (CPACK_PACKAGING_INSTALL_PREFIX "/home/s/safe")
else (UNIX)
    set (CPACK_GENERATOR "TGZ")
endif (UNIX)

