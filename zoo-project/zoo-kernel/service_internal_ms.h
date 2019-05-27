/*
 * Author : Gérald FENOY
 *
 *  Copyright 2010  Fondazione Edmund Mach. All rights reserved.
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
#ifndef ZOO_SERVICE_INTERNAL_MS_H
#define ZOO_SERVICE_INTERNAL_MS_H 1

#include <sys/stat.h>
#include "service.h"
#include "cpl_conv.h"
#include "cpl_multiproc.h"
#include "ogr_api.h"
#include "gdal.h"
#include "ogr_srs_api.h"
#ifdef WIN32
#include <unistd.h>
#endif


#include <mapserver.h>

#ifdef __cplusplus
extern "C" {
#endif
  map* getCorrespondance();
  void setMapSize(maps* output,double minx,double miny,double maxy,double maxx);
  void setReferenceUrl(maps* m,maps* tmpI);

  void setSrsInformations(maps* output,mapObj* m,layerObj* myLayer, char* pszProjection);
  
  void setMsExtent(maps* output,mapObj* m,layerObj* myLayer,
		   double minX,double minY,double maxX,double maxY);
  int tryOgr(maps* conf,maps* output,mapObj* m);
  
  int tryGdal(maps* conf,maps* output,mapObj* m);
  void outputMapfile(maps* conf,maps* outputs);
  void saveMapNames(maps*,maps*,char*);

#ifdef __cplusplus
}
#endif

#endif
 
