find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_VITERBIMODULE gnuradio-viterbiModule)

FIND_PATH(
    GR_VITERBIMODULE_INCLUDE_DIRS
    NAMES gnuradio/viterbiModule/api.h
    HINTS $ENV{VITERBIMODULE_DIR}/include
        ${PC_VITERBIMODULE_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_VITERBIMODULE_LIBRARIES
    NAMES gnuradio-viterbiModule
    HINTS $ENV{VITERBIMODULE_DIR}/lib
        ${PC_VITERBIMODULE_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-viterbiModuleTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_VITERBIMODULE DEFAULT_MSG GR_VITERBIMODULE_LIBRARIES GR_VITERBIMODULE_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_VITERBIMODULE_LIBRARIES GR_VITERBIMODULE_INCLUDE_DIRS)
