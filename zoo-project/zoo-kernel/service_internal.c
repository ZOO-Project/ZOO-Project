/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2015 GeoLabs SARL
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
#ifdef USE_MS
#include "service_internal_ms.h"
#else
#include "cpl_vsi.h"
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE -1
#endif

#ifndef WIN32
#include <dlfcn.h>
#endif

#define ERROR_MSG_MAX_LENGTH 1024

/**
 * Verify if a given language is listed in the lang list defined in the [main] 
 * section of the main.cfg file.
 * 
 * @param conf the map containing the settings from the main.cfg file
 * @param str the specific language
 * @return 1 if the specific language is listed, -1 in other case.
 */
int isValidLang(maps* conf,const char *str){
  map *tmpMap=getMapFromMaps(conf,"main","lang");
  char *tmp=zStrdup(tmpMap->value);
  char *pToken=strtok(tmp,",");
  int res=-1;
  while(pToken!=NULL){
    if(strcasecmp(str,pToken)==0){
      res=1;
      break;
    }
    pToken = strtok(NULL,",");
  }
  free(tmp);
  return res;
}

/**
 * Print the HTTP headers based on a map.
 * 
 * @param m the map containing the headers informations
 */
void printHeaders(maps* m){
  maps *_tmp=getMaps(m,"headers");
  if(_tmp!=NULL){
    map* _tmp1=_tmp->content;
    while(_tmp1!=NULL){
      printf("%s: %s\r\n",_tmp1->name,_tmp1->value);
      _tmp1=_tmp1->next;
    }
  }
}

/**
 * Add a land attribute to a XML node
 *
 * @param n the XML node to add the attribute
 * @param m the map containing the language key to add as xml:lang
 */
void addLangAttr(xmlNodePtr n,maps *m){
  map *tmpLmap=getMapFromMaps(m,"main","language");
  if(tmpLmap!=NULL)
    xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST tmpLmap->value);
  else
    xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST "en-US");
}

/**
 * Converts a hex character to its integer value 
 *
 * @param ch the char to convert
 * @return the converted char 
 */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/**
 * Converts an integer value to its hec character 
 *
 * @param code the char to convert
 * @return the converted char 
 */
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/**
 * Get the ongoing status of a running service 
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (MESSAGE|POURCENTAGE)
 */
char* _getStatus(maps* conf,int pid){
  char lid[1024];
  sprintf(lid,"%d",pid);
  setMapInMaps(conf,"lenv","lid",lid);
  semid lockid=getShmLockId(conf,1);
  if(
#ifdef WIN32
     lockid==NULL
#else
     lockid<0
#endif
     ){
	char* tmp = (char*) malloc(3*sizeof(char));
    sprintf(tmp,"%d",ZOO_LOCK_CREATE_FAILED);
    return tmp;
  }
  if(lockShm(lockid)<0){
    fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
    fflush(stderr);    
	char* tmp = (char*) malloc(3*sizeof(char));
    sprintf(tmp,"%d",ZOO_LOCK_ACQUIRE_FAILED);
    return tmp;
  }
  char *tmp=getStatus(pid);
  unlockShm(lockid);
  if(tmp==NULL || strncmp(tmp,"-1",2)==0){
    removeShmLock(conf,1);
  }
  return tmp;
}

#ifdef WIN32

#include <windows.h>
#include <fcgi_stdio.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define SHMEMSIZE 4096

size_t getKeyValue(maps* conf, char* key, size_t length){
  if(conf==NULL) {
    strncpy(key, "700666", length);
    return strlen(key);
  }
  
  map *tmpMap=getMapFromMaps(conf,"lenv","lid");
  if(tmpMap==NULL)
	tmpMap=getMapFromMaps(conf,"lenv","usid");

  if(tmpMap!=NULL){
	snprintf(key, length, "zoo_sem_%s", tmpMap->value);      
  }
  else {
	strncpy(key, "-1", length);  
  }
  return strlen(key);
}


semid getShmLockId(maps* conf, int nsems){
    semid sem_id;
	char key[MAX_PATH];
	getKeyValue(conf, key, MAX_PATH);
    
    sem_id = CreateSemaphore( NULL, nsems, nsems+1, key);
    if(sem_id==NULL){

#ifdef DEBUG
      fprintf(stderr,"Semaphore failed to create: %s\n", getLastErrorMessage());
#endif
      return NULL;
    }
#ifdef DEBUG
    fprintf(stderr,"%s Accessed !\n",key);
#endif

    return sem_id;
}

int removeShmLock(maps* conf, int nsems){
  semid sem_id=getShmLockId(conf,1);
  if (CloseHandle(sem_id) == 0) {
    fprintf(stderr,"Unable to remove semaphore: %s\n", getLastErrorMessage());
    return -1;
  }
#ifdef DEBUG
  fprintf(stderr,"%d Removed !\n",sem_id);
#endif
  return 0;
}

int lockShm(semid id){
  DWORD dwWaitResult=WaitForSingleObject(id,INFINITE);
  switch (dwWaitResult){
    case WAIT_OBJECT_0:
      return 0;
      break;
    case WAIT_TIMEOUT:
      return -1;
      break;
    default:
      return -2;
      break;
  }
  return 0;
}

int unlockShm(semid id){
  if(!ReleaseSemaphore(id,1,NULL)){
    return -1;
  }
  return 0;
}

static LPVOID lpvMemG = NULL;      // pointer to shared memory
static HANDLE hMapObjectG = NULL;  // handle to file mapping

int _updateStatus(maps *conf){
  LPWSTR lpszTmp;
  BOOL fInit;
  char *final_string=NULL;
  char *s=NULL;
  map *tmpMap1;
  map *tmpMap=getMapFromMaps(conf,"lenv","usid");
  semid lockid=getShmLockId(conf,1);
  if(lockid==NULL){
#ifdef DEBUG
    fprintf(stderr,"Unable to create semaphore on line %d!! \n",__LINE__);
#endif
    return ZOO_LOCK_CREATE_FAILED;
  }
  if(lockShm(lockid)<0){
#ifdef DEBUG
    fprintf(stderr,"Unable to create semaphore on line %d!! \n",__LINE__);
#endif
    return ZOO_LOCK_ACQUIRE_FAILED;
  }
  
  if(hMapObjectG==NULL)
    hMapObjectG = CreateFileMapping( 
				    INVALID_HANDLE_VALUE,   // use paging file
				    NULL,                   // default security attributes
				    PAGE_READWRITE,         // read/write access
				    0,                      // size: high 32-bits
				    SHMEMSIZE,              // size: low 32-bits
				    TEXT(tmpMap->value));   // name of map object
  if (hMapObjectG == NULL){
#ifdef DEBUG
    fprintf(stderr,"Unable to create shared memory segment: %s\n", getLastErrorMessage());
#endif
    return -2;
  }
  fInit = (GetLastError() != ERROR_ALREADY_EXISTS); 
  if(lpvMemG==NULL)
    lpvMemG = MapViewOfFile( 
			    hMapObjectG,     // object to map view of
			    FILE_MAP_WRITE, // read/write access
			    0,              // high offset:  map from
			    0,              // low offset:   beginning
			    0);             // default: map entire file
  if (lpvMemG == NULL){
#ifdef DEBUG
    fprintf(stderr,"Unable to create or access the shared memory segment %s !! \n",tmpMap->value);
#endif
    return -1;
  } 
  memset(lpvMemG, '\0', SHMEMSIZE);
  tmpMap=getMapFromMaps(conf,"lenv","status");
  tmpMap1=NULL;
  tmpMap1=getMapFromMaps(conf,"lenv","message");
  lpszTmp = (LPWSTR) lpvMemG;
  final_string=(char*)malloc((strlen(tmpMap1->value)+strlen(tmpMap->value)+2)*sizeof(char));
  sprintf(final_string,"%s|%s",tmpMap->value,tmpMap1->value);
  for(s=final_string;*s!='\0';*s++){
    *lpszTmp++ = *s;
  }
  *lpszTmp++ = '\0';
  free(final_string);
  unlockShm(lockid);
  return 0;
}

char* getStatus(int pid){
  char *lpszBuf=(char*) malloc(SHMEMSIZE*sizeof(char));
  int i=0;
  LPWSTR lpszTmp=NULL;
  LPVOID lpvMem = NULL;
  HANDLE hMapObject = NULL;
  BOOL fIgnore,fInit;
  char tmp[1024];
  sprintf(tmp,"%d",pid);
  if(hMapObject==NULL)
    hMapObject = CreateFileMapping( 
				   INVALID_HANDLE_VALUE,   // use paging file
				   NULL,                   // default security attributes
				   PAGE_READWRITE,         // read/write access
				   0,                      // size: high 32-bits
				   4096,                   // size: low 32-bits
				   TEXT(tmp));   // name of map object
  if (hMapObject == NULL){
#ifdef DEBUG
    fprintf(stderr,"ERROR on line %d\n",__LINE__);
#endif
    return "-1";
  }
  if((GetLastError() != ERROR_ALREADY_EXISTS)){
#ifdef DEBUG
    fprintf(stderr,"ERROR on line %d\n",__LINE__);
    fprintf(stderr,"READING STRING S %s\n", getLastErrorMessage());
#endif
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
  if (lpvMem == NULL){
#ifdef DEBUG
    fprintf(stderr,"READING STRING S %d\n",__LINE__);
    fprintf(stderr,"READING STRING S %s\n", getLastErrorMessage());
#endif
    return "-1"; 
  }
  lpszTmp = (LPWSTR) lpvMem;
  while (*lpszTmp){
    lpszBuf[i] = (char)*lpszTmp;
    *lpszTmp++; 
    lpszBuf[i+1] = '\0'; 
    i++;
  }
  return (char*)lpszBuf;
}

void unhandleStatus(maps *conf){
  BOOL fIgnore;
  fIgnore = UnmapViewOfFile(lpvMemG); 
  fIgnore = CloseHandle(hMapObjectG);
}

#else
/**
 * Number of time to try to access a semaphores set
 * @see getShmLockId
 */
#define MAX_RETRIES 10

#ifndef __APPLE__
union semun {
  int val;
  struct semid_ds *buf;
  ushort *array;
};
#endif

/**
 * Set in the pre-allocated key the zoo_sem_[SID] string 
 * where [SID] is the lid (if any) or usid value from the [lenv] section.
 *
 * @param conf the map containing the setting of the main.cfg file
 */
int getKeyValue(maps* conf){
  if(conf==NULL)
     return 700666;
  map *tmpMap=getMapFromMaps(conf,"lenv","lid");
  if(tmpMap==NULL)
    tmpMap=getMapFromMaps(conf,"lenv","usid");
  int key=-1;
  if(tmpMap!=NULL)
    key=atoi(tmpMap->value);
  return key;
}

/**
 * Try to create or access a semaphore set.
 *
 * @see getKeyValue
 * @param conf the map containing the setting of the main.cfg file
 * @param nsems number of semaphores
 * @return a semaphores set indentifier on success, -1 in other case
 */
int getShmLockId(maps* conf, int nsems){
    int i;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    semid sem_id;
    int key=getKeyValue(conf);
    
    sem_id = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);

    if (sem_id >= 0) { /* we got it first */
        sb.sem_op = 1; 
	sb.sem_flg = 0;
	arg.val=1;
        for(sb.sem_num = 0; sb.sem_num < nsems; sb.sem_num++) { 
            /* do a semop() to "free" the semaphores. */
            /* this sets the sem_otime field, as needed below. */
            if (semop(sem_id, &sb, 1) == -1) {
                int e = errno;
                semctl(sem_id, 0, IPC_RMID); /* clean up */
                errno = e;
                return -1; /* error, check errno */
            }
        }
    } else if (errno == EEXIST) { /* someone else got it first */
        int ready = 0;

        sem_id = semget(key, nsems, 0); /* get the id */
        if (sem_id < 0) return sem_id; /* error, check errno */

        /* wait for other process to initialize the semaphore: */
        arg.buf = &buf;
        for(i = 0; i < MAX_RETRIES && !ready; i++) {
            semctl(sem_id, nsems-1, IPC_STAT, arg);
            if (arg.buf->sem_otime != 0) {
#ifdef DEBUG
	      fprintf(stderr,"Semaphore acquired ...\n");
#endif
	      ready = 1;
            } else {
#ifdef DEBUG
	      fprintf(stderr,"Retry to access the semaphore later ...\n");
#endif
	      sleep(1);
            }
        }
	errno = ZOO_LOCK_ACQUIRE_FAILED;
        if (!ready) {
#ifdef DEBUG
	  fprintf(stderr,"Unable to access the semaphore ...\n");
#endif
	  errno = ETIME;
	  return -1;
        }
    } else {
        return sem_id; /* error, check errno */
    }
#ifdef DEBUG
    fprintf(stderr,"%d Created !\n",sem_id);
#endif
    return sem_id;
}

