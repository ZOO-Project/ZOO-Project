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

#include <openssl/md5.h>
#include <openssl/evp.h>
#include "caching.h"
#include "service.h"
#include "service_internal.h"
#include "response_print.h"
#ifdef MS_FORCE_LOCAL_FILE_USE
#include "ogr_api.h"
#include "mapserver.h"
#endif
/**
 * Compute md5
 * 
 * @param url the char* 
 * @return a char* representing the md5 of the url
 * @warning make sure to free resources returned by this function
 */
char* getMd5(char* url){
  EVP_MD_CTX *md5ctx=EVP_MD_CTX_create();
  char* fresult=(char*)malloc((EVP_MAX_MD_SIZE+1)*sizeof(char));
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int len;
  EVP_DigestInit(md5ctx, EVP_md5());
  EVP_DigestUpdate(md5ctx, url, strlen(url));
  EVP_DigestFinal_ex(md5ctx,result,&len);
  EVP_MD_CTX_destroy(md5ctx);
  int i;
  for(i = 0; i < len; i++){
    if(i>0){
      char *tmp=zStrdup(fresult);
      sprintf(fresult,"%s%02x", tmp,result[i]);
      free(tmp);
    }
    else
      sprintf(fresult,"%02x",result[i]);
  }
  return fresult;
}

/**
 * Compute md5 of a file
 * 
 * @param file the char* 
 * @return a char* representing the md5 of the url
 * @warning make sure to free resources returned by this function
 */
char* getMd5f(char* file){
  EVP_MD_CTX *md5ctx=EVP_MD_CTX_create();
  char* fresult=(char*)malloc((EVP_MAX_MD_SIZE+1)*sizeof(char));
  unsigned char result[EVP_MAX_MD_SIZE];
  unsigned int len;
  int bytes;
  int dlen=65536;
  unsigned char data[65537];
  FILE *inFile = fopen (file, "rb");
  EVP_DigestInit(md5ctx, EVP_md5());
  while ((bytes = fread (data, sizeof(unsigned char), dlen, inFile)) != 0)
    EVP_DigestUpdate(md5ctx, data, bytes);
  EVP_DigestFinal_ex(md5ctx,result,&len);
  EVP_MD_CTX_destroy(md5ctx);
  int i;
  for(i = 0; i < len; i++){
    if(i>0){
      char *tmp=zStrdup(fresult);
      sprintf(fresult,"%s%02x", tmp,result[i]);
      free(tmp);
    }
    else
      sprintf(fresult,"%02x",result[i]);
  }
  fclose (inFile);
  return fresult;
}



/**
 * Create a URL by appending every request header listed in the security 
 * section.This imply that the URL will contain any authentication 
 * informations that should be fowarded to the server from which de input
 * was download.
 * @param conf the main configuration maps
 * @param request the URL to transform.
 * @return a char* that contain the original URL plus potential header (only for
 * hosts that are not shared).
 * @warning Be sure to free the memory returned by this function.
 */
char* getFilenameForRequest(maps* conf, const char* request){
  map* passThrough=getMapFromMaps(conf,"security","attributes");
  map* targetHosts=getMapFromMaps(conf,"security","hosts");
  char* passedHeader[10];
  int cnt=0;
  char *res=zStrdup(request);
  char *toAppend=NULL;
  if(passThrough!=NULL && targetHosts!=NULL){
    char *tmp=zStrdup(passThrough->value);
    char *token, *saveptr;
    token = strtok_r (tmp, ",", &saveptr);
    int i;
    if((strstr(targetHosts->value,"*")!=NULL || isProtectedHost(targetHosts->value,request)==1) && strncasecmp(getProvenance(conf,request),"SHARED",6)!=0){
      while (token != NULL){
	int length=strlen(token)+6;
	char* tmp1=(char*)malloc(length*sizeof(char));
	map* tmpMap;
	snprintf(tmp1,6,"HTTP_");
	int j;
	for(j=0;token[j]!='\0';j++){
	  if(token[j]!='-')
	    tmp1[5+j]=toupper(token[j]);
	  else
	    tmp1[5+j]='_';
	  tmp1[5+j+1]='\0';
	}
	tmpMap = getMapFromMaps(conf,"renv",tmp1);
	if(tmpMap!=NULL){
	  if(toAppend==NULL){
	    toAppend=(char*)malloc((strlen(tmpMap->value)+1)*sizeof(char));
	    sprintf(toAppend,"%s",tmpMap->value);
	  }else{
	    char *tmp3=zStrdup(toAppend);
	    toAppend=(char*)realloc(toAppend,(strlen(tmpMap->value)+strlen(tmp3)+2)*sizeof(char));
	    sprintf(toAppend,"%s,%s",tmp3,tmpMap->value);
	    free(tmp3);
	  }
	}
	free(tmp1);
	cnt+=1;
	token = strtok_r (NULL, ",", &saveptr);
      }
    }
    free(tmp);
  }
  if(toAppend!=NULL){
    char *tmp3=zStrdup(res);
    res=(char*)realloc(res,(strlen(tmp3)+strlen(toAppend)+1)*sizeof(char));
    sprintf(res,"%s%s",tmp3,toAppend);
    free(tmp3);
    free(toAppend);
  }
  return res;
}

