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


#define CWL_MAX_PARTS 64

/* Defensive limits (override at compile time if needed). They do not replace
 * the request size limit that should already be enforced upstream. */
#ifndef CWL_MAX_BODY_SIZE
#define CWL_MAX_BODY_SIZE (16UL * 1024UL * 1024UL)
#endif
#ifndef CWL_MAX_HEADER_BLOCK
#define CWL_MAX_HEADER_BLOCK 16384
#endif
#define CWL_MAX_BOUNDARY_LEN 70   /* RFC 2046 */

#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /**
  * Result of the parsing helpers. NOMEM is kept distinct from INVALID so that
  * an allocation failure is reported as a server error, not as a bad request.
  */
  typedef enum {
    CWL_OK = 0,
    CWL_ERR_INVALID = 1,   // malformed or unsafe input
    CWL_ERR_NOMEM = 2      // allocation failure
  } cwlStatus;

  /**
   * Structure representing a single part of a multipart request.
   * This structure holds metadata and content for each part of a multipart request.
   * It is used when handling multipart requests, such as CWL multipart deploys.
   * Each part contains its Content-ID, proposed relative path, MIME type, and the actual data.
   * The data pointer points into the original body buffer and should not be freed separately.
   * The length field indicates the size of the data buffer.
   */
  typedef struct {
    char *contentId;        // Content-ID of the part (without < and >)
    char *contentLocation;  // relative path proposed by the client
    char *contentType;      // MIME type of the part
    const char *data;       // pointer INTO the body buffer (no copy, never freed)
    size_t len;             // length of the part's data
  } cwlPart;

  xmlXPathObjectPtr extractFromDoc (xmlDocPtr, const char *);
  int appendMapsToMaps (maps**, maps*, maps*, elements*);
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
  int parseInputHttpRequests(maps*,maps*, HINTERNET*);
  char *extractMimeParam(const char *contentType, const char *paramName);
  int handleCwlMultipartDeploy(maps*, const char*, size_t, const char*, const char*);

#ifdef __cplusplus
}
#endif

#endif