/**
 * Try to remove a semaphore set.
 *
 * @param conf the map containing the setting of the main.cfg file
 * @param nsems number of semaphores
 * @return 0 if the semaphore can be removed, -1 in other case.
 */
int removeShmLock(maps* conf, int nsems){
  union semun arg;
  int sem_id=getShmLockId(conf,nsems);
  if (semctl(sem_id, 0, IPC_RMID, arg) == -1) {
    perror("semctl");
    return -1;
  }
  return 0;
}

/**
 * Lock a semaphore set.
 *
 * @param id the semaphores set indetifier
 * @return 0 if the semaphore can be locked, -1 in other case.
 */
int lockShm(int id){
  struct sembuf sb;
  sb.sem_num = 0;
  sb.sem_op = -1;  /* set to allocate resource */
  sb.sem_flg = SEM_UNDO;
  if (semop(id, &sb, 1) == -1){
    perror("semop");
    return -1;
  }
  return 0;
}

/**
 * unLock a semaphore set.
 *
 * @param id the semaphores set indetifier
 * @return 0 if the semaphore can be locked, -1 in other case.
 */
int unlockShm(int id){
  struct sembuf sb;
  sb.sem_num = 0;
  sb.sem_op = 1;  /* free resource */
  sb.sem_flg = SEM_UNDO;
  if (semop(id, &sb, 1) == -1) {
    perror("semop");
    return -1;
  }
  return 0;
}

/**
 * Stop handling status repport.
 *
 * @param conf the map containing the setting of the main.cfg file
 */
void unhandleStatus(maps *conf){
  int shmid;
  key_t key;
  void *shm;
  struct shmid_ds shmids;
  map *tmpMap=getMapFromMaps(conf,"lenv","usid");
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

/**
 * Update the current of the running service.
 *
 * @see getKeyValue, getShmLockId, lockShm
 * @param conf the map containing the setting of the main.cfg file
 * @return 0 on success, -2 if shmget failed, -1 if shmat failed
 */
int _updateStatus(maps *conf){
  int shmid;
  char *shm,*s,*s1;
  map *tmpMap=NULL;
  key_t key=getKeyValue(conf);
  if(key!=-1){
    semid lockid=getShmLockId(conf,1);
    if(lockid<0)
      return ZOO_LOCK_CREATE_FAILED;
    if(lockShm(lockid)<0){
      return ZOO_LOCK_ACQUIRE_FAILED;
    }
    if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
#ifdef DEBUG
      fprintf(stderr,"shmget failed to create new Shared memory segment\n");
#endif
      unlockShm(lockid);
      return -2;
    }else{
      if ((shm = (char*) shmat(shmid, NULL, 0)) == (char *) -1) {
#ifdef DEBUG
	fprintf(stderr,"shmat failed to update value\n");
#endif
	unlockShm(lockid);
	return -1;
      }
      else{
	tmpMap=getMapFromMaps(conf,"lenv","status");
	s1=shm;
	for(s=tmpMap->value;*s!=NULL && *s!=0;s++){
	  *s1++=*s;
	}
	*s1++='|';
	tmpMap=getMapFromMaps(conf,"lenv","message");
	if(tmpMap!=NULL)
	  for(s=tmpMap->value;*s!=NULL && *s!=0;s++){
	    *s1++=*s;
	}
	*s1=NULL;
	shmdt((void *)shm);
	unlockShm(lockid);
      }
    }
  }
  return 0;
}

/**
 * Update the current of the running service.
 *
 * @see getKeyValue, getShmLockId, lockShm
 * @param pid the semaphores 
 * @return 0 on success, -2 if shmget failed, -1 if shmat failed
 */
char* getStatus(int pid){
  int shmid;
  key_t key;
  void *shm;
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
      char *ret=strdup((char*)shm);
      shmdt((void *)shm);
      return ret;
    }
  }
  return (char*)"-1";
}

#endif


/**
 * URLEncode an url
 *
 * @param str the url to encode
 * @return a url-encoded version of str
 * @warning be sure to free() the returned string after use
 */
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

/**
 * Decode an URLEncoded url
 *
 * @param str the URLEncoded url to decode
 * @return a url-decoded version of str
 * @warning be sure to free() the returned string after use
 */
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

/**
 * Replace the first letter by its upper case version in a new char array
 *
 * @param tmp the char*
 * @return a new char* with first letter in upper case
 * @warning be sure to free() the returned string after use
 */
char *zCapitalize1(char *tmp){
  char *res=zStrdup(tmp);
  if(res[0]>=97 && res[0]<=122)
    res[0]-=32;
  return res;
}

/**
 * Replace all letters by their upper case version in a new char array
 *
 * @param tmp the char*
 * @return a new char* with first letter in upper case
 * @warning be sure to free() the returned string after use
 */
char *zCapitalize(char *tmp){
  int i=0;
  char *res=zStrdup(tmp);
  for(i=0;i<strlen(res);i++)
    if(res[i]>=97 && res[i]<=122)
      res[i]-=32;
  return res;
}

/**
 * Search for an existing XML namespace in usedNS.
 * 
 * @param name the name of the XML namespace to search
 * @return the index of the XML namespace found or -1 if not found.
 */
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

/**
 * Add an XML namespace to the usedNS if it was not already used.
 * 
 * @param nr the xmlNodePtr to attach the XML namspace (can be NULL)
 * @param url the url of the XML namespace to add
 * @param name the name of the XML namespace to add
 * @return the index of the XML namespace added.
 */
