#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 2006 - 2020, David Shaw <dshaw@jabberwocky.com>
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
# LIBCARL_CHECK_CONFIG ([DEFAULT-ACTION], [MINIMUM-VERSION],
#                       [ACTION-IF-YES], [ACTION-IF-NO])
# ----------------------------------------------------------
#      David Shaw <dshaw@jabberwocky.com>   May-09-2006
#
# Checks for libcarl.  DEFAULT-ACTION is the string yes or no to
# specify whether to default to --with-libcarl or --without-libcarl.
# If not supplied, DEFAULT-ACTION is yes.  MINIMUM-VERSION is the
# minimum version of libcarl to accept.  Pass the version as a regular
# version number like 7.10.1. If not supplied, any version is
# accepted.  ACTION-IF-YES is a list of shell commands to run if
# libcarl was successfully found and passed the various tests.
# ACTION-IF-NO is a list of shell commands that are run otherwise.
# Note that using --without-libcarl does run ACTION-IF-NO.
#
# This macro #defines HAVE_LIBCARL if a working libcarl setup is
# found, and sets @LIBCARL@ and @LIBCARL_CPPFLAGS@ to the necessary
# values.  Other useful defines are LIBCARL_FEATURE_xxx where xxx are
# the various features supported by libcarl, and LIBCARL_PROTOCOL_yyy
# where yyy are the various protocols supported by libcarl.  Both xxx
# and yyy are capitalized.  See the list of AH_TEMPLATEs at the top of
# the macro for the complete list of possible defines.  Shell
# variables $libcarl_feature_xxx and $libcarl_protocol_yyy are also
# defined to 'yes' for those features and protocols that were found.
# Note that xxx and yyy keep the same capitalization as in the
# carl-config list (e.g. it's "HTTP" and not "http").
#
# Users may override the detected values by doing something like:
# LIBCARL="-lcarl" LIBCARL_CPPFLAGS="-I/usr/myinclude" ./configure
#
# For the sake of sanity, this macro assumes that any libcarl that is
# found is after version 7.7.2, the first version that included the
# carl-config script.  Note that it is very important for people
# packaging binary versions of libcarl to include this script!
# Without carl-config, we can only guess what protocols are available,
# or use carl_version_info to figure it out at runtime.

