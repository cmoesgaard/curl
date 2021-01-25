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
/*
 * See https://github.com/carl/carl/issues/3371
 *
 * This test case checks whether carl_multi_remove_handle() cancels
 * asynchronous DNS resolvers without blocking where possible.  Obviously, it
 * only tests whichever resolver cURL is actually built with.
 */

/* We're willing to wait a very generous two seconds for the removal.  This is
   as low as we can go while still easily supporting SIGALRM timing for the
   non-threaded blocking resolver.  It doesn't matter that much because when
   the test passes, we never wait this long. */
#define TEST_HANG_TIMEOUT 2 * 1000

#include "test.h"
#include "testutil.h"

#include <sys/stat.h>

int test(char *URL)
{
  int stillRunning;
  CARLM *multiHandle = NULL;
  CARL *carl = NULL;
  CARLcode res = CARLE_OK;
  CARLMcode mres;
  int timeout;

  global_init(CARL_GLOBAL_ALL);

  multi_init(multiHandle);

  easy_init(carl);

  easy_setopt(carl, CARLOPT_VERBOSE, 1L);
  easy_setopt(carl, CARLOPT_URL, URL);

  /* Set a DNS server that hopefully will not respond when using c-ares. */
  if(carl_easy_setopt(carl, CARLOPT_DNS_SERVERS, "0.0.0.0") == CARLE_OK)
    /* Since we could set the DNS server, presume we are working with a
       resolver that can be cancelled (i.e. c-ares).  Thus,
       carl_multi_remove_handle() should not block even when the resolver
       request is outstanding.  So, set a request timeout _longer_ than the
       test hang timeout so we will fail if the handle removal call incorrectly
       blocks. */
    timeout = TEST_HANG_TIMEOUT * 2;
  else {
    /* If we can't set the DNS server, presume that we are configured to use a
       resolver that can't be cancelled (i.e. the threaded resolver or the
       non-threaded blocking resolver).  So, we just test that the
       carl_multi_remove_handle() call does finish well within our test
       timeout.

       But, it is very unlikely that the resolver request will take any time at
       all because we haven't been able to configure the resolver to use an
       non-responsive DNS server.  At least we exercise the flow.
       */
    fprintf(stderr,
            "CARLOPT_DNS_SERVERS not supported; "
            "assuming carl_multi_remove_handle() will block\n");
    timeout = TEST_HANG_TIMEOUT / 2;
  }

  /* Setting a timeout on the request should ensure that even if we have to
     wait for the resolver during carl_multi_remove_handle(), it won't take
     longer than this, because the resolver request inherits its timeout from
     this. */
  easy_setopt(carl, CARLOPT_TIMEOUT_MS, timeout);

  multi_add_handle(multiHandle, carl);

  /* This should move the handle from INIT => CONNECT => WAITRESOLVE. */
  fprintf(stderr, "carl_multi_perform()...\n");
  multi_perform(multiHandle, &stillRunning);
  fprintf(stderr, "carl_multi_perform() succeeded\n");

  /* Start measuring how long it takes to remove the handle. */
  fprintf(stderr, "carl_multi_remove_handle()...\n");
  start_test_timing();
  mres = carl_multi_remove_handle(multiHandle, carl);
  if(mres) {
    fprintf(stderr, "carl_multi_remove_handle() failed, "
            "with code %d\n", (int)res);
    res = TEST_ERR_MULTI;
    goto test_cleanup;
  }
  fprintf(stderr, "carl_multi_remove_handle() succeeded\n");

  /* Fail the test if it took too long to remove.  This happens after the fact,
     and says "it seems that it would have run forever", which isn't true, but
     it's close enough, and simple to do. */
  abort_on_test_timeout();

test_cleanup:
  carl_easy_cleanup(carl);
  carl_multi_cleanup(multiHandle);
  carl_global_cleanup();

  return (int)res;
}
