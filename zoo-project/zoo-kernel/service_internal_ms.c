/*
 * Author : Gérald FENOY
 *
 *  Copyright 2010-2011 Fondazione Edmund Mach. All rights reserved.
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

/**
 * Cross platform definition of layerObj->class
 */
#ifndef WIN32
#define CLASS class
#else
#define CLASS _class
#endif
#include "service_internal_ms.h"
#include "server_internal.h"
#include "response_print.h"
#include "caching.h"

/**
 * Get a list of configuration keys having a corresponding mandatory ows_*.
 * Map composed by a main.cfg maps name as key and the corresponding 
 * MapServer Mafile Metadata name to use
 * see doc from here :
 *  - http://mapserver.org/ogc/wms_server.html
 *  - http://mapserver.org/ogc/wfs_server.html
 *  - http://mapserver.org/ogc/wcs_server.html
 *
 * @return a new map containing a table linking a name of a configuration key
 * to a corresponding mandatory ows_* keyword (ie. "fees" => "ows_fees").
 */
map* getCorrespondance(){
  map* res=createMap("encoding","ows_encoding");
  addToMap(res,"abstract","ows_abstract");
  addToMap(res,"title","ows_title");
  addToMap(res,"keywords","ows_keywordlist");
  addToMap(res,"fees","ows_fees");
  addToMap(res,"accessConstraints","ows_accessconstraints");
  addToMap(res,"providerName","ows_attribution_title");
  addToMap(res,"providerSite","ows_service_onlineresource");
  addToMap(res,"individualName","ows_contactperson");
  addToMap(res,"positionName","ows_contactposition");
  addToMap(res,"providerName","ows_contactorganization");
  addToMap(res,"role","ows_role");
  addToMap(res,"addressType","ows_addresstype");
  addToMap(res,"addressCity","ows_city");
  addToMap(res,"addressDeliveryPoint","ows_address");
  addToMap(res,"addressPostalCode","ows_postcode");
  addToMap(res,"addressAdministrativeArea","ows_stateorprovince");
  addToMap(res,"addressCountry","ows_country");
  addToMap(res,"phoneVoice","ows_contactvoicetelephone");
  addToMap(res,"phoneFacsimile","ows_contactfacsimiletelephone");
  addToMap(res,"addressElectronicMailAddress","ows_contactelectronicmailaddress");
  // Missing Madatory Informations
  addToMap(res,"hoursOfService","ows_hoursofservice");
  addToMap(res,"contactInstructions","ows_contactinstructions");
  return res;
}

/**
 * Return the current publish_id value
 * @param elem and maps pointer on which the search occur
 * @return the integer value of the publish_id field, if any, 0 otherwise
 */ 
int getPublishedId(maps* elem){
  if(elem!=NULL && elem->content!=NULL){
    map* myIndex=getMap(elem->content,"published_id");
    if(myIndex!=NULL){
      return atoi(myIndex->value);
    }
  }
  return 0;
}

/**
 * Add width and height keys to an output maps containing the maximum width
 * and height for displaying the full data extent.
 * Restriction to an image having a size of 640x480 (width * height)
 *
 * @param output 
 * @param minx the lower left x coordinate
 * @param miny the lower left y coordinate
 * @param maxx the upper right x coordinate
 * @param maxy the upper right y coordinate
 */
void setMapSize(maps* output,double minx,double miny,double maxx,double maxy){
  int imyIndex=getPublishedId(output);
  double maxWidth=640;
  double maxHeight=480;
  double deltaX=maxx-minx;
  double deltaY=maxy-miny;
  double qWidth;
  qWidth=maxWidth/deltaX;
  double qHeight;
  qHeight=maxHeight/deltaY;
#ifdef DEBUGMS
  fprintf(stderr,"deltaX : %.15f \ndeltaY : %.15f\n",deltaX,deltaY);
  fprintf(stderr,"qWidth : %.15f \nqHeight : %.15f\n",qWidth,qHeight);
#endif

  double width=deltaX*qWidth;
  double height=height=deltaY*qWidth;
  if(deltaX<deltaY){
    width=deltaX*qHeight;
    height=deltaY*qHeight;
  }
  if(height<0)
    height=-height;
  if(width<0)
    width=-width;
  char sWidth[1024];
  char sHeight[1024];
  sprintf(sWidth,"%.3f",width);
  sprintf(sHeight,"%.3f",height);
#ifdef DEBUGMS
  fprintf(stderr,"sWidth : %.15f \nsHeight : %.15f\n",sWidth,sHeight);
#endif
  if(output!=NULL){
    setMapArray(output->content,"width",imyIndex,sWidth);
    setMapArray(output->content,"height",imyIndex,sHeight);
  }
}

/**
 * Add a Reference key to an output containing the WMFS/WFS/WCS request for
 * accessing service result
 * 
 * @param m the conf maps containing the main.cfg settings
 * @param tmpI the specific output maps to add the Reference key
 */
