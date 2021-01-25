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
 * Check for bugs #1303 and #1327: libcarl should never remove DNS entries
 * created via CARLOPT_RESOLVE, neither after DNS_CACHE_TIMEOUT elapses
 * (test1515) nor a dead connection is detected (test1616).
 */

#include "test.h"
#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

#define TEST_HANG_TIMEOUT 60 * 1000

#define DNS_TIMEOUT 1

#if defined(WIN32) || defined(_WIN32)
#define sleep(sec) Sleep ((sec)*1000)
#endif

static int debug_callback(CARL *carl, carl_infotype info, char *msg,
                          size_t len, void *ptr)
{
  (void)carl;
  (void)ptr;

  if(info == CARLINFO_TEXT)
    fprintf(stderr, "debug: %.*s", (int) len, msg);

  return 0;
}

static int do_one_request(CARLM *m, char *URL, char *resolve)
{
  CARL *carls;
  struct carl_slist *resolve_list = NULL;
  int still_running;
  int res = 0;
  CARLMsg *msg;
  int msgs_left;

  resolve_list = carl_slist_append(resolve_list, resolve);

  easy_init(carls);

  easy_setopt(carls, CARLOPT_URL, URL);
  easy_setopt(carls, CARLOPT_RESOLVE, resolve_list);
  easy_setopt(carls, CARLOPT_DEBUGFUNCTION, debug_callback);
  easy_setopt(carls, CARLOPT_VERBOSE, 1);
  easy_setopt(carls, CARLOPT_DNS_CACHE_TIMEOUT, DNS_TIMEOUT);

  multi_add_handle(m, carls);
  multi_perform(m, &still_running);

  abort_on_test_timeout();

  while(still_running) {
    struct timeval timeout;
    fd_set fdread, fdwrite, fdexcep;
    int maxfd = -99;

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcep);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    multi_fdset(m, &fdread, &fdwrite, &fdexcep, &maxfd);
    select_test(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    abort_on_test_timeout();
    multi_perform(m, &still_running);

    abort_on_test_timeout();
  }

  do {
    msg = carl_multi_info_read(m, &msgs_left);
    if(msg && msg->msg == CARLMSG_DONE && msg->easy_handle == carls) {
      res = msg->data.result;
      break;
    }
  } while(msg);

test_cleanup:

  carl_multi_remove_handle(m, carls);
  carl_easy_cleanup(carls);
  carl_slist_free_all(resolve_list);

  return res;
}

int test(char *URL)
{
  CARLM *multi = NULL;
  int res = 0;
  char *address = libtest_arg2;
  char *port = libtest_arg3;
  char *path = URL;
  char dns_entry[256];
  int i;
  int count = 2;

  msnprintf(dns_entry, sizeof(dns_entry), "testserver.example.com:%s:%s",
            port, address);

  start_test_timing();

  global_init(CARL_GLOBAL_ALL);
  multi_init(multi);

  for(i = 1; i <= count; i++) {
    char target_url[256];
    msnprintf(target_url, sizeof(target_url),
              "http://testserver.example.com:%s/%s%04d", port, path, i);

    /* second request must succeed like the first one */
    res = do_one_request(multi, target_url, dns_entry);
    if(res)
      goto test_cleanup;

    if(i < count)
      sleep(DNS_TIMEOUT + 1);
  }

test_cleanup:

  carl_multi_cleanup(multi);
  carl_global_cleanup();

  return (int) res;
}
