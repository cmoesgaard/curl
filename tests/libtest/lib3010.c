/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2020 - 2020, Nicolas Sterchele, <nicolas@sterchelen.net>
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

#include "memdebug.h"

int test(char *URL)
{
  CARLcode ret = CARLE_OK;
  CARL *carl = NULL;
  carl_off_t retry_after;
  char *follow_url = NULL;

  carl_global_init(CARL_GLOBAL_ALL);
  carl = carl_easy_init();

  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, URL);
    ret = carl_easy_perform(carl);
    if(ret) {
      fprintf(stderr, "%s:%d carl_easy_perform() failed with code %d (%s)\n",
          __FILE__, __LINE__, ret, carl_easy_strerror(ret));
      goto test_cleanup;
    }
    carl_easy_getinfo(carl, CARLINFO_REDIRECT_URL, &follow_url);
    carl_easy_getinfo(carl, CARLINFO_RETRY_AFTER, &retry_after);
    printf("Retry-After: %" CARL_FORMAT_CARL_OFF_T "\n", retry_after);
    carl_easy_setopt(carl, CARLOPT_URL, follow_url);
    ret = carl_easy_perform(carl);
    if(ret) {
      fprintf(stderr, "%s:%d carl_easy_perform() failed with code %d (%s)\n",
          __FILE__, __LINE__, ret, carl_easy_strerror(ret));
      goto test_cleanup;
    }

    carl_easy_reset(carl);
    carl_easy_getinfo(carl, CARLINFO_RETRY_AFTER, &retry_after);
    printf("Retry-After: %" CARL_FORMAT_CARL_OFF_T "\n", retry_after);
  }

test_cleanup:
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return ret;
}

