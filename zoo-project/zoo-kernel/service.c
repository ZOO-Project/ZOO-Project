/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2015-2019 GeoLabs SARL
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

#include "service.h"

// knut: time utilities required for new log function (logMessage)
#include <ctime>
#include <chrono>
#ifdef WIN32
#include <process.h>
#endif

#if defined(_MSC_VER) && _MSC_VER < 1800
#include <stdarg.h>
/**
 * snprintf for Visual Studio compiler.
 *
 * See https://dxr.mozilla.org/mozilla-central/source/media/mtransport/third_party/nrappkit/src/util/util.c
 */
int snprintf(char *buffer, size_t n, const char *format, ...)
{
  va_list argp;
  int ret;
  va_start(argp, format);
  ret = _vscprintf(format, argp);
  vsnprintf_s(buffer, n, _TRUNCATE, format, argp);
  va_end(argp);
  return ret;
}
#endif

/**
 * Dump a map on stderr
 *
 * @param t the map to dump
 */
void _dumpMap(map* t){
  if(t!=NULL){
    fprintf(stderr,"%s: %s\n",t->name,t->value);
    fflush(stderr);
  }else{
    fprintf(stderr,"NULL\n");
    fflush(stderr);
  }
}

/**
 * Dump a map on stderr, see _dumpMap()
 *
 * @param t the map to dump
 */
void dumpMap(map* t){
  map* tmp=t;
  while(tmp!=NULL){
    _dumpMap(tmp);
    tmp=tmp->next;
  }
}

/**
 * Dump a map to a file 
 *
 * @param t the map to dump to file
 * @param file the file pointer to store the map
 */
void dumpMapToFile(map* t,FILE* file){
  map* tmp=t;
  while(tmp!=NULL){
    fprintf(file,"%s = %s\n",tmp->name,tmp->value);
    tmp=tmp->next;
  }
}

/**
 * Dump a maps on stderr, see dumpMap().
 *
 * @param m the map to dump
 */
void dumpMaps(maps* m){
  maps* tmp=m;
  while(tmp!=NULL){
    fprintf(stderr,"MAP => [%s] \n",tmp->name);
    fprintf(stderr," * CONTENT [%s] \n",tmp->name);
    dumpMap(tmp->content);
    if(tmp->child!=NULL){
      fprintf(stderr," * CHILD [%s] \n",tmp->name);
      dumpMaps(tmp->child);
      fprintf(stderr," * /CHILD [%s] \n",tmp->name);
    }
    tmp=tmp->next;
  }
}

/**
 * Dump a maps to a file, see dumpMapToFile().
 *
 * @param m the map to dump
 * @param file the the file pointer to store the map
 */
void _dumpMapsToFile(maps* m,FILE* file,int limit){
  maps* tmp=m;
  int cnt=0;
  while(tmp!=NULL){
    fprintf(file,"[%s]\n",tmp->name);
    if(tmp->child!=NULL){
      _dumpMapsToFile(tmp->child,file,limit);
    }else
      dumpMapToFile(tmp->content,file);
    fflush(file);
    tmp=tmp->next;
    if(limit>=0 && cnt==limit)
      tmp=NULL;
    cnt++;
  }
  fflush(file);
}

/**
 * Dump a maps to a file, see _dumpMapsToFile().
 *
 * @param m the map to dump
 * @param file_path the full path to the file name to store the map
 * @param limit the number limiting the maps to be dumped
 */
void dumpMapsToFile(maps* m,char* file_path,int limit){
  FILE* file=fopen(file_path,"w+");
  _dumpMapsToFile(m,file,limit);
  fflush(file);
  fclose(file);
}

/**
 * Create a new iotype*
 *
 * @return a pointer to the allocated iotype 
 */
iotype* createIoType(){
  iotype* io=(iotype*)malloc(IOTYPE_SIZE);
  io->content=NULL;
  io->next=NULL;
  return io;
}

/**
 * Create a new map
 *
 * @param name the key to add to the map
 * @param value the corresponding value to add to the map
 * @return a pointer to the allocated map 
 */
map* createMap(const char* name,const char* value){
  map* tmp=(map *)malloc(MAP_SIZE);
  tmp->name=zStrdup(name);
  tmp->value=zStrdup(value);
  tmp->next=NULL;
  return tmp;
}

/**
 * Create a new maps with the given name
 *
 * @param name of the maps
 * @return the allocated map
 */
maps* createMaps(const char* name){
  maps* tmp = (maps *) malloc (MAPS_SIZE);
  tmp->name = zStrdup (name);
  tmp->content = NULL;
  tmp->child = NULL;
  tmp->next = NULL;
  return tmp;
}

/**
 * Count number of map in a map
 *
 * @param m the map to count
 * @return number of map in a map
 */
int count(map* m){
  map* tmp=m;
  int c=0;
  while(tmp!=NULL){
    c++;
    tmp=tmp->next;
  }
  return c;
}

/**
 * Count number of maps in a maps
 *
 * @param m the maps to count
 * @return number of maps in a maps
 */
int maps_length(maps* m){
  maps* tmp=m;
  int c=0;
  while(tmp!=NULL){
    c++;
    tmp=tmp->next;
  }
  return c;
}

/**
 * Verify if a key exist in a map
 *
 * @param m the map to search for the key
 * @param key the key to search in the map
 * @return true if the key wwas found, false in other case
 */
