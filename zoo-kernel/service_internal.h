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

  void URLDecode(char *);
  char *url_encode(char *);
  
  void printExceptionReportResponse(maps*,map*);
  void printProcessResponse1(maps*,map*,int,service*,char*,int,maps*,maps*);
  void printProcessResponse(maps*,int,service*,char*,int,map*,map*);
  void printGetCapabilitiesResponse(service**,int,char*,maps*);
  xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr,char*,maps*);
  void printGetCapabilitiesForProcess(maps*,xmlNodePtr,service*);
  void printGetCapabilitiesDocument(xmlDocPtr);
  void printDescribeProcessResponse(service*,char*);
  xmlNodePtr printDescribeProcessHeader(xmlDocPtr,char*,maps*);
  void printDescribeProcessForProcess(maps*,xmlNodePtr,service**,int);
  void printDescribeProcessDocument(xmlDocPtr);
  void printDocument(xmlDocPtr);
  void printDescription(xmlNodePtr,xmlNsPtr,char*,map*);
  void printIOType1(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,maps*,char*);
  void printIOType(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,map*,char*);
  void printOutputDefinitions1(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,maps*,char*);
  void printOutputDefinitions(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,map*,char*);
  
  void outputResponse(service*,maps*,maps*,map*,int,maps*,int);

  char *base64(const unsigned char*,int);

  void addDefaultValues(maps**,elements*,maps*,char*);

#ifdef __cplusplus
}
#endif

#endif