int zooXmlAddNs(xmlNodePtr nr,const char* url,const char* name){
#ifdef DEBUG
  fprintf(stderr,"zooXmlAddNs %d %s \n",nbNs,name);
#endif
  int currId=-1;
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

/**
 * Free allocated memory to store used XML namespace.
 */
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

/**
 * Add a XML document to the iDocs.
 * 
 * @param value the string containing the XML document
 * @return the index of the XML document added.
 */
int zooXmlAddDoc(const char* value){
  int currId=0;
  nbDocs++;
  currId=nbDocs-1;
  iDocs[currId]=xmlParseMemory(value,strlen(value));
  return currId;
}

/**
 * Free allocated memort to store XML documents
 */
void zooXmlCleanupDocs(){
  int j;
  for(j=nbDocs-1;j>=0;j--){
    xmlFreeDoc(iDocs[j]);
  }
  nbDocs=0;
}

/**
 * Generate a SOAP Envelope node when required (if the isSoap key of the [main]
 * section is set to true).
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param n the node used as children of the generated soap:Envelope
 * @return the generated soap:Envelope (if isSoap=true) or the input node n 
 *  (when isSoap=false)
 */
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

/**
 * Generate a WPS header.
 * 
 * @param doc the document to add the header
 * @param m the conf maps containing the main.cfg settings
 * @param req the request type (GetCapabilities,DescribeProcess,Execute)
 * @param rname the root node name
 * @return the generated wps:rname xmlNodePtr (can be wps: Capabilities, 
 *  wps:ProcessDescriptions,wps:ExecuteResponse)
 */
xmlNodePtr printWPSHeader(xmlDocPtr doc,maps* m,const char* req,const char* rname){

  xmlNsPtr ns,ns_xsi;
  xmlNodePtr n;

  int wpsId=zooXmlAddNs(NULL,"http://schemas.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  n = xmlNewNode(ns, BAD_CAST rname);
  zooXmlAddNs(n,"http://www.opengis.net/ows/1.1","ows");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
  ns_xsi=usedNs[xsiId];
  
  char *tmp=(char*) malloc((86+strlen(req)+1)*sizeof(char));
  sprintf(tmp,"http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wps%s_response.xsd",req);
  xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST tmp);
  free(tmp);
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  addLangAttr(n,m);
  xmlNodePtr fn=soapEnvelope(m,n);
  xmlDocSetRootElement(doc, fn);
  return n;
}

/**
 * Generate a Capabilities header.
 * 
 * @param doc the document to add the header
 * @param m the conf maps containing the main.cfg settings
 * @return the generated wps:ProcessOfferings xmlNodePtr 
 */
xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr doc,maps* m){

  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr n,nc,nc1,nc2,nc3,nc4,nc5,nc6;
  n = printWPSHeader(doc,m,"GetCapabilities","Capabilities");
  maps* toto1=getMaps(m,"main");
  char tmp[256];

  int wpsId=zooXmlAddNs(NULL,"http://www.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  int xlinkId=zooXmlAddNs(NULL,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];

  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceIdentification");
  maps* tmp4=getMaps(m,"identification");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    const char *orderedFields[5];
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
    const char *tmpAddress[6];
    tmpAddress[0]="addressDeliveryPoint";
    tmpAddress[1]="addressCity";
    tmpAddress[2]="addressAdministrativeArea";
    tmpAddress[3]="addressPostalCode";
    tmpAddress[4]="addressCountry";
    tmpAddress[5]="addressElectronicMailAddress";
    const char *tmpPhone[2];
    tmpPhone[0]="phoneVoice";
    tmpPhone[1]="phoneFacsimile";
    const char *orderedFields[12];
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
		      char *tmp4=tmp2->name;
		      nc1 = xmlNewNode(ns_ows, BAD_CAST tmp4+5);
		      xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
		      xmlAddChild(nc5,nc1);
		    }
		}
		else 
		  if(strncmp(tmp2->name,"Address",7)==0){
		    int j;
		    for(j=0;j<6;j++)
		      if(strcasecmp(tmp2->name,tmpAddress[j])==0){
			char *tmp4=tmp2->name;
			nc1 = xmlNewNode(ns_ows, BAD_CAST tmp4+7);
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
      SERVICE_URL = strdup("not_defined");
  }
  else
    SERVICE_URL = strdup("not_defined");

  for(j=0;j<3;j++){
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Operation");
    xmlNewProp(nc1,BAD_CAST "name",BAD_CAST tmp2[j]);
    nc2 = xmlNewNode(ns_ows, BAD_CAST "DCP");
    nc3 = xmlNewNode(ns_ows, BAD_CAST "HTTP");
    nc4 = xmlNewNode(ns_ows, BAD_CAST "Get");
    sprintf(tmp,"%s",SERVICE_URL);
    xmlNewNsProp(nc4,ns_xlink,BAD_CAST "href",BAD_CAST tmp);
    xmlAddChild(nc3,nc4);
    nc4 = xmlNewNode(ns_ows, BAD_CAST "Post");
    xmlNewNsProp(nc4,ns_xlink,BAD_CAST "href",BAD_CAST tmp);
    xmlAddChild(nc3,nc4);
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
  
  free(SERVICE_URL);
  return nc;
}

/**
 * Add prefix to the service name.
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param level the map containing the level information
 * @param serv the service structure created from the zcfg file
 */
void addPrefix(maps* conf,map* level,service* serv){
  if(level!=NULL){
    char key[25];
    char* prefix=NULL;
    int clevel=atoi(level->value);
    int cl=0;
    for(cl=0;cl<clevel;cl++){
      sprintf(key,"sprefix_%d",cl);
      map* tmp2=getMapFromMaps(conf,"lenv",key);
      if(tmp2!=NULL){
	if(prefix==NULL)
	  prefix=zStrdup(tmp2->value);
	else{
	  int plen=strlen(prefix);
	  prefix=(char*)realloc(prefix,(plen+strlen(tmp2->value)+2)*sizeof(char));
	  memcpy(prefix+plen,tmp2->value,strlen(tmp2->value)*sizeof(char));
	  prefix[plen+strlen(tmp2->value)]=0;
	}
      }
    }
    if(prefix!=NULL){
      char* tmp0=strdup(serv->name);
      free(serv->name);
      serv->name=(char*)malloc((strlen(prefix)+strlen(tmp0)+1)*sizeof(char));
      sprintf(serv->name,"%s%s",prefix,tmp0);
      free(tmp0);
      free(prefix);
      prefix=NULL;
    }
  }
}

/**
 * Generate a wps:Process node for a servie and add it to a given node.
 * 
 * @param m the conf maps containing the main.cfg settings
 * @param nc the XML node to add the Process node
 * @param serv the service structure created from the zcfg file
 * @return the generated wps:ProcessOfferings xmlNodePtr 
 */
void printGetCapabilitiesForProcess(maps* m,xmlNodePtr nc,service* serv){
  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr n=NULL,nc1,nc2;
  /**
   * Initialize or get existing namspaces
   */
  int wpsId=zooXmlAddNs(NULL,"http://www.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];

  map* tmp1;
  if(serv->content!=NULL){
    nc1 = xmlNewNode(ns, BAD_CAST "Process");
    tmp1=getMap(serv->content,"processVersion");
    if(tmp1!=NULL)
      xmlNewNsProp(nc1,ns,BAD_CAST "processVersion",BAD_CAST tmp1->value);
    map* tmp3=getMapFromMaps(m,"lenv","level");
    addPrefix(m,tmp3,serv);
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

/**
 * Generate a ProcessDescription node for a servie and add it to a given node.
 * 
 * @param m the conf maps containing the main.cfg settings
 * @param nc the XML node to add the Process node
 * @param serv the servive structure created from the zcfg file
 * @return the generated wps:ProcessOfferings xmlNodePtr 
 */
void printDescribeProcessForProcess(maps* m,xmlNodePtr nc,service* serv){
  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr n,nc1;

  n=nc;
  
  int wpsId=zooXmlAddNs(NULL,"http://schemas.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(NULL,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];

  nc = xmlNewNode(NULL, BAD_CAST "ProcessDescription");
  const char *tmp4[3];
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
  
  tmp1=getMapFromMaps(m,"lenv","level");
  addPrefix(m,tmp1,serv);
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

  if(serv->inputs!=NULL){
    nc1 = xmlNewNode(NULL, BAD_CAST "DataInputs");
    elements* e=serv->inputs;
    printFullDescription(1,e,"Input",ns_ows,nc1);
    xmlAddChild(nc,nc1);
  }

  nc1 = xmlNewNode(NULL, BAD_CAST "ProcessOutputs");
  elements* e=serv->outputs;
  printFullDescription(0,e,"Output",ns_ows,nc1);
  xmlAddChild(nc,nc1);

  xmlAddChild(n,nc);

}

/**
 * Generate the required XML tree for the detailled metadata informations of 
 * inputs or outputs
 *
 * @param in 1 in case of inputs, 0 for outputs
 * @param elem the elements structure containing the metadata informations
 * @param type the name ("Input" or "Output") of the XML node to create
 * @param ns_ows the ows XML namespace
 * @param nc1 the XML node to use to add the created tree
 */
void printFullDescription(int in,elements *elem,const char* type,xmlNsPtr ns_ows,xmlNodePtr nc1){
  const char *orderedFields[13];
  orderedFields[0]="mimeType";
  orderedFields[1]="encoding";
  orderedFields[2]="schema";
  orderedFields[3]="dataType";
  orderedFields[4]="uom";
  orderedFields[5]="CRS";
  orderedFields[6]="value";
  orderedFields[7]="AllowedValues";
  orderedFields[8]="range";
  orderedFields[9]="rangeMin";
  orderedFields[10]="rangeMax";
  orderedFields[11]="rangeClosure";
  orderedFields[12]="rangeSpace";

  xmlNodePtr nc2,nc3,nc4,nc5,nc6,nc7,nc8,nc9;
  elements* e=elem;

  map* tmp1=NULL;
  while(e!=NULL){
    int default1=0;
    int isAnyValue=1;
    nc2 = xmlNewNode(NULL, BAD_CAST type);
    if(strncmp(type,"Input",5)==0){
      tmp1=getMap(e->content,"minOccurs");
      if(tmp1!=NULL){
	xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
      }else
	xmlNewProp(nc2,BAD_CAST "minOccurs",BAD_CAST "0");
      tmp1=getMap(e->content,"maxOccurs");
      if(tmp1!=NULL){
	if(strcasecmp(tmp1->value,"unbounded")!=0)
	  xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
	else
	  xmlNewProp(nc2,BAD_CAST "maxOccurs",BAD_CAST "1000");
      }else
	xmlNewProp(nc2,BAD_CAST "maxOccurs",BAD_CAST "1");
      if((tmp1=getMap(e->content,"maximumMegabytes"))!=NULL){
	xmlNewProp(nc2,BAD_CAST "maximumMegabytes",BAD_CAST tmp1->value);
      }
    }

    printDescription(nc2,ns_ows,e->name,e->content);

    /**
     * Build the (Literal/Complex/BoundingBox)Data node
     */
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

    iotype* _tmp0=NULL;
    iotype* _tmp=e->defaults;
    int datatype=0;
    bool hasUOM=false;
    bool hasUOM1=false;
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
	nc5 = xmlNewNode(NULL, BAD_CAST "Default");
      }
      else{
	nc4 = xmlNewNode(NULL, BAD_CAST "Default");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      
      tmp1=_tmp->content;

      if((tmp1=getMap(_tmp->content,"DataType"))!=NULL){
	nc8 = xmlNewNode(ns_ows, BAD_CAST "DataType");
	xmlAddChild(nc8,xmlNewText(BAD_CAST tmp1->value));
	char tmp[1024];
	sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	xmlNewNsProp(nc8,ns_ows,BAD_CAST "reference",BAD_CAST tmp);
	xmlAddChild(nc3,nc8);
	datatype=1;
      }
      
      if(strncmp(type,"Input",5)==0){

	if((tmp1=getMap(_tmp->content,"AllowedValues"))!=NULL){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST "AllowedValues");
	  char *token,*saveptr1;
	  token=strtok_r(tmp1->value,",",&saveptr1);
	  while(token!=NULL){
	    nc7 = xmlNewNode(ns_ows, BAD_CAST "Value");
	    char *tmps=strdup(token);
	    tmps[strlen(tmps)]=0;
	    xmlAddChild(nc7,xmlNewText(BAD_CAST tmps));
	    free(tmps);
	    xmlAddChild(nc6,nc7);
	    token=strtok_r(NULL,",",&saveptr1);
	  }
	  if(getMap(_tmp->content,"range")!=NULL ||
	     getMap(_tmp->content,"rangeMin")!=NULL ||
	     getMap(_tmp->content,"rangeMax")!=NULL ||
	     getMap(_tmp->content,"rangeClosure")!=NULL )
	    goto doRange;
	  xmlAddChild(nc3,nc6);
	  isAnyValue=-1;
	}

        tmp1=getMap(_tmp->content,"range");
	if(tmp1==NULL)
	  tmp1=getMap(_tmp->content,"rangeMin");
	if(tmp1==NULL)
	  tmp1=getMap(_tmp->content,"rangeMax");
	
	if(tmp1!=NULL && isAnyValue==1){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST "AllowedValues");
	doRange:
	  
	  /**
	   * Range: Table 46 OGC Web Services Common Standard
	   */
	  nc8 = xmlNewNode(ns_ows, BAD_CAST "Range");
	  
	  map* tmp0=getMap(tmp1,"range");
	  if(tmp0!=NULL){
	    char* pToken;
	    char* orig=zStrdup(tmp0->value);
	    /**
	     * RangeClosure: Table 47 OGC Web Services Common Standard
	     */
	    const char *tmp="closed";
	    if(orig[0]=='[' && orig[strlen(orig)-1]=='[')
	      tmp="closed-open";
	    else
	      if(orig[0]==']' && orig[strlen(orig)-1]==']')
		tmp="open-closed";
	      else
		if(orig[0]==']' && orig[strlen(orig)-1]=='[')
		  tmp="open";
	    xmlNewNsProp(nc8,ns_ows,BAD_CAST "rangeClosure",BAD_CAST tmp);
	    pToken=strtok(orig,",");
	    int nci0=0;
	    while(pToken!=NULL){
	      char *tmpStr=(char*) malloc((strlen(pToken))*sizeof(char));
	      if(nci0==0){
		nc7 = xmlNewNode(ns_ows, BAD_CAST "MinimumValue");
		strncpy( tmpStr, pToken+1, strlen(pToken)-1 );
		tmpStr[strlen(pToken)-1] = '\0';
	      }else{
		nc7 = xmlNewNode(ns_ows, BAD_CAST "MaximumValue");
		const char* bkt;
		if ( ( bkt = strchr(pToken, '[') ) != NULL || ( bkt = strchr(pToken, ']') ) != NULL ){
		    strncpy( tmpStr, pToken, bkt - pToken );
		    tmpStr[bkt - pToken] = '\0';
		  }
	      }
	      xmlAddChild(nc7,xmlNewText(BAD_CAST tmpStr));
	      free(tmpStr);
	      xmlAddChild(nc8,nc7);
	      nci0++;
	      pToken = strtok(NULL,",");
	    }		    
	    if(getMap(tmp1,"rangeSpacing")==NULL){
	      nc7 = xmlNewNode(ns_ows, BAD_CAST "Spacing");
	      xmlAddChild(nc7,xmlNewText(BAD_CAST "1"));
	      xmlAddChild(nc8,nc7);
	    }
	    free(orig);
	  }else{
	    
	    tmp0=getMap(tmp1,"rangeMin");
	    if(tmp0!=NULL){
	      nc7 = xmlNewNode(ns_ows, BAD_CAST "MinimumValue");
	      xmlAddChild(nc7,xmlNewText(BAD_CAST tmp0->value));
	      xmlAddChild(nc8,nc7);
	    }else{
	      nc7 = xmlNewNode(ns_ows, BAD_CAST "MinimumValue");
	      xmlAddChild(nc8,nc7);
	    }
	    tmp0=getMap(tmp1,"rangeMax");
	    if(tmp0!=NULL){
	      nc7 = xmlNewNode(ns_ows, BAD_CAST "MaximumValue");
	      xmlAddChild(nc7,xmlNewText(BAD_CAST tmp0->value));
	      xmlAddChild(nc8,nc7);
	    }else{
	      nc7 = xmlNewNode(ns_ows, BAD_CAST "MaximumValue");
	      xmlAddChild(nc8,nc7);
	    }
	    tmp0=getMap(tmp1,"rangeSpacing");
	    if(tmp0!=NULL){
	      nc7 = xmlNewNode(ns_ows, BAD_CAST "Spacing");
	      xmlAddChild(nc7,xmlNewText(BAD_CAST tmp0->value));
	      xmlAddChild(nc8,nc7);
	    }
	    tmp0=getMap(tmp1,"rangeClosure");
	    if(tmp0!=NULL){
	      const char *tmp="closed";
	      if(strcasecmp(tmp0->value,"co")==0)
		tmp="closed-open";
	      else
		if(strcasecmp(tmp0->value,"oc")==0)
		  tmp="open-closed";
		else
		  if(strcasecmp(tmp0->value,"o")==0)
		    tmp="open";
	      xmlNewNsProp(nc8,ns_ows,BAD_CAST "rangeClosure",BAD_CAST tmp);
	    }else
	      xmlNewNsProp(nc8,ns_ows,BAD_CAST "rangeClosure",BAD_CAST "closed");
	  }
	  if(_tmp0==NULL){
	    xmlAddChild(nc6,nc8);
	    _tmp0=e->supported;
	    if(_tmp0!=NULL &&
	       (getMap(_tmp0->content,"range")!=NULL ||
		getMap(_tmp0->content,"rangeMin")!=NULL ||
		getMap(_tmp0->content,"rangeMax")!=NULL ||
		getMap(_tmp0->content,"rangeClosure")!=NULL )){
	      tmp1=_tmp0->content;
	      goto doRange;
	    }
	  }else{
	    _tmp0=_tmp0->next;
	    if(_tmp0!=NULL){
	      xmlAddChild(nc6,nc8);
	      if(getMap(_tmp0->content,"range")!=NULL ||
		 getMap(_tmp0->content,"rangeMin")!=NULL ||
		 getMap(_tmp0->content,"rangeMax")!=NULL ||
		 getMap(_tmp0->content,"rangeClosure")!=NULL ){
		tmp1=_tmp0->content;
		goto doRange;
	      }
	    }
	  }
	  xmlAddChild(nc6,nc8);
	  xmlAddChild(nc3,nc6);
	  isAnyValue=-1;
	}
	
      }
    
      
    int oI=0;
    for(oI=0;oI<13;oI++)
      if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
#ifdef DEBUG
	printf("DATATYPE DEFAULT ? %s\n",tmp1->name);
#endif
	if(strcmp(tmp1->name,"asReference")!=0 &&
	   strncasecmp(tmp1->name,"DataType",8)!=0 &&
	   strcasecmp(tmp1->name,"extension")!=0 &&
	   strcasecmp(tmp1->name,"value")!=0 &&
	   strcasecmp(tmp1->name,"AllowedValues")!=0 &&
	   strncasecmp(tmp1->name,"range",5)!=0){
	  if(datatype!=1){
	    char *tmp2=zCapitalize1(tmp1->name);
	    nc9 = xmlNewNode(NULL, BAD_CAST tmp2);
	    free(tmp2);
	  }
	  else{
	    char *tmp2=zCapitalize(tmp1->name);
	    nc9 = xmlNewNode(ns_ows, BAD_CAST tmp2);
	    free(tmp2);
	  }
	  xmlAddChild(nc9,xmlNewText(BAD_CAST tmp1->value));
	  xmlAddChild(nc5,nc9);
	  if(strcasecmp(tmp1->name,"uom")==0)
	    hasUOM1=true;
	  hasUOM=true;
	}else 
	  
	  tmp1=tmp1->next;
      }
    
    
      if(datatype!=2){
	if(hasUOM==true){
	  xmlAddChild(nc4,nc5);
	  xmlAddChild(nc3,nc4);
	}else{
	  if(hasUOM1==false){
	    xmlFreeNode(nc5);
	    if(datatype==1)
	      xmlFreeNode(nc4);
	  }
	}
      }else{
	xmlAddChild(nc3,nc5);
      }
      
      if(datatype!=1 && default1<0){
	xmlFreeNode(nc5);
	if(datatype!=2)
	  xmlFreeNode(nc4);
      }

      map* metadata=e->metadata;
      xmlNodePtr n=NULL;
      int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
      xmlNsPtr ns_xlink=usedNs[xlinkId];

      while(metadata!=NULL){
	nc6=xmlNewNode(ns_ows, BAD_CAST "Metadata");
	xmlNewNsProp(nc6,ns_xlink,BAD_CAST metadata->name,BAD_CAST metadata->value);
	xmlAddChild(nc2,nc6);
	metadata=metadata->next;
      }

    }

    _tmp=e->supported;
    if(_tmp==NULL && datatype!=1)
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
	     strcmp(tmp1->name,"value")!=0 && 
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
	    }
	    xmlAddChild(nc5,nc6);
	  }
	  tmp1=tmp1->next;
	}
      if(hasSupported<=0){
	if(datatype==0){
	  xmlAddChild(nc4,nc5);
	  xmlAddChild(nc3,nc4);
	}else{
	  if(datatype!=1)
	    xmlAddChild(nc3,nc5);
	}
	hasSupported=1;
      }
      else
	if(datatype==0){
	  xmlAddChild(nc4,nc5);
	  xmlAddChild(nc3,nc4);
	}
	else
	  if(datatype!=1)
	    xmlAddChild(nc3,nc5);

      _tmp=_tmp->next;
    }

    if(hasSupported==0){
      if(datatype==0)
	xmlFreeNode(nc4);
      xmlFreeNode(nc5);
    }

    _tmp=e->defaults;
    if(datatype==1 && hasUOM1==true){
      xmlAddChild(nc4,nc5);
      xmlAddChild(nc3,nc4);
    }

    if(in>0 && datatype==1 &&
       getMap(_tmp->content,"AllowedValues")==NULL &&
       getMap(_tmp->content,"range")==NULL &&
       getMap(_tmp->content,"rangeMin")==NULL &&
       getMap(_tmp->content,"rangeMax")==NULL &&
       getMap(_tmp->content,"rangeClosure")==NULL ){
      tmp1=getMap(_tmp->content,"dataType");
      if(tmp1!=NULL && strcasecmp(tmp1->value,"boolean")==0){
	nc6 = xmlNewNode(ns_ows, BAD_CAST "AllowedValues");
	nc7 = xmlNewNode(ns_ows, BAD_CAST "Value");
	xmlAddChild(nc7,xmlNewText(BAD_CAST "true"));
	xmlAddChild(nc6,nc7);
	nc7 = xmlNewNode(ns_ows, BAD_CAST "Value");
	xmlAddChild(nc7,xmlNewText(BAD_CAST "false"));
	xmlAddChild(nc6,nc7);
	xmlAddChild(nc3,nc6);
      }
      else
	xmlAddChild(nc3,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
    }
    
    if((tmp1=getMap(_tmp->content,"value"))!=NULL){
      nc7 = xmlNewNode(NULL, BAD_CAST "DefaultValue");
      xmlAddChild(nc7,xmlNewText(BAD_CAST tmp1->value));
      xmlAddChild(nc3,nc7);
    }
    
    xmlAddChild(nc2,nc3);
    
    xmlAddChild(nc1,nc2);
    
    e=e->next;
  }
}

