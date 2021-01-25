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
 * Get a web page, extract the title with libxml.
 * </DESC>

 Written by Lars Nilsson

 GNU C++ compile command line suggestion (edit paths accordingly):

 g++ -Wall -I/opt/carl/include -I/opt/libxml/include/libxml2 htmltitle.cpp \
 -o htmltitle -L/opt/carl/lib -L/opt/libxml/lib -lcarl -lxml2
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <carl/carl.h>
#include <libxml/HTMLparser.h>

//
//  Case-insensitive string comparison
//

#ifdef _MSC_VER
#define COMPARE(a, b) (!_stricmp((a), (b)))
#else
#define COMPARE(a, b) (!strcasecmp((a), (b)))
#endif

//
//  libxml callback context structure
//

struct Context
{
  Context(): addTitle(false) { }

  bool addTitle;
  std::string title;
};

//
//  libcarl variables for error strings and returned data

static char errorBuffer[CARL_ERROR_SIZE];
static std::string buffer;

//
//  libcarl write callback function
//

static int writer(char *data, size_t size, size_t nmemb,
                  std::string *writerData)
{
  if(writerData == NULL)
    return 0;

  writerData->append(data, size*nmemb);

  return size * nmemb;
}

//
//  libcarl connection initialization
//

static bool init(CARL *&conn, char *url)
{
  CARLcode code;

  conn = carl_easy_init();

  if(conn == NULL) {
    fprintf(stderr, "Failed to create CARL connection\n");
    exit(EXIT_FAILURE);
  }

  code = carl_easy_setopt(conn, CARLOPT_ERRORBUFFER, errorBuffer);
  if(code != CARLE_OK) {
    fprintf(stderr, "Failed to set error buffer [%d]\n", code);
    return false;
  }

  code = carl_easy_setopt(conn, CARLOPT_URL, url);
  if(code != CARLE_OK) {
    fprintf(stderr, "Failed to set URL [%s]\n", errorBuffer);
    return false;
  }

  code = carl_easy_setopt(conn, CARLOPT_FOLLOWLOCATION, 1L);
  if(code != CARLE_OK) {
    fprintf(stderr, "Failed to set redirect option [%s]\n", errorBuffer);
    return false;
  }

  code = carl_easy_setopt(conn, CARLOPT_WRITEFUNCTION, writer);
  if(code != CARLE_OK) {
    fprintf(stderr, "Failed to set writer [%s]\n", errorBuffer);
    return false;
  }

  code = carl_easy_setopt(conn, CARLOPT_WRITEDATA, &buffer);
  if(code != CARLE_OK) {
    fprintf(stderr, "Failed to set write data [%s]\n", errorBuffer);
    return false;
  }

  return true;
}

//
//  libxml start element callback function
//

static void StartElement(void *voidContext,
                         const xmlChar *name,
                         const xmlChar **attributes)
{
  Context *context = static_cast<Context *>(voidContext);

  if(COMPARE(reinterpret_cast<char *>(name), "TITLE")) {
    context->title = "";
    context->addTitle = true;
  }
  (void) attributes;
}

//
//  libxml end element callback function
//

static void EndElement(void *voidContext,
                       const xmlChar *name)
{
  Context *context = static_cast<Context *>(voidContext);

  if(COMPARE(reinterpret_cast<char *>(name), "TITLE"))
    context->addTitle = false;
}

//
//  Text handling helper function
//

static void handleCharacters(Context *context,
                             const xmlChar *chars,
                             int length)
{
  if(context->addTitle)
    context->title.append(reinterpret_cast<char *>(chars), length);
}

//
//  libxml PCDATA callback function
//

static void Characters(void *voidContext,
                       const xmlChar *chars,
                       int length)
{
  Context *context = static_cast<Context *>(voidContext);

  handleCharacters(context, chars, length);
}

//
//  libxml CDATA callback function
//

static void cdata(void *voidContext,
                  const xmlChar *chars,
                  int length)
{
  Context *context = static_cast<Context *>(voidContext);

  handleCharacters(context, chars, length);
}

//
//  libxml SAX callback structure
//

static htmlSAXHandler saxHandler =
{
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  StartElement,
  EndElement,
  NULL,
  Characters,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  cdata,
  NULL
};

//
//  Parse given (assumed to be) HTML text and return the title
//

static void parseHtml(const std::string &html,
                      std::string &title)
{
  htmlParserCtxtPtr ctxt;
  Context context;

  ctxt = htmlCreatePushParserCtxt(&saxHandler, &context, "", 0, "",
                                  XML_CHAR_ENCODING_NONE);

  htmlParseChunk(ctxt, html.c_str(), html.size(), 0);
  htmlParseChunk(ctxt, "", 0, 1);

  htmlFreeParserCtxt(ctxt);

  title = context.title;
}

int main(int argc, char *argv[])
{
  CARL *conn = NULL;
  CARLcode code;
  std::string title;

  // Ensure one argument is given

  if(argc != 2) {
    fprintf(stderr, "Usage: %s <url>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  carl_global_init(CARL_GLOBAL_DEFAULT);

  // Initialize CARL connection

  if(!init(conn, argv[1])) {
    fprintf(stderr, "Connection initializion failed\n");
    exit(EXIT_FAILURE);
  }

  // Retrieve content for the URL

  code = carl_easy_perform(conn);
  carl_easy_cleanup(conn);

  if(code != CARLE_OK) {
    fprintf(stderr, "Failed to get '%s' [%s]\n", argv[1], errorBuffer);
    exit(EXIT_FAILURE);
  }

  // Parse the (assumed) HTML code
  parseHtml(buffer, title);

  // Display the extracted title
  printf("Title: %s\n", title.c_str());

  return EXIT_SUCCESS;
}
