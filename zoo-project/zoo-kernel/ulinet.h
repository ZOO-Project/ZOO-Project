/**
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
#define false -1
#endif

#define MAX_REQ 50

#ifdef _ULINET
static char CCookie[MAX_REQ][1024];
#else
extern char HEADER[MAX_REQ][3072];
extern char CCookie[MAX_REQ][1024];
#endif

#ifdef __cplusplus
extern "C" {
#endif

  //static pthread_mutex_t hMutexConnect = PTHREAD_MUTEX_INITIALIZER;

static char* waitingRequests[MAX_REQ];

struct MemoryStruct {
  char *memory;
  size_t size;
};

typedef struct {
  CURL *handle;
  struct curl_slist *header;
  char* filename;
  FILE* file;
  size_t size;
  unsigned char *pabyData;
  unsigned char *mimeType;
  int hasCacheFile;
  int nDataLen;
  int nDataAlloc;
  int id;
} _HINTERNET;

typedef struct {
  CURLM *handle;
  _HINTERNET ihandle[MAX_REQ];
  char *waitingRequests[MAX_REQ];
  char *agent;
  int nb;
} HINTERNET;

size_t write_data_into(void *buffer, size_t size, size_t nmemb, void *data);

size_t content_write_data(void *buffer, size_t size, size_t nmemb, void *data);

size_t header_write_data(void *buffer, size_t size, size_t nmemb, void *data);


void setProxy(CURL* handle,char* host,long port);


#if defined(macintosh) || (defined(__MACH__) && defined(__APPLE__))

#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>
char* CFStringToCString(CFStringRef dest,char * buffer);
OSStatus setProxiesForProtcol(CURL* handle,const char *proto);

#else

//#include <gconf/gconf-client.h>
int setProxiesForProtcol(CURL* handle,const char *proto);

#endif


#define INTERNET_OPEN_TYPE_DIRECT                      0
#define INTERNET_OPEN_TYPE_PRECONFIG                   1
#define INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY 2
#define INTERNET_OPEN_TYPE_PROXY                       3
#ifndef WIN32
typedef char* LPCTSTR;
#endif
HINTERNET InternetOpen(char* lpszAgent,int dwAccessType,char* lpszProxyName,char* lpszProxyBypass,int dwFlags);

void InternetCloseHandle(HINTERNET* handle);

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

HINTERNET InternetOpenUrl(HINTERNET* hInternet,LPCTSTR lpszUrl,LPCTSTR lpszHeaders,size_t dwHeadersLength,size_t dwFlags,size_t dwContext);

int processDownloads(HINTERNET* hInternet);

int freeCookieList(HINTERNET hInternet);

int InternetReadFile(_HINTERNET hInternet,LPVOID lpBuffer,int dwNumberOfBytesToRead,size_t *lpdwNumberOfBytesRead);

#ifdef __cplusplus
}
#endif

#endif

