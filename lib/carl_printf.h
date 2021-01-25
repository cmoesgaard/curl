#ifndef HEADER_CARL_PRINTF_H
#define HEADER_CARL_PRINTF_H
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

/*
 * This header should be included by ALL code in libcarl that uses any
 * *rintf() functions.
 */

#include <carl/mprintf.h>

# undef printf
# undef fprintf
# undef msnprintf
# undef vprintf
# undef vfprintf
# undef vsnprintf
# undef aprintf
# undef vaprintf
# define printf carl_mprintf
# define fprintf carl_mfprintf
# define msnprintf carl_msnprintf
# define vprintf carl_mvprintf
# define vfprintf carl_mvfprintf
# define mvsnprintf carl_mvsnprintf
# define aprintf carl_maprintf
# define vaprintf carl_mvaprintf
#endif /* HEADER_CARL_PRINTF_H */
