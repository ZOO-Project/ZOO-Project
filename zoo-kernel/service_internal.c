/**
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2011 GeoLabs SARL
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

#include "service_internal.h"

#ifdef WIN32
char *
strtok_r (char *s1, const char *s2, char **lasts)
{
  char *ret;
  if (s1 == NULL)
    s1 = *lasts;
  while (*s1 && strchr(s2, *s1))
    ++s1;
  if (*s1 == '\0')
    return NULL;
  ret = s1;
  while (*s1 && !strchr(s2, *s1))
    ++s1;
  if (*s1)
    *s1++ = '\0';
  *lasts = s1;
  return ret;
}
#endif

void addLangAttr(xmlNodePtr n,maps *m){
  map *tmpLmap=getMapFromMaps(m,"main","language");
  if(tmpLmap!=NULL)
    xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST tmpLmap->value);
  else
    xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST "en-US");
}

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

#ifdef WIN32

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define SHMEMSIZE 4096

static LPVOID lpvMemG = NULL;      // pointer to shared memory
static HANDLE hMapObjectG = NULL;  // handle to file mapping

void updateStatus(maps *conf){
	fprintf(stderr,"OK Final 1 \n");
	fflush(stderr);
	LPWSTR lpszTmp;
	BOOL fInit;
	char *s=NULL;
	map *tmpMap=getMapFromMaps(conf,"lenv","sid");
	fprintf(stderr,"OK Final 11 \n");
	fflush(stderr);
	if(hMapObjectG==NULL)
	hMapObjectG = CreateFileMapping( 
		INVALID_HANDLE_VALUE,   // use paging file
		NULL,                   // default security attributes
		PAGE_READWRITE,         // read/write access
		0,                      // size: high 32-bits
		SHMEMSIZE,              // size: low 32-bits
		TEXT(tmpMap->value));   // name of map object
	if (hMapObjectG == NULL){
		fprintf(stderr,"Unable to create share memory segment %s !! \n",tmpMap->value);
		return ;
	}
	fprintf(stderr,"OK Final 2 \n");
	fflush(stderr);
	fInit = (GetLastError() != ERROR_ALREADY_EXISTS); 
	if(lpvMemG==NULL)
	lpvMemG = MapViewOfFile( 
		hMapObjectG,     // object to map view of
		FILE_MAP_WRITE, // read/write access
		0,              // high offset:  map from
		0,              // low offset:   beginning
		0);             // default: map entire file
	if (lpvMemG == NULL){
		fprintf(stderr,"Unable to create or access the shared memory segment %s !! \n",tmpMap->value);
		return ;
	} 
	fprintf(stderr,"OK Final 3 \n");
	fflush(stderr);
	if (fInit)
		memset(lpvMemG, '\0', SHMEMSIZE);
	fprintf(stderr,"OK Final 4 \n");
	fflush(stderr);
	tmpMap=getMapFromMaps(conf,"lenv","status");
	lpszTmp = (LPWSTR) lpvMemG;
	for(s=tmpMap->value;*s!=NULL;s++)
		*lpszTmp++ = *s;
	*lpszTmp = '\0'; 
}

char* getStatus(int pid){
  LPWSTR lpszBuf=NULL;
  LPWSTR lpszTmp=NULL;
  LPVOID lpvMem = NULL;
  HANDLE hMapObject = NULL;
  BOOL fIgnore,fInit;
  char tmp[100];
  sprintf(tmp,"%i",pid);
  if(hMapObject==NULL)
    hMapObject = CreateFileMapping( 
				   INVALID_HANDLE_VALUE,   // use paging file
				   NULL,                   // default security attributes
				   PAGE_READWRITE,         // read/write access
				   0,                      // size: high 32-bits
				   4096,                   // size: low 32-bits
				   TEXT(tmp));   // name of map object
  if (hMapObject == NULL) 
    return FALSE;
  if((GetLastError() != ERROR_ALREADY_EXISTS)){
    fIgnore = UnmapViewOfFile(lpvMem); 
    fIgnore = CloseHandle(hMapObject);
    return "-1";
  }
  fInit=TRUE;
  if(lpvMem==NULL)
    lpvMem = MapViewOfFile( 
			   hMapObject,     // object to map view of
			   FILE_MAP_READ,  // read/write access
			   0,              // high offset:  map from
			   0,              // low offset:   beginning
			   0);             // default: map entire file
  if (lpvMem == NULL) 
    return "-1"; 
  lpszTmp = (LPWSTR) lpvMem;
  while (*lpszTmp!=NULL)
    *lpszBuf++ = *lpszTmp++;
  *lpszBuf = '\0';
  fIgnore = UnmapViewOfFile(lpvMem); 
  fIgnore = CloseHandle(hMapObject);
  return (char*)lpszBuf;
}

void unhandleStatus(maps *conf){
  BOOL fIgnore;
  fIgnore = UnmapViewOfFile(lpvMemG); 
  fIgnore = CloseHandle(hMapObjectG);
}
#else

void unhandleStatus(maps *conf){
  int shmid,i;
  key_t key;
  void *shm;
  struct shmid_ds shmids;
  char *s,*s1;
  map *tmpMap=getMapFromMaps(conf,"lenv","sid");
  if(tmpMap!=NULL){
    key=atoi(tmpMap->value);
    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
#ifdef DEBUG
      fprintf(stderr,"shmget failed to update value\n");
#endif
    }else{
      if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
#ifdef DEBUG
	fprintf(stderr,"shmat failed to update value\n");
#endif
      }else{
	shmdt(shm);
	shmctl(shmid,IPC_RMID,&shmids);
      }
    }
  }
}

void updateStatus(maps *conf){
  int shmid,i;
  key_t key;
  char *shm,*s,*s1;
  map *tmpMap=NULL;
  tmpMap=getMapFromMaps(conf,"lenv","sid");
  if(tmpMap!=NULL){
    key=atoi(tmpMap->value);
    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
#ifdef DEBUG
      fprintf(stderr,"shmget failed to create new Shared memory segment\n");
#endif
    }else{
      if ((shm = (char*) shmat(shmid, NULL, 0)) == (char *) -1) {
#ifdef DEBUG
	fprintf(stderr,"shmat failed to update value\n");
#endif
      }
      else{
	tmpMap=getMapFromMaps(conf,"lenv","status");
	s1=shm;
	for(s=tmpMap->value;*s!=NULL && *s!=0;s++){
	  *s1++=*s;
	}
	*s1=NULL;
	shmdt((void *)shm);
      }
    }
  }
}

char* getStatus(int pid){
  int shmid,i;
  key_t key;
  void *shm;
  char *s;
  key=pid;
  if ((shmid = shmget(key, SHMSZ, 0666)) < 0) {
#ifdef DEBUG
    fprintf(stderr,"shmget failed in getStatus\n");
#endif
  }else{
    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
#ifdef DEBUG
      fprintf(stderr,"shmat failed in getStatus\n");
#endif
    }else{
      return (char*)shm;
    }
  }
  return "-1";
}

#endif

#ifdef USE_JS

JSBool
JSUpdateStatus(JSContext *cx, uintN argc, jsval *argv1)
{
  jsval *argv = JS_ARGV(cx,argv1);
  JS_MaybeGC(cx);
  char *sid;
  int istatus=0;
  char *status=NULL;
  maps *conf;
  int i=0;
  if(argc>2){
#ifdef JS_DEBUG
    fprintf(stderr,"Number of arguments used to call the function : %i",argc);
#endif
    return JS_FALSE;
  }
  conf=mapsFromJSObject(cx,argv[0]);
  if(JS_ValueToInt32(cx,argv[1],&istatus)==JS_TRUE){
    char tmpStatus[4];
    sprintf(tmpStatus,"%i",istatus);
    tmpStatus[3]=0;
    status=strdup(tmpStatus);
  }
  if(getMapFromMaps(conf,"lenv","status")!=NULL){
    fprintf(stderr,"STATUS RETURNED : %s\n",status);
    if(status!=NULL){
      setMapInMaps(conf,"lenv","status",status);
      free(status);
    }
    else
      setMapInMaps(conf,"lenv","status","15");
    updateStatus(conf);
  }
  freeMaps(&conf);
  free(conf);
  JS_MaybeGC(cx);
  return JS_TRUE;
}

#endif



/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = (char*) malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = (char*) malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') { 
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

char *zCapitalize1(char *tmp){
        char *res=strdup(tmp);
        if(res[0]>=97 && res[0]<=122)
                res[0]-=32;
        return res;
}

char *zCapitalize(char *tmp){
  int i=0;
  char *res=strdup(tmp);
  for(i=0;i<strlen(res);i++)
    if(res[i]>=97 && res[i]<=122)
      res[i]-=32;
  return res;
}


int zooXmlSearchForNs(const char* name){
  int i;
  int res=-1;
  for(i=0;i<nbNs;i++)
    if(strncasecmp(name,nsName[i],strlen(nsName[i]))==0){
      res=i;
      break;
    }
  return res;
}

int zooXmlAddNs(xmlNodePtr nr,const char* url,const char* name){
#ifdef DEBUG
  fprintf(stderr,"zooXmlAddNs %d \n",nbNs);
#endif
  int currId=-1;
  int currNode=-1;
  if(nbNs==0){
    nbNs++;
    currId=0;
    nsName[currId]=strdup(name);
    usedNs[currId]=xmlNewNs(nr,BAD_CAST url,BAD_CAST name);
  }else{
    currId=zooXmlSearchForNs(name);
    if(currId<0){
      nbNs++;
      currId=nbNs-1;
      nsName[currId]=strdup(name);
      usedNs[currId]=xmlNewNs(nr,BAD_CAST url,BAD_CAST name);
    }
  }
  return currId;
}

void zooXmlCleanupNs(){
  int j;
#ifdef DEBUG
  fprintf(stderr,"zooXmlCleanup %d\n",nbNs);
#endif
  for(j=nbNs-1;j>=0;j--){
#ifdef DEBUG
    fprintf(stderr,"zooXmlCleanup %d\n",j);
#endif
    if(j==0)
      xmlFreeNs(usedNs[j]);
    free(nsName[j]);
    nbNs--;
  }
  nbNs=0;
}

xmlNodePtr soapEnvelope(maps* conf,xmlNodePtr n){
  map* soap=getMapFromMaps(conf,"main","isSoap");
  if(soap!=NULL && strcasecmp(soap->value,"true")==0){
    int lNbNs=nbNs;
    nsName[lNbNs]=strdup("soap");
    usedNs[lNbNs]=xmlNewNs(NULL,BAD_CAST "http://www.w3.org/2003/05/soap-envelope",BAD_CAST "soap");
    nbNs++;
    xmlNodePtr nr = xmlNewNode(usedNs[lNbNs], BAD_CAST "Envelope");
    nsName[nbNs]=strdup("soap");
    usedNs[nbNs]=xmlNewNs(nr,BAD_CAST "http://www.w3.org/2003/05/soap-envelope",BAD_CAST "soap");
    nbNs++;
    nsName[nbNs]=strdup("xsi");
    usedNs[nbNs]=xmlNewNs(nr,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
    nbNs++;
    xmlNsPtr ns_xsi=usedNs[nbNs-1];
    xmlNewNsProp(nr,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.w3.org/2003/05/soap-envelope http://www.w3.org/2003/05/soap-envelope");
    xmlNodePtr nr1 = xmlNewNode(usedNs[lNbNs], BAD_CAST "Body");
    xmlAddChild(nr1,n);
    xmlAddChild(nr,nr1);
    return nr;
  }else
    return n;
}

xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr doc,const char* service,maps* m){

  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr n,nc,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;
  xmlChar *xmlbuff;
  int buffersize;
  /**
   * Create the document and its temporary root.
   */
  int wpsId=zooXmlAddNs(NULL,"http://www.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  maps* toto1=getMaps(m,"main");

  n = xmlNewNode(ns, BAD_CAST "Capabilities");
  int owsId=zooXmlAddNs(n,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
  ns_xsi=usedNs[xsiId];
  int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];
  xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsGetCapabilities_response.xsd"); 
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  addLangAttr(n,m);
  
  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"version");
    if(tmp!=NULL){
      xmlNewProp(n,BAD_CAST "version",BAD_CAST tmp->value);
    }
    else
      xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  }
  else
    xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");

  char tmp[256];
  
  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceIdentification");
  maps* tmp4=getMaps(m,"identification");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    char *orderedFields[5];
    orderedFields[0]="Title";
    orderedFields[1]="Abstract";
    orderedFields[2]="Keywords";
    orderedFields[3]="Fees";
    orderedFields[4]="AccessConstraints";
    int oI=0;
    for(oI=0;oI<5;oI++)
      if((tmp2=getMap(tmp4->content,orderedFields[oI]))!=NULL){
	if(strcasecmp(tmp2->name,"abstract")==0 ||
	   strcasecmp(tmp2->name,"title")==0 ||
	   strcasecmp(tmp2->name,"accessConstraints")==0 ||
	   strcasecmp(tmp2->name,"fees")==0){
	  tmp2->name[0]=toupper(tmp2->name[0]);
	  nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	  xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	  xmlAddChild(nc,nc1);
	}
	else
	  if(strcmp(tmp2->name,"keywords")==0){
	    nc1 = xmlNewNode(ns_ows, BAD_CAST "Keywords");
	    char *toto=tmp2->value;
	    char buff[256];
	    int i=0;
	    int j=0;
	    while(toto[i]){
	      if(toto[i]!=',' && toto[i]!=0){
		buff[j]=toto[i];
		buff[j+1]=0;
		j++;
	      }
	      else{
		nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
		xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
		xmlAddChild(nc1,nc2);
		j=0;
	      }
	      i++;
	    }
	    if(strlen(buff)>0){
	      nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	      xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	      xmlAddChild(nc1,nc2);
	    }
	    xmlAddChild(nc,nc1);
	    nc2 = xmlNewNode(ns_ows, BAD_CAST "ServiceType");
	    xmlAddChild(nc2,xmlNewText(BAD_CAST "WPS"));
	    xmlAddChild(nc,nc2);
	    nc2 = xmlNewNode(ns_ows, BAD_CAST "ServiceTypeVersion");
	    xmlAddChild(nc2,xmlNewText(BAD_CAST "1.0.0"));
	    xmlAddChild(nc,nc2);	  
	  }
	tmp2=tmp2->next;
      }
  }
  else{
    fprintf(stderr,"TMP4 NOT FOUND !!");
    return NULL;
  }
  xmlAddChild(n,nc);

  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceProvider");
  nc3 = xmlNewNode(ns_ows, BAD_CAST "ServiceContact");
  nc4 = xmlNewNode(ns_ows, BAD_CAST "ContactInfo");
  nc5 = xmlNewNode(ns_ows, BAD_CAST "Phone");
  nc6 = xmlNewNode(ns_ows, BAD_CAST "Address");
  tmp4=getMaps(m,"provider");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    char *tmpAddress[6];
    tmpAddress[0]="addressDeliveryPoint";
    tmpAddress[1]="addressCity";
    tmpAddress[2]="addressAdministrativeArea";
    tmpAddress[3]="addressPostalCode";
    tmpAddress[4]="addressCountry";
    tmpAddress[5]="addressElectronicMailAddress";
    char *tmpPhone[2];
    tmpPhone[0]="phoneVoice";
    tmpPhone[1]="phoneFacsimile";
    char *orderedFields[12];
    orderedFields[0]="providerName";
    orderedFields[1]="providerSite";
    orderedFields[2]="individualName";
    orderedFields[3]="positionName";
    orderedFields[4]=tmpPhone[0];
    orderedFields[5]=tmpPhone[1];
    orderedFields[6]=tmpAddress[0];
    orderedFields[7]=tmpAddress[1];
    orderedFields[8]=tmpAddress[2];
    orderedFields[9]=tmpAddress[3];
    orderedFields[10]=tmpAddress[4];
    orderedFields[11]=tmpAddress[5];
    int oI=0;
    for(oI=0;oI<12;oI++)
      if((tmp2=getMap(tmp4->content,orderedFields[oI]))!=NULL){
	if(strcmp(tmp2->name,"keywords")!=0 &&
	   strcmp(tmp2->name,"serverAddress")!=0 &&
	   strcmp(tmp2->name,"lang")!=0){
	  tmp2->name[0]=toupper(tmp2->name[0]);
	  if(strcmp(tmp2->name,"ProviderName")==0){
	    nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	    xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	    xmlAddChild(nc,nc1);
	  }
	  else{
	    if(strcmp(tmp2->name,"ProviderSite")==0){
	      nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	      xmlNewNsProp(nc1,ns_xlink,BAD_CAST "href",BAD_CAST tmp2->value);
	      xmlAddChild(nc,nc1);
	    } 
	    else  
	      if(strcmp(tmp2->name,"IndividualName")==0 || 
		 strcmp(tmp2->name,"PositionName")==0){
		nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
		xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
		xmlAddChild(nc3,nc1);
	      } 
	      else 
		if(strncmp(tmp2->name,"Phone",5)==0){
		  int j;
		  for(j=0;j<2;j++)
		    if(strcasecmp(tmp2->name,tmpPhone[j])==0){
		      char *toto=NULL;
		      char *toto1=tmp2->name;
		      toto=strstr(toto1,"Phone");
		      nc1 = xmlNewNode(ns_ows, BAD_CAST toto1+5);
		      xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
		      xmlAddChild(nc5,nc1);
		    }
		}
		else 
		  if(strncmp(tmp2->name,"Address",7)==0){
		    int j;
		    for(j=0;j<6;j++)
		      if(strcasecmp(tmp2->name,tmpAddress[j])==0){
			char *toto=NULL;
			char *toto1=tmp2->name;
			toto=strstr(toto1,"Address");
			nc1 = xmlNewNode(ns_ows, BAD_CAST toto1+7);
			xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
			xmlAddChild(nc6,nc1);
		      }
		  }
	  }
	}
	else
	  if(strcmp(tmp2->name,"keywords")==0){
	    nc1 = xmlNewNode(ns_ows, BAD_CAST "Keywords");
	    char *toto=tmp2->value;
	    char buff[256];
	    int i=0;
	    int j=0;
	    while(toto[i]){
	      if(toto[i]!=',' && toto[i]!=0){
		buff[j]=toto[i];
		buff[j+1]=0;
		j++;
	      }
	      else{
		nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
		xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
		xmlAddChild(nc1,nc2);
		j=0;
	      }
	      i++;
	    }
	    if(strlen(buff)>0){
	      nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	      xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	      xmlAddChild(nc1,nc2);
	    }
	    xmlAddChild(nc,nc1);
	  }
	tmp2=tmp2->next;
      }
  }
  else{
    fprintf(stderr,"TMP4 NOT FOUND !!");
  }
  xmlAddChild(nc4,nc5);
  xmlAddChild(nc4,nc6);
  xmlAddChild(nc3,nc4);
  xmlAddChild(nc,nc3);
  xmlAddChild(n,nc);


  nc = xmlNewNode(ns_ows, BAD_CAST "OperationsMetadata");
  char *tmp2[3];
  tmp2[0]=strdup("GetCapabilities");
  tmp2[1]=strdup("DescribeProcess");
  tmp2[2]=strdup("Execute");
  int j=0;

  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"serverAddress");
    if(tmp!=NULL){
      SERVICE_URL = strdup(tmp->value);
    }
    else
      SERVICE_URL = strdup("not_found");
  }
  else
    SERVICE_URL = strdup("not_found");

  for(j=0;j<3;j++){
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Operation");
    xmlNewProp(nc1,BAD_CAST "name",BAD_CAST tmp2[j]);
    nc2 = xmlNewNode(ns_ows, BAD_CAST "DCP");
    nc3 = xmlNewNode(ns_ows, BAD_CAST "HTTP");
    nc4 = xmlNewNode(ns_ows, BAD_CAST "Get");
    sprintf(tmp,"%s/%s",SERVICE_URL,service);
    xmlNewNsProp(nc4,ns_xlink,BAD_CAST "href",BAD_CAST tmp);
    xmlAddChild(nc3,nc4);
    if(j>0){
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Post");
      xmlNewNsProp(nc4,ns_xlink,BAD_CAST "href",BAD_CAST tmp);
      xmlAddChild(nc3,nc4);
    }
    xmlAddChild(nc2,nc3);
    xmlAddChild(nc1,nc2);    
    xmlAddChild(nc,nc1);    
  }
  for(j=2;j>=0;j--)
    free(tmp2[j]);
  xmlAddChild(n,nc);

  nc = xmlNewNode(ns, BAD_CAST "ProcessOfferings");
  xmlAddChild(n,nc);

  nc1 = xmlNewNode(ns, BAD_CAST "Languages");
  nc2 = xmlNewNode(ns, BAD_CAST "Default");
  nc3 = xmlNewNode(ns, BAD_CAST "Supported");
  
  toto1=getMaps(m,"main");
  if(toto1!=NULL){
    map* tmp1=getMap(toto1->content,"lang");
    char *toto=tmp1->value;
    char buff[256];
    int i=0;
    int j=0;
    int dcount=0;
    while(toto[i]){
      if(toto[i]!=',' && toto[i]!=0){
	buff[j]=toto[i];
	buff[j+1]=0;
	j++;
      }
      else{
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	if(dcount==0){
	  xmlAddChild(nc2,nc4);
	  xmlAddChild(nc1,nc2);
	  dcount++;
	}
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	xmlAddChild(nc3,nc4);
	j=0;
	buff[j]=0;
      }
      i++;
    }
    if(strlen(buff)>0){
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
      xmlAddChild(nc4,xmlNewText(BAD_CAST buff));	      
      xmlAddChild(nc3,nc4);
    }
  }
  xmlAddChild(nc1,nc3);
  xmlAddChild(n,nc1);
  
  xmlNodePtr fn=soapEnvelope(m,n);
  xmlDocSetRootElement(doc, fn);
  //xmlFreeNs(ns);
  free(SERVICE_URL);
  return nc;
}

