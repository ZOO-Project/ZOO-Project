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
int snprintf(char *pcBuffer, size_t n, const char *pccFormat, ...)
{
  va_list argp;
  int ret;
  va_start(argp, pccFormat);
  ret = _vscprintf(pccFormat, argp);
  vsnprintf_s(pcBuffer, n, _TRUNCATE, pccFormat, argp);
  va_end(argp);
  return ret;
}
#endif

/**
 * Dump a map on stderr
 *
 * @param pmMap the map to dump
 */
void _dumpMap(map* pmMap){
  if(pmMap!=NULL){
    fprintf(stderr,"%s: %s\n",pmMap->name,pmMap->value);
    fflush(stderr);
  }else{
    fprintf(stderr,"NULL\n");
    fflush(stderr);
  }
}

/**
 * Dump a map on stderr, see _dumpMap()
 *
 * @param pmMap the map to dump
 */
void dumpMap(map* pmMap){
  map* pmTmp=pmMap;
  while(pmTmp!=NULL){
    _dumpMap(pmTmp);
    pmTmp=pmTmp->next;
  }
}

/**
 * Dump a map to a file 
 *
 * @param pmMap the map to dump to file
 * @param pfFile the file pointer to store the map
 */
void dumpMapToFile(map* pmMap,FILE* pfFile){
  map* pmTmp=pmMap;
  while(pmTmp!=NULL){
    fprintf(pfFile,"%s = %s\n",pmTmp->name,pmTmp->value);
    pmTmp=pmTmp->next;
  }
}

/**
 * Dump a maps on stderr, see dumpMap().
 *
 * @param pmMap the map to dump
 */
void dumpMaps(maps* pmMap){
  maps* pmTmp=pmMap;
  while(pmTmp!=NULL){
    fprintf(stderr,"MAP => [%s] \n",pmTmp->name);
    fprintf(stderr," * CONTENT [%s] \n",pmTmp->name);
    dumpMap(pmTmp->content);
    if(pmTmp->child!=NULL){
      fprintf(stderr," * CHILD [%s] \n",pmTmp->name);
      dumpMaps(pmTmp->child);
      fprintf(stderr," * /CHILD [%s] \n",pmTmp->name);
    }
    pmTmp=pmTmp->next;
  }
}

/**
 * Dump a maps to a file, see dumpMapToFile().
 *
 * @param pmsMaps the map to dump
 * @param psFile the the file pointer to store the map
 * @param iLimit the number of maps to print (0 for no limit)
 */
void _dumpMapsToFile(maps* pmsMaps,FILE* psFile,int iLimit){
  maps* tmp=pmsMaps;
  int cnt=0;
  while(tmp!=NULL){
    fprintf(psFile,"[%s]\n",tmp->name);
    if(tmp->child!=NULL){
      _dumpMapsToFile(tmp->child,psFile,iLimit);
    }else
      dumpMapToFile(tmp->content,psFile);
    fflush(psFile);
    tmp=tmp->next;
    cnt++;
    if(iLimit>=0 && cnt==iLimit)
      tmp=NULL;
  }
  fflush(psFile);
}

/**
 * Dump a maps to a file, see _dumpMapsToFile().
 *
 * @param pmsMaps the map to dump
 * @param pcaFilePath the the file pointer to store the map
 * @param iLimit the number of maps to print (0 for no limit)
 */
void dumpMapsToFile(maps* pmsMaps,char* pcaFilePath,int iLimit){
  FILE* psFile=fopen(pcaFilePath,"w+");
  _dumpMapsToFile(pmsMaps,psFile,iLimit);
  fflush(psFile);
  fclose(psFile);
}

/**
 * Create a new iotype*
 *
 * @return a pointer to the allocated iotype 
 */
iotype* createIoType(){
  iotype* pioIO=(iotype*)malloc(IOTYPE_SIZE);
  pioIO->content=NULL;
  pioIO->next=NULL;
  return pioIO;
}

/**
 * Create a new map
 *
 * @param pccName the key to add to the map
 * @param pccValue the corresponding value to add to the map
 * @return a pointer to the allocated map 
 */
map* createMap(const char* pccName,const char* pccValue){
  map* pmTtmp=(map *)malloc(MAP_SIZE);
  pmTtmp->name=zStrdup(pccName);
  pmTtmp->value=zStrdup(pccValue);
  pmTtmp->next=NULL;
  return pmTtmp;
}

/**
 * Create a new maps with the given name
 *
 * @param pccName of the maps
 * @return the allocated map
 */
maps* createMaps(const char* pccName){
  maps* pmTmp = (maps *) malloc (MAPS_SIZE);
  pmTmp->name = zStrdup (pccName);
  pmTmp->content = NULL;
  pmTmp->child = NULL;
  pmTmp->next = NULL;
  return pmTmp;
}

/**
 * Count number of map in a map
 *
 * @param pmMap the map to count
 * @return number of map in a map
 */
int count(map* pmMap){
  map* pmTmp=pmMap;
  int c=0;
  while(pmTmp!=NULL){
    c++;
    pmTmp=pmTmp->next;
  }
  return c;
}

/**
 * Count number of maps in a maps
 *
 * @param pmMap the maps to count
 * @return number of maps in a maps
 */
int maps_length(maps* pmMap){
  maps* pmTmp=pmMap;
  int c=0;
  while(pmTmp!=NULL){
    c++;
    pmTmp=pmTmp->next;
  }
  return c;
}

/**
 * Verify if a key exist in a map
 *
 * @param pmMap the map to search for the key
 * @param pccKey the key to search in the map
 * @return true if the key wwas found, false in other case
 */
bool hasKey(map* pmMap,const char *pccKey){
  map* pmTmp=pmMap;
  while(pmTmp!=NULL){
    if(strcasecmp(pmTmp->name,pccKey)==0)
      return true;
    pmTmp=pmTmp->next;
  }
#ifdef DEBUG_MAP
  fprintf(stderr,"NOT FOUND \n");
#endif
  return false;
}

/**
 * Access a specific maps
 *
 * @param pmMap the maps to search for the key
 * @param pccKey the key to search in the maps
 * @return a pointer on the maps found or NULL if not found
 */
maps* getMaps(maps* pmsMaps,const char *pccKey){
  maps* pmTmp=pmsMaps;
  while(pmTmp!=NULL){
    if(strcasecmp(pmTmp->name,pccKey)==0){
      return pmTmp;
    }
    pmTmp=pmTmp->next;
  }
  return NULL;
}

/**
 * Access a specific map
 *
 * @param pmMap the map to search for the key
 * @param pccKey the key to search in the map
 * @return a pointer on the map found or NULL if not found
 */
map* getMap(map* pmMap,const char *pccKey){
  map* pmTmp=pmMap;
  while(pmTmp!=NULL){
    if(strcasecmp(pmTmp->name,pccKey)==0){
      return pmTmp;
    }
    pmTmp=pmTmp->next;
  }
  return NULL;
}


/**
 * Access the last map
 *
 * @param pmMap the map to search for the lastest map
 * @return a pointer on the lastest map found or NULL if not found
 */
map* getLastMap(map* pmMap){
  map* pmTmp=pmMap;
  while(pmTmp!=NULL){
    if(pmTmp->next==NULL){
      return pmTmp;
    }
    pmTmp=pmTmp->next;
  }
  return NULL;
}

