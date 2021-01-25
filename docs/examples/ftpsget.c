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
 * Get a single file from an FTPS server.
 * </DESC>
 */

struct FtpFile {
  const char *filename;
  FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb,
                        void *stream)
{
  struct FtpFile *out = (struct FtpFile *)stream;
  if(!out->stream) {
    /* open file for writing */
    out->stream = fopen(out->filename, "wb");
    if(!out->stream)
      return -1; /* failure, can't open file to write */
  }
  return fwrite(buffer, size, nmemb, out->stream);
}


int main(void)
{
  CARL *carl;
  CARLcode res;
  struct FtpFile ftpfile = {
    "yourfile.bin", /* name to store the file as if successful */
    NULL
  };

  carl_global_init(CARL_GLOBAL_DEFAULT);

  carl = carl_easy_init();
  if(carl) {
    /*
     * You better replace the URL with one that works! Note that we use an
     * FTP:// URL with standard explicit FTPS. You can also do FTPS:// URLs if
     * you want to do the rarer kind of transfers: implicit.
     */
    carl_easy_setopt(carl, CARLOPT_URL,
                     "ftp://user@server/home/user/file.txt");
    /* Define our callback to get called when there's data to be written */
    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, my_fwrite);
    /* Set a pointer to our struct to pass to the callback */
    carl_easy_setopt(carl, CARLOPT_WRITEDATA, &ftpfile);

    /* We activate SSL and we require it for both control and data */
    carl_easy_setopt(carl, CARLOPT_USE_SSL, CARLUSESSL_ALL);

    /* Switch on full protocol/debug output */
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);

    res = carl_easy_perform(carl);

    /* always cleanup */
    carl_easy_cleanup(carl);

    if(CARLE_OK != res) {
      /* we failed */
      fprintf(stderr, "carl told us %d\n", res);
    }
  }

  if(ftpfile.stream)
    fclose(ftpfile.stream); /* close the local file */

  carl_global_cleanup();

  return 0;
}