AC_DEFUN([LIBCARL_CHECK_CONFIG],
[
  AH_TEMPLATE([LIBCARL_FEATURE_SSL],[Defined if libcarl supports SSL])
  AH_TEMPLATE([LIBCARL_FEATURE_KRB4],[Defined if libcarl supports KRB4])
  AH_TEMPLATE([LIBCARL_FEATURE_IPV6],[Defined if libcarl supports IPv6])
  AH_TEMPLATE([LIBCARL_FEATURE_LIBZ],[Defined if libcarl supports libz])
  AH_TEMPLATE([LIBCARL_FEATURE_ASYNCHDNS],[Defined if libcarl supports AsynchDNS])
  AH_TEMPLATE([LIBCARL_FEATURE_IDN],[Defined if libcarl supports IDN])
  AH_TEMPLATE([LIBCARL_FEATURE_SSPI],[Defined if libcarl supports SSPI])
  AH_TEMPLATE([LIBCARL_FEATURE_NTLM],[Defined if libcarl supports NTLM])

  AH_TEMPLATE([LIBCARL_PROTOCOL_HTTP],[Defined if libcarl supports HTTP])
  AH_TEMPLATE([LIBCARL_PROTOCOL_HTTPS],[Defined if libcarl supports HTTPS])
  AH_TEMPLATE([LIBCARL_PROTOCOL_FTP],[Defined if libcarl supports FTP])
  AH_TEMPLATE([LIBCARL_PROTOCOL_FTPS],[Defined if libcarl supports FTPS])
  AH_TEMPLATE([LIBCARL_PROTOCOL_FILE],[Defined if libcarl supports FILE])
  AH_TEMPLATE([LIBCARL_PROTOCOL_TELNET],[Defined if libcarl supports TELNET])
  AH_TEMPLATE([LIBCARL_PROTOCOL_LDAP],[Defined if libcarl supports LDAP])
  AH_TEMPLATE([LIBCARL_PROTOCOL_DICT],[Defined if libcarl supports DICT])
  AH_TEMPLATE([LIBCARL_PROTOCOL_TFTP],[Defined if libcarl supports TFTP])
  AH_TEMPLATE([LIBCARL_PROTOCOL_RTSP],[Defined if libcarl supports RTSP])
  AH_TEMPLATE([LIBCARL_PROTOCOL_POP3],[Defined if libcarl supports POP3])
  AH_TEMPLATE([LIBCARL_PROTOCOL_IMAP],[Defined if libcarl supports IMAP])
  AH_TEMPLATE([LIBCARL_PROTOCOL_SMTP],[Defined if libcarl supports SMTP])

  AC_ARG_WITH(libcarl,
     AS_HELP_STRING([--with-libcarl=PREFIX],[look for the carl library in PREFIX/lib and headers in PREFIX/include]),
     [_libcarl_with=$withval],[_libcarl_with=ifelse([$1],,[yes],[$1])])

  if test "$_libcarl_with" != "no" ; then

     AC_PROG_AWK

     _libcarl_version_parse="eval $AWK '{split(\$NF,A,\".\"); X=256*256*A[[1]]+256*A[[2]]+A[[3]]; print X;}'"

     _libcarl_try_link=yes

     if test -d "$_libcarl_with" ; then
        LIBCARL_CPPFLAGS="-I$withval/include"
        _libcarl_ldflags="-L$withval/lib"
        AC_PATH_PROG([_libcarl_config],[carl-config],[],
                     ["$withval/bin"])
     else
        AC_PATH_PROG([_libcarl_config],[carl-config],[],[$PATH])
     fi

     if test x$_libcarl_config != "x" ; then
        AC_CACHE_CHECK([for the version of libcarl],
           [libcarl_cv_lib_carl_version],
           [libcarl_cv_lib_carl_version=`$_libcarl_config --version | $AWK '{print $[]2}'`])

        _libcarl_version=`echo $libcarl_cv_lib_carl_version | $_libcarl_version_parse`
        _libcarl_wanted=`echo ifelse([$2],,[0],[$2]) | $_libcarl_version_parse`

        if test $_libcarl_wanted -gt 0 ; then
           AC_CACHE_CHECK([for libcarl >= version $2],
              [libcarl_cv_lib_version_ok],
              [
              if test $_libcarl_version -ge $_libcarl_wanted ; then
                 libcarl_cv_lib_version_ok=yes
              else
                 libcarl_cv_lib_version_ok=no
              fi
              ])
        fi

        if test $_libcarl_wanted -eq 0 || test x$libcarl_cv_lib_version_ok = xyes ; then
           if test x"$LIBCARL_CPPFLAGS" = "x" ; then
              LIBCARL_CPPFLAGS=`$_libcarl_config --cflags`
           fi
           if test x"$LIBCARL" = "x" ; then
              LIBCARL=`$_libcarl_config --libs`

              # This is so silly, but Apple actually has a bug in their
              # carl-config script.  Fixed in Tiger, but there are still
              # lots of Panther installs around.
              case "${host}" in
                 powerpc-apple-darwin7*)
                    LIBCARL=`echo $LIBCARL | sed -e 's|-arch i386||g'`
                 ;;
              esac
           fi

           # All carl-config scripts support --feature
           _libcarl_features=`$_libcarl_config --feature`

           # Is it modern enough to have --protocols? (7.12.4)
           if test $_libcarl_version -ge 461828 ; then
              _libcarl_protocols=`$_libcarl_config --protocols`
           fi
        else
           _libcarl_try_link=no
        fi

        unset _libcarl_wanted
     fi

     if test $_libcarl_try_link = yes ; then

        # we didn't find carl-config, so let's see if the user-supplied
        # link line (or failing that, "-lcarl") is enough.
        LIBCARL=${LIBCARL-"$_libcarl_ldflags -lcarl"}

        AC_CACHE_CHECK([whether libcarl is usable],
           [libcarl_cv_lib_carl_usable],
           [
           _libcarl_save_cppflags=$CPPFLAGS
           CPPFLAGS="$LIBCARL_CPPFLAGS $CPPFLAGS"
           _libcarl_save_libs=$LIBS
           LIBS="$LIBCARL $LIBS"

           AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <carl/carl.h>]],[[
/* Try and use a few common options to force a failure if we are
   missing symbols or can't link. */
int x;
carl_easy_setopt(NULL,CARLOPT_URL,NULL);
x=CARL_ERROR_SIZE;
x=CARLOPT_WRITEFUNCTION;
x=CARLOPT_WRITEDATA;
x=CARLOPT_ERRORBUFFER;
x=CARLOPT_STDERR;
x=CARLOPT_VERBOSE;
if (x) {;}
]])],libcarl_cv_lib_carl_usable=yes,libcarl_cv_lib_carl_usable=no)

           CPPFLAGS=$_libcarl_save_cppflags
           LIBS=$_libcarl_save_libs
           unset _libcarl_save_cppflags
           unset _libcarl_save_libs
           ])

        if test $libcarl_cv_lib_carl_usable = yes ; then

           # Does carl_free() exist in this version of libcarl?
           # If not, fake it with free()

           _libcarl_save_cppflags=$CPPFLAGS
           CPPFLAGS="$CPPFLAGS $LIBCARL_CPPFLAGS"
           _libcarl_save_libs=$LIBS
           LIBS="$LIBS $LIBCARL"

           AC_CHECK_FUNC(carl_free,,
              AC_DEFINE(carl_free,free,
                [Define carl_free() as free() if our version of carl lacks carl_free.]))

           CPPFLAGS=$_libcarl_save_cppflags
           LIBS=$_libcarl_save_libs
           unset _libcarl_save_cppflags
           unset _libcarl_save_libs

           AC_DEFINE(HAVE_LIBCARL,1,
             [Define to 1 if you have a functional carl library.])
           AC_SUBST(LIBCARL_CPPFLAGS)
           AC_SUBST(LIBCARL)

           for _libcarl_feature in $_libcarl_features ; do
              AC_DEFINE_UNQUOTED(AS_TR_CPP(libcarl_feature_$_libcarl_feature),[1])
              eval AS_TR_SH(libcarl_feature_$_libcarl_feature)=yes
           done

           if test "x$_libcarl_protocols" = "x" ; then

              # We don't have --protocols, so just assume that all
              # protocols are available
              _libcarl_protocols="HTTP FTP FILE TELNET LDAP DICT TFTP"

              if test x$libcarl_feature_SSL = xyes ; then
                 _libcarl_protocols="$_libcarl_protocols HTTPS"

                 # FTPS wasn't standards-compliant until version
                 # 7.11.0 (0x070b00 == 461568)
                 if test $_libcarl_version -ge 461568; then
                    _libcarl_protocols="$_libcarl_protocols FTPS"
                 fi
              fi

              # RTSP, IMAP, POP3 and SMTP were added in
              # 7.20.0 (0x071400 == 463872)
              if test $_libcarl_version -ge 463872; then
                 _libcarl_protocols="$_libcarl_protocols RTSP IMAP POP3 SMTP"
              fi
           fi

           for _libcarl_protocol in $_libcarl_protocols ; do
              AC_DEFINE_UNQUOTED(AS_TR_CPP(libcarl_protocol_$_libcarl_protocol),[1])
              eval AS_TR_SH(libcarl_protocol_$_libcarl_protocol)=yes
           done
        else
           unset LIBCARL
           unset LIBCARL_CPPFLAGS
        fi
     fi

     unset _libcarl_try_link
     unset _libcarl_version_parse
     unset _libcarl_config
     unset _libcarl_feature
     unset _libcarl_features
     unset _libcarl_protocol
     unset _libcarl_protocols
     unset _libcarl_version
     unset _libcarl_ldflags
  fi

  if test x$_libcarl_with = xno || test x$libcarl_cv_lib_carl_usable != xyes ; then
     # This is the IF-NO path
     ifelse([$4],,:,[$4])
  else
     # This is the IF-YES path
     ifelse([$3],,:,[$3])
  fi

  unset _libcarl_with
])dnl
