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

#include "testtrace.h"
#include "memdebug.h"

#ifdef LIB585

static int counter;

static carl_socket_t tst_opensocket(void *clientp,
                                    carlsocktype purpose,
                                    struct carl_sockaddr *addr)
{
  (void)clientp;
  (void)purpose;
  printf("[OPEN] counter: %d\n", ++counter);
  return socket(addr->family, addr->socktype, addr->protocol);
}

static int tst_closesocket(void *clientp, carl_socket_t sock)
{
  (void)clientp;
  printf("[CLOSE] counter: %d\n", counter--);
  return sclose(sock);
}

static void setupcallbacks(CARL *carl)
{
  carl_easy_setopt(carl, CARLOPT_OPENSOCKETFUNCTION, tst_opensocket);
  carl_easy_setopt(carl, CARLOPT_CLOSESOCKETFUNCTION, tst_closesocket);
  counter = 0;
}

#else
#define setupcallbacks(x) Curl_nop_stmt
#endif


int test(char *URL)
{
  CARLcode res;
  CARL *carl;
  char *ipstr = NULL;

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
  test_setopt(carl, CARLOPT_HEADER, 1L);

  libtest_debug_config.nohex = 1;
  libtest_debug_config.tracetime = 1;
  test_setopt(carl, CARLOPT_DEBUGDATA, &libtest_debug_config);
  test_setopt(carl, CARLOPT_DEBUGFUNCTION, libtest_debug_cb);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  if(libtest_arg3 && !strcmp(libtest_arg3, "activeftp"))
    test_setopt(carl, CARLOPT_FTPPORT, "-");

  setupcallbacks(carl);

  res = carl_easy_perform(carl);

  if(!res) {
    res = carl_easy_getinfo(carl, CARLINFO_PRIMARY_IP, &ipstr);
    if(libtest_arg2) {
      FILE *moo = fopen(libtest_arg2, "wb");
      if(moo) {
        carl_off_t time_namelookup;
        carl_off_t time_connect;
        carl_off_t time_pretransfer;
        carl_off_t time_starttransfer;
        carl_off_t time_total;
        fprintf(moo, "IP: %s\n", ipstr);
        carl_easy_getinfo(carl, CARLINFO_NAMELOOKUP_TIME_T, &time_namelookup);
        carl_easy_getinfo(carl, CARLINFO_CONNECT_TIME_T, &time_connect);
        carl_easy_getinfo(carl, CARLINFO_PRETRANSFER_TIME_T,
                          &time_pretransfer);
        carl_easy_getinfo(carl, CARLINFO_STARTTRANSFER_TIME_T,
                          &time_starttransfer);
        carl_easy_getinfo(carl, CARLINFO_TOTAL_TIME_T, &time_total);

        /* since the timing will always vary we only compare relative
           differences between these 5 times */
        if(time_namelookup > time_connect) {
          fprintf(moo, "namelookup vs connect: %" CARL_FORMAT_CARL_OFF_T
                  ".%06ld %" CARL_FORMAT_CARL_OFF_T ".%06ld\n",
                  (time_namelookup / 1000000),
                  (long)(time_namelookup % 1000000),
                  (time_connect / 1000000), (long)(time_connect % 1000000));
        }
        if(time_connect > time_pretransfer) {
          fprintf(moo, "connect vs pretransfer: %" CARL_FORMAT_CARL_OFF_T
                  ".%06ld %" CARL_FORMAT_CARL_OFF_T ".%06ld\n",
                  (time_connect / 1000000), (long)(time_connect % 1000000),
                  (time_pretransfer / 1000000),
                  (long)(time_pretransfer % 1000000));
        }
        if(time_pretransfer > time_starttransfer) {
          fprintf(moo, "pretransfer vs starttransfer: %" CARL_FORMAT_CARL_OFF_T
                  ".%06ld %" CARL_FORMAT_CARL_OFF_T ".%06ld\n",
                  (time_pretransfer / 1000000),
                  (long)(time_pretransfer % 1000000),
                  (time_starttransfer / 1000000),
                  (long)(time_starttransfer % 1000000));
        }
        if(time_starttransfer > time_total) {
          fprintf(moo, "starttransfer vs total: %" CARL_FORMAT_CARL_OFF_T
                  ".%06ld %" CARL_FORMAT_CARL_OFF_T ".%06ld\n",
                  (time_starttransfer / 1000000),
                  (long)(time_starttransfer % 1000000),
                  (time_total / 1000000), (long)(time_total % 1000000));
        }

        fclose(moo);
      }
    }
  }

test_cleanup:

  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return (int)res;
}
