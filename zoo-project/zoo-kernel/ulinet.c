/**
 *  ulinet.c
 *
 * Author : Gérald FENOY
 *
 * Copyright (c) 2008-2010 GeoLabs SARL
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
 *
 */

#define _ULINET
#include "ulinet.h"
#include <assert.h>

size_t write_data_into(void *buffer, size_t size, size_t nmemb, void *data){
  size_t realsize = size * nmemb;
  HINTERNET *psInternet;
  if(buffer==NULL){
    buffer=NULL;
    return -1;
  }
  psInternet=(HINTERNET *)data;
  if(psInternet->pabyData){
    psInternet->pabyData=(char*)realloc(psInternet->pabyData,psInternet->nDataLen+realsize+1);
    psInternet->nDataAlloc+=psInternet->nDataLen+realsize+1;
  }
  else{
    psInternet->pabyData=(char*)malloc(psInternet->nDataLen+realsize+1);
    psInternet->nDataAlloc=realsize+1;
  }

  if (psInternet->pabyData) {
    memcpy( psInternet->pabyData + psInternet->nDataLen, buffer, realsize);
    psInternet->nDataLen += realsize;
    psInternet->pabyData[psInternet->nDataLen] = 0;
  }

  buffer=NULL;
  return realsize;
}

size_t header_write_data(void *buffer, size_t size, size_t nmemb, void *data){
  if(strncmp("Set-Cookie: ",buffer,12)==0){
    int i;
    char env[1024];
    char path[1024];
    char domain[1024];
	char* tmp;
    for(i=0;i<12;i++)
#ifndef WIN32
      buffer++;
#else
	;
#endif
    sscanf(buffer,"%s; path=%s; domain=%s",env,path,domain);
    tmp=strcat(env,CCookie);
#ifdef MSG_LAF_OUT
    printf("\n**Cookie env : [%s] , path : [%s], domain : [%s]**\n",env,path,domain);
    printf("buffer : %d (%s) (%s) (%s)\n",(buffer==NULL),buffer,tmp,CCookie);
#endif
    strcpy(CCookie,tmp);
  }
  return size * nmemb;//write_data_into(buffer,size,nmemb,data,HEADER);
};


void setProxy(CURL* handle,char* host,long port){
}

/**
 * MACOSX
 */
#if defined(macintosh) || (defined(__MACH__) && defined(__APPLE__))


char* CFStringToCString(CFStringRef dest,char *buffer){
  CFStringEncoding encoding = kCFStringEncodingUTF8;
  Boolean bool2 = CFStringGetCString(dest,buffer,1024,encoding);
  if(bool2){
    printf("Loaded into local_buffer");
    return buffer;
  }
  return NULL;
}

OSStatus setProxiesForProtcol(CURL* handle,const char *proto){
  OSStatus		err;
  CFDictionaryRef proxyDict;
  CFArrayRef		proxies;
  
  CFStringRef key_enabled;
  CFStringRef key_host;
  CFStringRef key_port;
  
  bool proxy_enabled;
  char *proxy_host;
  long proxy_port;
  
  proxyDict = NULL;
  proxies = NULL;

  err = noErr;
  proxyDict = SCDynamicStoreCopyProxies(NULL);

  if(proto=="http"){
      key_enabled=kSCPropNetProxiesHTTPEnable;
      key_host=kSCPropNetProxiesHTTPProxy;
      key_port=kSCPropNetProxiesHTTPPort;
  }
  else
    if(proto=="https"){
      key_enabled=kSCPropNetProxiesHTTPSEnable;
      key_host=kSCPropNetProxiesHTTPSProxy;
      key_port=kSCPropNetProxiesHTTPSPort;
    }

  CFNumberGetValue(CFDictionaryGetValue(proxyDict,key_enabled),kCFNumberIntType,&proxy_enabled);
  if(proxy_enabled){
    CFNumberGetValue(CFDictionaryGetValue(proxyDict,key_port),CFNumberGetType(CFDictionaryGetValue(proxyDict,key_port)),&proxy_port);
    char buffer[1024];
    CFStringToCString(CFDictionaryGetValue(proxyDict,key_host),buffer);
    proxy_host=buffer;

#ifdef MSG_LAF_VERBOSE
    printf("\n**[PROXY SETTINGS DETECTION %s (%d) %s:%li (%s)]**\n",proto,proxy_enabled,(char*)proxy_host,proxy_port,buffer);
#endif

    if (proxyDict == NULL) {
      err = coreFoundationUnknownErr;
    }

    setProxy(handle,proxy_host,proxy_port);
  }
  return err;
}
#else
/**
 * Linux (Gnome)
 */
