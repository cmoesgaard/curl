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
 * Uses the CARLINFO_TLS_SESSION data.
 * </DESC>
 */

/* Note that this example currently requires carl to be linked against
   GnuTLS (and this program must also be linked against -lgnutls). */

#include <stdio.h>

#include <carl/carl.h>
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

static CARL *carl;

static size_t wrfu(void *ptr, size_t size, size_t nmemb, void *stream)
{
  const struct carl_tlssessioninfo *info;
  unsigned int cert_list_size;
  const gnutls_datum_t *chainp;
  CARLcode res;

  (void)stream;
  (void)ptr;

  res = carl_easy_getinfo(carl, CARLINFO_TLS_SESSION, &info);

  if(!res) {
    switch(info->backend) {
    case CARLSSLBACKEND_GNUTLS:
      /* info->internals is now the gnutls_session_t */
      chainp = gnutls_certificate_get_peers(info->internals, &cert_list_size);
      if((chainp) && (cert_list_size)) {
        unsigned int i;

        for(i = 0; i < cert_list_size; i++) {
          gnutls_x509_crt_t cert;
          gnutls_datum_t dn;

          if(GNUTLS_E_SUCCESS == gnutls_x509_crt_init(&cert)) {
            if(GNUTLS_E_SUCCESS ==
               gnutls_x509_crt_import(cert, &chainp[i], GNUTLS_X509_FMT_DER)) {
              if(GNUTLS_E_SUCCESS ==
                 gnutls_x509_crt_print(cert, GNUTLS_CRT_PRINT_FULL, &dn)) {
                fprintf(stderr, "Certificate #%u: %.*s", i, dn.size, dn.data);

                gnutls_free(dn.data);
              }
            }

            gnutls_x509_crt_deinit(cert);
          }
        }
      }
      break;
    case CARLSSLBACKEND_NONE:
    default:
      break;
    }
  }

  return size * nmemb;
}

int main(void)
{
  carl_global_init(CARL_GLOBAL_DEFAULT);

  carl = carl_easy_init();
  if(carl) {
    carl_easy_setopt(carl, CARLOPT_URL, "https://www.example.com/");

    carl_easy_setopt(carl, CARLOPT_WRITEFUNCTION, wrfu);

    carl_easy_setopt(carl, CARLOPT_SSL_VERIFYPEER, 0L);
    carl_easy_setopt(carl, CARLOPT_SSL_VERIFYHOST, 0L);

    carl_easy_setopt(carl, CARLOPT_VERBOSE, 0L);

    (void) carl_easy_perform(carl);

    carl_easy_cleanup(carl);
  }

  carl_global_cleanup();

  return 0;
}
