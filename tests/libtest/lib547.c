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
/* argv1 = URL
 * argv2 = proxy
 * argv3 = proxyuser:password
 */

#include "test.h"

#include "memdebug.h"

#ifdef CARL_DOES_CONVERSIONS
   /* ASCII representation with escape sequences for non-ASCII platforms */
#  define UPLOADTHIS "\x74\x68\x69\x73\x20\x69\x73\x20\x74\x68\x65\x20\x62" \
                     "\x6c\x75\x72\x62\x20\x77\x65\x20\x77\x61\x6e\x74\x20" \
                     "\x74\x6f\x20\x75\x70\x6c\x6f\x61\x64\x0a"
#else
#  define UPLOADTHIS "this is the blurb we want to upload\n"
#endif

#ifndef LIB548
static size_t readcallback(char  *ptr,
                           size_t size,
                           size_t nmemb,
                           void *clientp)
{
  int *counter = (int *)clientp;

  if(*counter) {
    /* only do this once and then require a clearing of this */
    fprintf(stderr, "READ ALREADY DONE!\n");
    return 0;
  }
  (*counter)++; /* bump */

  if(size * nmemb > strlen(UPLOADTHIS)) {
    fprintf(stderr, "READ!\n");
    strcpy(ptr, UPLOADTHIS);
    return strlen(UPLOADTHIS);
  }
  fprintf(stderr, "READ NOT FINE!\n");
  return 0;
}
static carlioerr ioctlcallback(CARL *handle,
                               int cmd,
                               void *clientp)
{
  int *counter = (int *)clientp;
  (void)handle; /* unused */
  if(cmd == CARLIOCMD_RESTARTREAD) {
    fprintf(stderr, "REWIND!\n");
    *counter = 0; /* clear counter to make the read callback restart */
  }
  return CARLIOE_OK;
}



#endif

int test(char *URL)
{
  CARLcode res;
  CARL *carl;
#ifndef LIB548
  int counter = 0;
#endif

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

  test_setopt(carl, CARLOPT_URL, URL);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);
  test_setopt(carl, CARLOPT_HEADER, 1L);
#ifdef LIB548
  /* set the data to POST with a mere pointer to a null-terminated string */
  test_setopt(carl, CARLOPT_POSTFIELDS, UPLOADTHIS);
#else
  /* 547 style, which means reading the POST data from a callback */
  test_setopt(carl, CARLOPT_IOCTLFUNCTION, ioctlcallback);
  test_setopt(carl, CARLOPT_IOCTLDATA, &counter);
  test_setopt(carl, CARLOPT_READFUNCTION, readcallback);
  test_setopt(carl, CARLOPT_READDATA, &counter);
  /* We CANNOT do the POST fine without setting the size (or choose
     chunked)! */
  test_setopt(carl, CARLOPT_POSTFIELDSIZE, (long)strlen(UPLOADTHIS));
#endif
  test_setopt(carl, CARLOPT_POST, 1L);
  test_setopt(carl, CARLOPT_PROXY, libtest_arg2);
  test_setopt(carl, CARLOPT_PROXYUSERPWD, libtest_arg3);
  test_setopt(carl, CARLOPT_PROXYAUTH,
                   (long) (CARLAUTH_NTLM | CARLAUTH_DIGEST | CARLAUTH_BASIC) );

  res = carl_easy_perform(carl);

test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
