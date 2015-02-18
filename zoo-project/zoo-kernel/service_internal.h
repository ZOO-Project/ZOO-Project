/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2013 GeoLabs SARL
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

/**
 * The default service url (overriden by serverAddress)
 */
#define DEFAULT_SERVICE_URL "http://www.zoo-project.org/"
/**
 * The time size
 */
#define TIME_SIZE 40

#include <libintl.h>
#include <locale.h>
/**
 * ZOO-Kernel internal messages translation function
 */
#define _(String) dgettext ("zoo-kernel",String)
/**
 * ZOO-Services messages translation function
 */
#define _ss(String) dgettext ("zoo-services",String)

/**
 * ZOO-Kernel was unable to create a lock
 */
#define ZOO_LOCK_CREATE_FAILED -4
/**
 * ZOO-Kernel was unable to acquire a lock
 */
#define ZOO_LOCK_ACQUIRE_FAILED -5
/**
 * ZOO-Kernel was unable to release a lock
 */
#define ZOO_LOCK_RELEASE_FAILED -6

#include <sys/stat.h>
#include <sys/types.h>
#include "cgic.h"
#ifndef WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#else
#include <direct.h>
#endif
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#ifdef WIN32
#ifndef USE_RUBY
#include <unistd.h>
#endif
#endif
#ifndef WIN32
#include <xlocale.h>
#endif
#include "ulinet.h"

#include "service.h"
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

extern   int conf_read(const char*,maps*);

#ifdef USE_JS
#ifdef WIN32
#define XP_WIN 1
#else
#define XP_UNIX 0
#endif
#include "service_internal_js.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif
#include <libxml/parser.h>
#include <libxml/xpath.h>

  /**
   * Maximum number of XML namespaces
   */
#define ZOO_NS_MAX 10
  /**
   * Maximum number of XML docs
   */
#define ZOO_DOC_MAX 20

  /**
   * Global char* to store the serverAddress value of the mmmmmain section
   */
  static char* SERVICE_URL;
  /**
   * Array of xmlNsPtr storing all used XML namespace
   */
  static xmlNsPtr usedNs[ZOO_NS_MAX];
  /**
   * Array storing names of the used XML namespace
   */
  static char* nsName[ZOO_NS_MAX];
  /**
   * Number of XML namespaces
   */
  static int nbNs=0;
  /**
   * Array of xmlDocPtr storing XML docs
   */
  static xmlDocPtr iDocs[ZOO_DOC_MAX];
  /**
   * Number of XML docs
   */
  static int nbDocs=0;

  int getServiceFromYAML(maps*,char*,service**,char *name);
  int readServiceFile(maps*, char*,service**,char *);
  int isValidLang(maps*,const char*);
  void addLangAttr(xmlNodePtr,maps*);

  void printHeaders(maps*);
  void unhandleStatus(maps*);
  int _updateStatus(maps*);
  char* _getStatus(maps*,int);
  char* getStatus(int);
  int removeShmLock(maps*, int);
  /**
   * Cross platform type used for Lock identifier
   */
#ifndef WIN32
#define semid int
#else
#define semid HANDLE
#endif
  semid getShmLockId(maps*,int);
  int lockShm(semid);
  int unlockShm(semid);

#ifdef USE_JS
#endif
  
  void readGeneratedFile(maps*,map*,char*);

  void URLDecode(char *);
  char *url_encode(char *);
  char *url_decode(char *);
  char* getEncoding(maps*);

  int zooXmlSearchForNs(const char*);
  int zooXmlAddNs(xmlNodePtr,const char*,const char*);
  void zooXmlCleanupNs();

  int zooXmlAddDoc(xmlNodePtr,const char*,const char*);
  void zooXmlCleanupDocs();
  
  void printExceptionReportResponse(maps*,map*);
  xmlNodePtr createExceptionReportNode(maps*,map*,int);
  void printProcessResponse(maps*,map*,int,service*,const char*,int,maps*,maps*);
  xmlNodePtr printWPSHeader(xmlDocPtr,maps*,const char*,const char*);
  xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr,maps*);
  void printGetCapabilitiesForProcess(maps*,xmlNodePtr,service*);
  void printDescribeProcessForProcess(maps*,xmlNodePtr,service*);
  void printFullDescription(int,elements*,const char*,xmlNsPtr,xmlNodePtr);
  void printDocument(maps*,xmlDocPtr,int);
  void printDescription(xmlNodePtr,xmlNsPtr,const char*,map*);
  void printIOType(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,xmlNsPtr,elements*,maps*,const char*);
  map* parseBoundingBox(const char*);
  void printBoundingBox(xmlNsPtr,xmlNodePtr,map*);
  void printBoundingBoxDocument(maps*,maps*,FILE*);
  void printOutputDefinitions(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,maps*,const char*);

  void outputResponse(service*,maps*,maps*,map*,int,maps*,int);

  char *base64(const char*,int);
  char *base64d(const char*,int,int*);
  void ensureDecodedBase64(maps**);
  void checkValidValue(map*,map**,const char*,const char**,int);
  char* addDefaultValues(maps**,elements*,maps*,int,map**);

  int errorException(maps *, const char *, const char *, const char*);

  xmlNodePtr soapEnvelope(maps*,xmlNodePtr);
  int checkForSoapEnvelope(xmlDocPtr);
  
  void addToCache(maps*,char*,char*,char*,int,char*,size_t);
  char* isInCache(maps*,char*);
  int runHttpRequests(maps**,maps**,HINTERNET*);
  int loadRemoteFile(maps**,map**,HINTERNET*,char*);

  char *readVSIFile(maps*,const char*);
  void parseIdentifier(maps*,char*,char*,char*);
  int updateStatus( maps*,const int,const char*);
  char* getInputValue( maps*,const char*,size_t*);
  int  setOutputValue( maps*, const char*, char*, size_t);
  
  char* getLastErrorMessage();
#ifdef __cplusplus
}
#endif

#endif
