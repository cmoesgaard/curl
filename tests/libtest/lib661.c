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

int test(char *URL)
{
   CARLcode res;
   CARL *carl = NULL;
   char *newURL = NULL;
   struct carl_slist *slist = NULL;

   if(carl_global_init(CARL_GLOBAL_ALL) != CARLE_OK) {
     fprintf(stderr, "carl_global_init() failed\n");
     return TEST_ERR_MAJOR_BAD;
   }

   carl = carl_easy_init();
   if(!carl) {
     fprintf(stderr, "carl_easy_init() failed\n");
     res = TEST_ERR_MAJOR_BAD;
     goto test_cleanup;
   }

   /* test: CARLFTPMETHOD_SINGLECWD with absolute path should
            skip CWD to entry path */
   newURL = aprintf("%s/folderA/661", URL);
   test_setopt(carl, CARLOPT_URL, newURL);
   test_setopt(carl, CARLOPT_VERBOSE, 1L);
   test_setopt(carl, CARLOPT_IGNORE_CONTENT_LENGTH, 1L);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_SINGLECWD);
   res = carl_easy_perform(carl);

   free(newURL);
   newURL = aprintf("%s/folderB/661", URL);
   test_setopt(carl, CARLOPT_URL, newURL);
   res = carl_easy_perform(carl);

   /* test: CARLFTPMETHOD_NOCWD with absolute path should
      never emit CWD (for both new and reused easy handle) */
   carl_easy_cleanup(carl);
   carl = carl_easy_init();
   if(!carl) {
     fprintf(stderr, "carl_easy_init() failed\n");
     res = TEST_ERR_MAJOR_BAD;
     goto test_cleanup;
   }

   free(newURL);
   newURL = aprintf("%s/folderA/661", URL);
   test_setopt(carl, CARLOPT_URL, newURL);
   test_setopt(carl, CARLOPT_VERBOSE, 1L);
   test_setopt(carl, CARLOPT_IGNORE_CONTENT_LENGTH, 1L);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_NOCWD);
   res = carl_easy_perform(carl);

   /* curve ball: CWD /folderB before reusing connection with _NOCWD */
   free(newURL);
   newURL = aprintf("%s/folderB/661", URL);
   test_setopt(carl, CARLOPT_URL, newURL);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_SINGLECWD);
   res = carl_easy_perform(carl);

   free(newURL);
   newURL = aprintf("%s/folderA/661", URL);
   test_setopt(carl, CARLOPT_URL, newURL);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_NOCWD);
   res = carl_easy_perform(carl);

   /* test: CARLFTPMETHOD_NOCWD with home-relative path should
      not emit CWD for first FTP access after login */
   carl_easy_cleanup(carl);
   carl = carl_easy_init();
   if(!carl) {
     fprintf(stderr, "carl_easy_init() failed\n");
     res = TEST_ERR_MAJOR_BAD;
     goto test_cleanup;
   }

   slist = carl_slist_append(NULL, "SYST");
   if(slist == NULL) {
     fprintf(stderr, "carl_slist_append() failed\n");
     res = TEST_ERR_MAJOR_BAD;
     goto test_cleanup;
   }

   test_setopt(carl, CARLOPT_URL, URL);
   test_setopt(carl, CARLOPT_VERBOSE, 1L);
   test_setopt(carl, CARLOPT_NOBODY, 1L);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_NOCWD);
   test_setopt(carl, CARLOPT_QUOTE, slist);
   res = carl_easy_perform(carl);

   /* test: CARLFTPMETHOD_SINGLECWD with home-relative path should
      not emit CWD for first FTP access after login */
   carl_easy_cleanup(carl);
   carl = carl_easy_init();
   if(!carl) {
     fprintf(stderr, "carl_easy_init() failed\n");
     res = TEST_ERR_MAJOR_BAD;
     goto test_cleanup;
   }

   test_setopt(carl, CARLOPT_URL, URL);
   test_setopt(carl, CARLOPT_VERBOSE, 1L);
   test_setopt(carl, CARLOPT_NOBODY, 1L);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_SINGLECWD);
   test_setopt(carl, CARLOPT_QUOTE, slist);
   res = carl_easy_perform(carl);

   /* test: CARLFTPMETHOD_NOCWD with home-relative path should
      not emit CWD for second FTP access when not needed +
      bonus: see if path buffering survives carl_easy_reset() */
   carl_easy_reset(carl);
   test_setopt(carl, CARLOPT_URL, URL);
   test_setopt(carl, CARLOPT_VERBOSE, 1L);
   test_setopt(carl, CARLOPT_NOBODY, 1L);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_NOCWD);
   test_setopt(carl, CARLOPT_QUOTE, slist);
   res = carl_easy_perform(carl);


test_cleanup:

   carl_slist_free_all(slist);
   free(newURL);
   carl_easy_cleanup(carl);
   carl_global_cleanup();

   return (int)res;
}
