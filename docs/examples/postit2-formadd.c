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
 * HTTP Multipart formpost with file upload and two additional parts.
 * </DESC>
 */
/* Example code that uploads a file name 'foo' to a remote script that accepts
 * "HTML form based" (as described in RFC1738) uploads using HTTP POST.
 *
 * The imaginary form we'll fill in looks like:
 *
 * <form method="post" enctype="multipart/form-data" action="examplepost.cgi">
 * Enter file: <input type="file" name="sendfile" size="40">
 * Enter file name: <input type="text" name="filename" size="30">
 * <input type="submit" value="send" name="submit">
 * </form>
 *
 */

#include <stdio.h>
#include <string.h>

#include <carl/carl.h>

int main(int argc, char *argv[])
{
  CARL *carl;
  CARLcode res;

  struct carl_httppost *formpost = NULL;
  struct carl_httppost *lastptr = NULL;
  struct carl_slist *headerlist = NULL;
  static const char buf[] = "Expect:";

  carl_global_init(CARL_GLOBAL_ALL);

  /* Fill in the file upload field */
  carl_formadd(&formpost,
               &lastptr,
               CARLFORM_COPYNAME, "sendfile",
               CARLFORM_FILE, "postit2.c",
               CARLFORM_END);

  /* Fill in the filename field */
  carl_formadd(&formpost,
               &lastptr,
               CARLFORM_COPYNAME, "filename",
               CARLFORM_COPYCONTENTS, "postit2.c",
               CARLFORM_END);


  /* Fill in the submit field too, even if this is rarely needed */
  carl_formadd(&formpost,
               &lastptr,
               CARLFORM_COPYNAME, "submit",
               CARLFORM_COPYCONTENTS, "send",
               CARLFORM_END);

  carl = carl_easy_init();
  /* initialize custom header list (stating that Expect: 100-continue is not
     wanted */
  headerlist = carl_slist_append(headerlist, buf);
  if(carl) {
    /* what URL that receives this POST */
    carl_easy_setopt(carl, CARLOPT_URL, "https://example.com/examplepost.cgi");
    if((argc == 2) && (!strcmp(argv[1], "noexpectheader")))
      /* only disable 100-continue header if explicitly requested */
      carl_easy_setopt(carl, CARLOPT_HTTPHEADER, headerlist);
    carl_easy_setopt(carl, CARLOPT_HTTPPOST, formpost);

    /* Perform the request, res will get the return code */
    res = carl_easy_perform(carl);
    /* Check for errors */
    if(res != CARLE_OK)
      fprintf(stderr, "carl_easy_perform() failed: %s\n",
              carl_easy_strerror(res));

    /* always cleanup */
    carl_easy_cleanup(carl);

    /* then cleanup the formpost chain */
    carl_formfree(formpost);
    /* free slist */
    carl_slist_free_all(headerlist);
  }
  return 0;
}
