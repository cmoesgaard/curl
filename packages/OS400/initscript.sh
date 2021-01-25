#!/bin/sh
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


setenv()

{
        #       Define and export.

        eval ${1}="${2}"
        export ${1}
}


case "${SCRIPTDIR}" in
/*)     ;;
*)      SCRIPTDIR="`pwd`/${SCRIPTDIR}"
esac

while true
do      case "${SCRIPTDIR}" in
        */.)    SCRIPTDIR="${SCRIPTDIR%/.}";;
        *)      break;;
        esac
done

#  The script directory is supposed to be in $TOPDIR/packages/os400.

TOPDIR=`dirname "${SCRIPTDIR}"`
TOPDIR=`dirname "${TOPDIR}"`
export SCRIPTDIR TOPDIR

#  Extract the SONAME from the library makefile.

SONAME=`sed -e '/^VERSIONINFO=/!d' -e 's/^.* \([0-9]*\):.*$/\1/' -e 'q' \
                                                < "${TOPDIR}/lib/Makefile.am"`
export SONAME


################################################################################
#
#                       Tunable configuration parameters.
#
################################################################################

setenv TARGETLIB        'CARL'                  # Target OS/400 program library.
setenv STATBNDDIR       'CARL_A'                # Static binding directory.
setenv DYNBNDDIR        'CARL'                  # Dynamic binding directory.
setenv SRVPGM           "CARL.${SONAME}"        # Service program.
setenv TGTCCSID         '500'                   # Target CCSID of objects.
setenv DEBUG            '*ALL'                  # Debug level.
setenv OPTIMIZE         '10'                    # Optimisation level
setenv OUTPUT           '*NONE'                 # Compilation output option.
setenv TGTRLS           'V6R1M0'                # Target OS release.
setenv IFSDIR           '/carl'                 # Installation IFS directory.

#       Define ZLIB availability and locations.

setenv WITH_ZLIB        0                       # Define to 1 to enable.
setenv ZLIB_INCLUDE     '/zlib/include'         # ZLIB include IFS directory.
setenv ZLIB_LIB         'ZLIB'                  # ZLIB library.
setenv ZLIB_BNDDIR      'ZLIB_A'                # ZLIB binding directory.

#       Define LIBSSH2 availability and locations.

setenv WITH_LIBSSH2     0                       # Define to 1 to enable.
setenv LIBSSH2_INCLUDE  '/libssh2/include'      # LIBSSH2 include IFS directory.
setenv LIBSSH2_LIB      'LIBSSH2'               # LIBSSH2 library.
setenv LIBSSH2_BNDDIR   'LIBSSH2_A'             # LIBSSH2 binding directory.


################################################################################

#       Need to get the version definitions.

LIBCARL_VERSION=`grep '^#define  *LIBCARL_VERSION '                     \
                        "${TOPDIR}/include/carl/carlver.h"              |
                sed 's/.*"\(.*\)".*/\1/'`
LIBCARL_VERSION_MAJOR=`grep '^#define  *LIBCARL_VERSION_MAJOR '         \
                        "${TOPDIR}/include/carl/carlver.h"              |
                sed 's/^#define  *LIBCARL_VERSION_MAJOR  *\([^ ]*\).*/\1/'`
LIBCARL_VERSION_MINOR=`grep '^#define  *LIBCARL_VERSION_MINOR '         \
                        "${TOPDIR}/include/carl/carlver.h"              |
                sed 's/^#define  *LIBCARL_VERSION_MINOR  *\([^ ]*\).*/\1/'`
LIBCARL_VERSION_PATCH=`grep '^#define  *LIBCARL_VERSION_PATCH '         \
                        "${TOPDIR}/include/carl/carlver.h"              |
                sed 's/^#define  *LIBCARL_VERSION_PATCH  *\([^ ]*\).*/\1/'`
LIBCARL_VERSION_NUM=`grep '^#define  *LIBCARL_VERSION_NUM '             \
                        "${TOPDIR}/include/carl/carlver.h"              |
                sed 's/^#define  *LIBCARL_VERSION_NUM  *0x\([^ ]*\).*/\1/'`
LIBCARL_TIMESTAMP=`grep '^#define  *LIBCARL_TIMESTAMP '                 \
                        "${TOPDIR}/include/carl/carlver.h"              |
                sed 's/.*"\(.*\)".*/\1/'`
export LIBCARL_VERSION
export LIBCARL_VERSION_MAJOR LIBCARL_VERSION_MINOR LIBCARL_VERSION_PATCH
export LIBCARL_VERSION_NUM LIBCARL_TIMESTAMP

################################################################################
#
#                       OS/400 specific definitions.
#
################################################################################

LIBIFSNAME="/QSYS.LIB/${TARGETLIB}.LIB"


################################################################################
#
#                               Procedures.
#
################################################################################

#       action_needed dest [src]
#
#       dest is an object to build
#       if specified, src is an object on which dest depends.
#
#       exit 0 (succeeds) if some action has to be taken, else 1.

action_needed()

{
        [ ! -e "${1}" ] && return 0
        [ "${2}" ] || return 1
        [ "${1}" -ot "${2}" ] && return 0
        return 1
}


