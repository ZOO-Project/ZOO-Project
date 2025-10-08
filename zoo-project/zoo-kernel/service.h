/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2024 GeoLabs SARL
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

#include "version.h"
#ifdef WIN32
#define ZOO_DLL_EXPORT __declspec( dllexport )
#else
#define ZOO_DLL_EXPORT 
#endif

// ISO8601 compatible date format (RFC 3339)
#define zDateFormat "%Y-%m-%dT%H:%M:%SZ"

// Debug Messages date format
#define zLogDateFormat "%Y-%m-%d %H:%M:%S"

#include <sys/time.h>
 // knut: add bool if necessary
#ifndef __cplusplus
#ifndef WIN32
#include <stdbool.h>
#else
typedef int bool;
#define false 0
#define true 1	
#endif
#endif
#ifndef __bool_true_false_are_defined
#define __bool_true_false_are_defined 1
#endif

#ifdef WIN32
#define strtok_r strtok_s
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif
#define zStrdup _strdup
#define zMkdir _mkdir
#define zGetpid _getpid
#define zOpen _open
#define zClose _close
#define zUnlink _unlink
#define zDup _dup
#define zDup2 _dup2
#define zWrite _write
#define zSleep Sleep
#define zGetCwd _getcwd
#include <sys/timeb.h>

struct ztimeval {
  long tv_sec; /* seconds */
  long tv_usec; /* and microseconds */
};
static int zGettimeofday(struct ztimeval* tp, void* tzp)
{
  if (tp == 0) {
    return -1;
  }
  
  struct _timeb theTime;
  _ftime(&theTime);
  tp->tv_sec = theTime.time;
  tp->tv_usec = theTime.millitm * 1000;
  
  return 0; // The gettimeofday() function shall return 0 on success
}

#define zStatStruct struct _stati64
#define zStat _stati64

#else
#include <sys/stat.h>
/**
 * The crossplatform strdup alias
 */
#define zStrdup strdup
/**
 * The crossplatform mkdir alias
 */
static int zMkdir(const char* pccPath){
  return mkdir(pccPath,0777);
}
/**
 * The crossplatform open alias
 */
#define zOpen open
/**
 * The crossplatform close alias
 */
#define zClose close
/**
 * The crossplatform unlink alias
 */
#define zUnlink unlink
/**
 * The crossplatform dup alias
 */
#define zDup dup
/**
 * The crossplatform dup2 alias
 */
#define zDup2 dup2
/**
 * The crossplatform write alias
 */
#define zWrite write
#include "unistd.h"
/**
 * The crossplatform sleep alias
 */
static int zSleep(const long millisecond){
  return usleep(millisecond*1000);
}
/**
 * The crossplatform gettimeofday alias
 */
#define zGettimeofday gettimeofday
/**
 * The crossplatform timeval alias
 */
#define ztimeval timeval
/**
 * The crossplatform getpid alias
 */
#define zGetpid getpid

#define zStatStruct struct stat64
#define zStat stat64

#define zGetCwd getcwd

#endif 

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#ifdef USE_MS
#include <mapserver.h>
#endif
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include <stdio.h>

#include <string.h>
#ifndef WIN32
#include <ctype.h>
#include <stdbool.h>
#endif

/**
 * The global accepted status for a service
 */
#define SERVICE_ACCEPTED 0
/**
 * The global started status for a service
 */
#define SERVICE_STARTED 1
/**
 * The global paused status for a service
 */
#define SERVICE_PAUSED 2
/**
 * The global succeeded status for a service
 */
#define SERVICE_SUCCEEDED 3
/**
 * The global failed status for a service
 */
#define SERVICE_FAILED 4
/**
 * The global dismissed status for a service
 */
#define SERVICE_DISMISSED 5
/**
 * The global deployed status for a service
 */
#define SERVICE_DEPLOYED 6
/**
 * The global undelployed status for a service
 */
#define SERVICE_UNDEPLOYED 7

/**
 * The time size, used to create a buffer for time string
 */
#define TIME_SIZE 40

/**
 * The global debug level: DEBUG
 */
#define ZOO_DEBUG_LEVEL_TRACE 0
/**
 * The global debug level: DEBUG
 */
#define ZOO_DEBUG_LEVEL_DEBUG 1
/**
 * The global debug level: INFO
 */
#define ZOO_DEBUG_LEVEL_INFO 2
/**
 * The global debug level: WARN
 */
#define ZOO_DEBUG_LEVEL_SUCCESS 3
/**
 * The global debug level: WARN
 */
