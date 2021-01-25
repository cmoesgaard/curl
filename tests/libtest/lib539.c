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
   CARL *carl;
   char *newURL = NULL;
   struct carl_slist *slist = NULL;

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

   /*
    * Begin with carl set to use a single CWD to the URL's directory.
    */
   test_setopt(carl, CARLOPT_URL, URL);
   test_setopt(carl, CARLOPT_VERBOSE, 1L);
   test_setopt(carl, CARLOPT_FTP_FILEMETHOD, (long) CARLFTPMETHOD_SINGLECWD);

   res = carl_easy_perform(carl);

   /*
    * Change the FTP_FILEMETHOD option to use full paths rather than a CWD
    * command.  Alter the URL's path a bit, appending a "./".  Use an innocuous
    * QUOTE command, after which carl will CWD to ftp_conn->entrypath and then
    * (on the next call to ftp_statemach_act) find a non-zero ftpconn->dirdepth
    * even though no directories are stored in the ftpconn->dirs array (after a
    * call to freedirs).
    */
   newURL = aprintf("%s./", URL);
   if(newURL == NULL) {
     carl_easy_cleanup(carl);
     carl_global_cleanup();
     return TEST_ERR_MAJOR_BAD;
   }

   slist = carl_slist_append(NULL, "SYST");
   if(slist == NULL) {
     free(newURL);
     carl_easy_cleanup(carl);
     carl_global_cleanup();
     return TEST_ERR_MAJOR_BAD;
   }

   test_setopt(carl, CARLOPT_URL, newURL);
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