void printGetCapabilitiesForProcess(maps* m,xmlNodePtr nc,service* serv){
  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr nr,n,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;
  /**
   * Initialize or get existing namspaces
   */
  int wpsId=zooXmlAddNs(NULL,"http://www.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];

  int cursor=0;
  map* tmp1;
  if(serv->content!=NULL){
    nc1 = xmlNewNode(ns, BAD_CAST "Process");
    tmp1=getMap(serv->content,"processVersion");
    if(tmp1!=NULL)
      xmlNewNsProp(nc1,ns,BAD_CAST "processVersion",BAD_CAST tmp1->value);
    printDescription(nc1,ns_ows,serv->name,serv->content);
    tmp1=serv->metadata;
    while(tmp1!=NULL){
      nc2 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
      xmlNewNsProp(nc2,ns_xlink,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
      xmlAddChild(nc1,nc2);
      tmp1=tmp1->next;
    }
    xmlAddChild(nc,nc1);
  }
}

xmlNodePtr printDescribeProcessHeader(xmlDocPtr doc,const char* service,maps* m){

  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr n,nr;
  xmlChar *xmlbuff;
  int buffersize;

  int wpsId=zooXmlAddNs(NULL,"http://schemas.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  n = xmlNewNode(ns, BAD_CAST "ProcessDescriptions");
  int owsId=zooXmlAddNs(n,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
  ns_xsi=usedNs[xsiId];
  
  xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  addLangAttr(n,m);

  xmlNodePtr fn=soapEnvelope(m,n);
  xmlDocSetRootElement(doc, fn);

  return n;
}

void printDescribeProcessForProcess(maps* m,xmlNodePtr nc,service* serv,int sc){
  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;

  char tmp[256];
  n=nc;
  
  int wpsId=zooXmlAddNs(NULL,"http://schemas.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(NULL,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];

  nc = xmlNewNode(NULL, BAD_CAST "ProcessDescription");
  char *tmp4[3];
  tmp4[0]="processVersion";
  tmp4[1]="storeSupported";
  tmp4[2]="statusSupported";
  int j=0;
  map* tmp1=NULL;
  for(j=0;j<3;j++){
    tmp1=getMap(serv->content,tmp4[j]);
    if(tmp1!=NULL){
      if(j==0)
	xmlNewNsProp(nc,ns,BAD_CAST "processVersion",BAD_CAST tmp1->value);      
      else
	xmlNewProp(nc,BAD_CAST tmp4[j],BAD_CAST tmp1->value);      
    }
    else{
      if(j>0)
	xmlNewProp(nc,BAD_CAST tmp4[j],BAD_CAST "false");      
    }
  }
  
  printDescription(nc,ns_ows,serv->name,serv->content);

  tmp1=serv->metadata;
  while(tmp1!=NULL){
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
    xmlNewNsProp(nc1,ns_xlink,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    xmlAddChild(nc,nc1);
    tmp1=tmp1->next;
  }

  tmp1=getMap(serv->content,"Profile");
  if(tmp1!=NULL){
    nc1 = xmlNewNode(ns, BAD_CAST "Profile");
    xmlAddChild(nc1,xmlNewText(BAD_CAST tmp1->value));
    xmlAddChild(nc,nc1);
  }

  nc1 = xmlNewNode(NULL, BAD_CAST "DataInputs");
  elements* e=serv->inputs;
  printFullDescription(e,"Input",ns_ows,nc1);
  xmlAddChild(nc,nc1);

  nc1 = xmlNewNode(NULL, BAD_CAST "ProcessOutputs");
  e=serv->outputs;
  printFullDescription(e,"Output",ns_ows,nc1);
  xmlAddChild(nc,nc1);

  xmlAddChild(n,nc);

}

void printFullDescription(elements *elem,const char* type,xmlNsPtr ns_ows,xmlNodePtr nc1){
  char *orderedFields[7];
  orderedFields[0]="mimeType";
  orderedFields[1]="encoding";
  orderedFields[2]="schema";
  orderedFields[3]="dataType";
  orderedFields[4]="uom";
  orderedFields[5]="CRS";
  orderedFields[6]="value";

  xmlNodePtr nc2,nc3,nc4,nc5,nc6,nc7;
  elements* e=elem;
  map* tmp1=NULL;
  while(e!=NULL){
    int default1=0;
    int isAnyValue=1;
    nc2 = xmlNewNode(NULL, BAD_CAST type);
    tmp1=getMap(e->content,"minOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }
    tmp1=getMap(e->content,"maxOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }

    printDescription(nc2,ns_ows,e->name,e->content);

    if(strncmp(type,"Output",6)==0){
      if(strncasecmp(e->format,"LITERALDATA",strlen(e->format))==0)
	nc3 = xmlNewNode(NULL, BAD_CAST "LiteralOutput");
      else if(strncasecmp(e->format,"COMPLEXDATA",strlen(e->format))==0)
	nc3 = xmlNewNode(NULL, BAD_CAST "ComplexOutput");
      else if(strncasecmp(e->format,"BOUNDINGBOXDATA",strlen(e->format))==0)
	nc3 = xmlNewNode(NULL, BAD_CAST "BoundingBoxOutput");
      else
	nc3 = xmlNewNode(NULL, BAD_CAST e->format);
    }else{
      if(strncasecmp(e->format,"LITERALDATA",strlen(e->format))==0){
	nc3 = xmlNewNode(NULL, BAD_CAST "LiteralData");
      }
      else if(strncasecmp(e->format,"COMPLEXDATA",strlen(e->format))==0)
	nc3 = xmlNewNode(NULL, BAD_CAST "ComplexData");
      else if(strncasecmp(e->format,"BOUNDINGBOXDATA",strlen(e->format))==0)
	nc3 = xmlNewNode(NULL, BAD_CAST "BoundingBoxData");
      else
	nc3 = xmlNewNode(NULL, BAD_CAST e->format);
    }
    iotype* _tmp=e->defaults;
    int datatype=0;
    bool hasDefault=false;
    bool hasUOM=false;
    if(_tmp!=NULL){
      if(strcmp(e->format,"LiteralOutput")==0 ||
	 strcmp(e->format,"LiteralData")==0){
     	datatype=1;
	nc4 = xmlNewNode(NULL, BAD_CAST "UOMs");
	nc5 = xmlNewNode(NULL, BAD_CAST "Default");
      }
      else if(strcmp(e->format,"BoundingBoxOutput")==0 ||
	      strcmp(e->format,"BoundingBoxData")==0){
	datatype=2;
	//nc4 = xmlNewNode(NULL, BAD_CAST "BoundingBoxOutput");
	nc5 = xmlNewNode(NULL, BAD_CAST "Default");
      }
      else{
	nc4 = xmlNewNode(NULL, BAD_CAST "Default");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      
      tmp1=_tmp->content;
      int avcnt=0;
      int dcnt=0;
      int oI=0;
      for(oI=0;oI<7;oI++)
	if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
	  //while(tmp1!=NULL){
#ifdef DEBUG
	  printf("DATATYPE DEFAULT ? %s\n",tmp1->name);
#endif
	  if(strncasecmp(tmp1->name,"DataType",8)==0){
	    nc6 = xmlNewNode(ns_ows, BAD_CAST "DataType");
	    xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	    char tmp[1024];
	    sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	    xmlNewNsProp(nc6,ns_ows,BAD_CAST "reference",BAD_CAST tmp);
	    xmlAddChild(nc3,nc6);
	    tmp1=tmp1->next;
	    datatype=1;
	    continue;
	  }
	  if(strcmp(tmp1->name,"asReference")!=0 &&
	     strncasecmp(tmp1->name,"DataType",8)!=0 &&
	     strcasecmp(tmp1->name,"extension")!=0 &&
	     strcasecmp(tmp1->name,"value")!=0 &&
	     strncasecmp(tmp1->name,"AllowedValues",13)!=0){
	    if(datatype!=1){
	      char *tmp2=zCapitalize1(tmp1->name);
	      nc6 = xmlNewNode(NULL, BAD_CAST tmp2);
	      free(tmp2);
	    }
	    else{
	      char *tmp2=zCapitalize(tmp1->name);
	      nc6 = xmlNewNode(ns_ows, BAD_CAST tmp2);
	      free(tmp2);
	    }
	    xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	    xmlAddChild(nc5,nc6);
	    hasUOM=true;
	  }else 
	    if(strncmp(type,"Input",5)==0){
	      if(strcmp(tmp1->name,"value")==0){
		nc7 = xmlNewNode(NULL, BAD_CAST "DefaultValue");
		xmlAddChild(nc7,xmlNewText(BAD_CAST tmp1->value));
		default1=1;
	      }
	      if(strncasecmp(tmp1->name,"AllowedValues",13)==0){
		nc6 = xmlNewNode(ns_ows, BAD_CAST "AllowedValues");
		fprintf(stderr,"ALLOWED VALUE %s\n",tmp1->value);
		char *token,*saveptr1;
		token=strtok_r(tmp1->value,",",&saveptr1);
		while(token!=NULL){
		  nc7 = xmlNewNode(ns_ows, BAD_CAST "Value");
		  char *tmps=strdup(token);
		  tmps[strlen(tmps)]=0;
		  xmlAddChild(nc7,xmlNewText(BAD_CAST tmps));
		  fprintf(stderr,"strgin : %s\n",tmps);
		  xmlAddChild(nc6,nc7);
		  token=strtok_r(NULL,",",&saveptr1);
		}
		xmlAddChild(nc3,nc6);
		isAnyValue=-1;
	      }
	      hasDefault=true;
	    }
	  tmp1=tmp1->next;
	  if(datatype!=2){
	    if(hasUOM==true){
	      xmlAddChild(nc4,nc5);
	      xmlAddChild(nc3,nc4);
	    }
	  }else{
	    xmlAddChild(nc3,nc5);
	  }
	 
	  if(strncmp(type,"Input",5)==0){
	    if(datatype==1 && isAnyValue==1 && avcnt==0){
	      xmlAddChild(nc3,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
	      hasDefault=true;
	      avcnt++;
	    }
	    if(datatype==1 && default1>0){
	      xmlAddChild(nc3,nc7);
	    }
	  }
	}
    }

    _tmp=e->supported;
    dumpMap(e->defaults->content);
    if(_tmp==NULL && (getMap(e->defaults->content,"uom")!=NULL || datatype!=1))
      _tmp=e->defaults;

    int hasSupported=-1;
    while(_tmp!=NULL){
      if(hasSupported<0){
	if(datatype==0){
	  nc4 = xmlNewNode(NULL, BAD_CAST "Supported");
	  nc5 = xmlNewNode(NULL, BAD_CAST "Format");
	}
	else
	  nc5 = xmlNewNode(NULL, BAD_CAST "Supported");
	hasSupported=0;
      }else
	if(datatype==0)
	  nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      tmp1=_tmp->content;
      int oI=0;
      for(oI=0;oI<6;oI++)
	if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
#ifdef DEBUG
	  printf("DATATYPE SUPPORTED ? %s\n",tmp1->name);
#endif
	  if(strcmp(tmp1->name,"asReference")!=0 && 
	     strcmp(tmp1->name,"DataType")!=0 &&
	     strcasecmp(tmp1->name,"extension")!=0){
	    if(datatype!=1){
	      char *tmp2=zCapitalize1(tmp1->name);
	      nc6 = xmlNewNode(NULL, BAD_CAST tmp2);
	      free(tmp2);
	    }
	    else{
	      char *tmp2=zCapitalize(tmp1->name);
	      nc6 = xmlNewNode(ns_ows, BAD_CAST tmp2);
	      free(tmp2);
	    }
	    if(datatype==2){
	      char *tmpv,*tmps;
	      tmps=strtok_r(tmp1->value,",",&tmpv);
	      while(tmps){
		xmlAddChild(nc6,xmlNewText(BAD_CAST tmps));
		xmlAddChild(nc5,nc6);
		tmps=strtok_r(NULL,",",&tmpv);
		if(tmps){
		  char *tmp2=zCapitalize1(tmp1->name);
		  nc6 = xmlNewNode(NULL, BAD_CAST tmp2);
		  free(tmp2);
		}
	      }
	    }
	    else{
	      xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	      xmlAddChild(nc5,nc6);
	    }
	  }
	  tmp1=tmp1->next;
	}
      if(hasSupported<=0){
	if(datatype!=2){
	  xmlAddChild(nc4,nc5);
	  xmlAddChild(nc3,nc4);
	}else
	  xmlAddChild(nc3,nc5);
	hasSupported=1;
      }
      else
	if(datatype!=2){
	  xmlAddChild(nc4,nc5);
	}
	else
	  xmlAddChild(nc3,nc5);
      _tmp=_tmp->next;
    }
    xmlAddChild(nc2,nc3);
    
    if(datatype!=2 && hasUOM==true){
      xmlAddChild(nc3,nc4);
      xmlAddChild(nc2,nc3);
    }else if(datatype!=2){
      if(hasDefault!=true && strncmp(type,"Input",5)==0)
	xmlAddChild(nc3,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
    }
    
    xmlAddChild(nc1,nc2);
    
    e=e->next;
  }
}

void printProcessResponse(maps* m,map* request, int pid,service* serv,const char* service,int status,maps* inputs,maps* outputs){
  xmlNsPtr ns,ns1,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc,nc1,nc2,nc3,pseudor;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  int buffersize;
  time_t time1;  
  time(&time1);
  nr=NULL;
  /**
   * Create the document and its temporary root.
   */
  doc = xmlNewDoc(BAD_CAST "1.0");
  int wpsId=zooXmlAddNs(NULL,"http://www.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];

  n = xmlNewNode(ns, BAD_CAST "ExecuteResponse");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  int owsId=zooXmlAddNs(n,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];
  int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
  ns_xsi=usedNs[xsiId];
  
  xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsExecute_response.xsd");
  
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  addLangAttr(n,m);

  char tmp[256];
  char url[1024];
  char stored_path[1024];
  memset(tmp,0,256);
  memset(url,0,1024);
  memset(stored_path,0,1024);
  maps* tmp_maps=getMaps(m,"main");
  if(tmp_maps!=NULL){
    map* tmpm1=getMap(tmp_maps->content,"serverAddress");
    /**
     * Check if the ZOO Service GetStatus is available in the local directory.
     * If yes, then it uses a reference to an URL which the client can access
     * to get information on the status of a running Service (using the 
     * percentCompleted attribute). 
     * Else fallback to the initial method using the xml file to write in ...
     */
    char ntmp[1024];
#ifndef WIN32
    getcwd(ntmp,1024);
#else
    _getcwd(ntmp,1024);
#endif
    struct stat myFileInfo;
    int statRes;
    char file_path[1024];
    sprintf(file_path,"%s/GetStatus.zcfg",ntmp);
    statRes=stat(file_path,&myFileInfo);
    if(statRes==0){
      char currentSid[128];
      map* tmpm=getMap(tmp_maps->content,"rewriteUrl");
      map *tmp_lenv=NULL;
      tmp_lenv=getMapFromMaps(m,"lenv","sid");
      if(tmp_lenv==NULL)
	sprintf(currentSid,"%i",pid);
      else
	sprintf(currentSid,"%s",tmp_lenv->value);
      if(tmpm==NULL || strcasecmp(tmpm->value,"false")==0){
	sprintf(url,"%s?request=Execute&service=WPS&version=1.0.0&Identifier=GetStatus&DataInputs=sid=%s&RawDataOutput=Result",tmpm1->value,currentSid);
      }else{
	if(strlen(tmpm->value)>0)
	  if(strcasecmp(tmpm->value,"true")!=0)
	    sprintf(url,"%s/%s/GetStatus/%s",tmpm1->value,tmpm->value,currentSid);
	  else
	    sprintf(url,"%s/GetStatus/%s",tmpm1->value,currentSid);
	else
	  sprintf(url,"%s/?request=Execute&service=WPS&version=1.0.0&Identifier=GetStatus&DataInputs=sid=%s&RawDataOutput=Result",tmpm1->value,currentSid);
      }
    }else{
      map* tmpm2=getMap(tmp_maps->content,"tmpUrl");
      if(tmpm1!=NULL && tmpm2!=NULL){
	sprintf(url,"%s/%s/%s_%i.xml",tmpm1->value,tmpm2->value,service,pid);
      }
    }
    if(tmpm1!=NULL)
      sprintf(tmp,"%s",tmpm1->value);
    tmpm1=getMapFromMaps(m,"main","TmpPath");
    sprintf(stored_path,"%s/%s_%i.xml",tmpm1->value,service,pid);
  }

  

  xmlNewProp(n,BAD_CAST "serviceInstance",BAD_CAST tmp);
  map* test=getMap(request,"storeExecuteResponse");
  bool hasStoredExecuteResponse=false;
  if(test!=NULL && strcasecmp(test->value,"true")==0){
    xmlNewProp(n,BAD_CAST "statusLocation",BAD_CAST url);
    hasStoredExecuteResponse=true;
  }

  nc = xmlNewNode(ns, BAD_CAST "Process");
  map* tmp2=getMap(serv->content,"processVersion");

  if(tmp2!=NULL)
    xmlNewNsProp(nc,ns,BAD_CAST "processVersion",BAD_CAST tmp2->value);
  
  printDescription(nc,ns_ows,serv->name,serv->content);
  fflush(stderr);

  xmlAddChild(n,nc);

  nc = xmlNewNode(ns, BAD_CAST "Status");
  const struct tm *tm;
  size_t len;
  time_t now;
  char *tmp1;
  map *tmpStatus;
  
  now = time ( NULL );
  tm = localtime ( &now );

  tmp1 = (char*)malloc((TIME_SIZE+1)*sizeof(char));

  len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );

  xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);

  char sMsg[2048];
  switch(status){
  case SERVICE_SUCCEEDED:
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessSucceeded");
    sprintf(sMsg,_("Service \"%s\" run successfully."),serv->name);
    nc3=xmlNewText(BAD_CAST sMsg);
    xmlAddChild(nc1,nc3);
    break;
  case SERVICE_STARTED:
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessStarted");
    tmpStatus=getMapFromMaps(m,"lenv","status");
    xmlNewProp(nc1,BAD_CAST "percentCompleted",BAD_CAST tmpStatus->value);
    sprintf(sMsg,_("ZOO Service \"%s\" is currently running. Please, reload this document to get the up-to-date status of the Service."),serv->name);
    nc3=xmlNewText(BAD_CAST sMsg);
    xmlAddChild(nc1,nc3);
    break;
  case SERVICE_ACCEPTED:
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessAccepted");
    sprintf(sMsg,_("Service \"%s\" was accepted by the ZOO Kernel and it run as a background task. Please consult the statusLocation attribtue providen in this document to get the up-to-date document."),serv->name);
    nc3=xmlNewText(BAD_CAST sMsg);
    xmlAddChild(nc1,nc3);
    break;
  case SERVICE_FAILED:
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessFailed");
    map *errorMap;
    map *te;
    te=getMapFromMaps(m,"lenv","code");
    if(te!=NULL)
      errorMap=createMap("code",te->value);
    else
      errorMap=createMap("code","NoApplicableCode");
    te=getMapFromMaps(m,"lenv","message");
    if(te!=NULL)
      addToMap(errorMap,"text",_ss(te->value));
    else
      addToMap(errorMap,"text",_("No more information available"));
    nc3=createExceptionReportNode(m,errorMap,0);
    freeMap(&errorMap);
    free(errorMap);
    xmlAddChild(nc1,nc3);
    break;
  default :
    printf(_("error code not know : %i\n"),status);
    //exit(1);
    break;
  }
  xmlAddChild(nc,nc1);
  xmlAddChild(n,nc);
  free(tmp1);

#ifdef DEBUG
  fprintf(stderr,"printProcessResponse 1 161\n");
#endif

  map* lineage=getMap(request,"lineage");
  if(lineage!=NULL && strcasecmp(lineage->value,"true")==0){
    nc = xmlNewNode(ns, BAD_CAST "DataInputs");
    int i;
    maps* mcursor=inputs;
    elements* scursor=NULL;
    while(mcursor!=NULL /*&& scursor!=NULL*/){
      scursor=getElements(serv->inputs,mcursor->name);
      printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Input");
      mcursor=mcursor->next;
    }
    xmlAddChild(n,nc);
    
#ifdef DEBUG
    fprintf(stderr,"printProcessResponse 1 177\n");
#endif

    nc = xmlNewNode(ns, BAD_CAST "OutputDefinitions");
    mcursor=outputs;
    scursor=NULL;
    while(mcursor!=NULL){
      scursor=getElements(serv->outputs,mcursor->name);
      printOutputDefinitions1(doc,nc,ns,ns_ows,scursor,mcursor,"Output");
      mcursor=mcursor->next;
    }
    xmlAddChild(n,nc);
  }
#ifdef DEBUG
  fprintf(stderr,"printProcessResponse 1 190\n");
#endif

  /**
   * Display the process output only when requested !
   */
  if(status==SERVICE_SUCCEEDED){
    nc = xmlNewNode(ns, BAD_CAST "ProcessOutputs");
    maps* mcursor=outputs;
    elements* scursor=serv->outputs;
    while(mcursor!=NULL){
      scursor=getElements(serv->outputs,mcursor->name);
      if(scursor!=NULL){
	printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Output");
      }
      mcursor=mcursor->next;
    }
    xmlAddChild(n,nc);
  }
#ifdef DEBUG
  fprintf(stderr,"printProcessResponse 1 202\n");
#endif
  nr=soapEnvelope(m,n);
  xmlDocSetRootElement(doc, nr);

  if(hasStoredExecuteResponse==true){
    /* We need to write the ExecuteResponse Document somewhere */
    FILE* output=fopen(stored_path,"w");
    xmlChar *xmlbuff;
    int buffersize;
    xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, "UTF-8", 1);
    fwrite(xmlbuff,1,xmlStrlen(xmlbuff)*sizeof(char),output);
    xmlFree(xmlbuff);
    fclose(output);
  }
  printDocument(m,doc,pid);

  xmlCleanupParser();
  zooXmlCleanupNs();
}


void printDocument(maps* m, xmlDocPtr doc,int pid){
  char *encoding=getEncoding(m);
  if(pid==getpid()){
    printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
  }
  fflush(stdout);
  xmlChar *xmlbuff;
  int buffersize;
  /*
   * Dump the document to a buffer and print it on stdout
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  printf("%s",xmlbuff);
  fflush(stdout);
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  zooXmlCleanupNs();
}

void printOutputDefinitions1(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,maps* m,const char* type){
  xmlNodePtr nc1;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=NULL;  
  if(e!=NULL && e->defaults!=NULL)
    tmp=e->defaults->content;
  else{
    /*
    dumpElements(e);
    */
    return;
  }
  while(tmp!=NULL){
    if(strncasecmp(tmp->name,"MIMETYPE",strlen(tmp->name))==0
       || strncasecmp(tmp->name,"ENCODING",strlen(tmp->name))==0
       || strncasecmp(tmp->name,"SCHEMA",strlen(tmp->name))==0
       || strncasecmp(tmp->name,"UOM",strlen(tmp->name))==0)
    xmlNewProp(nc1,BAD_CAST tmp->name,BAD_CAST tmp->value);
    tmp=tmp->next;
  }
  tmp=getMap(e->defaults->content,"asReference");
  if(tmp==NULL)
    xmlNewProp(nc1,BAD_CAST "asReference",BAD_CAST "false");

  tmp=e->content;

  printDescription(nc1,ns_ows,m->name,e->content);

  xmlAddChild(nc,nc1);

}

void printOutputDefinitions(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,map* m,const char* type){
  xmlNodePtr nc1,nc2,nc3;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=NULL;  
  if(e!=NULL && e->defaults!=NULL)
    tmp=e->defaults->content;
  else{
    /*
    dumpElements(e);
    */
    return;
  }
  while(tmp!=NULL){
    xmlNewProp(nc1,BAD_CAST tmp->name,BAD_CAST tmp->value);
    tmp=tmp->next;
  }
  tmp=getMap(e->defaults->content,"asReference");
  if(tmp==NULL)
    xmlNewProp(nc1,BAD_CAST "asReference",BAD_CAST "false");

  tmp=e->content;

  printDescription(nc1,ns_ows,m->name,e->content);

  xmlAddChild(nc,nc1);

}

void printIOType(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,xmlNsPtr ns_xlink,elements* e,maps* m,const char* type){
  xmlNodePtr nc1,nc2,nc3;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=NULL;
  if(e!=NULL)
    tmp=e->content;
  else
    tmp=m->content;
#ifdef DEBUG
  dumpMap(tmp);
  dumpElements(e);
#endif
  nc2=xmlNewNode(ns_ows, BAD_CAST "Identifier");
  if(e!=NULL)
    nc3=xmlNewText(BAD_CAST e->name);
  else
    nc3=xmlNewText(BAD_CAST m->name);
  xmlAddChild(nc2,nc3);
  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);
  // Extract Title required to be first element in the ZCFG file !
  bool isTitle=true;
  if(e!=NULL)
    tmp=getMap(e->content,"Title");
  else
    tmp=getMap(m->content,"Title");
  
  if(tmp!=NULL){
    nc2=xmlNewNode(ns_ows, BAD_CAST tmp->name);
    nc3=xmlNewText(BAD_CAST _ss(tmp->value));
    xmlAddChild(nc2,nc3);  
    xmlAddChild(nc1,nc2);
  }

  if(e!=NULL)
    tmp=getMap(e->content,"Abstract");
  else
    tmp=getMap(m->content,"Abstract");
  if(tmp!=NULL){
    nc2=xmlNewNode(ns_ows, BAD_CAST tmp->name);
    nc3=xmlNewText(BAD_CAST _ss(tmp->value));
    xmlAddChild(nc2,nc3);  
    xmlAddChild(nc1,nc2);
    xmlAddChild(nc,nc1);
  }

  /**
   * IO type Reference or full Data ?
   */
#ifdef DEBUG
  fprintf(stderr,"FORMAT %s %s\n",e->format,e->format);
#endif
  map *tmpMap=getMap(m->content,"Reference");
  if(tmpMap==NULL){
    nc2=xmlNewNode(ns_wps, BAD_CAST "Data");
    if(e!=NULL){
      if(strncasecmp(e->format,"LiteralOutput",strlen(e->format))==0)
	nc3=xmlNewNode(ns_wps, BAD_CAST "LiteralData");
      else
	if(strncasecmp(e->format,"ComplexOutput",strlen(e->format))==0)
	  nc3=xmlNewNode(ns_wps, BAD_CAST "ComplexData");
	else if(strncasecmp(e->format,"BoundingBoxOutput",strlen(e->format))==0)
	  nc3=xmlNewNode(ns_wps, BAD_CAST "BoundingBoxData");
	else
	  nc3=xmlNewNode(ns_wps, BAD_CAST e->format);
    }
    else{
      map* tmpV=getMapFromMaps(m,"format","value");
      if(tmpV!=NULL)
	nc3=xmlNewNode(ns_wps, BAD_CAST tmpV->value);
      else
	nc3=xmlNewNode(ns_wps, BAD_CAST "LitteralData");
    } 
    tmp=m->content;
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,"mimeType")==0 ||
	 strcasecmp(tmp->name,"encoding")==0 ||
	 strcasecmp(tmp->name,"schema")==0 ||
	 strcasecmp(tmp->name,"datatype")==0 ||
	 strcasecmp(tmp->name,"uom")==0)
	xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
      tmp=tmp->next;
      xmlAddChild(nc2,nc3);
    }
    if(e!=NULL && e->format!=NULL && strcasecmp(e->format,"BoundingBoxData")==0){
      map* bb=getMap(m->content,"value");
      if(bb!=NULL){
	map* tmpRes=parseBoundingBox(bb->value);
	printBoundingBox(ns_ows,nc3,tmpRes);
	freeMap(&tmpRes);
	free(tmpRes);
      }
    }else{
      if(e!=NULL)
	tmp=getMap(e->defaults->content,"mimeType");
      else
	tmp=NULL;
      map* tmp1=getMap(m->content,"encoding");
      map* tmp2=getMap(m->content,"mimeType");
      map* toto=getMap(m->content,"value");
      if((tmp1!=NULL && strncmp(tmp1->value,"base64",6)==0)
	 || (tmp2!=NULL && (strncmp(tmp2->value,"image/",6)==0 ||
			    (strncmp(tmp2->value,"application/",12)==0) &&
			    strncmp(tmp2->value,"application/json",16)!=0&&
			    strncmp(tmp2->value,"application/vnd.google-earth.kml",32)!=0)
	     )) {
	map* rs=getMap(m->content,"size");
	bool isSized=true;
	if(rs==NULL){
	  char tmp1[1024];
	  sprintf(tmp1,"%d",strlen(toto->value));
	  rs=createMap("size",tmp1);
	  isSized=false;
	}

	xmlAddChild(nc3,xmlNewText(BAD_CAST base64(toto->value, atoi(rs->value))));
	if(!isSized){
	  freeMap(&rs);
	  free(rs);
	}
      }
      else if(tmp2!=NULL){
	if(strncmp(tmp2->value,"text/js",7)==0 ||
	   strncmp(tmp2->value,"application/json",16)==0)
	  xmlAddChild(nc3,xmlNewCDataBlock(doc,BAD_CAST toto->value,strlen(toto->value)));
	else{
	  if(strncmp(tmp2->value,"text/xml",8)==0 ||
	     strncmp(tmp2->value,"application/vnd.google-earth.kml",32)!=0){
	    xmlDocPtr doc =
	      xmlParseMemory(BAD_CAST toto->value,strlen(BAD_CAST toto->value));
	    xmlNodePtr ir = xmlDocGetRootElement(doc);
	    xmlAddChild(nc3,ir);
	  }
	  else
	    xmlAddChild(nc3,xmlNewText(BAD_CAST toto->value));
	}
	xmlAddChild(nc2,nc3);
      }
      else
	xmlAddChild(nc3,xmlNewText(BAD_CAST toto->value));
    }
  }
  else{
    nc3=nc2=xmlNewNode(ns_wps, BAD_CAST "Reference");
    if(strcasecmp(type,"Output")==0)
      xmlNewProp(nc3,BAD_CAST "href",BAD_CAST tmpMap->value);
    else
      xmlNewNsProp(nc3,ns_xlink,BAD_CAST "href",BAD_CAST tmpMap->value);
    tmp=m->content;
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,"mimeType")==0 ||
	 strcasecmp(tmp->name,"encoding")==0 ||
	 strcasecmp(tmp->name,"schema")==0 ||
	 strcasecmp(tmp->name,"datatype")==0 ||
	 strcasecmp(tmp->name,"uom")==0)
	xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
      tmp=tmp->next;
      xmlAddChild(nc2,nc3);
    }
  }

  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);

}

