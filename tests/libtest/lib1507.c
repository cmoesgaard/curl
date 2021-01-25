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

/*
 * This is the list of basic details you need to tweak to get things right.
 */
#define USERNAME "user@example.com"
#define PASSWORD "123qwerty"
#define RECIPIENT "<1507-recipient@example.com>"
#define MAILFROM "<1507-realuser@example.com>"

#define MULTI_PERFORM_HANG_TIMEOUT 60 * 1000

static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *userp)
{
  (void)ptr;
  (void)size;
  (void)nmemb;
  (void)userp;
  return CARL_READFUNC_ABORT;
}

int test(char *URL)
{
   int res = 0;
   CARL *carl = NULL;
   CARLM *mcarl = NULL;
   int still_running = 1;
   struct timeval mp_start;
   struct carl_slist *rcpt_list = NULL;

   carl_global_init(CARL_GLOBAL_DEFAULT);

   easy_init(carl);

   multi_init(mcarl);

   rcpt_list = carl_slist_append(rcpt_list, RECIPIENT);
   /* more addresses can be added here
      rcpt_list = carl_slist_append(rcpt_list, "<others@example.com>");
   */

   carl_easy_setopt(carl, CARLOPT_URL, URL);
#if 0
   carl_easy_setopt(carl, CARLOPT_USERNAME, USERNAME);
   carl_easy_setopt(carl, CARLOPT_PASSWORD, PASSWORD);
#endif
   carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);
   carl_easy_setopt(carl, CARLOPT_READFUNCTION, read_callback);
   carl_easy_setopt(carl, CARLOPT_MAIL_FROM, MAILFROM);
   carl_easy_setopt(carl, CARLOPT_MAIL_RCPT, rcpt_list);
   carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);
   multi_add_handle(mcarl, carl);

   mp_start = tutil_tvnow();

  /* we start some action by calling perform right away */
  carl_multi_perform(mcarl, &still_running);

  while(still_running) {
    struct timeval timeout;
    int rc; /* select() return code */

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

    carl_multi_timeout(mcarl, &carl_timeo);
    if(carl_timeo >= 0) {
      timeout.tv_sec = carl_timeo / 1000;
      if(timeout.tv_sec > 1)
        timeout.tv_sec = 1;
      else
        timeout.tv_usec = (carl_timeo % 1000) * 1000;
    }

    /* get file descriptors from the transfers */
    carl_multi_fdset(mcarl, &fdread, &fdwrite, &fdexcep, &maxfd);

    /* In a real-world program you OF COURSE check the return code of the
       function calls.  On success, the value of maxfd is guaranteed to be
       greater or equal than -1.  We call select(maxfd + 1, ...), specially in
       case of (maxfd == -1), we call select(0, ...), which is basically equal
       to sleep. */

    rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

    if(tutil_tvdiff(tutil_tvnow(), mp_start) > MULTI_PERFORM_HANG_TIMEOUT) {
      fprintf(stderr, "ABORTING TEST, since it seems "
              "that it would have run forever.\n");
      break;
    }

    switch(rc) {
    case -1:
      /* select error */
      break;
    case 0: /* timeout */
    default: /* action */
      carl_multi_perform(mcarl, &still_running);
      break;
    }
  }

test_cleanup:

  carl_slist_free_all(rcpt_list);
  carl_multi_remove_handle(mcarl, carl);
  carl_multi_cleanup(mcarl);
  carl_easy_cleanup(carl);
  carl_global_cleanup();

  return res;
}
