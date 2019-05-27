/*
 * Author : Gérald Fenoy
 *
 *  Copyright 2008-2015 GeoLabs SARL. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef WIN32
#pragma comment(lib, "rpcrt4.lib")
#endif

#ifndef IMPORTSERVICE
#define IMPORTSERVICE "include" // default name of [include] block in main.cfg

#include "service.h"
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

extern   int conf_read(const char*,maps*);

#include <libxml/parser.h>
#include <libxml/xpath.h>

#ifdef __cplusplus
extern "C" {
#endif

  char *get_uuid();  
  char *base64(const char*,int);
  char *base64d(const char*,int,int*);
  void readBase64(map **);
  char *url_decode(char *);
  int getVersionId(const char*);
  void readGeneratedFile(maps*,map*,char*);
  int getServiceFromYAML(maps*,char*,service**,char *name);
  char* addDefaultValues(maps**,elements*,maps*,int,map**);
  char* getEncoding(maps*);
  void parseIdentifier(maps*,char*,char*,char*);
  void dumpMapsValuesToFiles(maps**,maps**);
  void runDismiss(maps*,char*);
  void runGetStatus(maps*,char*,char*);

  int isValidLang(maps*,const char*);
  
  char* getLastErrorMessage();
  int readServiceFile (maps *, char *, service **, char *);
  int createRegistry (maps*,registry **,char *);

  int zoo_path_compare(char* path1, char* path2);

#ifdef WIN32
  char* getMapsAsKVP(maps*,int,int);
#endif

#ifdef __cplusplus
}
#endif

#endif