#define ZOO_DEBUG_LEVEL_WARNING 4
/**
 * The global debug level: ERROR
 */
#define ZOO_DEBUG_LEVEL_ERROR 5
/**
 * The global debug level: FATAL
 */
#define ZOO_DEBUG_LEVEL_FATAL 6

/**
 * The global log level names
 */
static const char* pccLogLevel[7]={
  "TRACE",
  "DEBUG",
  "INFO",
  "SUCCESS",
  "WARNING",
  "ERROR",
  "CRITICAL"
};

/**
 * The global log level
 */
static int iZooLogLevel=1;

/**
 * The global log level
 */
static int iMinZooLogLevel=0;

/**
 * The _ZOO_DEBUG macro print a message with date time, process id (UNIX pid),
 * file name, function name, and line number.
 *
 * The message uses the ZOO_LOG_FORMAT to format the output.
 *
 * The values that should be included in this specific order in the
 * ZOO_LOG_FORMAT are:
 *
 *  * %d: the date time
 *  * %l: the log level
 *  * %p: the process identifier
 *  * %f: the file name
 *  * %u: the function name
 *  * %i: the line number
 *  * %m: the message
 *
 *  In case the ZOO_LOG_FORMAT is not defined, the default format is used:
 *  "%d %l %p %f:%u %m\n"
 *
 * @param message the message to print
 */
#define ZOO_DEBUG(message) _ZOO_DEBUG(message,__FILE__,__func__,__LINE__)

/**
 * The ZOO_COLOR_* variables are used to create colored output in the console.
 *
 * To use a color you should combine:
 *  * ZOO_COLOR_START or ZOO_COLOR_START_BOLD
 *  * ZOO_COLOR_STANDARD or ZOO_COLOR_HIGH_INTENSITY
 *  * ZOO_COLOR_BLACK, ZOO_COLOR_RED, ZOO_COLOR_GREEN, ZOO_COLOR_YELLOW,
 *    ZOO_COLOR_BLUE, ZOO_COLOR_MAGENTA, ZOO_COLOR_CYAN, ZOO_COLOR_WHITE
 *
 * To set the background color you should combine:
 * * ZOO_COLOR_START_BACKGROUND or ZOO_COLOR_START_BACKGROUND_HIGH_INTENSITY
 * * ZOO_COLOR_BLACK, ZOO_COLOR_RED, ZOO_COLOR_GREEN, ZOO_COLOR_YELLOW,
 *   ZOO_COLOR_BLUE, ZOO_COLOR_MAGENTA, ZOO_COLOR_CYAN, ZOO_COLOR_WHITE
 *
 * To reset the color formatting you should use ZOO_COLOR_RESET.
 *
 * The ZOO_COLOR_START or ZOO_COLOR_START_BOLD are used to start the color 
 * formatting.
 * The ZOO_COLOR_STANDARD or ZOO_COLOR_HIGH_INTENSITY are used to set the color
 * intensity.
 * The ZOO_COLOR_BLACK, ZOO_COLOR_RED, ZOO_COLOR_GREEN, ZOO_COLOR_YELLOW,
 * ZOO_COLOR_BLUE, ZOO_COLOR_MAGENTA, ZOO_COLOR_CYAN, ZOO_COLOR_WHITE are used
 * to set the color.
 * The ZOO_COLOR_START_BACKGROUND or ZOO_COLOR_START_BACKGROUND_HIGH_INTENSITY
 * are used to set the background color.
 * The ZOO_COLOR_RESET is used to reset the color formatting.
 */

/**
 * The ZOO_COLOR_START is used to start the color formatting.
 */
#define ZOO_COLOR_START "\e[0;"

/**
 * The ZOO_COLOR_START_BOLD is used to start the color formatting with bold text.
 */
#define ZOO_COLOR_START_BOLD "\e[1;"

/**
 * The ZOO_COLOR_STANDARD is used to set the color intensity to standard.
 */
#define ZOO_COLOR_STANDARD "3"

/**
 * The ZOO_COLOR_HIGH_INTENSITY is used to set the color intensity to high.
 */
#define ZOO_COLOR_HIGH_INTENSITY "9"

#define ZOO_COLOR_BLACK "0m"
#define ZOO_COLOR_RED "1m"
#define ZOO_COLOR_GREEN "2m"
#define ZOO_COLOR_YELLOW "3m"
#define ZOO_COLOR_BLUE "4m"
#define ZOO_COLOR_MAGENTA "5m"
#define ZOO_COLOR_CYAN "6m"
#define ZOO_COLOR_WHITE "7m"

