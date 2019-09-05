/*
 * Author : Gérald FENOY
 *
 *  Copyright 2014 GeoLabs SARL. All rights reserved.
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

#ifdef __cplusplus
extern "C" {
#endif
  /**
   * Name and corresponding attributes depending on the WPS version
   */
  static const char* jcapabilities[4]={
    "jobControlOptions","sync-execute async-execute dismiss",
    "outputTransmission","value reference"
  };

  json_object* mapToJson(map*);
  json_object* mapsToJson(maps*);
  json_object* serviceToJson(service*);
  void printGetCapabilitiesForProcessJ(registry*, maps*,void*,void*,service*);
  void printExceptionReportResponseJ(maps*,map*);
  void parseJRequest(maps*,service* s,json_object*,maps**,maps**);
  json_object*  printJResult(maps*,service*,maps*,int);
  json_object* printJobList(maps*);
  int createStatusFile(maps*,int);
  int json_getStatusFile(maps*);
  void produceApi(maps*,json_object*);
  //void printIOTypeJ(maps*, const char*, elements*,json_object*,service*);
#ifdef __cplusplus
}
#endif
#endif
