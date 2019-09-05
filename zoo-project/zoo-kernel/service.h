/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2019 GeoLabs SARL
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

#ifdef WIN32
#define ZOO_DLL_EXPORT __declspec( dllexport )
#else
#define ZOO_DLL_EXPORT 
#endif

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
/**
 * The crossplatform strdup alias
 */
#define zStrdup strdup
/**
 * The crossplatform mkdir alias
 */
#define zMkdir mkdir
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
	"ResultNotReady"
  };

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

  ZOO_DLL_EXPORT void _dumpMap(map*);
  ZOO_DLL_EXPORT void dumpMap(map*);
  ZOO_DLL_EXPORT void dumpMaps(maps* m);
  ZOO_DLL_EXPORT void dumpMapToFile(map*,FILE*); // (used only internally)
  ZOO_DLL_EXPORT void dumpMapsToFile(maps*,char*,int);
  ZOO_DLL_EXPORT map* createMap(const char*,const char*);
  ZOO_DLL_EXPORT maps* createMaps(const char*);
  ZOO_DLL_EXPORT int count(map*);
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
  ZOO_DLL_EXPORT void addIntToMap(map*,const char*,const int);
  ZOO_DLL_EXPORT void addIntToMapArray(map*,const char*,int,const int);
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

#ifdef __cplusplus
}
#endif

#endif