#define ZOO_COLOR_START_BACKGROUND "\e[4"
#define ZOO_COLOR_START_BACKGROUND_HIGH_INTENSITY "\e[0;10"

#define ZOO_COLOR_RESET "\e[0m"

#include "time.h"
/**
 * The ZOO_DEBUG macro print a message with date time, process id (UNIX pid), file name, function name, and line number
 *
 * The message uses the ZOO_LOG_FORMAT to format the output.
 *
 * The values that should be included in this specific order in the
 * ZOO_LOG_FORMAT are:
 *
 *  * %d: the date time
 *  * %l: the log level
 *  * %p: the process identifier
 *  * %f: the file name
 *  * %u: the function name
 *  * %i: the line number
 *  * %m: the message
 *
 *  The ZOO_DEBUG macro print messages in color, if stderr is a TTY.
 *
 * @param message the message to print
 * @param pccFile the file name
 * @param pccFunc the function name
 * @param iLine the line number
 */
#define _ZOO_DEBUG(message,pccFile,pccFunc,iLine) \
do {\
    if(iZooLogLevel<=iMinZooLogLevel)\
      break;\
      \
    int iSaTTY=isatty(2); \
    const struct tm *pcsTm; \
    time_t ttNow; \
    size_t stLen; \
    struct ztimeval sztv; \
    ttNow = time ( NULL ); \
    pcsTm = localtime ( &ttNow ); \
    char* pcaLocalTime = (char*)malloc((TIME_SIZE+1)*sizeof(char)); \
    zGettimeofday(&sztv, NULL); \
    int iMillisec = int(sztv.tv_usec/1000.0); \
    if (iMillisec>=1000) { \
      iMillisec -=1000; \
      sztv.tv_sec++; \
    } \
    stLen = strftime ( pcaLocalTime, TIME_SIZE, zLogDateFormat, pcsTm ); \
    char pcExtra[8]={0}; \
    int iLength=8; \
    int iCounter=0; \
    for(int iCnt=strlen(pccLogLevel[iZooLogLevel]);iCnt<8;iCnt++){ \
      pcExtra[iCounter]=' '; \
      iCounter++; \
    } \
    char* pcaFormated=NULL; \
    char acFormated[64]={0}; \
    char* apcaArgv[7]; \
    int aiTypes[7]={0,0,0,0,0,0,0}; \
    int iInternalCnt=0; \
    for(int iCnt=0;iCnt<strlen(ZOO_LOG_FORMAT);iCnt++){ \
      if(ZOO_LOG_FORMAT[iCnt]=='%'){ \
        int iLen=strlen(acFormated); \
        acFormated[iLen]=ZOO_LOG_FORMAT[iCnt]; \
        acFormated[iLen+1]='s'; \
        const char* pcColor="\e[0;97m"; \
        switch(iZooLogLevel){ \
          case ZOO_DEBUG_LEVEL_TRACE:\
            pcColor=ZOO_COLOR_START_BOLD\
              ZOO_COLOR_STANDARD \
              ZOO_COLOR_WHITE;\
            break;\
          case ZOO_DEBUG_LEVEL_DEBUG:\
            pcColor=ZOO_COLOR_START_BOLD\
              ZOO_COLOR_HIGH_INTENSITY\
              ZOO_COLOR_CYAN;\
            break;\
          case ZOO_DEBUG_LEVEL_INFO:\
            pcColor=ZOO_COLOR_START_BOLD\
              ZOO_COLOR_HIGH_INTENSITY\
              ZOO_COLOR_WHITE;\
            break;\
          case ZOO_DEBUG_LEVEL_SUCCESS:\
            pcColor=ZOO_COLOR_START_BOLD\
              ZOO_COLOR_HIGH_INTENSITY\
              ZOO_COLOR_GREEN;\
            break;\
          case ZOO_DEBUG_LEVEL_WARNING:\
            pcColor=ZOO_COLOR_START_BOLD\
              ZOO_COLOR_HIGH_INTENSITY\
              ZOO_COLOR_YELLOW;\
            break;\
          case ZOO_DEBUG_LEVEL_ERROR:\
            pcColor=ZOO_COLOR_START_BOLD\
              ZOO_COLOR_HIGH_INTENSITY\
              ZOO_COLOR_RED;\
            break;\
          case ZOO_DEBUG_LEVEL_FATAL:\
            pcColor=ZOO_COLOR_START_BACKGROUND\
              ZOO_COLOR_RED\
              ZOO_COLOR_START_BOLD\
              ZOO_COLOR_HIGH_INTENSITY\
              ZOO_COLOR_WHITE;\
            break;\
        } \
        if(ZOO_LOG_FORMAT[iCnt+1]=='d'){ \
          /* DATE Format: %s.%03d */ \
          apcaArgv[iInternalCnt]=(char*) malloc(37*sizeof(char)); \
          if(iSaTTY>0)\
            sprintf(apcaArgv[iInternalCnt],\
              ZOO_COLOR_START_BOLD\
              ZOO_COLOR_STANDARD\
              ZOO_COLOR_GREEN\
              "%s.%03d"\
              ZOO_COLOR_RESET,\
              pcaLocalTime,iMillisec); \
          else\
            sprintf(apcaArgv[iInternalCnt],"%s.%03d",pcaLocalTime,iMillisec); \
        } \
        else if(ZOO_LOG_FORMAT[iCnt+1]=='l'){ \
          /* Debug level: %s %s */ \
          apcaArgv[iInternalCnt]=(char*) malloc(36*sizeof(char)); \
          if(iSaTTY>0)\
            sprintf(apcaArgv[iInternalCnt],\
              "%s%s%s"\
              ZOO_COLOR_RESET,\
              pcColor,\
              pccLogLevel[iZooLogLevel],\
              pcExtra); \
          else\
            sprintf(apcaArgv[iInternalCnt],"%s %s",pccLogLevel[iZooLogLevel],pcExtra); \
        } \
        else if(ZOO_LOG_FORMAT[iCnt+1]=='p'){ \
          /* Process identifier: %p */ \
          apcaArgv[iInternalCnt]=(char*) malloc(10*sizeof(char)); \
          sprintf(apcaArgv[iInternalCnt],"%d",getpid()); \
        } \
        else if(ZOO_LOG_FORMAT[iCnt+1]=='f'){ \
          /* File name: %f */ \
          apcaArgv[iInternalCnt]=(char*) malloc((strlen(pccFile)+8)*sizeof(char)); \
          if(iSaTTY>0)\
            sprintf(apcaArgv[iInternalCnt],\
              ZOO_COLOR_START\
              ZOO_COLOR_STANDARD\
              ZOO_COLOR_CYAN\
              "%s",\
              pccFile); \
          else\
            sprintf(apcaArgv[iInternalCnt],"%s",pccFile); \
        } \
        else if(ZOO_LOG_FORMAT[iCnt+1]=='u'){ \
          /* Function name: %u */ \
          apcaArgv[iInternalCnt]=(char*) malloc((strlen(pccFunc)+1)*sizeof(char)); \
          sprintf(apcaArgv[iInternalCnt],"%s",pccFunc); \
        } \
        else if(ZOO_LOG_FORMAT[iCnt+1]=='i'){ \
          /* Line number: %i */ \
          apcaArgv[iInternalCnt]=(char*) malloc(14*sizeof(char)); \
          if(iSaTTY>0)\
            sprintf(apcaArgv[iInternalCnt],\
              "%d"\
              ZOO_COLOR_RESET,\
              iLine); \
          else\
            sprintf(apcaArgv[iInternalCnt],"%d",iLine); \
        } \
        else if(ZOO_LOG_FORMAT[iCnt+1]=='m'){ \
          /* Message: %m */ \
          apcaArgv[iInternalCnt]=(char*) malloc((strlen(message)+25)*sizeof(char)); \
          if(iSaTTY>0)\
            sprintf(apcaArgv[iInternalCnt],\
              "%s%s"\
              ZOO_COLOR_RESET,\
              pcColor,message); \
          else\
            sprintf(apcaArgv[iInternalCnt],"%s",message); \
        } \
        iInternalCnt++; \
        iCnt++; \
      }else{ \
          int iLen=0; \
          if(acFormated[0]!=0)\
            iLen=strlen(acFormated);\
          acFormated[iLen]=ZOO_LOG_FORMAT[iCnt];\
          acFormated[iLen+1]=0;\
      } \
    } \
    /* Minimal should be 2 (for the file+line) */ \
    switch(iInternalCnt) {\
      case 1: \
        fprintf(stderr,(char*)acFormated,\
            apcaArgv[0] \
        ); \
        break; \
      case 2: \
        fprintf(stderr,(char*)acFormated,\
            apcaArgv[0], \
            apcaArgv[1] \
        ); \
        break; \
      case 3: \
        fprintf(stderr,(char*)acFormated,\
            apcaArgv[0], \
            apcaArgv[1], \
            apcaArgv[2] \
        ); \
        break; \
      case 4: \
        fprintf(stderr,(char*)acFormated,\
            apcaArgv[0], \
            apcaArgv[1], \
            apcaArgv[2], \
            apcaArgv[3] \
        ); \
        break; \
      case 5: \
        fprintf(stderr,(char*)acFormated,\
            apcaArgv[0], \
            apcaArgv[1], \
            apcaArgv[2], \
            apcaArgv[3], \
            apcaArgv[4] \
        ); \
        break; \
      case 6: \
        fprintf(stderr,(char*)acFormated,\
            apcaArgv[0], \
            apcaArgv[1], \
            apcaArgv[2], \
            apcaArgv[3], \
            apcaArgv[4], \
            apcaArgv[5] \
        ); \
        break; \
      default: \
        fprintf(stderr,(char*)acFormated,\
            apcaArgv[0], \
            apcaArgv[1], \
            apcaArgv[2], \
            apcaArgv[3], \
            apcaArgv[4], \
            apcaArgv[5], \
            apcaArgv[6] \
        ); \
        break; \
    }\
    /* myPrint(acFormated,apcaArgv) */ \
    free(pcaLocalTime); \
    for(int iCnt=iInternalCnt-1;iCnt>=0;iCnt--){ \
      free(apcaArgv[iCnt]); \
    } \
} while(0)

