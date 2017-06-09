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

static double PRECISION = 0.001;
static int MAX_NUMBER_STRING_SIZE = 32;

/**
 * Double to ASCII
 */
char * dtoa(char *s, double n) {

  // handle special cases
  if (isnan(n)) {
    strcpy(s, "nan");
  } else if (isinf(n)) {
    strcpy(s, "inf");
  } else if (n == 0.0) {
    strcpy(s, "0");
  } else {
    int digit, m, m1;
    char *c = s;
    int neg = (n < 0);
    if (neg)
      n = -n;
    // calculate magnitude
    m = log10(n);
    int useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
    if (neg)
      *(c++) = '-';
    // set up for scientific notation
    if (useExp) {
      if (m < 0)
        m -= 1.0;
      n = n / pow(10.0, m);
      m1 = m;
      m = 0;
    }
    if (m < 1.0) {
      m = 0;
    }
    // convert the number
    while (n > PRECISION || m >= 0) {
      double weight = pow(10.0, m);
      if (weight > 0 && !isinf(weight)) {
        digit = floor(n / weight);
        n -= (digit * weight);
        *(c++) = '0' + digit;
      }
      if (m == 0 && n > 0)
        *(c++) = '.';
      m--;
    }
    if (useExp) {
      // convert the exponent
      int i, j;
      *(c++) = 'e';
      if (m1 > 0) {
        *(c++) = '+';
      } else {
        *(c++) = '-';
        m1 = -m1;
      }
      m = 0;
      while (m1 > 0) {
        *(c++) = '0' + m1 % 10;
        m1 /= 10;
        m++;
      }
      c -= m;
      for (i = 0, j = m-1; i<j; i++, j--) {
        // swap without temporary
        c[i] ^= c[j];
        c[j] ^= c[i];
        c[i] ^= c[j];
      }
      c += m;
    }
    *(c) = '\0';
  }
  return s;
}

/**
 * List of allowed raster styles and sub-styles
 */
enum MS_RASTER_STYLES{
  LINEAR_STRETCHING,
  CLASSIFY
};
enum LINEAR_STRETCHING_TYPE{
  MINMAX,
  MEANSTD
};
enum CLASSES_TYPE{
  AUTO,
  USER
};

/*
 * Functions producing the 'jet' colormap
 */
double interpolate( double val, double y0, double x0, double y1, double x1 ) {
    return (val-x0)*(y1-y0)/(x1-x0) + y0;
}

