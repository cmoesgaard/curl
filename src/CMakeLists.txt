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
set(EXE_NAME carl)

if(USE_MANUAL)
  # Use the C locale to ensure that only ASCII characters appear in the
  # embedded text. NROFF and MANOPT are set in the parent CMakeLists.txt
  add_custom_command(
    OUTPUT tool_hugehelp.c
    COMMAND ${CMAKE_COMMAND} -E echo "#include \"tool_setup.h\"" > tool_hugehelp.c
    COMMAND ${CMAKE_COMMAND} -E echo "#ifndef HAVE_LIBZ" >> tool_hugehelp.c
    COMMAND env LC_ALL=C "${NROFF}" ${NROFF_MANOPT}
            "${CARL_BINARY_DIR}/docs/carl.1" |
            "${PERL_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/mkhelp.pl" >> tool_hugehelp.c
    COMMAND ${CMAKE_COMMAND} -E echo "#else" >> tool_hugehelp.c
    COMMAND env LC_ALL=C "${NROFF}" ${NROFF_MANOPT}
            "${CARL_BINARY_DIR}/docs/carl.1" |
            "${PERL_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/mkhelp.pl" -c >> tool_hugehelp.c
    COMMAND ${CMAKE_COMMAND} -E echo "#endif /* HAVE_LIBZ */" >> tool_hugehelp.c
    DEPENDS
      generate-carl.1
      "${CARL_BINARY_DIR}/docs/carl.1"
      "${CMAKE_CURRENT_SOURCE_DIR}/mkhelp.pl"
      "${CMAKE_CURRENT_SOURCE_DIR}/tool_hugehelp.h"
    VERBATIM)
else()
  add_custom_command(
    OUTPUT tool_hugehelp.c
    COMMAND ${CMAKE_COMMAND} -E echo "/* built-in manual is disabled, blank function */" > tool_hugehelp.c
    COMMAND ${CMAKE_COMMAND} -E echo "#include \"tool_hugehelp.h\"" >> tool_hugehelp.c
    COMMAND ${CMAKE_COMMAND} -E echo "void hugehelp(void) {}" >> tool_hugehelp.c
    DEPENDS
      "${CMAKE_CURRENT_SOURCE_DIR}/tool_hugehelp.h"
    VERBATIM)

endif()

transform_makefile_inc("Makefile.inc" "${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake")
include(${CMAKE_CURRENT_BINARY_DIR}/Makefile.inc.cmake)

if(MSVC)
  list(APPEND CARL_FILES carl.rc)
endif()

# CARL_FILES comes from Makefile.inc
add_executable(
  ${EXE_NAME}
  ${CARL_FILES}
  )

add_executable(
  ${PROJECT_NAME}::${EXE_NAME}
  ALIAS ${EXE_NAME}
  )

if(CARL_HAS_LTO)
  set_target_properties(${EXE_NAME} PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
endif()

if(ENABLE_UNICODE AND MINGW)
  target_link_libraries(${EXE_NAME} -municode)
  # GCC doesn't know about wmain
  set_source_files_properties(tool_main.c PROPERTIES COMPILE_FLAGS "-Wno-missing-prototypes -Wno-missing-declarations")
endif()

source_group("carlX source files" FILES ${CARLX_CFILES})
source_group("carl source files" FILES ${CARL_CFILES})
source_group("carl header files" FILES ${CARL_HFILES})

include_directories(
  ${CARL_SOURCE_DIR}/lib        # To be able to reach "carl_setup_once.h"
  ${CARL_BINARY_DIR}/lib        # To be able to reach "carl_config.h"
  ${CARL_BINARY_DIR}/include    # To be able to reach "carl/carl.h"
  # This is needed as tool_hugehelp.c is generated in the binary dir
  ${CARL_SOURCE_DIR}/src        # To be able to reach "tool_hugehelp.h"
  )

#Build carl executable
target_link_libraries(${EXE_NAME} libcarl ${CARL_LIBS})

################################################################################

#SET_TARGET_PROPERTIES(${EXE_NAME} ARCHIVE_OUTPUT_DIRECTORY "blah blah blah")
#SET_TARGET_PROPERTIES(${EXE_NAME} RUNTIME_OUTPUT_DIRECTORY "blah blah blah")
#SET_TARGET_PROPERTIES(${EXE_NAME} LIBRARY_OUTPUT_DIRECTORY "blah blah blah")

#INCLUDE(ModuleInstall OPTIONAL)

install(TARGETS ${EXE_NAME} EXPORT ${TARGETS_EXPORT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
export(TARGETS ${EXE_NAME}
       APPEND FILE ${PROJECT_BINARY_DIR}/carl-target.cmake
       NAMESPACE ${PROJECT_NAME}::
)