bool hasKey(map* m,const char *key){
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

/**
 * Access a specific maps
 *
 * @param m the maps to search for the key
 * @param key the key to search in the maps
 * @return a pointer on the maps found or NULL if not found
 */
maps* getMaps(maps* m,const char *key){
  maps* tmp=m;
  while(tmp!=NULL){
    if(strcasecmp(tmp->name,key)==0){
      return tmp;
    }
    tmp=tmp->next;
  }
  return NULL;
}

/**
 * Access a specific map
 *
 * @param m the map to search for the key
 * @param key the key to search in the map
 * @return a pointer on the map found or NULL if not found
 */
map* getMap(map* m,const char *key){
  map* tmp=m;
  while(tmp!=NULL){
    if(strcasecmp(tmp->name,key)==0){
      return tmp;
    }
    tmp=tmp->next;
  }
  return NULL;
}


/**
 * Access the last map
 *
 * @param m the map to search for the lastest map
 * @return a pointer on the lastest map found or NULL if not found
 */
map* getLastMap(map* m){
  map* tmp=m;
  while(tmp!=NULL){
    if(tmp->next==NULL){
      return tmp;
    }
    tmp=tmp->next;
  }
  return NULL;
}

/**
 * Access a specific map from a maps
 *
 * @param m the maps to search for the key
 * @param key the key to search in the maps
 * @param subkey the key to search in the map (found for the key, if any)
 * @return a pointer on the map found or NULL if not found
 */
map* getMapFromMaps(maps* m,const char* key,const char* subkey){
  maps* _tmpm=getMaps(m,key);
  if(_tmpm!=NULL){
    map* _ztmpm=getMap(_tmpm->content,subkey);
    return _ztmpm;
  }
  else return NULL;
}

/**
 * Free allocated memory of a map.
 * Require to call free on mo after calling this function.
 *
 * @param mo the map to free
 */
void freeMap(map** mo){
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

/**
 * Free allocated memory of a maps.
 * Require to call free on mo after calling this function.
 *
 * @param mo the maps to free
 */
void freeMaps(maps** mo){
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
    if(_cursor->child!=NULL){
      freeMaps(&_cursor->child);
      free(_cursor->child);
    }
    if(_cursor->next!=NULL){
      freeMaps(&_cursor->next);
      free(_cursor->next);
    }
  }
}

/**
 * Verify if an elements contains a name equal to the given key.
 *
 * @param e the elements to search for the key
 * @param key the elements name to search
 * @return true if the elements contains the name, false in other cases.
 */ 
bool hasElement(elements* e,const char* key){
  elements* tmp=e;
  while(tmp!=NULL){
    if(strcasecmp(key,tmp->name)==0)
      return true;
    tmp=tmp->next;
  }
  return false;
}

/**
 * Access a specific elements named key.
 *
 * @param m the elements to search
 * @param key the elements name to search
 * @return a pointer to the specific element if found, NULL in other case.
 */ 
elements* getElements(elements* m,char *key){
  elements* tmp=m;
  while(tmp!=NULL){
    if(strcasecmp(tmp->name,key)==0)
      return tmp;
    tmp=tmp->next;
  }
  return NULL;
}

/**
 * Free allocated memory of an iotype.
 * Require to call free on i after calling this function.
 *
 * @param i the iotype to free
 */
void freeIOType(iotype** i){
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

/**
 * Free allocated memory of an elements.
 * Require to call free on e after calling this function.
 *
 * @param e the iotype to free
 */
void freeElements(elements** e){
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
    freeMap(&tmp->additional_parameters);
    if(tmp->additional_parameters!=NULL)
      free(tmp->additional_parameters);
    if(tmp->format!=NULL)
      free(tmp->format);
    freeElements(&tmp->child);
    if(tmp->child!=NULL){
      free(tmp->child);
    }
    if(tmp->defaults!=NULL){
      freeIOType(&tmp->defaults);
      free(tmp->defaults);
    }
    if(tmp->supported!=NULL){
      freeIOType(&tmp->supported);
      free(tmp->supported);
    }
    if(tmp->next!=NULL){
      freeElements(&tmp->next);
      free(tmp->next);
    }
  }
}


/**
 * Allocate memory for a service.
 * Require to call free after calling this function.
 *
 * @return the service
 */
service* createService(){
  service *s1 = (service *) malloc (SERVICE_SIZE);
  s1->name=NULL;
  s1->content=NULL;
  s1->metadata=NULL;
  s1->additional_parameters=NULL;
  s1->inputs=NULL;
  s1->outputs=NULL;
  return s1;
}

/**
 * Free allocated memory of a service.
 * Require to be invoked for every createService call.
 *
 * @param s the service to free
 */
void freeService(service** s){
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
    freeMap(&tmp->additional_parameters);
    if(tmp->additional_parameters!=NULL)
      free(tmp->additional_parameters);
    freeElements(&tmp->inputs);
    if(tmp->inputs!=NULL)
      free(tmp->inputs);
    freeElements(&tmp->outputs);
    if(tmp->outputs!=NULL)
      free(tmp->outputs);
  }
}

/**
 * Add key value pair to an existing map.
 *
 * @param m the map to add the KVP
 * @param n the key to add
 * @param v the corresponding value to add
 */
void addToMap(map* m,const char* n,const char* v){
    if (m != NULL) { // knut: add NULL-pointer check
        if (hasKey(m, n) == false) {
            map* _cursor = m;
            while (_cursor->next != NULL) {
                _cursor = _cursor->next;
            }
            _cursor->next = createMap(n, v);
        }
        else {
            map *tmp = getMap(m, n);
            if (tmp->value != NULL)
                free(tmp->value);
            tmp->value = zStrdup(v);
        }
    }
}

