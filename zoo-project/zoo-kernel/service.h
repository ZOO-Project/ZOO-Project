/**
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2012 GeoLabs SARL
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

#ifndef ZOO_SERVICE_H
#define ZOO_SERVICE_H 1

#pragma once

#ifdef WIN32
#ifndef USE_MS
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif
#ifndef snprintf
#define snprintf sprintf_s
#endif
#define zStrdup _strdup
#define zMkdir _mkdir
#define zOpen _open
#define zWrite _write
#define zSleep Sleep
#include <sys/timeb.h>
struct ztimeval {
  long tv_sec; /* seconds */
  long tv_usec; /* and microseconds */
};
static int zGettimeofday(struct ztimeval* tp, void* tzp)
{
  if (tp == 0) {
    return -1;
  }
  
  struct _timeb theTime;
  _ftime(&theTime);
  tp->tv_sec = theTime.time;
  tp->tv_usec = theTime.millitm * 1000;
  
  return 0; // The gettimeofday() function shall return 0 on success
}
#else
#define zStrdup strdup
#define zMkdir mkdir
#define zOpen open
#define zWrite write
#define zSleep sleep
#define zGettimeofday gettimeofday
#define ztimeval timeval
#endif 

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#ifdef USE_MS
#include <mapserver.h>
#endif
#endif
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#ifndef bool
#define bool int
#endif
#ifndef true
#define true 1
#define false -1
#endif
#endif

#define SERVICE_ACCEPTED 0
#define SERVICE_STARTED 1
#define SERVICE_PAUSED 2
#define SERVICE_SUCCEEDED 3
#define SERVICE_FAILED 4

#define ELEMENTS_SIZE (sizeof(char*)+(((2*sizeof(char*))+sizeof(maps*))*2)+sizeof(char*)+(((2*sizeof(char*))+sizeof(iotype*))*2)+sizeof(elements*))
#define MAP_SIZE (2*sizeof(char*))+sizeof(NULL)
#define IOTYPE_SIZE MAP_SIZE+sizeof(NULL)
#define MAPS_SIZE (2*sizeof(char*))+sizeof(map*)+MAP_SIZE
#define SERVICE_SIZE (ELEMENTS_SIZE*2)+(MAP_SIZE*2)+sizeof(char*)

#define SHMSZ     27

#include "version.h" 

#ifdef DEBUG_STACK
  void debugStack(const char* file,const int line){
    int stack;
    fprintf(stderr,"stack %p (%s: %d) \n",&stack,file,line);
  }
#endif

  /**
   * \struct map
   * \brief KVP linked list
   *
   * Deal with WPS KVP (name,value).
   * A map is defined as:
   *  - name : a key,
   *  - value: a value,
   *  - next : a pointer to the next map if any.
   */
  typedef struct map{
    char* name;
    char* value;
    struct map* next;
  } map;

#ifdef WIN32
#define NULLMAP ((map*) 0)
#else
#define NULLMAP NULL
#endif

  /**
   * \struct maps
   * \brief linked list of map pointer
   *
   * Small object to store WPS KVP set.
   * Maps is defined as:
   *  - a name, 
   *  - a content map,
   *  - a pointer to the next maps if any.
   */
  typedef struct maps{
    char* name;          
    struct map* content; 
    struct maps* next;   
  } maps;

  /**
   * \brief Dump a map on stderr
   */
  static void _dumpMap(map* t){
    if(t!=NULL){
      fprintf(stderr,"%s: %s\n",t->name,t->value);
      fflush(stderr);
	}else{
      fprintf(stderr,"NULL\n");
      fflush(stderr);
    }
  }

  static void dumpMap(map* t){
    map* tmp=t;
    while(tmp!=NULL){
      _dumpMap(tmp);
      tmp=tmp->next;
    }
  }

  static void dumpMapToFile(map* t,FILE* file){
    map* tmp=t;
    while(tmp!=NULL){
#ifdef DEBUG
      fprintf(stderr,"%s = %s\n",tmp->name,tmp->value);
#endif
      fprintf(file,"%s = %s\n",tmp->name,tmp->value);
      tmp=tmp->next;
    }
  }

  static void dumpMaps(maps* m){
    maps* tmp=m;
    while(tmp!=NULL){
      fprintf(stderr,"MAP => [%s] \n",tmp->name);
      dumpMap(tmp->content);
      tmp=tmp->next;
    }
  }

  static void dumpMapsToFile(maps* m,char* file_path){
    FILE* file=fopen(file_path,"w");
    maps* tmp=m;
    if(tmp!=NULL){
      fprintf(file,"[%s]\n",tmp->name);
      dumpMapToFile(tmp->content,file);
      fflush(file);
    }
    fclose(file);
  }

  static map* createMap(const char* name,const char* value){
    map* tmp=(map *)malloc(MAP_SIZE);
    tmp->name=zStrdup(name);
    tmp->value=zStrdup(value);
    tmp->next=NULL;
    return tmp;
  }

  static int count(map* m){
    map* tmp=m;
    int c=0;
    while(tmp!=NULL){
      c++;
      tmp=tmp->next;
    }
    return c;
  }
    
  static bool hasKey(map* m,const char *key){
    map* tmp=m;
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,key)==0)
	return true;
      tmp=tmp->next;
    }
