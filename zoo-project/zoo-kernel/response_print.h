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

#ifndef ZOO_RESPONSE_PRINT_H
#define ZOO_RESPONSE_PRINT_H 1

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
//#include <locale.h>
#include <locale.h> // knut: this appears to be a non-standard header file that has been removed in newer versions of glibc; it may be sufficient to include <locale.h> (see above) 
#endif
#include "ulinet.h"

#include "service.h"
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "cgic.h"

#ifdef __cplusplus
extern "C" {
#endif
  /**
   * Maximum number of XML namespaces
   */
#define ZOO_NS_MAX 10
  /**
   * Maximum number of XML docs
   */
#define ZOO_DOC_MAX 20

  /**
   * Global char* to store the serverAddress value of the [main] section
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

  /**
   * Definitions of acceptable final status
   */
  static char wpsStatus[3][11]={
    "Succeeded",
    "Failed",
    "Running"
  };
  /**
   * Definitions of schemas depending on the WPS version
   */
  static const char* schemas[2][8]={
    {"1.0.0","http://www.opengis.net/ows/1.1","http://www.opengis.net/wps/1.0.0","http://schemas.opengis.net/wps/1.0.0","%s %s/wps%s_response.xsd","http://schemas.opengis.net/ows/1.1.0/owsExceptionReport.xsd","1.1.0"},
    {"2.0.0","http://www.opengis.net/ows/2.0","http://www.opengis.net/wps/2.0","http://schemas.opengis.net/wps/2.0","http://www.opengis.net/wps/2.0 http://schemas.opengis.net/wps/2.0/wps.xsd","http://schemas.opengis.net/ows/2.0/owsExceptionReport.xsd","2.0.2","http://www.opengis.net/spec/wps/2.0/def/process-profile/"},
  };
  /**
   * Definitions of support requests (depending on the WPS version)
   */
  static int nbSupportedRequests=7;
  /**
   * Definitions of requests depending on the WPS version
   */
  static const char* requests[2][7]={
    {"GetCapabilities","DescribeProcess","Execute",NULL},
    {"GetCapabilities","DescribeProcess","Execute","GetStatus","GetResult","Dismiss",NULL},
  };
  /**
   * Definitions requests requiring identifier (depending on the WPS version)
   */
  static int nbReqIdentifier=2;
  /**
   * Definitions requests requiring jobid (only for WPS version 2.0.0)
   */
  static int nbReqJob=3;
  /**
   * Definitions of root node for response depending on the request and the WPS version
   */
  static const char root_nodes[2][4][20]={
    {"ProcessOfferings","ProcessDescriptions","ExecuteResponse",NULL},
    {"Contents","ProcessOfferings","Result",NULL}
  };

  /**
   * Name and corresponding attributes depending on the WPS version
   */
  static const char* capabilities[2][7]={
    {
      "Process",
      "processVersion","1",
      "storeSupported","true",
      "statusSupported","true"
    },
    {
      "ProcessSummary",
      "processVersion","1.0.0",
      "jobControlOptions","sync-execute async-execute dismiss",
      "outputTransmission","value reference"
    }
  };

  void addLangAttr(xmlNodePtr,maps*);

  void printHeaders(maps*);

  int zooXmlSearchForNs(const char*);
  int zooXmlAddNs(xmlNodePtr,const char*,const char*);
  void zooXmlCleanupNs();

  int zooXmlAddDoc(xmlNodePtr,const char*,const char*);
  void zooXmlCleanupDocs();
  
  void printExceptionReportResponse(maps*,map*);
  xmlNodePtr createExceptionReportNode(maps*,map*,int);
  void printProcessResponse(maps*,map*,int,service*,const char*,int,maps*,maps*);
  xmlNodePtr printWPSHeader(xmlDocPtr,maps*,const char*,const char*,const char*,int);
  xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr,maps*,const char*);
  void printGetCapabilitiesForProcess(registry*,maps*,xmlDocPtr,xmlNodePtr,service*);
  void printDescribeProcessForProcess(registry*,maps*,xmlDocPtr,xmlNodePtr,service*);
  void printFullDescription(xmlDocPtr,int,elements*,const char*,xmlNsPtr,xmlNsPtr,xmlNodePtr,int,int,const map*);
  void printDocument(maps*,xmlDocPtr,int);
  void printDescription(xmlNodePtr,xmlNsPtr,const char*,map*,int);
  void printIOType(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,xmlNsPtr,elements*,maps*,const char*,int);
  map* parseBoundingBox(const char*);
  void printBoundingBox(xmlNsPtr,xmlNodePtr,map*);
  void printBoundingBoxDocument(maps*,maps*,FILE*);
  void printOutputDefinitions(xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,elements*,maps*,const char*);
  void printStatusInfo(maps*,map*,char*);
  void addAdditionalParameters(map*,xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,int);
  void addMetadata(map*,xmlDocPtr,xmlNodePtr,xmlNsPtr,xmlNsPtr,int);

  void outputResponse(service*,maps*,maps*,map*,int,maps*,int);

  int errorException(maps *, const char *, const char *, const char*);

  xmlNodePtr soapEnvelope(maps*,xmlNodePtr);
  int checkForSoapEnvelope(xmlDocPtr);
  
#ifdef __cplusplus
}
#endif

#endif
