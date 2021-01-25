#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at https://carl.se/docs/copyright.html.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the COPYING file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
###########################################################################
#File defines convenience macros for available feature testing

# This macro checks if the symbol exists in the library and if it
# does, it prepends library to the list.  It is intended to be called
# multiple times with a sequence of possibly dependent libraries in
# order of least-to-most-dependent.  Some libraries depend on others
# to link correctly.
macro(check_library_exists_concat LIBRARY SYMBOL VARIABLE)
  check_library_exists("${LIBRARY};${CARL_LIBS}" ${SYMBOL} "${CMAKE_LIBRARY_PATH}"
    ${VARIABLE})
  if(${VARIABLE})
    set(CARL_LIBS ${LIBRARY} ${CARL_LIBS})
  endif()
endmacro()

# Check if header file exists and add it to the list.
# This macro is intended to be called multiple times with a sequence of
# possibly dependent header files.  Some headers depend on others to be
# compiled correctly.
macro(check_include_file_concat FILE VARIABLE)
  check_include_files("${CARL_INCLUDES};${FILE}" ${VARIABLE})
  if(${VARIABLE})
    set(CARL_INCLUDES ${CARL_INCLUDES} ${FILE})
    set(CARL_TEST_DEFINES "${CARL_TEST_DEFINES} -D${VARIABLE}")
  endif()
endmacro()

# For other carl specific tests, use this macro.
macro(carl_internal_test CARL_TEST)
  if(NOT DEFINED "${CARL_TEST}")
    set(MACRO_CHECK_FUNCTION_DEFINITIONS
      "-D${CARL_TEST} ${CARL_TEST_DEFINES} ${CMAKE_REQUIRED_FLAGS}")
    if(CMAKE_REQUIRED_LIBRARIES)
      set(CARL_TEST_ADD_LIBRARIES
        "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    endif()

    message(STATUS "Performing Curl Test ${CARL_TEST}")
    try_compile(${CARL_TEST}
      ${CMAKE_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/CMake/CurlTests.c
      CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING=${MACRO_CHECK_FUNCTION_DEFINITIONS}
      "${CARL_TEST_ADD_LIBRARIES}"
      OUTPUT_VARIABLE OUTPUT)
    if(${CARL_TEST})
      set(${CARL_TEST} 1 CACHE INTERNAL "Curl test ${FUNCTION}")
      message(STATUS "Performing Curl Test ${CARL_TEST} - Success")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
        "Performing Curl Test ${CARL_TEST} passed with the following output:\n"
        "${OUTPUT}\n")
    else()
      message(STATUS "Performing Curl Test ${CARL_TEST} - Failed")
      set(${CARL_TEST} "" CACHE INTERNAL "Curl test ${FUNCTION}")
      file(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
        "Performing Curl Test ${CARL_TEST} failed with the following output:\n"
        "${OUTPUT}\n")
    endif()
  endif()
endmacro()

macro(carl_nroff_check)
  find_program(NROFF NAMES gnroff nroff)
  if(NROFF)
    # Need a way to write to stdin, this will do
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/nroff-input.txt" "test")
    # Tests for a valid nroff option to generate a manpage
    foreach(_MANOPT "-man" "-mandoc")
      execute_process(COMMAND "${NROFF}" ${_MANOPT}
        OUTPUT_VARIABLE NROFF_MANOPT_OUTPUT
        INPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/nroff-input.txt"
        ERROR_QUIET)
      # Save the option if it was valid
      if(NROFF_MANOPT_OUTPUT)
        message("Found *nroff option: -- ${_MANOPT}")
        set(NROFF_MANOPT ${_MANOPT})
        set(NROFF_USEFUL ON)
        break()
      endif()
    endforeach()
    # No need for the temporary file
    file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/nroff-input.txt")
    if(NOT NROFF_USEFUL)
      message(WARNING "Found no *nroff option to get plaintext from man pages")
    endif()
  else()
    message(WARNING "Found no *nroff program")
  endif()
endmacro()

macro(optional_dependency DEPENDENCY)
  set(CARL_${DEPENDENCY} AUTO CACHE STRING "Build carl with ${DEPENDENCY} support (AUTO, ON or OFF)")
  set_property(CACHE CARL_${DEPENDENCY} PROPERTY STRINGS AUTO ON OFF)

  if(CARL_${DEPENDENCY} STREQUAL AUTO)
    find_package(${DEPENDENCY})
  elseif(CARL_${DEPENDENCY})
    find_package(${DEPENDENCY} REQUIRED)
  endif()
endmacro()
