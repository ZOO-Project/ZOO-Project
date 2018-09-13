/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2010 GeoLabs SARL
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

#ifndef ZOO_SERVICE_INTERNAL_JS_H
#define ZOO_SERVICE_INTERNAL_JS_H 1

#pragma once 
#ifdef WIN32
#include <wchar.h>
#endif
#include "service.h"
#include "service_internal.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ulinet.h"

#ifdef __cplusplus
extern "C" {
#endif

  char* JSValToChar(JSContext*, jsval*);
  JSObject* JSObject_FromMaps(JSContext *,maps*);
  JSObject* JSObject_FromMap(JSContext *,map*);
  maps* mapsFromJSObject(JSContext *,jsval);
  map* mapFromJSObject(JSContext *,jsval);
  JSBool JSTranslate(JSContext*, uintN, jsval*);
  JSBool JSRequest(JSContext*, uintN, jsval*);
  JSBool JSUpdateStatus(JSContext*,uintN,jsval *);
  JSBool JSSleep(JSContext*,uintN,jsval *);

  void reportError(JSContext *cx, const char *message, JSErrorReport *report);
  
  int zoo_js_support(maps**,map*,service*,maps **,maps **);

  JSObject *loadZooApiFile(JSContext*,JSObject*,char*);

#ifdef __cplusplus
}
#endif

#endif