void printDescription(xmlNodePtr root,xmlNsPtr ns_ows,const char* identifier,map* amap){
  xmlNodePtr nc2 = xmlNewNode(ns_ows, BAD_CAST "Identifier");
  xmlAddChild(nc2,xmlNewText(BAD_CAST identifier));
  xmlAddChild(root,nc2);
  map* tmp=amap;
  char *tmp2[2];
  tmp2[0]="Title";
  tmp2[1]="Abstract";
  int j=0;
  for(j=0;j<2;j++){
    map* tmp1=getMap(tmp,tmp2[j]);
    if(tmp1!=NULL){
      nc2 = xmlNewNode(ns_ows, BAD_CAST tmp2[j]);
      xmlAddChild(nc2,xmlNewText(BAD_CAST _ss(tmp1->value)));
      xmlAddChild(root,nc2);
    }
  }
}

char* getEncoding(maps* m){
  if(m!=NULL){
    map* tmp=getMap(m->content,"encoding");
    if(tmp!=NULL){
      return tmp->value;
    }
    else
      return "UTF-8";
  }
  else
    return "UTF-8";  
}

char* getVersion(maps* m){
  if(m!=NULL){
    map* tmp=getMap(m->content,"version");
    if(tmp!=NULL){
      return tmp->value;
    }
    else
      return "1.0.0";
  }
  else
    return "1.0.0";
}

