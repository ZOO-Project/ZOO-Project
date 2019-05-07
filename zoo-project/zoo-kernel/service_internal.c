/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2018 GeoLabs SARL
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

#define _LARGEFILE64_SOURCE 1
#ifdef USE_MS
#include "service_internal_ms.h"
#else
#include "cpl_vsi.h"
#endif
#include "service_internal.h"

#ifdef WIN32
// ref. https://docs.microsoft.com/en-us/windows/desktop/fileio/locking-and-unlocking-byte-ranges-in-files
__inline int fcntl(int fd, int cmd, ...)
{
  va_list a;
  va_start(a, cmd);
  switch(cmd)
    {
    case F_SETLK:
      {
	HANDLE h = (HANDLE)_get_osfhandle(fd);
	struct flock* l= va_arg(a, struct flock*);
	OVERLAPPED sOverlapped;
	sOverlapped.Offset = 0;
	sOverlapped.OffsetHigh = 0;
	switch(l->l_type)
	  {
	  case F_RDLCK:
	    {
	      if (!LockFileEx(h, LOCKFILE_FAIL_IMMEDIATELY, 0, l->l_len, 0, &sOverlapped)) 
		{
		  _set_errno(GetLastError() == ERROR_LOCK_VIOLATION ? EAGAIN : EBADF);
		  return -1;
		}
	    }
	    break;
	  case F_WRLCK:
	    {
	      if (!LockFileEx(h, LOCKFILE_FAIL_IMMEDIATELY|LOCKFILE_EXCLUSIVE_LOCK, 0, l->l_len, 0, &sOverlapped))
		{
		  _set_errno(GetLastError() == ERROR_LOCK_VIOLATION ? EAGAIN : EBADF);
		  return -1;
		}
	    }
	    break;
	  case F_UNLCK:
	    {
	      UnlockFileEx(h, 0, l->l_len, 0, &sOverlapped);
	    }
	    break;
	  default:
	    _set_errno(ENOTSUP);
	    return -1;
	  }
      }
      break;
    default:
      _set_errno(ENOTSUP);
      return -1;
    }
  return 0;
}
#endif
#define ERROR_MSG_MAX_LENGTH 1024

/**
 * Lock a file for read, write and upload.
 * @param conf the main configuration maps
 * @param filename the file to lock
 * @param mode define access: 'r' for read, 'w' for write
 * @return a new zooLock structure on sucess, NULL on failure 
 */