/**
 * Generate a wps:Execute XML document.
 * 
 * @param m the conf maps containing the main.cfg settings
 * @param request the map representing the HTTP request
 * @param pid the process identifier linked to a service
 * @param serv the serv structure created from the zcfg file
 * @param service the service name
 * @param status the status returned by the service
 * @param inputs the inputs provided
 * @param outputs the outputs generated by the service
 */
void printProcessResponse(maps* m,map* request, int pid,service* serv,const char* service,int status,maps* inputs,maps* outputs){
  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr nr,n,nc,nc1=NULL,nc3;
  xmlDocPtr doc;
  time_t time1;  
  time(&time1);
  nr=NULL;
  doc = xmlNewDoc(BAD_CAST "1.0");
  n = printWPSHeader(doc,m,"Execute","ExecuteResponse");
  int wpsId=zooXmlAddNs(NULL,"http://www.opengis.net/wps/1.0.0","wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(NULL,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];

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
      tmp_lenv=getMapFromMaps(m,"lenv","usid");
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
      int lpid;
      map* tmpm2=getMapFromMaps(m,"lenv","usid");
      lpid=atoi(tmpm2->value);
      tmpm2=getMap(tmp_maps->content,"tmpUrl");
      if(tmpm1!=NULL && tmpm2!=NULL){
	if( strncasecmp( tmpm2->value, "http://", 7) == 0 ||
	    strncasecmp( tmpm2->value, "https://", 8 ) == 0 ){
	  sprintf(url,"%s/%s_%i.xml",tmpm2->value,service,lpid);
	}else
	  sprintf(url,"%s/%s/%s_%i.xml",tmpm1->value,tmpm2->value,service,lpid);
      }
    }
    if(tmpm1!=NULL)
      sprintf(tmp,"%s",tmpm1->value);
    int lpid;
    tmpm1=getMapFromMaps(m,"lenv","usid");
    lpid=atoi(tmpm1->value);
    tmpm1=getMapFromMaps(m,"main","TmpPath");
    sprintf(stored_path,"%s/%s_%i.xml",tmpm1->value,service,lpid);
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
  
  map* tmpI=getMapFromMaps(m,"lenv","oIdentifier");
  printDescription(nc,ns_ows,tmpI->value,serv->content);

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
      printOutputDefinitions(doc,nc,ns,ns_ows,scursor,mcursor,"Output");
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
    map* testResponse=getMap(request,"RawDataOutput");
    if(testResponse==NULL)
      testResponse=getMap(request,"ResponseDocument");
    while(mcursor!=NULL){
      map* tmp0=getMap(mcursor->content,"inRequest");
      scursor=getElements(serv->outputs,mcursor->name);
      if(scursor!=NULL){
	if(testResponse==NULL || tmp0==NULL)
	  printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Output");
	else
	  if(tmp0!=NULL && strncmp(tmp0->value,"true",4)==0)
	    printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Output");
      }else
	/**
	 * In case there was no definition found in the ZCFG file but 
	 * present in the service code
	 */
	printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Output");
      mcursor=mcursor->next;
    }
    xmlAddChild(n,nc);
  }

  if(hasStoredExecuteResponse==true && status!=SERVICE_STARTED){
    semid lid=getShmLockId(m,1);
    if(lid<0)
      return;
    else{
#ifdef DEBUG
      fprintf(stderr,"LOCK %s %d !\n",__FILE__,__LINE__);
#endif
      lockShm(lid);
      /* We need to write the ExecuteResponse Document somewhere */
      FILE* output=fopen(stored_path,"w");
      if(output==NULL){
	/* If the file cannot be created return an ExceptionReport */
	char tmpMsg[1024];
	sprintf(tmpMsg,_("Unable to create the file : \"%s\" for storing the ExecuteResponse."),stored_path);

	errorException(m,tmpMsg,"InternalError",NULL);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	zooXmlCleanupNs();
	unlockShm(lid);
	return;
      }
      xmlChar *xmlbuff;
      int buffersize;
      xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, "UTF-8", 1);
      fwrite(xmlbuff,1,xmlStrlen(xmlbuff)*sizeof(char),output);
      xmlFree(xmlbuff);
      fclose(output);
#ifdef DEBUG
      fprintf(stderr,"UNLOCK %s %d !\n",__FILE__,__LINE__);
#endif
      unlockShm(lid);
      map* test1=getMap(request,"status");
      if(test1==NULL || strcasecmp(test1->value,"true")!=0){
	removeShmLock(m,1);
      }
    }
  }
  printDocument(m,doc,pid);

  xmlCleanupParser();
  zooXmlCleanupNs();
}

/**
 * Print a XML document.
 * 
 * @param m the conf maps containing the main.cfg settings
 * @param doc the XML document
 * @param pid the process identifier linked to a service
 */
