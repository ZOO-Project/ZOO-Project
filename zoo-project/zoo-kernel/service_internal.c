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

#include "fcgi_stdio.h"
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

#define ERROR_MSG_MAX_LENGTH 1024

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
/**
 * arg for semctl system calls. 
 */
union semun {
  int val; //!< value for SETVAL 
  struct semid_ds *buf; //!< buffer for IPC_STAT & IPC_SET
  ushort *array; //!< array for GETALL & SETALL
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

