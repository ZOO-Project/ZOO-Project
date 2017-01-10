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

#include "caching.h"
#include "service.h"
#include "service_internal.h"
#include "response_print.h"
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

/**
 * Compute md5
 * 
 * @param url the char* 
 * @return a char* representing the md5 of the url
 * @warning make sure to free resources returned by this function
 */
char* getMd5(char* url){
  EVP_MD_CTX md5ctx;
  char* fresult=(char*)malloc((EVP_MAX_MD_SIZE+1)*sizeof(char));
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int len;
  EVP_DigestInit(&md5ctx, EVP_md5());
  EVP_DigestUpdate(&md5ctx, url, strlen(url));
  EVP_DigestFinal_ex(&md5ctx,result,&len);
  EVP_MD_CTX_cleanup(&md5ctx);
  int i;
  for(i = 0; i < len; i++){
    if(i>0){
      char *tmp=strdup(fresult);
      sprintf(fresult,"%s%02x", tmp,result[i]);
      free(tmp);
    }
    else
      sprintf(fresult,"%02x",result[i]);
  }
  return fresult;
}


/**
 * Cache a file for a given request.
 * For each cached file, the are two files stored, a .zca and a .zcm containing
 * the downloaded content and the mimeType respectively. 
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param request the url used too fetch the content
 * @param content the downloaded content
 * @param mimeType the content mimeType 
 * @param length the content size
 * @param filepath a buffer for storing the path of the cached file; may be NULL
 * @param max_path the size of the allocated filepath buffer  
 */
void addToCache(maps* conf,char* request,char* content,char* mimeType,int length, 
                char* filepath, size_t max_path){
  map* tmp=getMapFromMaps(conf,"main","cacheDir");
  if(tmp!=NULL){
    char* md5str=getMd5(request);
    char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
    sprintf(fname,"%s/%s.zca",tmp->value,md5str);
#ifdef DEBUG
    fprintf(stderr,"Cache list : %s\n",fname);
    fflush(stderr);
#endif
    FILE* fo=fopen(fname,"w+");
    if(fo==NULL){
#ifdef DEBUG
      fprintf (stderr, "Failed to open %s for writing: %s\n",fname, strerror(errno));
#endif
      filepath = NULL;	
      return;
    }
    fwrite(content,sizeof(char),length,fo);
    fclose(fo);
	
	if (filepath != NULL) {
		strncpy(filepath, fname, max_path);
	}	

    sprintf(fname,"%s/%s.zcm",tmp->value,md5str);
    fo=fopen(fname,"w+");
#ifdef DEBUG
    fprintf(stderr,"MIMETYPE: %s\n",mimeType);
#endif
    fwrite(mimeType,sizeof(char),strlen(mimeType),fo);
    fclose(fo);

    free(md5str);
    free(fname);
  }
  else {
	  filepath = NULL;
  }	  
}

/**
 * Verify if a url is available in the cache
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param request the url
 * @return the full name of the cached file if any, NULL in other case
 * @warning make sure to free resources returned by this function (if not NULL)
 */
char* isInCache(maps* conf,char* request){
  map* tmpM=getMapFromMaps(conf,"main","cacheDir");
  if(tmpM!=NULL){
    char* md5str=getMd5(request);
#ifdef DEBUG
    fprintf(stderr,"MD5STR : (%s)\n\n",md5str);
#endif
    char* fname=(char*)malloc(sizeof(char)*(strlen(tmpM->value)+strlen(md5str)+6));
    sprintf(fname,"%s/%s.zca",tmpM->value,md5str);
    struct stat f_status;
    int s=stat(fname, &f_status);
    if(s==0 && f_status.st_size>0){
      free(md5str);
      return fname;
    }
    free(md5str);
    free(fname);
  }
  return NULL;
}

/**
 * Read the downloaded file for a specific input
 *
 * @param m the maps containing the settings of the main.cfg file
 * @param in the input
 * @param index the input index
 * @param hInternet the internet connection
 * @param error the error map pointer
 * @return 0 in case of success, -1 in case of failure
 */
