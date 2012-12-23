/**
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
#ifdef USE_MS

#include <sys/stat.h>
#include "service_internal.h"
#include "service.h"
#include "cpl_conv.h"
#include "cpl_multiproc.h"
#include "ogr_api.h"
#include "gdal.h"
#include "ogr_srs_api.h"
#include "ulinet.h"
#ifdef WIN32
#include <unistd.h>
#endif

#include <mapserver.h>

#ifdef __cplusplus
extern "C" {
#endif
  //#include <mapserver.h>
  /**
   * Map composed by a main.cfg maps name as key and the corresponding 
   * MapServer Mafile Metadata name to use
   * see doc from here :
   *  - http://mapserver.org/ogc/wms_server.html
   *  - http://mapserver.org/ogc/wfs_server.html
   *  - http://mapserver.org/ogc/wcs_server.html
   */
  map* getCorrespondance();
  void setMapSize(maps* output,double minx,double miny,double maxy,double maxx);
  void setReferenceUrl(maps* m,maps* tmpI);

  /**
   * Set projection using Authority Code and Name if available or fallback to 
   * proj4 definition if available or fallback to default EPSG:4326
   */
  void setSrsInformations(maps* output,mapObj* m,layerObj* myLayer, char* pszProjection);
  
  void setMsExtent(maps* output,mapObj* m,layerObj* myLayer,
		   double minX,double minY,double maxX,double maxY);
  int tryOgr(maps* conf,maps* output,mapObj* m);
  
  int tryGdal(maps* conf,maps* output,mapObj* m);
  /**
   * Create a MapFile for WMS, WFS or WCS Service output
   */
  void outputMapfile(maps* conf,maps* outputs);
#ifdef __cplusplus
}
#endif

#endif
#endif
 