struct zooLock* lockFile(maps* conf,const char* filename,const char mode){
  struct stat f_status;
  int itn=0;
  int s;
  struct zooLock* myLock=(struct zooLock*)malloc(sizeof(struct flock)+sizeof(FILE*)+sizeof(char*));
  int len=6;
  char *myTemplate="%s.lock";
  int res=-1;
 retryLockFile:
  myLock->filename=(char*)malloc((strlen(filename)+len)*sizeof(char));
  sprintf(myLock->filename,myTemplate,filename);
  s=stat(myLock->filename, &f_status);
  if(s==0 && mode!='r'){
    if(itn<ZOO_LOCK_MAX_RETRY){
      itn++;
#ifdef DEBUG
      fprintf(stderr,"(%d) Wait for write lock on %s, tried %d times (sleep) ... \n",zGetpid(),myLock->filename,itn);
      fflush(stderr);
#endif
      zSleep(5);
      free(myLock->filename);
      goto retryLockFile;
    }else{
      free(myLock->filename);
      free(myLock);
      return NULL;
    }
  }else{
    char local_mode[3];
    memset(local_mode,0,3);
    if(mode=='w')
      sprintf(local_mode,"%c+",mode);
    else
      sprintf(local_mode,"%c",mode);
    myLock->lockfile=fopen(myLock->filename,local_mode);
    char tmp[512];
    sprintf(tmp,"%d",zGetpid());
    if(myLock->lockfile==NULL){
      myLock->lockfile=fopen(myLock->filename,"w+");
      fwrite(tmp,sizeof(char),strlen(tmp),myLock->lockfile);
      fflush(myLock->lockfile);
      fclose(myLock->lockfile);
      myLock->lockfile=fopen(myLock->filename,local_mode);
    }/*else
       fprintf(stderr,"%s %d %d\n",__FILE__,__LINE__,(myLock->lockfile==NULL));*/
    if(mode!='r'){
      fwrite(tmp,sizeof(char),strlen(tmp),myLock->lockfile);
      fflush(myLock->lockfile);
    }
    int cnt=0;
    if(mode=='r'){
      myLock->lock.l_type = F_RDLCK;
    }else
      myLock->lock.l_type = F_WRLCK;
    myLock->lock.l_whence = 0;
    myLock->lock.l_start = 0;
    myLock->lock.l_len = strlen(tmp)*sizeof(char);
    while (true) {
      if((res=fcntl(fileno(myLock->lockfile), F_SETLK, &(myLock->lock)))==-1 &&
	 (errno==EAGAIN || errno==EACCES)){
	  if(cnt >= ZOO_LOCK_MAX_RETRY){
	    char message[51];	  
	    sprintf(message,"Unable to get the lock after %d attempts.\n",cnt);
	    setMapInMaps(conf,"lenv","message",message);
	    fclose(myLock->lockfile);
	    free(myLock->filename);
	    free(myLock);
	    return NULL;
	  }
#ifdef DEBUG
	  fprintf(stderr,"(%d) Wait for lock on  %s, tried %d times ... \n",zGetpid(),myLock->filename,cnt);
	  fflush(stderr);
#endif
	  zSleep(1);
	  cnt++;
	}else
	   break;
    }
    if(res<0){
      char *tmp;
      if(errno==EBADF)
	tmp="Either: the filedes argument is invalid; you requested a read lock but the filedes is not open for read access; or, you requested a write lock but the filedes is not open for write access.";
      else
	if(errno==EINVAL)
	  tmp="Either the lockp argument doesn’t specify valid lock information, or the file associated with filedes doesn’t support locks.";
	else
	  tmp="The system has run out of file lock resources; there are already too many file locks in place.";
#ifdef DEBUG
      fprintf(stderr,"Unable to get the lock on %s due to the following error: %s\n",myLock->filename,tmp);
#endif
      return NULL;
    }
    return myLock;
  }
}

/**
 * Remove a lock.
 * @param conf the main configuration maps
 * @param s the zooLock structure
 * @return 0 on success, -1 on failure.
 */
int unlockFile(maps* conf,struct zooLock* s){
  int res=-1;
  if(s!=NULL){
    s->lock.l_type = F_UNLCK;
    res=fcntl(fileno(s->lockfile), F_SETLK, &s->lock);
    if(res==-1)
      return res;
    fclose(s->lockfile);
#ifndef WIN32
    // Check if there is any process locking a file and delete the lock if not.
    s->lock.l_type = F_WRLCK;
    if(fcntl(fileno(s->lockfile), F_GETLK, &s->lock)!=-1 && s->lock.l_type == F_UNLCK){
#endif
      zUnlink(s->filename);
#ifndef WIN32
    }
#endif
    free(s->filename);
    free(s);
  }
  return res;
}

#ifndef RELY_ON_DB
#include "dirent.h"

/**
 * Read the sid file attached of a service if any
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (temporary/final result)
 */
char* getStatusId(maps* conf,char* pid){
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  char* fbkpid =
    (char *)
    malloc ((strlen (r_inputs->value) + strlen (pid) + 7) * sizeof (char));
  sprintf (fbkpid, "%s/%s.sid", r_inputs->value, pid);
  FILE* f0 = fopen (fbkpid, "r");
  if(f0!=NULL){
    long flen;
    char *fcontent;
    fseek (f0, 0, SEEK_END);
    flen = ftell (f0);
    fseek (f0, 0, SEEK_SET);
    fcontent = (char *) malloc ((flen + 1) * sizeof (char));
    fread(fcontent,flen,1,f0);
    fcontent[flen]=0;
    fclose(f0);
    return fcontent;
  }else
    return NULL;
}

/**
 * Acquire the global lock
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @return a semid 
 */
semid acquireLock(maps* conf){
  semid lockid;
  int itn=0;
 toRetry1:
  lockid=getShmLockId(conf,1);
  if(
#ifdef WIN32
     lockid==NULL
#else
     lockid<0
#endif
     ){
#ifdef WIN32
    return NULL;
#else
    return -1;
#endif
  }
  if(lockShm(lockid)<0){
#ifdef WIN32
      return NULL;
#else
    if(itn<ZOO_LOCK_MAX_RETRY){
      itn++;
      goto toRetry1;
    }else
      return -1;
#endif
  }else
    return lockid;
}