/**
 * Add a key and an integer value to an existing map.
 *
 * @param m the map to add the KVP
 * @param n the key to add
 * @param v the corresponding value to add
 */
void addIntToMap(map* m,const char* n,const int v){
  char svalue[10];
  sprintf(svalue,"%d",v);
  if(hasKey(m,n)==false){
    map* _cursor=m;
    while(_cursor->next!=NULL){
      _cursor=_cursor->next;
    }
    _cursor->next=createMap(n,svalue);
  }
  else{
    map *tmp=getMap(m,n);
    if(tmp->value!=NULL)
      free(tmp->value);
    tmp->value=zStrdup(svalue);
  }
}

/**
 * Add a key and a binary value to an existing map.
 *
 * @param m the map to add the KVP
 * @param n the key to add
 * @param v the corresponding value to add
 * @param size the size of the given value
 * @return a pointer to the updated map m
 */
map* addToMapWithSize(map* m,const char* n,const char* v,int size){
  char sin[128];
  char sname[10]="size";
  map *tmp;
  if(hasKey(m,n)==false){
    map* _cursor=m;
    if(_cursor!=NULL){
      addToMap(m,n,"");
    }else{
      m=createMap(n,"");
    }
  }
  if(strlen(n)>5)
    sprintf(sname,"size_%s",n+6);
  tmp=getMap(m,n);
  if(tmp->value!=NULL)
    free(tmp->value);
  tmp->value=(char*)malloc((size+1)*sizeof(char));
  if(v!=NULL)
    memmove(tmp->value,v,size*sizeof(char));
  tmp->value[size]=0;
  sprintf(sin,"%d",size);
  addToMap(m,sname,sin);
  return m;
}

/**
 * Add a map at the end of another map.
 *
 * @param mo the map to add mi
 * @param mi the map to add to mo
 */
void addMapToMap(map** mo,map* mi){
  map* tmp=mi;
  map* _cursor=*mo;
  while(tmp!=NULL){
    if(_cursor==NULL){
      *mo=createMap(tmp->name,tmp->value);
      (*mo)->next=NULL;
    }
    else{
      map* tmp1=getMap(*mo,tmp->name);
      if(tmp1==NULL){
	while(_cursor->next!=NULL)
	  _cursor=_cursor->next;
	_cursor->next=createMap(tmp->name,tmp->value);
      }
      else{
	addToMap(*mo,tmp->name,tmp->value);
      }
    }
    _cursor=*mo;
    tmp=tmp->next;
  }
}

/**
 * Add a map to iotype.
 *
 * @param io the iotype to add the map
 * @param mi the map to add to io
 */
void addMapToIoType(iotype** io,map* mi){
  iotype* tmp=*io;
  while(tmp->next!=NULL){
    tmp=tmp->next;
  }
  tmp->next=(iotype*)malloc(IOTYPE_SIZE);
  tmp->next->content=NULL;
  addMapToMap(&tmp->next->content,mi);
  tmp->next->next=NULL;
}

/**
 * Access a specific map or set its value.
 *
 * @param m the map to search for the key
 * @param key the key to search/add in the map
 * @param value the value to add if the key does not exist
 * @return a pointer on the map found or NULL if not found
 */