void printExceptionReportResponse(maps* m,map* s){
  int buffersize;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  xmlNodePtr n;

  doc = xmlNewDoc(BAD_CAST "1.0");
  maps* tmpMap=getMaps(m,"main");
  char *encoding=getEncoding(tmpMap);
  if(m!=NULL){
    map *tmpSid=getMapFromMaps(m,"lenv","sid");
    if(tmpSid!=NULL){
      if( getpid()==atoi(tmpSid->value) )
	printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
    }
    else
      printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
  }else
    printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
  n=createExceptionReportNode(m,s,1);
  xmlDocSetRootElement(doc, n);
  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  printf("%s",xmlbuff);
  fflush(stdout);
  xmlFreeDoc(doc);
  xmlFree(xmlbuff);
  xmlCleanupParser();
  zooXmlCleanupNs();
}

xmlNodePtr createExceptionReportNode(maps* m,map* s,int use_ns){
  
  int buffersize;
  xmlChar *xmlbuff;
  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr n,nc,nc1,nc2;

  maps* tmpMap=getMaps(m,"main");

  int nsid=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns=usedNs[nsid];
  n = xmlNewNode(ns, BAD_CAST "ExceptionReport");

  if(use_ns==1){
    ns_ows=xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
    int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
    ns_xsi=usedNs[xsiId];
    int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
    ns_xlink=usedNs[xlinkId];
    xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.opengis.net/ows/1.1 http://schemas.opengis.net/ows/1.1.0/owsExceptionReport.xsd");
  }
  addLangAttr(n,m);
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.1.0");
  
  nc = xmlNewNode(ns, BAD_CAST "Exception");

  map* tmp=getMap(s,"code");
  if(tmp!=NULL)
    xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST tmp->value);
  else
    xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST "NoApplicableCode");

  tmp=getMap(s,"text");
  nc1 = xmlNewNode(ns, BAD_CAST "ExceptionText");
  nc2=NULL;
  if(tmp!=NULL){
    xmlNodeSetContent(nc1, BAD_CAST tmp->value);
  }
  else{
    xmlNodeSetContent(nc1, BAD_CAST _("No debug message available"));
  }
  xmlAddChild(nc,nc1);
  xmlAddChild(n,nc);
  return n;
}


