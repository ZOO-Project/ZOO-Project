/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2013 GeoLabs SARL
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

#ifndef ZOO_SERVICE_INTERNAL_H
#define ZOO_SERVICE_INTERNAL_H 1

#pragma once 

/**
 * The default service url (overriden by serverAddress)
 */
#define DEFAULT_SERVICE_URL "http://www.zoo-project.org/"
/**
 * The time size
 */
#define TIME_SIZE 40

#include <libintl.h>
#include <locale.h>
/**
 * ZOO-Kernel internal messages translation function
 */
#define _(String) dgettext ("zoo-kernel",String)
/**
 * ZOO-Services messages translation function
 */
#define _ss(String) dgettext ("zoo-services",String)

/**
 * ZOO-Kernel was unable to create a lock
 */
#define ZOO_LOCK_CREATE_FAILED -4
/**
 * ZOO-Kernel was unable to acquire a lock
 */
#define ZOO_LOCK_ACQUIRE_FAILED -5
/**
 * ZOO-Kernel was unable to release a lock
 */
#define ZOO_LOCK_RELEASE_FAILED -6
/**
 * Number of time the ZOO-Kernel will try to acquire lock
 */
#define ZOO_LOCK_MAX_RETRY 180

#include <sys/stat.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#else
#include <direct.h>
#endif
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#ifndef USE_RUBY
#include <unistd.h>
#endif
#ifndef WIN32
#include <locale.h>
#endif

#include <fcntl.h>
  
#include "service.h"

#if defined(macintosh) || (defined(__MACH__) && defined(__APPLE__))

#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>

#endif

#ifdef WIN32
// fcntl flock definitions
#define F_SETLK  8   // Non-Blocking set or clear a lock
#define F_SETLKW 9   // Blocking set or clear a lock
#define F_GETLK 10
#define F_RDLCK  1   // read lock
#define F_WRLCK  2   // write lock
#define F_UNLCK  3   // remove lock
struct flock {
    short l_type;   // F_RDLCK, F_WRLCK, or F_UNLCK
    short l_whence; // flag to choose starting offset, must be SEEK_SET
    long  l_start;  // relative offset, in bytes, must be 0
    long  l_len;    // length, in bytes; 0 means lock to EOF, must be 0
    short l_pid;    // unused (returned with the unsupported F_GETLK)
    short l_xxx;    // reserved for future use
};
#endif

/**
 * The lock structure used by the ZOO-Kernel to ensure atomicity of operations
 *
 */ 
typedef struct zooLock{
  struct flock lock; //!< The lock
  FILE* lockfile;    //!< The pointer to the lock file
  char* filename;    //!< The filename to lock
} zooLock;

static zooLock** zoo_file_locks=NULL;
static int zoo_file_locks_cnt=0;

#ifdef __cplusplus
extern "C" {
#endif

  
  ZOO_DLL_EXPORT char *readVSIFile(maps*,const char*);
  ZOO_DLL_EXPORT int  setOutputValue( maps*, const char*, char*, size_t);
  ZOO_DLL_EXPORT char* getInputValue( maps*,const char*,size_t*);

  ZOO_DLL_EXPORT struct zooLock* lockFile(maps*,const char*,const char);
  ZOO_DLL_EXPORT int unlockFile(maps*,struct zooLock*);

  ZOO_DLL_EXPORT void unhandleStatus(maps*);
  ZOO_DLL_EXPORT int _updateStatus(maps*);
  ZOO_DLL_EXPORT char* _getStatus(maps*,char*);
  ZOO_DLL_EXPORT char* _getStatusFile(maps*,char*);
  ZOO_DLL_EXPORT char* getStatus(int);
  ZOO_DLL_EXPORT char* getStatusId(maps*,char*);

  ZOO_DLL_EXPORT int updateStatus( maps*,const int,const char*);
  ZOO_DLL_EXPORT int removeShmLock(maps*, int);
  /**
   * Cross platform type used for Lock identifier
   */
#ifndef WIN32
#define semid int
#else
#include <windows.h>
#define semid HANDLE
#endif
  ZOO_DLL_EXPORT semid acquireLock(maps*);
  ZOO_DLL_EXPORT semid getShmLockId(maps*,int);
  ZOO_DLL_EXPORT int lockShm(semid);
  ZOO_DLL_EXPORT int unlockShm(semid);

  ZOO_DLL_EXPORT char* file_exists(const char* dir, const char* name); 

#ifdef __cplusplus
}
#endif

#endif