void printDocument(maps* m, xmlDocPtr doc,int pid){
  char *encoding=getEncoding(m);
  if(pid==getpid()){
    printHeaders(m);
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

/**
 * Print a XML document.
 * 
 * @param doc the XML document (unused)
 * @param nc the XML node to add the output definition
 * @param ns_wps the wps XML namespace
 * @param ns_ows the ows XML namespace
 * @param e the output elements 
 * @param m the conf maps containing the main.cfg settings
 * @param type the type (unused)
 */
void printOutputDefinitions(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,maps* m,const char* type){
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

/**
 * Generate XML nodes describing inputs or outputs metadata.
 * 
 * @param doc the XML document 
 * @param nc the XML node to add the definition
 * @param ns_wps the wps namespace
 * @param ns_ows the ows namespace
 * @param ns_xlink the xlink namespace
 * @param e the output elements 
 * @param m the conf maps containing the main.cfg settings
 * @param type the type
 */
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
#ifdef USE_MS
    map* testMap=getMap(tmp,"requestedMimeType");
#endif
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,"mimeType")==0 ||
	 strcasecmp(tmp->name,"encoding")==0 ||
	 strcasecmp(tmp->name,"schema")==0 ||
	 strcasecmp(tmp->name,"datatype")==0 ||
	 strcasecmp(tmp->name,"uom")==0){
#ifdef USE_MS
	if(testMap==NULL || (testMap!=NULL && strncasecmp(testMap->value,"text/xml",8)==0)){
#endif
	  xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
#ifdef USE_MS
	}
	else
	  if(strcasecmp(tmp->name,"mimeType")==0){
	    if(testMap!=NULL)
	      xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST testMap->value);
	    else 
	      xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
	  }
#endif
      }
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
#ifdef USE_MS
      /**
       * In case of OGC WebServices output use, as the data was requested
       * with asReference=false we have to download the resulting OWS request
       * stored in the Reference map value.
       */
      map* testMap=getMap(m->content,"requestedMimeType");
      if(testMap!=NULL){
	HINTERNET hInternet;
	char* tmpValue;
	size_t dwRead;
	hInternet=InternetOpen(
#ifndef WIN32
			       (LPCTSTR)
#endif
			       "ZooWPSClient\0",
			       INTERNET_OPEN_TYPE_PRECONFIG,
			       NULL,NULL, 0);
	testMap=getMap(m->content,"Reference");
	loadRemoteFile(&m,&m->content,&hInternet,testMap->value);
	processDownloads(&hInternet);
	tmpValue=(char*)malloc((hInternet.ihandle[0].nDataLen+1)*sizeof(char));
	InternetReadFile(hInternet.ihandle[0],(LPVOID)tmpValue,hInternet.ihandle[0].nDataLen,&dwRead);
	InternetCloseHandle(&hInternet);
      }
#endif
      map* tmp1=getMap(m->content,"encoding");
      map* tmp2=getMap(m->content,"mimeType");
      map* tmp3=getMap(m->content,"value");
      int hasValue=1;
      if(tmp3==NULL){
	tmp3=createMap("value","");
	hasValue=-1;
      }
      if((tmp1!=NULL && strncmp(tmp1->value,"base64",6)==0)
	 || (tmp2!=NULL && (strncmp(tmp2->value,"image/",6)==0 ||
			    (strncmp(tmp2->value,"application/",12)==0 &&
			     strncmp(tmp2->value,"application/json",16)!=0&&
			     strncmp(tmp2->value,"application/x-javascript",24)!=0&&
			     strncmp(tmp2->value,"application/vnd.google-earth.kml",32)!=0))
	     )) {
	map* rs=getMap(m->content,"size");
	bool isSized=true;
	if(rs==NULL){
	  char tmp1[1024];
	  sprintf(tmp1,"%ld",strlen(tmp3->value));
	  rs=createMap("size",tmp1);
	  isSized=false;
	}

	xmlAddChild(nc3,xmlNewText(BAD_CAST base64(tmp3->value, atoi(rs->value))));
	if(tmp1==NULL || (tmp1!=NULL && strncmp(tmp1->value,"base64",6)!=0))
	  xmlNewProp(nc3,BAD_CAST "encoding",BAD_CAST "base64");
	if(!isSized){
	  freeMap(&rs);
	  free(rs);
	}
      }
      else if(tmp2!=NULL){
	if(strncmp(tmp2->value,"text/js",7)==0 ||
	   strncmp(tmp2->value,"application/json",16)==0)
	  xmlAddChild(nc3,xmlNewCDataBlock(doc,BAD_CAST tmp3->value,strlen(tmp3->value)));
	else{
	  if(strncmp(tmp2->value,"text/xml",8)==0 ||
	     strncmp(tmp2->value,"application/vnd.google-earth.kml",32)==0){
	    int li=zooXmlAddDoc(tmp3->value);
	    xmlDocPtr doc = iDocs[li];
	    xmlNodePtr ir = xmlDocGetRootElement(doc);
	    xmlAddChild(nc3,ir);
	  }
	  else
	    xmlAddChild(nc3,xmlNewText(BAD_CAST tmp3->value));
	}
	xmlAddChild(nc2,nc3);
      }
      else{
	xmlAddChild(nc3,xmlNewText(BAD_CAST tmp3->value));
      }
      if(hasValue<0){
	freeMap(&tmp3);
	free(tmp3);
      }
    }
  }
  else{
    tmpMap=getMap(m->content,"Reference");
    nc3=nc2=xmlNewNode(ns_wps, BAD_CAST "Reference");
    if(strcasecmp(type,"Output")==0)
      xmlNewProp(nc3,BAD_CAST "href",BAD_CAST tmpMap->value);
    else
      xmlNewNsProp(nc3,ns_xlink,BAD_CAST "href",BAD_CAST tmpMap->value);
    tmp=m->content;
#ifdef USE_MS
    map* testMap=getMap(tmp,"requestedMimeType");
#endif
    while(tmp!=NULL){
      if(strcasecmp(tmp->name,"mimeType")==0 ||
	 strcasecmp(tmp->name,"encoding")==0 ||
	 strcasecmp(tmp->name,"schema")==0 ||
	 strcasecmp(tmp->name,"datatype")==0 ||
	 strcasecmp(tmp->name,"uom")==0){
#ifdef USE_MS
	if(testMap!=NULL  && strncasecmp(testMap->value,"text/xml",8)!=0){
	  if(strcasecmp(tmp->name,"mimeType")==0)
	    xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST testMap->value);
	}
	else
#endif
	  if(strcasecmp(tmp->name,"datatype")==0)
	    xmlNewProp(nc3,BAD_CAST "mimeType",BAD_CAST "text/plain");
	  else
	    xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
      }
      tmp=tmp->next;
      xmlAddChild(nc2,nc3);
    }
  }
  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);

}

/**
 * Create XML node with basic ows metadata informations (Identifier,Title,Abstract)
 *
 * @param root the root XML node to add the description
 * @param ns_ows the ows XML namespace
 * @param identifier the identifier to use
 * @param amap the map containing the ows metadata informations 
 */
void printDescription(xmlNodePtr root,xmlNsPtr ns_ows,const char* identifier,map* amap){
  xmlNodePtr nc2 = xmlNewNode(ns_ows, BAD_CAST "Identifier");
  
  xmlAddChild(nc2,xmlNewText(BAD_CAST identifier));
  xmlAddChild(root,nc2);
  map* tmp=amap;
  const char *tmp2[2];
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

/**
 * Access the value of the encoding key in a maps
 *
 * @param m the maps to search for the encoding key
 * @return the value of the encoding key in a maps if encoding key exists,
 *  "UTF-8" in other case.
 */
char* getEncoding(maps* m){
  if(m!=NULL){
    map* tmp=getMap(m->content,"encoding");
    if(tmp!=NULL){
      return tmp->value;
    }
    else
      return (char*)"UTF-8";
  }
  else
    return (char*)"UTF-8";  
}

/**
 * Access the value of the version key in a maps
 *
 * @param m the maps to search for the version key
 * @return the value of the version key in a maps if encoding key exists,
 *  "1.0.0" in other case.
 */
char* getVersion(maps* m){
  if(m!=NULL){
    map* tmp=getMap(m->content,"version");
    if(tmp!=NULL){
      return tmp->value;
    }
    else
      return (char*)"1.0.0";
  }
  else
    return (char*)"1.0.0";
}

/**
 * Print an OWS ExceptionReport Document and HTTP headers (when required) 
 * depending on the code.
 * Set hasPrinted value to true in the [lenv] section.
 * 
 * @param m the maps containing the settings of the main.cfg file
 * @param s the map containing the text,code,locator keys
 */
void printExceptionReportResponse(maps* m,map* s){
  if(getMapFromMaps(m,"lenv","hasPrinted")!=NULL)
    return;
  int buffersize;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  xmlNodePtr n;

  zooXmlCleanupNs();
  doc = xmlNewDoc(BAD_CAST "1.0");
  maps* tmpMap=getMaps(m,"main");
  char *encoding=getEncoding(tmpMap);
  const char *exceptionCode;
  
  map* tmp=getMap(s,"code");
  if(tmp!=NULL){
    if(strcmp(tmp->value,"OperationNotSupported")==0 ||
       strcmp(tmp->value,"NoApplicableCode")==0)
      exceptionCode="501 Not Implemented";
    else
      if(strcmp(tmp->value,"MissingParameterValue")==0 ||
	 strcmp(tmp->value,"InvalidUpdateSequence")==0 ||
	 strcmp(tmp->value,"OptionNotSupported")==0 ||
	 strcmp(tmp->value,"VersionNegotiationFailed")==0 ||
	 strcmp(tmp->value,"InvalidParameterValue")==0)
	exceptionCode="400 Bad request";
      else
	exceptionCode="501 Internal Server Error";
  }
  else
    exceptionCode="501 Internal Server Error";

  if(m!=NULL){
    map *tmpSid=getMapFromMaps(m,"lenv","sid");
    if(tmpSid!=NULL){
      if( getpid()==atoi(tmpSid->value) ){
	printHeaders(m);
	printf("Content-Type: text/xml; charset=%s\r\nStatus: %s\r\n\r\n",encoding,exceptionCode);
      }
    }
    else{
      printHeaders(m);
      printf("Content-Type: text/xml; charset=%s\r\nStatus: %s\r\n\r\n",encoding,exceptionCode);
    }
  }else{
    printf("Content-Type: text/xml; charset=%s\r\nStatus: %s\r\n\r\n",encoding,exceptionCode);
  }
  n=createExceptionReportNode(m,s,1);
  xmlDocSetRootElement(doc, n);
  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  printf("%s",xmlbuff);
  fflush(stdout);
  xmlFreeDoc(doc);
  xmlFree(xmlbuff);
  xmlCleanupParser();
  zooXmlCleanupNs();
  if(m!=NULL)
    setMapInMaps(m,"lenv","hasPrinted","true");
}

/**
 * Create an OWS ExceptionReport Node.
 * 
 * @param m the conf maps
 * @param s the map containing the text,code,locator keys
 * @param use_ns (0/1) choose if you want to generate an ExceptionReport or 
 *  ows:ExceptionReport node respectively
 * @return the ExceptionReport/ows:ExceptionReport node
 */
xmlNodePtr createExceptionReportNode(maps* m,map* s,int use_ns){
  
  xmlNsPtr ns,ns_xsi;
  xmlNodePtr n,nc,nc1;

  int nsid=zooXmlAddNs(NULL,"http://www.opengis.net/ows","ows");
  ns=usedNs[nsid];
  if(use_ns==0){
    ns=NULL;
  }
  n = xmlNewNode(ns, BAD_CAST "ExceptionReport");
  if(use_ns==1){
    xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST"ows");
    int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
    ns_xsi=usedNs[xsiId];
    xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.opengis.net/ows/1.1 http://schemas.opengis.net/ows/1.1.0/owsExceptionReport.xsd");
  }


  addLangAttr(n,m);
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.1.0");
  
  int length=1;
  int cnt=0;
  map* len=getMap(s,"length");
  if(len!=NULL)
    length=atoi(len->value);
  for(cnt=0;cnt<length;cnt++){
    nc = xmlNewNode(ns, BAD_CAST "Exception");
    
    map* tmp=getMapArray(s,"code",cnt);
    if(tmp==NULL)
      tmp=getMap(s,"code");
    if(tmp!=NULL)
      xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST tmp->value);
    else
      xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST "NoApplicableCode");
    
    tmp=getMapArray(s,"locator",cnt);
    if(tmp==NULL)
      tmp=getMap(s,"locator");
    if(tmp!=NULL && strcasecmp(tmp->value,"NULL")!=0)
      xmlNewProp(nc,BAD_CAST "locator",BAD_CAST tmp->value);

    tmp=getMapArray(s,"text",cnt);
    nc1 = xmlNewNode(ns, BAD_CAST "ExceptionText");
    if(tmp!=NULL){
      xmlNodePtr txt=xmlNewText(BAD_CAST tmp->value);
      xmlAddChild(nc1,txt);
    }
    else{
      xmlNodeSetContent(nc1, BAD_CAST _("No debug message available"));
    }
    xmlAddChild(nc,nc1);
    xmlAddChild(n,nc);
  }
  return n;
}

