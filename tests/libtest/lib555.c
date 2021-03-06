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

/* This test case is supposed to be identical to 547 except that this uses the
 * multi interface and 547 is easy interface.
 *
 * argv1 = URL
 * argv2 = proxy
 * argv3 = proxyuser:password
 */

#include "test.h"
#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define TEST_HANG_TIMEOUT 60 * 1000

static const char uploadthis[] =
#ifdef CARL_DOES_CONVERSIONS
  /* ASCII representation with escape sequences for non-ASCII platforms */
  "\x74\x68\x69\x73\x20\x69\x73\x20\x74\x68\x65\x20\x62\x6c\x75\x72"
  "\x62\x20\x77\x65\x20\x77\x61\x6e\x74\x20\x74\x6f\x20\x75\x70\x6c"
  "\x6f\x61\x64\x0a";
#else
  "this is the blurb we want to upload\n";
#endif

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

  if(size * nmemb > strlen(uploadthis)) {
    fprintf(stderr, "READ!\n");
    strcpy(ptr, uploadthis);
    return strlen(uploadthis);
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


int test(char *URL)
{
  int res = 0;
  CARL *carl = NULL;
  int counter = 0;
  CARLM *m = NULL;
  int running = 1;

  start_test_timing();

  global_init(CARL_GLOBAL_ALL);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_URL, URL);
  easy_setopt(carl, CARLOPT_VERBOSE, 1L);
  easy_setopt(carl, CARLOPT_HEADER, 1L);

  /* read the POST data from a callback */
  easy_setopt(carl, CARLOPT_IOCTLFUNCTION, ioctlcallback);
  easy_setopt(carl, CARLOPT_IOCTLDATA, &counter);
  easy_setopt(carl, CARLOPT_READFUNCTION, readcallback);
  easy_setopt(carl, CARLOPT_READDATA, &counter);
  /* We CANNOT do the POST fine without setting the size (or choose
     chunked)! */
  easy_setopt(carl, CARLOPT_POSTFIELDSIZE, (long)strlen(uploadthis));

  easy_setopt(carl, CARLOPT_POST, 1L);
  easy_setopt(carl, CARLOPT_PROXY, libtest_arg2);
  easy_setopt(carl, CARLOPT_PROXYUSERPWD, libtest_arg3);
  easy_setopt(carl, CARLOPT_PROXYAUTH,
                   (long) (CARLAUTH_NTLM | CARLAUTH_DIGEST | CARLAUTH_BASIC) );

  multi_init(m);

  multi_add_handle(m, carl);

  while(running) {
    struct timeval timeout;
    fd_set fdread, fdwrite, fdexcep;
    int maxfd = -99;

    timeout.tv_sec = 0;
    timeout.tv_usec = 100000L; /* 100 ms */

    multi_perform(m, &running);

    abort_on_test_timeout();

#ifdef TPF
    sleep(1); /* avoid ctl-10 dump */
#endif

    if(!running)
      break; /* done */

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    multi_fdset(m, &fdread, &fdwrite, &fdexcep, &maxfd);

    /* At this point, maxfd is guaranteed to be greater or equal than -1. */

    select_test(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    abort_on_test_timeout();
  }

test_cleanup:

  /* proper cleanup sequence - type PA */

  carl_multi_remove_handle(m, carl);
  carl_multi_cleanup(m);
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
