#ifndef HEADER_CARL_CARLX_H
#define HEADER_CARL_CARLX_H
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
 * Defines protos and includes all header files that provide the carlx_*
 * functions. The carlx_* functions are not part of the libcarl API, but are
 * stand-alone functions whose sources can be built and linked by apps if need
 * be.
 */

#include <carl/mprintf.h>
/* this is still a public header file that provides the carl_mprintf()
   functions while they still are offered publicly. They will be made library-
   private one day */

#include "strcase.h"
/* "strcase.h" provides the strcasecompare protos */

#include "strtoofft.h"
/* "strtoofft.h" provides this function: carlx_strtoofft(), returns a
   carl_off_t number from a given string.
*/

#include "nonblock.h"
/* "nonblock.h" provides carlx_nonblock() */

#include "warnless.h"
/* "warnless.h" provides functions:

  carlx_ultous()
  carlx_ultouc()
  carlx_uztosi()
*/

#include "carl_multibyte.h"
/* "carl_multibyte.h" provides these functions and macros:

  carlx_convert_UTF8_to_wchar()
  carlx_convert_wchar_to_UTF8()
  carlx_convert_UTF8_to_tchar()
  carlx_convert_tchar_to_UTF8()
  carlx_unicodefree()
*/

#include "version_win32.h"
/* "version_win32.h" provides carlx_verify_windows_version() */

/* Now setup carlx_ * names for the functions that are to become carlx_ and
   be removed from a future libcarl official API:
   carlx_getenv
   carlx_mprintf (and its variations)
   carlx_strcasecompare
   carlx_strncasecompare

*/

#define carlx_getenv carl_getenv
#define carlx_mvsnprintf carl_mvsnprintf
#define carlx_msnprintf carl_msnprintf
#define carlx_maprintf carl_maprintf
#define carlx_mvaprintf carl_mvaprintf
#define carlx_msprintf carl_msprintf
#define carlx_mprintf carl_mprintf
#define carlx_mfprintf carl_mfprintf
#define carlx_mvsprintf carl_mvsprintf
#define carlx_mvprintf carl_mvprintf
#define carlx_mvfprintf carl_mvfprintf

#ifdef ENABLE_CARLX_PRINTF
/* If this define is set, we define all "standard" printf() functions to use
   the carlx_* version instead. It makes the source code transparent and
   easier to understand/patch. Undefine them first. */
# undef printf
# undef fprintf
# undef sprintf
# undef msnprintf
# undef vprintf
# undef vfprintf
# undef vsprintf
# undef mvsnprintf
# undef aprintf
# undef vaprintf

# define printf carlx_mprintf
# define fprintf carlx_mfprintf
# define sprintf carlx_msprintf
# define msnprintf carlx_msnprintf
# define vprintf carlx_mvprintf
# define vfprintf carlx_mvfprintf
# define mvsnprintf carlx_mvsnprintf
# define aprintf carlx_maprintf
# define vaprintf carlx_mvaprintf
#endif /* ENABLE_CARLX_PRINTF */

#endif /* HEADER_CARL_CARLX_H */