#       canonicalize_path path
#
#       Return canonicalized path as:
#       - Absolute
#       - No . or .. component.

canonicalize_path()

{
        if expr "${1}" : '^/' > /dev/null
        then    P="${1}"
        else    P="`pwd`/${1}"
        fi

        R=
        IFSSAVE="${IFS}"
        IFS="/"

        for C in ${P}
        do      IFS="${IFSSAVE}"
                case "${C}" in
                .)      ;;
                ..)     R=`expr "${R}" : '^\(.*/\)..*'`
                        ;;
                ?*)     R="${R}${C}/"
                        ;;
                *)      ;;
                esac
        done

        IFS="${IFSSAVE}"
        echo "/`expr "${R}" : '^\(.*\)/'`"
}


#       make_module module_name source_name [additional_definitions]
#
#       Compile source name into ASCII module if needed.
#       As side effect, append the module name to variable MODULES.
#       Set LINK to "YES" if the module has been compiled.

make_module()

{
        MODULES="${MODULES} ${1}"
        MODIFSNAME="${LIBIFSNAME}/${1}.MODULE"
        action_needed "${MODIFSNAME}" "${2}" || return 0;
        SRCDIR=`dirname \`canonicalize_path "${2}"\``

        #       #pragma convert has to be in the source file itself, i.e.
        #               putting it in an include file makes it only active
        #               for that include file.
        #       Thus we build a temporary file with the pragma prepended to
        #               the source file and we compile that themporary file.

        echo "#line 1 \"${2}\"" > __tmpsrcf.c
        echo "#pragma convert(819)" >> __tmpsrcf.c
        echo "#line 1" >> __tmpsrcf.c
        cat "${2}" >> __tmpsrcf.c
        CMD="CRTCMOD MODULE(${TARGETLIB}/${1}) SRCSTMF('__tmpsrcf.c')"
#       CMD="${CMD} SYSIFCOPT(*IFS64IO) OPTION(*INCDIRFIRST *SHOWINC *SHOWSYS)"
        CMD="${CMD} SYSIFCOPT(*IFS64IO) OPTION(*INCDIRFIRST)"
        CMD="${CMD} LOCALETYPE(*LOCALE) FLAG(10)"
        CMD="${CMD} INCDIR('/qibm/proddata/qadrt/include'"
        CMD="${CMD} '${TOPDIR}/include/carl' '${TOPDIR}/include' '${SRCDIR}'"
        CMD="${CMD} '${TOPDIR}/packages/OS400'"

        if [ "${WITH_ZLIB}" != "0" ]
        then    CMD="${CMD} '${ZLIB_INCLUDE}'"
        fi

        if [ "${WITH_LIBSSH2}" != "0" ]
        then    CMD="${CMD} '${LIBSSH2_INCLUDE}'"
        fi

        CMD="${CMD} ${INCLUDES})"
        CMD="${CMD} TGTCCSID(${TGTCCSID}) TGTRLS(${TGTRLS})"
        CMD="${CMD} OUTPUT(${OUTPUT})"
        CMD="${CMD} OPTIMIZE(${OPTIMIZE})"
        CMD="${CMD} DBGVIEW(${DEBUG})"

        DEFINES="${3} BUILDING_LIBCARL"

        if [ "${WITH_ZLIB}" != "0" ]
        then    DEFINES="${DEFINES} HAVE_LIBZ HAVE_ZLIB_H"
        fi

        if [ "${WITH_LIBSSH2}" != "0" ]
        then    DEFINES="${DEFINES} USE_LIBSSH2 HAVE_LIBSSH2_H"
        fi

        if [ "${DEFINES}" ]
        then    CMD="${CMD} DEFINE(${DEFINES})"
        fi

        system "${CMD}"
        rm -f __tmpsrcf.c
        LINK=YES
}


#       Determine DB2 object name from IFS name.

db2_name()

{
        if [ "${2}" = 'nomangle' ]
        then    basename "${1}"                                         |
                tr 'a-z-' 'A-Z_'                                        |
                sed -e 's/\..*//'                                       \
                    -e 's/^\(.\).*\(.........\)$/\1\2/'
        else    basename "${1}"                                         |
                tr 'a-z-' 'A-Z_'                                        |
                sed -e 's/\..*//'                                       \
                    -e 's/^CARL_*/C/'                                   \
                    -e 's/^\(.\).*\(.........\)$/\1\2/'
        fi
}


#       Copy IFS file replacing version info.

versioned_copy()

{
        sed -e "s/@LIBCARL_VERSION@/${LIBCARL_VERSION}/g"               \
            -e "s/@LIBCARL_VERSION_MAJOR@/${LIBCARL_VERSION_MAJOR}/g"   \
            -e "s/@LIBCARL_VERSION_MINOR@/${LIBCARL_VERSION_MINOR}/g"   \
            -e "s/@LIBCARL_VERSION_PATCH@/${LIBCARL_VERSION_PATCH}/g"   \
            -e "s/@LIBCARL_VERSION_NUM@/${LIBCARL_VERSION_NUM}/g"       \
            -e "s/@LIBCARL_TIMESTAMP@/${LIBCARL_TIMESTAMP}/g"           \
                < "${1}" > "${2}"
}
