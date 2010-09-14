/**
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2010 GeoLabs SARL
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

#ifndef ZOO_SERVICE_INTERNAL_H
#define ZOO_SERVICE_INTERNAL_H 1

#pragma once 

#define DEFAULT_SERVICE_URL "http://dev.geolabs.fr/zoo-wps/"
#define TIME_SIZE 40

#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#ifndef WIN32
#include <xlocale.h>
#endif
#include "service.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

extern   int getServiceFromFile(char*,service**);
extern   int conf_read(char*,maps*);

#ifdef __cplusplus
extern "C" {
#endif
#include <libxml/parser.h>
  static char* SERVICE_URL;
  static xmlNsPtr usedNs[5];
  static char* nsName[5];
  static int nbNs=0;


  void URLDecode(char *);
  char *url_encode(char *);
  char* getEncoding(maps*);

  int zooXmlSearchForNs(char*);
  int zooXmlAddNs(xmlNodePtr,char*,char*);
  void zooXmlCleanupNs();
  
  void printExceptionReportResponse(maps*,map*);
  void printProcessResponse(maps*,map*,int,service*,char*,int,maps*,maps*);
  xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr,char*,maps*);
  void printGetCapabilitiesForProcess(maps*,xmlNodePtr,service*);
  xmlNodePtr printDescribeProcessHeader(xmlDocPtr,char*,maps*);
  void printDescribeProcessForProcess(maps*,xmlNodePtr,service*,int);
  void printDocument(maps*,xmlDocPtr,int);
  void printDescription(xmlNodePtr,xmlNsPtr,char*,map*);
  void printIOType(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,maps*,char*);
  void printOutputDefinitions1(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,maps*,char*);
  
  void outputResponse(service*,maps*,maps*,map*,int,maps*,int);

  char *base64(const unsigned char*,int);

  char* addDefaultValues(maps**,elements*,maps*,char*);

  /*defined in zoo_loader.c*/ 
  int errorException(maps *m, const char *message, const char *errorcode);

#ifdef __cplusplus
}
#endif

#endif
