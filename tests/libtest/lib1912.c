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
#include "warnless.h"
#include "memdebug.h"

#define print_err(name, exp) \
  fprintf(stderr, "Type mismatch for CARLOPT_%s (expected %s)\n", name, exp);

int test(char *URL)
{
/* Only test if GCC typechecking is available */
  int error = 0;
#ifdef CARLINC_TYPECHECK_GCC_H
  const struct carl_easyoption *o;
  for(o = carl_easy_option_next(NULL);
      o;
      o = carl_easy_option_next(o)) {
    /* Test for mismatch OR missing typecheck macros */
    if(carlcheck_long_option(o->id) !=
        (o->type == CARLOT_LONG || o->type == CARLOT_VALUES)) {
      print_err(o->name, "CARLOT_LONG or CARLOT_VALUES");
      error++;
    }
    if(carlcheck_off_t_option(o->id) != (o->type == CARLOT_OFF_T)) {
      print_err(o->name, "CARLOT_OFF_T");
      error++;
    }
    if(carlcheck_string_option(o->id) != (o->type == CARLOT_STRING)) {
      print_err(o->name, "CARLOT_STRING");
      error++;
    }
    if(carlcheck_slist_option(o->id) != (o->type == CARLOT_SLIST)) {
      print_err(o->name, "CARLOT_SLIST");
      error++;
    }
    if(carlcheck_cb_data_option(o->id) != (o->type == CARLOT_CBPTR)) {
      print_err(o->name, "CARLOT_CBPTR");
      error++;
    }
    /* From here: only test that the type matches if macro is known */
    if(carlcheck_write_cb_option(o->id) && (o->type != CARLOT_FUNCTION)) {
      print_err(o->name, "CARLOT_FUNCTION");
      error++;
    }
    if(carlcheck_conv_cb_option(o->id) && (o->type != CARLOT_FUNCTION)) {
      print_err(o->name, "CARLOT_FUNCTION");
      error++;
    }
    if(carlcheck_postfields_option(o->id) && (o->type != CARLOT_OBJECT)) {
      print_err(o->name, "CARLOT_OBJECT");
      error++;
    }
    /* Todo: no gcc typecheck for CARLOPTTYPE_BLOB types? */
  }
#endif
  (void)URL;
  return error;
}