void setReferenceUrl(maps* m,maps* tmpI){
  int imyIndex=getPublishedId(tmpI);
  if(getMapArray(tmpI->content,"ref_wms_link",imyIndex)!=NULL)
    return;
  outputMapfile(m,tmpI);
  map *msUrl=getMapFromMaps(m,"main","mapserverAddress");
  if(msUrl==NULL){
    errorException (m, _("Unable to find any mapserverAddress defined in the main.cfg file"),
		    "InternalError", NULL);
    exit(-1);
  }
  if(getMapArray(tmpI->content,"ref_wms_link",imyIndex)!=NULL)
    return;
  int finalProto=-1;
  map *msOgcVersion=getMapFromMaps(m,"main","msOgcVersion");
  map *dataPath=getMapFromMaps(m,"main","dataPath");
  map *sid=getMapFromMaps(m,"lenv","usid");
  map* format=getMapArray(tmpI->content,"mimeType",imyIndex);
  map* rformat=getMapArray(tmpI->content,"requestedMimeType",imyIndex);
  map* width=getMapArray(tmpI->content,"width",imyIndex);
  map* height=getMapArray(tmpI->content,"height",imyIndex);
  map* protoMap=getMapArray(tmpI->content,"msOgc",imyIndex);
  map* versionMap=getMapArray(tmpI->content,"msOgcVersion",imyIndex);
  map* datatype=getMapArray(tmpI->content,"geodatatype",imyIndex);
  map* layerName=getMapArray(tmpI->content,"msLayer",imyIndex);
  char options[4][5][25]={
    {"WMS","1.3.0","GetMap","layers=%s","wms_extent"},
    {"WFS","1.0.0","GetFeature","typename=%s","wcs_extent"},
    {"WCS","2.0.0","GetCoverage","coverageid=%s","wcs_extent"},
    {"WCS","1.0.0","GetCoverage","coverage=%s","wcs_extent"}
  };
  map *nbElements=getMapArray(tmpI->content,"nb_features",imyIndex);
  if(nbElements==NULL)
    nbElements=getMapArray(tmpI->content,"nb_pixels",imyIndex);
  if(datatype==NULL || strncmp(datatype->value,"other",5)==0 || (nbElements!=NULL && atoi(nbElements->value)==0)){
    map* minNb=getMap(tmpI->content,"minoccurs");
    map* useMs=getMap(tmpI->content,"useMapserver");
    if((minNb==NULL || atoi(minNb->value)>=1) && useMs!=NULL && strncasecmp(useMs->value,"true",4)==0){
      int lIndex=0;
      maps* lenv=getMaps(m,"lenv");
      if(getMapFromMaps(m,"lenv","mapErrorNb")==NULL)
	setMapInMaps(m,"lenv","mapErrorNb","0");
      else{
	map* tmpV=getMapFromMaps(m,"lenv","mapErrorNb");
	lIndex=atoi(tmpV->value)+1;
	addIntToMap(lenv->content,"mapErrorNb",lIndex);
      }
      setMapInMaps(m,"lenv","mapError","true");
      setMapArray(lenv->content,"locator",lIndex,tmpI->name);
      if(nbElements==NULL)
	setMapArray(lenv->content,"message",lIndex,_("The ZOO-Kernel was able to retrieve the data but could not read it as geographic data."));
      else
	setMapArray(lenv->content,"message",lIndex,_("The ZOO-Kernel was able to retrieve the data but could not access any feature or pixel in te resulting file."));
      if(getMapFromMaps(m,"lenv","state")==NULL)
	errorException (m, _("Unable to find any geographic data"), "WrongInputData", tmpI->name);
    }
    return;
  }
  int proto=0;
  if(rformat==NULL){
    rformat=getMapArray(tmpI->content,"mimeType",imyIndex);
  }
  if(strncasecmp(rformat->value,"text/xml",8)==0)
    proto=1;
  if(strncasecmp(rformat->value,"image/tiff",10)==0 ||
     strncasecmp(rformat->value,"image/geotiff",10)==0)
    proto=2;
  int hasFormat=-1;
  if(protoMap!=NULL){
    hasFormat=1;
    if(strncasecmp(protoMap->value,"WMS",3)==0){
      proto=0;
      rformat=createMap("value","image/png");
    }
    else{
      if(strncasecmp(protoMap->value,"WFS",3)==0){
	proto=1;
	rformat=createMap("value","text/xml");
      }
      else {
	proto=2;
	rformat=createMap("value","image/tiff");
      }
    }
  }
  char *protoVersion=options[proto][1];
  if(proto==1){
    if(msOgcVersion!=NULL)
      protoVersion=msOgcVersion->value;
    if(versionMap!=NULL)
      protoVersion=versionMap->value;
  }


  map* extent=getMapArray(tmpI->content,options[proto][4],imyIndex);
  map* crs=getMapArray(tmpI->content,"crs",imyIndex);
  int hasCRS=1;
  if(crs==NULL){
    crs=getMapFromMaps(m,"main","crs");
    if(crs==NULL){
      crs=createMap("crs","epsg:4326");
      hasCRS=0;
    }
  }
  char layers[128];
  if(layerName==NULL)
    sprintf(layers,options[proto][3],tmpI->name);
  else
    sprintf(layers,options[proto][3],layerName->value);

  if(format==NULL || width==NULL || height==NULL || extent==NULL){
    char tmpStr[1024];
    sprintf(tmpStr,_("Unable to create the mapfile for %s because of missing values."),tmpI->name);
    errorException (m, tmpStr,
		    "InternalError", NULL);
    exit(-1);
    return;
  }

  if(proto==0){
    hasFormat=1;
    rformat=createMap("mimeType","image/png");
  }else{
    if(proto==1){
      rformat=createMap("mimeType","text/xml");
      hasFormat=1;
    }
    else
      if(proto==2){
        rformat=createMap("mimeType","image/tiff");
	hasFormat=1;
	finalProto=1;
      }
  }
  
  char* webService_url=(char*)malloc((strlen(msUrl->value)+strlen(rformat->value)+strlen(tmpI->name)+strlen(width->value)+strlen(height->value)+strlen(extent->value)+256)*sizeof(char));


  if(proto>0){
    if(proto==2)
      finalProto=1;
    sprintf(webService_url,
	    "%s?map=%s/%s_%d_%s.map&request=%s&service=%s&version=%s&%s&format=%s&bbox=%s&crs=%s",
	    msUrl->value,
	    dataPath->value,
	    tmpI->name,
	    imyIndex,
	    sid->value,
	    options[proto][2],
	    options[proto][0],
	    protoVersion,
	    layers,
	    rformat->value,
	    extent->value,
	    crs->value
	    );
    if(datatype!=NULL && strncasecmp(datatype->value,"raster",6)==0){
      setMapArray(tmpI->content,"ref_wcs_link",imyIndex,webService_url);
    }
    else{
      setMapArray(tmpI->content,"ref_wfs_link",imyIndex,webService_url);
    }
    proto=0;
    freeMap(&rformat);
    free(rformat);
    rformat=createMap("mimeType","image/png");
  }
  else{
    sprintf(webService_url,
	    "%s?map=%s/%s_%d_%s.map&request=%s&service=%s&version=%s&%s&width=%s&height=%s&format=%s&bbox=%s&crs=%s",
	    msUrl->value,
	    dataPath->value,
	    tmpI->name,
	    imyIndex,
	    sid->value,
	    options[proto][2],
	    options[proto][0],
	    protoVersion,
	    layers,
	    width->value,
	    height->value,
	    rformat->value,
	    extent->value,
	    crs->value
	    );
    setMapArray(tmpI->content,"ref_wms_link",imyIndex,webService_url);
    if(datatype!=NULL && strncasecmp(datatype->value,"raster",6)==0){
      proto=2;
      freeMap(&rformat);
      free(rformat);
      rformat=createMap("mimeType","image/tiff");
    }
    else{
      proto=1;
      freeMap(&rformat);
      free(rformat);
      rformat=createMap("mimeType","text/xml");
    }
  }
  setMapArray(tmpI->content,"Reference",imyIndex,webService_url);
  memset(layers,0,128);
  sprintf(layers,options[proto][3],tmpI->name);
  protoVersion=options[proto][1];
  extent=getMapArray(tmpI->content,options[proto][4],imyIndex);
  memset(webService_url,0,strlen(webService_url));
  if(proto>0){
    if(proto==2)
      finalProto=1;
    sprintf(webService_url,
	    "%s?map=%s/%s_%d_%s.map&request=%s&service=%s&version=%s&%s&format=%s&bbox=%s&crs=%s",
	    msUrl->value,
	    dataPath->value,
	    tmpI->name,
	    imyIndex,
	    sid->value,
	    options[proto][2],
	    options[proto][0],
	    protoVersion,
	    layers,
	    rformat->value,
	    extent->value,
	    crs->value
	    );
    if(datatype!=NULL && strncasecmp(datatype->value,"raster",6)==0){
      setMapArray(tmpI->content,"ref_wcs_link",imyIndex,webService_url);
    }
    else{
      setMapArray(tmpI->content,"ref_wfs_link",imyIndex,webService_url);
    }
  }else{
    sprintf(webService_url,
	    "%s?map=%s/%s_%d_%s.map&request=%s&service=%s&version=%s&%s&width=%s&height=%s&format=%s&bbox=%s&crs=%s",
	    msUrl->value,
	    dataPath->value,
	    tmpI->name,
	    imyIndex,
	    sid->value,
	    options[proto][2],
	    options[proto][0],
	    protoVersion,
	    layers,
	    width->value,
	    height->value,
	    rformat->value,
	    extent->value,
	    crs->value
	    );
    setMapArray(tmpI->content,"ref_wms_link",imyIndex,webService_url);
  }
  if(finalProto>0){
    proto=3;
    memset(layers,0,128);
    sprintf(layers,options[proto][3],tmpI->name);
    protoVersion=options[proto][1];
    extent=getMapArray(tmpI->content,options[proto][4],imyIndex);
    memset(webService_url,0,strlen(webService_url));
     freeMap(&rformat);
    free(rformat);
    rformat=createMap("value","image/tiff");
    sprintf(webService_url,
	    "%s?map=%s/%s_%d_%s.map&request=%s&service=%s&version=%s&%s&format=%s&bbox=%s&crs=%s",
	    msUrl->value,
	    dataPath->value,
	    tmpI->name,
	    imyIndex,
	    sid->value,
	    options[proto][2],
	    options[proto][0],
	    protoVersion,
	    layers,
	    rformat->value,
	    extent->value,
	    crs->value
	    );
    setMapArray(tmpI->content,"ref_wcs_preview_link",imyIndex,webService_url);
  }
  if(hasCRS==0){
    freeMap(&crs);
    free(crs);
  }
  freeMap(&rformat);
  free(rformat);
  free(webService_url);
}

