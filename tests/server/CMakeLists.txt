#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 2009 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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
set(TARGET_LABEL_PREFIX "Test server ")

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4306")
endif()

function(SETUP_EXECUTABLE TEST_NAME)    # ARGN are the files in the test
  add_executable(${TEST_NAME} EXCLUDE_FROM_ALL ${ARGN})
  add_dependencies(testdeps ${TEST_NAME})
  string(TOUPPER ${TEST_NAME} UPPER_TEST_NAME)

  include_directories(
    ${CARL_SOURCE_DIR}/lib      # To be able to reach "carl_setup_once.h"
    ${CARL_BINARY_DIR}/lib      # To be able to reach "carl_config.h"
    ${CARL_BINARY_DIR}/include  # To be able to reach "carl/carl.h"
    )
  if(USE_ARES)
    include_directories(${CARES_INCLUDE_DIR})
  endif()

  target_link_libraries(${TEST_NAME} ${CARL_LIBS})

  # Test servers simply are standalone programs that do not use libcarl
  # library.  For convenience and to ease portability of these servers,
  # some source code files from the libcarl subdirectory are also used
  # to build the servers.  In order to achieve proper linkage of these
  # files on Win32 targets it is necessary to build the test servers
  # with CARL_STATICLIB defined, independently of how libcarl is built.
  if(BUILD_SHARED_LIBS)
    set_target_properties(${TEST_NAME} PROPERTIES
      COMPILE_DEFINITIONS CARL_STATICLIB)       # ${UPPER_TEST_NAME}
  endif()
  set_target_properties(${TEST_NAME} PROPERTIES
    PROJECT_LABEL "${TARGET_LABEL_PREFIX}${TEST_NAME}")
endfunction()


transform_makefile_inc("Makefile.inc"
  "${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake")
include(${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake)

foreach(EXECUTABLE_NAME ${noinst_PROGRAMS})
  setup_executable(${EXECUTABLE_NAME} ${${EXECUTABLE_NAME}_SOURCES})
endforeach()


# SET(useful
# getpart.c getpart.h
# ${CARL_SOURCE_DIR}/lib/strequal.c
# ${CARL_SOURCE_DIR}/lib/base64.c
# ${CARL_SOURCE_DIR}/lib/mprintf.c
# ${CARL_SOURCE_DIR}/lib/memdebug.c
# ${CARL_SOURCE_DIR}/lib/timeval.c
# )

# SETUP_EXECUTABLE(sws sws.c util.c util.h ${useful})
# SETUP_EXECUTABLE(resolve resolve.c util.c util.h ${useful})
# SETUP_EXECUTABLE(sockfilt sockfilt.c util.c util.h ${useful} ${CARL_SOURCE_DIR}/lib/inet_pton.c)
# SETUP_EXECUTABLE(getpart testpart.c ${useful})
# SETUP_EXECUTABLE(tftpd tftpd.c util.c util.h ${useful} tftp.h)
