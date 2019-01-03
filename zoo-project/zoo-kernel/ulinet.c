/*
 *  ulinet.c
 *
 * Author : Gérald FENOY
 *
 * Copyright (c) 2008-2015 GeoLabs SARL
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
#include <ctype.h>

/**
 * Write the downloaded content to a _HINTERNET structure
 *
 * @param buffer the buffer to read
 * @param size size of each member
 * @param nmemb number of element to read
 * @param data the _HINTERNET structure to write in
 * @return the size red, -1 if buffer is NULL
 */
size_t write_data_into(void *buffer, size_t size, size_t nmemb, void *data){
  size_t realsize = size * nmemb;
  _HINTERNET *psInternet;
  if(buffer==NULL){
    buffer=NULL;
    return -1;
  }
  psInternet=(_HINTERNET *)data;
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

/**
 * In case of presence of "Set-Cookie" in the headers red, store the cookie
 * identifier in cookie
 *
 * @param buffer the buffer to read
 * @param size size of each member
 * @param nmemb number of element to read
 * @param data the _HINTERNET structure to write in
 * @return the size red, -1 if buffer is NULL
 * @see cookie
 */
size_t header_write_data(void *buffer, size_t size, size_t nmemb, void *data){
  if(strncmp("Set-Cookie: ",(char*)buffer,12)==0){
    int i;
    char* tmp;
    int cnt;
    _HINTERNET *psInternet;
    for(i=0;i<12;i++)
#ifndef WIN32
      buffer++;
#else
	;
#endif
    tmp=strtok((char*) buffer,";"); // knut: added cast to char*
    cnt=0;
    psInternet=(_HINTERNET *)data;
    if(tmp!=NULL && psInternet!=NULL){
      psInternet->cookie=(char*)malloc(sizeof(char)*(strlen(tmp)+1));
      sprintf(psInternet->cookie,"%s",tmp);
    }
  }
  return size * nmemb;//write_data_into(buffer,size,nmemb,data,HEADER);
};

/**
 * Define the proxy to use for a CURL handler
 * 
 * @param handle the CURL handler
 * @param host the proxy host (including http://)
 * @param port the proxy port
 */
void setProxy(CURL* handle,char* host,long port){
  char* proxyDef=(char*)malloc((strlen(host)+10+2)*sizeof(char));
  sprintf(proxyDef,"%s:%ld",host,port);
  curl_easy_setopt(handle,CURLOPT_PROXY,proxyDef);
  free(proxyDef);
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
 * Should autodetect the proxy configuration (do nothing on linux)
 *
 * @param handle a CURL handle
 * @param proto the protocol requiring the use of a proxy
 */
int setProxiesForProtcol(CURL* handle,const char *proto){
#ifdef MSG_LAF_VERBOSE
  fprintf( stderr, "setProxiesForProtocol (do nothing) ...\n" );
#endif
  return 0;
}
#endif

/**
 * Create a HINTERNET
 *
 * @param lpszAgent the HTPP User-Agent to use to send requests
 * @param  dwAccessType type of access required
 * @param  lpszProxyName the name of the proxy server(s) to use 
 * @param  lpszProxyBypass ip address or host names which should not be routed
 *  through the proxy
 * @param  dwFlags Options (INTERNET_FLAG_ASYNC,INTERNET_FLAG_FROM_CACHE,INTERNET_FLAG_OFFLINE)
 * @return the created HINTERNET
 */
HINTERNET InternetOpen(char* lpszAgent,int dwAccessType,char* lpszProxyName,char* lpszProxyBypass,int dwFlags){
  HINTERNET ret;
  ret.handle=curl_multi_init();
  ret.agent=strdup(lpszAgent);
  ret.nb=0;
  ret.ihandle[ret.nb].header=NULL;
  return ret;
}

/**
 * Add missing headers to an existing _HINTERNET
 * 
 * 
 * @param handle the _HINTERNET pointer
 * @param key the header parameter name
 * @param value the header parameter value
 * @return 0 if the operation succeeded, -1 in other case.
 */
int AddMissingHeaderEntry(_HINTERNET* handle,const char* key,const char* value){
  int length=strlen(key)+strlen(value)+3;
  char *entry=(char*)malloc((length)*sizeof(char));
  if(entry==NULL)
    return -1;
  snprintf (entry, length, "%s: %s", key, value);
  handle->header = curl_slist_append (handle->header, entry);
  free(entry);
  return 0;
}

/**
 * Verify if a host is protected (appear in [security] > hosts)
 *
 * @param protectedHosts string containing all the protected hosts (coma separated)
 * @param url string used to extract the host from
 * @return 1 if the host is listed as protected, 0 in other case
 */
int isProtectedHost(const char* protectedHosts,const char* url){
  char *token, *saveptr;
  int cnt;
  char* host; 
  
  // knut: make a copy of url since strtok family modifies first argument and cannot be used on constant strings  
  char* urlcpy = (char*) malloc(sizeof(char)*(strlen(url)+1)); 
  urlcpy = strncpy(urlcpy, url, strlen(url)+1); // since count > strlen(url), a null character is properly appended
 
  //token = strtok_r (url, "//", &saveptr);
  token = strtok_r (urlcpy, "//", &saveptr);   // knut
  cnt=0;
  while(token!=NULL && cnt<=1){
    fprintf(stderr,"%s %d %s \n",__FILE__,__LINE__,token);
    if(cnt==1)
      fprintf(stderr,"%s %d %s \n",__FILE__,__LINE__,strstr(protectedHosts,token));
    fflush(stderr);
    if(cnt==1 && strstr(protectedHosts,token)!=NULL){
      fprintf(stderr,"%s %d %s \n",__FILE__,__LINE__,strstr(protectedHosts,token));
      free(urlcpy); 
      return 1;
    }
    token = strtok_r (NULL, "/", &saveptr);
    cnt+=1;
  }
  free(urlcpy);
  return 0;
}

/**
 * Add headers defined in [security] > attributes to an existing HINTERNET
 * @see isProtectedHost, AddMissingHeaderEntry
 * 
 * @param handle the _HINTERNET pointer
 * @param conf the header parameter name
 * @param value the header parameter value
 * @return 0 if the operation succeeded, -1 in other case.
 */
void AddHeaderEntries(HINTERNET* handle,maps* conf){
  map* passThrough=getMapFromMaps(conf,"security","attributes");
  map* targetHosts=getMapFromMaps(conf,"security","hosts");
  char* passedHeader[10];
  int cnt=0;
  if(passThrough!=NULL && targetHosts!=NULL){
    char *tmp=zStrdup(passThrough->value);
    char *token, *saveptr;
    int i;    
    token = strtok_r (tmp, ",", &saveptr);
    for(i=0;i<handle->nb;i++){
      if(targetHosts->value[0]=='*' || isProtectedHost(targetHosts->value,handle->ihandle[i].url)==1){
	while (token != NULL){
	  int j;
      map* tmpMap;	  
	  int length=strlen(token)+6;
	  char* tmp1=(char*)malloc(length*sizeof(char));
	  snprintf(tmp1,6,"HTTP_");
	  for(j=0;token[j]!='\0';j++){
	    if(token[j]!='-')
	      tmp1[5+j]=toupper(token[i]);
	    else
	      tmp1[5+j]='_';
	    tmp1[5+j+1]='\0';
	  }
	  fprintf(stderr,"%s %d %s \n",__FILE__,__LINE__,tmp1);
	  tmpMap = getMapFromMaps(conf,"renv",tmp1);
	  if(tmpMap!=NULL)	    
	    AddMissingHeaderEntry(&handle->ihandle[i],token,tmpMap->value);
	  free(tmp1);
	  if(handle->ihandle[i].header!=NULL)
	    curl_easy_setopt(handle->ihandle[i].handle,CURLOPT_HTTPHEADER,handle->ihandle[i].header);
	  cnt+=1;
	  token = strtok_r (NULL, ",", &saveptr);
	}
      }
    }
    free(tmp);
  }
}

/**
 * Close a HINTERNET connection and free allocated resources
 *
 * @param handle0 the HINTERNET connection to close
 */
void InternetCloseHandle(HINTERNET* handle0){
  int i=0;
  for(i=0;i<handle0->nb;i++){
    _HINTERNET handle=handle0->ihandle[i];
    if(handle.hasCacheFile>0){
      fclose(handle.file);
      unlink(handle.filename);
    }
    else{
      handle.pabyData = NULL;
      handle.nDataAlloc = handle.nDataLen = 0;
    }
    if(handle0->ihandle[i].header!=NULL){
      curl_slist_free_all(handle0->ihandle[i].header);
      handle0->ihandle[i].header=NULL;
    }
    if(handle.post!=NULL)
      free(handle.post);
    if(handle.url!=NULL)
      free(handle.url);
    if(handle.mimeType!=NULL)
      free(handle.mimeType);
    if(handle.cookie!=NULL)
      free(handle.cookie);
  }
  if(handle0->handle)
    curl_multi_cleanup(handle0->handle);
  free(handle0->agent);
  for(i=handle0->nb-1;i>=0;i--){
    free(handle0->waitingRequests[i]);
  }
}

/**
 * Create a new element in the download queue
 *
 * @param hInternet the HINTERNET connection to add the download link
 * @param lpszUrl the url to download
 * @param lpszHeaders the additional headers to be sent to the HTTP server
 * @param dwHeadersLength the size of the additional headers
 * @param dwFlags desired download mode (INTERNET_FLAG_NO_CACHE_WRITE for not using cache file)
 * @param dwContext not used
 */
HINTERNET InternetOpenUrl(HINTERNET* hInternet,LPCTSTR lpszUrl,LPCTSTR lpszHeaders,size_t dwHeadersLength,size_t dwFlags,size_t dwContext){

  char filename[255];
  struct MemoryStruct header;

  hInternet->ihandle[hInternet->nb].handle=curl_easy_init( );
  hInternet->ihandle[hInternet->nb].hasCacheFile=0;
  hInternet->ihandle[hInternet->nb].nDataAlloc = 0;
  hInternet->ihandle[hInternet->nb].url = NULL;
  hInternet->ihandle[hInternet->nb].mimeType = NULL;
  hInternet->ihandle[hInternet->nb].cookie = NULL;
  hInternet->ihandle[hInternet->nb].nDataLen = 0;
  hInternet->ihandle[hInternet->nb].id = hInternet->nb;
  hInternet->ihandle[hInternet->nb].nDataAlloc = 0;
  hInternet->ihandle[hInternet->nb].pabyData = NULL;
  hInternet->ihandle[hInternet->nb].post = NULL;
  
  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_COOKIEFILE, "ALL");
#ifndef TIGER
  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_COOKIELIST, "ALL");
#endif
  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_USERAGENT, hInternet->agent);
  
  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_FOLLOWLOCATION,1);
  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_MAXREDIRS,3);
  
  header.memory=NULL;
  header.size = 0;

  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_HEADERFUNCTION, header_write_data);
  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_WRITEHEADER, (void *)&header);

