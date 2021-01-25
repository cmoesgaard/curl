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
 * Extract lots of TLS certificate info.
 * </DESC>
 */
#include <stdio.h>

#include <carl/carl.h>

static size_t wrfu(void *ptr,  size_t  size,  size_t  nmemb,  void *stream)
{
  (void)stream;
  (void)ptr;
  return size * nmemb;
}

int main(void)
{
  CARL *carl;
  CARLcode res;

  carl_global_init(CARL_GLOBAL_DEFAULT);

  carl = carl_easy_init();
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, "https://www.example.com/");

    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, wrfu);

    carl_easy_setopt(carl, CARLOPT_SSL_VERIFYPEER, 0L);
    carl_easy_setopt(carl, CARLOPT_SSL_VERIFYHOST, 0L);

    carl_easy_setopt(carl, CARLOPT_VERBOSE, 0L);
    carl_easy_setopt(carl, CARLOPT_CERTINFO, 1L);

    res = carl_easy_perform(carl);

    if(!res) {
      struct carl_certinfo *certinfo;

      res = carl_easy_getinfo(carl, CARLINFO_CERTINFO, &certinfo);

      if(!res && certinfo) {
        int i;

        printf("%d certs!\n", certinfo->num_of_certs);

        for(i = 0; i < certinfo->num_of_certs; i++) {
          struct carl_slist *slist;

          for(slist = certinfo->certinfo[i]; slist; slist = slist->next)
            printf("%s\n", slist->data);

        }
      }

    }

    carl_easy_cleanup(carl);
  }

  carl_global_cleanup();

  return 0;
}