/**
 * Print an OWS ExceptionReport.
 * 
 * @param m the conf maps
 * @param message the error message 
 * @param errorcode the error code
 * @param locator the potential locator
 */
int errorException(maps *m, const char *message, const char *errorcode, const char *locator) 
{
  map* errormap = createMap("text", message);
  addToMap(errormap,"code", errorcode);
  if(locator!=NULL)
    addToMap(errormap,"locator", locator);
  else
    addToMap(errormap,"locator", "NULL");
  printExceptionReportResponse(m,errormap);
  freeMap(&errormap);
  free(errormap);
  return -1;
}

/**
 * Read a file generated by a service.
 * 
 * @param m the conf maps
 * @param content the output item
 * @param filename the file to read
 */
void readGeneratedFile(maps* m,map* content,char* filename){
  FILE * file=fopen(filename,"rb");
  if(file==NULL){
    fprintf(stderr,"Failed to open file %s for reading purpose.\n",filename);
    setMapInMaps(m,"lenv","message","Unable to read produced file. Please try again later");
    return ;
  }
  fseek(file, 0, SEEK_END);
  long count = ftell(file);
  rewind(file);
  struct stat file_status; 
  stat(filename, &file_status);
  map* tmpMap1=getMap(content,"value");
  if(tmpMap1==NULL){
    addToMap(content,"value","");
    tmpMap1=getMap(content,"value");
  }
  free(tmpMap1->value);
  tmpMap1->value=(char*) malloc((count+1)*sizeof(char));  
  fread(tmpMap1->value,1,count*sizeof(char),file);
  fclose(file);
  char rsize[100];
  sprintf(rsize,"%ld",count*sizeof(char));
  addToMap(tmpMap1,"size",rsize);
}

/**
 * Generate the output response (RawDataOutput or ResponseDocument)
 *
 * @param s the service structure containing the metadata informations
 * @param request_inputs the inputs provided to the service for execution
 * @param request_outputs the outputs updated by the service execution
 * @param request_inputs1 the map containing the HTTP request
 * @param cpid the process identifier attached to a service execution
 * @param m the conf maps containing the main.cfg settings
 * @param res the value returned by the service execution
 */
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
  
  maps* tmpSess=getMaps(m,"senv");
  if(tmpSess!=NULL){
    map *_tmp=getMapFromMaps(m,"lenv","cookie");
    char* sessId=NULL;
    if(_tmp!=NULL){
      printf("Set-Cookie: %s; HttpOnly\r\n",_tmp->value);
      printf("P3P: CP=\"IDC DSP COR ADM DEVi TAIi PSA PSD IVAi IVDi CONi HIS OUR IND CNT\"\r\n");
      char session_file_path[100];
      char *tmp1=strtok(_tmp->value,";");
      if(tmp1!=NULL)
	sprintf(session_file_path,"%s",strstr(tmp1,"=")+1);
      else
	sprintf(session_file_path,"%s",strstr(_tmp->value,"=")+1);
      sessId=strdup(session_file_path);
    }else{
      maps* t=getMaps(m,"senv");
      map*p=t->content;
      while(p!=NULL){
	if(strstr(p->name,"ID")!=NULL){
	  sessId=strdup(p->value);
	  break;
	}
	p=p->next;
      }
    }
    char session_file_path[1024];
    map *tmpPath=getMapFromMaps(m,"main","sessPath");
    if(tmpPath==NULL)
      tmpPath=getMapFromMaps(m,"main","tmpPath");
    sprintf(session_file_path,"%s/sess_%s.cfg",tmpPath->value,sessId);
    FILE* teste=fopen(session_file_path,"w");
    if(teste==NULL){
      char tmpMsg[1024];
      sprintf(tmpMsg,_("Unable to create the file : \"%s\" for storing the session maps."),session_file_path);
      errorException(m,tmpMsg,"InternalError",NULL);

      return;
    }
    else{
      fclose(teste);
      dumpMapsToFile(tmpSess,session_file_path);
    }
  }
  
  if(res==SERVICE_FAILED){
    map *lenv;
    lenv=getMapFromMaps(m,"lenv","message");
    char *tmp0;
    if(lenv!=NULL){
      tmp0=(char*)malloc((strlen(lenv->value)+strlen(_("Unable to run the Service. The message returned back by the Service was the following: "))+1)*sizeof(char));
      sprintf(tmp0,_("Unable to run the Service. The message returned back by the Service was the following: %s"),lenv->value);
    }
    else{
      tmp0=(char*)malloc((strlen(_("Unable to run the Service. No more information was returned back by the Service."))+1)*sizeof(char));
      sprintf(tmp0,"%s",_("Unable to run the Service. No more information was returned back by the Service."));
    }
    errorException(m,tmp0,"InternalError",NULL);
    free(tmp0);
    return;
  }


  map *tmp1=getMapFromMaps(m,"main","tmpPath");
  if(asRaw==0){
#ifdef DEBUG
    fprintf(stderr,"REQUEST_OUTPUTS FINAL\n");
    dumpMaps(request_outputs);
#endif
    maps* tmpI=request_outputs;

    while(tmpI!=NULL){
#ifdef USE_MS
      map* testMap=getMap(tmpI->content,"useMapserver");	
#endif
      toto=getMap(tmpI->content,"asReference");
#ifdef USE_MS
      if(toto!=NULL && strcasecmp(toto->value,"true")==0 && testMap==NULL)
#else
      if(toto!=NULL && strcasecmp(toto->value,"true")==0)
#endif
	{
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

	  map *gfile=getMap(tmpI->content,"generated_file");
	  char *file_name;
	  if(gfile!=NULL){
	    gfile=getMap(tmpI->content,"expected_generated_file");
	    if(gfile==NULL){
	      gfile=getMap(tmpI->content,"generated_file");
	    }
	    readGeneratedFile(m,tmpI->content,gfile->value);	    
	    file_name=(char*)malloc((strlen(gfile->value)+strlen(tmp1->value)+1)*sizeof(char));
	    for(int i=0;i<strlen(gfile->value);i++)
	      file_name[i]=gfile->value[i+strlen(tmp1->value)];
	  }
	  else{
	    map *ext=getMap(tmpI->content,"extension");
	    char *file_path;
	    bool hasExt=true;
	    if(ext==NULL){
	      // We can fallback to a default list of supported formats using
	      // mimeType information if present here. Maybe we can add more formats
	      // here.
	      // If mimeType was not found, we then set txt as the default extension
	      map* mtype=getMap(tmpI->content,"mimeType");
	      if(mtype!=NULL) {
		if(strncasecmp(mtype->value,"text/xml",8)==0)
		  ext=createMap("extension","xml");
		else if(strncasecmp(mtype->value,"application/zip",15)==0)
		  ext=createMap("extension","zip");
		else if(strncasecmp(mtype->value,"application/json",16)==0)
		  ext=createMap("extension","js");
		else if(strncmp(mtype->value,"application/vnd.google-earth.kml",32)==0)
		  ext=createMap("extension","kml");
		else if(strncmp(mtype->value,"image/",6)==0)
		  ext=createMap("extension",strstr(mtype->value,"/")+1);
	    else if(strcasecmp(mtype->value,"text/html")==0)
	      ext=createMap("extension","html");	  
		else
		  ext=createMap("extension","txt");
	      }
	      else
		ext=createMap("extension","txt");
	      hasExt=false;
	    }
	    file_name=(char*)malloc((strlen(s->name)+strlen(ext->value)+strlen(tmpI->name)+1024)*sizeof(char));
	    int cpid0=cpid+time(NULL);
	    sprintf(file_name,"%s_%s_%i.%s",s->name,tmpI->name,cpid0,ext->value);
	    file_path=(char*)malloc((strlen(tmp1->value)+strlen(file_name)+2)*sizeof(char));
	    sprintf(file_path,"%s/%s",tmp1->value,file_name);
	    FILE *ofile=fopen(file_path,"wb");
	    if(ofile==NULL){
	      char tmpMsg[1024];
	      sprintf(tmpMsg,_("Unable to create the file : \"%s\" for storing the %s final result."),file_name,tmpI->name);
	      errorException(m,tmpMsg,"InternalError",NULL);
	      free(file_name);
	      free(file_path);
	      return;
	    }
	    free(file_path);
	    if(!hasExt){
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
	    fclose(ofile);

	  }
	  map *tmp2=getMapFromMaps(m,"main","tmpUrl");
	  map *tmp3=getMapFromMaps(m,"main","serverAddress");
	  char *file_url;
	  if(strncasecmp(tmp2->value,"http://",7)==0 ||
	     strncasecmp(tmp2->value,"https://",8)==0){
	    file_url=(char*)malloc((strlen(tmp2->value)+strlen(file_name)+2)*sizeof(char));
	    sprintf(file_url,"%s/%s",tmp2->value,file_name);
	  }else{
	    file_url=(char*)malloc((strlen(tmp3->value)+strlen(tmp2->value)+strlen(file_name)+3)*sizeof(char));
	    sprintf(file_url,"%s/%s/%s",tmp3->value,tmp2->value,file_name);
	  }
	  addToMap(tmpI->content,"Reference",file_url);
	  free(format);
	  free(file_name);
	  free(file_url);	
	  
	}
#ifdef USE_MS
      else{
	if(testMap!=NULL){
	  setReferenceUrl(m,tmpI);
	}
      }
#endif
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
  else{
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
      map *gfile=getMap(tmpI->content,"generated_file");
      if(gfile!=NULL){
	gfile=getMap(tmpI->content,"expected_generated_file");
	if(gfile==NULL){
	  gfile=getMap(tmpI->content,"generated_file");
	}
	readGeneratedFile(m,tmpI->content,gfile->value);
      }
      toto=getMap(tmpI->content,"value");
      if(toto==NULL){
	char tmpMsg[1024];
	sprintf(tmpMsg,_("Wrong RawDataOutput parameter, unable to fetch any result for the name your provided : \"%s\"."),tmpI->name);
	errorException(m,tmpMsg,"InvalidParameterValue","RawDataOutput");
	return;
      }
      map* fname=getMapFromMaps(tmpI,tmpI->name,"filename");
      if(fname!=NULL)
	printf("Content-Disposition: attachment; filename=\"%s\"\r\n",fname->value);
      map* rs=getMapFromMaps(tmpI,tmpI->name,"size");
      if(rs!=NULL)
	printf("Content-Length: %s\r\n",rs->value);
      printHeaders(m);
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
      if(rs!=NULL)
	fwrite(toto->value,1,atoi(rs->value),stdout);
      else
	fwrite(toto->value,1,strlen(toto->value),stdout);
#ifdef DEBUG
      dumpMap(toto);
#endif
    }
  }
}


/**
 * Base64 encoding of a char*
 *
 * @param input the value to encode
 * @param length the value length
 * @return the buffer containing the base64 value
 * @warning make sure to free the returned value
 */
char *base64(const char *input, int length)
{
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, length+1);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  char *buff = (char *)malloc((bptr->length+1)*sizeof(char));
  memcpy(buff, bptr->data, bptr->length);
  buff[bptr->length-1] = 0;

  BIO_free_all(b64);

  return buff;
}

