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
set(LIB_NAME libcarl)

if(BUILD_SHARED_LIBS)
  set(CARL_STATICLIB NO)
else()
  set(CARL_STATICLIB YES)
endif()

# Use:
# * CARL_STATICLIB
configure_file(carl_config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/carl_config.h)

transform_makefile_inc("Makefile.inc" "${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake")
include(${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake)

list(APPEND HHEADERS
  ${CMAKE_CURRENT_BINARY_DIR}/carl_config.h
  )

if(WIN32)
  list(APPEND CSOURCES libcarl.rc)
endif()

# SET(CSOURCES
# #  memdebug.c -not used
# # nwlib.c - Not used
# # strtok.c - specify later
# # strtoofft.c - specify later
# )

# #OPTION(CARL_MALLOC_DEBUG "Debug mallocs in Curl" OFF)
# MARK_AS_ADVANCED(CARL_MALLOC_DEBUG)
# IF(CARL_MALLOC_DEBUG)
# SET(CSOURCES ${CSOURCES}
# memdebug.c
# )
# ENDIF(CARL_MALLOC_DEBUG)

# # only build compat strtoofft if we need to
# IF(NOT HAVE_STRTOLL AND NOT HAVE__STRTOI64)
# SET(CSOURCES ${CSOURCES}
# strtoofft.c
# )
# ENDIF(NOT HAVE_STRTOLL AND NOT HAVE__STRTOI64)


# The rest of the build

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
if(USE_ARES)
  include_directories(${CARES_INCLUDE_DIR})
endif()

add_library(
  ${LIB_NAME}
  ${HHEADERS} ${CSOURCES}
  )

add_library(
  ${PROJECT_NAME}::${LIB_NAME}
  ALIAS ${LIB_NAME}
  )

if(NOT BUILD_SHARED_LIBS)
    set_target_properties(${LIB_NAME} PROPERTIES INTERFACE_COMPILE_DEFINITIONS CARL_STATICLIB)
endif()

target_link_libraries(${LIB_NAME} ${CARL_LIBS})

if(WIN32)
  add_definitions(-D_USRDLL)
endif()

set_target_properties(${LIB_NAME} PROPERTIES COMPILE_DEFINITIONS BUILDING_LIBCARL)

if(HIDES_CARL_PRIVATE_SYMBOLS)
  set_property(TARGET ${LIB_NAME} APPEND PROPERTY COMPILE_DEFINITIONS "CARL_HIDDEN_SYMBOLS")
  set_property(TARGET ${LIB_NAME} APPEND PROPERTY COMPILE_FLAGS ${CARL_CFLAG_SYMBOLS_HIDE})
endif()

# Remove the "lib" prefix since the library is already named "libcarl".
set_target_properties(${LIB_NAME} PROPERTIES PREFIX "")
set_target_properties(${LIB_NAME} PROPERTIES IMPORT_PREFIX "")

if(CARL_HAS_LTO)
  set_target_properties(${LIB_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()

if(WIN32)
  if(BUILD_SHARED_LIBS)
    # Add "_imp" as a suffix before the extension to avoid conflicting with the statically linked "libcarl.lib"
    set_target_properties(${LIB_NAME} PROPERTIES IMPORT_SUFFIX "_imp.lib")
  endif()
endif()

target_include_directories(${LIB_NAME} INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CARL_SOURCE_DIR}/include>)

install(TARGETS ${LIB_NAME}
  EXPORT ${TARGETS_EXPORT_NAME}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

export(TARGETS ${LIB_NAME}
       APPEND FILE ${PROJECT_BINARY_DIR}/libcarl-target.cmake
       NAMESPACE ${PROJECT_NAME}::
)