/**
 * Set projection for a layer in a MAPFILE using Authority Code and Name if
 * available or fallback to proj4 definition if available or fallback to
 * default EPSG:4326
 *
 * @param output the output maps
 * @param m the opened mapObj
 * @param myLayer the layerObj
 * @param pszProjection a char* containing the SRS definition in WKT format
 */
void setSrsInformations(maps* output,mapObj* m,layerObj* myLayer,
			char* pszProjection){
  OGRSpatialReferenceH  hSRS;
  map* msSrs=NULL;
  int imyIndex=getPublishedId(output);
  hSRS = OSRNewSpatialReference(NULL);
  if( pszProjection!=NULL && strlen(pszProjection)>1){
    if(OSRImportFromWkt( hSRS, &pszProjection ) == CE_None ){
      char *proj4Str=NULL;
      if(OSRGetAuthorityName(hSRS,NULL)!=NULL && 
	 OSRGetAuthorityCode(hSRS,NULL)!=NULL){
	char tmpSrs[20];
	sprintf(tmpSrs,"%s:%s",
		OSRGetAuthorityName(hSRS,NULL),OSRGetAuthorityCode(hSRS,NULL));
	msLoadProjectionStringEPSG(&m->projection,tmpSrs);
	msLoadProjectionStringEPSG(&myLayer->projection,tmpSrs);
	
	char tmpSrss[256];
	sprintf(tmpSrss,"EPSG:4326 EPSG:900913 EPSG:3857 %s",tmpSrs);
	
	msInsertHashTable(&(m->web.metadata), "ows_srs", tmpSrss);
	msInsertHashTable(&(myLayer->metadata), "ows_srs", tmpSrss);
	
#ifdef DEBUGMS
	fprintf(stderr,"isGeo %b\n\n",OSRIsGeographic(hSRS)==TRUE);
#endif
	if(output!=NULL){
	  if(OSRIsGeographic(hSRS)==TRUE)
	    setMapArray(output->content,"crs_isGeographic",imyIndex,"true");
	  else
	    setMapArray(output->content,"crs_isGeographic",imyIndex,"false");
	  setMapArray(output->content,"crs",imyIndex,tmpSrs);
	}
      }
      else{
	OSRExportToProj4(hSRS,&proj4Str);
	if(proj4Str!=NULL && strlen(proj4Str)>0){
#ifdef DEBUGMS
	  fprintf(stderr,"PROJ (%s)\n",proj4Str);
#endif
	  msLoadProjectionString(&(m->projection),proj4Str);
	  msLoadProjectionString(&(myLayer->projection),proj4Str);
	  if(output!=NULL){ 
	    if(OSRIsGeographic(hSRS)==TRUE)
	      setMapArray(output->content,"crs_isGeographic",imyIndex,"true");
	    else
	      setMapArray(output->content,"crs_isGeographic",imyIndex,"false");
	  }
	  free(proj4Str);
	}
	else{
	  msLoadProjectionStringEPSG(&m->projection,"EPSG:4326");
	  msLoadProjectionStringEPSG(&myLayer->projection,"EPSG:4326");
	  if(output!=NULL){
	    setMapArray(output->content,"crs_isGeographic",imyIndex,"true");
	  }
	}
	if(output!=NULL){
	  setMapArray(output->content,"crs",imyIndex,"EPSG:4326");
	  setMapArray(output->content,"real_extent",imyIndex,"true");
	}
	msInsertHashTable(&(m->web.metadata),"ows_srs", "EPSG:4326 EPSG:900913 EPSG:3857");
	msInsertHashTable(&(myLayer->metadata),"ows_srs","EPSG:4326 EPSG:900913 EPSG:3857");
      }
    }
  }
  else{
    if(output!=NULL){
      msSrs=getMapArray(output->content,"msSrs",imyIndex);
    }
    if(msSrs!=NULL){
      msLoadProjectionStringEPSG(&m->projection,msSrs->value);
      msLoadProjectionStringEPSG(&myLayer->projection,msSrs->value);
      char tmpSrs[128];
      sprintf(tmpSrs,"%s EPSG:4326 EPSG:900913 EPSG:3857",msSrs->value);
      msInsertHashTable(&(m->web.metadata),"ows_srs",tmpSrs);
      msInsertHashTable(&(myLayer->metadata),"ows_srs",tmpSrs);
    }else{
      msLoadProjectionStringEPSG(&m->projection,"EPSG:4326");
      msLoadProjectionStringEPSG(&myLayer->projection,"EPSG:4326");
      msInsertHashTable(&(m->web.metadata),"ows_srs","EPSG:4326 EPSG:900913 EPSG:3857");
      msInsertHashTable(&(myLayer->metadata),"ows_srs","EPSG:4326 EPSG:900913 EPSG:3857");
    }
    if(output!=NULL){
      setMapArray(output->content,"crs",imyIndex,msSrs->value);
      setMapArray(output->content,"crs_isGeographic",imyIndex,"true");
    }
  }

  OSRDestroySpatialReference( hSRS );
}

/**
 * Set the MAPFILE extent, the the ows_extent for the layer, add wms_extent and
 * wfs_extent to the output maps and call setMapSize.
 *
 * @param output the specific output
 * @param m the mapObj
 * @param myLayer the layerObj
 * @param minX the lower left x coordinate
 * @param minY the lower left y coordinate
 * @param maxX the upper right x coordinate
 * @param maxY the upper right y coordinate
 * @see setMapSize
 */
void setMsExtent(maps* output,mapObj* m,layerObj* myLayer,
		 double minX,double minY,double maxX,double maxY){
  int imyIndex=getPublishedId(output);
  msMapSetExtent(m,minX,minY,maxX,maxY);
  //m->maxsize=4096;
#ifdef DEBUGMS
  fprintf(stderr,"Extent %.15f %.15f %.15f %.15f\n",minX,minY,maxX,maxY);
#endif
  char tmpExtent[1024];
  sprintf(tmpExtent,"%.15f %.15f %.15f %.15f",minX,minY,maxX,maxY);
#ifdef DEBUGMS
  fprintf(stderr,"Extent %s\n",tmpExtent);
#endif
  msInsertHashTable(&(myLayer->metadata), "ows_extent", tmpExtent);
  
  if(output!=NULL){
    map* test=getMapArray(output->content,"real_extent",imyIndex);
    pointObj min, max;
    projectionObj tempSrs;
    min.x = m->extent.minx;
    min.y = m->extent.miny;
    max.x = m->extent.maxx;
    max.y = m->extent.maxy;
    char tmpSrsStr[1024];
    msInitProjection(&tempSrs);
    msLoadProjectionStringEPSG(&tempSrs,"EPSG:4326");

    msProjectPoint(&(myLayer->projection),&tempSrs,&min);
    msProjectPoint(&myLayer->projection,&tempSrs,&max);
  
    if(test!=NULL){
      sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f",min.y,min.x,max.y,max.x);
      map* isGeo=getMapArray(output->content,"crs_isGeographic",imyIndex);
#ifdef DEBUGMS
      fprintf(stderr,"isGeo = %s\n",isGeo->value);
#endif
      if(isGeo!=NULL && strcasecmp("true",isGeo->value)==0){
	sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f",min.y,min.x,max.y,max.x);
	setMapArray(output->content,"wgs84_extent",imyIndex,tmpExtent);
        sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f", minY,minX, maxY, maxX);
      }else{
	sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f",min.x,min.y,max.x,max.y);
	setMapArray(output->content,"wgs84_extent",imyIndex,tmpExtent);
      }
      setMapArray(output->content,"wms_extent",imyIndex,tmpExtent);
      sprintf(tmpSrsStr,"%.15f,%.15f,%.15f,%.15f",min.x,min.y,max.x,max.y);
      setMapArray(output->content,"wcs_extent",imyIndex,tmpExtent);
    }else{
      sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f",min.x,min.y,max.x,max.y);
      setMapArray(output->content,"wgs84_extent",imyIndex,tmpExtent);
      sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f",minX, minY, maxX, maxY);
      map* isGeo=getMapArray(output->content,"crs_isGeographic",imyIndex);
      if(isGeo!=NULL){
#ifdef DEBUGMS
	fprintf(stderr,"isGeo = %s\n",isGeo->value);
#endif
	if(isGeo!=NULL && strcasecmp("true",isGeo->value)==0)
	  sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f", minY,minX, maxY, maxX);
      }
      setMapArray(output->content,"wms_extent",imyIndex,tmpExtent); 
      sprintf(tmpExtent,"%.15f,%.15f,%.15f,%.15f",minX,minY,maxX,maxY);
      setMapArray(output->content,"wcs_extent",imyIndex,tmpExtent);
    }
  }

  setMapSize(output,minX,minY,maxX,maxY);
}

