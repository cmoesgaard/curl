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

#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define EXCESSIVE 10*1000*1000
int test(char *URL)
{
  CARLcode res = 0;
  CARL *carl = NULL;
  char *longurl = malloc(EXCESSIVE);
  CARLU *u;
  (void)URL;

  if(!longurl)
    return 1;

  memset(longurl, 'a', EXCESSIVE);
  longurl[EXCESSIVE-1] = 0;

  global_init(CARL_GLOBAL_ALL);
  easy_init(carl);

  res = carl_easy_setopt(carl, CARLOPT_URL, longurl);
  printf("CARLOPT_URL %d bytes URL == %d\n",
         EXCESSIVE, (int)res);

  res = carl_easy_setopt(carl, CARLOPT_POSTFIELDS, longurl);
  printf("CARLOPT_POSTFIELDS %d bytes data == %d\n",
         EXCESSIVE, (int)res);

  u = carl_url();
  if(u) {
    CARLUcode uc = carl_url_set(u, CARLUPART_URL, longurl, 0);
    printf("CARLUPART_URL %d bytes URL == %d\n",
           EXCESSIVE, (int)uc);
    uc = carl_url_set(u, CARLUPART_SCHEME, longurl, CARLU_NON_SUPPORT_SCHEME);
    printf("CARLUPART_SCHEME %d bytes scheme == %d\n",
           EXCESSIVE, (int)uc);
    uc = carl_url_set(u, CARLUPART_USER, longurl, 0);
    printf("CARLUPART_USER %d bytes user == %d\n",
           EXCESSIVE, (int)uc);
    carl_url_cleanup(u);
  }

test_cleanup:
  free(longurl);
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res; /* return the final return code */
}