#ifdef MSG_LAF_VERBOSE
  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_VERBOSE, 1);
#endif

      
  switch(dwFlags)
    {
    case INTERNET_FLAG_NO_CACHE_WRITE:
      hInternet->ihandle[hInternet->nb].hasCacheFile=-1;
      curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_WRITEFUNCTION, write_data_into);
      curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_WRITEDATA, (void*)&hInternet->ihandle[hInternet->nb]);
      break;
    default:
      sprintf(hInternet->ihandle[hInternet->nb].filename,"/tmp/ZOO_Cache%d",(int)time(NULL));
      hInternet->ihandle[hInternet->nb].filename[24]=0;
#ifdef MSG_LAF_VERBOSE
      fprintf(stderr,"file=%s",hInternet->ihandle[hInternet->nb].filename);
#endif
      hInternet->ihandle[hInternet->nb].filename=filename;
      hInternet->ihandle[hInternet->nb].file=fopen(hInternet->ihandle[hInternet->nb].filename,"w+");
    
      hInternet->ihandle[hInternet->nb].hasCacheFile=1;
      curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_WRITEFUNCTION, NULL);
      curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle, CURLOPT_WRITEDATA, hInternet->ihandle[hInternet->nb].file);
      hInternet->ihandle[hInternet->nb].nDataLen=0;
      break;
    }