/**
 * Access a specific map from a maps
 *
 * @param pmMap the maps to search for the key
 * @param pccKey the key to search in the maps
 * @param pccSubkey the key to search in the map (found for the key, if any)
 * @return a pointer on the map found or NULL if not found
 */
map* getMapFromMaps(maps* pmMap,const char* pccKey,const char* pccSubkey){
  maps* pmTmp=getMaps(pmMap,pccKey);
  if(pmTmp!=NULL){
    map* pmTmp1=getMap(pmTmp->content,pccSubkey);
    return pmTmp1;
  }
  else return NULL;
}

/**
 * Free allocated memory of a map.
 * Require to call free on mo after calling this function.
 *
 * @param pmMap the map to free
 */
void freeMap(map** pmMap){
  map* pmCursor=*pmMap;
  if(pmCursor!=NULL){
#ifdef DEBUG
    fprintf(stderr,"freeMap\n");
#endif
    if(pmCursor->name!=NULL)
      free(pmCursor->name);
    if(pmCursor->value!=NULL)
      free(pmCursor->value);
    if(pmCursor->next!=NULL){
      freeMap(&pmCursor->next);
      if(pmCursor->next!=NULL)
	free(pmCursor->next);
    }
  }
}

/**
 * Free allocated memory of a maps.
 * Require to call free on mo after calling this function.
 *
 * @param pmMap the maps to free
 */
void freeMaps(maps** pmMap){
  maps* pmCursor=*pmMap;
  if(pmCursor && pmCursor!=NULL){
#ifdef DEBUG
    fprintf(stderr,"freeMaps\n");
#endif
    free(pmCursor->name);
    if(pmCursor->content!=NULL){
      freeMap(&pmCursor->content);
      free(pmCursor->content);
    }
    if(pmCursor->child!=NULL){
      freeMaps(&pmCursor->child);
      free(pmCursor->child);
    }
    if(pmCursor->next!=NULL){
      freeMaps(&pmCursor->next);
      free(pmCursor->next);
    }
  }
}

/**
 * Verify if an elements contains a name equal to the given key.
 *
 * @param peElem the elements to search for the key
 * @param pccKey the elements name to search
 * @return true if the elements contains the name, false in other cases.
 */ 
bool hasElement(elements* peElem,const char* pccKey){
  elements* peTmp=peElem;
  while(peTmp!=NULL){
    if(strcasecmp(pccKey,peTmp->name)==0)
      return true;
    peTmp=peTmp->next;
  }
  return false;
}

/**
 * Access a specific elements named key.
 *
 * @param peElem the elements to search
 * @param pccKey the elements name to search
 * @return a pointer to the specific element if found, NULL in other case.
 */ 
elements* getElements(elements* peElem,const char *pccKey){
  elements* peTmp=peElem;
  while(peTmp!=NULL){
    if(strcasecmp(peTmp->name,pccKey)==0)
      return peTmp;
    peTmp=peTmp->next;
  }
  return NULL;
}

/**
 * Free allocated memory of an iotype.
 * Require to call free on i after calling this function.
 *
 * @param piotIO the iotype to free
 */
void freeIOType(iotype** piotIO){
  iotype* piotCursor=*piotIO;
  if(piotCursor!=NULL){
    if(piotCursor->next!=NULL){
      freeIOType(&piotCursor->next);
      free(piotCursor->next);
    }
    freeMap(&piotCursor->content);
    free(piotCursor->content);
  }
}

/**
 * Free allocated memory of an elements.
 * Require to call free on e after calling this function.
 *
 * @param peElem the iotype to free
 */
