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
#define MAX_WAIT_MSECS 180*1000 /* Wait max. 180 seconds */
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
    psInternet->pabyData=(unsigned char*)realloc(psInternet->pabyData,psInternet->nDataLen+realsize+1);
    psInternet->nDataAlloc+=psInternet->nDataLen+realsize+1;
  }
  else{
    psInternet->pabyData=(unsigned char*)malloc(psInternet->nDataLen+realsize+1);
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
  
  CFStringRef key_enabled = NULL;
  CFStringRef key_host = NULL;
  CFStringRef key_port = NULL;
  
  bool proxy_enabled;
  char *proxy_host;
  long proxy_port;
  
  proxyDict = NULL;
  proxies = NULL;

  err = noErr;
  proxyDict = SCDynamicStoreCopyProxies(NULL);

  if(strncmp(proto,"http",4)==0){
      key_enabled=kSCPropNetProxiesHTTPEnable;
      key_host=kSCPropNetProxiesHTTPProxy;
      key_port=kSCPropNetProxiesHTTPPort;
  }
  else
    if(strncmp(proto,"https",5)==0){
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
  ret.handle=curl_multi_init();
  ret.ihandle=NULL;
  ret.hasCacheFile=0;
  ret.nDataAlloc = 0;
  ret.mimeType = NULL;
  ret.agent=strdup(lpszAgent);
  return ret;
}

void InternetCloseHandle(HINTERNET handle){
  if(handle.hasCacheFile>0){
    fclose(handle.file);
    unlink(handle.filename);
    handle.mimeType = NULL;
  }
  else{
    handle.pabyData = NULL;
    handle.mimeType = NULL;
    handle.nDataAlloc = handle.nDataLen = 0;
  }
  if(handle.ihandle!=NULL)
    curl_easy_cleanup(handle.ihandle);
  if(handle.handle)
    curl_multi_cleanup(handle.handle);
  free(handle.agent);
}

HINTERNET InternetOpenUrl(HINTERNET hInternet,LPCTSTR lpszUrl,LPCTSTR lpszHeaders,size_t dwHeadersLength,size_t dwFlags,size_t dwContext){

  char filename[255];
  struct MemoryStruct header;

  hInternet.nDataLen = 0;

  hInternet.nDataAlloc = 0;
  hInternet.pabyData= NULL;

  if(hInternet.ihandle!=NULL)
    curl_easy_cleanup(hInternet.ihandle);
  hInternet.ihandle=curl_easy_init( );
  curl_easy_setopt(hInternet.ihandle, CURLOPT_COOKIEFILE, "ALL");
#ifndef TIGER
  curl_easy_setopt(hInternet.ihandle, CURLOPT_COOKIELIST, "ALL");
#endif
  curl_easy_setopt(hInternet.ihandle, CURLOPT_USERAGENT, hInternet.agent);
  
  curl_easy_setopt(hInternet.ihandle,CURLOPT_FOLLOWLOCATION,1);
  curl_easy_setopt(hInternet.ihandle,CURLOPT_MAXREDIRS,3);
  
  header.memory=NULL;
  header.size = 0;

  curl_easy_setopt(hInternet.ihandle, CURLOPT_HEADERFUNCTION, header_write_data);
  curl_easy_setopt(hInternet.ihandle, CURLOPT_WRITEHEADER, (void *)&header);

#ifdef MSG_LAF_VERBOSE
  curl_easy_setopt(hInternet.ihandle, CURLOPT_VERBOSE, 1);
#endif

      
  switch(dwFlags)
    {
    case INTERNET_FLAG_NO_CACHE_WRITE:    
      hInternet.hasCacheFile=-1;
      curl_easy_setopt(hInternet.ihandle, CURLOPT_WRITEFUNCTION, write_data_into);
      curl_easy_setopt(hInternet.ihandle, CURLOPT_WRITEDATA, &hInternet);
      break;
    default:
      sprintf(filename,"/tmp/ZOO_Cache%d",(int)time(NULL));
      filename[24]=0;
#ifdef MSG_LAF_VERBOSE
      fprintf(stderr,"file=%s",filename);
#endif
      hInternet.filename=filename;
      hInternet.file=fopen(hInternet.filename,"w+");
    
      hInternet.hasCacheFile=1;
      curl_easy_setopt(hInternet.ihandle, CURLOPT_WRITEFUNCTION, NULL);
      curl_easy_setopt(hInternet.ihandle, CURLOPT_WRITEDATA, hInternet.file);
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
    curl_easy_setopt(hInternet.ihandle,CURLOPT_POST,1);
#ifdef ULINET_DEBUG
    fprintf(stderr,"** (%s) %d **\n",lpszHeaders,dwHeadersLength);
    curl_easy_setopt(hInternet.ihandle,CURLOPT_VERBOSE,1);
#endif
    curl_easy_setopt(hInternet.ihandle,CURLOPT_POSTFIELDS,lpszHeaders);
    //curl_easy_setopt(hInternet.handle,CURLOPT_POSTFIELDSIZE,dwHeadersLength+1);
    if(hInternet.header!=NULL)
      curl_easy_setopt(hInternet.ihandle,CURLOPT_HTTPHEADER,hInternet.header);
  }

  curl_easy_setopt(hInternet.ihandle,CURLOPT_URL,lpszUrl);

  curl_multi_add_handle(hInternet.handle,hInternet.ihandle);
  
  int still_running=0;
  int msgs_left=0;
  do{
    curl_multi_perform(hInternet.handle, &still_running);
  }while(still_running);

  CURLMsg *msg=NULL;
  while ((msg = curl_multi_info_read(hInternet.handle, &msgs_left))) {
    if (msg->msg == CURLMSG_DONE) {
      CURL *eh=NULL;
      eh = msg->easy_handle;
      curl_easy_getinfo(eh,CURLINFO_CONTENT_TYPE,&hInternet.mimeType);
    }
    else {
      fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
    }
  }
  curl_multi_remove_handle(hInternet.handle, hInternet.ihandle);

#ifdef ULINET_DEBUG
  fprintf(stderr,"DEBUG MIMETYPE: %s\n",hInternet.mimeType);
  fflush(stderr);
#endif
  return hInternet;
};

int freeCookieList(HINTERNET hInternet){
  memset(&CCookie[0],0,1024);
#ifndef TIGER
  curl_easy_setopt(hInternet.ihandle, CURLOPT_COOKIELIST, "ALL");
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
    memcpy(lpBuffer, hInternet.pabyData, hInternet.nDataLen );
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