/**
 * The memory size to create an elements
 */
#define ELEMENTS_SIZE (sizeof(char*)+(((2*sizeof(char*))+sizeof(maps*))*3)+sizeof(char*)+((sizeof(map*) + sizeof(iotype*))*2)+(2*sizeof(elements*)))
/**
 * The memory size to create a map
 */
//#define MAP_SIZE (2*sizeof(char*))+sizeof(NULL) // knut: size of NULL pointer may be different from regular pointer (platform dependent)
#define MAP_SIZE (2*sizeof(char*))+sizeof(map*)
/**
 * The memory size to create an iotype
 */
//#define IOTYPE_SIZE MAP_SIZE+sizeof(NULL)
#define IOTYPE_SIZE sizeof(map*) + sizeof(iotype*)
/**
 * The memory size to create a maps
 */
//#define MAPS_SIZE (2*sizeof(char*))+sizeof(map*)+MAP_SIZE
#define MAPS_SIZE sizeof(char*)+sizeof(map*)+(2*sizeof(maps*))
/**
 * The memory size to create a service
 */
//#define SERVICE_SIZE (ELEMENTS_SIZE*2)+(MAP_SIZE*2)+sizeof(char*)
#define SERVICE_SIZE sizeof(char*) + 3*sizeof(map*) + 2*sizeof(elements*)
/**
 * The memory size to create a services
 */
