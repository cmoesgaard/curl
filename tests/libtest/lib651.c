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

#include "memdebug.h"

static char buffer[17000]; /* more than 16K */

int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;
  CARLFORMcode formrc;
  struct carl_httppost *formpost = NULL;
  struct carl_httppost *lastptr = NULL;

  /* create a buffer with AAAA...BBBBB...CCCC...etc */
  int i;
  int size = (int)sizeof(buffer)/1000;

  for(i = 0; i < size ; i++)
    memset(&buffer[i * 1000], 65 + i, 1000);

  buffer[ sizeof(buffer)-1] = 0; /* null-terminate */

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  /* Check proper name and data copying. */
  formrc = carl_formadd(&formpost, &lastptr,
                        CARLFORM_COPYNAME, "hello",
                        CARLFORM_COPYCONTENTS, buffer,
                        CARLFORM_END);

  if(formrc)
    printf("carl_formadd(1) = %d\n", (int) formrc);


  carl = carl_easy_init();
  if(!carl) {
    fprintf(stderr, "carl_easy_init() failed\n");
    carl_formfree(formpost);
    carl_global_cleanup();
    return TEST_ERR_MAJOR_BAD;
  }

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

  /* send a multi-part formpost */
  test_setopt(carl, CARLOPT_HTTPPOST, formpost);

  /* get verbose debug output please */
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(carl, CARLOPT_HEADER, 1L);

  /* Perform the request, res will get the return code */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);

  /* now cleanup the formpost chain */
  carl_formfree(formpost);

  carl_global_cleanup();

  return res;
}