void outputResponse(service* s,maps* request_inputs,maps* request_outputs,
		    map* request_inputs1,int cpid,maps* m,int res){
#ifdef DEBUG
  dumpMaps(request_inputs);
  dumpMaps(request_outputs);
  fprintf(stderr,"printProcessResponse\n");
#endif
  map* toto=getMap(request_inputs1,"RawDataOutput");
  int asRaw=0;
  if(toto!=NULL)
    asRaw=1;
  
  map *_tmp=getMapFromMaps(m,"lenv","cookie");
  if(_tmp!=NULL){
    printf("Set-Cookie: %s\r\n",_tmp->value);
    maps *tmpSess=getMaps(m,"senv");
    if(tmpSess!=NULL){
      char session_file_path[1024];
      map *tmpPath=getMapFromMaps(m,"main","sessPath");
      if(tmpPath==NULL)
	tmpPath=getMapFromMaps(m,"main","tmpPath");
      char *tmp1=strtok(_tmp->value,";");
      if(tmp1!=NULL)
	sprintf(session_file_path,"%s/sess_%s.cfg",tmpPath->value,strstr(tmp1,"=")+1);
      else
	sprintf(session_file_path,"%s/sess_%s.cfg",tmpPath->value,strstr(_tmp->value,"=")+1);
      dumpMapsToFile(tmpSess,session_file_path);
    }
  }
  if(asRaw==0){
#ifdef DEBUG
    fprintf(stderr,"REQUEST_OUTPUTS FINAL\n");
    dumpMaps(request_outputs);
#endif
    maps* tmpI=request_outputs;
    while(tmpI!=NULL){
      toto=getMap(tmpI->content,"asReference");
      if(toto!=NULL && strcasecmp(toto->value,"true")==0){
	elements* in=getElements(s->outputs,tmpI->name);
	char *format=NULL;
	if(in!=NULL){
	  format=strdup(in->format);
	}else
	  format=strdup("LiteralData");
	if(strcasecmp(format,"BoundingBoxData")==0){
	  addToMap(tmpI->content,"extension","xml");
	  addToMap(tmpI->content,"mimeType","text/xml");
	  addToMap(tmpI->content,"encoding","UTF-8");
	  addToMap(tmpI->content,"schema","http://schemas.opengis.net/ows/1.1.0/owsCommon.xsd");
	}
	map *ext=getMap(tmpI->content,"extension");
	map *tmp1=getMapFromMaps(m,"main","tmpPath");
	char *file_name;
	bool hasExt=true;
	if(ext==NULL){
	  // We can fallback to a default list of supported formats using
	  // mimeType information if present here. Maybe we can add more formats
	  // here.
	  // If mimeType was not found, we then set txt as the default extension.
	  map* mtype=getMap(tmpI->content,"mimeType");
	  if(mtype!=NULL){
	    if(strcasecmp(mtype->value,"text/xml")==0)
	      ext=createMap("extension","xml");
	    else if(strcasecmp(mtype->value,"application/json")==0)
	      ext=createMap("extension","js");
	    else if(strncmp(mtype->value,"application/vnd.google-earth.kml",32)!=0)
	      ext=createMap("extension","kml");
	    else
	      ext=createMap("extension","txt");
	  }
	  else
	    ext=createMap("extension","txt");
	  hasExt=false;
	}
	file_name=(char*)malloc((strlen(tmp1->value)+strlen(s->name)+strlen(ext->value)+strlen(tmpI->name)+13)*sizeof(char));
	sprintf(file_name,"%s/%s_%s_%i.%s",tmp1->value,s->name,tmpI->name,cpid+100000,ext->value);
	FILE *ofile=fopen(file_name,"w");
	if(ofile==NULL)
	  fprintf(stderr,"Unable to create file on disk implying segfault ! \n");
	map *tmp2=getMapFromMaps(m,"main","tmpUrl");
	map *tmp3=getMapFromMaps(m,"main","serverAddress");
	char *file_url;
	file_url=(char*)malloc((strlen(tmp3->value)+strlen(tmp2->value)+strlen(s->name)+strlen(ext->value)+strlen(tmpI->name)+13)*sizeof(char));
	sprintf(file_url,"%s/%s/%s_%s_%i.%s",tmp3->value,tmp2->value,s->name,tmpI->name,cpid+100000,ext->value);
	addToMap(tmpI->content,"Reference",file_url);
	if(hasExt!=true){
	  freeMap(&ext);
	  free(ext);
	}
	toto=getMap(tmpI->content,"value");
	if(strcasecmp(format,"BoundingBoxData")!=0){
	  map* size=getMap(tmpI->content,"size");
	  if(size!=NULL && toto!=NULL)
	    fwrite(toto->value,1,atoi(size->value)*sizeof(char),ofile);
	  else
	    if(toto!=NULL && toto->value!=NULL)
	      fwrite(toto->value,1,strlen(toto->value)*sizeof(char),ofile);
	}else{
	  printBoundingBoxDocument(m,tmpI,ofile);
	}
	free(format);
	fclose(ofile);
	free(file_name);
	free(file_url);	
      }
      tmpI=tmpI->next;
    }
    map *r_inputs=getMap(s->content,"serviceProvider");
#ifdef DEBUG
    fprintf(stderr,"SERVICE : %s\n",r_inputs->value);
    dumpMaps(m);
#endif
    printProcessResponse(m,request_inputs1,cpid,
			 s,r_inputs->value,res,
			 request_inputs,
			 request_outputs);
  }
  else
    if(res!=SERVICE_FAILED){
      /**
       * We get the requested output or fallback to the first one if the 
       * requested one is not present in the resulting outputs maps.
       */
      maps* tmpI=NULL;
      map* tmpIV=getMap(request_inputs1,"RawDataOutput");
      if(tmpIV!=NULL){
	tmpI=getMaps(request_outputs,tmpIV->value);
      }
      if(tmpI==NULL)
	tmpI=request_outputs;
      elements* e=getElements(s->outputs,tmpI->name);
      if(e!=NULL && strcasecmp(e->format,"BoundingBoxData")==0){
	printBoundingBoxDocument(m,tmpI,NULL);
      }else{
	toto=getMap(tmpI->content,"value");
	if(toto==NULL){
	  char tmpMsg[1024];
	  sprintf(tmpMsg,_("Wrong RawDataOutput parameter, unable to fetch any result for the name your provided : \"%s\"."),tmpI->name);
	  map * errormap = createMap("text",tmpMsg);
	  addToMap(errormap,"code", "InvalidParameterValue");
	  printExceptionReportResponse(m,errormap);
	  freeMap(&errormap);
	  free(errormap);
	  return;
	}
	char mime[1024];
	map* mi=getMap(tmpI->content,"mimeType");
#ifdef DEBUG
	fprintf(stderr,"SERVICE OUTPUTS\n");
	dumpMaps(request_outputs);
	fprintf(stderr,"SERVICE OUTPUTS\n");
#endif
	map* en=getMap(tmpI->content,"encoding");
	if(mi!=NULL && en!=NULL)
	  sprintf(mime,
		  "Content-Type: %s; charset=%s\r\nStatus: 200 OK\r\n\r\n",
		  mi->value,en->value);
	else
	  if(mi!=NULL)
	    sprintf(mime,
		    "Content-Type: %s; charset=UTF-8\r\nStatus: 200 OK\r\n\r\n",
		    mi->value);
	  else
	    sprintf(mime,"Content-Type: text/plain; charset=utf-8\r\nStatus: 200 OK\r\n\r\n");
	printf("%s",mime);
	if(mi!=NULL && strncmp(mi->value,"image",5)==0){
	  map* rs=getMapFromMaps(tmpI,tmpI->name,"size");
	  fwrite(toto->value,atoi(rs->value),1,stdout);
	}
	else
	  printf("%s",toto->value);
#ifdef DEBUG
	dumpMap(toto);
#endif
      }
    }else{
      char tmp[1024];
      map * errormap;
      map *lenv;
      lenv=getMapFromMaps(m,"lenv","message");
      if(lenv!=NULL)
	sprintf(tmp,_("Unable to run the Service. The message returned back by the Service was the following : %s"),lenv->value);
      else
	sprintf(tmp,_("Unable to run the Service. No more information was returned back by the Service."));
      errormap = createMap("text",tmp);      
      addToMap(errormap,"code", "InternalError");
      printExceptionReportResponse(m,errormap);
      freeMap(&errormap);
      free(errormap);
    }
}

