/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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
#include "test.h"

#include "testtrace.h"
#include "memdebug.h"

int test(char *URL)
{
  CARLcode ret;
  CARL *hnd;
  carl_global_init(CARL_GLOBAL_ALL);

  hnd = carl_easy_init();
  carl_easy_setopt(hnd, CARLOPT_URL, URL);
  carl_easy_setopt(hnd, CARLOPT_VERBOSE, 1L);
  carl_easy_setopt(hnd, CARLOPT_HEADER, 1L);
  carl_easy_setopt(hnd, CARLOPT_USERPWD, "testuser:testpass");
  carl_easy_setopt(hnd, CARLOPT_USERAGENT, "lib1568");
  carl_easy_setopt(hnd, CARLOPT_HTTPAUTH, (long)CARLAUTH_DIGEST);
  carl_easy_setopt(hnd, CARLOPT_MAXREDIRS, 50L);
  carl_easy_setopt(hnd, CARLOPT_PORT, (long)atoi(libtest_arg2));

  ret = carl_easy_perform(hnd);

  carl_easy_cleanup(hnd);
  hnd = NULL;

  carl_global_cleanup();
  return (int)ret;
}

