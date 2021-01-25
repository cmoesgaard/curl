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

/* test case and code based on https://github.com/carl/carl/issues/2847 */

#include "testtrace.h"
#include "testutil.h"
#include "warnless.h"
#include "memdebug.h"

static char g_Data[40 * 1024]; /* POST 40KB */

static int sockopt_callback(void *clientp, carl_socket_t carlfd,
                            carlsocktype purpose)
{
#if defined(SOL_SOCKET) && defined(SO_SNDBUF)
  int sndbufsize = 4 * 1024; /* 4KB send buffer */
  (void) clientp;
  (void) purpose;
  setsockopt(carlfd, SOL_SOCKET, SO_SNDBUF,
             (const char *)&sndbufsize, sizeof(sndbufsize));
#else
  (void)clientp;
  (void)carlfd;
  (void)purpose;
#endif
  return CARL_SOCKOPT_OK;
}

int test(char *URL)
{
  CARLcode code;
  CARLcode res;
  struct carl_slist *pHeaderList = NULL;
  CARL *carl = carl_easy_init();
  memset(g_Data, 'A', sizeof(g_Data)); /* send As! */

  carl_easy_setopt(carl, CARLOPT_SOCKOPTFUNCTION, sockopt_callback);
  carl_easy_setopt(carl, CARLOPT_URL, URL);
  carl_easy_setopt(carl, CARLOPT_POSTFIELDS, g_Data);
  carl_easy_setopt(carl, CARLOPT_POSTFIELDSIZE, (long)sizeof(g_Data));

  libtest_debug_config.nohex = 1;
  libtest_debug_config.tracetime = 1;
  test_setopt(carl, CARLOPT_DEBUGDATA, &libtest_debug_config);
  test_setopt(carl, CARLOPT_DEBUGFUNCTION, libtest_debug_cb);
  test_setopt(carl, CARLOPT_VERBOSE, 1L);

  /* Remove "Expect: 100-continue" */
  pHeaderList = carl_slist_append(pHeaderList, "Expect:");

  carl_easy_setopt(carl, CARLOPT_HTTPHEADER, pHeaderList);

  code = carl_easy_perform(carl);

  if(code == CARLE_OK) {
    carl_off_t uploadSize;
    carl_easy_getinfo(carl, CARLINFO_SIZE_UPLOAD_T, &uploadSize);

    printf("uploadSize = %ld\n", (long)uploadSize);

    if((size_t) uploadSize == sizeof(g_Data)) {
      printf("!!!!!!!!!! PASS\n");
    }
    else {
      printf("sent %d, libcarl says %d\n",
             (int)sizeof(g_Data), (int)uploadSize);
    }
  }
  else {
    printf("carl_easy_perform() failed. e = %d\n", code);
  }
  test_cleanup:
  carl_slist_free_all(pHeaderList);
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return 0;
}
