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

int test(char *URL)
{
  CARLcode res = 0;
  CARL *carl = NULL;
  long protocol = 0;

  global_init(CARL_GLOBAL_ALL);
  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);
  res = carl_easy_perform(carl);
  if(res) {
    fprintf(stderr, "carl_easy_perform() returned %d (%s)\n",
            res, carl_easy_strerror(res));
    goto test_cleanup;
  }

  res = carl_easy_getinfo(carl, CARLINFO_PROTOCOL, &protocol);
  if(res) {
    fprintf(stderr, "carl_easy_getinfo() returned %d (%s)\n",
            res, carl_easy_strerror(res));
    goto test_cleanup;
  }

  printf("Protocol: %lx\n", protocol);

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return 0;

test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res; /* return the final return code */
}
