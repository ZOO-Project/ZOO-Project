/*
 * Author : Gérald FENOY
 *
 *  Copyright 2008-2009 GeoLabs SARL. All rights reserved.
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

#ifndef _ULINET_H
#define _ULINET_H

#include "fcgi_stdio.h"
#include <stdlib.h>
#include <fcntl.h>
#include <curl/curl.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <string.h>
#include "time.h"
#ifdef USE_JS
#ifdef WIN32
#define XP_WIN 1
#else
#define XP_UNIX 0
#endif
#include "jsapi.h"
#endif
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#define false 0
#endif

#define MAX_REQ 50

#ifdef _ULINET
static char CCookie[MAX_REQ][1024];
#else
/**
 * The cookies found
 */
extern char CCookie[MAX_REQ][1024];
#endif

#ifdef __cplusplus
extern "C" {
#endif

  struct MemoryStruct {
    char *memory; //!< the memory space to store data 
    size_t size; //!< size of the memory space
  };

  /**
   * Individual CURL handler
   */
  typedef struct {
    CURL *handle; //!< the CURL handler
    struct curl_slist *header; //!< the headers to send
    char* filename; //!< the cached file name
    FILE* file; //!< the file pointer
    unsigned char *pabyData; //!< the downloaded content
    char *mimeType; //!< the mimeType returned by the server
    char *post; //!< the potential POST XML content
    int hasCacheFile; //!< 1 if we used a cache file
    int nDataLen; //!< the length of the downloaded content
    int nDataAlloc; //!< 
    long code; //!< the last received response code
    int id; //!< The position of the element in the queue
  } _HINTERNET;

  /**
   * Multiple CURL handlers
   */
  typedef struct {
    CURLM *handle; //!< the CURLM handler
    _HINTERNET ihandle[MAX_REQ]; //!< individual handlers in the queue 
    char *waitingRequests[MAX_REQ]; //!< request in the queue
    char *agent; //!< The User-Agent to use for HTTP request
    int nb; //!< number of element in the queue 
  } HINTERNET;

  size_t write_data_into(void*,size_t,size_t,void*);

  size_t header_write_data(void*,size_t,size_t,void*);

  void setProxy(CURL*,char*,long);

#if defined(macintosh) || (defined(__MACH__) && defined(__APPLE__))

#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
  char* CFStringToCString(CFStringRef,char*);
  OSStatus setProxiesForProtcol(CURL*,const char*);

#else

//#include <gconf/gconf-client.h>
  int setProxiesForProtcol(CURL*,const char*);

#endif


#define INTERNET_OPEN_TYPE_DIRECT                      0
#define INTERNET_OPEN_TYPE_PRECONFIG                   1
#define INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY 2
#define INTERNET_OPEN_TYPE_PROXY                       3
#ifndef WIN32
typedef char* LPCTSTR;
#endif
  HINTERNET InternetOpen(char*,int,char*,char*,int);

  void InternetCloseHandle(HINTERNET*);

#define INTERNET_FLAG_EXISTING_CONNECT         0
#define INTERNET_FLAG_HYPERLINK                1
#define INTERNET_FLAG_IGNORE_CERT_CN_INVALID   2
#define INTERNET_FLAG_IGNORE_CERT_DATE_INVALID 3
#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP  4
#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS 5
#define INTERNET_FLAG_KEEP_CONNECTION          6
#define INTERNET_FLAG_NEED_FILE                7
#define INTERNET_FLAG_NO_AUTH                  8
#define INTERNET_FLAG_NO_AUTO_REDIRECT         9
#define INTERNET_FLAG_NO_CACHE_WRITE          10
//typedef char* LPVOID;
#ifndef WIN32
  typedef void* LPVOID;
  typedef void* LPTSTR;
  typedef size_t* LPDWORD;
#endif
#ifndef bool
#define bool int
#endif

#  define CHECK_INET_HANDLE(h) (h.handle != 0)

  HINTERNET InternetOpenUrl(HINTERNET*,LPCTSTR,LPCTSTR,size_t,size_t,size_t);

  int processDownloads(HINTERNET*);

  int freeCookieList(HINTERNET);

  int InternetReadFile(_HINTERNET,LPVOID,int,size_t*);

  int setBasicAuth(HINTERNET,char*,char*);

#ifdef __cplusplus
}
#endif

#endif

