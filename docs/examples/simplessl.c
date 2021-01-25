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
 * Shows HTTPS usage with client certs and optional ssl engine use.
 * </DESC>
 */
#include <stdio.h>

#include <carl/carl.h>

/* some requirements for this to work:
   1.   set pCertFile to the file with the client certificate
   2.   if the key is passphrase protected, set pPassphrase to the
        passphrase you use
   3.   if you are using a crypto engine:
   3.1. set a #define USE_ENGINE
   3.2. set pEngine to the name of the crypto engine you use
   3.3. set pKeyName to the key identifier you want to use
   4.   if you don't use a crypto engine:
   4.1. set pKeyName to the file name of your client key
   4.2. if the format of the key file is DER, set pKeyType to "DER"

   !! verify of the server certificate is not implemented here !!

   **** This example only works with libcarl 7.9.3 and later! ****

*/

int main(void)
{
  CARL *carl;
  CARLcode res;
  FILE *headerfile;
  const char *pPassphrase = NULL;

  static const char *pCertFile = "testcert.pem";
  static const char *pCACertFile = "cacert.pem";
  static const char *pHeaderFile = "dumpit";

  const char *pKeyName;
  const char *pKeyType;

  const char *pEngine;

#ifdef USE_ENGINE
  pKeyName  = "rsa_test";
  pKeyType  = "ENG";
  pEngine   = "chil";            /* for nChiper HSM... */
#else
  pKeyName  = "testkey.pem";
  pKeyType  = "PEM";
  pEngine   = NULL;
#endif

  headerfile = fopen(pHeaderFile, "wb");

  carl_global_init(CARL_GLOBAL_DEFAULT);

  carl = carl_easy_init();
  if(carl) {
    /* what call to write: */
    carl_easy_setopt(carl, CARLOPT_URL, "HTTPS://your.favourite.ssl.site");
    carl_easy_setopt(carl, CARLOPT_HEADERDATA, headerfile);

    do { /* dummy loop, just to break out from */
      if(pEngine) {
        /* use crypto engine */
        if(carl_easy_setopt(carl, CARLOPT_SSLENGINE, pEngine) != CARLE_OK) {
          /* load the crypto engine */
          fprintf(stderr, "can't set crypto engine\n");
          break;
        }
        if(carl_easy_setopt(carl, CARLOPT_SSLENGINE_DEFAULT, 1L) != CARLE_OK) {
          /* set the crypto engine as default */
          /* only needed for the first time you load
             a engine in a carl object... */
          fprintf(stderr, "can't set crypto engine as default\n");
          break;
        }
      }
      /* cert is stored PEM coded in file... */
      /* since PEM is default, we needn't set it for PEM */
      carl_easy_setopt(carl, CARLOPT_SSLCERTTYPE, "PEM");

      /* set the cert for client authentication */
      carl_easy_setopt(carl, CARLOPT_SSLCERT, pCertFile);

      /* sorry, for engine we must set the passphrase
         (if the key has one...) */
      if(pPassphrase)
        carl_easy_setopt(carl, CARLOPT_KEYPASSWD, pPassphrase);

      /* if we use a key stored in a crypto engine,
         we must set the key type to "ENG" */
      carl_easy_setopt(carl, CARLOPT_SSLKEYTYPE, pKeyType);

      /* set the private key (file or ID in engine) */
      carl_easy_setopt(carl, CARLOPT_SSLKEY, pKeyName);

      /* set the file with the certs vaildating the server */
      carl_easy_setopt(carl, CARLOPT_CAINFO, pCACertFile);

      /* disconnect if we can't validate server's cert */
      carl_easy_setopt(carl, CARLOPT_SSL_VERIFYPEER, 1L);

      /* Perform the request, res will get the return code */
      res = carl_easy_perform(carl);
      /* Check for errors */
      if(res != CARLE_OK)
        fprintf(stderr, "carl_easy_perform() failed: %s\n",
                carl_easy_strerror(res));

      /* we are done... */
    } while(0);
    /* always cleanup */
    carl_easy_cleanup(carl);
  }

  carl_global_cleanup();

  return 0;
}
