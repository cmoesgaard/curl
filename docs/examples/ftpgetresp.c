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
#include <stdio.h>

#include <carl/carl.h>

/* <DESC>
 * Similar to ftpget.c but also stores the received response-lines
 * in a separate file using our own callback!
 * </DESC>
 */
static size_t
write_response(void *ptr, size_t size, size_t nmemb, void *data)
{
  FILE *writehere = (FILE *)data;
  return fwrite(ptr, size, nmemb, writehere);
}

#define FTPBODY "ftp-list"
#define FTPHEADERS "ftp-responses"

int main(void)
{
  CARL *carl;
  CARLcode res;
  FILE *ftpfile;
  FILE *respfile;

  /* local file name to store the file as */
  ftpfile = fopen(FTPBODY, "wb"); /* b is binary, needed on win32 */

  /* local file name to store the FTP server's response lines in */
  respfile = fopen(FTPHEADERS, "wb"); /* b is binary, needed on win32 */

  carl = carl_easy_init();
  if(carl) {
    /* Get a file listing from sunet */
    carl_easy_setopt(carl, CARLOPT_URL, "ftp://ftp.example.com/");
    carl_easy_setopt(carl, CARLOPT_WRITEDATA, ftpfile);
    /* If you intend to use this on windows with a libcarl DLL, you must use
       CARLOPT_WRITEFUNCTION as well */
    carl_easy_setopt(carl, CARLOPT_HEADERFUNCTION, write_response);
    carl_easy_setopt(carl, CARLOPT_HEADERDATA, respfile);
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);
  }

  fclose(ftpfile); /* close the local file */
  fclose(respfile); /* close the response file */

  return 0;
}