/**
 * Try to open a vector output and define the corresponding layer in the MAPFILE
 *
 * @param conf the conf maps containing the main.cfg settings
 * @param output the specific output maps
 * @param m the mapObj
 */
int tryOgr(maps* conf,maps* output,mapObj* m){
  int imyIndex=getPublishedId(output);
  map* tmpMap=getMapArray(output->content,"storage",imyIndex);
  char *pszDataSource=tmpMap->value;

  /**
   * Try to open the DataSource using OGR
   */
  OGRRegisterAll();
  /**
   * Try to load the file as ZIP
   *
  OGRDataSourceH poDS1 = NULL;
  OGRSFDriverH *poDriver1 = NULL;
  char *dsName=(char*)malloc((8+strlen(pszDataSource)+1)*sizeof(char));
  char *odsName=zStrdup(pszDataSource);
  char *sdsName=zStrdup(pszDataSource);
  char *demo=".data";
  sdsName[strlen(sdsName)-(strlen(demo)-1)]='d';
  sdsName[strlen(sdsName)-(strlen(demo)-2)]='i';
  sdsName[strlen(sdsName)-(strlen(demo)-3)]='r';
  sdsName[strlen(sdsName)-(strlen(demo)-4)]=0;

  odsName[strlen(odsName)-(strlen(demo)-1)]='z';
  odsName[strlen(odsName)-(strlen(demo)-2)]='i';
  odsName[strlen(odsName)-(strlen(demo)-3)]='p';
  odsName[strlen(odsName)-(strlen(demo)-4)]=0;
  sprintf(dsName,"/vsizip/%s",odsName);

#ifdef DEBUGMS
  fprintf(stderr,"Try loading %s, %s, %s\n",dsName,odsName,dsName);
#endif

  FILE* file = fopen(pszDataSource, "rb");
  FILE* fileZ = fopen(odsName, "wb");
  free(odsName);
  fseek(file, 0, SEEK_END);
  unsigned long fileLen=ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer=(char *)malloc(fileLen+1);
  fread(buffer, fileLen, 1, file);
  fwrite(buffer,fileLen, 1, fileZ);
  fclose(file);
  fclose(fileZ);
  free(buffer);
#ifdef DEBUGMS
  fprintf(stderr,"Try loading %s",dsName);
#endif
  poDS1 = OGROpen( dsName, FALSE, poDriver1 );
  if( poDS1 == NULL ){
    fprintf(stderr,"Unable to access the DataSource as ZIP File\n");
    setMapInMaps(conf,"lenv","message","Unable to open datasource in read only mode");
    fprintf(stderr,"Remove ZIP File!\n");
    unlink(odsName);
    //OGR_DS_Destroy(poDS1);
  }else{
#ifdef DEBUGMS
    fprintf(stderr,"The DataSource is a  ZIP File\n");
#endif
    char** demo=VSIReadDir(dsName);
    int i=0;
    zMkdir(sdsName
#ifndef WIN32
		,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH 
#endif
		);
    while(demo[i]!=NULL){
#ifdef DEBUGMS
      fprintf(stderr,"ZIP File content : %s\n",demo[i]);
#endif
      char *tmpDs=(char*)malloc((strlen(dsName)+strlen(demo[i])+2)*sizeof(char));
      sprintf(tmpDs,"%s/%s",dsName,demo[i]);
      fprintf(stderr,"read : %s\n",tmpDs);
      
      VSILFILE* vsif=VSIFOpenL(tmpDs,"rb");
#ifdef DEBUGMS
      fprintf(stderr,"open : %s\n",tmpDs);
#endif
      VSIFSeekL(vsif,0,SEEK_END);
      vsi_l_offset size=VSIFTellL(vsif);
#ifdef DEBUGMS
      fprintf(stderr,"size : %d\n",size);
#endif
      VSIFSeekL(vsif,0,SEEK_SET);
      char *vsifcontent=(char*) malloc(((int)size+1)*sizeof(char));
      VSIFReadL(vsifcontent,1,(size_t)size,vsif);
      char *fpath=(char*) malloc((strlen(sdsName)+strlen(demo[1])+2)*sizeof(char));
      sprintf(fpath,"%s/%s",sdsName,demo[i]);
      int f=zOpen(fpath,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
      zWrite(f,vsifcontent,(int)size);
      close(f);
      chmod(fpath,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
      char* tmpP=strstr(fpath,".shp");
      if(tmpP==NULL)
	tmpP=strstr(fpath,".SHP");
      if(tmpP!=NULL){
#ifdef DEBUGMS
	fprintf(stderr,"*** DEBUG %s\n",strstr(tmpP,"."));
#endif
	if( strcmp(tmpP,".shp")==0 || strcmp(tmpP,".SHP")==0 ){
	  tmpMap=getMap(output->content,"storage");
	  free(tmpMap->value);
	  tmpMap->value=(char*) malloc((strlen(fpath)+1)*sizeof(char));
	  sprintf(tmpMap->value,"%s",fpath);
	  pszDataSource=tmpMap->value;
#ifdef DEBUGMS
	  fprintf(stderr,"*** DEBUG %s\n",pszDataSource);
#endif
	}
      }
      VSIFCloseL(vsif);
      i++;
    }
    OGR_DS_Destroy(poDS1);
    }
  free(sdsName);
  free(dsName);*/
  
  OGRDataSourceH poDS = NULL;
  OGRSFDriverH *poDriver = NULL;
  poDS = OGROpen( pszDataSource, FALSE, poDriver );
  if( poDS == NULL ){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to access the DataSource %s\n",pszDataSource);
#endif
    setMapInMaps(conf,"lenv","message","Unable to open datasource in read only mode");
#ifdef DEBUGMS
    fprintf(stderr,"Unable to access the DataSource, exit! \n"); 
#endif
    return -1;
  }

  setMapArray(output->content,"gdalType",imyIndex,OGR_Dr_GetName(OGR_DS_GetDriver(poDS)));  
  setMapArray(output->content,"geodatatype",imyIndex,"vector");
  int iLayer = 0;
  for( iLayer=0; iLayer < OGR_DS_GetLayerCount(poDS); iLayer++ ){
    OGRLayerH poLayer = OGR_DS_GetLayer(poDS,iLayer);

    if( poLayer == NULL ){
#ifdef DEBUGMS
      fprintf(stderr,"Unable to access the DataSource Layer \n");
#endif
      setMapInMaps(conf,"lenv","message","Unable to open datasource in read only mode");
      return -1;
    }

    /**
     * Add a new layer set name, data
     */
    layerObj* myLayer=NULL;
    if(getMapArray(output->content,"msInclude",imyIndex)==NULL){
      if(msGrowMapLayers(m)==NULL){
	return -1;
      }
      if(initLayer((m->layers[m->numlayers]), m) == -1){
	return -1;
      }
      myLayer=m->layers[m->numlayers];
    }else{
      myLayer=m->layers[m->numlayers-1];
    }
    
#ifdef DEBUGMS
    dumpMaps(output);
#endif
    myLayer->name = zStrdup(output->name);
    myLayer->tileitem=NULL;
    myLayer->data = zStrdup(OGR_L_GetName(poLayer));
    myLayer->connection = zStrdup(pszDataSource);
    myLayer->index = m->numlayers;
    myLayer->dump = MS_TRUE;
    myLayer->status = MS_ON;
    msConnectLayer(myLayer,MS_OGR,pszDataSource);

    addIntToMapArray(output->content,"nb_features",imyIndex,OGR_L_GetFeatureCount(poLayer,1));

    /**
     * Detect the Geometry Type or use Polygon
     */
    if(OGR_L_GetGeomType(poLayer) != wkbUnknown){
      switch(OGR_L_GetGeomType(poLayer)){
      case wkbPoint:
      case wkbMultiPoint:
      case wkbPoint25D:
      case wkbMultiPoint25D:
#ifdef DEBUGMS
	fprintf(stderr,"POINT DataSource Layer \n");
#endif
	myLayer->type = MS_LAYER_POINT;
	break;
      case wkbLineString :
      case wkbMultiLineString :
      case wkbLineString25D:
      case wkbMultiLineString25D:
#ifdef DEBUGMS
	fprintf(stderr,"LINE DataSource Layer \n");
#endif
	myLayer->type = MS_LAYER_LINE;
	break;
      case wkbPolygon:
      case wkbMultiPolygon:
      case wkbPolygon25D:
      case wkbMultiPolygon25D:
#ifdef DEBUGMS
	fprintf(stderr,"POLYGON DataSource Layer \n");
#endif
	myLayer->type = MS_LAYER_POLYGON;
	break;
      default:
	myLayer->type = MS_LAYER_POLYGON;
	break;
      }
    }else
      myLayer->type = MS_LAYER_POLYGON;

    /**
     * Detect spatial reference or use WGS84
     */
    OGRSpatialReferenceH srs=OGR_L_GetSpatialRef(poLayer);
    if(srs!=NULL){
      char *wkt=NULL;
      OSRExportToWkt(srs,&wkt);
      setSrsInformations(output,m,myLayer,wkt);
      free(wkt);
    }
    else{
      setMapArray(output->content,"crs",imyIndex,"EPSG:4326");
      setMapArray(output->content,"crs_isGeographic",imyIndex,"true");
      msLoadProjectionStringEPSG(&m->projection,"EPSG:4326");
      msInsertHashTable(&(m->web.metadata), "ows_srs", "EPSG:4326 EPSG:900913 EPSG:3857");
      msInsertHashTable(&(myLayer->metadata), "ows_srs", "EPSG:4326 EPSG:900913 EPSG:3857");
    }

    OGREnvelope oExt;
    if (OGR_L_GetExtent(poLayer,&oExt, TRUE) == OGRERR_NONE){
      setMsExtent(output,m,myLayer,oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
      char extent[1024];
      memset(&extent,0,1024);
      sprintf(extent,"%d,%d,%d,%d",oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
      setMapArray(output->content,"boundingbox",imyIndex,extent);
    }
  
    /**
     * Detect the FID column or use the first attribute field as FID
     */
    char *fid=(char*)OGR_L_GetFIDColumn(poLayer);
    if(strlen(fid)==0){
      OGRFeatureDefnH def=OGR_L_GetLayerDefn(poLayer);
      int fIndex=0;
      for(fIndex=0;fIndex<OGR_FD_GetFieldCount(def);fIndex++){
	OGRFieldDefnH fdef=OGR_FD_GetFieldDefn(def,fIndex);
	fid=(char*)OGR_Fld_GetNameRef(fdef);
	break;
      }
    }
    msInsertHashTable(&(myLayer->metadata), "gml_featureid", fid);
    msInsertHashTable(&(myLayer->metadata), "gml_include_items", "all");
    msInsertHashTable(&(myLayer->metadata), "ows_name", output->name);
    map* tmpMap=getMapArray(output->content,"title",imyIndex);
    if(tmpMap!=NULL)
      msInsertHashTable(&(myLayer->metadata), "ows_title", tmpMap->value);
    else
      msInsertHashTable(&(myLayer->metadata), "ows_title", "Default Title");

    if(getMapArray(output->content,"msInclude",imyIndex)==NULL){
      if(msGrowLayerClasses(myLayer) == NULL)
	return -1;
      if(initClass((myLayer->CLASS[myLayer->numclasses])) == -1)
	return -1;
      if(msGrowClassStyles(myLayer->CLASS[myLayer->numclasses]) == NULL)
	return -1;
      if(initStyle(myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]) == -1)
	return -1;
      /**
       * Apply msStyle else fallback to the default style
       */
      tmpMap=getMap(output->content,"msStyle");
      if(tmpMap==NULL){
        switch(myLayer->type){
	case MS_LAYER_POLYGON:
	  tmpMap=getMapFromMaps(conf,"main","msStylePoly");
	  break;
	case MS_LAYER_LINE:
	  tmpMap=getMapFromMaps(conf,"main","msStyleLine");
	  break;
	default:
	  tmpMap=getMapFromMaps(conf,"main","msStylePoint");
	  break;
        }
      }
      if(tmpMap!=NULL)
	msUpdateStyleFromString(myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles],tmpMap->value,0);
      else{
	/**
	 * Set style
	 */
	myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.red=125;
	myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.green=125;
	myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.blue=255;
	myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->outlinecolor.red=80;
	myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->outlinecolor.green=80;
	myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->outlinecolor.blue=80;
	
	/**
	 * Set specific style depending on type
	 */
	if(myLayer->type == MS_LAYER_POLYGON)
	  myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->width=3;
	if(myLayer->type == MS_LAYER_LINE){
	  myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->width=3;
	  myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->outlinewidth=1.5;
	}
	if(myLayer->type == MS_LAYER_POINT){
	  myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->symbol=1;
	  myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->size=15;
	}
	
      }
      myLayer->CLASS[myLayer->numclasses]->numstyles++;
      myLayer->numclasses++;
    
      m->layerorder[m->numlayers] = m->numlayers;
      m->numlayers++;

    }
  }
  OGR_DS_Destroy(poDS);
  //OGRCleanupAll();

  return 1;
}

