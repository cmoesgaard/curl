/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2021, Daniel Stenberg, <daniel@haxx.se>, et al.
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
#include "warnless.h"
#include "memdebug.h"

/* The maximum string length limit (CARL_MAX_INPUT_LENGTH) is an internal
   define not publicly exposed so we set our own */
#define MAX_INPUT_LENGTH 8000000

static char buffer[MAX_INPUT_LENGTH + 2];

int test(char *URL)
{
  const struct carl_easyoption *o;
  CARL *easy;
  int error = 0;
  (void)URL;

  carl_global_init(CARL_GLOBAL_ALL);
  easy = carl_easy_init();
  if(!easy) {
    carl_global_cleanup();
    return 1;
  }

  /* make it a zero terminated C string with just As */
  memset(buffer, 'A', MAX_INPUT_LENGTH + 1);
  buffer[MAX_INPUT_LENGTH + 1] = 0;

  printf("string length: %d\n", (int)strlen(buffer));

  for(o = carl_easy_option_next(NULL);
      o;
      o = carl_easy_option_next(o)) {
    if(o->type == CARLOT_STRING) {
      CARLcode result;
      /*
       * Whitelist string options that are safe for abuse
       */
      switch(o->id) {
      case CARLOPT_PROXY_TLSAUTH_TYPE:
      case CARLOPT_TLSAUTH_TYPE:
        continue;
      default:
        /* check this */
        break;
      }

      /* This is a string. Make sure that passing in a string longer
         CARL_MAX_INPUT_LENGTH returns an error */
      result = carl_easy_setopt(easy, o->id, buffer);
      switch(result) {
      case CARLE_BAD_FUNCTION_ARGUMENT: /* the most normal */
      case CARLE_UNKNOWN_OPTION: /* left out from the build */
      case CARLE_NOT_BUILT_IN: /* not supported */
        break;
      default:
        /* all other return codes are unexpected */
        fprintf(stderr, "carl_easy_setopt(%s...) returned %d\n",
                o->name, (int)result);
        error++;
        break;
      }
    }
  }
  carl_easy_cleanup(easy);
  carl_global_cleanup();
  return error;
}