/**
 * Base64 decoding of a char*
 *
 * @param input the value to decode
 * @param length the value length
 * @param red the value length
 * @return the buffer containing the base64 value 
 * @warning make sure to free the returned value
 */
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

/**
 * Make sure that each value encoded in base64 in a maps is decoded.
 *
 * @param in the maps containing the values
 */
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

/**
 * Add the default values defined in the zcfg to a maps.
 *
 * @param out the maps containing the inputs or outputs given in the initial
 *  HTTP request
 * @param in the description of all inputs or outputs available for a service
 * @param m the maps containing the settings of the main.cfg file
 * @param type 0 for inputs and 1 for outputs
 * @param err the map to store potential missing mandatory input parameters or
 *  wrong output names depending on the type.
 * @return "" if no error was detected, the name of last input or output causing
 *  an error.
 */
char* addDefaultValues(maps** out,elements* in,maps* m,int type,map** err){
  map *res=*err;
  elements* tmpInputs=in;
  maps* out1=*out;
  char *result=NULL;
  int nb=0;
  if(type==1){
    while(out1!=NULL){
      if(getElements(in,out1->name)==NULL){
	if(res==NULL){
	  res=createMap("value",out1->name);
	}else{
	  setMapArray(res,"value",nb,out1->name);
	}
	nb++;
	result=out1->name;
      }
      out1=out1->next;
    }
    if(res!=NULL){
      *err=res;
      return result;
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
	if(tmpMapMinO!=NULL){
	  if(atoi(tmpMapMinO->value)>=1){
	    freeMaps(&tmpMaps2);
	    free(tmpMaps2);
	    if(res==NULL){
	      res=createMap("value",tmpInputs->name);
	    }else{
	      setMapArray(res,"value",nb,tmpInputs->name);
	    }
	    nb++;
	    result=tmpInputs->name;
	  }
	  else{
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("minOccurs",tmpMapMinO->value);
	    else
	      addToMap(tmpMaps2->content,"minOccurs",tmpMapMinO->value);
	  }
	}
	if(res==NULL){
	  map* tmpMaxO=getMap(tmpInputs->content,"maxOccurs");
	  if(tmpMaxO!=NULL){
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("maxOccurs",tmpMaxO->value);
	    else
	      addToMap(tmpMaps2->content,"maxOccurs",tmpMaxO->value);
	  }
	  map* tmpMaxMB=getMap(tmpInputs->content,"maximumMegabytes");
	  if(tmpMaxMB!=NULL){
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("maximumMegabytes",tmpMaxMB->value);
	    else
	      addToMap(tmpMaps2->content,"maximumMegabytes",tmpMaxMB->value);
	  }
	}
      }

      if(res==NULL){
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
	    tmpMaps->content=createMap("maxOccurs",tmpMaxO->value);
	  else
	    addToMap(tmpMaps->content,"maxOccurs",tmpMaxO->value);
	}
	map* tmpMaxMB=getMap(tmpInputs->content,"maximumMegabytes");
	if(tmpMaxMB!=NULL){
	  if(tmpMaps->content==NULL)
	    tmpMaps->content=createMap("maximumMegabytes",tmpMaxMB->value);
	  else
	    addToMap(tmpMaps->content,"maximumMegabytes",tmpMaxMB->value);
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
	int hasPassed=-1;
	while(tmpContent!=NULL){
	  if((cval=getMap(tmpMaps->content,tmpContent->name))==NULL){
#ifdef DEBUG
	    fprintf(stderr,"addDefaultValues %s => %s\n",tmpContent->name,tmpContent->value);
#endif
	    if(tmpMaps->content==NULL)
	      tmpMaps->content=createMap(tmpContent->name,tmpContent->value);
	    else
	      addToMap(tmpMaps->content,tmpContent->name,tmpContent->value);
	    
	    if(hasPassed<0 && type==0 && getMap(tmpMaps->content,"isArray")!=NULL){
	      map* length=getMap(tmpMaps->content,"length");
	      int i;
	      char *tcn=strdup(tmpContent->name);
	      for(i=1;i<atoi(length->value);i++){
#ifdef DEBUG
		dumpMap(tmpMaps->content);
		fprintf(stderr,"addDefaultValues %s_%d => %s\n",tcn,i,tmpContent->value);
#endif
		int len=strlen((char*) tcn);
		char *tmp1=(char *)malloc((len+10)*sizeof(char));
		sprintf(tmp1,"%s_%d",tcn,i);
#ifdef DEBUG
		fprintf(stderr,"addDefaultValues %s => %s\n",tmp1,tmpContent->value);
#endif
		addToMap(tmpMaps->content,tmp1,tmpContent->value);
		free(tmp1);
		hasPassed=1;
	      }
	      free(tcn);
	    }
	  }
	  tmpContent=tmpContent->next;
	}
#ifdef USE_MS
	/**
	 * check for useMapServer presence
	 */
	map* tmpCheck=getMap(tmpIoType->content,"useMapServer");
	if(tmpCheck!=NULL){
	  // Get the default value
	  tmpIoType=getIoTypeFromElement(tmpInputs,tmpInputs->name,NULL);
	  tmpCheck=getMap(tmpMaps->content,"mimeType");
	  addToMap(tmpMaps->content,"requestedMimeType",tmpCheck->value);
	  map* cursor=tmpIoType->content;
	  while(cursor!=NULL){
	    addToMap(tmpMaps->content,cursor->name,cursor->value);
	    cursor=cursor->next;
	  }
	  
	  cursor=tmpInputs->content;
	  while(cursor!=NULL){
	    if(strcasecmp(cursor->name,"Title")==0 ||
	       strcasecmp(cursor->name,"Abstract")==0)
	      addToMap(tmpMaps->content,cursor->name,cursor->value);
           cursor=cursor->next;
	  }
	}
#endif
      }
      if(tmpMaps->content==NULL)
	tmpMaps->content=createMap("inRequest","true");
      else
	addToMap(tmpMaps->content,"inRequest","true");

    }
    tmpInputs=tmpInputs->next;
  }
  if(res!=NULL){
    *err=res;
    return result;
  }
  return "";
}

/**
 * Parse a BoundingBox string
 *
 * [OGC 06-121r3](http://portal.opengeospatial.org/files/?artifact_id=20040):
 *  10.2 Bounding box
 * 
 *
 * Value is provided as : lowerCorner,upperCorner,crs,dimension
 * Exemple : 189000,834000,285000,962000,urn:ogc:def:crs:OGC:1.3:CRS84
 *
 * A map to store boundingbox informations should contain:
 *  - lowerCorner : double,double (minimum within this bounding box)
 *  - upperCorner : double,double (maximum within this bounding box)
 *  - crs : URI (Reference to definition of the CRS)
 *  - dimensions : int 
 * 
 * Note : support only 2D bounding box.
 *
 * @param value the char* containing the KVP bouding box
 * @return a map containing all the bounding box keys
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
 * Create required XML nodes for boundingbox and update the current XML node
 *
 * @param ns_ows the ows XML namespace
 * @param n the XML node to update
 * @param boundingbox the map containing the boundingbox definition
 */
void printBoundingBox(xmlNsPtr ns_ows,xmlNodePtr n,map* boundingbox){

  xmlNodePtr lw=NULL,uc=NULL;

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

/**
 * Print an ows:BoundingBox XML document
 *
 * @param m the maps containing the settings of the main.cfg file
 * @param boundingbox the maps containing the boundingbox definition
 * @param file the file to print the BoundingBox (if NULL then print on stdout)
 * @see parseBoundingBox, printBoundingBox
 */
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

/**
 * Compute md5
 * 
 * @param url the char* 
 * @return a char* representing the md5 of the url
 * @warning make sure to free ressources returned by this function
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
 */
void addToCache(maps* conf,char* request,char* content,char* mimeType,int length){
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
      fprintf (stderr, "Failed to open %s for writting: %s\n",fname, strerror(errno));
      return;
    }
    fwrite(content,sizeof(char),length,fo);
    fclose(fo);

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
}

/**
 * Verify if a url is available in the cache
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param request the url
 * @return the full name of the cached file if any, NULL in other case
 * @warning make sure to free ressources returned by this function (if not NULL)
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
 * Effectively run all the HTTP requests in the queue
 *
 * @param m the maps containing the settings of the main.cfg file
 * @param inputs the maps containing the inputs (defined in the requests+added
 *  per default based on the zcfg file)
 * @param hInternet the HINTERNET pointer
 * @return 0 on success
 */
