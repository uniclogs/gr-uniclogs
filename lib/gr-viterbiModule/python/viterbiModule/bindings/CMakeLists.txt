# Copyright 2020 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Check if there is C++ code at all
########################################################################
if(NOT viterbiModule_sources)
    message(STATUS "No C++ sources... skipping python bindings")
    return()
endif(NOT viterbiModule_sources)

########################################################################
# Check for pygccxml
########################################################################
gr_python_check_module_raw("pygccxml" "import pygccxml" PYGCCXML_FOUND)

include(GrPybind)

########################################################################
# Python Bindings
########################################################################

list(APPEND viterbiModule_python_files python_bindings.cc)

gr_pybind_make_oot(viterbiModule ../../.. gr::viterbiModule "${viterbiModule_python_files}")

# copy bindings extension for use in QA test module
add_custom_command(
    TARGET viterbiModule_python
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:viterbiModule_python>
            ${PROJECT_BINARY_DIR}/test_modules/gnuradio/viterbiModule/)

install(
    TARGETS viterbiModule_python
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/viterbiModule
    COMPONENT pythonapi)
