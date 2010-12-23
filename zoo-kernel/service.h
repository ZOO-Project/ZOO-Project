/**
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2010 GeoLabs SARL
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define bool int
#define true 1
#define false -1

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

  /**
   * \struct maps
   * \brief linked list of map pointer
   *
   * Small object to store WPS KVP set.
   */
  typedef struct maps{
    char* name;          
    struct map* content; 
    struct maps* next;   
  } maps;

  /**
   * \struct map
   * \brief KVP linked list
   *
   * Deal with WPS KVP (name,value).
   */
  typedef struct map{
    char* name;       /* The key */
    char* value;      /* The value */
    struct map* next; /* Next couple */
  } map;

#ifdef WIN32
#define NULLMAP ((map*) 0)
#else
#define NULLMAP NULL
#endif

  static void _dumpMap(map* t){
    if(t!=NULL){
      fprintf(stderr,"[%s] => [%s] \n",t->name,t->value);
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

  static void dumpMaps(maps* m){
    maps* tmp=m;
    while(tmp!=NULL){
      fprintf(stderr,"MAP => [%s] \n",tmp->name);
      dumpMap(tmp->content);
      tmp=tmp->next;
    }
  }

  static map* createMap(const char* name,const char* value){
    map* tmp=(map *)malloc(MAP_SIZE);
    tmp->name=strdup(name);
    tmp->value=strdup(value);
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
      if(strlen(tmp->name)==strlen(key) && strncasecmp(tmp->name,key,strlen(key))==0)
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
      if(strlen(tmp->name)==strlen(key) && strncasecmp(tmp->name,key,strlen(key))==0){
	return tmp;
      }
      tmp=tmp->next;
    }
    return NULL;
  }

  static map* getMap(map* m,const char *key){
    map* tmp=m;
    while(tmp!=NULL){
      if(strlen(tmp->name)==strlen(key) && strncasecmp(tmp->name,key,strlen(key))==0){
	return tmp;
      }
      tmp=tmp->next;
    }
    return NULL;
  }

  static map* getMapFromMaps(maps* m,char* key,char* subkey){
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
    fflush(stderr);
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

  typedef struct iotype{
    struct map* content;
    struct iotype* next;
  } iotype;

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

  static bool hasElement(elements* e,char* key){
    elements* tmp=e;
    while(tmp!=NULL){
      if(strlen(tmp->name)==strlen(key) && strncasecmp(key,tmp->name,strlen(key))==0)
	return true;
      tmp=tmp->next;
    }
    return false;
  }

  static elements* getElements(elements* m,char *key){
    elements* tmp=m;
    while(tmp!=NULL){
      if(strlen(tmp->name)==strlen(key) && strncasecmp(tmp->name,key,strlen(tmp->name))==0)
	return tmp;
      tmp=tmp->next;
    }
    return NULL;
  }


  static void freeIOType(iotype** i){
    iotype* _cursor=*i;
    if(_cursor!=NULL){
      freeMap(&_cursor->content);
      free(_cursor->content);
      if(_cursor->next!=NULL){
	freeIOType(&_cursor->next);
	free(_cursor->next);
      }
    }
  }

  static void freeElements(elements** e){
    elements* tmp=*e;
    if(tmp!=NULL){
      free(tmp->name);
      freeMap(&tmp->content);
      free(tmp->content);
      freeMap(&tmp->metadata);
      free(tmp->metadata);
      free(tmp->format);
      freeIOType(&tmp->defaults);
      if(tmp->defaults!=NULL)
	free(tmp->defaults);
      freeIOType(&tmp->supported);
      if(tmp->supported!=NULL)
	free(tmp->supported);
      freeElements(&tmp->next);
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
      while(_cursor->next!=NULL)
	_cursor=_cursor->next;
      _cursor->next=createMap(n,v);
    }
    else{
      map *tmp=getMap(m,n);
      if(tmp->value!=NULL)
      	free(tmp->value);
      tmp->value=strdup(v);
    }
  }

  static void addMapToMap(map** mo,map* mi){
    map* tmp=mi;
    map* _cursor=*mo;
    if(tmp==NULL){
      if(_cursor!=NULL){
	while(_cursor!=NULL)
	  _cursor=_cursor->next;
	_cursor=NULL;
      }else
	*mo=NULL;
    }
    while(tmp!=NULL){
      if(_cursor==NULL){
	if(*mo==NULL)
	  *mo=createMap(tmp->name,tmp->value);
	else
	  addToMap(*mo,tmp->name,tmp->value);
      }
      else{
#ifdef DEBUG
	fprintf(stderr,"_CURSOR\n");
	dumpMap(_cursor);
#endif
	while(_cursor!=NULL)
	  _cursor=_cursor->next;
	_cursor=createMap(tmp->name,tmp->value);
	_cursor->next=NULL;
      }
      tmp=tmp->next;
#ifdef DEBUG
      fprintf(stderr,"MO\n");
      dumpMap(*mo);
#endif
    }
  }

  static void addMapToIoType(iotype** io,map* mi){
    iotype* tmp=*io;
    while(tmp!=NULL){
#ifdef DEBUG
      fprintf(stderr,">> CURRENT MAP");
      dumpMap(tmp->content);
#endif
      tmp=tmp->next;
    }
#ifdef DEBUG
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    tmp=(iotype*)malloc(IOTYPE_SIZE);
#ifdef DEBUG
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    tmp->content=NULL;
#ifdef DEBUG
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    addMapToMap(&tmp->content,mi);
#ifdef DEBUG
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    dumpMap(tmp->content);
    tmp->next=NULL;
  }


  static maps* dupMaps(maps** mo){
    maps* _cursor=*mo;
    maps* res=NULL;
    if(_cursor!=NULL){
      res=(maps*)malloc(MAPS_SIZE);
      res->name=strdup(_cursor->name);
      res->content=NULL;
      res->next=NULL;
      map* mc=_cursor->content;
      if(mc!=NULL){
	addMapToMap(&res->content,mc);
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
	(*mo)->next=NULL;
      }
      else{
	while(_cursor->next!=NULL)
	  _cursor=_cursor->next;
	_cursor->next=dupMaps(&tmp);
      }
      tmp=tmp->next;
    }
  }


  static void* setMapInMaps(maps* m,char* key,char* subkey,char *value){
    maps* _tmpm=getMaps(m,key);
    if(_tmpm!=NULL){
      map* _ztmpm=getMap(_tmpm->content,subkey);
      if(_ztmpm!=NULL){
	free(_ztmpm->value);
	_ztmpm->value=strdup(value);
	dumpMap(_ztmpm);
      }else{
	addToMap(_tmpm->content,subkey,value);
      }
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
      tmp->name=strdup(e->name);
      tmp->content=NULL;
      addMapToMap(&tmp->content,e->content);
      tmp->metadata=NULL;
      addMapToMap(&tmp->metadata,e->metadata);
      tmp->format=strdup(e->format);
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
	tmp->supported->next=NULL;
	addMapToMap(&tmp->supported->content,e->supported->content);
	iotype *etmp=*(&tmp->supported->next) ;
	iotype *tmp2=e->supported->next;
	while(tmp2!=NULL){
	  etmp=(iotype*)malloc(IOTYPE_SIZE);
	  etmp->content=NULL;
	  addMapToMap(&etmp->content,tmp2->content);
	  etmp->next=NULL;
#ifdef DEBUG
	  fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
	  dumpMap(tmp->defaults->content);
#endif
	  tmp2=tmp2->next;
	  etmp=etmp->next;
	}
      }
      else
	tmp->supported=NULL;
      tmp->next=dupElements(cursor->next);
    }
    return tmp;
  }

  static void addToElements(elements** m,elements* e){
    elements* _cursor=*m;
    elements* tmp=e;
    if(_cursor==NULL){
      *m=dupElements(tmp);
    }else{
      while(_cursor->next!=NULL)
	_cursor=_cursor->next;
      _cursor->next=dupElements(tmp);
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

#ifdef __cplusplus
}
#endif

#endif