#ifdef DEBUG_MAP
    fprintf(stderr,"NOT FOUND \n");
#endif
    return false;
  }

  static maps* getMaps(maps* m,const char *key){
    maps* tmp=m;
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,key)==0){
	return tmp;
      }
      tmp=tmp->next;
    }
    return NULL;
  }

  static map* getMap(map* m,const char *key){
    map* tmp=m;
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,key)==0){
	return tmp;
      }
      tmp=tmp->next;
    }
    return NULL;
  }


  static map* getLastMap(map* m){
    map* tmp=m;
    while(tmp!=NULL){
      if(tmp->next==NULL){
	return tmp;
      }
      tmp=tmp->next;
    }
    return NULL;
  }

  static map* getMapFromMaps(maps* m,const char* key,const char* subkey){
    maps* _tmpm=getMaps(m,key);
    if(_tmpm!=NULL){
      map* _ztmpm=getMap(_tmpm->content,subkey);
      return _ztmpm;
    }
    else return NULL;
  }


  static void freeMap(map** mo){
    map* _cursor=*mo;
    if(_cursor!=NULL){
#ifdef DEBUG
      fprintf(stderr,"freeMap\n");
#endif
      free(_cursor->name);
      free(_cursor->value);
      if(_cursor->next!=NULL){
	freeMap(&_cursor->next);
	free(_cursor->next);
      }
    }
  }

  static void freeMaps(maps** mo){
    maps* _cursor=*mo;
    if(_cursor && _cursor!=NULL){
#ifdef DEBUG
      fprintf(stderr,"freeMaps\n");
#endif
      free(_cursor->name);
      if(_cursor->content!=NULL){
      	freeMap(&_cursor->content);
      	free(_cursor->content);
      }
      if(_cursor->next!=NULL){
	freeMaps(&_cursor->next);
	free(_cursor->next);
      }
    }
  }

  /**
   * \brief Not named linked list
   *
   * Used to store informations about formats, such as mimeType, encoding ... 
   *
   * An iotype is defined as :
   *  - a content map,
   *  - a pointer to the next iotype if any.
   */
  typedef struct iotype{
    struct map* content;
    struct iotype* next;
  } iotype;

  /**
   * \brief Metadata information about input or output.
   *
   * The elements are used to store metadata informations defined in the ZCFG.
   *
   * An elements is defined as :
   *  - a name,
   *  - a content map,
   *  - a metadata map,
   *  - a format (possible values are LiteralData, ComplexData or 
   * BoundingBoxData),
   *  - a default iotype,
   *  - a pointer to the next elements id any.
   */
  typedef struct elements{
    char* name;
    struct map* content;
    struct map* metadata;
    char* format;
    struct iotype* defaults;
    struct iotype* supported;
    struct elements* next;
  } elements;

  typedef struct service{
    char* name;
    struct map* content;
    struct map* metadata;
    struct elements* inputs;
    struct elements* outputs; 
  } service;

  typedef struct services{
    struct service* content; 
    struct services* next; 
  } services;

  static bool hasElement(elements* e,const char* key){
    elements* tmp=e;
    while(tmp!=NULL){
      if(strcasecmp(key,tmp->name)==0)
	return true;
      tmp=tmp->next;
    }
    return false;
  }

  static elements* getElements(elements* m,char *key){
    elements* tmp=m;
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,key)==0)
	return tmp;
      tmp=tmp->next;
    }
    return NULL;
  }


  static void freeIOType(iotype** i){
    iotype* _cursor=*i;
    if(_cursor!=NULL){
      if(_cursor->next!=NULL){
	freeIOType(&_cursor->next);
	free(_cursor->next);
      }
      freeMap(&_cursor->content);
      free(_cursor->content);
    }
  }

  static void freeElements(elements** e){
    elements* tmp=*e;
    if(tmp!=NULL){
      if(tmp->name!=NULL)
	free(tmp->name);
      freeMap(&tmp->content);
      if(tmp->content!=NULL)
	free(tmp->content);
      freeMap(&tmp->metadata);
      if(tmp->metadata!=NULL)
	free(tmp->metadata);
      if(tmp->format!=NULL)
	free(tmp->format);
      freeIOType(&tmp->defaults);
      if(tmp->defaults!=NULL)
	free(tmp->defaults);
      freeIOType(&tmp->supported);
      if(tmp->supported!=NULL){
	free(tmp->supported);
      }
      freeElements(&tmp->next);
      if(tmp->next!=NULL)
	free(tmp->next);
    }
  }

  static void freeService(service** s){
    service* tmp=*s;
    if(tmp!=NULL){
      if(tmp->name!=NULL)
	free(tmp->name);
      freeMap(&tmp->content);
      if(tmp->content!=NULL)
	free(tmp->content);
      freeMap(&tmp->metadata);
      if(tmp->metadata!=NULL)
	free(tmp->metadata);
      freeElements(&tmp->inputs);
      if(tmp->inputs!=NULL)
	free(tmp->inputs);
      freeElements(&tmp->outputs);
      if(tmp->outputs!=NULL)
	free(tmp->outputs);
    }
  }

  static void addToMap(map* m,const char* n,const char* v){
    if(hasKey(m,n)==false){
      map* _cursor=m;
      while(_cursor->next!=NULL){
	_cursor=_cursor->next;
      }
      _cursor->next=createMap(n,v);
    }
    else{
      map *tmp=getMap(m,n);
      if(tmp->value!=NULL)
	free(tmp->value);
      tmp->value=zStrdup(v);
    }
  }

  static void addToMapWithSize(map* m,const char* n,const char* v,int size){
    if(hasKey(m,n)==false){
      map* _cursor=m;
      if(_cursor!=NULL){
	addToMap(m,n,"");
      }else{
	m=createMap(n,"");
      }
    }
    map *tmp=getMap(m,n);
    if(tmp->value!=NULL)
      free(tmp->value);
    tmp->value=(char*)malloc((size+1)*sizeof(char));
    memmove(tmp->value,v,size*sizeof(char));
    tmp->value[size]=0;
    char sin[128];
    sprintf(sin,"%ld",size);
    addToMap(m,"size",sin);
  }

  static void addMapToMap(map** mo,map* mi){
    map* tmp=mi;
    map* _cursor=*mo;
    while(tmp!=NULL){
      if(_cursor==NULL){
	*mo=createMap(tmp->name,tmp->value);
	(*mo)->next=NULL;
      }
      else{
#ifdef DEBUG
	fprintf(stderr,"_CURSOR\n");
	dumpMap(_cursor);
#endif
	while(_cursor->next!=NULL)
	  _cursor=_cursor->next;
	map* tmp1=getMap(*mo,tmp->name);
	if(tmp1==NULL){
	  _cursor->next=createMap(tmp->name,tmp->value);
	}
	else{
	  addToMap(*mo,tmp->name,tmp->value);
	}
      }
      _cursor=*mo;
      tmp=tmp->next;
#ifdef DEBUG
      fprintf(stderr,"MO\n");
      dumpMap(*mo);
#endif
    }
  }

  static void addMapToIoType(iotype** io,map* mi){
    iotype* tmp=*io;
    while(tmp->next!=NULL){
      tmp=tmp->next;
    }
    tmp->next=(iotype*)malloc(IOTYPE_SIZE);
    tmp->next->content=NULL;
    addMapToMap(&tmp->next->content,mi);
    tmp->next->next=NULL;
  }

  static map* getMapOrFill(map** m,const char *key,const char* value){
    map* tmp=*m;
    map* tmpMap=getMap(tmp,key);
    if(tmpMap==NULL){
      if(tmp!=NULL){
	addToMap((*m),key,value);
      }
      else
	(*m)=createMap(key,value);
      tmpMap=getMap(*m,key);
    }
    return tmpMap;
  }

  static bool contains(map* m,map* i){
    while(i!=NULL){      
      if(strcasecmp(i->name,"value")!=0 &&
	 strcasecmp(i->name,"xlink:href")!=0 &&
	 strcasecmp(i->name,"useMapServer")!=0 &&
	 strcasecmp(i->name,"asReference")!=0){
	map *tmp;
	if(hasKey(m,i->name) && (tmp=getMap(m,i->name))!=NULL && 
	   strcasecmp(i->value,tmp->value)!=0)
	  return false;
      }
      i=i->next;
    }
    return true;
  }

  static iotype* getIoTypeFromElement(elements* e,char *name, map* values){
    elements* cursor=e;
    while(cursor!=NULL){
      if(strcasecmp(cursor->name,name)==0){
	if(contains(cursor->defaults->content,values)==true)
	  return cursor->defaults;
	else{
	  iotype* tmp=cursor->supported;
	  while(tmp!=NULL){
	    if(contains(tmp->content,values)==true)
	      return tmp;	    
	    tmp=tmp->next;
	  }
	}
      }
      cursor=cursor->next;
    }
    return NULL;
  }

  static void loadMapBinary(map** out,map* in,int pos){
    map* size=getMap(in,"size");
    map *lout=*out;
    if(size!=NULL && pos>0){
      char tmp[11];
      sprintf(tmp,"size_%d",pos);
      size=getMap(in,tmp);
      sprintf(tmp,"value_%d",pos);
      map* tmpVin=getMap(in,tmp);
      map* tmpVout=getMap(lout,tmp);
      free(tmpVout->value);
      tmpVout->value=(char*)malloc((atoi(size->value)+1)*sizeof(char));
      memmove(tmpVout->value,tmpVin->value,atoi(size->value)*sizeof(char));
      tmpVout->value[atoi(size->value)]=0;
    }else{
      if(size!=NULL){
	map* tmpVin=getMap(in,"value");
	map* tmpVout=getMap(lout,"value");
	free(tmpVout->value);
	tmpVout->value=(char*)malloc((atoi(size->value)+1)*sizeof(char));
	memmove(tmpVout->value,tmpVin->value,atoi(size->value)*sizeof(char));
	tmpVout->value[atoi(size->value)]=0;
      }
    }
  }
  
  static void loadMapBinaries(map** out,map* in){
    map* size=getMap(in,"size");
    map* length=getMap(in,"length");
    if(length!=NULL){
      int len=atoi(length->value);
      int i=0;
      for(i=0;i<len;i++){
	loadMapBinary(out,in,i);
      }
    }
    else
      if(size!=NULL)
	loadMapBinary(out,in,-1);
    char* tmpSized=NULL;
    
  }

  static maps* dupMaps(maps** mo){
    maps* _cursor=*mo;
    maps* res=NULL;
    if(_cursor!=NULL){
      res=(maps*)malloc(MAPS_SIZE);
      res->name=zStrdup(_cursor->name);
      res->content=NULL;
      res->next=NULL;
      map* mc=_cursor->content;
      if(mc!=NULL){
	addMapToMap(&res->content,mc);
	loadMapBinaries(&res->content,mc);
      }
      res->next=dupMaps(&_cursor->next);
    }
    return res;
  }

  static void addMapsToMaps(maps** mo,maps* mi){
    maps* tmp=mi;
    maps* _cursor=*mo;
    while(tmp!=NULL){
      if(_cursor==NULL){
	*mo=dupMaps(&mi);
      }
      else{
	while(_cursor->next!=NULL)
	  _cursor=_cursor->next;
	maps* tmp1=getMaps(*mo,tmp->name);
	if(tmp1==NULL)
	  _cursor->next=dupMaps(&tmp);
	else
	  addMapToMap(&tmp1->content,tmp->content);
	_cursor=*mo;
      }
      tmp=tmp->next;
    }
  }

  static map* getMapArray(map* m,const char* key,int index){
    char tmp[1024];
    if(index>0)
      sprintf(tmp,"%s_%d",key,index);
    else
      sprintf(tmp,"%s",key);
#ifdef DEBUG
    fprintf(stderr,"** KEY %s\n",tmp);
#endif
    map* tmpMap=getMap(m,tmp);
#ifdef DEBUG
    if(tmpMap!=NULL)
      dumpMap(tmpMap);
#endif
    return tmpMap;
  }


  static void setMapArray(map* m,char* key,int index,char* value){
    char tmp[1024];
    if(index>0)
      sprintf(tmp,"%s_%d",key,index);
    else
      sprintf(tmp,"%s",key);
    map* tmpSize=getMapArray(m,"size",index);
    if(tmpSize!=NULL && strncasecmp(key,"value",5)==0){
#ifdef DEBUG
      fprintf(stderr,"%s\n",tmpSize->value);
#endif
      map* ptr=getMapOrFill(&m,tmp,(char *)"");
      free(ptr->value);
      ptr->value=(char*)malloc((atoi(tmpSize->value)+1)*sizeof(char));
      memcpy(ptr->value,value,atoi(tmpSize->value)); 
    }
    else
      addToMap(m,tmp,value);
  }

  static map* getMapType(map* mt){
    map* tmap=getMap(mt,(char *)"mimeType");
    if(tmap==NULL){
      tmap=getMap(mt,"dataType");
      if(tmap==NULL){
	tmap=getMap(mt,"CRS");
      }
    }
#ifdef DEBUG
	dumpMap(tmap);
#endif
    return tmap;
  }

  static int addMapsArrayToMaps(maps** mo,maps* mi,char* typ){
    maps* tmp=mi;    
    maps* _cursor=getMaps(*mo,tmp->name);

    if(_cursor==NULL)
      return -1;

    map* tmpLength=getMap(_cursor->content,"length");
    char tmpLen[10];
    int len=1;
    if(tmpLength!=NULL){
      len=atoi(tmpLength->value);
    }

    char *tmpV[11]={
      (char*)"size",
      (char*)"value",
      (char*)"uom",
      (char*)"Reference",
      (char*)"cache_file",
      (char*)"fmimeType",
      (char*)"xlink:href",
      typ,
      (char*)"schema",
      (char*)"encoding",
      (char*)"isCached"
    };
    sprintf(tmpLen,"%d",len+1);
    addToMap(_cursor->content,"length",tmpLen);
    int i=0;
    for(i=0;i<11;i++){
      map* tmpVI=getMap(tmp->content,tmpV[i]);
      if(tmpVI!=NULL){
#ifdef DEBUG
	fprintf(stderr,"%s = %s\n",tmpV[i],tmpVI->value);
#endif
	if(i<7)
	  setMapArray(_cursor->content,tmpV[i],len,tmpVI->value);
	else
	  if(strncasecmp(tmpV[7],"mimeType",8)==0)
	    setMapArray(_cursor->content,tmpV[i],len,tmpVI->value);
      }
    }
    
    addToMap(_cursor->content,"isArray","true");
    return 0;
  }

  static void setMapInMaps(maps* m,const char* key,const char* subkey,const char *value){
    maps* _tmpm=getMaps(m,key);
    if(_tmpm!=NULL){
      map* _ztmpm=getMap(_tmpm->content,subkey);
      if(_ztmpm!=NULL){
	if(_ztmpm->value!=NULL)
	  free(_ztmpm->value);
	_ztmpm->value=zStrdup(value);
      }else{
	maps *tmp=(maps*)malloc(MAPS_SIZE);
	tmp->name=zStrdup(key);
	tmp->content=createMap(subkey,value);
	tmp->next=NULL;
	addMapsToMaps(&_tmpm,tmp);
	freeMaps(&tmp);
	free(tmp);
      }
    }else{
      maps *tmp=(maps*)malloc(MAPS_SIZE);
      tmp->name=zStrdup(key);
      tmp->content=createMap(subkey,value);
      tmp->next=NULL;
      addMapsToMaps(&m,tmp);
      freeMaps(&tmp);
      free(tmp);
    }
  }


  static void dumpElements(elements* e){
    elements* tmp=e;
    while(tmp!=NULL){
      fprintf(stderr,"ELEMENT [%s]\n",tmp->name);
      fprintf(stderr," > CONTENT [%s]\n",tmp->name);
      dumpMap(tmp->content);
      fprintf(stderr," > METADATA [%s]\n",tmp->name);
      dumpMap(tmp->metadata);
      fprintf(stderr," > FORMAT [%s]\n",tmp->format);
      iotype* tmpio=tmp->defaults;
      int ioc=0;
      while(tmpio!=NULL){
	fprintf(stderr," > DEFAULTS [%s] (%i)\n",tmp->name,ioc);
	dumpMap(tmpio->content);
	tmpio=tmpio->next;
	ioc++;
      }
      tmpio=tmp->supported;
      ioc=0;
      while(tmpio!=NULL){
	fprintf(stderr," > SUPPORTED [%s] (%i)\n",tmp->name,ioc);
	dumpMap(tmpio->content);
	tmpio=tmpio->next;
	ioc++;
      }
      fprintf(stderr,"------------------\n");
      tmp=tmp->next;
    }
  }

  static void dumpElementsAsYAML(elements* e){
    elements* tmp=e;
    int i;
    while(tmp!=NULL){
      for(i=0;i<2;i++)
	fprintf(stderr," ");
      fprintf(stderr,"%s:\n",tmp->name);
      map* mcurs=tmp->content;
      while(mcurs!=NULL){
	for(i=0;i<4;i++)
	  fprintf(stderr," ");
	_dumpMap(mcurs);
	mcurs=mcurs->next;
      }
      mcurs=tmp->metadata;
      if(mcurs!=NULL){
	for(i=0;i<4;i++)
	  fprintf(stderr," ");
	fprintf(stderr,"MetaData:\n");
	while(mcurs!=NULL){
	  for(i=0;i<6;i++)
	    fprintf(stderr," ");
	  _dumpMap(mcurs);
	  mcurs=mcurs->next;
	}
      }
      for(i=0;i<4;i++)
	fprintf(stderr," ");
      fprintf(stderr,"%s:\n",tmp->format);
      iotype* tmpio=tmp->defaults;
      int ioc=0;
      while(tmpio!=NULL){
	for(i=0;i<6;i++)
	  fprintf(stderr," ");
	fprintf(stderr,"default:\n");
	mcurs=tmpio->content;
	while(mcurs!=NULL){
	  for(i=0;i<8;i++)
	    fprintf(stderr," ");
	  if(strcasecmp(mcurs->name,"range")==0){
	    fprintf(stderr,"range: \"%s\"\n",mcurs->value);
	  }else
	    _dumpMap(mcurs);
	  mcurs=mcurs->next;
	}
	tmpio=tmpio->next;
	ioc++;
      }
      tmpio=tmp->supported;
      ioc=0;
      while(tmpio!=NULL){
	for(i=0;i<6;i++)
	  fprintf(stderr," ");
	fprintf(stderr,"supported:\n");
	mcurs=tmpio->content;
	while(mcurs!=NULL){
	  for(i=0;i<8;i++)
	    fprintf(stderr," ");
	  if(strcasecmp(mcurs->name,"range")==0){
	    fprintf(stderr,"range: \"%s\"\n",mcurs->value);
	  }else
	    _dumpMap(mcurs);
	  mcurs=mcurs->next;
	}
	tmpio=tmpio->next;
	ioc++;
      }
      tmp=tmp->next;
    }
  }


  static elements* dupElements(elements* e){
    elements* cursor=e;
    elements* tmp=NULL;
    if(cursor!=NULL){
#ifdef DEBUG
      fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
      dumpElements(e);
      fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
#endif
      tmp=(elements*)malloc(ELEMENTS_SIZE);
      tmp->name=zStrdup(e->name);
      tmp->content=NULL;
      addMapToMap(&tmp->content,e->content);
      tmp->metadata=NULL;
      addMapToMap(&tmp->metadata,e->metadata);
      tmp->format=zStrdup(e->format);
      if(e->defaults!=NULL){
	tmp->defaults=(iotype*)malloc(IOTYPE_SIZE);
	tmp->defaults->content=NULL;
	addMapToMap(&tmp->defaults->content,e->defaults->content);
	tmp->defaults->next=NULL;
#ifdef DEBUG
	fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
	dumpMap(tmp->defaults->content);
#endif
      }else
	tmp->defaults=NULL;
      if(e->supported!=NULL){
	tmp->supported=(iotype*)malloc(IOTYPE_SIZE);
	tmp->supported->content=NULL;
	addMapToMap(&tmp->supported->content,e->supported->content);
	tmp->supported->next=NULL;
	iotype *tmp2=e->supported->next;
	while(tmp2!=NULL){
	  addMapToIoType(&tmp->supported,tmp2->content);
#ifdef DEBUG
	  fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
	  dumpMap(tmp->defaults->content);
#endif
	  tmp2=tmp2->next;
	}
      }
      else
	tmp->supported=NULL;
      tmp->next=dupElements(cursor->next);
    }
    return tmp;
  }

  static void addToElements(elements** m,elements* e){
    elements* tmp=e;
    if(*m==NULL){
      *m=dupElements(tmp);
    }else{
      addToElements(&(*m)->next,tmp);
    }
  }

  static void dumpService(service* s){
    fprintf(stderr,"++++++++++++++++++\nSERVICE [%s]\n++++++++++++++++++\n",s->name);
    if(s->content!=NULL){
      fprintf(stderr,"CONTENT MAP\n");
      dumpMap(s->content);
      fprintf(stderr,"CONTENT METADATA\n");
      dumpMap(s->metadata);
    }
    if(s->inputs!=NULL){
      fprintf(stderr,"INPUT ELEMENTS [%s]\n------------------\n",s->name);
      dumpElements(s->inputs);
    }
    if(s->outputs!=NULL){
      fprintf(stderr,"OUTPUT ELEMENTS [%s]\n------------------\n",s->name);
      dumpElements(s->outputs);
    }
    fprintf(stderr,"++++++++++++++++++\n");
  }

  static void dumpServiceAsYAML(service* s){
    int i;
    fprintf(stderr,"# %s\n\n",s->name);
    if(s->content!=NULL){
      map* mcurs=s->content;
      dumpMap(mcurs);
      mcurs=s->metadata;
      if(mcurs!=NULL){
	fprintf(stderr,"MetaData:\n");
	while(mcurs!=NULL){
	  for(i=0;i<2;i++)
	    fprintf(stderr," ");
	  _dumpMap(mcurs);
	  mcurs=mcurs->next;
	}
      }
    }
    if(s->inputs!=NULL){
      fprintf(stderr,"\ninputs:\n");
      dumpElementsAsYAML(s->inputs);
    }
    if(s->outputs!=NULL){
      fprintf(stderr,"\noutputs:\n");
      dumpElementsAsYAML(s->outputs);
    }
  }

  static void mapsToCharXXX(maps* m,char*** c){
    maps* tm=m;
    int i=0;
    int j=0;
    char tmp[10][30][1024];
    memset(tmp,0,1024*10*10);
    while(tm!=NULL){
      if(i>=10)
	break;
      strcpy(tmp[i][j],"name");
      j++;
      strcpy(tmp[i][j],tm->name);
      j++;
      map* tc=tm->content;
      while(tc!=NULL){
	if(j>=30)
	  break;
	strcpy(tmp[i][j],tc->name);
	j++;
	strcpy(tmp[i][j],tc->value);
	j++;
	tc=tc->next;
      }
      tm=tm->next;
      j=0;
      i++;
    }
    memcpy(c,tmp,10*10*1024);
  }

  static void charxxxToMaps(char*** c,maps**m){
    maps* trorf=*m;
    int i,j;
    char tmp[10][30][1024];
    memcpy(tmp,c,10*30*1024);
    for(i=0;i<10;i++){
      if(strlen(tmp[i][1])==0)
	break;
      trorf->name=tmp[i][1];
      trorf->content=NULL;
      trorf->next=NULL;
      for(j=2;j<29;j+=2){
	if(strlen(tmp[i][j+1])==0)
	  break;
	if(trorf->content==NULL)
	  trorf->content=createMap(tmp[i][j],tmp[i][j+1]);
	else
	  addToMap(trorf->content,tmp[i][j],tmp[i][j+1]);
      }
      trorf=trorf->next;
    }
    m=&trorf;
  }

