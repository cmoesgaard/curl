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

struct headerinfo {
  size_t largest;
};

static size_t header(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t headersize = size * nmemb;
  struct headerinfo *info = (struct headerinfo *)stream;
  (void)ptr;

  if(headersize > info->largest)
    /* remember the longest header */
    info->largest = headersize;

  return nmemb * size;
}

int test(char *URL)
{
  CARLcode code;
  CARL *carl = NULL;
  int res = 0;
  struct headerinfo info = {0};

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_HEADERFUNCTION, header);
  easy_setopt(carl, CARLOPT_HEADERDATA, &info);
  easy_setopt(carl, CARLOPT_VERBOSE, 1L);
  easy_setopt(carl, CARLOPT_URL, URL);

  code = carl_easy_perform(carl);
  if(CARLE_OK != code) {
    fprintf(stderr, "%s:%d carl_easy_perform() failed, "
            "with code %d (%s)\n",
            __FILE__, __LINE__, (int)code, carl_easy_strerror(code));
    res = TEST_ERR_MAJOR_BAD;
    goto test_cleanup;
  }

  printf("Max: %ld\n", (long)info.largest);

test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
