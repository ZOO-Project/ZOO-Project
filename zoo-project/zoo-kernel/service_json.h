/*
 * Author : Gérald FENOY
 *
 *  Copyright 2014-2021 GeoLabs SARL. All rights reserved.
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

#ifndef ZOO_JSON_H
#define ZOO_JSON_H 1

#pragma once

#include "response_print.h"
#include <stdio.h>
#include <ctype.h>
#include <service.h>
#include <json_object.h>
#include "json_c_version.h"

#if (JSON_C_MINOR_VERSION<=12)
#define 	JSON_C_TO_STRING_NOSLASHESCAPE   (1<<4)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Names of field to output in the status object
   */
  static const char* statusFields[6]={
    "type", "processID","created","started","finished","updated"
  };

  /**
   * Coreresponding names of field from the data table to fetch informations
   */
  static const char* statusFieldsC[6]={
    "itype", "processid","creation_time","start_time","end_time","updated_time"
  };

  /**
   * Coreresponding names of field from the data table to fetch informations
   */
  static const char* statusSearchFields[2]={
    "processid","status"
  };

  /**
   * Coreresponding names of field from the data table to fetch informations
   */
  static const char* statusSearchFieldsReal[2]={
    "processid","fstate"
  };

  /**
   * Definitions of acceptable final status
   */
  static char oapipStatus[5][11]={
    "successful",
    "failed",
    "running",
    "dismissed",
    "accepted"
  };

  /**
   * Name and corresponding attributes depending on the WPS version
   */
  static const char* jcapabilities[]={
    "jobControlOptions","sync-execute async-execute dismiss",
    "outputTransmission","value reference",
    "jobControlOptions","async-execute dismiss"
  };

  /**
   * Possible field names corresponding to pccRFields
   * @see pccRFields
   */
  static const char* pccFields[9]={
    "type",
    "encoding",
    "schema",
    "mediaType",
    "mediaEncoding",
    "mediaSchema",
    "contentMediaType",
    "contentEncoding",
    "contentSchema"
  };

  /**
   * Corresponding fields available in WPS for pccFields
   * @see pccFields
   */
  static const char* pccRFields[3]={
    "mimeType",
    "encoding",
    "schema"
  };

  /**
   * Equivalent range keywords for WPS version 1 and 2
   */
  static const char* rangeCorrespondances[4][2]={
    { "rangeClosure", "rangeClosure" },
    { "rangeMin", "minimum" },
    { "rangeMax", "maximum"},
    { "rangeSpacing", "spacing" }
  };

  maps* jsonToMaps(json_object*);
  map* jsonToMap(json_object*);
  json_object* mapToJson(map*);
  json_object* mapsToJson(maps*);
  json_object* serviceToJson(service*);
  void printLiteralValueJ(maps*,map*,map*,json_object*,const char*);
  void printGetCapabilitiesForProcessJ(registry*, maps*,void*,void*,service*);
  void printExceptionReportResponseJ(maps**,map*);
  void parseJRequest(maps*,service*,json_object*,map*,maps**,maps**);
  json_object*  printJResult(maps**,service*,maps*,int);
  json_object* printJobStatus(maps**,char*);
  json_object* printJobList(maps**);
  json_object* printFilteredJobList(maps**,map*);
  int createNextLinks(maps*,json_object*);
  int createStatusFile(maps*,int);
  void json_getStatusAttributes(maps*,map*,json_object*,int);
  json_object* createStatus(maps*,int);
  char* json_getStatusFilePath(maps*);
  json_object* parseJson(maps*,char*);
  json_object* json_readFile(maps*,char*);
  char* getResultPath(maps*,char*);
  int json_getStatusFile(maps*);
  void produceApi(maps*,json_object*);
  void produceRoot(maps*,json_object*);
  void produceConformances(maps*,json_object*);
  void outputSingleJsonComplexRes(maps*,maps*,json_object*,json_object*,char*,long);
  void outputSingleJsonLiteralData(maps*,maps*,json_object*,const char*,char*);
  bool jsonIsFalse(json_bool);
#ifdef DRU_ENABLED
  void handleDRUError(maps**);
  int convertOGCAppPkgToExecute(maps*,map*,json_object**);
  json_object* convertCwlToOGCAppPkg(maps*,map*);
  bool serviceIsDRU(maps*,char*);
  int handlePackage(maps**,char*,char*,char*);
#endif
  bool serviceIsFilter(maps*,char*);
  //void printIOTypeJ(maps*, const char*, elements*,json_object*,service*);
#ifdef __cplusplus
}
#endif
#endif
