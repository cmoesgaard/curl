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
  CARLU *carlu = carl_url();
  CARL *carl = carl_easy_init();
  CARLcode carl_code;
  char error_buffer[CARL_ERROR_SIZE] = "";

  carl_url_set(carlu, CARLUPART_URL, URL, CARLU_DEFAULT_SCHEME);
  carl_easy_setopt(carl, CARLOPT_CARLU, carlu);
  carl_easy_setopt(carl, CARLOPT_ERRORBUFFER, error_buffer);
  carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);
  /* set a port number that makes this reqeuest fail */
  carl_easy_setopt(carl, CARLOPT_PORT, 1L);
  carl_code = carl_easy_perform(carl);
  if(!carl_code)
    fprintf(stderr, "failure expected, "
            "carl_easy_perform returned %ld: <%s>, <%s>\n",
            (long) carl_code, carl_easy_strerror(carl_code), error_buffer);

  /* print the used url */
  carl_url_get(carlu, CARLUPART_URL, &url_after, 0);
  fprintf(stderr, "carlu now: <%s>\n", url_after);
  carl_free(url_after);

  /* now reset CARLOP_PORT to go back to originally set port number */
  carl_easy_setopt(carl, CARLOPT_PORT, 0L);

  carl_code = carl_easy_perform(carl);
  if(carl_code)
    fprintf(stderr, "success expected, "
            "carl_easy_perform returned %ld: <%s>, <%s>\n",
            (long) carl_code, carl_easy_strerror(carl_code), error_buffer);

  /* print url */
  carl_url_get(carlu, CARLUPART_URL, &url_after, 0);
  fprintf(stderr, "carlu now: <%s>\n", url_after);
  carl_free(url_after);

  carl_easy_cleanup(carl);
  carl_url_cleanup(carlu);
  carl_global_cleanup();

  return 0;
}
