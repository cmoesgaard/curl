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

static char teststring[] =
#ifdef CARL_DOES_CONVERSIONS
  /* ASCII representation with escape sequences for non-ASCII platforms */
{ '\x54', '\x68', '\x69', '\x73', '\x00', '\x20', '\x69', '\x73', '\x20',
  '\x74', '\x65', '\x73', '\x74', '\x20', '\x62', '\x69', '\x6e', '\x61',
  '\x72', '\x79', '\x20', '\x64', '\x61', '\x74', '\x61', '\x20', '\x77',
  '\x69', '\x74', '\x68', '\x20', '\x61', '\x6e', '\x20', '\x65', '\x6d',
  '\x62', '\x65', '\x64', '\x64', '\x65', '\x64', '\x20', '\x4e', '\x55',
  '\x4c'};
#else
{   'T', 'h', 'i', 's', '\0', ' ', 'i', 's', ' ', 't', 'e', 's', 't', ' ',
    'b', 'i', 'n', 'a', 'r', 'y', ' ', 'd', 'a', 't', 'a', ' ',
    'w', 'i', 't', 'h', ' ', 'a', 'n', ' ',
    'e', 'm', 'b', 'e', 'd', 'd', 'e', 'd', ' ', 'N', 'U', 'L'};
#endif


int test(char *URL)
{
  CARL *carl;
  CARLcode res = CARLE_OK;

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

  /* First set the URL that is about to receive our POST. */
  test_setopt(carl, CARLOPT_URL, URL);

#ifdef LIB545
  test_setopt(carl, CARLOPT_POSTFIELDSIZE, (long) sizeof(teststring));
#endif

  test_setopt(carl, CARLOPT_COPYPOSTFIELDS, teststring);

  test_setopt(carl, CARLOPT_VERBOSE, 1L); /* show verbose for debug */
  test_setopt(carl, CARLOPT_HEADER, 1L); /* include header */

  /* Update the original data to detect non-copy. */
  strcpy(teststring, "FAIL");

#ifdef LIB545
  {
    CARL *handle2;
    handle2 = carl_easy_duphandle(carl);
    carl_easy_cleanup(carl);

    carl = handle2;
  }
#endif

  /* Now, this is a POST request with binary 0 embedded in POST data. */
  res = carl_easy_perform(carl);

test_cleanup:

  /* always cleanup */
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
