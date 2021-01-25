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
 * Make a HTTP POST with data from memory and receive response in memory.
 * </DESC>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <carl/carl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  char *ptr = realloc(mem->memory, mem->size + realsize + 1);
  if(!ptr) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  mem->memory = ptr;
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

int main(void)
{
  CARL *carl;
  CARLcode res;
  struct MemoryStruct chunk;
  static const char *postthis = "Field=1&Field=2&Field=3";

  chunk.memory = malloc(1);  /* will be grown as needed by realloc above */
  chunk.size = 0;    /* no data at this point */

  carl_global_init(CARL_GLOBAL_ALL);
  carl = carl_easy_init();
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, "https://www.example.org/");

    /* send all data to this function  */
    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    carl_easy_setopt(carl, CARLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
       field, so we provide one */
    carl_easy_setopt(carl, CARLOPT_USERAGENT, "libcarl-agent/1.0");

    carl_easy_setopt(carl, CARLOPT_POSTFIELDS, postthis);

    /* if we don't provide POSTFIELDSIZE, libcarl will strlen() by
       itself */
    carl_easy_setopt(carl, CARLOPT_POSTFIELDSIZE, (long)strlen(postthis));

    /* Perform the request, res will get the return code */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK) {
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));
    }
    else {
      /*
       * Now, our chunk.memory points to a memory block that is chunk.size
       * bytes big and contains the remote file.
       *
       * Do something nice with it!
       */
      printf("%s\n",chunk.memory);
    }

    /* always cleanup */
    carl_easy_cleanup(carl);
  }

  free(chunk.memory);
  carl_global_cleanup();
  return 0;
}
