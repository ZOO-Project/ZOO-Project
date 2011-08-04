#ifndef ZOO_SERVICE_INTERNAL_MS_H
#define ZOO_SERVICE_INTERNAL_MS_H 1
#ifdef USE_MS

#include <sys/stat.h>
#include "service_internal.h"
#include "service.h"
#include "cpl_conv.h"
#include "ogr_api.h"
#include "gdal.h"
#include "ogr_srs_api.h"
#include "ulinet.h"

#include <mapserver.h>

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

#endif
#endif
 