/**
 * Read the cache file of a running service 
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (temporary/final result)
 */
char* _getStatusFile(maps* conf,char* pid){
  map* tmpTmap = getMapFromMaps (conf, "main", "tmpPath");

  struct dirent *dp;
  DIR *dirp = opendir(tmpTmap->value);
  char fileName[1024];
  int hasFile=-1;
  if(dirp!=NULL){
    char tmp[128];
    sprintf(tmp,"_%s.xml",pid);
    while ((dp = readdir(dirp)) != NULL){
#ifdef DEBUG
      fprintf(stderr,"File : %s searched : %s\n",dp->d_name,tmp);
#endif
      if(strstr(dp->d_name,"final_")==0 && strstr(dp->d_name,tmp)!=0){
	sprintf(fileName,"%s/%s",tmpTmap->value,dp->d_name);
	hasFile=1;
	break;
      }
    }
  }
  if(hasFile>0){
    semid lockid;
    char* stat=getStatusId(conf,pid);
    if(stat!=NULL){
      setMapInMaps(conf,"lenv","lid",stat);
      lockid=acquireLock(conf);
      if(lockid<0)
	return NULL;
    }

    //FILE* f0 = fopen (fileName, "r");
    // knut: open file in binary mode to avoid conversion of line endings (yielding extra bytes) on Windows platforms
    FILE* f0 = fopen(fileName, "rb"); 
    if(f0!=NULL){
      fseek (f0, 0, SEEK_END);
      long flen = ftell (f0);
      fseek (f0, 0, SEEK_SET);
      char *tmps1 = (char *) malloc ((flen + 1) * sizeof (char));
      fread(tmps1,flen,1,f0);
      tmps1[flen]=0;
      fclose(f0);
      if(stat!=NULL){
	unlockShm(lockid);
	free(stat);
      }
      return tmps1;
    }
    else{
      if(stat!=NULL){
	unlockShm(lockid);
	free(stat);
      }
      return NULL;
    }
  }
  else
    return NULL;
}

/**
 * Get the ongoing status of a running service 
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (MESSAGE|POURCENTAGE)
 */
char* _getStatus(maps* conf,char* lid){
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  char* fbkpid =
    (char *)
    malloc ((strlen (r_inputs->value) + strlen (lid) + 9) * sizeof (char));
  sprintf (fbkpid, "%s/%s.status", r_inputs->value, lid);
  FILE* f0 = fopen (fbkpid, "r");
  if(f0!=NULL){    
    semid lockid = NULL;
    char* stat;
    long flen;
    stat=getStatusId(conf,lid);
    if(stat!=NULL){
      setMapInMaps(conf,"lenv","lid",stat);
      lockid=acquireLock(conf);
      if(lockid<0)
	return NULL;
    }
    fseek (f0, 0, SEEK_END);
    flen = ftell (f0);
    if(flen>0){
      char *fcontent;
      fseek (f0, 0, SEEK_SET);
      fcontent = (char *) malloc ((flen + 1) * sizeof (char));
      fread(fcontent,flen,1,f0);
      fcontent[flen]=0;
      fclose(f0);
      free(fbkpid);
      if(stat!=NULL){
#ifndef WIN32
	removeShmLock(conf,1);
#else
	unlockShm(lockid);
#endif
	free(stat);
      }
      return fcontent;
    }
    fclose(f0);
    free(fbkpid);
    if(stat!=NULL){
      removeShmLock(conf,1);
      free(stat);
    }
    return NULL;
  }else{
    free(fbkpid);
    char* stat=getStatusId(conf,lid);
    setMapInMaps(conf,"lenv","lid",stat);
    removeShmLock(conf,1);
    return NULL;
  }
}

/**
 * Stop handling status repport.
 *
 * @param conf the map containing the setting of the main.cfg file
 */