/**
 * Try to open a raster output and define the corresponding layer in the MAPFILE
 *
 * @param conf the conf maps containing the main.cfg settings
 * @param output the specific output maps
 * @param m the mapObj
 */
int tryGdal(maps* conf,maps* output,mapObj* m){
  int imyIndex=getPublishedId(output);
  map* tmpMap=getMapArray(output->content,"storage",imyIndex);
  map* styleMap=getMap(output->content,"msStyle");
  char *pszFilename=tmpMap->value;
  GDALDatasetH hDataset;
  GDALRasterBandH hBand;
  double adfGeoTransform[6];
  int i, iBand;
  
  /**
   * Try to open the DataSource using GDAL
   */
  GDALAllRegister();
  hDataset = GDALOpen( pszFilename, GA_Update ); /*GA_ReadOnly*/
  if( hDataset == NULL ){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to access the DataSource %s \n",pszFilename);
#endif
    setMapArray(output->content,"geodatatype",imyIndex,"other");
    setMapInMaps(conf,"lenv","message","gdalinfo failed - unable to open");
    GDALDestroyDriverManager();
    return -1;
  }
#ifdef DEBUGMS
  fprintf(stderr,"Accessing the DataSource %s %d\n",pszFilename,__LINE__);
#endif

  setMapArray(output->content,"geodatatype",imyIndex,"raster");
  /**
   * Add a new layer set name, data
   */
  if(msGrowMapLayers(m)==NULL){
    return -1;
  }
  if(initLayer((m->layers[m->numlayers]), m) == -1){
    return -1;
  }
  m->layers[m->numlayers]->index=m->numlayers;

  layerObj* myLayer=m->layers[m->numlayers];
  myLayer->name = zStrdup(output->name);
  myLayer->tileitem=NULL;
  myLayer->data = zStrdup(pszFilename);
  myLayer->index = m->numlayers;
  myLayer->dump = MS_TRUE;
  myLayer->status = MS_ON;
  myLayer->type = MS_LAYER_RASTER;

  char *title=output->name;
  tmpMap=getMapArray(output->content,"title",imyIndex);
  if(tmpMap!=NULL)
    title=tmpMap->value;
  char *abstract=output->name;
  tmpMap=getMapArray(output->content,"abstract",imyIndex);
  if(tmpMap!=NULL)
    abstract=tmpMap->value;

  msInsertHashTable(&(myLayer->metadata), "ows_label", title);
  msInsertHashTable(&(myLayer->metadata), "ows_title", title);
  msInsertHashTable(&(myLayer->metadata), "ows_abstract", abstract);
  msInsertHashTable(&(myLayer->metadata), "ows_rangeset_name", output->name);
  msInsertHashTable(&(myLayer->metadata), "ows_rangeset_label", title);

  /**
   * Set Map Size to the raster size
   */
  m->width=GDALGetRasterXSize( hDataset );
  m->height=GDALGetRasterYSize( hDataset );
  if(m->width>4096 || m->height>4096){
    if(m->width>m->height)
      m->maxsize=m->width;
    else  
      m->maxsize=m->height;
  }else
    m->maxsize=4096;
  addIntToMapArray(output->content,"nb_pixels",imyIndex,GDALGetRasterXSize( hDataset )*GDALGetRasterYSize( hDataset ));
  int pixel_type=GDALGetRasterDataType( hDataset );
  addIntToMapArray(output->content,"pixel_data_type",imyIndex,pixel_type);

  int outputIndex=msGetOutputFormatIndex(m,"tiff");
  if(outputIndex>=0){
    m->outputformatlist[outputIndex]->imagemode=((pixel_type==GDT_Byte)?MS_IMAGEMODE_BYTE:((pixel_type==GDT_Int16 || pixel_type==GDT_UInt16)?MS_IMAGEMODE_INT16:((pixel_type!=GDT_Float32)?MS_IMAGEMODE_FLOAT32:MS_IMAGEMODE_BYTE)));
    outputIndex=msGetOutputFormatIndex(m,"geotiff");
    if(outputIndex>=0)
      m->outputformatlist[outputIndex]->imagemode=((pixel_type==GDT_Byte)?MS_IMAGEMODE_BYTE:((pixel_type==GDT_Int16 || pixel_type==GDT_UInt16)?MS_IMAGEMODE_INT16:((pixel_type!=GDT_Float32)?MS_IMAGEMODE_FLOAT32:MS_IMAGEMODE_BYTE)));
  }
  //
    
  /**
   * Set projection using Authority Code and Name if available or fallback to 
   * proj4 definition if available or fallback to default EPSG:4326
   */
  const char *tRef=GDALGetProjectionRef( hDataset );
  if( tRef != NULL && strlen(tRef)>0 ){
    char *pszProjection;
    pszProjection = (char *) GDALGetProjectionRef( hDataset );
#ifdef DEBUGMS
    fprintf(stderr,"Accessing the DataSource %s\n",GDALGetProjectionRef( hDataset ));
#endif
    setSrsInformations(output,m,myLayer,pszProjection);
  }else{
    fprintf(stderr,"NO SRS FOUND %s %d ! %s\n",__FILE__,__LINE__,GDALGetProjectionRef( hDataset ));
    fflush(stderr);
    CPLErr sp=GDALSetProjection( hDataset , "+init=epsg:4326" );
    if(sp!=CE_None){
      fprintf(stderr,"NO SRS SET ! %s\n",CPLGetLastErrorMsg());
    }
  }

  /**
   * Set extent
   */
  if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None ){
    if( adfGeoTransform[2] == 0.0 && adfGeoTransform[4] == 0.0 ){

      double minX = adfGeoTransform[0]
	+ adfGeoTransform[2] * GDALGetRasterYSize(hDataset);
      double minY = adfGeoTransform[3]
	+ adfGeoTransform[5] * GDALGetRasterYSize(hDataset);

      double maxX = adfGeoTransform[0]
	+ adfGeoTransform[1] * GDALGetRasterXSize(hDataset);
      double maxY = adfGeoTransform[3]
	+ adfGeoTransform[4] * GDALGetRasterXSize(hDataset);

      setMsExtent(output,m,myLayer,minX,minY,maxX,maxY);
      char extent[1024];
      memset(&extent,0,1024);
      sprintf(extent,"%d,%d,%d,%d",minX,minY,maxX,maxY);
      setMapArray(output->content,"boundingbox",imyIndex,extent);
    }
  }else{
    int scale=1;
    if(m->width>2048){
      addIntToMapArray(output->content,"width",imyIndex,2048);
      scale=2048/m->width;
    }else
      addIntToMapArray(output->content,"width",imyIndex,m->width);
    addIntToMapArray(output->content,"height",imyIndex,m->height*scale);
  }

  /**
   * Extract information about available bands to set the bandcount and the
   * processing directive
   */
  char nBands[3];
  memset(&nBands,0,3);
  int nBandsI=GDALGetRasterCount( hDataset );
  if(nBandsI<100){
    sprintf(nBands,"%d",GDALGetRasterCount( hDataset ));
    msInsertHashTable(&(myLayer->metadata), "ows_bandcount", nBands);
  }
  if(styleMap==NULL || strstr(styleMap->value,"BANDS=")==NULL){
    if(nBandsI>=3)
      if(nBandsI==4)
	msLayerAddProcessing(myLayer,"BANDS=1,2,3,4");
      else
	msLayerAddProcessing(myLayer,"BANDS=1,2,3");
    else if(nBandsI>=2)
      msLayerAddProcessing(myLayer,"BANDS=1,2");
    else
      msLayerAddProcessing(myLayer,"BANDS=1");
  }

  /**
   * Name available Bands
   */
  char lBands[7];
  char *nameBands=NULL;
  for( iBand = 0; iBand < nBandsI; iBand++ ){
    memset(&lBands,0,7);
    sprintf(lBands,"Band%d",iBand+1);    
    if(nameBands==NULL){
      nameBands=(char*)malloc((strlen(lBands)+1)*sizeof(char));
      sprintf(nameBands,"%s",lBands);
    }else{
      /*if(iBand<4)*/{
	char *tmpS=zStrdup(nameBands);
	nameBands=(char*)realloc(nameBands,(strlen(tmpS)+strlen(lBands)+2)*sizeof(char));
	sprintf(nameBands,"%s %s",tmpS,lBands);
	free(tmpS);
      }
    }
  }
  if(nameBands!=NULL){
    msInsertHashTable(&(myLayer->metadata), "ows_bandnames", nameBands);
    free(nameBands);
  }

  /**
   * Loops over metadata information to setup specific information
   */
  for( iBand = 0; iBand < nBandsI; iBand++ ){
    double      pdfMin, pdfMax, pdfMean, pdfStdDev;
    hBand = GDALGetRasterBand( hDataset, iBand+1 );

    CPLErrorReset();
    GDALGetRasterStatistics( hBand, TRUE, TRUE, &pdfMin, &pdfMax, &pdfMean, &pdfStdDev);
    char tmpN[21];
    sprintf(tmpN,"Band%d",iBand+1);
    if (CPLGetLastErrorType() == CE_None){
      char tmpMm[100],tmpMp[100],tmpNb[3];
      sprintf(tmpMm,"%.3f %.3f",pdfMin,pdfMax);
      if(pdfMin!=pdfMax && (styleMap==NULL || strstr(styleMap->value,"SCALE_")==NULL)){
        sprintf(tmpNb,"%d",iBand+1);
        if(styleMap==NULL || strstr(styleMap->value,"BANDS=")==NULL || strstr(strstr(styleMap->value,"BANDS="),tmpNb)!=NULL){ 
	  if(pdfMean-(2*pdfStdDev)<0)
	    sprintf(tmpMp,"SCALE_%d=%.3f,%.3f",iBand+1,0.0,pdfMean+(2*pdfStdDev));
	  else
	    sprintf(tmpMp,"SCALE_%d=%.3f,%.3f",iBand+1,pdfMean-(2*pdfStdDev),pdfMean+(2*pdfStdDev));
	  msLayerAddProcessing(myLayer,tmpMp);
        }
      }
      char tmpI[31];      
      sprintf(tmpI,"%s_interval",tmpN);
      msInsertHashTable(&(myLayer->metadata), tmpI, tmpMm);
      map* test=getMap(output->content,"msClassify");
      if(test!=NULL && strncasecmp(test->value,"true",4)==0){
	/**
	 * Classify one band raster pixel value using regular interval
	 */
	int _tmpColors[10][3]={
	  {102,153,204},
	  {51,102,153},
	  {102,102,204},
	  {51,204,0},
	  {153,255,102},
	  {204,255,102},
	  {102,204,153},
	  {255,69,64},
	  {255,192,115},
	  {255,201,115}
	};
	  
	if(nBandsI==1){
	  double delta=pdfMax-pdfMin;
	  double interval=delta/10;
	  double cstep=pdfMin;
	  for(i=0;i<10;i++){
	    /**
	     * Create a new class
	     */
	    if(msGrowLayerClasses(myLayer) == NULL)
	      return -1;
	    if(initClass((myLayer->CLASS[myLayer->numclasses])) == -1)
	      return -1;
	    if(msGrowClassStyles(myLayer->CLASS[myLayer->numclasses]) == NULL)
	      return -1;
	    if(initStyle(myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]) == -1)
	      return -1;
	    
	    /**
	     * Set class name
	     */
	    char className[7];
	    sprintf(className,"class%d",i);
	    myLayer->CLASS[myLayer->numclasses]->name=zStrdup(className);
	    
	    /**
	     * Set expression
	     */
	    char expression[1024];
	    if(i+1<10)
	      sprintf(expression,"([pixel]>=%.3f AND [pixel]<%.3f)",cstep,cstep+interval);
	    else
	      sprintf(expression,"([pixel]>=%.3f AND [pixel]<=%.3f)",cstep,cstep+interval);
	    msLoadExpressionString(&myLayer->CLASS[myLayer->numclasses]->expression,expression);
	    
	    /**
	     * Set color
	     */
	    myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.red=_tmpColors[i][0];
	    myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.green=_tmpColors[i][1];
	    myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.blue=_tmpColors[i][2];
	    cstep+=interval;
	    myLayer->CLASS[myLayer->numclasses]->numstyles++;
	    myLayer->numclasses++;
	    
	  }
	  
	  char tmpMm[100];
	  sprintf(tmpMm,"%.3f %.3f",pdfMin,pdfMax);
	  
	}
      }
      else{
	if(nBandsI==1){
	  myLayer->offsite.red=0;
	  myLayer->offsite.green=0;
	  myLayer->offsite.blue=0;
	}
      }
    }
    if( strlen(GDALGetRasterUnitType(hBand)) > 0 ){
      char tmpU[31];
      sprintf(tmpU,"%s_band_uom",tmpN);
      msInsertHashTable(&(myLayer->metadata), tmpU, GDALGetRasterUnitType(hBand));
    }

  }
  if(styleMap==NULL || strstr(styleMap->value,"RESAMPLE")==NULL)
    msLayerAddProcessing(myLayer,"RESAMPLE=BILINEAR");
  
  if(styleMap!=NULL && strlen(styleMap->value)>9){
    msUpdateLayerFromString(myLayer,styleMap->value,MS_FALSE);
  }
  
  m->layerorder[m->numlayers] = m->numlayers;
  m->numlayers++;
  GDALClose( hDataset );
  GDALDestroyDriverManager();
  CPLCleanupTLS();
  storeMd5(pszFilename);
  return 1;
}

