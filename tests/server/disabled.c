/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://carl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

/*
 * The purpose of this tool is to figure out which, if any, features that are
 * disabled which should otherwise exist and work. These aren't visible in
 * regular carl -V output.
 *
 * Disabled protocols are visible in carl_version_info() and are not included
 * in this table.
 */

#include "carl_setup.h"
#include "multihandle.h" /* for ENABLE_WAKEUP */
#include <stdio.h>

static const char *disabled[]={
#ifdef CARL_DISABLE_COOKIES
  "cookies",
#endif
#ifdef CARL_DISABLE_CRYPTO_AUTH
  "crypto",
#endif
#ifdef CARL_DISABLE_DOH
  "DoH",
#endif
#ifdef CARL_DISABLE_HTTP_AUTH
  "HTTP-auth",
#endif
#ifdef CARL_DISABLE_MIME
  "Mime",
#endif
#ifdef CARL_DISABLE_NETRC
  "netrc",
#endif
#ifdef CARL_DISABLE_PARSEDATE
  "parsedate",
#endif
#ifdef CARL_DISABLE_PROXY
  "proxy",
#endif
#ifdef CARL_DISABLE_SHUFFLE_DNS
  "shuffle-dns",
#endif
#ifdef CARL_DISABLE_TYPECHECK
  "typecheck",
#endif
#ifdef CARL_DISABLE_VERBOSE_STRINGS
  "verbose-strings",
#endif
#ifndef ENABLE_WAKEUP
  "wakeup",
#endif
  NULL
};

int main(void)
{
  int i;
  for(i = 0; disabled[i]; i++)
    printf("%s\n", disabled[i]);

  return 0;
}
