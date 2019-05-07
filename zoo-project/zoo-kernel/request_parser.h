/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2015 GeoLabs SARL
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

#ifndef ZOO_REQUEST_PARSER_H
#define ZOO_REQUEST_PARSER_H 1

#pragma once 

#include "service.h"
#include "ulinet.h"

#include <libxml/parser.h>
#include <libxml/xpath.h>

#ifdef __cplusplus
extern "C" {
#endif

  xmlXPathObjectPtr extractFromDoc (xmlDocPtr, const char *);
  int appendMapsToMaps (maps*, maps*, maps*, elements*);
  void ensureDecodedBase64(maps**);
  int kvpParseInputs(maps**,service*,map*,maps**,HINTERNET*);
  int xmlParseBoundingBox(maps**,map**,xmlDocPtr);
  int kvpParseOutputs(maps**,map *,maps**);
  int xmlParseInputs(maps**,service*,maps**,xmlDocPtr,xmlNodeSet*,HINTERNET*);
  int xmlParseBoundingBox(maps** main_conf,map** current_input,xmlDocPtr doc);
  int xmlParseOutputs(maps**,map**,maps**,xmlDocPtr,xmlNodePtr,bool);
  int xmlParseRequest(maps**,const char*,map**,service*,maps**,maps**,HINTERNET*);
  int parseRequest(maps**,map**,service*,maps**,maps**,HINTERNET*);
  void checkValidValue(map*,map**,const char*,const char**,int);
  int validateRequest(maps**,service*,map*,maps**,maps**,HINTERNET*);
  void parseCookie(maps**,const char*);
  
#ifdef __cplusplus
}
#endif

#endif
