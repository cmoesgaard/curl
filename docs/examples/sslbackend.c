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
 * Shows HTTPS usage with client certs and optional ssl engine use.
 * </DESC>
 */
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <carl/carl.h>

/*
 * An SSL-enabled libcarl is required for this sample to work (at least one
 * SSL backend has to be configured).
 *
 *  **** This example only works with libcarl 7.56.0 and later! ****
*/

int main(int argc, char **argv)
{
  const char *name = argc > 1 ? argv[1] : "openssl";
  CARLsslset result;

  if(!strcmp("list", name)) {
    const carl_ssl_backend **list;
    int i;

    result = carl_global_sslset((carl_sslbackend)-1, NULL, &list);
    assert(result == CARLSSLSET_UNKNOWN_BACKEND);

    for(i = 0; list[i]; i++)
      printf("SSL backend #%d: '%s' (ID: %d)\n",
             i, list[i]->name, list[i]->id);

    return 0;
  }
  else if(isdigit((int)(unsigned char)*name)) {
    int id = atoi(name);

    result = carl_global_sslset((carl_sslbackend)id, NULL, NULL);
  }
  else
    result = carl_global_sslset((carl_sslbackend)-1, name, NULL);

  if(result == CARLSSLSET_UNKNOWN_BACKEND) {
    fprintf(stderr, "Unknown SSL backend id: %s\n", name);
    return 1;
  }

  assert(result == CARLSSLSET_OK);

  printf("Version with SSL backend '%s':\n\n\t%s\n", name, carl_version());

  return 0;
}
