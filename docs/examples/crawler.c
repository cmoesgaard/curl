/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Web crawler based on carl and libxml2.
 * Copyright (C) 2018 - 2020 Jeroen Ooms <jeroenooms@gmail.com>
 * License: MIT
 *
 * To compile:
 *   gcc crawler.c $(pkg-config --cflags --libs libxml-2.0 libcarl)
 *
 */
/* <DESC>
 * Web crawler based on carl and libxml2 to stress-test carl with
 * hundreds of concurrent connections to various servers.
 * </DESC>
 */

/* Parameters */
int max_con = 200;
int max_total = 20000;
int max_requests = 500;
int max_link_per_page = 5;
int follow_relative_links = 0;
char *start_page = "https://www.reuters.com";

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <carl/carl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>

int pending_interrupt = 0;
void sighandler(int dummy)
{
  pending_interrupt = 1;
}

/* resizable buffer */
typedef struct {
  char *buf;
  size_t size;
} memory;

size_t grow_buffer(void *contents, size_t sz, size_t nmemb, void *ctx)
{
  size_t realsize = sz * nmemb;
  memory *mem = (memory*) ctx;
  char *ptr = realloc(mem->buf, mem->size + realsize);
  if(!ptr) {
    /* out of memory */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
  mem->buf = ptr;
  memcpy(&(mem->buf[mem->size]), contents, realsize);
  mem->size += realsize;
  return realsize;
}

CARL *make_handle(char *url)
{
  CARL *handle = carl_easy_init();

  /* Important: use HTTP2 over HTTPS */
  carl_easy_setopt(handle, CARLOPT_HTTP_VERSION, CARL_HTTP_VERSION_2TLS);
  carl_easy_setopt(handle, CARLOPT_URL, url);

  /* buffer body */
  memory *mem = malloc(sizeof(memory));
  mem->size = 0;
  mem->buf = malloc(1);
  carl_easy_setopt(handle, CARLOPT_WRITEFUNCTION, grow_buffer);
  carl_easy_setopt(handle, CARLOPT_WRITEDATA, mem);
  carl_easy_setopt(handle, CARLOPT_PRIVATE, mem);

  /* For completeness */
  carl_easy_setopt(handle, CARLOPT_ACCEPT_ENCODING, "");
  carl_easy_setopt(handle, CARLOPT_TIMEOUT, 5L);
  carl_easy_setopt(handle, CARLOPT_FOLLOWLOCATION, 1L);
  carl_easy_setopt(handle, CARLOPT_MAXREDIRS, 10L);
  carl_easy_setopt(handle, CARLOPT_CONNECTTIMEOUT, 2L);
  carl_easy_setopt(handle, CARLOPT_COOKIEFILE, "");
  carl_easy_setopt(handle, CARLOPT_FILETIME, 1L);
  carl_easy_setopt(handle, CARLOPT_USERAGENT, "mini crawler");
  carl_easy_setopt(handle, CARLOPT_HTTPAUTH, CARLAUTH_ANY);
  carl_easy_setopt(handle, CARLOPT_UNRESTRICTED_AUTH, 1L);
  carl_easy_setopt(handle, CARLOPT_PROXYAUTH, CARLAUTH_ANY);
  carl_easy_setopt(handle, CARLOPT_EXPECT_100_TIMEOUT_MS, 0L);
  return handle;
}

/* HREF finder implemented in libxml2 but could be any HTML parser */
size_t follow_links(CARLM *multi_handle, memory *mem, char *url)
{
  int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | \
             HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
  htmlDocPtr doc = htmlReadMemory(mem->buf, mem->size, url, NULL, opts);
  if(!doc)
    return 0;
  xmlChar *xpath = (xmlChar*) "//a/@href";
  xmlXPathContextPtr context = xmlXPathNewContext(doc);
  xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context);
  xmlXPathFreeContext(context);
  if(!result)
    return 0;
  xmlNodeSetPtr nodeset = result->nodesetval;
  if(xmlXPathNodeSetIsEmpty(nodeset)) {
    xmlXPathFreeObject(result);
    return 0;
  }
  size_t count = 0;
  int i;
  for(i = 0; i < nodeset->nodeNr; i++) {
    double r = rand();
    int x = r * nodeset->nodeNr / RAND_MAX;
    const xmlNode *node = nodeset->nodeTab[x]->xmlChildrenNode;
    xmlChar *href = xmlNodeListGetString(doc, node, 1);
    if(follow_relative_links) {
      xmlChar *orig = href;
      href = xmlBuildURI(href, (xmlChar *) url);
      xmlFree(orig);
    }
    char *link = (char *) href;
    if(!link || strlen(link) < 20)
      continue;
    if(!strncmp(link, "http://", 7) || !strncmp(link, "https://", 8)) {
      carl_multi_add_handle(multi_handle, make_handle(link));
      if(count++ == max_link_per_page)
        break;
    }
    xmlFree(link);
  }
  xmlXPathFreeObject(result);
  return count;
}

int is_html(char *ctype)
{
  return ctype != NULL && strlen(ctype) > 10 && strstr(ctype, "text/html");
}

int main(void)
{
  signal(SIGINT, sighandler);
  LIBXML_TEST_VERSION;
  carl_global_init(CARL_GLOBAL_DEFAULT);
  CARLM *multi_handle = carl_multi_init();
  carl_multi_setopt(multi_handle, CARLMOPT_MAX_TOTAL_CONNECTIONS, max_con);
  carl_multi_setopt(multi_handle, CARLMOPT_MAX_HOST_CONNECTIONS, 6L);

  /* enables http/2 if available */
#ifdef CARLPIPE_MULTIPLEX
  carl_multi_setopt(multi_handle, CARLMOPT_PIPELINING, CARLPIPE_MULTIPLEX);
#endif

  /* sets html start page */
  carl_multi_add_handle(multi_handle, make_handle(start_page));

  int msgs_left;
  int pending = 0;
  int complete = 0;
  int still_running = 1;
  while(still_running && !pending_interrupt) {
    int numfds;
    carl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
    carl_multi_perform(multi_handle, &still_running);

    /* See how the transfers went */
    CARLMsg *m = NULL;
    while((m = carl_multi_info_read(multi_handle, &msgs_left))) {
      if(m->msg == CARLMSG_DONE) {
        CARL *handle = m->easy_handle;
        char *url;
        memory *mem;
        carl_easy_getinfo(handle, CARLINFO_PRIVATE, &mem);
        carl_easy_getinfo(handle, CARLINFO_EFFECTIVE_URL, &url);
        if(m->data.result == CARLE_OK) {
          long res_status;
          carl_easy_getinfo(handle, CARLINFO_RESPONSE_CODE, &res_status);
          if(res_status == 200) {
            char *ctype;
            carl_easy_getinfo(handle, CARLINFO_CONTENT_TYPE, &ctype);
            printf("[%d] HTTP 200 (%s): %s\n", complete, ctype, url);
            if(is_html(ctype) && mem->size > 100) {
              if(pending < max_requests && (complete + pending) < max_total) {
                pending += follow_links(multi_handle, mem, url);
                still_running = 1;
              }
            }
          }
          else {
            printf("[%d] HTTP %d: %s\n", complete, (int) res_status, url);
          }
        }
        else {
          printf("[%d] Connection failure: %s\n", complete, url);
        }
        carl_multi_remove_handle(multi_handle, handle);
        carl_easy_cleanup(handle);
        free(mem->buf);
        free(mem);
        complete++;
        pending--;
      }
    }
  }
  carl_multi_cleanup(multi_handle);
  carl_global_cleanup();
  return 0;
}
