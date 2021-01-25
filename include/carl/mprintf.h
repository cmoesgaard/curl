#ifndef CARLINC_MPRINTF_H
#define CARLINC_MPRINTF_H
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

#include <stdarg.h>
#include <stdio.h> /* needed for FILE */
#include "carl.h"  /* for CARL_EXTERN */

#ifdef  __cplusplus
extern "C" {
#endif

CARL_EXTERN int carl_mprintf(const char *format, ...);
CARL_EXTERN int carl_mfprintf(FILE *fd, const char *format, ...);
CARL_EXTERN int carl_msprintf(char *buffer, const char *format, ...);
CARL_EXTERN int carl_msnprintf(char *buffer, size_t maxlength,
                               const char *format, ...);
CARL_EXTERN int carl_mvprintf(const char *format, va_list args);
CARL_EXTERN int carl_mvfprintf(FILE *fd, const char *format, va_list args);
CARL_EXTERN int carl_mvsprintf(char *buffer, const char *format, va_list args);
CARL_EXTERN int carl_mvsnprintf(char *buffer, size_t maxlength,
                                const char *format, va_list args);
CARL_EXTERN char *carl_maprintf(const char *format, ...);
CARL_EXTERN char *carl_mvaprintf(const char *format, va_list args);

#ifdef  __cplusplus
}
#endif

#endif /* CARLINC_MPRINTF_H */
