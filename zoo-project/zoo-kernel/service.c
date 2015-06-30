/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2015 GeoLabs SARL
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

#ifdef _MSC_VER
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
 * @param file the file to store the map
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
    dumpMap(tmp->content);
    tmp=tmp->next;
  }
}

/**
 * Dump a maps to a file, see dumpMapToFile().
 *
 * @param m the map to dump
 * @param file_path the full path to the file name to store the map
 */
void dumpMapsToFile(maps* m,char* file_path,int limit){
  FILE* file=fopen(file_path,"w+");
  maps* tmp=m;
  int cnt=0;
  while(tmp!=NULL){
    fprintf(file,"[%s]\n",tmp->name);
    dumpMapToFile(tmp->content,file);
    fflush(file);
    tmp=tmp->next;
    cnt++;
    if(limit>=0 && cnt==limit)
      tmp=NULL;
  }
  fflush(file);
  fclose(file);
}

/**
 * Create a new map
 *
 * @param name the key to add to the map
 * @param value the corresponding value to add to the map
 * @return the allocated map
 */
map* createMap(const char* name,const char* value){
  map* tmp=(map *)malloc(MAP_SIZE);
  tmp->name=zStrdup(name);
  tmp->value=zStrdup(value);
  tmp->next=NULL;
  return tmp;
}

/**
 * Count number of map in a map
 *
 * @param m the maps to count
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

/**
 * Free allocated memory of a service.
 * Require to call free on e after calling this function.
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
 */
void addToMapWithSize(map* m,const char* n,const char* v,int size){
  if(hasKey(m,n)==false){
    map* _cursor=m;
    if(_cursor!=NULL){
      addToMap(m,n,"");
    }else{
      m=createMap(n,"");
    }
  }
  char sname[10]="size";
  if(strlen(n)>5)
    sprintf(sname,"size_%s",n+6);
  map *tmp=getMap(m,n);
  if(tmp->value!=NULL)
    free(tmp->value);
  tmp->value=(char*)malloc((size+1)*sizeof(char));
  memmove(tmp->value,v,size*sizeof(char));
  tmp->value[size]=0;
  char sin[128];
  sprintf(sin,"%d",size);
  addToMap(m,sname,sin);
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
  if(index>0){
    sprintf(tmp,"%s_%d",key,index);
    map* len=getMap(m,"length");
    if((len!=NULL && atoi(len->value)<index+1) || len==NULL){
      char tmp0[5];
      sprintf(tmp0,"%d",index+1);
      addToMap(m,"length",tmp0);
    }
  }
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

  if(_cursor==NULL)
    return -1;

  map* tmpLength=getMap(_cursor->content,"length");
  char tmpLen[10];
  int len=1;
  if(tmpLength!=NULL){
    len=atoi(tmpLength->value);
  }

  char *tmpV[12]={
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
    (char*)"isCached"
  };
  sprintf(tmpLen,"%d",len+1);
  addToMap(_cursor->content,"length",tmpLen);
  int i=0;
  for(i=0;i<12;i++){
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

/**
 * Dump an elements on stderr
 *
 * @param e the elements to dump
 */
void dumpElements(elements* e){
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

/**
 * Dump an elements on stderr using the YAML syntaxe
 *
 * @param e the elements to dump
 */
void dumpElementsAsYAML(elements* e){
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

/**
 * Duplicate an elements
 * 
 * @param e the elements to clone
 * @return the allocated elements containing a copy of the elements e
 */
elements* dupElements(elements* e){
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
    if(e->format!=NULL)
      tmp->format=zStrdup(e->format);
    else
      tmp->format=NULL;
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
    *m=dupElements(tmp);
  }else{
    addToElements(&(*m)->next,tmp);
  }
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
    dumpElementsAsYAML(s->inputs);
  }
  if(s->outputs!=NULL){
    fprintf(stderr,"\noutputs:\n");
    dumpElementsAsYAML(s->outputs);
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
    fprintf(stderr,"%s \n",p->name);
    services* s=p->content;
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
  if(r==NULL)
    return;
  service* ls=*s;
  if(ls->content==NULL)
    return;
  map* profile=getMap(ls->content,"extend");
  map* level=getMap(ls->content,"level");
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
