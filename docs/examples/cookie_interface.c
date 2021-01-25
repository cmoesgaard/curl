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
 * Import and export cookies with COOKIELIST.
 * </DESC>
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <carl/carl.h>

static void
print_cookies(CARL *carl)
{
  CARLcode res;
  struct carl_slist *cookies;
  struct carl_slist *nc;
  int i;

  printf("Cookies, carl knows:\n");
  res = carl_easy_getinfo(carl, CARLINFO_COOKIELIST, &cookies);
  if(res != CARLE_OK) {
    fprintf(stderr, "Curl carl_easy_getinfo failed: %s\n",
            carl_easy_strerror(res));
    exit(1);
  }
  nc = cookies;
  i = 1;
  while(nc) {
    printf("[%d]: %s\n", i, nc->data);
    nc = nc->next;
    i++;
  }
  if(i == 1) {
    printf("(none)\n");
  }
  carl_slist_free_all(cookies);
}

int
main(void)
{
  CARL *carl;
  CARLcode res;

  carl_global_init(CARL_GLOBAL_ALL);
  carl = carl_easy_init();
  if(carl) {
    char nline[256];

    carl_easy_setopt(carl, CARLOPT_URL, "https://www.example.com/");
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);
    carl_easy_setopt(carl, CARLOPT_COOKIEFILE, ""); /* start cookie engine */
    res = carl_easy_perform(carl);
    if(res != CARLE_OK) {
      fprintf(stderr, "Curl perform failed: %s\n", carl_easy_strerror(res));
      return 1;
    }

    print_cookies(carl);

    printf("Erasing carl's knowledge of cookies!\n");
    carl_easy_setopt(carl, CARLOPT_COOKIELIST, "ALL");

    print_cookies(carl);

    printf("-----------------------------------------------\n"
           "Setting a cookie \"PREF\" via cookie interface:\n");
#ifdef WIN32
#define snprintf _snprintf
#endif
    /* Netscape format cookie */
    snprintf(nline, sizeof(nline), "%s\t%s\t%s\t%s\t%lu\t%s\t%s",
             ".example.com", "TRUE", "/", "FALSE",
             (unsigned long)time(NULL) + 31337UL,
             "PREF", "hello example, i like you very much!");
    res = carl_easy_setopt(carl, CARLOPT_COOKIELIST, nline);
    if(res != CARLE_OK) {
      fprintf(stderr, "Curl carl_easy_setopt failed: %s\n",
              carl_easy_strerror(res));
      return 1;
    }

    /* HTTP-header style cookie. If you use the Set-Cookie format and don't
    specify a domain then the cookie is sent for any domain and will not be
    modified, likely not what you intended. Starting in 7.43.0 any-domain
    cookies will not be exported either. For more information refer to the
    CARLOPT_COOKIELIST documentation.
    */
    snprintf(nline, sizeof(nline),
      "Set-Cookie: OLD_PREF=3d141414bf4209321; "
      "expires=Sun, 17-Jan-2038 19:14:07 GMT; path=/; domain=.example.com");
    res = carl_easy_setopt(carl, CARLOPT_COOKIELIST, nline);
    if(res != CARLE_OK) {
      fprintf(stderr, "Curl carl_easy_setopt failed: %s\n",
              carl_easy_strerror(res));
      return 1;
    }

    print_cookies(carl);

    res = carl_easy_perform(carl);
    if(res != CARLE_OK) {
      fprintf(stderr, "Curl perform failed: %s\n", carl_easy_strerror(res));
      return 1;
    }

    carl_easy_cleanup(carl);
  }
  else {
    fprintf(stderr, "Curl init failed!\n");
    return 1;
  }

  carl_global_cleanup();
  return 0;
}