void freeElements(elements** peElem){
  elements* peTmp=*peElem;
  if(peTmp!=NULL){
    if(peTmp->name!=NULL)
      free(peTmp->name);
    freeMap(&peTmp->content);
    if(peTmp->content!=NULL)
      free(peTmp->content);
    freeMap(&peTmp->metadata);
    if(peTmp->metadata!=NULL)
      free(peTmp->metadata);
    freeMap(&peTmp->additional_parameters);
    if(peTmp->additional_parameters!=NULL)
      free(peTmp->additional_parameters);
    if(peTmp->format!=NULL)
      free(peTmp->format);
    freeElements(&peTmp->child);
    if(peTmp->child!=NULL){
      free(peTmp->child);
    }
    if(peTmp->defaults!=NULL){
      freeIOType(&peTmp->defaults);
      free(peTmp->defaults);
    }
    if(peTmp->supported!=NULL){
      freeIOType(&peTmp->supported);
      free(peTmp->supported);
    }
    if(peTmp->next!=NULL){
      freeElements(&peTmp->next);
      free(peTmp->next);
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
  service *psService = (service *) malloc (SERVICE_SIZE);
  psService->name=NULL;
  psService->content=NULL;
  psService->metadata=NULL;
  psService->additional_parameters=NULL;
  psService->inputs=NULL;
  psService->outputs=NULL;
  return psService;
}

/**
 * Free allocated memory of a service.
 * Require to be invoked for every createService call.
 *
 * @param psService the service to free
 */
void freeService(service** psService){
  service* psTmp=*psService;
  if(psTmp!=NULL){
    if(psTmp->name!=NULL)
      free(psTmp->name);
    freeMap(&psTmp->content);
    if(psTmp->content!=NULL)
      free(psTmp->content);
    freeMap(&psTmp->metadata);
    if(psTmp->metadata!=NULL)
      free(psTmp->metadata);
    freeMap(&psTmp->additional_parameters);
    if(psTmp->additional_parameters!=NULL)
      free(psTmp->additional_parameters);
    freeElements(&psTmp->inputs);
    if(psTmp->inputs!=NULL)
      free(psTmp->inputs);
    freeElements(&psTmp->outputs);
    if(psTmp->outputs!=NULL)
      free(psTmp->outputs);
  }
}

/**
 * Add key value pair to an existing map.
 *
 * @param pMap the map to add the KVP
 * @param pccName the key to add
 * @param pccValue the corresponding value to add
 */
void addToMap(map* pMap,const char* pccName,const char* pccValue){
  if (pMap != NULL) { // knut: add NULL-pointer check
    if (hasKey(pMap, pccName) == false) {
      map* pmCursor = pMap;
      while (pmCursor->next != NULL) {
	pmCursor = pmCursor->next;
      }
      pmCursor->next = createMap(pccName, pccValue);
    }
    else {
      map *tmp = getMap(pMap, pccName);
      if (tmp->value != NULL)
	free(tmp->value);
      tmp->value = zStrdup(pccValue);
    }
  }
}

/**
 * Add a key and an integer value to an existing map.
 *
 * @param pMap the map to add the KVP
 * @param pccName the key to add
 * @param iValue the corresponding value to add
 */
void addIntToMap(map* pMap,const char* pccName,const int iValue){
  char acValue[10];
  sprintf(acValue,"%d",iValue);
  if(hasKey(pMap,pccName)==false){
    map* pmCursor=pMap;
    while(pmCursor->next!=NULL){
      pmCursor=pmCursor->next;
    }
    pmCursor->next=createMap(pccName,acValue);
  }
  else{
    map *pmTmp=getMap(pMap,pccName);
    if(pmTmp->value!=NULL)
      free(pmTmp->value);
    pmTmp->value=zStrdup(acValue);
  }
}

/**
 * Add a key and a binary value to an existing map.
 *
 * @param pMap the map to add the KVP
 * @param pccName the key to add
 * @param pccValue the corresponding value to add
 * @param iSize the size of the given value
 * @return a pointer to the updated map m
 */
map* addToMapWithSize(map* pMap,const char* pccName,const char* pccValue,int iSize){
  char acIn[128];
  char acName[10]="size";
  map *pmTmp;
  if(hasKey(pMap,pccName)==false){
    map* _cursor=pMap;
    if(_cursor!=NULL){
      addToMap(pMap,pccName,"");
    }else{
      pMap=createMap(pccName,"");
    }
  }
  if(strlen(pccName)>5)
    sprintf(acName,"size_%s",pccName+6);
  pmTmp=getMap(pMap,pccName);
  if(pmTmp->value!=NULL)
    free(pmTmp->value);
  pmTmp->value=(char*)malloc((iSize+1)*sizeof(char));
  if(pccValue!=NULL)
    memmove(pmTmp->value,pccValue,iSize*sizeof(char));
  pmTmp->value[iSize]=0;
  sprintf(acIn,"%d",iSize);
  addToMap(pMap,acName,acIn);
  return pMap;
}

/**
 * Add a map at the end of another map.
 *
 * @param pmMapOut the map to add pmMapIn to
 * @param pmMapIn the map to add to pmMapOut
 */
void addMapToMap(map** pmMapOut,map* pmMapIn){
  map* pmTmp=pmMapIn;
  map* pmCursor=*pmMapOut;
  while(pmTmp!=NULL){
    if(pmCursor==NULL){
      *pmMapOut=createMap(pmTmp->name,pmTmp->value);
      (*pmMapOut)->next=NULL;
    }
    else{
      map* pmTmp1=getMap(*pmMapOut,pmTmp->name);
      if(pmTmp1==NULL){
	while(pmCursor->next!=NULL)
	  pmCursor=pmCursor->next;
	pmCursor->next=createMap(pmTmp->name,pmTmp->value);
      }
      else{
	addToMap(*pmMapOut,pmTmp->name,pmTmp->value);
      }
    }
    pmCursor=*pmMapOut;
    pmTmp=pmTmp->next;
  }
}

/**
 * Add a map to iotype.
 *
 * @param piotType the iotype to add the map
 * @param pmMap the map to add to io
 */
void addMapToIoType(iotype** piotType,map* pmMap){
  iotype* piotTmp=*piotType;
  while(piotTmp->next!=NULL){
    piotTmp=piotTmp->next;
  }
  piotTmp->next=(iotype*)malloc(IOTYPE_SIZE);
  piotTmp->next->content=NULL;
  addMapToMap(&piotTmp->next->content,pmMap);
  piotTmp->next->next=NULL;
}

/**
 * Access a specific map or set its value.
 *
 * @param ppmMap the map to search for the key
 * @param pccKey the key to search/add in the map
 * @param pccValue the value to add if the key does not exist
 * @return a pointer on the map found or NULL if not found
 */
map* getMapOrFill(map** ppmMap,const char *pccKey,const char* pccValue){
  map* pmTmp=*ppmMap;
  map* pmTmp1=getMap(pmTmp,pccKey);
  if(pmTmp1==NULL){
    if(pmTmp!=NULL){
      addToMap((*ppmMap),pccKey,pccValue);
    }
    else
      (*ppmMap)=createMap(pccKey,pccValue);
    pmTmp1=getMap(*ppmMap,pccKey);
  }
  return pmTmp1;
}

/**
 * Verify if a map is contained in another map.
 *
 * @param pmMap the map to search for i
 * @param pmSearch the map to search in m
 * @return true if i was found in m, false in other case
 */
bool contains(map* pmMap,map* pmSearch){
  while(pmSearch!=NULL){      
    if(strcasecmp(pmSearch->name,"value")!=0 &&
       strcasecmp(pmSearch->name,"xlink:href")!=0 &&
       strcasecmp(pmSearch->name,"useMapServer")!=0 &&
       strcasecmp(pmSearch->name,"asReference")!=0){
      map *tmp;
      if(hasKey(pmMap,pmSearch->name) &&
	 (tmp=getMap(pmMap,pmSearch->name))!=NULL && 
	 strcasecmp(pmSearch->value,tmp->value)!=0)
	return false;
    }
    pmSearch=pmSearch->next;
  }
  return true;
}

/**
 * Access a specific iotype from an elements.
 *
 * @param peElem the elements to search for the name
 * @param pcName the name to search in the elements e
 * @param pcValues the map to verify it was contained in the defaults or 
 *  supported content of the elements e
 * @return a pointer on the iotype found or NULL if not found
 */
iotype* getIoTypeFromElement(elements* peElem,char *pcName, map* pcValues){
  elements* peCursor=peElem;
  if(pcValues!=NULL){
    while(peCursor!=NULL){
      if(strcasecmp(peCursor->name,pcName)==0 && (peCursor->defaults!=NULL || peCursor->supported!=NULL)){
	if(contains(peCursor->defaults->content,pcValues)==true)
	  return peCursor->defaults;
	else{
	  iotype* tmp=peCursor->supported;
	  while(tmp!=NULL){
	    if(contains(tmp->content,pcValues)==true)
	      return tmp;	    
	    tmp=tmp->next;
	  }
	}
      }
      peCursor=peCursor->next;
    }
  }else{
    while(peCursor!=NULL){
      if(strcasecmp(peCursor->name,pcName)==0 && peCursor->defaults!=NULL){
	return peCursor->defaults;
      }
      peCursor=peCursor->next;
    }
  }
  return NULL;
}

/**
 * Load binary values from a map (in) and add them to another map (out)
 *
 * @param pmOut the map to add binaries values
 * @param pmIn the map containing the binary values to add ti out
 * @param iPos index of the binary in an array (in case of "MapArray")
 */
void loadMapBinary(map** ppmOut,map* pmIn,int iPos){
  map* pmSize=getMap(pmIn,"size");
  map *pmOut=*ppmOut;
  map *pmTmpVin,*pmTmpVout;
  if(pmSize!=NULL && iPos>0){
    char tmp[11];
    sprintf(tmp,"size_%d",iPos);
    pmSize=getMap(pmIn,tmp);
    sprintf(tmp,"value_%d",iPos);
    pmTmpVin=getMap(pmIn,tmp);
    pmTmpVout=getMap(pmOut,tmp);
    free(pmTmpVout->value);
    pmTmpVout->value=(char*)malloc((atoi(pmSize->value)+1)*sizeof(char));
    memmove(pmTmpVout->value,pmTmpVin->value,atoi(pmSize->value)*sizeof(char));
    pmTmpVout->value[atoi(pmSize->value)]=0;
  }else{
    if(pmSize!=NULL){
      pmTmpVin=getMap(pmIn,"value");
      pmTmpVout=getMap(pmOut,"value");
      free(pmTmpVout->value);
      pmTmpVout->value=(char*)malloc((atoi(pmSize->value)+1)*sizeof(char));
      memmove(pmTmpVout->value,pmTmpVin->value,atoi(pmSize->value)*sizeof(char));
      pmTmpVout->value[atoi(pmSize->value)]=0;
    }
  }
}

/**
 * Load binary values from a map (in) and add them to another map (out).
 * This function will take care of MapArray.
 * @see loadMapBinary
 *
 * @param ppmOut the map to add binaries values
 * @param pmIn the map containing the binary values to add ti out
 */
void loadMapBinaries(map** ppmOut,map* pmIn){
  map* pmSize=getMap(pmIn,"size");
  map* pmLength=getMap(pmIn,"length");
  map* pmToload=getMap(pmIn,"to_load");
  if(pmToload!=NULL && strcasecmp(pmToload->value,"false")==0){
#ifdef DEBUG
    fprintf(stderr,"NO LOAD %s %d \n",__FILE__,__LINE__);
#endif
    return ;
  }
  if(pmLength!=NULL){
    int len=atoi(pmLength->value);
    int i=0;
    for(i=0;i<len;i++){
      loadMapBinary(ppmOut,pmIn,i);
    }
  }
  else
    if(pmSize!=NULL)
      loadMapBinary(ppmOut,pmIn,-1);
}

/**
 * Duplicate a Maps
 * 
 * @param ppmsOut the maps to clone
 * @return the allocated maps containing a copy of the mo maps
 */
maps* dupMaps(maps** ppmsOut){
  maps* pmsCursor=*ppmsOut;
  maps* pmRes=NULL;
  if(pmsCursor!=NULL){
    map* pmContent=pmsCursor->content;
    maps* pmsChild=pmsCursor->child;
    pmRes=createMaps(pmsCursor->name);
    if(pmContent!=NULL){
      addMapToMap(&pmRes->content,pmContent);
      loadMapBinaries(&pmRes->content,pmContent);
    }
    if(pmsChild!=NULL){
      pmRes->child=dupMaps(&pmsChild);
    }
    pmRes->next=dupMaps(&pmsCursor->next);
  }
  return pmRes;
}

/**
 * Add a maps at the end of another maps.
 *
 * @see addMapToMap, dupMaps, getMaps
 * @param ppmsOut the maps to add mi
 * @param pmIn the maps to add to mo
 */
void addMapsToMaps(maps** ppmsOut,maps* pmIn){
  maps* pmsTmp=pmIn;
  maps* pmsCursor=*ppmsOut;
  while(pmsTmp!=NULL){
    if(pmsCursor==NULL){
      *ppmsOut=dupMaps(&pmIn);
    }
    else{
      maps* pmsTmp1=getMaps(*ppmsOut,pmsTmp->name);
      if(pmsTmp1==NULL){
	while(pmsCursor->next!=NULL)
	  pmsCursor=pmsCursor->next;
	pmsCursor->next=dupMaps(&pmsTmp);
	if(pmsTmp->child!=NULL)
	  pmsCursor->next->child=dupMaps(&pmsTmp->child);
	else
	  pmsCursor->next->child=NULL;
	return;
      }
      else{
	addMapToMap(&pmsTmp1->content,pmsTmp->content);
	if(pmsTmp->child!=NULL)
	  pmsTmp1->child=dupMaps(&pmsTmp->child);
	else
	  pmsTmp1->child=NULL;
      }
      pmsCursor=*ppmsOut;
    }
    pmsTmp=pmsTmp->next;
  }
}

/**
 * Access a specific map array element
 *
 * @param pmMap the map to search for the key
 * @param pccKey the key to search in the map
 * @param iIndex of the MapArray 
 * @return a pointer on the map found or NULL if not found
 */
map* getMapArray(map* pmMap,const char* pccKey,int iIndex){
  char acTmp[1024];
  map* pmTmp;
  if(iIndex>0)
    sprintf(acTmp,"%s_%d",pccKey,iIndex);
  else
    sprintf(acTmp,"%s",pccKey);
#ifdef DEBUG
  fprintf(stderr,"** KEY %s\n",acTmp);
#endif
  pmTmp=getMap(pmMap,acTmp);
#ifdef DEBUG
  if(pmTmp!=NULL)
    dumpMap(pmTmp);
#endif
  return pmTmp;
}

/**
 * Add a key value in a MapArray for a specific index
 *
 * @param pmMap the map to search for the key
 * @param pccKey the key to search in the map
 * @param iIndex the index of the MapArray 
 * @param pccValue the value to set in the MapArray 
 * @return a pointer on the map found or NULL if not found
 */
void setMapArray(map* pmMap,const char* pccKey,int iIndex,const char* pccValue){
  char acTmp[1024];
  map* pmSize;
  if(iIndex>0){
    map* pmLen=getMap(pmMap,"length");
    sprintf(acTmp,"%s_%d",pccKey,iIndex);
    if((pmLen!=NULL && atoi(pmLen->value)<iIndex+1) || pmLen==NULL){
      char acTmp0[5];
      sprintf(acTmp0,"%d",iIndex+1);
      addToMap(pmMap,"length",acTmp0);
    }
  }
  else{
    sprintf(acTmp,"%s",pccKey);
    addToMap(pmMap,"length","1");
  }
  pmSize=getMapArray(pmMap,"size",iIndex);
  if(pmSize!=NULL && strncasecmp(pccKey,"value",5)==0){
    map* pmPtr=getMapOrFill(&pmMap,acTmp,(char *)"");
#ifdef DEBUG
    fprintf(stderr,"%s\n",pmSize->value);
#endif
    free(pmPtr->value);
    pmPtr->value=(char*)malloc((atoi(pmSize->value)+1)*sizeof(char));
    memcpy(pmPtr->value,pccValue,atoi(pmSize->value)); 
  }
  else
    addToMap(pmMap,acTmp,pccValue);
}

/**
 * Add a key and an integer value to an existing map array.
 *
 * @param pmMap the map to add the KVP
 * @param pccName the key to add
 * @param iIndex the index of the MapArray 
 * @param icValue the corresponding value to add
 */
void addIntToMapArray(map* pmMap,const char* pccName,int iIndex,const int icValue){
  char acValue[10];
  sprintf(acValue,"%d",icValue);
  setMapArray(pmMap,pccName,iIndex,acValue);
}

/**
 * Access the map "type"
 *
 * @param pmMap the map
 * @return a pointer on the map for mimeType/dataType/CRS if found, NULL in
 *  other case
 */
map* getMapType(map* pmMap){
  map* pmMime=getMap(pmMap,(char *)"mimeType");
  if(pmMime==NULL){
    pmMime=getMap(pmMap,"dataType");
    if(pmMime==NULL){
      pmMime=getMap(pmMap,"CRS");
    }
  }
#ifdef DEBUG
  dumpMap(pmMime);
#endif
  return pmMime;
}

/**
 * Add a Maps containing a MapArray to a Maps
 *
 * @see getMapType
 * @param pmsOut the maps
 * @param pmsIn the maps
 * @param pcType the map "type"
 * @return 
 */
int addMapsArrayToMaps(maps** pmsOut,maps* pmsIn,char* pcType){
  maps* pmsTmp=pmsIn;
  maps* pmsCursor=getMaps(*pmsOut,pmsTmp->name);
  char acLen[10];
  int iLen=1;
  char *acV[14]={
    (char*)"size",
    (char*)"value",
    (char*)"uom",
    (char*)"Reference",
    (char*)"Order",
    (char*)"cache_file",
    (char*)"fmimeType",
    (char*)"xlink:href",
    pcType,
    (char*)"schema",
    (char*)"encoding",
    (char*)"isCached",
    (char*)"LowerCorner",
    (char*)"UpperCorner"
  };
  int iCounter=0;
  map* pmLength;
  
  if(pmsCursor==NULL)
    return -1;

  pmLength=getMap(pmsCursor->content,"length");
  if(pmLength!=NULL){
    iLen=atoi(pmLength->value);
  }

  sprintf(acLen,"%d",iLen+1);
  addToMap(pmsCursor->content,"length",acLen);
  for(iCounter=0;iCounter<14;iCounter++){
    map* pmTmp=getMap(pmsTmp->content,acV[iCounter]);
    if(pmTmp!=NULL){
#ifdef DEBUG
      fprintf(stderr,"%s = %s\n",pmTmp[iCounter],pmTmp->value);
#endif
      setMapArray(pmsCursor->content,acV[iCounter],iLen,pmTmp->value);
    }
  }
    
  addToMap(pmsCursor->content,"isArray","true");
  return 0;
}

/**
 * Set a key value pair to a map contained in a Maps
 *
 * @param pmsMaps the maps
 * @param pccKey the maps name
 * @param pccSubkey the map name included in the maps corresponding to key
 * @param pccValue the corresponding value to add in the map
 */
void setMapInMaps(maps* pmsMaps,const char* pccKey,const char* pccSubkey,const char *pccValue){
  maps* pmsTmp=getMaps(pmsMaps,pccKey);
  if(pmsTmp!=NULL){
    map* pmTmpSub=getMap(pmsTmp->content,pccSubkey);
    if(pmTmpSub!=NULL){
      if(pmTmpSub->value!=NULL)
	free(pmTmpSub->value);
      pmTmpSub->value=zStrdup(pccValue);
    }else{
      maps *pmsToAdd=createMaps(pccKey);
      pmsToAdd->content=createMap(pccSubkey,pccValue);
      addMapsToMaps(&pmsTmp,pmsToAdd);
      freeMaps(&pmsToAdd);
      free(pmsToAdd);
    }
  }else{
    maps *pmsToAdd=createMaps(pccKey);
    pmsToAdd->content=createMap(pccSubkey,pccValue);
    addMapsToMaps(&pmsMaps,pmsToAdd);
    freeMaps(&pmsToAdd);
    free(pmsToAdd);
  }
}

/**
 * Create an empty elements
 *
 * @return a pointer to the allocated elements
 */
elements* createEmptyElements(){
  elements* peRes=(elements*)malloc(ELEMENTS_SIZE);
  peRes->name=NULL;
  peRes->content=NULL;
  peRes->metadata=NULL;
  peRes->additional_parameters=NULL;  
  peRes->format=NULL;
  peRes->defaults=NULL;
  peRes->supported=NULL;
  peRes->child=NULL;
  peRes->next=NULL;
  return peRes;
}

/**
 * Create a named elements
 *
 * @param pcName the elements name
 * @return a pointer to the allocated elements
 */
elements* createElements(const char* pcName){
  elements* peRes=(elements*)malloc(ELEMENTS_SIZE);
  peRes->name=zStrdup(pcName);
  peRes->content=NULL;
  peRes->metadata=NULL;
  peRes->additional_parameters=NULL;
  peRes->format=NULL;
  peRes->defaults=NULL;
  peRes->supported=NULL;
  peRes->child=NULL;
  peRes->next=NULL;
  return peRes;
}

/**
 * Set the name of an elements
 *
 * @param peElem the elements to modify
 * @param pcName the elements name
 * @return a pointer to the allocated elements
 */
void setElementsName(elements** ppeElem,char* pcName){
  elements* peRes=*ppeElem;
  peRes->name=zStrdup(pcName);
  peRes->content=NULL;
  peRes->metadata=NULL;
  peRes->format=NULL;
  peRes->defaults=NULL;
  peRes->supported=NULL;
  peRes->child=NULL;
  peRes->next=NULL;
}

/**
 * Dump an elements on stderr
 *
 * @param peElem the elements to dump
 */
void dumpElements(elements* peElem){
  elements* peTmp=peElem;
  while(peTmp!=NULL){
    iotype* piotTmp=peTmp->defaults;
    int ioc=0;
    fprintf(stderr,"ELEMENT [%s]\n",peTmp->name);
    fprintf(stderr," > CONTENT [%s]\n",peTmp->name);
    dumpMap(peTmp->content);
    fprintf(stderr," > METADATA [%s]\n",peTmp->name);
    dumpMap(peTmp->metadata);
    fprintf(stderr," > ADDITIONAL PARAMETERS [%s]\n",peTmp->name);
    dumpMap(peTmp->additional_parameters);
    fprintf(stderr," > FORMAT [%s]\n",peTmp->format);
    while(piotTmp!=NULL){
      fprintf(stderr," > DEFAULTS [%s] (%i)\n",peTmp->name,ioc);
      dumpMap(piotTmp->content);
      piotTmp=piotTmp->next;
      ioc++;
    }
    piotTmp=peTmp->supported;
    ioc=0;
    while(piotTmp!=NULL){
      fprintf(stderr," > SUPPORTED [%s] (%i)\n",peTmp->name,ioc);
      dumpMap(piotTmp->content);
      piotTmp=piotTmp->next;
      ioc++;
    }
    if(peTmp->child!=NULL){
      fprintf(stderr," > CHILD \n");
      dumpElements(peTmp->child);
    }
    fprintf(stderr,"------------------\n");
    peTmp=peTmp->next;
  }
}

/**
 * Dump an elements on stderr using the YAML syntaxe
 *
 * @param peElem the elements to dump
 * @param iLevel the current level
 */
void dumpElementsAsYAML(elements* peElem,int iLevel){
  elements* peTmp=peElem;
  int i;
  while(peTmp!=NULL){
    map* pmCurs=peTmp->content;
    int ioc=0;
    iotype* piotTmp;
    for(i=0;i<2+(4*iLevel);i++)
      fprintf(stderr," ");
    fprintf(stderr,"%s:\n",peTmp->name);
    while(pmCurs!=NULL){
      for(i=0;i<4+(4*iLevel);i++)
	fprintf(stderr," ");
      _dumpMap(pmCurs);
      pmCurs=pmCurs->next;
    }
    pmCurs=peTmp->metadata;
    if(pmCurs!=NULL){
      for(i=0;i<4+(4*iLevel);i++)
	fprintf(stderr," ");
      fprintf(stderr,"MetaData:\n");
      while(pmCurs!=NULL){
	for(i=0;i<6+(4*iLevel);i++)
	  fprintf(stderr," ");
	_dumpMap(pmCurs);
	pmCurs=pmCurs->next;
      }
    }
    for(i=0;i<4+(4*iLevel);i++)
      fprintf(stderr," ");
    if(peTmp->format!=NULL)
      fprintf(stderr,"%s:\n",peTmp->format);
    else{
      fprintf(stderr,"Child:\n");
      if(peTmp->child!=NULL)
	dumpElementsAsYAML(peTmp->child,iLevel+1);
    }
    piotTmp=peTmp->defaults;
    while(piotTmp!=NULL){
      for(i=0;i<6+(4*iLevel);i++)
	fprintf(stderr," ");
      fprintf(stderr,"default:\n");
      pmCurs=piotTmp->content;
      while(pmCurs!=NULL){
	for(i=0;i<8+(4*iLevel);i++)
	  fprintf(stderr," ");
	if(strcasecmp(pmCurs->name,"range")==0){
	  fprintf(stderr,"range: \"%s\"\n",pmCurs->value);
	}else
	  _dumpMap(pmCurs);
	pmCurs=pmCurs->next;
      }
      piotTmp=piotTmp->next;
      ioc++;
    }
    piotTmp=peTmp->supported;
    ioc=0;
    while(piotTmp!=NULL){
      for(i=0;i<6+(4*iLevel);i++)
	fprintf(stderr," ");
      fprintf(stderr,"supported:\n");
      pmCurs=piotTmp->content;
      while(pmCurs!=NULL){
	for(i=0;i<8+(4*iLevel);i++)
	  fprintf(stderr," ");
	if(strcasecmp(pmCurs->name,"range")==0){
	  fprintf(stderr,"range: \"%s\"\n",pmCurs->value);
	}else
	  _dumpMap(pmCurs);
	pmCurs=pmCurs->next;
      }
      piotTmp=piotTmp->next;
      ioc++;
    }
    peTmp=peTmp->next;
  }
}

/**
 * Duplicate an elements
 * 
 * @param peElem the elements to clone
 * @return the allocated elements containing a copy of the elements e
 */
elements* dupElements(elements* peElem){
  elements* peCursor=peElem;
  elements* peTmp=NULL;
  if(peCursor!=NULL && peCursor->name!=NULL){
#ifdef DEBUG
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
    dumpElements(e);
    fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
#endif
    peTmp=(elements*)malloc(ELEMENTS_SIZE);
    peTmp->name=zStrdup(peCursor->name);
    peTmp->content=NULL;
    addMapToMap(&peTmp->content,peCursor->content);
    peTmp->metadata=NULL;
    addMapToMap(&peTmp->metadata,peCursor->metadata);
    peTmp->additional_parameters=NULL;
    addMapToMap(&peTmp->additional_parameters,peCursor->additional_parameters);
    if(peCursor->format!=NULL)
      peTmp->format=zStrdup(peCursor->format);
    else
      peTmp->format=NULL;
    if(peCursor->defaults!=NULL){
      peTmp->defaults=(iotype*)malloc(IOTYPE_SIZE);
      peTmp->defaults->content=NULL;
      addMapToMap(&peTmp->defaults->content,peCursor->defaults->content);
      peTmp->defaults->next=NULL;
#ifdef DEBUG
      fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
      dumpMap(peTmp->defaults->content);
#endif
    }else
      peTmp->defaults=NULL;
    if(peCursor->supported!=NULL && peCursor->supported->content!=NULL){
      iotype *piotTmp=peCursor->supported->next;
      peTmp->supported=(iotype*)malloc(IOTYPE_SIZE);
      peTmp->supported->content=NULL;
      addMapToMap(&peTmp->supported->content,peCursor->supported->content);
      peTmp->supported->next=NULL;
            while(piotTmp!=NULL){
	addMapToIoType(&peTmp->supported,piotTmp->content);
#ifdef DEBUG
	fprintf(stderr,">> %s %i\n",__FILE__,__LINE__);
	dumpMap(peTmp->defaults->content);
#endif
	piotTmp=piotTmp->next;
      }
    }
    else
      peTmp->supported=NULL;
    if(peCursor->child!=NULL)
      peTmp->child=dupElements(peCursor->child);
    else
      peTmp->child=NULL;
    if(peCursor->next!=NULL)
      peTmp->next=dupElements(peCursor->next);
    else
      peTmp->next=NULL;
  }
  return peTmp;
}

/**
 * Add an elements to another elements.
 *
 * @see dupElements
 * @param ppeElem the elements to add the e
 * @param peELem the elements to be added to m
 */
void addToElements(elements** ppeElem,elements* peELem){
  elements* peTmp=peELem;
  if(*ppeElem==NULL){
    (*ppeElem)=dupElements(peTmp);
  }else{
    addToElements(&(*ppeElem)->next,peTmp);
  }
}

/**
 * Set the name of a service
 *
 * @param ppsServ the service
 * @param pcName the service name
 */
void setServiceName(service** ppsServ,char* pcName){
  service* psRes=*ppsServ;
  psRes->name=zStrdup(pcName);
  psRes->content=NULL;
  psRes->metadata=NULL;
  psRes->inputs=NULL;
  psRes->outputs=NULL;
}

/**
 * Dump a service on stderr
 *
 * @param psServ the service to dump
 */
void dumpService(service* psServ){
  if(psServ==NULL)
    return;
  fprintf(stderr,"++++++++++++++++++\nSERVICE [%s]\n++++++++++++++++++\n",psServ->name);
  if(psServ->content!=NULL){
    fprintf(stderr,"CONTENT MAP\n");
    dumpMap(psServ->content);
    if(psServ->metadata!=NULL)
      fprintf(stderr,"CONTENT METADATA\n");
    dumpMap(psServ->metadata);
    if(psServ->additional_parameters!=NULL)
      fprintf(stderr,"CONTENT AdditionalParameters\n");
    dumpMap(psServ->additional_parameters);
  }
  if(psServ->inputs!=NULL){
    fprintf(stderr,"INPUT ELEMENTS [%s]\n------------------\n",psServ->name);
    dumpElements(psServ->inputs);
  }
  if(psServ->outputs!=NULL){
    fprintf(stderr,"OUTPUT ELEMENTS [%s]\n------------------\n",psServ->name);
    dumpElements(psServ->outputs);
  }
  fprintf(stderr,"++++++++++++++++++\n");
}

/**
 * Dump a service on stderr using the YAML syntaxe
 *
 * @param psServ the service to dump
 */
void dumpServiceAsYAML(service* psServ){
  int i;
  fprintf(stderr,"# %s\n\n",psServ->name);
  if(psServ->content!=NULL){
    map* pmCurs=psServ->content;
    dumpMap(pmCurs);
    pmCurs=psServ->metadata;
    if(pmCurs!=NULL){
      fprintf(stderr,"MetaData:\n");
      while(pmCurs!=NULL){
	for(i=0;i<2;i++)
	  fprintf(stderr," ");
	_dumpMap(pmCurs);
	pmCurs=pmCurs->next;
      }
    }
  }
  if(psServ->inputs!=NULL){
    fprintf(stderr,"\ninputs:\n");
    dumpElementsAsYAML(psServ->inputs,0);
  }
  if(psServ->outputs!=NULL){
    fprintf(stderr,"\noutputs:\n");
    dumpElementsAsYAML(psServ->outputs,0);
  }
}

/**
 * Duplicate a service
 * 
 * @param psServ the service to clone
 * @return the allocated service containing a copy of the serfvice s
 */
service* dupService(service* psServ){
  service *psRes=(service*)malloc(SERVICE_SIZE);
  psRes->name=zStrdup(psServ->name);
  psRes->content=NULL;
  addMapToMap(&psRes->content,psServ->content);
  psRes->metadata=NULL;
  addMapToMap(&psRes->metadata,psServ->metadata);
  psRes->additional_parameters=NULL;
  addMapToMap(&psRes->additional_parameters,psServ->additional_parameters);
  psRes->inputs=dupElements(psServ->inputs);
  psRes->outputs=dupElements(psServ->outputs);
  return psRes;
}

/**
 * Print the registry on stderr.
 * 
 * @param prReg the registry
 */
void dumpRegistry(registry* prReg){
  registry* prCurs=prReg;
  while(prCurs!=NULL){
    services* psServ=prCurs->content;
    fprintf(stderr,"%s \n",prCurs->name);
    psServ=prCurs->content;
    while(psServ!=NULL){
      dumpService(psServ->content);
      psServ=psServ->next;
    }
    prCurs=prCurs->next;
  }
}

/**
 * Add a service to the registry
 *
 * @param prReg the resgitry to add the service
 * @param pcName the registry name to update
 * @param psContent the service to add
 */
bool addServiceToRegistry(registry** prReg,char* pcName,service* psContent){
  registry *l=*prReg;
  int isInitial=-1;
  if(l==NULL){
    l=(registry*)malloc(REGISTRY_SIZE);
    isInitial=1;
  }
  if(l!=NULL){
    int hasLevel=-1;
    while(isInitial<0 && l!=NULL){
      if(l->name!=NULL && strcasecmp(pcName,l->name)==0){
	hasLevel=1;
	break;
      }
      l=l->next;
    }
    if(hasLevel<0){
      if(isInitial<0)
	l=(registry*)malloc(REGISTRY_SIZE);
      l->name=zStrdup(pcName);
      l->content=NULL;
      l->next=NULL;
    }
    if(l->content==NULL){
      l->content=(services*)malloc(SERVICES_SIZE);
      l->content->content=dupService(psContent);
      l->content->next=NULL;
    }
    else{
      services* psServ=l->content;
      while(psServ->next!=NULL)
	psServ=psServ->next;
      psServ->next=(services*)malloc(SERVICES_SIZE);
      psServ->next->content=dupService(psContent);
      psServ->next->next=NULL;
    }
    l->next=NULL;
    if(isInitial>0)
      *prReg=l;
    else{
      registry *prCurs=*prReg;
      while(prCurs->next!=NULL)
	prCurs=prCurs->next;
      prCurs->next=l;
      prCurs->next->next=NULL;
    }
    return true;
  }
  else
    return false;
}

/**
 * Free memory allocated for the registry
 *
 * @param prReg the registry
 */
void freeRegistry(registry** prReg){
  registry* prLocalRef=*prReg;
  while(prLocalRef!=NULL){
    services* psServ=prLocalRef->content;
    free(prLocalRef->name);
    while(psServ!=NULL){
      service* psServ1=psServ->content;
      psServ=psServ->next;
      if(psServ1!=NULL){
	freeService(&psServ1);
	free(psServ1);
	psServ1=NULL;
      }
    }
    prLocalRef=prLocalRef->next;
  }    
}

/**
 * Access a service in the registry
 *
 * @param prReg the registry
 * @param pcLevel the regitry to search ("concept", "generic" or "implementation")
 * @param pcName the service name
 * @return the service pointer if a corresponding service was found or NULL
 */
service* getServiceFromRegistry(registry* prReg,char  *pcLevel,char* pcName){
  registry *prLocalRef=prReg;
  while(prLocalRef!=NULL){
    if(strcasecmp(prLocalRef->name,pcLevel)==0){
      services* psServ=prLocalRef->content;
      while(psServ!=NULL){
	if(psServ->content!=NULL && strcasecmp(psServ->content->name,pcName)==0)
	  return psServ->content;
	psServ=psServ->next;
      }
      break;
    }
    prLocalRef=prLocalRef->next;
  }
  return NULL;
}

/**
 * Apply inheritance to an out map from a reference in map
 *
 * @param ppmOut the map to update
 * @param pmIn the reference map (containing inherited properties)
 */
void inheritMap(map** ppmOut,map* pmIn){
  map* pmContent=pmIn;
  if((*ppmOut)==NULL){
    addMapToMap(ppmOut,pmIn);
    return;
  }
  while(pmContent!=NULL){
    map* pmCurrent=getMap(*ppmOut,pmContent->name);
    if(pmCurrent==NULL)
      addToMap(*ppmOut,pmCurrent->name,pmCurrent->value);
    pmCurrent=pmCurrent->next;
  }
}

/**
 * Apply inheritance to an out iotype from a reference in iotype
 * 
 * @param ppiotOut the iotype to update
 * @param piotIn the reference iotype (containing inherited properties)
 */
void inheritIOType(iotype** ppiotOut,iotype* piotIn){
  iotype* piotInCurosor=piotIn;
  iotype* ppiotOutCursor=*ppiotOut;
  if(piotInCurosor!=NULL){
    if(*ppiotOut==NULL){
      *ppiotOut=(iotype*)malloc(IOTYPE_SIZE);
      (*ppiotOut)->content=NULL;
      addMapToMap(&(*ppiotOut)->content,piotInCurosor->content);
      (*ppiotOut)->next=NULL;
      ppiotOutCursor=*ppiotOut;
      inheritIOType(&ppiotOutCursor->next,piotInCurosor->next);
    }else{
      inheritIOType(&ppiotOutCursor->next,piotInCurosor->next);
    }
  }
}

/**
 * Apply inheritance to an out elements from a reference in elements
 * 
 * @param ppeOut the elements to update
 * @param peIn the reference elements (containing inherited properties)
 */
void inheritElements(elements** ppeOut,elements* peIn){
  elements* peContent=peIn;
  while(peContent!=NULL && *ppeOut!=NULL){
    elements* peCurrent=getElements(*ppeOut,peContent->name);
    if(peCurrent==NULL)
      addToElements(ppeOut,peContent);
    else{
      inheritMap(&peCurrent->content,peContent->content);
      inheritMap(&peCurrent->metadata,peContent->metadata);
      if(peCurrent->format==NULL && peContent->format!=NULL)
	peCurrent->format=zStrdup(peContent->format);
      inheritIOType(&peCurrent->defaults,peContent->defaults);
      if(peCurrent->supported==NULL)
	inheritIOType(&peCurrent->supported,peContent->supported);
      else{
	iotype* piotTmp=peContent->supported;
	while(piotTmp!=NULL){
	  addMapToIoType(&peCurrent->supported,piotTmp->content);
	  piotTmp=piotTmp->next;
	}
      }
    }
    peContent=peContent->next;
  }
}

/**
 * Apply inheritance to a service based on a registry
 * 
 * @param prReg the registry storing profiles hierarchy
 * @param psServ the service to update depending on its inheritance
 */
void inheritance(registry *prReg,service** psServ){
  service* psCursor=*psServ;
  map *profile,*level;
  if(prReg==NULL)
    return;
  if(psCursor==NULL || psCursor->content==NULL)
    return;
  profile=getMap(psCursor->content,"extend");
  level=getMap(psCursor->content,"level");
  if(profile!=NULL&&level!=NULL){
    service* s1;
    if(strncasecmp(level->value,"profile",7)==0)
      s1=getServiceFromRegistry(prReg,(char*)"generic",profile->value);
    else
      s1=getServiceFromRegistry(prReg,level->value,profile->value);
      
    inheritMap(&psCursor->content,s1->content);
    inheritMap(&psCursor->metadata,s1->metadata);
    if(psCursor->inputs==NULL && s1->inputs!=NULL){
      psCursor->inputs=dupElements(s1->inputs);
    }else{
      inheritElements(&psCursor->inputs,s1->inputs);
    }
    if(psCursor->outputs==NULL && s1->outputs!=NULL){
      psCursor->outputs=dupElements(s1->outputs);
    }else
      inheritElements(&psCursor->outputs,s1->outputs);
  }
}

/**
 * Convert a maps to a char*** (only used for Fortran support)
 *
 * @param pmsMap the maps to convert
 * @param pppcValues the resulting array
 */
void mapsToCharXXX(maps* pmsMap,char*** pppcValues){
  maps* pmsCursor=pmsMap;
  int i=0;
  int j=0;
  char aaacTmp[10][30][1024];
  memset(aaacTmp,0,1024*10*10);
  while(pmsCursor!=NULL){
    map* pmContent=pmsCursor->content;
    if(i>=10)
      break;
    strcpy(aaacTmp[i][j],"name");
    j++;
    strcpy(aaacTmp[i][j],pmsCursor->name);
    j++;
    while(pmContent!=NULL){
      if(j>=30)
	break;
      strcpy(aaacTmp[i][j],pmContent->name);
      j++;
      strcpy(aaacTmp[i][j],pmContent->value);
      j++;
      pmContent=pmContent->next;
    }
    pmsCursor=pmsCursor->next;
    j=0;
    i++;
  }
  memcpy(pppcValues,aaacTmp,10*10*1024);
}

/**
 * Convert a char*** to a maps (only used for Fortran support)
 *
 * @param pppcValues the array to convert
 * @param ppmsMaps the resulting maps
 */
void charxxxToMaps(char*** pppcValues,maps** ppmsMaps){
  maps* pmsCursor=*ppmsMaps;
  int i,j;
  char aaaTmp[10][30][1024];
  memcpy(aaaTmp,pppcValues,10*30*1024);
  for(i=0;i<10;i++){
    if(strlen(aaaTmp[i][1])==0)
      break;
    pmsCursor->name=aaaTmp[i][1];
    pmsCursor->content=NULL;
    pmsCursor->next=NULL;
    for(j=2;j<29;j+=2){
      if(strlen(aaaTmp[i][j+1])==0)
	break;
      if(pmsCursor->content==NULL)
	pmsCursor->content=createMap(aaaTmp[i][j],aaaTmp[i][j+1]);
      else
	addToMap(pmsCursor->content,aaaTmp[i][j],aaaTmp[i][j+1]);
    }
    pmsCursor=pmsCursor->next;
  }
  ppmsMaps=&pmsCursor;
}

/**
 * Verify that a map has a value
 *
 * @param pmMap pointer to map that should be checked
 * @return true if map has a value or false if value is missing/empty/NULL
 */
bool nonempty(map* pmMap) {
  return (pmMap != NULL && pmMap->value != NULL && strlen(pmMap->value) > 0 && strcmp(pmMap->value, "NULL") != 0);
}

/**
 * Verify that a particular map value exists in a maps
 * data structure, and obtain that value
 *
 * @param pmsSource pointer to maps structure
 * @param pccNode name of maps node to search
 * @param pccKey name of map node to find
 * @param ppmKvp address to the map* if it exists, otherwise NULL
 * @return true if map has a value or false if value is missing/NULL
 *
 * @note The map assigned to kvp is owned by the source maps
 */
bool hasvalue(maps* pmsSource, const char* pccNode, const char* pccKey, map** ppmKvp) {
  *ppmKvp = getMapFromMaps(pmsSource, pccNode, pccKey);
  return (*ppmKvp != NULL && (*ppmKvp)->value != NULL &&
	  strlen((*ppmKvp)->value) > 0 && strcmp((*ppmKvp)->value, "NULL") != 0);
}

/*
 * Set error message in configuration maps
 *
 * @param pmsaConf reference to configuration maps
 * @param pccService name of service
 * @param weExc WPSException code
 * @param pccMessage exception text (default: exception text in WPS specification)
 */
void setErrorMessage(maps*& pmsaConf, const char* pccService, WPSException weExc, const char* pccMessage) {
  if (pccMessage == NULL) {
    pccMessage = WPSExceptionText[weExc];
  }
  size_t len = strlen(pccService) + strlen(": ") + strlen(pccMessage) + strlen(": ") + strlen(WPSExceptionCode[weExc]) + 16;
  char* pcMsg = (char*)malloc(len * sizeof(char));
  if (pcMsg != NULL) {
    snprintf(pcMsg, len * sizeof(char), "\n%s: %s: %s\n", pccService, pccMessage, WPSExceptionCode[weExc]);
    setMapInMaps(pmsaConf, "lenv", "message", pcMsg);
    free(pcMsg);
  }
}

/**
 * Print debug message
 *
 * @param pccSource the file invoking the function
 * @param pccFunction the function calling for logMessage
 * @param iLine the line number
 * @param pccFile the file to log informations
 * @param pccMessage the message to be print
 */
void logMessage(const char* pccSource, const char* pccFunction, int iLne, const char* pccFile, const char* pccMessage) { //, const char* source, const char* function, int line) {

  size_t msglen = 512;
  const char empty[] = "";

  FILE* pfLog;

  // system time, process time [nanoseconds]   
  unsigned long long sys_t, proc_t;

  // processor time consumed by the program:
  clock_t t = clock();

  // system time:
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

  std::time_t now_t = std::chrono::system_clock::to_time_t(now);
  std::tm* tm = localtime(&now_t);
  char* pcStr = asctime(tm);
  pcStr[strlen(pcStr) - 1] = '\0'; // remove newline

  sys_t = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
  //proc_t = (unsigned long long)(1.0e9*t/CLOCKS_PER_SEC);
  proc_t = t;

  if (pccMessage != NULL) {
    msglen += strlen(pccMessage);
  }
  else {
    pccMessage = empty;
  }
  //getLastErrorMessage(); // cgiScriptName  
  char* pcText = (char*)malloc(sizeof(char)*msglen);

  snprintf(pcText, msglen, "pid: %d %s line %d %s() %s systime: %lld ns ticks: %lld %s\n",
	   zGetpid(), pccSource, iLne, pccFunction, pcStr, sys_t, proc_t, pccMessage); // __FILE__ __LINE__ __func__ //

  if (pccFile != NULL && (pfLog = fopen(pccFile, "a+")) != NULL) {
    fputs(pcText, pfLog);
    fclose(pfLog);
  }
  else {
#ifdef MSG_LOG_FILE
    if ((pfLog = fopen(MSG_LOG_FILE, "a+")) != NULL) {
      fputs(pcText, pfLog);
      fclose(pfLog);
    }
#endif
  }

  if (pcText != NULL) free(pcText);
}

// knut:
// Example:
// zooLog;
// zooLogMsg(NULL, getLastErrorMessage()); 
// zooLogMsg("log.txt", getLastErrorMessage()); 

#ifdef WIN32
#ifndef USE_MS
char *strcasestr (char const *pccA, char const *pccB)
  {
    char *pcX = zStrdup (pccA);
    char *pcY = zStrdup (pccB);
 
      pcX = _strlwr (pcX);
      pcY = _strlwr (pcY);
    char *pos = strstr (pcX, pcY);
    char *ret = pos == NULL ? NULL : (char *) (pccA + (pos - pcX));
      free (pcX);
      free (pcY);
      return ret;
  };
#else
   ;
#endif
#endif

/*
 * Dynamically allocate memory for a map value 
 *
 * @param node the map for which the value buffer should be allocated
 * @param num_bytes the number of bytes to allocate
 * @return pointer to the allocated memory buffer
 *
 * This function will free, and hence delete, any existing value in the map.
 * The memory should be deallocated by calling freeMap.
 */
char* allocateMapValue(map* pmNode, size_t sNumBytes)
{
  if (pmNode == NULL) {
    return NULL;
  }

  if (pmNode->value != NULL) {
    free(pmNode->value);
  }
  pmNode->value = (char*) malloc(sNumBytes);
		
  return pmNode->value;
}