char *base64(const char *input, int length)
{
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, length);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  char *buff = (char *)malloc((bptr->length)*sizeof(char));
  memcpy(buff, bptr->data, bptr->length-1);
  buff[bptr->length-1] = 0;

  BIO_free_all(b64);

  return buff;
}

char *base64d(const char *input, int length,int* red)
{
  BIO *b64, *bmem;

  char *buffer = (char *)malloc(length);
  if(buffer){
    memset(buffer, 0, length);
    b64 = BIO_new(BIO_f_base64());
    if(b64){
      bmem = BIO_new_mem_buf((unsigned char*)input,length);
      bmem = BIO_push(b64, bmem);
      *red=BIO_read(bmem, buffer, length);
      buffer[length-1]=0;
      BIO_free_all(bmem);
    }
  }
  return buffer;
}

void ensureDecodedBase64(maps **in){
  maps* cursor=*in;
  while(cursor!=NULL){
    map *tmp=getMap(cursor->content,"encoding");
    if(tmp!=NULL && strncasecmp(tmp->value,"base64",6)==0){
      tmp=getMap(cursor->content,"value");
      addToMap(cursor->content,"base64_value",tmp->value);
      int size=0;
      char *s=strdup(tmp->value);
      free(tmp->value);
      tmp->value=base64d(s,strlen(s),&size);
      free(s);
      char sizes[1024];
      sprintf(sizes,"%d",size);
      addToMap(cursor->content,"size",sizes);
    }
    cursor=cursor->next;
  }
}