bool setProxiesForProtcol(CURL* handle,const char *proto){
#ifdef MSG_LAF_VERBOSE
  fprintf( stderr, "setProxiesForProtocol (do nothing) ...\n" );
#endif
}
#endif

HINTERNET InternetOpen(char* lpszAgent,int dwAccessType,char* lpszProxyName,char* lpszProxyBypass,int dwFlags){
  
  HINTERNET ret;
  struct MemoryStruct header;
  ret.hasCacheFile=0;
  ret.nDataAlloc = 0;

  ret.handle=curl_easy_init();

  curl_easy_setopt(ret.handle, CURLOPT_COOKIEFILE, "ALL");
#ifndef TIGER
  curl_easy_setopt(ret.handle, CURLOPT_COOKIELIST, "ALL");
#endif
  curl_easy_setopt(ret.handle, CURLOPT_USERAGENT, lpszAgent);
  
  curl_easy_setopt(ret.handle,CURLOPT_FOLLOWLOCATION,1);
  curl_easy_setopt(ret.handle,CURLOPT_MAXREDIRS,3);
  
  header.memory=NULL;
  header.size = 0;

  curl_easy_setopt(ret.handle, CURLOPT_HEADERFUNCTION, header_write_data);
  curl_easy_setopt(ret.handle, CURLOPT_WRITEHEADER, (void *)&header);

#ifdef MSG_LAF_VERBOSE
  curl_easy_setopt(ret.handle, CURLOPT_VERBOSE, 1);
#endif

  return ret;
}

static size_t 
CurlWriteCB(void *buffer, size_t size, size_t nmemb, void *reqInfo){
  HINTERNET *psInternet = (HINTERNET *) reqInfo;

  memcpy( psInternet->pabyData + psInternet->nDataLen, buffer,  nmemb * size );
  psInternet->nDataLen += nmemb * size;
  psInternet->pabyData[psInternet->nDataLen] = 0;

  return nmemb *size;
}

void InternetCloseHandle(HINTERNET handle){
  if(handle.hasCacheFile>0){
    fclose(handle.file);
    unlink(handle.filename);
  }
  else{
    handle.pabyData = NULL;
    handle.nDataAlloc = handle.nDataLen = 0;
  }
  if(handle.handle)
    curl_easy_cleanup(handle.handle);
  curl_global_cleanup();
}

HINTERNET InternetOpenUrl(HINTERNET hInternet,LPCTSTR lpszUrl,LPCTSTR lpszHeaders,size_t dwHeadersLength,size_t dwFlags,size_t dwContext){

  char filename[255];
  hInternet.nDataLen = 0;

  hInternet.nDataAlloc = 0;
  hInternet.pabyData= NULL;
      
  switch(dwFlags)
    {
    case INTERNET_FLAG_NO_CACHE_WRITE:    
      hInternet.hasCacheFile=-1;
      curl_easy_setopt(hInternet.handle, CURLOPT_WRITEFUNCTION, write_data_into);
      curl_easy_setopt(hInternet.handle, CURLOPT_WRITEDATA, &hInternet);
      break;
    default:
      sprintf(filename,"/tmp/ZOO_Cache%d",(int)time(NULL));
      filename[24]=0;
      fprintf(stderr,"file=%s",filename);
#ifdef MSG_LAF_VERBOSE
      fprintf(stderr,"file=%s",filename);
#endif
      hInternet.filename=filename;
      hInternet.file=fopen(hInternet.filename,"w+");
    
      hInternet.hasCacheFile=1;
      curl_easy_setopt(hInternet.handle, CURLOPT_WRITEFUNCTION, NULL);
      curl_easy_setopt(hInternet.handle, CURLOPT_WRITEDATA, hInternet.file);
      hInternet.nDataLen=0;
      break;
    }
#ifdef ULINET_DEBUG
  fprintf(stderr,"URL (%s)\nBODY (%s)\n",lpszUrl,lpszHeaders);
#endif
  if(lpszHeaders!=NULL && strlen(lpszHeaders)>0){
#ifdef MSG_LAF_VERBOSE
    fprintf(stderr,"FROM ULINET !!");
    fprintf(stderr,"HEADER : %s\n",lpszHeaders);
#endif
    //curl_easy_setopt(hInternet.handle,CURLOPT_COOKIE,lpszHeaders);
    curl_easy_setopt(hInternet.handle,CURLOPT_POST,1);
#ifdef ULINET_DEBUG
    fprintf(stderr,"** (%s) %d **\n",lpszHeaders,dwHeadersLength);
    curl_easy_setopt(hInternet.handle,CURLOPT_VERBOSE,1);
#endif
    curl_easy_setopt(hInternet.handle,CURLOPT_POSTFIELDS,lpszHeaders);
    //curl_easy_setopt(hInternet.handle,CURLOPT_POSTFIELDSIZE,dwHeadersLength+1);
    if(hInternet.header!=NULL)
      curl_easy_setopt(hInternet.handle,CURLOPT_HTTPHEADER,hInternet.header);
  }

  curl_easy_setopt(hInternet.handle,CURLOPT_URL,lpszUrl);
  curl_easy_perform(hInternet.handle);

  return hInternet;
};