//#define SERVICES_SIZE SERVICE_SIZE+sizeof(services*)
#define SERVICES_SIZE sizeof(service*)+sizeof(services*)
/**
 * The memory size to create a registry
 */
//#define REGISTRY_SIZE SERVICES_SIZE+sizeof(char*)
#define REGISTRY_SIZE sizeof(char*)+sizeof(services*)+sizeof(registry*)

#define SHMSZ     27

#include "version.h" 

#ifdef DEBUG_STACK
  void debugStack(const char* file,const int line){
    int stack;
    fprintf(stderr,"stack %p (%s: %d) \n",&stack,file,line);
  }
#endif

  /**
   * KVP linked list
   */
  typedef struct map{
    char* name; //!< the key
    char* value; //!< the value
    struct map* next; //!< the pointer to the next map if any or NULL
  } map;

#ifdef WIN32
#define NULLMAP ((map*) 0)
// knut: see new definition above
//#define bool int
//#define true 1
//#define false 0
#else
#define NULLMAP NULL
#endif

  /**
   * linked list of map pointer
   *
   * Small object to store WPS KVP set.
   */
  typedef struct maps{
    char* name; //!< the maps name
    struct map* content; //!< the content map
    struct maps* child; //!< the child maps
    struct maps* next; //!< the pointer to the next maps if any or NULL
  } maps;
  
  /**
   * Not named linked list
   *
   * Used to store information about formats, such as mimeType, encoding ... 
   */
  typedef struct iotype{
    struct map* content; //!< the content map
    struct iotype* next; //!< the pointer to the next iotype if any or NULL
  } iotype;

  /**
   * Metadata information about input or output.
   *
   * The elements are used to store metadata information defined in the ZCFG.
   */
  typedef struct elements{
    char* name; //!< the name
    struct map* content; //!< the content map
    struct map* metadata; //!< the metadata map
    struct map* additional_parameters; //!< the additional parameters map
    char* format; //!< the format: LiteralData or ComplexData or BoundingBoxData
    struct iotype* defaults; //!< the default iotype 
    struct iotype* supported; //!< the supported iotype 
    struct elements* child; //!< the pointer to the children element if any (or NULL)
    struct elements* next; //!< the pointer to the next element if any (or NULL)
  } elements;

  /**
   * Metadata information about a full Service.
   */
  typedef struct service{
    char* name; //!< the name
    struct map* content; //!< the content map
    struct map* metadata; //!< the metadata map
    struct map* additional_parameters; //!< the additional parameters map
    struct elements* inputs; //!< the inputs elements
    struct elements* outputs; //!< the outputs elements
  } service;

  /**
   * Services chained list.
   */
  typedef struct services{
    struct service* content; //!< the content service pointer
    struct services* next; //!< the pointer to the next services*
  } services;

  /**
   * Profile registry.
   */
  typedef struct registry{
    char *name; //!< the name
    struct services* content; //!< the content services pointer
    struct registry* next; //!< the next registry pointer
  } registry;

  /**
   * Statuses list
   */
  static const char* ZOO_STATUS[] = {
    "SERVICE_ACCEPTED",
    "SERVICE_STARTED",
    "SERVICE_PAUSED",
    "SERVICE_SUCCEEDED",
    "SERVICE_FAILED",
    "SERVICE_DISMISSED",
    "SERVICE_DEPLOYED",
    "SERVICE_UNDEPLOYED",
    NULL
  };

  // knut
  enum WPSException {
	  /*
	  * StatusOK is not a WPS exception, it is added
	  * here for convenience.
	  */
	  StatusOK,
	  /*
	  * See WPS 1.0 specification, Table 38 and Table 62.
	  */
	  MissingParameterValue,
	  InvalidParameterValue,
	  NoApplicableCode,
	  NotEnoughStorage,
	  ServerBusy,
	  FileSizeExceeded,
	  StorageNotSupported,
	  VersionNegotiationFailed,
	  /*
	  * See WPS 2.0 specification, Tables 41, 46, 48, and 50.
	  */
	  NoSuchProcess,
	  NoSuchMode,
	  NoSuchInput,
	  NoSuchOutput,
	  DataNotAccessible,
	  SizeExceeded,
	  TooManyInputs,
	  TooManyOutputs,
	  NoSuchFormat,
	  WrongInputData,
	  InternalServerError,
	  NoSuchJob,
	  ResultNotReady
  };

  /**
   * Standard WPS exception codes
   * @see WPSExceptionText
   */
  static const char* const WPSExceptionCode[] = {
    "StatusOK",
    "MissingParameterValue",
    "InvalidParameterValue",
    "NoApplicableCode",
    "NotEnoughStorage",
    "ServerBusy",
    "FileSizeExceeded",
    "StorageNotSupported",
    "VersionNegotiationFailed",
    "NoSuchProcess",
    "NoSuchMode",
    "NoSuchInput",
    "NoSuchOutput",
    "DataNotAccessible",
    "SizeExceeded",
    "TooManyInputs",
    "TooManyOutputs",
    "NoSuchFormat",
    "WrongInputData",
    "InternalServerError",
    "NoSuchJob",
    "ResultNotReady",
    "InvalidQueryParameterValue",
    "DuplicatedProcess",
    "ImmutableProcess",
    "UnsupportedMediaType",
    "WorkflowNotFound"
  };

  /**
   * WPS exception codes to OGC API - Processes ones
   * @see WPSExceptionCode, OAPIPExceptionCode
   */
  static const int OAPIPCorrespondances[8][2] = {
    {9,0},
    {20,1},
    {21,2},
    {22,3},
    {23,4},
    {24,5},
    {25,6},
    {26,7}
  };

  /**
   * OGC API - Processes exception limits
   * 
   * Used to determine which exceptionsUrl to use for the current exception.
   * The first value is the index in the WPSExceptionCode array, and the second
   * one is the index in the exceptionsUrl map array (i.e. when exception code 
   * index is 4 or upper, we should use exceptionUrl_1).
   * @see WPSExceptionCode, OAPIPCorrespondances
   */
  static const int OAPIPExceptionLimits[1][2] = {
    {4,1}
  };

  /**
   * OGC API - Processes exception codes
   * @see WPSExceptionCode, OAPIPCorrespondances
   */
  static const char* const OAPIPExceptionCode[] = {
    "no-such-process",
    "no-such-job",
    "result-not-ready",
    "invalid-query-parameter-value",
    "duplicated-process",
    "immutable-process",
    "unsupported-media-type",
    "workflow-not-found"
  };

  /**
   * OGC API - Processes supported Content-Type list
   */
  static const char* const OAPIPSupportedContentTypes[] = {
    "application/json",
    "application/cwl",
    "application/cwl+yaml",
    "application/ogcapppkg+json",
    NULL
  };

  /**
   * WPS exception text (associated with the exception code)
   * @see WPSExceptionCode
   */
  static const char* const WPSExceptionText[] = {
	"No problem detected",
	"Operation request does not include a parameter value, and this server did not declare a default value for that parameter.",
	"Operation request contains an invalid parameter value.",
	"No other exceptionCode specified by this service and server applies to this exception.",
	"The server does not have enough space available to store the inputs and outputs associated with the request.",
	"The server is too busy to accept and queue the request at this time.",
	"The file size of one of the input parameters was too large for this process to handle.",
	"Execute operation request included transmission=”reference” for one of the outputs, but storage is not offered by this server.",
	"Service version for a ComplexData xlink:href input was not supported by the referenced server, and version negotiation failed.",
	"One of the identifiers passed does not match with any of the processes offered by this server.",
	"The process does not permit the desired execution mode.",
	"One or more of the input identifiers passed does not match with any of the input identifiers of this process.",
	"One or more of the output identifiers passed does not match with any of the input identifiers of this process.",
	"One of the referenced input data sets was inaccessible.",
	"The size of one of the input parameters was too large for this process to handle.",
	"Too many input items have been specified.",
	"Too many output items have been specified.",
	"One or more of the input or output formats specified in the request did not match with any of the formats defined for that particular input or output.",
	"One or more of inputs for which the service was able to retrieve the data but could not read it.",
	"",
	"The JobID from the request does not match any of the Jobs running on this server.",
	"The result for the requested JobID has not yet been generated."
  };

  ZOO_DLL_EXPORT int zooDebugPrint( const char * format, ... );
  ZOO_DLL_EXPORT void _dumpMap(map*);
  ZOO_DLL_EXPORT void dumpMap(map*);
  ZOO_DLL_EXPORT void dumpMaps(maps* m);
  ZOO_DLL_EXPORT void dumpMapToFile(map*,FILE*); // (used only internally)
  ZOO_DLL_EXPORT void dumpMapsToFile(maps*,char*,int);
  ZOO_DLL_EXPORT map* createMap(const char*,const char*);
  ZOO_DLL_EXPORT maps* createMaps(const char*);
  ZOO_DLL_EXPORT int count(map*);
  ZOO_DLL_EXPORT int countMaps(maps*);
  ZOO_DLL_EXPORT int countMapName(map*,const char*);
  ZOO_DLL_EXPORT int countMapNameValue(map*,const char*);
  ZOO_DLL_EXPORT bool hasKey(map*,const char*);
  ZOO_DLL_EXPORT maps* getMaps(maps*,const char*);
  ZOO_DLL_EXPORT map* getMap(map*,const char*);
  ZOO_DLL_EXPORT map* getLastMap(map*);
  ZOO_DLL_EXPORT map* getMapFromMaps(maps*,const char*,const char*);
  ZOO_DLL_EXPORT void freeMap(map**);
  ZOO_DLL_EXPORT void freeMaps(maps** mo);
  ZOO_DLL_EXPORT iotype* createIoType();
  ZOO_DLL_EXPORT elements* createEmptyElements();
  ZOO_DLL_EXPORT elements* createElements(const char*);
  ZOO_DLL_EXPORT void setElementsName(elements**,char*);
  ZOO_DLL_EXPORT bool hasElement(elements*,const char*);
  ZOO_DLL_EXPORT elements* getElements(elements*,const char*);
  ZOO_DLL_EXPORT void freeIOType(iotype**);
  ZOO_DLL_EXPORT void freeElements(elements**);
  ZOO_DLL_EXPORT void setServiceName(service**,char*);
  ZOO_DLL_EXPORT service* createService();
  ZOO_DLL_EXPORT void freeService(service**);
  ZOO_DLL_EXPORT void addToMap(map*,const char*,const char*);
  ZOO_DLL_EXPORT void addToMapA(map*,const char*,const char*);
  ZOO_DLL_EXPORT void addIntToMap(map*,const char*,const int);
  ZOO_DLL_EXPORT void addIntToMapArray(map*,const char*,int,const unsigned long);
  ZOO_DLL_EXPORT map* addToMapWithSize(map*,const char*,const char*,int);
  ZOO_DLL_EXPORT void addMapToMap(map**,map*);
  ZOO_DLL_EXPORT void addMapToIoType(iotype**,map*);
  ZOO_DLL_EXPORT map* getMapOrFill(map**,const char*,const char*);
  ZOO_DLL_EXPORT bool contains(map*,map*);
  ZOO_DLL_EXPORT iotype* getIoTypeFromElement(elements*,char*, map*);
  ZOO_DLL_EXPORT void loadMapBinary(map**,map*,int);
  ZOO_DLL_EXPORT void loadMapBinaries(map**,map*);
  ZOO_DLL_EXPORT maps* dupMaps(maps**);
  ZOO_DLL_EXPORT void addMapsToMaps(maps**,maps*);
  ZOO_DLL_EXPORT map* getMapArray(map*,const char*,int);
  ZOO_DLL_EXPORT char* getMapArrayKey(map*,const char*,int);
  ZOO_DLL_EXPORT void setMapArray(map*,const char*,int,const char*);
  ZOO_DLL_EXPORT map* getMapType(map*);
  ZOO_DLL_EXPORT int addMapsArrayToMaps(maps**,maps*,char*);
  ZOO_DLL_EXPORT void setMapInMaps(maps*,const char*,const char*,const char*);
  ZOO_DLL_EXPORT void dumpElements(elements*);
  ZOO_DLL_EXPORT void dumpElementsAsYAML(elements*,int);
  ZOO_DLL_EXPORT elements* dupElements(elements*);
  ZOO_DLL_EXPORT void addToElements(elements**,elements*);
  ZOO_DLL_EXPORT void dumpService(service*);
  ZOO_DLL_EXPORT void dumpServiceAsYAML(service*);
  ZOO_DLL_EXPORT service* dupService(service*);
  ZOO_DLL_EXPORT void dumpRegistry(registry*);
  ZOO_DLL_EXPORT bool addServiceToRegistry(registry**,char*,service*);
  ZOO_DLL_EXPORT void freeRegistry(registry**);
  ZOO_DLL_EXPORT service* getServiceFromRegistry(registry*,char*,char*);
  ZOO_DLL_EXPORT void inheritMap(map**,map*);
  ZOO_DLL_EXPORT void inheritIOType(iotype**,iotype*);
  ZOO_DLL_EXPORT void inheritElements(elements**,elements*);
  ZOO_DLL_EXPORT void inheritance(registry*,service**);
  ZOO_DLL_EXPORT void mapsToCharXXX(maps*,char***);
  ZOO_DLL_EXPORT void charxxxToMaps(char***,maps**);
  // OGC-API - Processes - Part 1: Core processes list restriction
  ZOO_DLL_EXPORT void updateCnt(maps*, const char*, const char*);
  ZOO_DLL_EXPORT bool compareCnt(maps*, const char*, const char*);
