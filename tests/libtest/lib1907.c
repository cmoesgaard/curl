/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2019 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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
  char *url_after;
  CARL *carl;
  CARLcode carl_code;
  char error_buffer[CARL_ERROR_SIZE] = "";

  carl_global_init(CARL_GLOBAL_DEFAULT);
  carl = carl_easy_init();
  carl_easy_setopt(carl, CARLOPT_URL, URL);
  carl_easy_setopt(carl, CARLOPT_ERRORBUFFER, error_buffer);
  carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);
  carl_code = carl_easy_perform(carl);
  if(!carl_code)
    fprintf(stderr, "failure expected, "
            "carl_easy_perform returned %ld: <%s>, <%s>\n",
            (long) carl_code, carl_easy_strerror(carl_code), error_buffer);

  /* print the used url */
  if(!carl_easy_getinfo(carl, CARLINFO_EFFECTIVE_URL, &url_after))
    printf("Effective URL: %s\n", url_after);

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return 0;
}
