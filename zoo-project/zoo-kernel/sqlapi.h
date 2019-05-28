/*
 * Author : David Saggiorato
 *
 *  Copyright 2008-2009 GeoLabs SARL. All rights reserved.
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

#ifndef ZOO_SQLAPI_H
#define ZOO_SQLAPI_H 1

#ifdef META_DB
#include "ogrsf_frmts.h"
#include "ogr_p.h"
#if GDAL_VERSION_MAJOR >= 2
#include <gdal_priv.h>
#endif
#endif

#include "service.h"
#include "service_internal.h"

#ifdef META_DB
extern "C" 
#if GDAL_VERSION_MAJOR >=2
  GDALDataset
#else
  OGRDataSource 
#endif
  **zoo_DS;
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef META_DB
  ZOO_DLL_EXPORT char* _createInitString(maps*,const char*);
  ZOO_DLL_EXPORT int _init_sql(maps*,const char*);
  ZOO_DLL_EXPORT OGRLayer *fetchSql(maps*,int,const char*);
  ZOO_DLL_EXPORT void cleanFetchSql(maps*,int,OGRLayer*);
#endif
#ifdef RELY_ON_DB
  ZOO_DLL_EXPORT int init_sql(maps*);
  ZOO_DLL_EXPORT void close_sql(maps*,int);
  ZOO_DLL_EXPORT int execSql(maps*,int,const char*);
  ZOO_DLL_EXPORT void recordStoredFile(maps*,const char*,const char*,const char*);
  ZOO_DLL_EXPORT void recordServiceStatus(maps*);
  ZOO_DLL_EXPORT void recordResponse(maps*,char*);
  ZOO_DLL_EXPORT void readFinalRes(maps*,char*,map*);
  ZOO_DLL_EXPORT int isRunning(maps*,char*);
  ZOO_DLL_EXPORT char* getStatusId(maps*,char*);
  ZOO_DLL_EXPORT void removeService(maps*,char*);
  ZOO_DLL_EXPORT void end_sql();
#endif

#ifdef __cplusplus
}
#endif

#endif