int freeCookieList(HINTERNET hInternet){
  memset(&CCookie[0],0,1024);
#ifndef TIGER
  curl_easy_setopt(hInternet.handle, CURLOPT_COOKIELIST, "ALL");
#endif
  return 1;
}

int InternetReadFile(HINTERNET hInternet,LPVOID lpBuffer,int dwNumberOfBytesToRead, size_t *lpdwNumberOfBytesRead){
  int dwDataSize;

  if(hInternet.hasCacheFile>0){
    fseek (hInternet.file , 0 , SEEK_END);
    dwDataSize=ftell(hInternet.file); //taille du ficher
    rewind (hInternet.file);
  }
  else{
    memset(lpBuffer,0,hInternet.nDataLen+1);
    memcpy( lpBuffer, hInternet.pabyData, hInternet.nDataLen );
    dwDataSize=hInternet.nDataLen;
    free( hInternet.pabyData );
    hInternet.pabyData=NULL;
  }

  if( dwNumberOfBytesToRead /* buffer size */ < dwDataSize )
    return 0;

#ifdef MSG_LAF_VERBOSE
  printf("\nfile size : %dko\n",dwDataSize/1024);
#endif

  if(hInternet.hasCacheFile>0){
    *lpdwNumberOfBytesRead = fread(lpBuffer,1,dwDataSize,hInternet.file); 
  }
  else{
    *lpdwNumberOfBytesRead = hInternet.nDataLen;
    free( hInternet.pabyData );
    hInternet.pabyData = NULL;
    hInternet.nDataAlloc = hInternet.nDataLen = 0;
  }

  CCookie[0]=0;

  if( *lpdwNumberOfBytesRead < dwDataSize )
      return 0;
  else
      return 1; // TRUE
}

bool InternetGetCookie(LPCTSTR lpszUrl,LPCTSTR lpszCookieName,LPTSTR lpszCookieData,LPDWORD lpdwSize){

  bool ret=1;  
  int count=0;
  int hasCookie=-1;
  char TMP[1024];
  int j;
  int tmpC=0;
  lpszUrl=NULL;

  for(j=0;j<strlen(CCookie);j++){
    if(lpszCookieName[count]==CCookie[j]){
      hasCookie=1;
      count++;
      if(count==strlen(lpszCookieName))
	break;
      continue;
    }
  }

  if(hasCookie>0){
    if(CCookie[count]=='='){
      int i=0;
      count++;
      for(i=count;i<strlen(CCookie);i++){
	if(CCookie[i]!=';'){
	  TMP[tmpC]=CCookie[i];
	  tmpC++;
	}
	else{
	  break;
	}
      }
    }
  }
  else
    return -1;

  TMP[tmpC]=0;
  strncpy(lpszCookieData,TMP,strlen(TMP)+1);
  lpdwSize=(size_t*) strlen(lpszCookieData);

#ifdef MSG_LAF_VERBOSE
  printf("Cookie returned : (%s)",(char*)lpszCookieData);
#endif

  return ret;

}

#ifdef USE_JS
#define XP_WIN 1
#include "jsapi.h"

char* JSValToChar(JSContext* context, jsval* arg) {
  char *c;
  char *tmp;
  JSString *jsmsg;
  size_t len;
  int i;
  if(!JSVAL_IS_STRING(*arg)) {
    return NULL;
  }
  jsmsg = JS_ValueToString(context,*arg);
  len = JS_GetStringLength(jsmsg);
  tmp = JS_EncodeString(context,jsmsg);
  c = (char*)malloc((len+1)*sizeof(char));
  c[len] = '\0';
#ifdef ULINET_DEBUG
  fprintf(stderr,"%d \n",len);
#endif
  for(i = 0;i < len;i++) {
    c[i] = tmp[i];
    c[i+1] = 0;
  }
#ifdef ULINET_DEBUG
  fprintf(stderr,"%s \n",c);
#endif
  return c;
}

