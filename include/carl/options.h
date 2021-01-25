#ifndef CARLINC_OPTIONS_H
#define CARLINC_OPTIONS_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2018 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
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

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum {
  CARLOT_LONG,    /* long (a range of values) */
  CARLOT_VALUES,  /*      (a defined set or bitmask) */
  CARLOT_OFF_T,   /* carl_off_t (a range of values) */
  CARLOT_OBJECT,  /* pointer (void *) */
  CARLOT_STRING,  /*         (char * to zero terminated buffer) */
  CARLOT_SLIST,   /*         (struct carl_slist *) */
  CARLOT_CBPTR,   /*         (void * passed as-is to a callback) */
  CARLOT_BLOB,    /* blob (struct carl_blob *) */
  CARLOT_FUNCTION /* function pointer */
} carl_easytype;

/* Flag bits */

/* "alias" means it is provided for old programs to remain functional,
   we prefer another name */
#define CARLOT_FLAG_ALIAS (1<<0)

/* The CARLOPTTYPE_* id ranges can still be used to figure out what type/size
   to use for carl_easy_setopt() for the given id */
struct carl_easyoption {
  const char *name;
  CARLoption id;
  carl_easytype type;
  unsigned int flags;
};

CARL_EXTERN const struct carl_easyoption *
carl_easy_option_by_name(const char *name);

CARL_EXTERN const struct carl_easyoption *
carl_easy_option_by_id (CARLoption id);

CARL_EXTERN const struct carl_easyoption *
carl_easy_option_next(const struct carl_easyoption *prev);

#ifdef __cplusplus
} /* end of extern "C" */
#endif
#endif /* CARLINC_OPTIONS_H */
