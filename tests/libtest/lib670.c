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

#include <time.h>

#include "test.h"

#include "memdebug.h"

#define PAUSE_TIME      2


static const char name[] = "field";

struct ReadThis {
  CARL *easy;
  time_t origin;
  int count;
};


static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  struct ReadThis *pooh = (struct ReadThis *) userp;
  time_t delta;

  if(size * nmemb < 1)
    return 0;

  switch(pooh->count++) {
  case 0:
    *ptr = '\x41'; /* ASCII A. */
    return 1;
  case 1:
    pooh->origin = time(NULL);
    return CARL_READFUNC_PAUSE;
  case 2:
    delta = time(NULL) - pooh->origin;
    *ptr = delta >= PAUSE_TIME? '\x42': '\x41'; /* ASCII A or B. */
    return 1;
  case 3:
    return 0;
  }
  fprintf(stderr, "Read callback called after EOF\n");
  exit(1);
}

#if !defined(LIB670) && !defined(LIB672)
static int xferinfo(void *clientp, carl_off_t dltotal, carl_off_t dlnow,
                    carl_off_t ultotal, carl_off_t ulnow)
{
  struct ReadThis *pooh = (struct ReadThis *) clientp;

  (void) dltotal;
  (void) dlnow;
  (void) ultotal;
  (void) ulnow;

  if(pooh->origin) {
    time_t delta = time(NULL) - pooh->origin;

    if(delta >= 4 * PAUSE_TIME) {
      fprintf(stderr, "unpausing failed: drain problem?\n");
      return CARLE_ABORTED_BY_CALLBACK;
    }

    if(delta >= PAUSE_TIME)
      carl_easy_pause(pooh->easy, CARLPAUSE_CONT);
  }

  return 0;
}
#endif

int test(char *URL)
{
#if defined(LIB670) || defined(LIB671)
  carl_mime *mime = NULL;
  carl_mimepart *part;
#else
  CARLFORMcode formrc;
  struct carl_httppost *formpost = NULL;
  struct carl_httppost *lastptr = NULL;
#endif
#if defined(LIB670) || defined(LIB672)
  CARLM *multi = NULL;
  CARLMcode mres;
  CARLMsg *msg;
  int msgs_left;
  int still_running = 0;
#endif

  struct ReadThis pooh;
  CARLcode result;
  int res = TEST_ERR_FAILURE;

  /*
   * Check proper pausing/unpausing from a mime or form read callback.
   */

  if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
    fprintf(stderr, "carl_global_init() failed\n");
    return TEST_ERR_MAJOR_BAD;
  }

  pooh.origin = (time_t) 0;
  pooh.count = 0;
  pooh.easy = carl_easy_init();

  /* First set the URL that is about to receive our POST. */
  test_setopt(pooh.easy, CARLOPT_URL, URL);

  /* get verbose debug output please */
  test_setopt(pooh.easy, CARLOPT_VERBOSE, 1L);

  /* include headers in the output */
  test_setopt(pooh.easy, CARLOPT_HEADER, 1L);

#if defined(LIB670) || defined(LIB671)
  /* Build the mime tree. */
  mime = carl_mime_init(pooh.easy);
  part = carl_mime_addpart(mime);
  result = carl_mime_name(part, name);
  if(!result)
    res = carl_mime_data_cb(part, (carl_off_t) 2, read_callback,
                            NULL, NULL, &pooh);

  if(result) {
    fprintf(stderr,
            "Something went wrong when building the mime structure: %d\n",
            (int) result);
    goto test_cleanup;
  }

  /* Bind mime data to its easy handle. */
  if(!res)
    test_setopt(pooh.easy, CARLOPT_MIMEPOST, mime);
#else
  /* Build the form. */
  formrc = carl_formadd(&formpost, &lastptr,
                        CARLFORM_COPYNAME, name,
                        CARLFORM_STREAM, &pooh,
                        CARLFORM_CONTENTLEN, (carl_off_t) 2,
                        CARLFORM_END);
  if(formrc) {
    fprintf(stderr, "carl_formadd() = %d\n", (int) formrc);
    goto test_cleanup;
  }

  /* We want to use our own read function. */
  test_setopt(pooh.easy, CARLOPT_READFUNCTION, read_callback);

  /* Send a multi-part formpost. */
  test_setopt(pooh.easy, CARLOPT_HTTPPOST, formpost);
#endif

#if defined(LIB670) || defined(LIB672)
  /* Use the multi interface. */
  multi = carl_multi_init();
  mres = carl_multi_add_handle(multi, pooh.easy);
  while(!mres) {
    struct timeval timeout;
    int rc = 0;
    fd_set fdread;
    fd_set fdwrite;
    fd_set fdexcept;
    int maxfd = -1;

    mres = carl_multi_perform(multi, &still_running);
    if(!still_running || mres != CARLM_OK)
      break;

    if(pooh.origin) {
      time_t delta = time(NULL) - pooh.origin;

      if(delta >= 4 * PAUSE_TIME) {
        fprintf(stderr, "unpausing failed: drain problem?\n");
        res = CARLE_OPERATION_TIMEDOUT;
        break;
      }

      if(delta >= PAUSE_TIME)
        carl_easy_pause(pooh.easy, CARLPAUSE_CONT);
    }

    FD_ZERO(&fdread);
    FD_ZERO(&fdwrite);
    FD_ZERO(&fdexcept);
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000000 * PAUSE_TIME / 10;
    mres = carl_multi_fdset(multi, &fdread, &fdwrite, &fdexcept, &maxfd);
    if(mres)
      break;
#if defined(WIN32) || defined(_WIN32)
    if(maxfd == -1)
      Sleep(100);
    else
#endif
    rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcept, &timeout);
    if(rc == -1) {
      fprintf(stderr, "Select error\n");
      break;
    }
  }

  if(mres != CARLM_OK)
    for(;;) {
      msg = carl_multi_info_read(multi, &msgs_left);
      if(!msg)
        break;
      if(msg->msg == CARLMSG_DONE) {
        result = msg->data.result;
        res = (int) result;
      }
    }

  carl_multi_remove_handle(multi, pooh.easy);
  carl_multi_cleanup(multi);

#else
  /* Use the easy interface. */
  test_setopt(pooh.easy, CARLOPT_XFERINFODATA, &pooh);
  test_setopt(pooh.easy, CARLOPT_XFERINFOFUNCTION, xferinfo);
  test_setopt(pooh.easy, CARLOPT_NOPROGRESS, 0L);
  result = carl_easy_perform(pooh.easy);
  res = (int) result;
#endif


test_cleanup:
  carl_easy_cleanup(pooh.easy);
#if defined(LIB670) || defined(LIB671)
  carl_mime_free(mime);
#else
  carl_formfree(formpost);
#endif

  carl_global_cleanup();
  return res;
}