char* addDefaultValues(maps** out,elements* in,maps* m,int type){
  elements* tmpInputs=in;
  maps* out1=*out;
  if(type==1){
    while(out1!=NULL){
      if(getElements(in,out1->name)==NULL)
	return out1->name;
      out1=out1->next;
    }
    out1=*out;
  }
  while(tmpInputs!=NULL){
    maps *tmpMaps=getMaps(out1,tmpInputs->name);
    if(tmpMaps==NULL){
      maps* tmpMaps2=(maps*)malloc(MAPS_SIZE);
      tmpMaps2->name=strdup(tmpInputs->name);
      tmpMaps2->content=NULL;
      tmpMaps2->next=NULL;
      
      if(type==0){
	map* tmpMapMinO=getMap(tmpInputs->content,"minOccurs");
	if(tmpMapMinO!=NULL)
	  if(atoi(tmpMapMinO->value)>=1){
	    freeMaps(&tmpMaps2);
	    free(tmpMaps2);
	    return tmpInputs->name;
	  }
	  else{
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("minOccurs",tmpMapMinO->value);
	    else
	      addToMap(tmpMaps2->content,"minOccurs",tmpMapMinO->value);
	  }
	map* tmpMaxO=getMap(tmpInputs->content,"maxOccurs");
	if(tmpMaxO!=NULL)
	  if(tmpMaps2->content==NULL)
	    tmpMaps2->content=createMap("maxOccurs",tmpMaxO->value);
	  else
	    addToMap(tmpMaps2->content,"maxOccurs",tmpMaxO->value);
      }

      iotype* tmpIoType=tmpInputs->defaults;
      if(tmpIoType!=NULL){
	map* tmpm=tmpIoType->content;
	while(tmpm!=NULL){
	  if(tmpMaps2->content==NULL)
	    tmpMaps2->content=createMap(tmpm->name,tmpm->value);
	  else
	    addToMap(tmpMaps2->content,tmpm->name,tmpm->value);
	  tmpm=tmpm->next;
	}
      }
      addToMap(tmpMaps2->content,"inRequest","false");
      if(type==0){
	map *tmpMap=getMap(tmpMaps2->content,"value");
	if(tmpMap==NULL)
	  addToMap(tmpMaps2->content,"value","NULL");
      }
      if(out1==NULL){
	*out=dupMaps(&tmpMaps2);
	out1=*out;
      }
      else
	addMapsToMaps(&out1,tmpMaps2);
      freeMap(&tmpMaps2->content);
      free(tmpMaps2->content);
      tmpMaps2->content=NULL;
      freeMaps(&tmpMaps2);
      free(tmpMaps2);
      tmpMaps2=NULL;
    }
    else{
      iotype* tmpIoType=getIoTypeFromElement(tmpInputs,tmpInputs->name,
					     tmpMaps->content);
      if(type==0) {
	/**
	 * In case of an Input maps, then add the minOccurs and maxOccurs to the
	 * content map.
	 */
	map* tmpMap1=getMap(tmpInputs->content,"minOccurs");
	if(tmpMap1!=NULL){
	  if(tmpMaps->content==NULL)
	    tmpMaps->content=createMap("minOccurs",tmpMap1->value);
	  else
	    addToMap(tmpMaps->content,"minOccurs",tmpMap1->value);
	}
	map* tmpMaxO=getMap(tmpInputs->content,"maxOccurs");
	if(tmpMaxO!=NULL){
	  if(tmpMaps->content==NULL)
	    tmpMaps->content=createMap("maxOccurs",tmpMap1->value);
	  else
	    addToMap(tmpMaps->content,"maxOccurs",tmpMap1->value);
	}
	/**
	 * Parsing BoundingBoxData, fill the following map and then add it to
	 * the content map of the Input maps: 
	 * lowerCorner, upperCorner, srs and dimensions
	 * cf. parseBoundingBox
	 */
	if(strcasecmp(tmpInputs->format,"BoundingBoxData")==0){
	  maps* tmpI=getMaps(*out,tmpInputs->name);
	  if(tmpI!=NULL){
	    map* tmpV=getMap(tmpI->content,"value");
	    if(tmpV!=NULL){
	      char *tmpVS=strdup(tmpV->value);
	      map* tmp=parseBoundingBox(tmpVS);
	      free(tmpVS);
	      map* tmpC=tmp;
	      while(tmpC!=NULL){
		addToMap(tmpMaps->content,tmpC->name,tmpC->value);
		tmpC=tmpC->next;
	      }
	      freeMap(&tmp);
	      free(tmp);
	    }
	  }
	}
      }

      if(tmpIoType!=NULL){
	map* tmpContent=tmpIoType->content;
	map* cval=NULL;

	while(tmpContent!=NULL){
	  if((cval=getMap(tmpMaps->content,tmpContent->name))==NULL){
#ifdef DEBUG
	    fprintf(stderr,"addDefaultValues %s => %s\n",tmpContent->name,tmpContent->value);
#endif
	    if(tmpMaps->content==NULL)
	      tmpMaps->content=createMap(tmpContent->name,tmpContent->value);
	    else
	      addToMap(tmpMaps->content,tmpContent->name,tmpContent->value);
	  }
	  tmpContent=tmpContent->next;
	}
      }
      if(tmpMaps->content==NULL)
	tmpMaps->content=createMap("inRequest","true");
      else
	addToMap(tmpMaps->content,"inRequest","true");

    }
    tmpInputs=tmpInputs->next;
  }
  return "";
}