#ifdef WIN32
  extern char *url_encode(char *);

  static char* getMapsAsKVP(maps* m,int length,int type){
    char *dataInputsKVP=(char*) malloc(length*sizeof(char));
    char *dataInputsKVPi=NULL;
    maps* curs=m;
    int i=0;
    while(curs!=NULL){
      map *inRequest=getMap(curs->content,"inRequest");
      map *hasLength=getMap(curs->content,"length");
      if((inRequest!=NULL && strncasecmp(inRequest->value,"true",4)==0) ||
	 inRequest==NULL){
	if(i==0)
	  if(type==0){
	    sprintf(dataInputsKVP,"%s=",curs->name);
	    if(hasLength!=NULL){
	      dataInputsKVPi=(char*)malloc((strlen(curs->name)+2)*sizeof(char));
	      sprintf(dataInputsKVPi,"%s=",curs->name);
	    }
	  }
	  else
	    sprintf(dataInputsKVP,"%s",curs->name);
	else{
	  char *temp=zStrdup(dataInputsKVP);
	  if(type==0)
	    sprintf(dataInputsKVP,"%s;%s=",temp,curs->name);
	  else
	    sprintf(dataInputsKVP,"%s;%s",temp,curs->name);
	}
	map* icurs=curs->content;
	if(type==0){
	  char *temp=zStrdup(dataInputsKVP);
	  if(getMap(curs->content,"xlink:href")!=NULL)
	    sprintf(dataInputsKVP,"%sReference",temp);
	  else{
	    if(hasLength!=NULL){
	      int j;
	      for(j=0;j<atoi(hasLength->value);j++){
		map* tmp0=getMapArray(curs->content,"value",j);
		if(j==0)
		  free(temp);
		temp=zStrdup(dataInputsKVP);
		if(j==0)
		  sprintf(dataInputsKVP,"%s%s",temp,tmp0->value);
		else
		  sprintf(dataInputsKVP,"%s;%s%s",temp,dataInputsKVPi,tmp0->value);
	      }
	    }
	    else
	      sprintf(dataInputsKVP,"%s%s",temp,icurs->value);
	  }
	  free(temp);
	}
	while(icurs!=NULL){
	  if(strncasecmp(icurs->name,"value",5)!=0 &&
	     strncasecmp(icurs->name,"mimeType_",9)!=0 &&
	     strncasecmp(icurs->name,"dataType_",9)!=0 &&
	     strncasecmp(icurs->name,"size",4)!=0 &&
	     strncasecmp(icurs->name,"length",4)!=0 &&
	     strncasecmp(icurs->name,"isArray",7)!=0 &&
	     strcasecmp(icurs->name,"Reference")!=0 &&
	     strcasecmp(icurs->name,"minOccurs")!=0 &&
	     strcasecmp(icurs->name,"maxOccurs")!=0 &&
	     strncasecmp(icurs->name,"fmimeType",9)!=0 &&
	     strcasecmp(icurs->name,"inRequest")!=0){
	    char *itemp=zStrdup(dataInputsKVP);
	    if(strcasecmp(icurs->name,"xlink:href")!=0)
	      sprintf(dataInputsKVP,"%s@%s=%s",itemp,icurs->name,icurs->value);
	    else
	      sprintf(dataInputsKVP,"%s@%s=%s",itemp,icurs->name,url_encode(icurs->value));
	    free(itemp);
	  }
	  icurs=icurs->next;
	}
      }
      curs=curs->next;
      i++;
    }
    return dataInputsKVP;
  }
#endif

#ifdef __cplusplus
}
#endif

#endif
