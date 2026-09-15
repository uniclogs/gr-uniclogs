find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_UNICLOGS gnuradio-uniclogs)

FIND_PATH(
    GR_UNICLOGS_INCLUDE_DIRS
    NAMES gnuradio/uniclogs/api.h
    HINTS $ENV{UNICLOGS_DIR}/include
        ${PC_UNICLOGS_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_UNICLOGS_LIBRARIES
    NAMES gnuradio-uniclogs
    HINTS $ENV{UNICLOGS_DIR}/lib
        ${PC_UNICLOGS_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-uniclogsTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_UNICLOGS DEFAULT_MSG GR_UNICLOGS_LIBRARIES GR_UNICLOGS_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_UNICLOGS_LIBRARIES GR_UNICLOGS_INCLUDE_DIRS)