HINTERNET setHeader(HINTERNET handle,JSContext *cx,JSObject *header){
  jsuint length=0;
  jsint i=0;
  char *tmp1;
#ifdef ULINET_DEBUG
  fprintf(stderr,"setHeader\n");
#endif
  if(JS_IsArrayObject(cx,header)){
#ifdef ULINET_DEBUG
    fprintf(stderr,"header is an array\n");
#endif
    JS_GetArrayLength(cx,header,&length);
#ifdef ULINET_DEBUG
    fprintf(stderr,"header is an array of %d elements\n",length);
#endif
    handle.header=NULL;
    for(i=0;i<length;i++){
      jsval tmp;
      JS_GetElement(cx,header,i,&tmp);
      tmp1=JSValToChar(cx,&tmp);
#ifdef ULINET_DEBUG
      fprintf(stderr,"Element of array n° %d, value : %s\n",i,tmp1);
#endif
      handle.header=curl_slist_append(handle.header, tmp1);
      free(tmp1);
    }
  }
  else{
    fprintf(stderr,"not an array !!!!!!!\n");
  }
  return handle;
}

JSBool
JSRequest(JSContext *cx, uintN argc, jsval *argv1)
{
  jsval *argv = JS_ARGV(cx,argv1);
  HINTERNET hInternet;
  HINTERNET res;
  HINTERNET res1;
  JSObject *header;
  char *url;
  char *method;
  char* tmpValue;
  size_t dwRead;
  int i=0;
  JS_MaybeGC(cx);
  hInternet=InternetOpen((LPCTSTR)"ZooWPSClient\0",
			 INTERNET_OPEN_TYPE_PRECONFIG,
			 NULL,NULL, 0);
  if(!CHECK_INET_HANDLE(hInternet))
    return JS_FALSE;
  if(argc>=2){
    method=JSValToChar(cx,&argv[0]);
    url=JSValToChar(cx,&argv[1]);
  }
  else{
    method=strdup("GET");
    url=JSValToChar(cx,argv);
  }
  if(argc==4){
    char *body;
    body=JSValToChar(cx,&argv[2]);
    header=JSVAL_TO_OBJECT(argv[3]);
#ifdef ULINET_DEBUG
    fprintf(stderr,"URL (%s) \nBODY (%s)\n",url,body);
#endif
    if(JS_IsArrayObject(cx,header))
      res1=setHeader(hInternet,cx,header);
#ifdef ULINET_DEBUG
    fprintf(stderr,"BODY (%s)\n",body);
#endif
    res=InternetOpenUrl(res1,url,body,strlen(body),
			INTERNET_FLAG_NO_CACHE_WRITE,0);    
    free(body);
  }else{
    if(argc==3){
      char *body=JSValToChar(cx,&argv[2]);
      res=InternetOpenUrl(hInternet,url,body,strlen(body),
			  INTERNET_FLAG_NO_CACHE_WRITE,0);
      free(body);
    }
    res=InternetOpenUrl(hInternet,url,NULL,0,
			INTERNET_FLAG_NO_CACHE_WRITE,0);
  }
  tmpValue=(char*)malloc((res.nDataLen+1)*sizeof(char));
  InternetReadFile(res,(LPVOID)tmpValue,res.nDataLen,&dwRead);
  fprintf(stderr,"content downloaded (%d) (%s) \n",dwRead,tmpValue);
  if(dwRead==0){
    JS_SET_RVAL(cx, argv1,STRING_TO_JSVAL(JS_NewStringCopyN(cx,"Unable to access the file.",strlen("Unable to access the file."))));
    return JS_TRUE;
  }

#ifdef ULINET_DEBUG
  fprintf(stderr,"content downloaded (%d) (%s) \n",dwRead,tmpValue);
#endif
  JS_SET_RVAL(cx, argv1,STRING_TO_JSVAL(JS_NewStringCopyN(cx,tmpValue,strlen(tmpValue))));
  free(url);
  if(argc>=2)
    free(method);
  if(argc==4 && res.header!=NULL){
    curl_slist_free_all(res.header);
  }
  InternetCloseHandle(hInternet);
  JS_MaybeGC(cx);
  return JS_TRUE;
}
#endif