/**
 * parseBoundingBox : parse a BoundingBox string
 *
 * OGC 06-121r3 : 10.2 Bounding box
 *
 * value is provided as : lowerCorner,upperCorner,crs,dimension
 * exemple : 189000,834000,285000,962000,urn:ogc:def:crs:OGC:1.3:CRS84
 *
 * Need to create a map to store boundingbox informations :
 *  - lowerCorner : double,double (minimum within this bounding box)
 *  - upperCorner : double,double (maximum within this bounding box)
 *  - crs : URI (Reference to definition of the CRS)
 *  - dimensions : int 
 * 
 * Note : support only 2D bounding box.
 */
map* parseBoundingBox(const char* value){
  map *res=NULL;
  if(value!=NULL){
    char *cv,*cvp;
    cv=strtok_r((char*) value,",",&cvp);
    int cnt=0;
    int icnt=0;
    char *currentValue=NULL;
    while(cv){
      if(cnt<2)
	if(currentValue!=NULL){
	  char *finalValue=(char*)malloc((strlen(currentValue)+strlen(cv)+1)*sizeof(char));
	  sprintf(finalValue,"%s%s",currentValue,cv);
	  switch(cnt){
	  case 0:
	    res=createMap("lowerCorner",finalValue);
	    break;
	  case 1:
	    addToMap(res,"upperCorner",finalValue);
	    icnt=-1;
	    break;
	  }
	  cnt++;
	  free(currentValue);
	  currentValue=NULL;
	  free(finalValue);
	}
	else{
	  currentValue=(char*)malloc((strlen(cv)+2)*sizeof(char));
	  sprintf(currentValue,"%s ",cv);
	}
      else
	if(cnt==2){
	  addToMap(res,"crs",cv);
	  cnt++;
	}
	else
	  addToMap(res,"dimensions",cv);
      icnt++;
      cv=strtok_r(NULL,",",&cvp);
    }
  }
  return res;
}

/**
 * printBoundingBox : fill a BoundingBox node (ows:BoundingBox or 
 * wps:BoundingBoxData). Set crs and dimensions attributes, add 
 * Lower/UpperCorner nodes to a pre-existing XML node.
 */
void printBoundingBox(xmlNsPtr ns_ows,xmlNodePtr n,map* boundingbox){

  xmlNodePtr bb,lw,uc;

  map* tmp=getMap(boundingbox,"value");

  tmp=getMap(boundingbox,"lowerCorner");
  if(tmp!=NULL){
    lw=xmlNewNode(ns_ows,BAD_CAST "LowerCorner");
    xmlAddChild(lw,xmlNewText(BAD_CAST tmp->value));
  }

  tmp=getMap(boundingbox,"upperCorner");
  if(tmp!=NULL){
    uc=xmlNewNode(ns_ows,BAD_CAST "UpperCorner");
    xmlAddChild(uc,xmlNewText(BAD_CAST tmp->value));
  }

  tmp=getMap(boundingbox,"crs");
  if(tmp!=NULL)
    xmlNewProp(n,BAD_CAST "crs",BAD_CAST tmp->value);

  tmp=getMap(boundingbox,"dimensions");
  if(tmp!=NULL)
    xmlNewProp(n,BAD_CAST "dimensions",BAD_CAST tmp->value);

  xmlAddChild(n,lw);
  xmlAddChild(n,uc);

}

void printBoundingBoxDocument(maps* m,maps* boundingbox,FILE* file){
  if(file==NULL)
    rewind(stdout);
  xmlNodePtr n;
  xmlDocPtr doc;
  xmlNsPtr ns_ows,ns_xsi;
  xmlChar *xmlbuff;
  int buffersize;
  char *encoding=getEncoding(m);
  map *tmp;
  if(file==NULL){
    int pid=0;
    tmp=getMapFromMaps(m,"lenv","sid");
    if(tmp!=NULL)
      pid=atoi(tmp->value);
    if(pid==getpid()){
      printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
    }
    fflush(stdout);
  }

  doc = xmlNewDoc(BAD_CAST "1.0");
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  n = xmlNewNode(ns_ows, BAD_CAST "BoundingBox");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
  ns_xsi=usedNs[xsiId];
  xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.opengis.net/ows/1.1 http://schemas.opengis.net/ows/1.1.0/owsCommon.xsd");
  map *tmp1=getMap(boundingbox->content,"value");
  tmp=parseBoundingBox(tmp1->value);
  printBoundingBox(ns_ows,n,tmp);
  xmlDocSetRootElement(doc, n);

  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  if(file==NULL)
    printf("%s",xmlbuff);
  else{
    fprintf(file,"%s",xmlbuff);
  }

  if(tmp!=NULL){
    freeMap(&tmp);
    free(tmp);
  }
  xmlFree(xmlbuff);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  zooXmlCleanupNs();
  
}


unsigned char* getMd5(char* url){
  EVP_MD_CTX md5ctx;
  unsigned char* fresult=(char*)malloc((EVP_MAX_MD_SIZE+1)*sizeof(char));
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
 * Cache a file for a given request
 */
void addToCache(maps* conf,char* request,char* content,int length){
  map* tmp=getMapFromMaps(conf,"main","cacheDir");
  if(tmp!=NULL){
    unsigned char* md5str=getMd5(request);
    char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
    sprintf(fname,"%s/%s.zca",tmp->value,md5str);
#ifdef DEBUG
    fprintf(stderr,"Cache list : %s\n",fname);
    fflush(stderr);
#endif
    FILE* fo=fopen(fname,"w+");
    fwrite(content,sizeof(char),length,fo);
    fclose(fo);
    free(md5str);
    free(fname);
  }
}

char* isInCache(maps* conf,char* request){
  map* tmpM=getMapFromMaps(conf,"main","cacheDir");
  if(tmpM!=NULL){
    unsigned char* md5str=getMd5(request);
#ifdef DEBUG
    fprintf(stderr,"MD5STR : (%s)\n\n",md5str);
#endif
    char* fname=(char*)malloc(sizeof(char)*(strlen(tmpM->value)+38));
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
 * loadRemoteFile:
 * Try to load file from cache or download a remote file if not in cache
 */
void loadRemoteFile(maps* m,map* content,HINTERNET hInternet,char *url){
  HINTERNET res;
  char* fcontent;
  char* cached=isInCache(m,url);
  int fsize;
  if(cached!=NULL){
    struct stat f_status;
    int s=stat(cached, &f_status);
    if(s==0){
      fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(cached,"r");
      fread(fcontent,sizeof(char),f_status.st_size,f);
      fsize=f_status.st_size;
    }
  }else{
    res=InternetOpenUrl(hInternet,url,NULL,0,INTERNET_FLAG_NO_CACHE_WRITE,0);
    fcontent=(char*)calloc((res.nDataLen+1),sizeof(char));
    if(fcontent == NULL){
      return errorException(m, _("Unable to allocate memory."), "InternalError");
    }
    size_t dwRead;
    InternetReadFile(res, (LPVOID)fcontent, res.nDataLen, &dwRead);
    fcontent[res.nDataLen]=0;
    fsize=res.nDataLen;
  }
  map* tmpMap=getMapOrFill(content,"value","");
  free(tmpMap->value);
  tmpMap->value=(char*)malloc((fsize+1)*sizeof(char));
  memcpy(tmpMap->value,fcontent,(fsize)*sizeof(char)); 
  char ltmp1[256];
  sprintf(ltmp1,"%d",fsize);
  addToMap(content,"size",ltmp1);
  if(cached==NULL)
    addToCache(m,url,fcontent,fsize);
  dumpMap(content);
  free(fcontent);
  free(cached);
}

int errorException(maps *m, const char *message, const char *errorcode) 
{
  map* errormap = createMap("text", message);
  addToMap(errormap,"code", errorcode);
  printExceptionReportResponse(m,errormap);
  freeMap(&errormap);
  free(errormap);
  return -1;
}
