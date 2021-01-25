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

#define TEST_HANG_TIMEOUT 60 * 1000

static char const testData[] = ".abc\0xyz";
static off_t const testDataSize = sizeof(testData) - 1;

int test(char *URL)
{
  CARL *easy;
  CARLM *multi_handle;
  int still_running; /* keep number of running handles */
  CARLMsg *msg; /* for picking up messages with the transfer status */
  int msgs_left; /* how many messages are left */
  int res = CARLE_OK;

  start_test_timing();

  global_init(CARL_GLOBAL_ALL);

  /* Allocate one CARL handle per transfer */
  easy = carl_easy_init();

  /* init a multi stack */
  multi_handle = carl_multi_init();

  /* add the individual transfer */
  carl_multi_add_handle(multi_handle, easy);

  /* set the options (I left out a few, you'll get the point anyway) */
  carl_easy_setopt(easy, CARLOPT_URL, URL);
  carl_easy_setopt(easy, CARLOPT_POSTFIELDSIZE_LARGE,
                   (carl_off_t)testDataSize);
  carl_easy_setopt(easy, CARLOPT_POSTFIELDS, testData);

  /* we start some action by calling perform right away */
  carl_multi_perform(multi_handle, &still_running);

  abort_on_test_timeout();

  do {
    struct timeval timeout;
    int rc; /* select() return code */
    CARLMcode mc; /* carl_multi_fdset() return code */

    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcep;
    int maxfd = -1;

    long carl_timeo = -1;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);

    /* set a suitable timeout to play around with */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    carl_multi_timeout(multi_handle, &carl_timeo);
    if(carl_timeo >= 0) {
      timeout.tv_sec = carl_timeo / 1000;
      if(timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (carl_timeo % 1000) * 1000;
    }

    /* get file descriptors from the transfers */
    mc = carl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

    if(mc != CARLM_OK) {
      fprintf(stderr, "carl_multi_fdset() failed, code %d.\n", mc);
      break;
    }

    /* On success the value of maxfd is guaranteed to be >= -1. We call
       select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
       no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
       to sleep 100ms, which is the minimum suggested value in the
       carl_multi_fdset() doc. */

    if(maxfd == -1) {
#if defined(WIN32) || defined(_WIN32)
      Sleep(100);
      rc = 0;
#else
      /* Portable sleep for platforms other than Windows. */
      struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
      rc = select(0, NULL, NULL, NULL, &wait);
#endif
    }
    else {
      /* Note that on some platforms 'timeout' may be modified by select().
         If you need access to the original value save a copy beforehand. */
      rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
    }

    switch(rc) {
    case -1:
      /* select error */
      break;
    case 0: /* timeout */
    default: /* action */
      carl_multi_perform(multi_handle, &still_running);
      break;
    }

    abort_on_test_timeout();
  } while(still_running);

  /* See how the transfers went */
  do {
    msg = carl_multi_info_read(multi_handle, &msgs_left);
    if(msg && msg->msg == CARLMSG_DONE) {
      printf("HTTP transfer completed with status %d\n", msg->data.result);
      break;
    }

    abort_on_test_timeout();
  } while(msg);

test_cleanup:
  carl_multi_cleanup(multi_handle);

  /* Free the CARL handles */
  carl_easy_cleanup(easy);
  carl_global_cleanup();

  return res;
}
