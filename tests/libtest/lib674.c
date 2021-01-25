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

/*
 * Get a single URL without select().
 */

int test(char *URL)
{
  CARL *handle = NULL;
  CARL *handle2;
  CARLcode res = 0;
  CARLU *urlp = NULL;
  CARLUcode uc = 0;

  global_init(CARL_GLOBAL_ALL);
  easy_init(handle);

  urlp = carl_url();

  if(!urlp) {
    fprintf(stderr, "problem init URL api.");
    goto test_cleanup;
  }

  uc = carl_url_set(urlp, CARLUPART_URL, URL, 0);
  if(uc) {
    fprintf(stderr, "problem setting CARLUPART_URL.");
    goto test_cleanup;
  }

  /* demonstrate override behavior */


  easy_setopt(handle, CARLOPT_CARLU, urlp);
  easy_setopt(handle, CARLOPT_VERBOSE, 1L);

  res = carl_easy_perform(handle);

  if(res) {
    fprintf(stderr, "%s:%d carl_easy_perform() failed with code %d (%s)\n",
            __FILE__, __LINE__, res, carl_easy_strerror(res));
    goto test_cleanup;
  }

  handle2 = carl_easy_duphandle(handle);
  res = carl_easy_perform(handle2);
  carl_easy_cleanup(handle2);

test_cleanup:

  carl_url_cleanup(urlp);
  carl_easy_cleanup(handle);
  carl_global_cleanup();

  return res;
}