map* getMapOrFill(map** m,const char *key,const char* value){
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

/**
 * Verify if a map is contained in another map.
 *
 * @param m the map to search for i
 * @param i the map to search in m
 * @return true if i was found in m, false in other case
 */
bool contains(map* m,map* i){
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

/**
 * Access a specific iotype from an elements.
 *
 * @param e the elements to search for the name
 * @param name the name to search in the elements e
 * @param values the map to verify it was contained in the defaults or 
 *  supported content of the elements e
 * @return a pointer on the iotype found or NULL if not found
 */
iotype* getIoTypeFromElement(elements* e,char *name, map* values){
  elements* cursor=e;
  if(values!=NULL){
    while(cursor!=NULL){
      if(strcasecmp(cursor->name,name)==0 && (cursor->defaults!=NULL || cursor->supported!=NULL)){
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
  }else{
    while(cursor!=NULL){
      if(strcasecmp(cursor->name,name)==0 && cursor->defaults!=NULL){
	return cursor->defaults;
      }
      cursor=cursor->next;
    }
  }
  return NULL;
}

/**
 * Load binary values from a map (in) and add them to another map (out)
 *
 * @param out the map to add binaries values
 * @param in the map containing the binary values to add ti out
 * @param pos index of the binary in an array (in case of "MapArray")
 */
void loadMapBinary(map** out,map* in,int pos){
  map* size=getMap(in,"size");
  map *lout=*out;
  map *tmpVin,*tmpVout;
  if(size!=NULL && pos>0){
    char tmp[11];
    sprintf(tmp,"size_%d",pos);
    size=getMap(in,tmp);
    sprintf(tmp,"value_%d",pos);
    tmpVin=getMap(in,tmp);
    tmpVout=getMap(lout,tmp);
    free(tmpVout->value);
    tmpVout->value=(char*)malloc((atoi(size->value)+1)*sizeof(char));
    memmove(tmpVout->value,tmpVin->value,atoi(size->value)*sizeof(char));
    tmpVout->value[atoi(size->value)]=0;
  }else{
    if(size!=NULL){
      tmpVin=getMap(in,"value");
      tmpVout=getMap(lout,"value");
      free(tmpVout->value);
      tmpVout->value=(char*)malloc((atoi(size->value)+1)*sizeof(char));
      memmove(tmpVout->value,tmpVin->value,atoi(size->value)*sizeof(char));
      tmpVout->value[atoi(size->value)]=0;
    }
  }
}
  
/**
 * Load binary values from a map (in) and add them to another map (out).
 * This function will take care of MapArray.
 * @see loadMapBinary
 *
 * @param out the map to add binaries values
 * @param in the map containing the binary values to add ti out
 */
void loadMapBinaries(map** out,map* in){
  map* size=getMap(in,"size");
  map* length=getMap(in,"length");
  map* toload=getMap(in,"to_load");
  if(toload!=NULL && strcasecmp(toload->value,"false")==0){
#ifdef DEBUG
    fprintf(stderr,"NO LOAD %s %d \n",__FILE__,__LINE__);
#endif
    return ;
  }
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
}

/**
 * Duplicate a Maps
 * 
 * @param mo the maps to clone
 * @return the allocated maps containing a copy of the mo maps
 */
maps* dupMaps(maps** mo){
  maps* _cursor=*mo;
  maps* res=NULL;
  if(_cursor!=NULL){
    map* mc=_cursor->content;
    maps* mcs=_cursor->child;
    res=createMaps(_cursor->name);
    if(mc!=NULL){
      addMapToMap(&res->content,mc);
      loadMapBinaries(&res->content,mc);
    }
    if(mcs!=NULL){
      res->child=dupMaps(&mcs);
    }
    res->next=dupMaps(&_cursor->next);
  }
  return res;
}

/**
 * Add a maps at the end of another maps.
 *
 * @see addMapToMap, dupMaps, getMaps
 * @param mo the maps to add mi
 * @param mi the maps to add to mo
 */
void addMapsToMaps(maps** mo,maps* mi){
  maps* tmp=mi;
  maps* _cursor=*mo;
  while(tmp!=NULL){
    if(_cursor==NULL){
      *mo=dupMaps(&mi);
    }
    else{
      maps* tmp1=getMaps(*mo,tmp->name);
      while(_cursor->next!=NULL)
	_cursor=_cursor->next;
      if(tmp1==NULL){
	_cursor->next=dupMaps(&tmp);
	if(tmp->child!=NULL)
	  _cursor->next->child=dupMaps(&tmp->child);
	else
	  _cursor->next->child=NULL;
      }
      else{
	addMapToMap(&tmp1->content,tmp->content);
	if(tmp->child!=NULL)
	  tmp1->child=dupMaps(&tmp->child);
	else
	  tmp1->child=NULL;
      }
      _cursor=*mo;
    }
    tmp=tmp->next;
  }
}

/**
 * Access a specific map array element
 *
 * @param m the map to search for the key
 * @param key the key to search in the map
 * @param index of the MapArray 
 * @return a pointer on the map found or NULL if not found
 */
map* getMapArray(map* m,const char* key,int index){
  char tmp[1024];
  map* tmpMap;
  if(index>0)
    sprintf(tmp,"%s_%d",key,index);
  else
    sprintf(tmp,"%s",key);
#ifdef DEBUG
  fprintf(stderr,"** KEY %s\n",tmp);
#endif
  tmpMap=getMap(m,tmp);
#ifdef DEBUG
  if(tmpMap!=NULL)
    dumpMap(tmpMap);
#endif
  return tmpMap;
}

/**
 * Add a key value in a MapArray for a specific index
 *
 * @param m the map to search for the key
 * @param key the key to search in the map
 * @param index the index of the MapArray 
 * @param value the value to set in the MapArray 
 * @return a pointer on the map found or NULL if not found
 */
void setMapArray(map* m,const char* key,int index,const char* value){
  char tmp[1024];
  map* tmpSize;
  if(index>0){
    map* len=getMap(m,"length");
    sprintf(tmp,"%s_%d",key,index);
    if((len!=NULL && atoi(len->value)<index+1) || len==NULL){
      char tmp0[5];
      sprintf(tmp0,"%d",index+1);
      addToMap(m,"length",tmp0);
    }
  }
  else{
    sprintf(tmp,"%s",key);
    addToMap(m,"length","1");
  }
  tmpSize=getMapArray(m,"size",index);
  if(tmpSize!=NULL && strncasecmp(key,"value",5)==0){
    map* ptr=getMapOrFill(&m,tmp,(char *)"");
#ifdef DEBUG
    fprintf(stderr,"%s\n",tmpSize->value);
#endif
    free(ptr->value);
    ptr->value=(char*)malloc((atoi(tmpSize->value)+1)*sizeof(char));
    memcpy(ptr->value,value,atoi(tmpSize->value)); 
  }
  else
    addToMap(m,tmp,value);
}

/**
 * Add a key and an integer value to an existing map array.
 *
 * @param m the map to add the KVP
 * @param n the key to add
 * @param index the index of the MapArray 
 * @param v the corresponding value to add
 */
void addIntToMapArray(map* m,const char* n,int index,const int v){
  char svalue[10];
  sprintf(svalue,"%d",v);
  setMapArray(m,n,index,svalue);
}

/**
 * Access the map "type"
 *
 * @param mt the map
 * @return a pointer on the map for mimeType/dataType/CRS if found, NULL in
 *  other case
 */
map* getMapType(map* mt){
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

/**
 * Add a Maps containing a MapArray to a Maps
 *
 * @see getMapType
 * @param mo the maps
 * @param mi the maps
 * @param typ the map "type"
 * @return 
 */
int addMapsArrayToMaps(maps** mo,maps* mi,char* typ){
  maps* tmp=mi;    
  maps* _cursor=getMaps(*mo,tmp->name);
  char tmpLen[10];
  int len=1;
  char *tmpV[14]={
    (char*)"size",
    (char*)"value",
    (char*)"uom",
    (char*)"Reference",
    (char*)"Order",
    (char*)"cache_file",
    (char*)"fmimeType",
    (char*)"xlink:href",
    typ,
    (char*)"schema",
    (char*)"encoding",
    (char*)"isCached",
    (char*)"LowerCorner",
    (char*)"UpperCorner"
  };
  int i=0;
  map* tmpLength;
  
  if(_cursor==NULL)
    return -1;

  tmpLength=getMap(_cursor->content,"length");
  if(tmpLength!=NULL){
    len=atoi(tmpLength->value);
  }

  sprintf(tmpLen,"%d",len+1);
  addToMap(_cursor->content,"length",tmpLen);
  for(i=0;i<14;i++){
    map* tmpVI=getMap(tmp->content,tmpV[i]);
    if(tmpVI!=NULL){
#ifdef DEBUG
      fprintf(stderr,"%s = %s\n",tmpV[i],tmpVI->value);
#endif
      setMapArray(_cursor->content,tmpV[i],len,tmpVI->value);
    }
  }
    
  addToMap(_cursor->content,"isArray","true");
  return 0;
}

/**
 * Set a key value pair to a map contained in a Maps
 *
 * @param m the maps
 * @param key the maps name
 * @param subkey the map name included in the maps corresponding to key
 * @param value the corresponding value to add in the map
 */
void setMapInMaps(maps* m,const char* key,const char* subkey,const char *value){
  maps* _tmpm=getMaps(m,key);
  if(_tmpm!=NULL){
    map* _ztmpm=getMap(_tmpm->content,subkey);
    if(_ztmpm!=NULL){
      if(_ztmpm->value!=NULL)
	free(_ztmpm->value);
      _ztmpm->value=zStrdup(value);
    }else{
      maps *tmp=createMaps(key);
      tmp->content=createMap(subkey,value);
      addMapsToMaps(&_tmpm,tmp);
      freeMaps(&tmp);
      free(tmp);
    }
  }else{
    maps *tmp=createMaps(key);
    tmp->content=createMap(subkey,value);
    addMapsToMaps(&m,tmp);
    freeMaps(&tmp);
    free(tmp);
  }
}

/**
 * Create an empty elements
 *
 * @return a pointer to the allocated elements
 */
elements* createEmptyElements(){
  elements* res=(elements*)malloc(ELEMENTS_SIZE);
  res->name=NULL;
  res->content=NULL;
  res->metadata=NULL;
  res->additional_parameters=NULL;  
  res->format=NULL;
  res->defaults=NULL;
  res->supported=NULL;
  res->child=NULL;
  res->next=NULL;
  return res;
}

/**
 * Create a named elements
 *
 * @param name the elements name
 * @return a pointer to the allocated elements
 */
elements* createElements(const char* name){
  elements* res=(elements*)malloc(ELEMENTS_SIZE);
  res->name=zStrdup(name);
  res->content=NULL;
  res->metadata=NULL;
  res->additional_parameters=NULL;
  res->format=NULL;
  res->defaults=NULL;
  res->supported=NULL;
  res->child=NULL;
  res->next=NULL;
  return res;
}

/**
 * Set the name of an elements
 *
 * @param name the elements name
 * @return a pointer to the allocated elements
 */
void setElementsName(elements** elem,char* name){
  elements* res=*elem;
  res->name=zStrdup(name);
  res->content=NULL;
  res->metadata=NULL;
  res->format=NULL;
  res->defaults=NULL;
  res->supported=NULL;
  res->child=NULL;
  res->next=NULL;
}

/**
 * Dump an elements on stderr
 *
 * @param e the elements to dump
 */
void dumpElements(elements* e){
  elements* tmp=e;
  while(tmp!=NULL){
    iotype* tmpio=tmp->defaults;
    int ioc=0;
    fprintf(stderr,"ELEMENT [%s]\n",tmp->name);
    fprintf(stderr," > CONTENT [%s]\n",tmp->name);
    dumpMap(tmp->content);
    fprintf(stderr," > METADATA [%s]\n",tmp->name);
    dumpMap(tmp->metadata);
    fprintf(stderr," > ADDITIONAL PARAMETERS [%s]\n",tmp->name);
    dumpMap(tmp->additional_parameters);
    fprintf(stderr," > FORMAT [%s]\n",tmp->format);
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
    if(tmp->child!=NULL){
      fprintf(stderr," > CHILD \n");
      dumpElements(tmp->child);
    }
    fprintf(stderr,"------------------\n");
    tmp=tmp->next;
  }
}

/**
 * Dump an elements on stderr using the YAML syntaxe
 *
 * @param e the elements to dump
 */
void dumpElementsAsYAML(elements* e,int level){
  elements* tmp=e;
  int i;
  while(tmp!=NULL){
    map* mcurs=tmp->content;
    int ioc=0;
    iotype* tmpio;
    for(i=0;i<2+(4*level);i++)
      fprintf(stderr," ");
    fprintf(stderr,"%s:\n",tmp->name);
    while(mcurs!=NULL){
      for(i=0;i<4+(4*level);i++)
	fprintf(stderr," ");
      _dumpMap(mcurs);
      mcurs=mcurs->next;
    }
    mcurs=tmp->metadata;
    if(mcurs!=NULL){
      for(i=0;i<4+(4*level);i++)
	fprintf(stderr," ");
      fprintf(stderr,"MetaData:\n");
      while(mcurs!=NULL){
	for(i=0;i<6+(4*level);i++)
	  fprintf(stderr," ");
	_dumpMap(mcurs);
	mcurs=mcurs->next;
      }
    }
    for(i=0;i<4+(4*level);i++)
      fprintf(stderr," ");
    if(tmp->format!=NULL)
      fprintf(stderr,"%s:\n",tmp->format);
    else{
      fprintf(stderr,"Child:\n");
      if(tmp->child!=NULL)
	dumpElementsAsYAML(tmp->child,level+1);
    }
    tmpio=tmp->defaults;
    while(tmpio!=NULL){
      for(i=0;i<6+(4*level);i++)
	fprintf(stderr," ");
      fprintf(stderr,"default:\n");
      mcurs=tmpio->content;
      while(mcurs!=NULL){
	for(i=0;i<8+(4*level);i++)
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
      for(i=0;i<6+(4*level);i++)
	fprintf(stderr," ");
      fprintf(stderr,"supported:\n");
      mcurs=tmpio->content;
      while(mcurs!=NULL){
	for(i=0;i<8+(4*level);i++)
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

/**
 * Duplicate an elements
 * 
 * @param e the elements to clone
 * @return the allocated elements containing a copy of the elements e
 */
elements* dupElements(elements* e){
  elements* cursor=e;
  elements* tmp=NULL;
  if(cursor!=NULL && cursor->name!=NULL){
#ifdef DEBUG
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
    dumpElements(e);
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
#endif
    tmp=(elements*)malloc(ELEMENTS_SIZE);
    tmp->name=zStrdup(cursor->name);
    tmp->content=NULL;
    addMapToMap(&tmp->content,cursor->content);
    tmp->metadata=NULL;
    addMapToMap(&tmp->metadata,cursor->metadata);
    tmp->additional_parameters=NULL;
    addMapToMap(&tmp->additional_parameters,cursor->additional_parameters);
    if(cursor->format!=NULL)
      tmp->format=zStrdup(cursor->format);
    else
      tmp->format=NULL;
    if(cursor->defaults!=NULL){
      tmp->defaults=(iotype*)malloc(IOTYPE_SIZE);
      tmp->defaults->content=NULL;
      addMapToMap(&tmp->defaults->content,cursor->defaults->content);
      tmp->defaults->next=NULL;
#ifdef DEBUG
      fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
      dumpMap(tmp->defaults->content);
#endif
    }else
      tmp->defaults=NULL;
    if(cursor->supported!=NULL && cursor->supported->content!=NULL){
      iotype *tmp2=cursor->supported->next;
      tmp->supported=(iotype*)malloc(IOTYPE_SIZE);
      tmp->supported->content=NULL;
      addMapToMap(&tmp->supported->content,cursor->supported->content);
      tmp->supported->next=NULL;
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
    if(cursor->child!=NULL)
      tmp->child=dupElements(cursor->child);
    else
      tmp->child=NULL;
    if(cursor->next!=NULL)
      tmp->next=dupElements(cursor->next);
    else
      tmp->next=NULL;
  }
  return tmp;
}

/**
 * Add an elements to another elements.
 *
 * @see dupElements
 * @param m the elements to add the e
 * @param e the elements to be added to m
 */
void addToElements(elements** m,elements* e){
  elements* tmp=e;
  if(*m==NULL){
    (*m)=dupElements(tmp);
  }else{
    addToElements(&(*m)->next,tmp);
  }
}

/**
 * Set the name of a service
 *
 * @param name the service name
 */
void setServiceName(service** serv,char* name){
  service* res=*serv;
  res->name=zStrdup(name);
  res->content=NULL;
  res->metadata=NULL;
  res->inputs=NULL;
  res->outputs=NULL;
}

/**
 * Dump a service on stderr
 *
 * @param s the service to dump
 */
void dumpService(service* s){
  if(s==NULL)
    return;
  fprintf(stderr,"++++++++++++++++++\nSERVICE [%s]\n++++++++++++++++++\n",s->name);
  if(s->content!=NULL){
    fprintf(stderr,"CONTENT MAP\n");
    dumpMap(s->content);
    if(s->metadata!=NULL)
      fprintf(stderr,"CONTENT METADATA\n");
    dumpMap(s->metadata);
    if(s->additional_parameters!=NULL)
      fprintf(stderr,"CONTENT AdditionalParameters\n");
    dumpMap(s->additional_parameters);
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

/**
 * Dump a service on stderr using the YAML syntaxe
 *
 * @param s the service to dump
 */
void dumpServiceAsYAML(service* s){
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
    dumpElementsAsYAML(s->inputs,0);
  }
  if(s->outputs!=NULL){
    fprintf(stderr,"\noutputs:\n");
    dumpElementsAsYAML(s->outputs,0);
  }
}

/**
 * Duplicate a service
 * 
 * @param s the service to clone
 * @return the allocated service containing a copy of the serfvice s
 */
service* dupService(service* s){
  service *res=(service*)malloc(SERVICE_SIZE);
  res->name=zStrdup(s->name);
  res->content=NULL;
  addMapToMap(&res->content,s->content);
  res->metadata=NULL;
  addMapToMap(&res->metadata,s->metadata);
  res->additional_parameters=NULL;
  addMapToMap(&res->additional_parameters,s->additional_parameters);
  res->inputs=dupElements(s->inputs);
  res->outputs=dupElements(s->outputs);
  return res;
}

/**
 * Print the registry on stderr.
 * 
 * @param r the registry
 */
void dumpRegistry(registry* r){
  registry* p=r;
  while(p!=NULL){
    services* s=p->content;
    fprintf(stderr,"%s \n",p->name);
    s=p->content;
    while(s!=NULL){
      dumpService(s->content);
      s=s->next;
    }
    p=p->next;
  }
}

/**
 * Add a service to the registry
 *
 * @param reg the resgitry to add the service
 * @param name the registry name to update
 * @param content the service to add
 */
bool addServiceToRegistry(registry** reg,char* name,service* content){
  registry *l=*reg;
  int isInitial=-1;
  if(l==NULL){
    l=(registry*)malloc(REGISTRY_SIZE);
    isInitial=1;
  }
  if(l!=NULL){
    int hasLevel=-1;
    while(isInitial<0 && l!=NULL){
      if(l->name!=NULL && strcasecmp(name,l->name)==0){
	hasLevel=1;
	break;
      }
      l=l->next;
    }
    if(hasLevel<0){
      if(isInitial<0)
	l=(registry*)malloc(REGISTRY_SIZE);
      l->name=zStrdup(name);
      l->content=NULL;
      l->next=NULL;
    }
    if(l->content==NULL){
      l->content=(services*)malloc(SERVICES_SIZE);
      l->content->content=dupService(content);
      l->content->next=NULL;
    }
    else{
      services* s=l->content;
      while(s->next!=NULL)
	s=s->next;
      s->next=(services*)malloc(SERVICES_SIZE);
      s->next->content=dupService(content);
      s->next->next=NULL;
    }
    l->next=NULL;
    if(isInitial>0)
      *reg=l;
    else{
      registry *r=*reg;
      while(r->next!=NULL)
	r=r->next;
      r->next=l;
      r->next->next=NULL;
    }
    return true;
  }
  else
    return false;
}

/**
 * Free memory allocated for the registry
 *
 * @param r the registry
 */
void freeRegistry(registry** r){
  registry* lr=*r;
  while(lr!=NULL){
    services* s=lr->content;
    free(lr->name);
    while(s!=NULL){
      service* s1=s->content;
      s=s->next;
      if(s1!=NULL){
	freeService(&s1);
	free(s1);
	s1=NULL;
      }
    }
    lr=lr->next;
  }    
}

/**
 * Access a service in the registry
 *
 * @param r the registry
 * @param level the regitry to search ("concept", "generic" or "implementation")
 * @param sname the service name
 * @return the service pointer if a corresponding service was found or NULL
 */
service* getServiceFromRegistry(registry* r,char  *level,char* sname){
  registry *lr=r;
  while(lr!=NULL){
    if(strcasecmp(lr->name,level)==0){
      services* s=lr->content;
      while(s!=NULL){
	if(s->content!=NULL && strcasecmp(s->content->name,sname)==0)
	  return s->content;
	s=s->next;
      }
      break;
    }
    lr=lr->next;
  }
  return NULL;
}

/**
 * Apply inheritance to an out map from a reference in map
 *
 * @param out the map to update
 * @param in the reference map (containing inherited properties)
 */
void inheritMap(map** out,map* in){
  map* content=in;
  if((*out)==NULL){
    addMapToMap(out,in);
    return;
  }
  while(content!=NULL){
    map* cmap=getMap(*out,content->name);
    if(cmap==NULL)
      addToMap(*out,content->name,content->value);
    content=content->next;
  }
}

/**
 * Apply inheritance to an out iotype from a reference in iotype
 * 
 * @param out the iotype to update
 * @param in the reference iotype (containing inherited properties)
 */
void inheritIOType(iotype** out,iotype* in){
  iotype* io=in;
  iotype* oio=*out;
  if(io!=NULL){
    if(*out==NULL){
      *out=(iotype*)malloc(IOTYPE_SIZE);
      (*out)->content=NULL;
      addMapToMap(&(*out)->content,io->content);
      (*out)->next=NULL;
      oio=*out;
      inheritIOType(&oio->next,io->next);
    }else{
      inheritIOType(&oio->next,io->next);
    }
  }
}

/**
 * Apply inheritance to an out elements from a reference in elements
 * 
 * @param out the elements to update
 * @param in the reference elements (containing inherited properties)
 */
void inheritElements(elements** out,elements* in){
  elements* content=in;
  while(content!=NULL && *out!=NULL){
    elements* cmap=getElements(*out,content->name);
    if(cmap==NULL)
      addToElements(out,content);
    else{
      inheritMap(&cmap->content,content->content);
      inheritMap(&cmap->metadata,content->metadata);
      if(cmap->format==NULL && content->format!=NULL)
	cmap->format=zStrdup(content->format);
      inheritIOType(&cmap->defaults,content->defaults);
      if(cmap->supported==NULL)
	inheritIOType(&cmap->supported,content->supported);
      else{
	iotype* p=content->supported;
	while(p!=NULL){
	  addMapToIoType(&cmap->supported,p->content);
	  p=p->next;
	}
      }
    }
    content=content->next;
  }
}

/**
 * Apply inheritance to a service based on a registry
 * 
 * @param r the registry storing profiles hierarchy
 * @param s the service to update depending on its inheritance
 */
void inheritance(registry *r,service** s){
  service* ls=*s;
  map *profile,*level;
  if(r==NULL)
    return;
  if(ls==NULL || ls->content==NULL)
    return;
  profile=getMap(ls->content,"extend");
  level=getMap(ls->content,"level");
  if(profile!=NULL&&level!=NULL){
    service* s1;
    if(strncasecmp(level->value,"profile",7)==0)
      s1=getServiceFromRegistry(r,(char*)"generic",profile->value);
    else
      s1=getServiceFromRegistry(r,level->value,profile->value);
      
    inheritMap(&ls->content,s1->content);
    inheritMap(&ls->metadata,s1->metadata);
    if(ls->inputs==NULL && s1->inputs!=NULL){
      ls->inputs=dupElements(s1->inputs);
    }else{
      inheritElements(&ls->inputs,s1->inputs);
    }
    if(ls->outputs==NULL && s1->outputs!=NULL){
      ls->outputs=dupElements(s1->outputs);
    }else
      inheritElements(&ls->outputs,s1->outputs);
  }
}

/**
 * Convert a maps to a char*** (only used for Fortran support)
 *
 * @param m the maps to convert
 * @param c the resulting array
 */
void mapsToCharXXX(maps* m,char*** c){
  maps* tm=m;
  int i=0;
  int j=0;
  char tmp[10][30][1024];
  memset(tmp,0,1024*10*10);
  while(tm!=NULL){
    map* tc=tm->content;
    if(i>=10)
      break;
    strcpy(tmp[i][j],"name");
    j++;
    strcpy(tmp[i][j],tm->name);
    j++;
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

/**
 * Convert a char*** to a maps (only used for Fortran support)
 *
 * @param c the array to convert
 * @param m the resulting maps
 */
void charxxxToMaps(char*** c,maps**m){
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

/**
 * Verify that a map has a value
 *
 * @param map pointer to map that should be checked
 * @return true if map has a value or false if value is missing/empty/NULL
 */
bool nonempty(map* map) {
	return (map != NULL && map->value != NULL && strlen(map->value) > 0 && strcmp(map->value, "NULL") != 0);
}

/**
 * Verify that a particular map value exists in a maps
 * data structure, and obtain that value
 *
 * @param source pointer to maps structure
 * @param node name of maps node to search
 * @param key name of map node to find
 * @param kvp address to the map* if it exists, otherwise NULL
 * @return true if map has a value or false if value is missing/NULL
 *
 * @note The map assigned to kvp is owned by the source maps
 */
bool hasvalue(maps* source, const char* node, const char* key, map** kvp) {
	*kvp = getMapFromMaps(source, node, key);
	return (*kvp != NULL && (*kvp)->value != NULL &&
		strlen((*kvp)->value) > 0 && strcmp((*kvp)->value, "NULL") != 0);
}

/*
 * Set error message in configuration maps
 *
 * @param conf reference to configuration maps
 * @param service name of service
 * @param exc WPSException code
 * @param message exception text (default: exception text in WPS specification)
 */
void setErrorMessage(maps*& conf, const char* service, WPSException exc, const char* message) {

	if (message == NULL) {
		message = WPSExceptionText[exc];
	}

	size_t len = strlen(service) + strlen(": ") + strlen(message) + strlen(": ") + strlen(WPSExceptionCode[exc]) + 16;
	char* msg = (char*)malloc(len * sizeof(char));

	if (msg != NULL) {
		snprintf(msg, len * sizeof(char), "\n%s: %s: %s\n", service, message, WPSExceptionCode[exc]);
		setMapInMaps(conf, "lenv", "message", msg);
		free(msg);
	}
}

void logMessage(const char* source, const char* function, int line, const char* file, const char* message) { //, const char* source, const char* function, int line) {

	size_t msglen = 512;
	const char empty[] = "";

	FILE* log;

	// system time, process time [nanoseconds]   
	unsigned long long sys_t, proc_t;

	// processor time consumed by the program:
	clock_t t = clock();

	// system time:
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	std::time_t now_t = std::chrono::system_clock::to_time_t(now);
	std::tm* tm = localtime(&now_t);
	char* str = asctime(tm);
	str[strlen(str) - 1] = '\0'; // remove newline

	sys_t = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
	//proc_t = (unsigned long long)(1.0e9*t/CLOCKS_PER_SEC);
	proc_t = t;

	if (message != NULL) {
		msglen += strlen(message);
	}
	else {
		message = empty;
	}
	//getLastErrorMessage(); // cgiScriptName  
	char* text = (char*)malloc(sizeof(char)*msglen);

	snprintf(text, msglen, "pid: %d %s line %d %s() %s systime: %lld ns ticks: %lld %s\n",
		zGetpid(), source, line, function, str, sys_t, proc_t, message); // __FILE__ __LINE__ __func__ //

	if (file != NULL && (log = fopen(file, "a+")) != NULL) {
		fputs(text, log);
		fclose(log);
	}
	else {
#ifdef MSG_LOG_FILE
		if ((log = fopen(MSG_LOG_FILE, "a+")) != NULL) {
			fputs(text, log);
			fclose(log);
		}
#endif
	}

	if (text != NULL) free(text);
}

// knut:
// Example:
// zooLog;
// zooLogMsg(NULL, getLastErrorMessage()); 
// zooLogMsg("log.txt", getLastErrorMessage()); 