#ifdef ULINET_DEBUG
  fprintf(stderr,"URL (%s)\nBODY (%s)\n",lpszUrl,lpszHeaders);
#endif
  if(lpszHeaders!=NULL && strlen(lpszHeaders)>0){
#ifdef MSG_LAF_VERBOSE
    fprintf(stderr,"FROM ULINET !!");
    fprintf(stderr,"HEADER : [%s] %d\n",lpszHeaders,dwHeadersLength);
#endif
    curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_POST,1);
#ifdef ULINET_DEBUG
    fprintf(stderr,"** (%s) %d **\n",lpszHeaders,dwHeadersLength);
    curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_VERBOSE,1);
#endif
    hInternet->ihandle[hInternet->nb].post=strdup(lpszHeaders);
    curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_POSTFIELDS,hInternet->ihandle[hInternet->nb].post);
    curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_POSTFIELDSIZE,(long)dwHeadersLength);
  }
  if(hInternet->ihandle[hInternet->nb].header!=NULL)
    curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_HTTPHEADER,hInternet->ihandle[hInternet->nb].header);

  curl_easy_setopt(hInternet->ihandle[hInternet->nb].handle,CURLOPT_URL,lpszUrl);
  hInternet->ihandle[hInternet->nb].url = zStrdup(lpszUrl);

  curl_multi_add_handle(hInternet->handle,hInternet->ihandle[hInternet->nb].handle);
  
  hInternet->ihandle[hInternet->nb].header=NULL;
  ++hInternet->nb;

