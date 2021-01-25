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
 * Upload to SFTP, resuming a previously aborted transfer.
 * </DESC>
 */

#include <stdlib.h>
#include <stdio.h>
#include <carl/carl.h>

/* read data to upload */
static size_t readfunc(char *ptr, size_t size, size_t nmemb, void *stream)
{
  FILE *f = (FILE *)stream;
  size_t n;

  if(ferror(f))
    return CARL_READFUNC_ABORT;

  n = fread(ptr, size, nmemb, f) * size;

  return n;
}

/*
 * sftpGetRemoteFileSize returns the remote file size in byte; -1 on error
 */
static carl_off_t sftpGetRemoteFileSize(const char *i_remoteFile)
{
  CARLcode result = CARLE_GOT_NOTHING;
  carl_off_t remoteFileSizeByte = -1;
  CARL *carlHandlePtr = carl_easy_init();

  carl_easy_setopt(carlHandlePtr, CARLOPT_VERBOSE, 1L);

  carl_easy_setopt(carlHandlePtr, CARLOPT_URL, i_remoteFile);
  carl_easy_setopt(carlHandlePtr, CARLOPT_NOPROGRESS, 1);
  carl_easy_setopt(carlHandlePtr, CARLOPT_NOBODY, 1);
  carl_easy_setopt(carlHandlePtr, CARLOPT_HEADER, 1);
  carl_easy_setopt(carlHandlePtr, CARLOPT_FILETIME, 1);

  result = carl_easy_perform(carlHandlePtr);
  if(CARLE_OK == result) {
    result = carl_easy_getinfo(carlHandlePtr,
                               CARLINFO_CONTENT_LENGTH_DOWNLOAD_T,
                               &remoteFileSizeByte);
    if(result)
      return -1;
    printf("filesize: %" CARL_FORMAT_CARL_OFF_T "\n", remoteFileSizeByte);
  }
  carl_easy_cleanup(carlHandlePtr);

  return remoteFileSizeByte;
}


static int sftpResumeUpload(CARL *carlhandle, const char *remotepath,
                            const char *localpath)
{
  FILE *f = NULL;
  CARLcode result = CARLE_GOT_NOTHING;

  carl_off_t remoteFileSizeByte = sftpGetRemoteFileSize(remotepath);
  if(-1 == remoteFileSizeByte) {
    printf("Error reading the remote file size: unable to resume upload\n");
    return -1;
  }

  f = fopen(localpath, "rb");
  if(!f) {
    perror(NULL);
    return 0;
  }

  carl_easy_setopt(carlhandle, CARLOPT_UPLOAD, 1L);
  carl_easy_setopt(carlhandle, CARLOPT_URL, remotepath);
  carl_easy_setopt(carlhandle, CARLOPT_READFUNCTION, readfunc);
  carl_easy_setopt(carlhandle, CARLOPT_READDATA, f);

#ifdef _WIN32
  _fseeki64(f, remoteFileSizeByte, SEEK_SET);
#else
  fseek(f, (long)remoteFileSizeByte, SEEK_SET);
#endif
  carl_easy_setopt(carlhandle, CARLOPT_APPEND, 1L);
  result = carl_easy_perform(carlhandle);

  fclose(f);

  if(result == CARLE_OK)
    return 1;
  else {
    fprintf(stderr, "%s\n", carl_easy_strerror(result));
    return 0;
  }
}

int main(void)
{
  const char *remote = "sftp://user:pass@example.com/path/filename";
  const char *filename = "filename";
  CARL *carlhandle = NULL;

  carl_global_init(CARL_GLOBAL_ALL);
  carlhandle = carl_easy_init();

  if(!sftpResumeUpload(carlhandle, remote, filename)) {
    printf("resumed upload using carl %s failed\n", carl_version());
  }

  carl_easy_cleanup(carlhandle);
  carl_global_cleanup();

  return 0;
}