/**
 * Store MD5 of the content of a file
 * @file char* the full path of the file
 */
int storeMd5(char* file){
  char* storage=zStrdup(file);
  char* md5fstr=getMd5f(file);
  storage[strlen(storage)-2]='m';
  storage[strlen(storage)-1]='d';
  FILE* fo=fopen(storage,"w+");
  if(fo==NULL)
    return 1;
  fwrite(md5fstr,sizeof(char),strlen(md5fstr),fo);
  free(md5fstr);
  free(storage);
  fclose(fo);
  return 0;
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
void cacheFile(maps* conf,char* request,char* mimeType,int length,char* filename){
  map* tmp=getMapFromMaps(conf,"main","cacheDir");
  char contentr[4096];
  int cred=0;
  if(tmp!=NULL){
    char* myRequest=getFilenameForRequest(conf,request);
    char* md5str=getMd5(myRequest);
    free(myRequest);
    char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
    // Store md5
    char* md5fstr=getMd5f(filename);
    sprintf(fname,"%s/%s.zmd",tmp->value,md5str);
    FILE* fo=fopen(fname,"w+");
#ifdef DEBUG
    fprintf(stderr,"filename: %s\n",filename);
    fprintf(stderr,"MD5: %s\n",md5fstr);
#endif
    fwrite(md5fstr,sizeof(char),strlen(md5fstr),fo);
    free(md5fstr);
    fclose(fo);
    
    sprintf(fname,"%s/%s.zca",tmp->value,md5str);
    zooLock* lck=lockFile(conf,fname,'w');
    if(lck!=NULL){
#ifdef DEBUG
      fprintf(stderr,"Cache list : %s\n",fname);
      fflush(stderr);
#endif
      FILE* fi=fopen(filename,"rb");
      sprintf(fname,"%s/%s.zca",tmp->value,md5str);
      fo=fopen(fname,"w+");
      if(fo==NULL){
#ifdef DEBUG
	fprintf (stderr, "Failed to open %s for writing: %s\n",fname, strerror(errno));
#endif
	unlockFile(conf,lck);
	return;
      }
      if(fi==NULL){
#ifdef DEBUG
	fprintf (stderr, "Failed to open %s for reading: %s\n",filename, strerror(errno));
#endif
	unlockFile(conf,lck);
	return;
      }
      memset(contentr,0,4096);
      while((cred=fread(contentr,sizeof(char),4096,fi))>0){
	fwrite(contentr,sizeof(char),cred,fo);
	fflush(fo);
	memset(contentr,0,4096);
      }
      unlockFile(conf,lck);
      fclose(fo);
      fclose(fi);

      // Store mimeType
      sprintf(fname,"%s/%s.zcm",tmp->value,md5str);
      fo=fopen(fname,"w+");
#ifdef DEBUG
      fprintf(stderr,"MIMETYPE: %s\n",mimeType);
#endif
      fwrite(mimeType,sizeof(char),strlen(mimeType),fo);
      fclose(fo);

      // Store provenance
      sprintf(fname,"%s/%s.zcp",tmp->value,md5str);
      fo=fopen(fname,"w+");
      char* origin=getProvenance(conf,request);
#ifdef DEBUG
      fprintf(stderr,"ORIGIN: %s\n",mimeType);
#endif
      fwrite(origin,sizeof(char),strlen(origin),fo);
      fclose(fo);

      free(md5str);

    }
    free(fname);
  }
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
    char* myRequest=getFilenameForRequest(conf,request);
    char* md5str=getMd5(myRequest);
    free(myRequest);
    char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
    sprintf(fname,"%s/%s.zca",tmp->value,md5str);
    zooLock* lck=lockFile(conf,fname,'w');
    if(lck!=NULL){
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
	unlockFile(conf,lck);
	return;
      }
      fwrite(content,sizeof(char),length,fo);
      unlockFile(conf,lck);
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

      sprintf(fname,"%s/%s.zcp",tmp->value,md5str);
      fo=fopen(fname,"w+");
      char* origin=getProvenance(conf,request);
#ifdef DEBUG
      fprintf(stderr,"ORIGIN: %s\n",mimeType);
#endif
      fwrite(origin,sizeof(char),strlen(origin),fo);
      fclose(fo);

      free(md5str);
      free(fname);
    }
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
  map* tmpUrl=getMapFromMaps(conf,"main","tmpUrl");
  map* tmpM=getMapFromMaps(conf,"main","cacheDir");
  if(tmpM==NULL)
    tmpM=getMapFromMaps(conf,"main","tmpPath");
  if(strstr(request,tmpUrl->value)!=NULL){
    map* tmpPath=getMapFromMaps(conf,"main","tmpPath");
    char* tmpStr=strstr(request,tmpUrl->value);
    char* tmpStr1=zStrdup(tmpStr+strlen(tmpUrl->value));
    char* res=(char*) malloc((strlen(tmpPath->value)+strlen(tmpStr1)+2)*sizeof(char));
    sprintf(res,"%s/%s",tmpPath->value,tmpStr1);
    free(tmpStr1);
    return res;
  }
#ifdef MS_FORCE_LOCAL_FILE_USE
  map* msUrl=getMapFromMaps(conf,"main","mapserverAddress");
  if(msUrl!=NULL && strstr(request,msUrl->value)!=NULL){
    char *tmpStr=strstr(request,"?");
    char *cursor=zStrdup(tmpStr+1);
    char *token, *saveptr;
    token = strtok_r (cursor, "&", &saveptr);
    while(token!=NULL){
      char *token1, *saveptr1;
      token1 = strtok_r (token, "=", &saveptr1);
      char *name=NULL;
      while(token1!=NULL){
	if(name==NULL)
	  name=zStrdup(token1);
	else
	  if(strcasecmp(name,"map")==0){
	    mapObj *myMap=msLoadMap(token1,NULL);
	    char * res=zStrdup(myMap->layers[0]->data);
	    free(name);
	    free(cursor);
	    msFreeMap(myMap);
	    return res;
	  }
	token1 = strtok_r (NULL, "=", &saveptr1);
      }
      token = strtok_r (NULL, "&", &saveptr);
    }
    free(cursor);
  }
#endif  
  if(strncasecmp(request,"file://",7)==0){
    char* tmpStr=zStrdup(request+7);
    fprintf(stderr,"**** %s %d %s \n",__FILE__,__LINE__,tmpStr);
    return tmpStr;
  }
  else{
    char* myRequest=getFilenameForRequest(conf,request);
    char* md5str=getMd5(myRequest);
    free(myRequest);
#ifdef DEBUG
    fprintf(stderr,"MD5STR : (%s)\n\n",md5str);
#endif
    char* fname=(char*)malloc(sizeof(char)*(strlen(tmpM->value)+strlen(md5str)+6));
    sprintf(fname,"%s/%s.zca",tmpM->value,md5str);
    zStatStruct f_status;
    int s=zStat(fname, &f_status);
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
  
  int shouldClean=-1;
  map* tmp1;
  char sindex[5];
  maps* content=*in;
  map* length=getMap(content->content,"length");
  map* memUse=getMapFromMaps(*m,"main","memory");
  if(length==NULL){
    length=createMap("length","1");
    shouldClean=1;
  }
  for(int i=0;i<atoi(length->value);i++){
    char* fcontent;
    char *mimeType=NULL;
    int fsize=0;
    char oriname[12];
    char cname[15];
    char vname[11];
    char vname1[11];
    char sname[9];
    char mname[15];
    char icname[14];
    char xname[16];
    char bname[8];
    char hname[11];
    char oname[12];
    char ufile[12];    
    if(*index>0)
      sprintf(vname1,"value_%d",*index);
    else
      sprintf(vname1,"value");
    
    if(i>0){
      sprintf(cname,"cache_file_%d",i);
      tmp1=getMap(content->content,cname);
      sprintf(oriname,"origin_%d",i);
      sprintf(vname,"value_%d",i);
      sprintf(sname,"size_%d",i);
      sprintf(mname,"mimeType_%d",i);
      sprintf(icname,"isCached_%d",i);
      sprintf(xname,"Reference_%d",i);
      sprintf(bname,"body_%d",i);
      sprintf(hname,"headers_%d",i);
      sprintf(oname,"Order_%d",i);
      sprintf(ufile,"use_file_%d",i);
    }else{
      sprintf(cname,"cache_file");
      sprintf(oriname,"origin");
      sprintf(vname,"value");
      sprintf(sname,"size");
      sprintf(mname,"mimeType");
      sprintf(icname,"isCached");
      sprintf(xname,"Reference");
      sprintf(bname,"body");
      sprintf(hname,"headers");
      sprintf(oname,"Order");
      sprintf(ufile,"use_file");
    }
    
    map* tmap=getMap(content->content,oname);
    sprintf(sindex,"%d",*index+1);
    if((tmp1=getMap(content->content,xname))!=NULL && tmap!=NULL && strcasecmp(tmap->value,sindex)==0){

      if(getMap(content->content,icname)==NULL) {
	if(memUse==NULL || strcasecmp(memUse->value,"load")==0){
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
	}
	fsize=hInternet->ihandle[*index].nDataLen;
	if(hInternet->ihandle[*index].mimeType==NULL)
	  mimeType=zStrdup("none");
	else
	  mimeType=zStrdup(hInternet->ihandle[*index].mimeType);	      
	
	map* tmpMap=getMapOrFill(&(*in)->content,vname,"");
	if(memUse==NULL || strcasecmp(memUse->value,"load")==0){
	  free(tmpMap->value);
	  tmpMap->value=(char*)malloc((fsize+1)*sizeof(char));
	  if(tmpMap->value==NULL){
	    return errorException(*m, _("Unable to allocate memory"), "InternalError",NULL);
	  }
	  memcpy(tmpMap->value,fcontent,(fsize+1)*sizeof(char));
	}else
	  addToMap((*in)->content,ufile,"true");
	if(hInternet->ihandle[*index].code!=200){
	  const char *error_rep_str=_("Unable to download the file for the input <%s>, response code was : %d.");
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
	char *request=NULL;
	if(tmp!=NULL){
	  map* tmp2;
	  char* md5str=NULL;
	  if((tmp2=getMap(content->content,bname))!=NULL){
	    char *tmpStr=(char*)malloc((strlen(tmp1->value)+strlen(tmp2->value)+1)*sizeof(char));
	    sprintf(tmpStr,"%s%s",tmp1->value,tmp2->value);
	    if((tmp2=getMap(content->content,"headers"))!=NULL){
	      char *tmpStr2=zStrdup(tmpStr);
	      free(tmpStr);
	      tmpStr=(char*)malloc((strlen(tmpStr2)+strlen(tmp2->value)+1)*sizeof(char));
	      sprintf(tmpStr,"%s%s",tmpStr2,tmp2->value);
	      free(tmpStr2);
	    }
	    md5str=getMd5(tmpStr);
	    request=zStrdup(tmpStr);
	    free(tmpStr);
	  }else{
	    char *myRequest=getFilenameForRequest(*m,tmp1->value);
	    md5str=getMd5(myRequest);
	    request=zStrdup(tmp1->value);
	    free(myRequest);
	  }
	  char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
	  sprintf(fname,"%s/%s.zca",tmp->value,md5str);
	  addToMap((*in)->content,cname,fname);
	  free(fname);
	}
	addToMap((*in)->content,sname,ltmp1);
	addToMap((*in)->content,mname,mimeType);
	char* origin=getProvenance(*m,request);
	addToMap((*in)->content,oriname,origin);
	if(memUse==NULL || strcasecmp(memUse->value,"load")==0){
	  addToCache(*m,request,fcontent,mimeType,fsize, NULL, 0);
	  free(fcontent);
	}else{
	  addToMap((*in)->content,ufile,"true");
	  cacheFile(*m,request,mimeType,fsize,hInternet->ihandle[*index].filename);
	}
	free(mimeType);
	free(request);
	(*index)++;
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
    AddHeaderEntries(hInternet,*m);
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
  hInternet->waitingRequests[hInternet->nb]=zStrdup(url);
  if(req)
    InternetOpenUrl(hInternet,hInternet->waitingRequests[hInternet->nb],NULL,0,INTERNET_FLAG_NO_CACHE_WRITE,0,*m);
  maps *oreq=getMaps(*m,"orequests");
  if(oreq==NULL){
    oreq=createMaps("orequests");
    oreq->content=createMap("value",url);
    addMapsToMaps(m,oreq);
    freeMaps(&oreq);
    free(oreq);
  }else{
    setMapArray(oreq->content,"value",hInternet->nb,url);
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
  char *origin=NULL;
  long long fsize=0;
  map* memUse=getMapFromMaps(*m,"main","memory");

  map* t=getMap(*content,"xlink:href");
  if(t==NULL){
    t=getMap((*content),"href");
    addToMap(*content,"xlink:href",url);
  }

  if(cached!=NULL){
    zStatStruct f_status;
    int s=zStat(cached, &f_status);
    if(s==0){
      zooLock* lck=lockFile(*m,cached,'r');
      if(lck==NULL)
	return -1;
      fsize=f_status.st_size;
      if(memUse==NULL || strcasecmp(memUse->value,"load")==0){
	fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
	FILE* f=fopen(cached,"rb");
	if(f!=NULL){
	  fread(fcontent,f_status.st_size,1,f);
	  fcontent[fsize]=0;
	  fclose(f);
	}
      }
      addToMap(*content,"cache_file",cached);
      unlockFile(*m,lck);
    }
    cached[strlen(cached)-1]='m';
    s=zStat(cached, &f_status);
    if(s==0){
      zooLock* lck=lockFile(*m,cached,'r');
      if(lck==NULL)
	return -1;
      mimeType=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(cached,"rb");
      fread(mimeType,f_status.st_size,1,f);
      mimeType[f_status.st_size]=0;
      fclose(f);
      unlockFile(*m,lck);
    }
    cached[strlen(cached)-1]='p';
    s=zStat(cached, &f_status);
    if(s==0){
      zooLock* lck=lockFile(*m,cached,'r');
      if(lck==NULL)
	return -1;
      origin=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(cached,"rb");
      fread(origin,f_status.st_size,1,f);
      mimeType[f_status.st_size]=0;
      fclose(f);
      unlockFile(*m,lck);
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
  if(origin!=NULL){
    addToMap(*content,"origin",origin);
  }

  map* tmpMap=getMapOrFill(content,"value","");
  if(memUse==NULL || strcasecmp(memUse->value,"load")==0){
    free(tmpMap->value);
    tmpMap->value=(char*)malloc((fsize+1)*sizeof(char));
    if(tmpMap->value==NULL || fcontent == NULL)
      return errorException(*m, _("Unable to allocate memory"), "InternalError",NULL);
    memcpy(tmpMap->value,fcontent,(fsize+1)*sizeof(char));
  }
  
  char ltmp1[256];
  sprintf(ltmp1,"%ld",fsize);
  addToMap(*content,"size",ltmp1);
  if(cached==NULL){
    if(memUse==NULL || strcasecmp(memUse->value,"load")==0)
      addToCache(*m,url,fcontent,mimeType,fsize, NULL, 0);
    else
      cacheFile(*m,url,mimeType,fsize,hInternet->ihandle[hInternet->nb-1].filename);
  }
  else{
    addToMap(*content,"isCached","true");
    map* tmp=getMapFromMaps(*m,"main","cacheDir");
    map* tmp1=getMap((*content),"cache_file");
    if(tmp!=NULL && tmp1==NULL){
      map *c=getMap((*content),"xlink:href");
      if(strncasecmp(c->value,"file://",7)!=0){
	char *myRequest=getFilenameForRequest(*m,c->value);
	char* md5str=getMd5(myRequest);
	free(myRequest);
	char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
	sprintf(fname,"%s/%s.zca",tmp->value,md5str);
	addToMap(*content,"cache_file",fname);
	free(fname);
	free(md5str);
      }
    }
  }
  if(fcontent!=NULL)
    free(fcontent);
  if(mimeType!=NULL)
    free(mimeType);
  if(cached!=NULL)
    free(cached);
  return 0;
}
