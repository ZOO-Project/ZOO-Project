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

#include <sys/stat.h>
#include <sys/types.h>
#include "cgic.h"
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
#include <xlocale.h>
#endif

#include "service.h"

#if defined(macintosh) || (defined(__MACH__) && defined(__APPLE__))

#include <CoreServices/CoreServices.h>
#include <SystemConfiguration/SystemConfiguration.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

  char *readVSIFile(maps*,const char*);
  int  setOutputValue( maps*, const char*, char*, size_t);
  char* getInputValue( maps*,const char*,size_t*);

  void unhandleStatus(maps*);
  int _updateStatus(maps*);
  char* _getStatus(maps*,int);
  char* getStatus(int);
  int updateStatus( maps*,const int,const char*);
  int removeShmLock(maps*, int);
  /**
   * Cross platform type used for Lock identifier
   */
#ifndef WIN32
#define semid int
#else
#define semid HANDLE
#endif
  semid getShmLockId(maps*,int);
  int lockShm(semid);
  int unlockShm(semid);

#ifdef USE_JS
#endif
  
#ifdef __cplusplus
}
#endif

#endif
