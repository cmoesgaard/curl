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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/* <DESC>
 * Performs an FTP upload and renames the file just after a successful
 * transfer.
 * </DESC>
 */

#define LOCAL_FILE      "/tmp/uploadthis.txt"
#define UPLOAD_FILE_AS  "while-uploading.txt"
#define REMOTE_URL      "ftp://example.com/"  UPLOAD_FILE_AS
#define RENAME_FILE_TO  "renamed-and-fine.txt"

/* NOTE: if you want this example to work on Windows with libcarl as a
   DLL, you MUST also provide a read callback with CARLOPT_READFUNCTION.
   Failing to do so will give you a crash since a DLL may not use the
   variable's memory when passed in to it from an app like this. */
static size_t read_callback(char *ptr, size_t size, size_t nmemb, void *stream)
{
  carl_off_t nread;
  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */
  size_t retcode = fread(ptr, size, nmemb, stream);

  nread = (carl_off_t)retcode;

  fprintf(stderr, "*** We read %" CARL_FORMAT_CARL_OFF_T
          " bytes from file\n", nread);
  return retcode;
}

int main(void)
{
  CARL *carl;
  CARLcode res;
  FILE *hd_src;
  struct stat file_info;
  carl_off_t fsize;

  struct carl_slist *headerlist = NULL;
  static const char buf_1 [] = "RNFR " UPLOAD_FILE_AS;
  static const char buf_2 [] = "RNTO " RENAME_FILE_TO;

  /* get the file size of the local file */
  if(stat(LOCAL_FILE, &file_info)) {
    printf("Couldn't open '%s': %s\n", LOCAL_FILE, strerror(errno));
    return 1;
  }
  fsize = (carl_off_t)file_info.st_size;

  printf("Local file size: %" CARL_FORMAT_CARL_OFF_T " bytes.\n", fsize);

  /* get a FILE * of the same file */
  hd_src = fopen(LOCAL_FILE, "rb");

  /* In windows, this will init the winsock stuff */
  carl_global_init(CARL_GLOBAL_ALL);

  /* get a carl handle */
  carl = carl_easy_init();
  if(carl) {
    /* build a list of commands to pass to libcarl */
    headerlist = carl_slist_append(headerlist, buf_1);
    headerlist = carl_slist_append(headerlist, buf_2);

    /* we want to use our own read function */
    carl_easy_setopt(carl, CARLOPT_READFUNCTION, read_callback);

    /* enable uploading */
    carl_easy_setopt(carl, CARLOPT_UPLOAD, 1L);

    /* specify target */
    carl_easy_setopt(carl, CARLOPT_URL, REMOTE_URL);

    /* pass in that last of FTP commands to run after the transfer */
    carl_easy_setopt(carl, CARLOPT_POSTQUOTE, headerlist);

    /* now specify which file to upload */
    carl_easy_setopt(carl, CARLOPT_READDATA, hd_src);

    /* Set the size of the file to upload (optional).  If you give a *_LARGE
       option you MUST make sure that the type of the passed-in argument is a
       carl_off_t. If you use CARLOPT_INFILESIZE (without _LARGE) you must
       make sure that to pass in a type 'long' argument. */
    carl_easy_setopt(carl, CARLOPT_INFILESIZE_LARGE,
                     (carl_off_t)fsize);

    /* Now run off and do what you've been told! */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* clean up the FTP commands list */
    carl_slist_free_all(headerlist);

    /* always cleanup */
    carl_easy_cleanup(carl);
  }
  fclose(hd_src); /* close the local file */

  carl_global_cleanup();
  return 0;
}
