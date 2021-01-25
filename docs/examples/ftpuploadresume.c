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
/* <DESC>
 * Upload to FTP, resuming failed transfers.
 * </DESC>
 */

#include <stdlib.h>
#include <stdio.h>
#include <carl/carl.h>

/* parse headers for Content-Length */
static size_t getcontentlengthfunc(void *ptr, size_t size, size_t nmemb,
                                   void *stream)
{
  int r;
  long len = 0;

  r = sscanf(ptr, "Content-Length: %ld\n", &len);
  if(r)
    *((long *) stream) = len;

  return size * nmemb;
}

/* discard downloaded data */
static size_t discardfunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
  (void)ptr;
  (void)stream;
  return size * nmemb;
}

/* read data to upload */
static size_t readfunc(char *ptr, size_t size, size_t nmemb, void *stream)
{
  FILE *f = stream;
  size_t n;

  if(ferror(f))
    return CARL_READFUNC_ABORT;

  n = fread(ptr, size, nmemb, f) * size;

  return n;
}


static int upload(CARL *carlhandle, const char *remotepath,
                  const char *localpath, long timeout, long tries)
{
  FILE *f;
  long uploaded_len = 0;
  CARLcode r = CARLE_GOT_NOTHING;
  int c;

  f = fopen(localpath, "rb");
  if(!f) {
    perror(NULL);
    return 0;
  }

  carl_easy_setopt(carlhandle, CARLOPT_UPLOAD, 1L);

  carl_easy_setopt(carlhandle, CARLOPT_URL, remotepath);

  if(timeout)
    carl_easy_setopt(carlhandle, CARLOPT_FTP_RESPONSE_TIMEOUT, timeout);

  carl_easy_setopt(carlhandle, CARLOPT_HEADERFUNCTION, getcontentlengthfunc);
  carl_easy_setopt(carlhandle, CARLOPT_HEADERDATA, &uploaded_len);

  carl_easy_setopt(carlhandle, CARLOPT_WRITEFUNCTION, discardfunc);

  carl_easy_setopt(carlhandle, CARLOPT_READFUNCTION, readfunc);
  carl_easy_setopt(carlhandle, CARLOPT_READDATA, f);

  /* disable passive mode */
  carl_easy_setopt(carlhandle, CARLOPT_FTPPORT, "-");
  carl_easy_setopt(carlhandle, CARLOPT_FTP_CREATE_MISSING_DIRS, 1L);

  carl_easy_setopt(carlhandle, CARLOPT_VERBOSE, 1L);

  for(c = 0; (r != CARLE_OK) && (c < tries); c++) {
    /* are we resuming? */
    if(c) { /* yes */
      /* determine the length of the file already written */

      /*
       * With NOBODY and NOHEADER, libcarl will issue a SIZE
       * command, but the only way to retrieve the result is
       * to parse the returned Content-Length header. Thus,
       * getcontentlengthfunc(). We need discardfunc() above
       * because HEADER will dump the headers to stdout
       * without it.
       */
      carl_easy_setopt(carlhandle, CARLOPT_NOBODY, 1L);
      carl_easy_setopt(carlhandle, CARLOPT_HEADER, 1L);

      r = carl_easy_perform(carlhandle);
      if(r != CARLE_OK)
        continue;

      carl_easy_setopt(carlhandle, CARLOPT_NOBODY, 0L);
      carl_easy_setopt(carlhandle, CARLOPT_HEADER, 0L);

      fseek(f, uploaded_len, SEEK_SET);

      carl_easy_setopt(carlhandle, CARLOPT_APPEND, 1L);
    }
    else { /* no */
      carl_easy_setopt(carlhandle, CARLOPT_APPEND, 0L);
    }

    r = carl_easy_perform(carlhandle);
  }

  fclose(f);

  if(r == CARLE_OK)
    return 1;
  else {
    fprintf(stderr, "%s\n", carl_easy_strerror(r));
    return 0;
  }
}

int main(void)
{
  CARL *carlhandle = NULL;

  carl_global_init(CARL_GLOBAL_ALL);
  carlhandle = carl_easy_init();

  upload(carlhandle, "ftp://user:pass@example.com/path/file", "C:\\file",
         0, 3);

  carl_easy_cleanup(carlhandle);
  carl_global_cleanup();

  return 0;
}