void unhandleStatus(maps *conf){	
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  map* usid = getMapFromMaps (conf, "lenv", "usid");
  char* fbkpid =
    (char *) malloc ((strlen (r_inputs->value) + strlen (usid->value) + 9) 
		     * sizeof (char));
  sprintf (fbkpid, "%s/%s.status", r_inputs->value, usid->value);
  zUnlink(fbkpid);
  free(fbkpid);
}

/**
 * Update the current status of the running service.
 *
 * @see acquireLock, lockShm
 * @param conf the map containing the setting of the main.cfg file
 * @return 0 on success, -2 if shmget failed, -1 if shmat failed
 */
int _updateStatus(maps *conf){
	
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  map* sid = getMapFromMaps (conf, "lenv", "usid");
  
  char* fbkpid =
    (char *)
    malloc ((strlen (r_inputs->value) + strlen (sid->value) + 9) * sizeof (char));
  sprintf (fbkpid, "%s/%s.status", r_inputs->value, sid->value);
  map* status=getMapFromMaps(conf,"lenv","status");
  map* msg=getMapFromMaps(conf,"lenv","message");
  if(status!=NULL && msg!=NULL &&
     status->value!=NULL && msg->value!=NULL && 
     strlen(status->value)>0 && strlen(msg->value)>1){    
    semid lockid = NULL;
	
    char* stat=getStatusId(conf,sid->value);
    if(stat!=NULL){
      lockid=acquireLock(conf);
      if(lockid<0){
	dumpMap(status);
	return ZOO_LOCK_ACQUIRE_FAILED;
      }
    }
    FILE* fstatus=fopen(fbkpid,"w");
    if(fstatus!=NULL){
      fprintf(fstatus,"%s|%s",status->value,msg->value);
      fflush(fstatus);
      fclose(fstatus);
    }
    if(stat!=NULL){
      unlockShm(lockid);
      free(stat);
    }
  }

  return 0;
}

#endif

#ifdef WIN32

#define SHMEMSIZE 4096

size_t getKeyValue(maps* conf, char* key, size_t length){
  if(conf==NULL) {
    strncpy(key, "700666", length);
    return strlen(key);
  }
  
  map *tmpMap=getMapFromMaps(conf,"lenv","lid");
  if(tmpMap==NULL)
    tmpMap=getMapFromMaps(conf,"lenv","osid");

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
#ifdef DEBUG
    fprintf(stderr,"Unable to remove semaphore: %s\n", getLastErrorMessage());
#endif
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
 * Set in the pre-allocated key the zoo_sem_[OSID] string 
 * where [OSID] is the lid (if any) or osid value from the [lenv] section.
 *
 * @param conf the map containing the setting of the main.cfg file
 */
int getKeyValue(maps* conf){
  if(conf==NULL)
     return 700666;
  map *tmpMap=getMapFromMaps(conf,"lenv","lid");
  if(tmpMap==NULL)
    tmpMap=getMapFromMaps(conf,"lenv","osid");
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
	setMapInMaps(conf,"lenv","semaphore","Created");
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
	      zSleep(1000);
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
	setMapInMaps(conf,"lenv","semaphore","Acquired");
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
#ifdef DEBUG
    perror("semctl remove");
#endif
    return -1;
  }
#ifdef DEBUG
  fprintf(stderr,"Semaphore removed!\n");
#endif
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
#ifdef DEBUG
    perror("semop lock");
#endif
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
#ifdef DEBUG
    perror("semop unlock");
#endif
    return -1;
  }
  return 0;
}

/**
 * Get the current status of the running service.
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
 * @warning make sure to free resources returned by this function
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

/**
 * Check if file exists in specified folder
 *
 * @param dir the folder in which to search for file
 * @param name the name of the file (not full path) 
 * @return a character string with the full path [dir/name], or NULL if the file does not exist
 *
 * @attention Caller is responsible for applying free() to the returned pointer
 */
char* file_exists(const char* dir, const char* name) {
	const char* d = (dir != NULL ? dir : ".");
	if (name != NULL) {
		size_t length = strlen(d) + strlen(name) + 2; // including file separator and \0 character
		char* path = (char*)calloc(length, sizeof(char));
		snprintf(path, length, "%s/%s", d, name);

		struct stat buffer;
		if (stat(path, &buffer) != 0) {
			free(path);
			path = NULL;
		}
		return path;
	}
	else {
		return NULL;
	}
}
