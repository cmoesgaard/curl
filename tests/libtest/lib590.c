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
#include "test.h"

/*
  Based on a bug report recipe by Rene Bernhardt in
  https://carl.se/mail/lib-2011-10/0323.html

  It is reproducible by the following steps:

  - Use a proxy that offers NTLM and Negotiate ( CARLOPT_PROXY and
  CARLOPT_PROXYPORT)
  - Tell libcarl NOT to use Negotiate  CARL_EASY_SETOPT(CARLOPT_PROXYAUTH,
  CARLAUTH_BASIC | CARLAUTH_DIGEST | CARLAUTH_NTLM)
  - Start the request
*/

#include "memdebug.h"

int test(char *URL)
{
  CARLcode res;
  CARL *carl;

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_HEADER, 1L);
  test_setopt(carl, CARLOPT_PROXYAUTH,
              (long) (CARLAUTH_BASIC | CARLAUTH_DIGEST | CARLAUTH_NTLM));
  test_setopt(carl, CARLOPT_PROXY, libtest_arg2); /* set in first.c */
  test_setopt(carl, CARLOPT_PROXYUSERPWD, "me:password");

  res = carl_easy_perform(carl);

  test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
