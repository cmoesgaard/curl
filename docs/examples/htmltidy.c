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
 * Download a document and use libtidy to parse the HTML.
 * </DESC>
 */
/*
 * LibTidy => https://www.html-tidy.org/
 */

#include <stdio.h>
#include <tidy.h>
#include <tidybuffio.h>
#include <carl/carl.h>

/* carl write callback, to fill tidy's input buffer...  */
uint write_cb(char *in, uint size, uint nmemb, TidyBuffer *out)
{
  uint r;
  r = size * nmemb;
  tidyBufAppend(out, in, r);
  return r;
}

/* Traverse the document tree */
void dumpNode(TidyDoc doc, TidyNode tnod, int indent)
{
  TidyNode child;
  for(child = tidyGetChild(tnod); child; child = tidyGetNext(child) ) {
    ctmbstr name = tidyNodeGetName(child);
    if(name) {
      /* if it has a name, then it's an HTML tag ... */
      TidyAttr attr;
      printf("%*.*s%s ", indent, indent, "<", name);
      /* walk the attribute list */
      for(attr = tidyAttrFirst(child); attr; attr = tidyAttrNext(attr) ) {
        printf(tidyAttrName(attr));
        tidyAttrValue(attr)?printf("=\"%s\" ",
                                   tidyAttrValue(attr)):printf(" ");
      }
      printf(">\n");
    }
    else {
      /* if it doesn't have a name, then it's probably text, cdata, etc... */
      TidyBuffer buf;
      tidyBufInit(&buf);
      tidyNodeGetText(doc, child, &buf);
      printf("%*.*s\n", indent, indent, buf.bp?(char *)buf.bp:"");
      tidyBufFree(&buf);
    }
    dumpNode(doc, child, indent + 4); /* recursive */
  }
}


int main(int argc, char **argv)
{
  if(argc == 2) {
    CARL *carl;
    char carl_errbuf[CARL_ERROR_SIZE];
    TidyDoc tdoc;
    TidyBuffer docbuf = {0};
    TidyBuffer tidy_errbuf = {0};
    int err;

    carl = carl_easy_init();
    carl_easy_setopt(carl, CARLOPT_URL, argv[1]);
    carl_easy_setopt(carl, CARLOPT_ERRORBUFFER, carl_errbuf);
    carl_easy_setopt(carl, CARLOPT_NOPROGRESS, 0L);
    carl_easy_setopt(carl, CARLOPT_VERBOSE, 1L);
    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, write_cb);

    tdoc = tidyCreate();
    tidyOptSetBool(tdoc, TidyForceOutput, yes); /* try harder */
    tidyOptSetInt(tdoc, TidyWrapLen, 4096);
    tidySetErrorBuffer(tdoc, &tidy_errbuf);
    tidyBufInit(&docbuf);

    carl_easy_setopt(carl, CARLOPT_WRITEDATA, &docbuf);
    err = carl_easy_perform(carl);
    if(!err) {
      err = tidyParseBuffer(tdoc, &docbuf); /* parse the input */
      if(err >= 0) {
        err = tidyCleanAndRepair(tdoc); /* fix any problems */
        if(err >= 0) {
          err = tidyRunDiagnostics(tdoc); /* load tidy error buffer */
          if(err >= 0) {
            dumpNode(tdoc, tidyGetRoot(tdoc), 0); /* walk the tree */
            fprintf(stderr, "%s\n", tidy_errbuf.bp); /* show errors */
          }
        }
      }
    }
    else
      fprintf(stderr, "%s\n", carl_errbuf);

    /* clean-up */
    carl_easy_cleanup(carl);
    tidyBufFree(&docbuf);
    tidyBufFree(&tidy_errbuf);
    tidyRelease(tdoc);
    return err;

  }
  else
    printf("usage: %s <url>\n", argv[0]);

  return 0;
}