int readCurrentInput(maps** m,maps** in,int* index,HINTERNET* hInternet,map** error){
  map* tmp1;
  char sindex[5];
  maps* content=*in;
  map* length=getMap(content->content,"length");
  int shouldClean=-1;
  if(length==NULL){
    length=createMap("length","1");
    shouldClean=1;
  }
  for(int i=0;i<atoi(length->value);i++){
    char* fcontent;
    char *mimeType=NULL;
    int fsize=0;
    char cname[15];
    char vname[11];
    char vname1[11];
    char sname[9];
    char mname[15];
    char icname[14];
    char xname[16];
    char oname[12];
    if(*index>0)
      sprintf(vname1,"value_%d",*index);
    else
      sprintf(vname1,"value");
    
    if(i>0){
      tmp1=getMap(content->content,cname);
      sprintf(cname,"cache_file_%d",i);
      sprintf(vname,"value_%d",i);
      sprintf(sname,"size_%d",i);
      sprintf(mname,"mimeType_%d",i);
      sprintf(icname,"isCached_%d",i);
      sprintf(xname,"Reference_%d",i);
      sprintf(oname,"Order_%d",i);
    }else{
      sprintf(cname,"cache_file");
      sprintf(vname,"value");
      sprintf(sname,"size");
      sprintf(mname,"mimeType");
      sprintf(icname,"isCached");
      sprintf(xname,"Reference");
      sprintf(oname,"Order");
    }
    
    map* tmap=getMap(content->content,oname);
    sprintf(sindex,"%d",*index+1);
    if((tmp1=getMap(content->content,xname))!=NULL && tmap!=NULL && strcasecmp(tmap->value,sindex)==0){
      
      if(getMap(content->content,icname)==NULL){
	fcontent=(char*)malloc((hInternet->ihandle[*index].nDataLen+1)*sizeof(char));
	if(fcontent == NULL){
	  errorException(*m, _("Unable to allocate memory"), "InternalError",NULL);
	  return -1;
	}
	size_t dwRead;
	InternetReadFile(hInternet->ihandle[*index], 
			 (LPVOID)fcontent, 
			 hInternet->ihandle[*index].nDataLen, 
			 &dwRead);
	fcontent[hInternet->ihandle[*index].nDataLen]=0;
	fsize=hInternet->ihandle[*index].nDataLen;
	if(hInternet->ihandle[*index].mimeType==NULL)
	  mimeType=zStrdup("none");
	else
	  mimeType=zStrdup(hInternet->ihandle[*index].mimeType);	      
	
	map* tmpMap=getMapOrFill(&(*in)->content,vname,"");
	free(tmpMap->value);
	tmpMap->value=(char*)malloc((fsize+1)*sizeof(char));
	if(tmpMap->value==NULL){
	  return errorException(*m, _("Unable to allocate memory"), "InternalError",NULL);
	}
	memcpy(tmpMap->value,fcontent,(fsize+1)*sizeof(char));
	if(hInternet->ihandle[*index].code!=200){
	  char *error_rep_str=_("Unable to download the file for the input <%s>, response code was : %d.");
	  char *error_msg=(char*)malloc((strlen(error_rep_str)+strlen(content->name)+4)*sizeof(char));
	  sprintf(error_msg,error_rep_str,content->name,hInternet->ihandle[*index].code);
	  if(*error==NULL){
	    *error=createMap("text",error_msg);
	    addToMap(*error,"locator",content->name);
	    addToMap(*error,"code","InvalidParameterValue");
	  }else{
	    int nb=1;
	    map* tmpMap=getMap(*error,"length");
	    if(tmpMap!=NULL)
	      nb=atoi(tmpMap->value);
	    setMapArray(*error,"text",nb,error_msg);
	    setMapArray(*error,"locator",nb,content->name);
	    setMapArray(*error,"code",nb,"InvalidParameterValue");
	  }
	  return -1;
	}
	
	char ltmp1[256];
	sprintf(ltmp1,"%d",fsize);
	map* tmp=getMapFromMaps(*m,"main","cacheDir");
	if(tmp!=NULL){
	  char* md5str=getMd5(tmp1->value);
	  char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
	  sprintf(fname,"%s/%s.zca",tmp->value,md5str);
	  addToMap((*in)->content,cname,fname);
	  free(fname);
	}
	addToMap((*in)->content,sname,ltmp1);
	addToMap((*in)->content,mname,mimeType);
	addToCache(*m,tmp1->value,fcontent,mimeType,fsize, NULL, 0);
	free(fcontent);
	free(mimeType);
	*index++;
	
      }
    }
  }
  if(shouldClean>0){
    freeMap(&length);
    free(length);
  }
  return 0;
}

/**
 * Effectively run all the HTTP requests in the queue
 *
 * @param m the maps containing the settings of the main.cfg file
 * @param inputs the maps containing the inputs (defined in the requests+added
 *  per default based on the zcfg file)
 * @param hInternet the HINTERNET pointer
 * @param error the error map pointer
 * @return 0 on success, -1 on failure
 */
