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

# TODO build a special libcarlu library for unittests.
return()

set(UT_SRC
  unit1300.c
  unit1301.c
  unit1302.c
  unit1303.c
  unit1304.c
  unit1305.c
  unit1307.c
  unit1308.c
  unit1309.c
  unit1330.c
# Broken link on Linux
#  unit1394.c
  unit1395.c
  unit1396.c
  unit1397.c
  unit1398.c
  unit1600.c
  unit1601.c
  unit1603.c
# Broken link on Linux
#  unit1604.c
  unit1620.c
  unit1655.c
  )

set(UT_COMMON_FILES ../libtest/first.c ../libtest/test.h carlcheck.h)
include_directories(
  ${CARL_SOURCE_DIR}/lib          # To be able to reach "carl_setup_once.h"
  ${CARL_SOURCE_DIR}/tests/libtest
  ${CARL_SOURCE_DIR}/src
  ${CARL_BINARY_DIR}/lib          # To be able to reach "carl_config.h"
  ${CARL_BINARY_DIR}/include      # To be able to reach "carl/carl.h"
)

foreach(_testfile ${UT_SRC})

  get_filename_component(_testname ${_testfile} NAME_WE)
  add_executable(${_testname} EXCLUDE_FROM_ALL ${_testfile} ${UT_COMMON_FILES})
  #add_dependencies(testdeps ${_testname})
  target_link_libraries(${_testname} libcarl ${CARL_LIBS})
  set_target_properties(${_testname}
      PROPERTIES COMPILE_DEFINITIONS "UNITTESTS")
endforeach()
