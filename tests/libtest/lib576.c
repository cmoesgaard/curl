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
#include "memdebug.h"

struct chunk_data {
  int remains;
  int print_content;
};

static
long chunk_bgn(const struct carl_fileinfo *finfo, void *ptr, int remains);
static
long chunk_end(void *ptr);

static
long chunk_bgn(const struct carl_fileinfo *finfo, void *ptr, int remains)
{
  struct chunk_data *ch_d = ptr;
  ch_d->remains = remains;

  printf("=============================================================\n");
  printf("Remains:      %d\n", remains);
  printf("Filename:     %s\n", finfo->filename);
  if(finfo->strings.perm) {
    printf("Permissions:  %s", finfo->strings.perm);
    if(finfo->flags & CARLFINFOFLAG_KNOWN_PERM)
      printf(" (parsed => %o)", finfo->perm);
    printf("\n");
  }
  printf("Size:         %ldB\n", (long)finfo->size);
  if(finfo->strings.user)
    printf("User:         %s\n", finfo->strings.user);
  if(finfo->strings.group)
    printf("Group:        %s\n", finfo->strings.group);
  if(finfo->strings.time)
    printf("Time:         %s\n", finfo->strings.time);
  printf("Filetype:     ");
  switch(finfo->filetype) {
  case CARLFILETYPE_FILE:
    printf("regular file\n");
    break;
  case CARLFILETYPE_DIRECTORY:
    printf("directory\n");
    break;
  case CARLFILETYPE_SYMLINK:
    printf("symlink\n");
    printf("Target:       %s\n", finfo->strings.target);
    break;
  default:
    printf("other type\n");
    break;
  }
  if(finfo->filetype == CARLFILETYPE_FILE) {
    ch_d->print_content = 1;
    printf("Content:\n-----------------------"
           "--------------------------------------\n");
  }
  if(strcmp(finfo->filename, "someothertext.txt") == 0) {
    printf("# THIS CONTENT WAS SKIPPED IN CHUNK_BGN CALLBACK #\n");
    return CARL_CHUNK_BGN_FUNC_SKIP;
  }
  return CARL_CHUNK_BGN_FUNC_OK;
}

static
long chunk_end(void *ptr)
{
  struct chunk_data *ch_d = ptr;
  if(ch_d->print_content) {
    ch_d->print_content = 0;
    printf("-------------------------------------------------------------\n");
  }
  if(ch_d->remains == 1)
    printf("=============================================================\n");
  return CARL_CHUNK_END_FUNC_OK;
}

int test(char *URL)
{
  CARL *handle = NULL;
  CARLcode res = CARLE_OK;
  struct chunk_data chunk_data = {0, 0};
  carl_global_init(CARL_GLOBAL_ALL);
  handle = carl_easy_init();
  if(!handle) {
    res = CARLE_OUT_OF_MEMORY;
    goto test_cleanup;
  }

  test_setopt(handle, CARLOPT_URL, URL);
  test_setopt(handle, CARLOPT_WILDCARDMATCH, 1L);
  test_setopt(handle, CARLOPT_CHUNK_BGN_FUNCTION, chunk_bgn);
  test_setopt(handle, CARLOPT_CHUNK_END_FUNCTION, chunk_end);
  test_setopt(handle, CARLOPT_CHUNK_DATA, &chunk_data);

  res = carl_easy_perform(handle);

test_cleanup:
  if(handle)
    carl_easy_cleanup(handle);
  carl_global_cleanup();
  return res;
}