#ifdef ULINET_DEBUG
  fprintf(stderr,"DEBUG MIMETYPE: %s\n",hInternet.mimeType);
  fflush(stderr);
#endif
  return *hInternet;
};

/**
 * Download all opened urls in the queue
 *
 * @param hInternet the HINTERNET structure containing the queue
 * @return 0
 */
int processDownloads(HINTERNET* hInternet){
  int still_running=0;
  int msgs_left=0;
  int i=0;
  do{
    curl_multi_perform(hInternet->handle, &still_running);
  }while(still_running);  
  for(i=0;i<hInternet->nb;i++){
    char *tmp;
    curl_easy_getinfo(hInternet->ihandle[i].handle,CURLINFO_CONTENT_TYPE,&tmp);
    if(tmp!=NULL)
      hInternet->ihandle[i].mimeType=strdup(tmp);
    curl_easy_getinfo(hInternet->ihandle[i].handle,CURLINFO_RESPONSE_CODE,&hInternet->ihandle[i].code);
    curl_multi_remove_handle(hInternet->handle, hInternet->ihandle[i].handle);
    curl_easy_cleanup(hInternet->ihandle[i].handle);
  }
  return 0;
}

/**
 * Initialize the cookie for a specific index (hInternet.nb)
 *
 * @param hInternet the HINTERNET structure to know the cookie index to reset
 * @return 1
 * @see HINTERNET
 */
int freeCookieList(HINTERNET hInternet){
  if(hInternet.ihandle[hInternet.nb].cookie)
    free(hInternet.ihandle[hInternet.nb].cookie);
  hInternet.ihandle[hInternet.nb].cookie=NULL;
#ifndef TIGER
  curl_easy_setopt(hInternet.ihandle[hInternet.nb].handle, CURLOPT_COOKIELIST, "ALL");
#endif
  return 1;
}

/**
 * Copy a downloaded content
 * 
 * @param hInternet the _HINTERNET structure
 * @param lpBuffer the memory space to copy the downloaded content
 * @param dwNumberOfBytesToRead the size of lpBuffer
 * @param lpdwNumberOfBytesRead number of bytes red
 * @return 1 on success, 0 if failure
 */
int InternetReadFile(_HINTERNET hInternet,LPVOID lpBuffer,int dwNumberOfBytesToRead, size_t *lpdwNumberOfBytesRead){
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

  if( *lpdwNumberOfBytesRead < dwDataSize )
      return 0;
  else
      return 1; // TRUE
}


/**
 * Use basic authentication for accessing a resource
 * 
 * @param hInternet the _HINTERNET structure
 * @param login the login to use to authenticate
 * @param passwd the password to use to authenticate
 */
int setBasicAuth(HINTERNET hInternet,char* login,char* passwd){
  char *tmp;
  tmp=(char*)malloc((strlen(login)+strlen(passwd)+2)*sizeof(char));
  sprintf(tmp,"%s:%s",login,passwd);
  if(curl_easy_setopt(hInternet.ihandle[hInternet.nb].handle,CURLOPT_USERPWD,tmp)==CURLE_OUT_OF_MEMORY){
    free(tmp);
    return -1;
  }
  curl_easy_setopt(hInternet.ihandle[hInternet.nb].handle, CURLOPT_HTTPAUTH,CURLAUTH_ANY);
  free(tmp);
  return 0;
}