int runHttpRequests(maps** m,maps** inputs,HINTERNET* hInternet,map** error){
  int hasAFailure=0;
  if(hInternet!=NULL && hInternet->nb>0){
    processDownloads(hInternet);
    maps* content=*inputs;
    int index=0;
    while(content!=NULL){
      if(content->child!=NULL){
	maps* cursor=content->child;
	while(cursor!=NULL){
	  int red=readCurrentInput(m,&cursor,&index,hInternet,error);
	  if(red<0)
	    hasAFailure=red;
	  cursor=cursor->next;
	}
      }
      else{
	int red=readCurrentInput(m,&content,&index,hInternet,error);
	if(red<0)
	  hasAFailure=red;
      }
      content=content->next;
    }
  }
  return hasAFailure;
}

/**
 * Add a request in the download queue
 *
 * @param m the maps containing the settings of the main.cfg file
 * @param url the url to add to the queue
 */
void addRequestToQueue(maps** m,HINTERNET* hInternet,const char* url,bool req){
  hInternet->waitingRequests[hInternet->nb]=strdup(url);
  if(req)
    InternetOpenUrl(hInternet,hInternet->waitingRequests[hInternet->nb],NULL,0,INTERNET_FLAG_NO_CACHE_WRITE,0);
  maps *oreq=getMaps(*m,"orequests");
  if(oreq==NULL){
    oreq=createMaps("orequests");
    oreq->content=createMap("value",url);
    addMapsToMaps(m,oreq);
    freeMaps(&oreq);
    free(oreq);
  }else{
    setMapArray(oreq->content,"value",hInternet->nb-1,url);
  }
}

/**
 * Try to load file from cache or download a remote file if not in cache
 *
 * @param m the maps containing the settings of the main.cfg file
 * @param content the map to update
 * @param hInternet the HINTERNET pointer
 * @param url the url to fetch
 * @return 0
 */
int loadRemoteFile(maps** m,map** content,HINTERNET* hInternet,char *url){
  char* fcontent = NULL;
  char* cached=isInCache(*m,url);
  char *mimeType=NULL;
  int fsize=0;

  map* t=getMap(*content,"xlink:href");
  if(t==NULL){
    t=getMap((*content),"href");
    addToMap(*content,"xlink:href",url);
  }

  if(cached!=NULL){

    struct stat f_status;
    int s=stat(cached, &f_status);
    if(s==0){
      fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(cached,"rb");
      fread(fcontent,f_status.st_size,1,f);
      fsize=f_status.st_size;
      fcontent[fsize]=0;
      fclose(f);
      addToMap(*content,"cache_file",cached);
    }
    cached[strlen(cached)-1]='m';
    s=stat(cached, &f_status);
    if(s==0){
      mimeType=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(cached,"rb");
      fread(mimeType,f_status.st_size,1,f);
      mimeType[f_status.st_size]=0;
      fclose(f);
    }

  }else{    
    addRequestToQueue(m,hInternet,url,true);
    return 0;
  }
  if(fsize==0){
    return errorException(*m, _("Unable to download the file."), "InternalError",NULL);
  }
  if(mimeType!=NULL){
    addToMap(*content,"fmimeType",mimeType);
  }

  map* tmpMap=getMapOrFill(content,"value","");
    
  free(tmpMap->value);
  tmpMap->value=(char*)malloc((fsize+1)*sizeof(char));
  if(tmpMap->value==NULL || fcontent == NULL)
    return errorException(*m, _("Unable to allocate memory"), "InternalError",NULL);
  memcpy(tmpMap->value,fcontent,(fsize+1)*sizeof(char));

  char ltmp1[256];
  sprintf(ltmp1,"%d",fsize);
  addToMap(*content,"size",ltmp1);
  if(cached==NULL){
    addToCache(*m,url,fcontent,mimeType,fsize, NULL, 0);
  }
  else{
    addToMap(*content,"isCached","true");
    map* tmp=getMapFromMaps(*m,"main","cacheDir");
    if(tmp!=NULL){
      map *c=getMap((*content),"xlink:href");
      char* md5str=getMd5(c->value);
      char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
      sprintf(fname,"%s/%s.zca",tmp->value,md5str);
      addToMap(*content,"cache_file",fname);
      free(fname);
    }
  }
  free(fcontent);
  free(mimeType);
  free(cached);
  return 0;
}
