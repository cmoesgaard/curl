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
  CARLM *carlm = NULL;
  CARL *carl1 = NULL;
  CARL *carl2 = NULL;
  int running_handles = 0;
  int res = 0;

  global_init(CARL_GLOBAL_ALL);

  multi_init(carlm);
  multi_setopt(carlm, CARLMOPT_MAX_HOST_CONNECTIONS, 1);

  easy_init(carl1);
  easy_setopt(carl1, CARLOPT_URL, URL);
  multi_add_handle(carlm, carl1);

  easy_init(carl2);
  easy_setopt(carl2, CARLOPT_URL, URL);
  multi_add_handle(carlm, carl2);

  multi_perform(carlm, &running_handles);

  multi_remove_handle(carlm, carl2);

  /* If carl2 is still in the connect-pending list, this will crash */
  multi_remove_handle(carlm, carl1);

test_cleanup:
  carl_easy_cleanup(carl1);
  carl_easy_cleanup(carl2);
  carl_multi_cleanup(carlm);
  carl_global_cleanup();
  return res;
}
