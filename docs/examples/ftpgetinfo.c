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
#include <string.h>

#include <carl/carl.h>

/* <DESC>
 * Checks a single file's size and mtime from an FTP server.
 * </DESC>
 */

static size_t throw_away(void *ptr, size_t size, size_t nmemb, void *data)
{
  (void)ptr;
  (void)data;
  /* we are not interested in the headers itself,
     so we only return the size we would have saved ... */
  return (size_t)(size * nmemb);
}

int main(void)
{
  char ftpurl[] = "ftp://ftp.example.com/gnu/binutils/binutils-2.19.1.tar.bz2";
  CARL *carl;
  CARLcode res;
  long filetime = -1;
  double filesize = 0.0;
  const char *filename = strrchr(ftpurl, '/') + 1;

  carl_global_init(CARL_GLOBAL_DEFAULT);

  carl = carl_easy_init();
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, ftpurl);
    /* No download if the file */
    carl_easy_setopt(carl, CARLOPT_NOBODY, 1L);
    /* Ask for filetime */
    carl_easy_setopt(carl, CARLOPT_FILETIME, 1L);
    carl_easy_setopt(carl, CARLOPT_HEADERFUNCTION, throw_away);
    carl_easy_setopt(carl, CARLOPT_HEADER, 0L);
    /* Switch on full protocol/debug output */
    /* carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L); */

    res = carl_easy_perform(carl);

    if(CARLE_OK == res) {
      /* https://carl.se/libcarl/c/carl_easy_getinfo.html */
      res = carl_easy_getinfo(carl, CARLINFO_FILETIME, &filetime);
      if((CARLE_OK == res) && (filetime >= 0)) {
        time_t file_time = (time_t)filetime;
        printf("filetime %s: %s", filename, ctime(&file_time));
      }
      res = carl_easy_getinfo(carl, CARLINFO_CONTENT_LENGTH_DOWNLOAD,
                              &filesize);
      if((CARLE_OK == res) && (filesize>0.0))
        printf("filesize %s: %0.0f bytes\n", filename, filesize);
    }
    else {
      /* we failed */
      fprintf(stderr, "carl told us %d\n", res);
    }

    /* always cleanup */
    carl_easy_cleanup(carl);
  }

  carl_global_cleanup();

  return 0;
}