/**
 * Create a MapFile for WMS, WFS or WCS Service output
 *
 * @param conf the conf maps containing the main.cfg settings
 * @param outputs a specific output maps
 */
void outputMapfile(maps* conf,maps* outputs){
  /**
   * First store the value on disk
   */
  int imyIndex=getPublishedId(outputs);
  map* mime=getMapArray(outputs->content,"mimeType",imyIndex);
  map* msUrl=getMapFromMaps(conf,"main","mapserverAddress");
  map* dataPath=getMapFromMaps(conf,"main","dataPath");
  char *ext="data";
  if(mime!=NULL)
    if(strncasecmp(mime->value,"application/json",16)==0)
      ext="json";

  map* storage=getMapArray(outputs->content,"storage",imyIndex);
  if(storage==NULL){
    map* tmpMap=getMapFromMaps(conf,"main","dataPath");
    map* sidMap=getMapFromMaps(conf,"lenv","usid");
    char *pszDataSource=(char*)malloc((strlen(tmpMap->value)+strlen(sidMap->value)+strlen(outputs->name)+17)*sizeof(char));
    sprintf(pszDataSource,"%s/ZOO_DATA_%d_%s_%s.%s",tmpMap->value,imyIndex,outputs->name,sidMap->value,ext);
    int f=zOpen(pszDataSource,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
    map *gfile=getMapArray(outputs->content,"generated_file",imyIndex);
    if(gfile!=NULL){
      readGeneratedFile(conf,outputs->content,gfile->value);	    
    }
    map* sizeMap=getMapArray(outputs->content,"size",imyIndex);
    map* vData=getMapArray(outputs->content,"value",imyIndex);
    if(sizeMap!=NULL){
      zWrite(f,vData->value,atoi(sizeMap->value)*sizeof(char));
    }
    else{
      zWrite(f,vData->value,(strlen(vData->value)+1)*sizeof(char));
    }
    close(f);
    setMapArray(outputs->content,"storage",imyIndex,pszDataSource);
    free(pszDataSource);
  }

  /*
   * Create an empty map, set name, default size and extent
   */
  map* mapfileTemplate=getMapArray(outputs->content,"msInclude",imyIndex);
  mapObj *myMap=NULL;
  if(mapfileTemplate==NULL){
    myMap=msNewMapObj();
  }
  else{
    map* dataPath=getMapFromMaps(conf,"main","dataPath");
    map* sid=getMapFromMaps(conf,"lenv","sid");
    char *mapfileTemplatePath=(char*)malloc(((strlen(dataPath->value)+strlen(sid->value)+strlen(outputs->name)+10)*sizeof(char)));
    sprintf(mapfileTemplatePath,"%s/%s_%s.map",dataPath->value,outputs->name,sid->value);
    myMap=msLoadMap(mapfileTemplate->value,mapfileTemplatePath);
    if(myMap==NULL){
      setMapInMaps(conf,"lenv","message",_("Unable to open your template mapfile!"));
      return ;
    }
  }
  free(myMap->name);
  myMap->name=zStrdup("ZOO-Project_WXS_Server");
  msMapSetSize(myMap,2048,2048);
  msMapSetExtent(myMap,-1,-1,1,1);
  
  /*
   * Set imagepath and imageurl using tmpPath and tmpUrl from main.cfg
   */
  map *tmp1=getMapFromMaps(conf,"main","tmpPath");
  myMap->web.imagepath=zStrdup(tmp1->value);
  tmp1=getMapFromMaps(conf,"main","tmpUrl");
  myMap->web.imageurl=zStrdup(tmp1->value);
  
  /*
   * Define supported output formats
   */
  outputFormatObj *o1=msCreateDefaultOutputFormat(NULL,"AGG/PNG","png");
  o1->imagemode=MS_IMAGEMODE_RGBA;
  o1->transparent=MS_TRUE;
  o1->inmapfile=MS_TRUE;
  msAppendOutputFormat(myMap,msCloneOutputFormat(o1));
  msFreeOutputFormat(o1);

#ifdef USE_KML
  outputFormatObj *o2=msCreateDefaultOutputFormat(NULL,"KML","kml");
  if(!o2){
    perror("Unable to initialize KML driver");
    fprintf(stderr,"Unable to initialize KML driver !\n");
  }else{
    o2->inmapfile=MS_TRUE;  
    msAppendOutputFormat(myMap,msCloneOutputFormat(o2));
    msFreeOutputFormat(o2);
  }
#endif

  outputFormatObj *o3=msCreateDefaultOutputFormat(NULL,"GDAL/GTiff","tiff");
  if(!o3)
    fprintf(stderr,"Unable to initialize GDAL driver !\n");
  else{
    o3->imagemode=MS_IMAGEMODE_INT16;
    o3->inmapfile=MS_TRUE;  
    msAppendOutputFormat(myMap,msCloneOutputFormat(o3));
    msFreeOutputFormat(o3);
  }

  outputFormatObj *o4=msCreateDefaultOutputFormat(NULL,"GDAL/AAIGRID","grd");
  if(!o4)
    fprintf(stderr,"Unable to initialize GDAL driver !\n");
  else{
    o4->imagemode=MS_IMAGEMODE_BYTE;
    o4->inmapfile=MS_TRUE;  
    msAppendOutputFormat(myMap,msCloneOutputFormat(o4));
    msFreeOutputFormat(o4);
  }

#ifdef USE_CAIRO
  outputFormatObj *o5=msCreateDefaultOutputFormat(NULL,"CAIRO/PNG","cairopng");
  if(!o5)
    fprintf(stderr,"Unable to initialize CAIRO driver !\n");
  else{
    o5->imagemode=MS_IMAGEMODE_RGBA;
    o5->transparent=MS_TRUE;
    o5->inmapfile=MS_TRUE;
    msAppendOutputFormat(myMap,msCloneOutputFormat(o5));
    msFreeOutputFormat(o5);
  }
#endif

  
  outputFormatObj *o6=msCreateDefaultOutputFormat(NULL,"GDAL/GTiff","geotiff");
  if(!o6)
    fprintf(stderr,"Unable to initialize GDAL driver !\n");
  else{
    o6->imagemode=MS_IMAGEMODE_BYTE;
    o6->mimetype=strdup("image/geotiff");
    o6->inmapfile=MS_TRUE;
    msAppendOutputFormat(myMap,msCloneOutputFormat(o6));
    msFreeOutputFormat(o6);
  }

  
  /*
   * Set default projection to EPSG:4326
   */
  msLoadProjectionStringEPSG(&myMap->projection,"EPSG:4326");
  myMap->transparent=1;

  /*
   * Set mapserver PROJ_LIB/GDAL_DATA or any other config parameter from 
   * the main.cfg [mapserver] section if any
   */
  maps *tmp3=getMaps(conf,"mapserver");
  if(tmp3!=NULL){
    map* tmp4=tmp3->content;
    while(tmp4!=NULL){
      msSetConfigOption(myMap,tmp4->name,tmp4->value);
      tmp4=tmp4->next;
    }
  }

  /**
   * Set a ows_rootlayer_title,  
   */
  if (msInsertHashTable(&(myMap->web.metadata), "ows_rootlayer_name", "ZOO_Project_Layer") == NULL){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to add metadata");
#endif
    return;
  }
  if (msInsertHashTable(&(myMap->web.metadata), "ows_rootlayer_title", "ZOO_Project_Layer") == NULL){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to add metadata");
#endif
    return;
  }

  /**
   * Enable all the WXS requests using ows_enable_request
   * see http://mapserver.org/trunk/development/rfc/ms-rfc-67.html
   */
  if (msInsertHashTable(&(myMap->web.metadata), "ows_enable_request", "*") == NULL){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to add metadata");
#endif
    return;
  }
  msInsertHashTable(&(myMap->web.metadata), "ows_srs", "EPSG:4326");

  /**
   * Set metadata extracted from main.cfg file maps
   */
  maps* cursor=conf;
  map* correspondance=getCorrespondance();
  while(cursor!=NULL){
    if(strstr(cursor->name,"_help")==NULL){
      map* _cursor=cursor->content;
      map* vMap;
      while(_cursor!=NULL){
	if((vMap=getMap(correspondance,_cursor->name))!=NULL){
	  if (msInsertHashTable(&(myMap->web.metadata), vMap->value, _cursor->value) == NULL){
#ifdef DEBUGMS
	    fprintf(stderr,"Unable to add metadata");
#endif
	    freeMap(&correspondance);
	    free(correspondance);
	    return;
	  }
	}
	_cursor=_cursor->next;
      }
    }
    cursor=cursor->next;
  }
  freeMap(&correspondance);
  free(correspondance);

  map* sid=getMapFromMaps(conf,"lenv","usid");
  char *mapPath=
    (char*)malloc((14+strlen(sid->value)+strlen(outputs->name)+strlen(dataPath->value))*sizeof(char));
  sprintf(mapPath,"%s/%s_%d_%s.map",dataPath->value,outputs->name,imyIndex,sid->value);
  char *mapUrl=
    (char*)malloc((6+strlen(mapPath)+strlen(msUrl->value))*sizeof(char));
  sprintf(mapUrl,"%s?map=%s",msUrl->value,mapPath);

  if (msInsertHashTable(&(myMap->web.metadata), "ows_onlineresource", mapUrl) == NULL){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to add metadata");
#endif
    return;
  }

  if(tryOgr(conf,outputs,myMap)<0)
    if(tryGdal(conf,outputs,myMap)<0)
      return ;

  char *tmpPath=(char*)malloc((13+strlen(dataPath->value))*sizeof(char));
  sprintf(tmpPath,"%s/symbols.sym",dataPath->value);
  msInitSymbolSet(&myMap->symbolset);
  myMap->symbolset.filename=zStrdup(tmpPath);
  free(tmpPath);

  msSaveMap(myMap,mapPath);
  saveMapNames(conf,outputs,mapPath);
  free(mapPath);
  //free(myMap->symbolset.filename);
  //msFreeSymbolSet(&myMap->symbolset);
  msFreeMap(myMap);
  //msFree(myMap);
  msGDALCleanup();
}

/**
 * Save the map fullpath in a text file (.maps)
 * @param conf the main configuration map pointer
 * @param output the current output for which a mapfile has been generated
 * @param mapfile the mapfile saved to store in the text file
 */
void saveMapNames(maps* conf,maps* output,char* mapfile){
  map* storage=getMap(output->content,"storage");
  char *tmp=zStrdup(storage->value);
  tmp[strlen(tmp)-strlen(strrchr(tmp,'.'))]=0;
  char* mapName=(char*)malloc((strlen(tmp)+6)*sizeof(char*));
  sprintf(mapName,"%s.maps",tmp);
  FILE* myMaps=fopen(mapName,"a");
  if(myMaps!=NULL){
    fprintf(myMaps,"%s\n",mapfile);
    fclose(myMaps);
  }
  free(mapName);
  free(tmp);
}