#if defined(_MSC_VER) && _MSC_VER < 1800
  // snprintf for Visual Studio compiler;
  // it is also used by services (e.g., GetStatus), therefore exported to shared library
  ZOO_DLL_EXPORT int snprintf(char *buffer, size_t n, const char *format, ...);
#endif

  // knut: some new utility functions; logMessage is primarily intended for debugging 	
  ZOO_DLL_EXPORT bool nonempty(map* map);
  ZOO_DLL_EXPORT bool hasvalue(maps* source, const char* node, const char* key, map** kvp);
#ifdef __cplusplus
  ZOO_DLL_EXPORT void setErrorMessage(maps*& conf, const char* service, WPSException exc, const char* message = NULL);
  ZOO_DLL_EXPORT void logMessage(const char* source, const char* function, int line, const char* file = NULL, const char* message = NULL);
#endif
  #define zooLogMsg(file,message) logMessage(__FILE__, __func__, __LINE__, (file), (message)) 
  #define zooLog logMessage(__FILE__, __func__, __LINE__)  
  
  // knut : function for pre-allocated memory for a map value;
  // processing algorithms may be able to write directly to this space, thereby avoiding unneccesary copying of data
  ZOO_DLL_EXPORT char* allocateMapValue(map* node, size_t num_bytes);
  
  ZOO_DLL_EXPORT char* getValueFromMaps(maps*,const char*);
  ZOO_DLL_EXPORT void _translateChar (char*, char, char);
#ifdef __cplusplus
}
#endif

#endif

#ifdef LOG_CONSOLE_ENABLED
  void logConsoleMessage(const char*);
#endif