int runHttpRequests(maps** m,maps** inputs,HINTERNET* hInternet){
  if(hInternet->nb>0){
    processDownloads(hInternet);
    maps* content=*inputs;
    map* tmp1;
    int index=0;
    while(content!=NULL){
      
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
	char icname[14];
	char xname[16];
	if(index>0)
	  sprintf(vname1,"value_%d",index);
	else
	  sprintf(vname1,"value");

	if(i>0){
	  tmp1=getMap(content->content,cname);
	  sprintf(cname,"cache_file_%d",i);
	  sprintf(vname,"value_%d",i);
	  sprintf(sname,"size_%d",i);
	  sprintf(icname,"isCached_%d",i);
	  sprintf(xname,"Reference_%d",i);
	}else{
	  sprintf(cname,"cache_file");
	  sprintf(vname,"value");
	  sprintf(icname,"isCached");
	  sprintf(sname,"size");
	  sprintf(xname,"Reference");
	}

	map* tmap=getMapFromMaps(*m,"orequests",vname1);
	if((tmp1=getMap(content->content,xname))!=NULL && strcasecmp(tmap->value,tmp1->value)==0 ){
	  if(getMap(content->content,icname)==NULL){
	    
	    fcontent=(char*)malloc((hInternet->ihandle[index].nDataLen+1)*sizeof(char));
	    if(fcontent == NULL){
	      return errorException(*m, _("Unable to allocate memory."), "InternalError",NULL);
	    }
	    size_t dwRead;
	    InternetReadFile(hInternet->ihandle[index], 
			     (LPVOID)fcontent, 
			     hInternet->ihandle[index].nDataLen, 
			     &dwRead);
	    fcontent[hInternet->ihandle[index].nDataLen]=0;
	    fsize=hInternet->ihandle[index].nDataLen;
	    if(hInternet->ihandle[index].mimeType==NULL)
	      mimeType=strdup("none");
	    else
		  mimeType=strdup(hInternet->ihandle[index].mimeType);	      
	    
	    map* tmpMap=getMapOrFill(&content->content,vname,"");
	    free(tmpMap->value);
	    tmpMap->value=(char*)malloc((fsize+1)*sizeof(char));
	    if(tmpMap->value==NULL){
	      return errorException(*m, _("Unable to allocate memory."), "InternalError",NULL);
	    }
	    memcpy(tmpMap->value,fcontent,(fsize+1)*sizeof(char));
	    
	    char ltmp1[256];
	    sprintf(ltmp1,"%d",fsize);
	    map* tmp=getMapFromMaps(*m,"main","cacheDir");
	    if(tmp!=NULL){
	      char* md5str=getMd5(tmp1->value);
	      char* fname=(char*)malloc(sizeof(char)*(strlen(tmp->value)+strlen(md5str)+6));
	      sprintf(fname,"%s/%s.zca",tmp->value,md5str);
	      addToMap(content->content,cname,fname);
	      free(fname);
	    }
	    addToMap(content->content,sname,ltmp1);
	    addToCache(*m,tmp1->value,fcontent,mimeType,fsize);
	    free(fcontent);
	    free(mimeType);
	    dumpMaps(content);
	    index++;

	  }
	}
      }
      if(shouldClean>0){
	freeMap(&length);
	free(length);
      }
      
      content=content->next;
    }
    
  }
  return 0;
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
  char* fcontent;
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
    hInternet->waitingRequests[hInternet->nb]=strdup(url);
    InternetOpenUrl(hInternet,hInternet->waitingRequests[hInternet->nb],NULL,0,INTERNET_FLAG_NO_CACHE_WRITE,0);
    maps *oreq=getMaps(*m,"orequests");
    if(oreq==NULL){
      oreq=(maps*)malloc(MAPS_SIZE);
      oreq->name=zStrdup("orequests");
      oreq->content=createMap("value",url);
      oreq->next=NULL;
      addMapsToMaps(m,oreq);
      freeMaps(&oreq);
      free(oreq);
    }else{
      setMapArray(oreq->content,"value",hInternet->nb-1,url);
    }
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
  if(tmpMap->value==NULL)
    return errorException(*m, _("Unable to allocate memory."), "InternalError",NULL);
  memcpy(tmpMap->value,fcontent,(fsize+1)*sizeof(char));

  char ltmp1[256];
  sprintf(ltmp1,"%d",fsize);
  addToMap(*content,"size",ltmp1);
  if(cached==NULL){
    addToCache(*m,url,fcontent,mimeType,fsize);
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

/**
 * Read a file using the GDAL VSI API 
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param dataSource the datasource name to read
 * @warning make sure to free ressources returned by this function
 */
char *readVSIFile(maps* conf,const char* dataSource){
    VSILFILE * fichier=VSIFOpenL(dataSource,"rb");
    VSIStatBufL file_status;
    VSIStatL(dataSource, &file_status);
    if(fichier==NULL){
      char tmp[1024];
      sprintf(tmp,"Failed to open file %s for reading purpose. File seems empty %lld.",
	      dataSource,file_status.st_size);
      setMapInMaps(conf,"lenv","message",tmp);
      return NULL;
    }
    char *res1=(char *)malloc(file_status.st_size*sizeof(char));
    VSIFReadL(res1,1,file_status.st_size*sizeof(char),fichier);
    res1[file_status.st_size-1]=0;
    VSIFCloseL(fichier);
    VSIUnlink(dataSource);
    return res1;
}

/**
 * Extract the service identifier from the full service identifier
 * ie: 
 *  - Full service name: OTB.BandMath
 *  - Service name: BandMath
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param conf_dir the full path to the ZOO-Kernel directory
 * @param identifier the full service name (potentialy including a prefix, ie:
 *  Prefix.MyService)
 * @param buffer the resulting service identifier (without any prefix)
 */
void parseIdentifier(maps* conf,char* conf_dir,char *identifier,char* buffer){
  setMapInMaps(conf,"lenv","oIdentifier",identifier);
  char *lid=zStrdup(identifier);
  char *saveptr1;
  char *tmps1=strtok_r(lid,".",&saveptr1);
  int level=0;
  char key[25];
  char levels[18];
  while(tmps1!=NULL){
    char *test=zStrdup(tmps1);
    char* tmps2=(char*)malloc((strlen(test)+2)*sizeof(char));
    sprintf(key,"sprefix_%d",level);
    sprintf(tmps2,"%s.",test);
    sprintf(levels,"%d",level);
    setMapInMaps(conf,"lenv","level",levels);
    setMapInMaps(conf,"lenv",key,tmps2);
    free(tmps2);
    free(test);
    level++;
    tmps1=strtok_r(NULL,".",&saveptr1);
  }
  int i=0;
  sprintf(buffer,"%s",conf_dir);
  for(i=0;i<level;i++){
    char *tmp0=zStrdup(buffer);
    sprintf(key,"sprefix_%d",i);
    map* tmp00=getMapFromMaps(conf,"lenv",key);
    if(tmp00!=NULL)
      sprintf(buffer,"%s/%s",tmp0,tmp00->value);
    free(tmp0);
    buffer[strlen(buffer)-1]=0;
    if(i+1<level){
      map* tmpMap=getMapFromMaps(conf,"lenv","metapath");
      if(tmpMap==NULL || strlen(tmpMap->value)==0){
	char *tmp01=zStrdup(tmp00->value);
	tmp01[strlen(tmp01)-1]=0;
	setMapInMaps(conf,"lenv","metapath",tmp01);
	free(tmp01);
	tmp01=NULL;
      }
      else{
	if(tmp00!=NULL && tmpMap!=NULL){
	  char *tmp00s=zStrdup(tmp00->value);
	  tmp00s[strlen(tmp00s)-1]=0;
	  char *value=(char*)malloc((strlen(tmp00s)+strlen(tmpMap->value)+2)*sizeof(char));
	  sprintf(value,"%s/%s",tmpMap->value,tmp00s);
	  setMapInMaps(conf,"lenv","metapath",value);
	  free(value);
	  free(tmp00s);
	  value=NULL;
	}
      }
    }else{
      char *tmp01=zStrdup(tmp00->value);
      tmp01[strlen(tmp01)-1]=0;
      setMapInMaps(conf,"lenv","Identifier",tmp01);
      free(tmp01);
    }
  }
  char *tmp0=zStrdup(buffer);
  sprintf(buffer,"%s.zcfg",tmp0);
  free(tmp0);
  free(lid);
}

/**
 * Update the status of an ongoing service 
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param percentCompleted percentage of completude of execution of the service
 * @param message information about the current step executed
 * @return the value of _updateStatus
 * @see _updateStatus
 */
int updateStatus( maps* conf, const int percentCompleted, const char* message ){
  char tmp[4];
  snprintf(tmp,4,"%d",percentCompleted);
  setMapInMaps( conf, "lenv", "status", tmp );
  setMapInMaps( conf, "lenv", "message", message);
  return _updateStatus( conf );
}

/**
 * Access an input value 
 *
 * @param inputs the maps to search for the input value
 * @param parameterName the input name to fetch the value
 * @param numberOfBytes the resulting size of the value to add (for binary
 *  values), -1 for basic char* data
 * @return a pointer to the input value if found, NULL in other case.
 */
char* getInputValue( maps* inputs, const char* parameterName, size_t* numberOfBytes){
  map* res=getMapFromMaps(inputs,parameterName,"value");
  if(res!=NULL){
    map* size=getMapFromMaps(inputs,parameterName,"size");
    if(size!=NULL){
      *numberOfBytes=(size_t)atoi(size->value);
      return res->value;
    }else{
      *numberOfBytes=strlen(res->value);
      return res->value;
    }
  }
  return NULL;
}

/**
 * Set an output value 
 *
 * @param outputs the maps to define the output value
 * @param parameterName the output name to set the value
 * @param data the value to set
 * @param numberOfBytes size of the value to add (for binary values), -1 for
 *  basic char* data
 * @return 0
 */
int  setOutputValue( maps* outputs, const char* parameterName, char* data, size_t numberOfBytes ){
  if(numberOfBytes==-1){
    setMapInMaps(outputs,parameterName,"value",data);
  }else{
    char size[1024];
    map* tmp=getMapFromMaps(outputs,parameterName,"value");
    if(tmp==NULL){
      setMapInMaps(outputs,parameterName,"value","");
      tmp=getMapFromMaps(outputs,parameterName,"value");
    }
    free(tmp->value);
    tmp->value=(char*) malloc((numberOfBytes+1)*sizeof(char));
    memcpy(tmp->value,data,numberOfBytes);
    sprintf(size,"%lu",numberOfBytes);
    setMapInMaps(outputs,parameterName,"size",size);
  }
  return 0;
}

/**
 * Verify if a parameter value is valid.
 * 
 * @param request the request map
 * @param res the error map potentially generated
 * @param toCheck the parameter to use
 * @param avalues the acceptable values (or null if testing only for presence)
 * @param mandatory verify the presence of the parameter if mandatory > 0 
 */
void checkValidValue(map* request,map** res,const char* toCheck,const char** avalues,int mandatory){
  map* lres=*res;
  map* r_inputs = getMap (request,toCheck);
  if (r_inputs == NULL){
    if(mandatory>0){
      char *replace=_("Mandatory parameter <%s> was not specified");
      char *message=(char*)malloc((strlen(replace)+strlen(toCheck)+1)*sizeof(char));
      sprintf(message,replace,toCheck);
      if(lres==NULL){
	lres=createMap("code","MissingParameterValue");
	addToMap(lres,"text",message);
	addToMap(lres,"locator",toCheck);       
      }else{
	int length=1;
	map* len=getMap(lres,"length");
	if(len!=NULL){
	  length=atoi(len->value);
	}
	setMapArray(lres,"text",length,message);
	setMapArray(lres,"locator",length,toCheck);
	setMapArray(lres,"code",length,"MissingParameter");
      }
      free(message);
    }
  }else{
    if(avalues==NULL)
      return;
    int nb=0;
    int hasValidValue=-1;
    while(avalues[nb]!=NULL){
      if(strcasecmp(avalues[nb],r_inputs->value)==0){
	hasValidValue=1;
	break;
      }
      nb++;
    }
    if(hasValidValue<0){
      char *replace=_("Ununderstood <%s> value, %s %s the only acceptable value.");
      nb=0;
      char *vvalues=NULL;
      char* num=_("is");
      while(avalues[nb]!=NULL){
	char *tvalues;
	if(vvalues==NULL){
	  vvalues=(char*)malloc((strlen(avalues[nb])+3)*sizeof(char));
	  sprintf(vvalues,"%s",avalues[nb]);
	}
	else{
	  tvalues=zStrdup(vvalues);
	  vvalues=(char*)realloc(vvalues,(strlen(tvalues)+strlen(avalues[nb])+3)*sizeof(char));
	  sprintf(vvalues,"%s, %s",tvalues,avalues[nb]);
	  free(tvalues);
	  num=_("are");
	}
	nb++;
      }
      char *message=(char*)malloc((strlen(replace)+strlen(num)+strlen(vvalues)+strlen(toCheck)+1)*sizeof(char));
      sprintf(message,replace,toCheck,vvalues,num);
      if(lres==NULL){
	lres=createMap("code","InvalidParameterValue");
	addToMap(lres,"text",message);
	addToMap(lres,"locator",toCheck);       
      }else{
	int length=1;
	map* len=getMap(lres,"length");
	if(len!=NULL){
	  length=atoi(len->value);
	}
	setMapArray(lres,"text",length,message);
	setMapArray(lres,"locator",length,toCheck);
	setMapArray(lres,"code",length,"InvalidParameterValue");
      }
    }
  }
  if(lres!=NULL){
    *res=lres;
  }
}

/**
 * Access the last error message returned by the OS when trying to dynamically
 * load a shared library.
 *
 * @return the last error message
 * @warning The character string returned from getLastErrorMessage resides
 * in a static buffer. The application should not write to this
 * buffer or attempt to free() it.
 */ 
char* getLastErrorMessage() {                                              
#ifdef WIN32
  LPVOID lpMsgBuf;
  DWORD errCode = GetLastError();
  static char msg[ERROR_MSG_MAX_LENGTH];
  size_t i;
  
  DWORD length = FormatMessage(
			       FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			       FORMAT_MESSAGE_FROM_SYSTEM |
			       FORMAT_MESSAGE_IGNORE_INSERTS,
			       NULL,
			       errCode,
			       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			       (LPTSTR) &lpMsgBuf,
			       0, NULL );	
  
#ifdef UNICODE		
  wcstombs_s( &i, msg, ERROR_MSG_MAX_LENGTH,
	      (wchar_t*) lpMsgBuf, _TRUNCATE );
#else
  strcpy_s( msg, ERROR_MSG_MAX_LENGTH,
	    (char *) lpMsgBuf );		
#endif	
  LocalFree(lpMsgBuf);
  
  return msg;
#else
  return dlerror();
#endif
}