double base( double val ) {
    if ( val <= 0.25 ) return 0;
    else if ( val <= 0.75 ) return interpolate( val, 0.0, 0.25, 1.0, 0.75 );
    else if ( val <= 1.25 ) return 1.0;
    else if ( val <= 1.75 ) return interpolate( val, 1.0, 1.25, 0.0, 1.75 );
    else return 0.0;
}

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
    addToMap(output->content,"width",sWidth);
    addToMap(output->content,"height",sHeight);
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
  outputMapfile(m,tmpI);
  map *msUrl=getMapFromMaps(m,"main","mapserverAddress");
  if(msUrl==NULL){
    errorException (m, _("Unable to find any mapserverAddress defined in the main.cfg file"),
		    "InternalError", NULL);
    exit(-1);
  }
  map *msOgcVersion=getMapFromMaps(m,"main","msOgcVersion");
  map *dataPath=getMapFromMaps(m,"main","dataPath");
  map *sid=getMapFromMaps(m,"lenv","usid");
  map* format=getMap(tmpI->content,"mimeType");
  map* rformat=getMap(tmpI->content,"requestedMimeType");
  map* width=getMap(tmpI->content,"width");
  map* height=getMap(tmpI->content,"height");
  map* protoMap=getMap(tmpI->content,"msOgc");
  map* versionMap=getMap(tmpI->content,"msOgcVersion");
  char options[3][5][25]={
    {"WMS","1.3.0","GetMap","layers=%s","wms_extent"},
    {"WFS","1.1.0","GetFeature","typename=%s","wcs_extent"},
    {"WCS","1.1.0","GetCoverage","coverage=%s","wcs_extent"}
  };
  int proto=0;
  if(rformat==NULL){
    rformat=getMap(tmpI->content,"mimeType");
  }
  if(strncasecmp(rformat->value,"text/xml",8)==0)
    proto=1;
  if(strncasecmp(rformat->value,"image/tiff",10)==0)
    proto=2;
  if(protoMap!=NULL){
    if(strncasecmp(protoMap->value,"WMS",3)==0)
      proto=0;
    else{
      if(strncasecmp(protoMap->value,"WFS",3)==0)
	proto=1;
      else 
	proto=2;
    }
  }
  char *protoVersion=options[proto][1];
  if(proto==1){
    if(msOgcVersion!=NULL)
      protoVersion=msOgcVersion->value;
    if(versionMap!=NULL)
      protoVersion=versionMap->value;
  }

  map* extent=getMap(tmpI->content,options[proto][4]);
  map* crs=getMap(tmpI->content,"crs");
  int hasCRS=1;
  if(crs==NULL){
    crs=getMapFromMaps(m,"main","crs");
    if(crs==NULL){
      crs=createMap("crs","epsg:4326");
      hasCRS=0;
    }
  }
  char layers[128];
  sprintf(layers,options[proto][3],tmpI->name);

  char* webService_url=(char*)malloc((strlen(msUrl->value)+strlen(format->value)+strlen(tmpI->name)+strlen(width->value)+strlen(height->value)+strlen(extent->value)+256)*sizeof(char));

  if(proto>0){
    sprintf(webService_url,
	    "%s?map=%s/%s_%s.map&request=%s&service=%s&version=%s&%s&format=%s&bbox=%s&crs=%s",
	    msUrl->value,
	    dataPath->value,
	    tmpI->name,
	    sid->value,
	    options[proto][2],
	    options[proto][0],
	    protoVersion,
	    layers,
	    rformat->value,
	    extent->value,
	    crs->value
	    );
  }
  else{
    sprintf(webService_url,
	    "%s?map=%s/%s_%s.map&request=%s&service=%s&version=%s&%s&width=%s&height=%s&format=%s&bbox=%s&crs=%s",
	    msUrl->value,
	    dataPath->value,
	    tmpI->name,
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
  }
  if(hasCRS==0){
    freeMap(&crs);
    free(crs);
  }
  addToMap(tmpI->content,"Reference",webService_url);
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
	    addToMap(output->content,"crs_isGeographic","true");
	  else
	    addToMap(output->content,"crs_isGeographic","false");
	  addToMap(output->content,"crs",tmpSrs);
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
	      addToMap(output->content,"crs_isGeographic","true");
	    else
	      addToMap(output->content,"crs_isGeographic","false");
	  }
	  free(proj4Str);
	}
	else{
	  msLoadProjectionStringEPSG(&m->projection,"EPSG:4326");
	  msLoadProjectionStringEPSG(&myLayer->projection,"EPSG:4326");
	  if(output!=NULL){
	    addToMap(output->content,"crs_isGeographic","true");
	  }
	}
	if(output!=NULL){
	  addToMap(output->content,"crs","EPSG:4326");
	  addToMap(output->content,"real_extent","true");
	}
	msInsertHashTable(&(m->web.metadata),"ows_srs", "EPSG:4326 EPSG:900913 EPSG:3857");
	msInsertHashTable(&(myLayer->metadata),"ows_srs","EPSG:4326 EPSG:900913 EPSG:3857");
      }
    }
  }
  else{
    if(output!=NULL){
      msSrs=getMap(output->content,"msSrs");
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
      addToMap(output->content,"crs",msSrs->value);
      addToMap(output->content,"crs_isGeographic","true");
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
  msMapSetExtent(m,minX,minY,maxX,maxY);
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
    map* test=getMap(output->content,"real_extent");
    if(test!=NULL){
      pointObj min, max;
      projectionObj tempSrs;
      min.x = m->extent.minx;
      min.y = m->extent.miny;
      max.x = m->extent.maxx;
      max.y = m->extent.maxy;
      char tmpSrsStr[1024];
      msInitProjection(&tempSrs);
      msLoadProjectionStringEPSG(&tempSrs,"EPSG:4326");

      msProjectPoint(&(m->projection),&tempSrs,&min);
      msProjectPoint(&m->projection,&tempSrs,&max);
      
      sprintf(tmpExtent,"%.3f,%.3f,%.3f,%.3f",min.y,min.x,max.y,max.x);
      map* isGeo=getMap(output->content,"crs_isGeographic");
#ifdef DEBUGMS
      fprintf(stderr,"isGeo = %s\n",isGeo->value);
#endif
      if(isGeo!=NULL && strcasecmp("true",isGeo->value)==0)
        sprintf(tmpExtent,"%f,%f,%f,%f", minY,minX, maxY, maxX);
      addToMap(output->content,"wms_extent",tmpExtent);
      sprintf(tmpSrsStr,"%.3f,%.3f,%.3f,%.3f",min.x,min.y,max.x,max.y);
      addToMap(output->content,"wcs_extent",tmpExtent);
    }else{
      sprintf(tmpExtent,"%f,%f,%f,%f",minX, minY, maxX, maxY);
      map* isGeo=getMap(output->content,"crs_isGeographic");
      if(isGeo!=NULL){
#ifdef DEBUGMS
	fprintf(stderr,"isGeo = %s\n",isGeo->value);
#endif
	if(isGeo!=NULL && strcasecmp("true",isGeo->value)==0)
	  sprintf(tmpExtent,"%f,%f,%f,%f", minY,minX, maxY, maxX);
      }
      addToMap(output->content,"wms_extent",tmpExtent); 
      sprintf(tmpExtent,"%.3f,%.3f,%.3f,%.3f",minX,minY,maxX,maxY);
      addToMap(output->content,"wcs_extent",tmpExtent);
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

  map* tmpMap=getMap(output->content,"storage");
  char *pszDataSource=tmpMap->value;

  /**
   * Try to open the DataSource using OGR
   */
  OGRRegisterAll();
  /**
   * Try to load the file as ZIP
   */

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
    OGR_DS_Destroy(poDS1);
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
  }
  free(sdsName);
  free(dsName);

  OGRDataSourceH poDS = NULL;
  OGRSFDriverH *poDriver = NULL;
  poDS = OGROpen( pszDataSource, FALSE, poDriver );
  if( poDS == NULL ){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to access the DataSource %s\n",pszDataSource);
#endif
    setMapInMaps(conf,"lenv","message","Unable to open datasource in read only mode");
    OGR_DS_Destroy(poDS);
    OGRCleanupAll();
#ifdef DEBUGMS
    fprintf(stderr,"Unable to access the DataSource, exit! \n"); 
#endif
    return -1;
  }

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
    if(msGrowMapLayers(m)==NULL){
      return -1;
    }
    if(initLayer((m->layers[m->numlayers]), m) == -1){
      return -1;
    }

    layerObj* myLayer=m->layers[m->numlayers];
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
      addToMap(output->content,"crs","EPSG:4326");
      addToMap(output->content,"crs_isGeographic","true");
      msLoadProjectionStringEPSG(&m->projection,"EPSG:4326");
      msInsertHashTable(&(m->web.metadata), "ows_srs", "EPSG:4326 EPSG:900913 EPSG:3857");
      msInsertHashTable(&(myLayer->metadata), "ows_srs", "EPSG:4326 EPSG:900913 EPSG:3857");
    }

    OGREnvelope oExt;
    if (OGR_L_GetExtent(poLayer,&oExt, TRUE) == OGRERR_NONE){
      setMsExtent(output,m,myLayer,oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
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
    map* tmpMap=getMap(output->content,"title");
    if(tmpMap!=NULL)
      msInsertHashTable(&(myLayer->metadata), "ows_title", tmpMap->value);
    else
      msInsertHashTable(&(myLayer->metadata), "ows_title", "Default Title");
    
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

  OGR_DS_Destroy(poDS);
  OGRCleanupAll();

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

  /*
   * Detect the raster style
   */

  /* msRasterResample (NEAREST/AVERAGE/BILINEAR) */
  const char * msRasterResamplingPropertyName       = "msRasterResample";
  /* msRasterStyle (linearStretching/classify) */
  const char * msRasterStylePropertyName            = "msRasterStyle";
  const char * msRasterStyleLinearStretchingPropertyValue       = "linearStretching";
  const char * msRasterStyleColorPalettePropertyValue           = "classify";
  const char * msRasterStyleOptionsPropertyName     = "msRasterStyleOptions";
  /* options for linear stretching */
  const char * msRasterStyleLinearStretchingMinMaxPropertyName  = "minMax";
  const char * msRasterStyleLinearStretchingMeanStdPropertyName = "meanStd";

  const unsigned int msRasterStyleClassifyAutoMaximumNumberOfClasses = 256;

  // Default raster style
  int defaultStyleType = LINEAR_STRETCHING;
  int defaultLinearstretchingType = MEANSTD;
  int defaultClassifyType = AUTO;

  // Check if there is a defined raster style type
  int styleType = defaultStyleType;
  int linearStretchingType = defaultLinearstretchingType;
  int classifyType = defaultClassifyType;
  map* msRasterStyle=getMap(output->content, msRasterStylePropertyName);
  char * msRasterStyleOptionsContent = "";
  char * msRasterStyleFileContent = "";
  if(msRasterStyle!=NULL)
    {
#ifdef DEBUGMS
    fprintf(stderr,"msRasterStyle=%s\n", msRasterStyle->value);
#endif
    // Check if there is options attached
    map* msRasterStyleOptions=getMap(output->content, msRasterStyleOptionsPropertyName);
    if (msRasterStyleOptions!=NULL)
      {
      msRasterStyleOptionsContent = msRasterStyleOptions->value;
#ifdef DEBUGMS
      fprintf(stderr,"msRasterStyleOptions=%s\n", msRasterStyleOptionsContent);
#endif

      }

    if (strncasecmp(msRasterStyle->value, msRasterStyleLinearStretchingPropertyValue,
        strlen(msRasterStyleLinearStretchingPropertyValue))==0)
      {
#ifdef DEBUGMS
      fprintf(stderr,"The raster style is linear stretching\n");
#endif
      styleType = LINEAR_STRETCHING;

      if (strlen(msRasterStyleOptionsContent)>0)
        {
        if (strncasecmp(msRasterStyleOptionsContent, msRasterStyleLinearStretchingMinMaxPropertyName,
            strlen(msRasterStyleLinearStretchingMinMaxPropertyName))==0)
          {
          linearStretchingType = MINMAX;
#ifdef DEBUGMS
          fprintf(stderr,"The raster style linear stretching option is minmax\n");
#endif
          }
        else if (strncasecmp(msRasterStyleOptionsContent, msRasterStyleLinearStretchingMeanStdPropertyName,
            strlen(msRasterStyleLinearStretchingMeanStdPropertyName))==0)
          {
          linearStretchingType = MEANSTD;
#ifdef DEBUGMS
          fprintf(stderr,"The raster style linear stretching option is meanstd\n");
#endif
          }
        else
          {
          fprintf(stderr,"Unknown raster style linear stretching method: %s\n", msRasterStyleOptionsContent);
          }
        } // raster style options (for linear stretching) are not empty
      else
        {
        fprintf(stderr,"Raster style options for linear stretching are empty. Using default.\n");
        }
      } // raster style is linear stretching

    else if (strncasecmp(msRasterStyle->value, msRasterStyleColorPalettePropertyValue,
        strlen(msRasterStyleColorPalettePropertyValue))==0)
      {
#ifdef DEBUGMS
      fprintf(stderr,"The raster style is classify\n");
#endif
      styleType = CLASSIFY;
      if (strlen(msRasterStyleOptionsContent)==0)
        {
        classifyType = AUTO;
#ifdef DEBUGMS
        fprintf(stderr,"The raster style classes is set to auto\n");
#endif
        } // raster style is classify, with automatic styling
      else
        {
        classifyType = USER;
#ifdef DEBUGMS
        fprintf(stderr,"The raster style classes is user defined: %s\n", msRasterStyleOptionsContent);
#endif
        }
      } // raster style is classes

    else
      {
      fprintf(stderr,"Unknown raster style: %s. Using default.", msRasterStyle->value);
      }

    } // raster style is not null

  /*
   * This is just for the backward compatibility and will be deprecated
   */
  {
  map* test=getMap(output->content,"msClassify");
  if(test!=NULL && strncasecmp(test->value,"true",4)==0)
    {
    styleType = CLASSIFY;
    classifyType = AUTO;
    }
  }

#ifdef DEBUGMS
  fprintf(stderr,"Styling options:\n\tRasterStyle=%i\n\tLinearStretching options=%i\n\tClassify options=%i\n",
      styleType,linearStretchingType, classifyType);
#endif

  /*
   * Get storage
   */
  map* tmpMap=getMap(output->content,"storage");
  char *pszFilename=tmpMap->value;
  GDALDatasetH hDataset;
  GDALRasterBandH hBand;
  double adfGeoTransform[6];
  int i, iBand;

  /**
   * Try to open the DataSource using GDAL
   */
  GDALAllRegister();
  hDataset = GDALOpen( pszFilename, GA_ReadOnly );
  if( hDataset == NULL ){
#ifdef DEBUGMS
    fprintf(stderr,"Unable to access the DataSource %s \n",pszFilename);
#endif
    setMapInMaps(conf,"lenv","message","gdalinfo failed - unable to open");
    GDALDestroyDriverManager();
    return -1;
  }
#ifdef DEBUGMS
  fprintf(stderr,"Accessing the DataSource %s %d\n",pszFilename,__LINE__);
#endif

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
  tmpMap=getMap(output->content,"title");
  if(tmpMap!=NULL)
    title=tmpMap->value;
  char *abstract=output->name;
  tmpMap=getMap(output->content,"abstract");
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
    fprintf(stderr,"NO SRS FOUND ! %s\n",GDALGetProjectionRef( hDataset ));    
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

    }
  }

  /**
   * Extract information about available bands to set the bandcount and the
   * processing directive
   */
  char nBands[3];
  int nBandsI=GDALGetRasterCount( hDataset );
  sprintf(nBands,"%d",GDALGetRasterCount( hDataset ));
  msInsertHashTable(&(myLayer->metadata), "ows_bandcount", nBands);
  if(nBandsI>=3)
    msLayerAddProcessing(myLayer,"BANDS=1,2,3");
  else if(nBandsI>=2)
    msLayerAddProcessing(myLayer,"BANDS=1,2");
  else
    msLayerAddProcessing(myLayer,"BANDS=1");

  /**
   * Name available Bands
   */
  char lBands[7];
  char *nameBands=NULL;
  for( iBand = 0; iBand < nBandsI; iBand++ ){
    sprintf(lBands,"Band%d",iBand+1);
    if(nameBands==NULL){
      nameBands=(char*)malloc((strlen(lBands)+1)*sizeof(char));
      sprintf(nameBands,"%s",lBands);
    }else{
      if(iBand<4){
        char *tmpS=zStrdup(nameBands);
        nameBands=(char*)realloc(nameBands,(strlen(nameBands)+strlen(lBands)+1)*sizeof(char));
        sprintf(nameBands,"%s %s",tmpS,lBands);
        free(tmpS);
      }
    }
  }
  msInsertHashTable(&(myLayer->metadata), "ows_bandnames", nameBands);

  /**
   * Loops over metadata information to setup specific information
   */
  for( iBand = 0; iBand < nBandsI; iBand++ )
    {

    // Compute statistics of the current band
    hBand = GDALGetRasterBand( hDataset, iBand+1 );
    CPLErrorReset();
    double min = 0.0;
    double max = 0.0;
    double mean = 0.0;
    double std = 0.0;
    GDALComputeRasterStatistics  (hBand, 1, &min, &max, &mean, &std, NULL, NULL);
#ifdef DEBUGMS
      fprintf(stderr,"Computed raster stats for band %i: min=%.3f max=%.3f mean=%.3f std=%.3f\n",
          iBand, min, max, mean, std);
#endif
    char bandIdentifier[21];
    sprintf(bandIdentifier,"Band%d",iBand+1);
    if (CPLGetLastErrorType() == CE_None)
      {
      char bandInterval[100];
      sprintf(bandInterval,"%.3f %.3f",min,max);
      char bandIntervalIdentifier[21];
      sprintf(bandIntervalIdentifier,"%s_interval",bandIdentifier);
      msInsertHashTable(&(myLayer->metadata), bandIntervalIdentifier, bandInterval);

      // Apply the raster style
      if(styleType == LINEAR_STRETCHING)
        {

        char msProcessingDirective[1024];
        double low = 0.0;
        double hi = 1.0;

        char s1[MAX_NUMBER_STRING_SIZE];
        char s2[MAX_NUMBER_STRING_SIZE];

        int bn = iBand+1;
        if(linearStretchingType==MINMAX)
          {
          low = min;
          hi = max;
          }
        else if (linearStretchingType==MEANSTD)
          {
          low = mean - 2*std;
          hi = mean + 2*std;
          }
#ifdef DEBUGMS
      fprintf(stderr,"Processing the raster using a stretch btw %.3f and %.3f\n", low, hi);
#endif
        sprintf(msProcessingDirective, "SCALE_%d=%s,%s", bn, dtoa(s1,low), dtoa(s2,hi));
        msLayerAddProcessing(myLayer,msProcessingDirective);

        } // styleType is LINEAR_STRETCHING
      else if( styleType == CLASSIFY )
        {
        if(iBand==0)
          {
          if (classifyType == USER)
            {
#ifdef DEBUGMS
            fprintf(stderr,"Processing the raster using the style given in %s\n",msRasterStyleOptionsContent);
#endif

            // Read the mapfile
            FILE *externalMapfile;
            externalMapfile = fopen(msRasterStyleOptionsContent, "rb");
            if (externalMapfile != NULL)
              {
              long lSize;
              char *buffer;

              fseek( externalMapfile , 0L , SEEK_END);
              lSize = ftell( externalMapfile );
              rewind( externalMapfile );

              /* allocate memory for entire content */
              buffer = calloc( 1, lSize+1 );
              if( !buffer )
                {
                fprintf(stderr,"Unable to allocate buffer for file %s. Switching to default classes style.\n",msRasterStyleOptionsContent);
                classifyType = defaultClassifyType;
                }
              else
                {

                /* copy the file into the buffer */
                if( 1!=fread( buffer , lSize, 1 , externalMapfile) )
                  {
                  fclose(externalMapfile);
                  free(buffer);
                  fprintf(stderr,"Unable to read entire file %s. Switching to default classes style.\n",msRasterStyleOptionsContent);
                  classifyType = defaultClassifyType;
                  }
                else
                  {

                  /* do your work here, buffer is a string contains the whole text */

                  fclose(externalMapfile);
                  msUpdateLayerFromString(myLayer, buffer, 0);
                  free(buffer);
                  } // can read entire file
                } // can allocate buffer
              } // file exist
            else
              {
              fprintf(stderr,"Unable to read file %s. Switching to default classes style.\n",msRasterStyleOptionsContent);
              classifyType = defaultClassifyType;
              } // file doesn't exist

            } // classify type is USER

          if (classifyType == AUTO)
            {
            // The number of classes is min(delta, maxNbOfClasses)
            double delta = max - min;
            double step = 1.0;
            double lowBound = 1.0 * min;
            unsigned int numberOfClasses = msRasterStyleClassifyAutoMaximumNumberOfClasses;
            if (delta < msRasterStyleClassifyAutoMaximumNumberOfClasses)
              {
              numberOfClasses = (unsigned int) delta + 1;
              }
            else
              {
              step = delta / (1.0 * msRasterStyleClassifyAutoMaximumNumberOfClasses);
              }
#ifdef DEBUGMS
            fprintf(stderr,"Processing the raster using %d classes with values from %.3f with a step of %.3f\n",numberOfClasses, lowBound, step);
#endif

            for(i=0; i<numberOfClasses; i++)
              {
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
              if(i+1<numberOfClasses)
                sprintf(expression,"([pixel]>=%.3f AND [pixel]<%.3f)",lowBound,lowBound+step);
              else
                sprintf(expression,"([pixel]>=%.3f AND [pixel]<=%.3f)",lowBound,lowBound+step);
              msLoadExpressionString(&myLayer->CLASS[myLayer->numclasses]->expression,expression);
              lowBound += step;

              /**
               * Set color
               */
              double g = i / (0.5*numberOfClasses) ; // must be in [-1,1]
              myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.red=(int)(255*base(g-0.5));
              myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.green=(int)(255*base(g));
              myLayer->CLASS[myLayer->numclasses]->styles[myLayer->CLASS[myLayer->numclasses]->numstyles]->color.blue=(int)(255*base(g+0.5));
              myLayer->CLASS[myLayer->numclasses]->numstyles++;
              myLayer->numclasses++;

              } // next class

            } // classify type is AUTO

          } // styleType is CLASSIFY
        } //iBand is 0

      } // If no error with GDAL functions
    else
      {
      fprintf(stderr,"Unable to compute raster statistics!\n");
      }

    /*
     * Set offsite
     */
    int offsiteR = 0;
    int offsiteG = 0;
    int offsiteB = 0;
    int hasNoData = 0;
    double noDataValue = GDALGetRasterNoDataValue(hBand, &hasNoData);
    if (hasNoData)
      {
#ifdef DEBUGMS
      fprintf(stderr,"No data detected (%.3f)\n", noDataValue);
#endif
      offsiteR = (int) noDataValue;
      offsiteG = (int) noDataValue;
      offsiteB = (int) noDataValue;
      }
    myLayer->offsite.red    = offsiteR;
    myLayer->offsite.green  = offsiteG;
    myLayer->offsite.blue   = offsiteB;
#ifdef DEBUGMS
    fprintf(stderr,"Setting OFFSITE to (%d,%d,%d)\n",offsiteR, offsiteG, offsiteB);
#endif

    /*
     * Insert units
     */
    if( strlen(GDALGetRasterUnitType(hBand)) > 0 )
      {
      char tmpU[21];
      sprintf(tmpU,"%s_band_uom",bandIdentifier);
      msInsertHashTable(&(myLayer->metadata), tmpU, GDALGetRasterUnitType(hBand));
      }

    } // next band

  /*
   * Check if there is resample option
   */
  char msResampleOptionDirective[1024];
  char * msRasterResampleOptionContent = "BILINEAR";
  map* msRasterResamplingOption=getMap(output->content, msRasterResamplingPropertyName);
  if (msRasterResamplingOption!=NULL)
    {
    msRasterResampleOptionContent = msRasterResamplingOption->value;
    }
  sprintf(msResampleOptionDirective, "RESAMPLE=%s",msRasterResampleOptionContent);
  msLayerAddProcessing(myLayer, msResampleOptionDirective);

  m->layerorder[m->numlayers] = m->numlayers;
  m->numlayers++;
  GDALClose( hDataset );
  GDALDestroyDriverManager();
  CPLCleanupTLS();
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
  map* mime=getMap(outputs->content,"mimeType");
  char *ext="data";
  if(mime!=NULL)
    if(strncasecmp(mime->value,"application/json",16)==0)
      ext="json";

  map* tmpMap=getMapFromMaps(conf,"main","dataPath");
  map* sidMap=getMapFromMaps(conf,"lenv","usid");
  char *pszDataSource=(char*)malloc((strlen(tmpMap->value)+strlen(sidMap->value)+strlen(outputs->name)+17)*sizeof(char));
  sprintf(pszDataSource,"%s/ZOO_DATA_%s_%s.%s",tmpMap->value,outputs->name,sidMap->value,ext); 
  int f=zOpen(pszDataSource,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  map *gfile=getMap(outputs->content,"generated_file");
  if(gfile!=NULL){
    readGeneratedFile(conf,outputs->content,gfile->value);	    
  }
  map* sizeMap=getMap(outputs->content,"size");
  map* vData=getMap(outputs->content,"value");
  if(sizeMap!=NULL){
    zWrite(f,vData->value,atoi(sizeMap->value)*sizeof(char));
  }
  else{
    zWrite(f,vData->value,(strlen(vData->value)+1)*sizeof(char));
  }
  close(f);
  addToMap(outputs->content,"storage",pszDataSource);
  free(pszDataSource);

  /*
   * Create an empty map, set name, default size and extent
   */
  mapObj *myMap=msNewMapObj();
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
  o2->inmapfile=MS_TRUE;  
  msAppendOutputFormat(myMap,msCloneOutputFormat(o2));
  msFreeOutputFormat(o2);
#endif

  outputFormatObj *o3=msCreateDefaultOutputFormat(NULL,"GDAL/GTiff","tiff");
  if(!o3)
    fprintf(stderr,"Unable to initialize GDAL driver !\n");
  else{
    o3->imagemode=MS_IMAGEMODE_BYTE;
    o3->inmapfile=MS_TRUE;  
    msAppendOutputFormat(myMap,msCloneOutputFormat(o3));
    msFreeOutputFormat(o3);
  }

  outputFormatObj *o4=msCreateDefaultOutputFormat(NULL,"GDAL/AAIGRID","grd");
  if(!o4)
    fprintf(stderr,"Unable to initialize GDAL driver !\n");
  else{
    o4->imagemode=MS_IMAGEMODE_INT16;
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

  /*
   * Set default projection to EPSG:4326
   */
  msLoadProjectionStringEPSG(&myMap->projection,"EPSG:4326");
  myMap->transparent=1;

  /**
   * Set metadata extracted from main.cfg file maps
   */
  maps* cursor=conf;
  map* correspondance=getCorrespondance();
  while(cursor!=NULL){
    map* _cursor=cursor->content;
    map* vMap;
    while(_cursor!=NULL){
      if((vMap=getMap(correspondance,_cursor->name))!=NULL){
	if (msInsertHashTable(&(myMap->web.metadata), vMap->value, _cursor->value) == NULL){
#ifdef DEBUGMS
	  fprintf(stderr,"Unable to add metadata");
#endif
	  return;
	}
      }
      _cursor=_cursor->next;
    }
    cursor=cursor->next;
  }
  freeMap(&correspondance);
  free(correspondance);

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

  if(tryOgr(conf,outputs,myMap)<0)
    if(tryGdal(conf,outputs,myMap)<0)
      return ;

  tmp1=getMapFromMaps(conf,"main","dataPath");
  char *tmpPath=(char*)malloc((13+strlen(tmp1->value))*sizeof(char));
  sprintf(tmpPath,"%s/symbols.sym",tmp1->value);
  msInitSymbolSet(&myMap->symbolset);
  myMap->symbolset.filename=zStrdup(tmpPath);
  free(tmpPath);

  map* sid=getMapFromMaps(conf,"lenv","usid");
  char *mapPath=
    (char*)malloc((7+strlen(sid->value)+strlen(outputs->name)+strlen(tmp1->value))*sizeof(char));
  sprintf(mapPath,"%s/%s_%s.map",tmp1->value,outputs->name,sid->value);
  msSaveMap(myMap,mapPath);
  free(mapPath);
  msGDALCleanup();
  msFreeMap(myMap);
}

