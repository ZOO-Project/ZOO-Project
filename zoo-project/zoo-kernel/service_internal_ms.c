#ifdef USE_MS

#include "service_internal_ms.h"

/**
 * Map composed by a main.cfg maps name as key and the corresponding 
 * MapServer Mafile Metadata name to use
 * see doc from here :
 *  - http://mapserver.org/ogc/wms_server.html
 *  - http://mapserver.org/ogc/wfs_server.html
 *  - http://mapserver.org/ogc/wcs_server.html
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

void setReferenceUrl(maps* m,maps* tmpI){
  dumpMaps(tmpI);
  outputMapfile(m,tmpI);
  map *msUrl=getMapFromMaps(m,"main","mapserverAddress");
  map *msOgcVersion=getMapFromMaps(m,"main","msOgcVersion");
  map *dataPath=getMapFromMaps(m,"main","dataPath");
  map *sid=getMapFromMaps(m,"lenv","sid");
  map* format=getMap(tmpI->content,"mimeType");
  map* rformat=getMap(tmpI->content,"requestedMimeType");
  map* width=getMap(tmpI->content,"width");
  map* height=getMap(tmpI->content,"height");
  map* protoMap=getMap(tmpI->content,"msOgc");
  map* versionMap=getMap(tmpI->content,"msOgcVersion");
  char options[3][5][25]={
    {"WMS","1.3.0","GetMap","layers=%s","wms_extent"},
    {"WFS","1.1.0","GetFeature","typename=%s","wms_extent"},
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
  if(protoMap!=NULL)
    if(strncasecmp(protoMap->value,"WMS",3)==0)
      proto=0;
    else if(strncasecmp(protoMap->value,"WFS",3)==0)
      proto=1;
    else 
      proto=2;
  
  char *protoVersion=options[proto][1];
  if(proto==1){
    if(msOgcVersion!=NULL)
      protoVersion=msOgcVersion->value;
    if(versionMap!=NULL)
      protoVersion=versionMap->value;
  }

  map* extent=getMap(tmpI->content,options[proto][4]);
  map* crs=getMap(tmpI->content,"crs");
  char layers[128];
  sprintf(layers,options[proto][3],tmpI->name);

  char* webService_url=(char*)malloc((strlen(msUrl->value)+strlen(format->value)+strlen(tmpI->name)+strlen(width->value)+strlen(height->value)+strlen(extent->value)+256)*sizeof(char));

  if(proto>0)
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
  else
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
  addToMap(tmpI->content,"Reference",webService_url);

}

/**
 * Set projection using Authority Code and Name if available or fallback to 
 * proj4 definition if available or fallback to default EPSG:4326
 */
void setSrsInformations(maps* output,mapObj* m,layerObj* myLayer,
			char* pszProjection){
  OGRSpatialReferenceH  hSRS;
  map* msSrs=NULL;
  hSRS = OSRNewSpatialReference(NULL);
  if( pszProjection!=NULL && strlen(pszProjection)>1 &&
      OSRImportFromWkt( hSRS, &pszProjection ) == CE_None ){
    char *proj4Str=NULL;
    if(OSRGetAuthorityName(hSRS,NULL)!=NULL && OSRGetAuthorityCode(hSRS,NULL)!=NULL){
      char tmpSrs[20];
      sprintf(tmpSrs,"%s:%s",
	      OSRGetAuthorityName(hSRS,NULL),OSRGetAuthorityCode(hSRS,NULL));
      msLoadProjectionStringEPSG(&m->projection,tmpSrs);
      msLoadProjectionStringEPSG(&myLayer->projection,tmpSrs);
      
      char tmpSrss[256];
      fprintf(tmpSrss,"EPSG:4326 EPSG:900913 %s",tmpSrs);

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
      if(proj4Str!=NULL){
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
      }
      msInsertHashTable(&(m->web.metadata),"ows_srs", "EPSG:4326 EPSG:900913");
      msInsertHashTable(&(myLayer->metadata),"ows_srs","EPSG:4326 EPSG:900913");
    }
  }
  else{
    if(output!=NULL){
      msSrs=getMap(output->content,"msSrs");
    }
    if(msSrs!=NULL){
      if(output!=NULL){
	addToMap(output->content,"crs",msSrs->value);
	addToMap(output->content,"crs_isGeographic","true");
      }
      msLoadProjectionStringEPSG(&m->projection,msSrs->value);
      msLoadProjectionStringEPSG(&myLayer->projection,msSrs->value);
      char tmpSrs[128];
      sprintf(tmpSrs,"%s EPSG:4326 EPSG:900913",msSrs);
      msInsertHashTable(&(m->web.metadata),"ows_srs",tmpSrs);
      msInsertHashTable(&(myLayer->metadata),"ows_srs",tmpSrs);
    }else{
      if(output!=NULL){
	addToMap(output->content,"crs","EPSG:4326");
	addToMap(output->content,"crs_isGeographic","true");
      }
      msLoadProjectionStringEPSG(&m->projection,"EPSG:4326");
      msLoadProjectionStringEPSG(&myLayer->projection,"EPSG:4326");
      msInsertHashTable(&(m->web.metadata),"ows_srs","EPSG:4326 EPSG:900913");
      msInsertHashTable(&(myLayer->metadata),"ows_srs","EPSG:4326 EPSG:900913");
    }
  }

  OSRDestroySpatialReference( hSRS );
}

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
    sprintf(tmpExtent,"%f,%f,%f,%f",minX, minY, maxX, maxY);
    map* isGeo=getMap(output->content,"crs_isGeographic");
    fprintf(stderr,"isGeo = %s\n",isGeo->value);
    if(isGeo!=NULL && strcasecmp("true",isGeo->value)==0)
      sprintf(tmpExtent,"%f,%f,%f,%f", minY,minX, maxY, maxX);
    addToMap(output->content,"wms_extent",tmpExtent); 
    sprintf(tmpExtent,"%.3f,%.3f,%.3f,%.3f",minX,minY,maxX,maxY);
    addToMap(output->content,"wcs_extent",tmpExtent);
  }

  setMapSize(output,minX,minY,maxX,maxY);
}

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

  OGRDataSourceH *poDS1 = NULL;
  OGRSFDriverH *poDriver1 = NULL;
  char *dsName=(char*)malloc((8+strlen(pszDataSource)+1)*sizeof(char));
  char *odsName=strdup(pszDataSource);
  char *sdsName=strdup(pszDataSource);
  char *demo=strstr(odsName,".");
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
  fseek(file, 0, SEEK_END);
  unsigned long fileLen=ftell(file);
  fseek(file, 0, SEEK_SET);
  char *buffer=(char *)malloc(fileLen+1);
  fread(buffer, fileLen, 1, file);
  fwrite(buffer,fileLen, 1, fileZ);
  fclose(file);
  fclose(fileZ);
  free(buffer);
  fprintf(stderr,"Try loading %s",dsName);
  poDS1 = OGROpen( dsName, FALSE, poDriver1 );
  if( poDS1 == NULL ){
    fprintf(stderr,"Unable to access the DataSource as ZIP File\n");
    setMapInMaps(conf,"lenv","message","Unable to open datasource in read only mode");
    OGR_DS_Destroy(poDS1);
  }else{
    fprintf(stderr,"The DataSource is a  ZIP File\n");
    char** demo=VSIReadDir(dsName);
    int i=0;
    mkdir(sdsName,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH );
    while(demo[i]!=NULL){
      fprintf(stderr,"ZIP File content : %s\n",demo[i]);
      char *tmpDs=(char*)malloc((strlen(dsName)+strlen(demo[i])+2)*sizeof(char));
      sprintf(tmpDs,"%s/%s",dsName,demo[i]);
      fprintf(stderr,"read : %s\n",tmpDs);
      
      VSILFILE* vsif=VSIFOpenL(tmpDs,"rb");
      fprintf(stderr,"open : %s\n",tmpDs);
      VSIFSeekL(vsif,0,SEEK_END);
      int size=VSIFTellL(vsif);
      fprintf(stderr,"size : %d\n",size);
      VSIFSeekL(vsif,0,SEEK_SET);
      char *vsifcontent=(char*) malloc((size+1)*sizeof(char));
      VSIFReadL(vsifcontent,1,size,vsif);
      char *fpath=(char*) malloc((strlen(sdsName)+strlen(demo[1])+2)*sizeof(char));
      sprintf(fpath,"%s/%s",sdsName,demo[i]);
      int f=open(fpath,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
      write(f,vsifcontent,size);
      close(f);
      chmod(fpath,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
      char* tmpP=strstr(fpath,".shp");
      if(tmpP==NULL)
	tmpP=strstr(fpath,".SHP");
      if(tmpP!=NULL){
	fprintf(stderr,"*** DEBUG %s\n",strstr(tmpP,"."));
	if( strcmp(tmpP,".shp")==0 || strcmp(tmpP,".SHP")==0 ){
	  tmpMap=getMap(output->content,"storage");
	  free(tmpMap->value);
	  tmpMap->value=(char*) malloc((strlen(fpath)+1)*sizeof(char));
	  sprintf(tmpMap->value,"%s",fpath);
	  pszDataSource=tmpMap->value;
	  fprintf(stderr,"*** DEBUG %s\n",pszDataSource);
	}
      }
      VSIFCloseL(vsif);
      i++;
    }

  }

  OGRDataSourceH *poDS = NULL;
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
    OGRLayerH *poLayer = OGR_DS_GetLayer(poDS,iLayer);

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
    dumpMaps(output);
    myLayer->name = strdup(output->name);
    myLayer->tileitem=NULL;
    myLayer->data = strdup(OGR_L_GetName(poLayer));
    myLayer->connection = strdup(pszDataSource);
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
    }
    else{
      addToMap(output->content,"crs","EPSG:4326");
      addToMap(output->content,"crs_isGeographic","true");
      msLoadProjectionStringEPSG(&m->projection,"EPSG:4326");
      msInsertHashTable(&(m->web.metadata), "ows_srs", "EPSG:4326 EPSG:900913");
      msInsertHashTable(&(myLayer->metadata), "ows_srs", "EPSG:4326 EPSG:900913");
    }

    map* crs=getMap(output->content,"crs");
    map* isGeo=getMap(output->content,"crs_isGeographic");

    OGREnvelope oExt;
    if (OGR_L_GetExtent(poLayer,&oExt, TRUE) == OGRERR_NONE){
      setMsExtent(output,m,myLayer,oExt.MinX, oExt.MinY, oExt.MaxX, oExt.MaxY);
    }
  
    /**
     * Detect the FID column or use the first attribute field as FID
     */
    char *fid=OGR_L_GetFIDColumn(poLayer);
    if(strlen(fid)==0){
      OGRFeatureDefnH def=OGR_L_GetLayerDefn(poLayer);
      int fIndex=0;
      for(fIndex=0;fIndex<OGR_FD_GetFieldCount(def);fIndex++){
	OGRFieldDefnH fdef=OGR_FD_GetFieldDefn(def,fIndex);
	fid=OGR_Fld_GetNameRef(fdef);
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
      return;
    if(initClass((myLayer->class[myLayer->numclasses])) == -1)
      return;
    myLayer->class[myLayer->numclasses]->type = myLayer->type;
    if(msGrowClassStyles(myLayer->class[myLayer->numclasses]) == NULL)
      return ;
    if(initStyle(myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]) == -1)
      return;

    /**
     * Apply msStyle else fallback to the default style
     */
    tmpMap=getMap(output->content,"msStyle");
    if(tmpMap!=NULL)
      msUpdateStyleFromString(myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles],tmpMap->value,0);
    else{
      /**
       * Set style
       */
      myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->color.red=125;
      myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->color.green=125;
      myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->color.blue=255;
      myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->outlinecolor.red=80;
      myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->outlinecolor.green=80;
      myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->outlinecolor.blue=80;

      /**
       * Set specific style depending on type
       */
      if(myLayer->type == MS_LAYER_POLYGON)
	myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->width=3;
      if(myLayer->type == MS_LAYER_LINE){
	myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->width=3;
	myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->outlinewidth=1.5;
      }
      if(myLayer->type == MS_LAYER_POINT){
	myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->symbol=1;
	myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->size=15;
      }

    }
    myLayer->class[myLayer->numclasses]->numstyles++;
    myLayer->numclasses++;
    m->layerorder[m->numlayers] = m->numlayers;
    m->numlayers++;

  }

  OGR_DS_Destroy(poDS);
  OGRCleanupAll();

  return 1;
}


int tryGdal(maps* conf,maps* output,mapObj* m){

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
    fprintf(stderr,"Unable to access the DataSource \n");
#endif
    setMapInMaps(conf,"lenv","message","gdalinfo failed - unable to open");
    GDALDestroyDriverManager();
    return -1;
  }
#ifdef DEBUGMS
  fprintf(stderr,"Accessing the DataSource %s\n",__LINE__);
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

  layerObj* myLayer=m->layers[m->numlayers];
  myLayer->name = strdup(output->name);
  myLayer->tileitem=NULL;
  myLayer->data = strdup(pszFilename);
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
  if( GDALGetProjectionRef( hDataset ) != NULL ){
    OGRSpatialReferenceH  hSRS;
    char *pszProjection;
    pszProjection = (char *) GDALGetProjectionRef( hDataset );
#ifdef DEBUGMS
    fprintf(stderr,"Accessing the DataSource %s\n",GDALGetProjectionRef( hDataset ));
#endif
    setSrsInformations(output,m,myLayer,pszProjection);
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
  char nBands[2];
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
  char lBands[6];
  char *nameBands=NULL;
  for( iBand = 0; iBand < nBandsI; iBand++ ){
    sprintf(lBands,"Band%d",iBand+1);
    if(nameBands==NULL){
      nameBands=(char*)malloc((strlen(lBands)+1)*sizeof(char));
      sprintf(nameBands,"%s",lBands);
    }else{
      if(iBand<4){
	char *tmpS=strdup(nameBands);
	nameBands=(char*)realloc(nameBands,(strlen(nameBands)+strlen(lBands)+1)*sizeof(char));
	sprintf(nameBands,"%s %s",tmpS,lBands);
	free(tmpS);
      }
    }
  }
  msInsertHashTable(&(myLayer->metadata), "ows_bandnames", nameBands);
  
  /**
   * Loops over metadata informations to setup specific informations
   */
  for( iBand = 0; iBand < nBandsI; iBand++ ){
    int         bGotNodata, bSuccess;
    double      adfCMinMax[2], dfNoData;
    int         nBlockXSize, nBlockYSize, nMaskFlags;
    double      dfMean, dfStdDev;
    hBand = GDALGetRasterBand( hDataset, iBand+1 );

    CPLErrorReset();
    GDALComputeRasterMinMax( hBand, FALSE, adfCMinMax );
    char tmpN[21];
    sprintf(tmpN,"Band%d",iBand+1);
    if (CPLGetLastErrorType() == CE_None){
      char tmpMm[100];
      sprintf(tmpMm,"%.3f %.3f",adfCMinMax[0],adfCMinMax[1]);
      char tmpI[21];
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
	  double delta=adfCMinMax[1]-adfCMinMax[0];
	  double interval=delta/10;
	  double cstep=adfCMinMax[0];
	  for(i=0;i<10;i++){
	    /**
	     * Create a new class
	     */
	    if(msGrowLayerClasses(myLayer) == NULL)
	      return;
	    if(initClass((myLayer->class[myLayer->numclasses])) == -1)
	      return;
	    myLayer->class[myLayer->numclasses]->type = myLayer->type;
	    if(msGrowClassStyles(myLayer->class[myLayer->numclasses]) == NULL)
	      return ;
	    if(initStyle(myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]) == -1)
	      return;
	    
	    /**
	     * Set class name
	     */
	    char className[7];
	    sprintf(className,"class%d",i);
	    myLayer->class[myLayer->numclasses]->name=strdup(className);
	    
	    /**
	     * Set expression
	     */
	    char expression[1024];
	    if(i+1<10)
	      sprintf(expression,"([pixel]>=%.3f AND [pixel]<%.3f)",cstep,cstep+interval);
	    else
	      sprintf(expression,"([pixel]>=%.3f AND [pixel]<=%.3f)",cstep,cstep+interval);
	    msLoadExpressionString(&myLayer->class[myLayer->numclasses]->expression,expression);
	    
	    /**
	     * Set color
	     */
	    myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->color.red=_tmpColors[i][0];
	    myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->color.green=_tmpColors[i][1];
	    myLayer->class[myLayer->numclasses]->styles[myLayer->class[myLayer->numclasses]->numstyles]->color.blue=_tmpColors[i][2];
	    cstep+=interval;
	    myLayer->class[myLayer->numclasses]->numstyles++;
	    myLayer->numclasses++;
	    
	  }
	  
	  char tmpMm[100];
	  sprintf(tmpMm,"%.3f %.3f",adfCMinMax[0],adfCMinMax[1]);
	  
	}
      }
    }
    if( strlen(GDALGetRasterUnitType(hBand)) > 0 ){
      char tmpU[21];
      sprintf(tmpU,"%s_band_uom",tmpN);
      msInsertHashTable(&(myLayer->metadata), tmpU, GDALGetRasterUnitType(hBand));
    }

  }

  m->layerorder[m->numlayers] = m->numlayers;
  m->numlayers++;
  GDALClose( hDataset );
  GDALDestroyDriverManager();
  CPLCleanupTLS();
  return 1;
}

/**
 * Create a MapFile for WMS, WFS or WCS Service output
 */
void outputMapfile(maps* conf,maps* outputs){

  /**
   * Firs store the value on disk
   */
  map* tmpMap=getMapFromMaps(conf,"main","dataPath");
  map* sidMap=getMapFromMaps(conf,"lenv","sid");
  char *pszDataSource=(char*)malloc((strlen(tmpMap->value)+strlen(sidMap->value)+strlen(outputs->name)+17)*sizeof(char));
  sprintf(pszDataSource,"%s/ZOO_DATA_%s_%s.data",tmpMap->value,outputs->name,sidMap->value);
  int f=open(pszDataSource,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
  map* sizeMap=getMap(outputs->content,"size");
  map* vData=getMap(outputs->content,"value");
  if(sizeMap!=NULL){
    write(f,vData->value,atoi(sizeMap->value)*sizeof(char));
  }
  else{
    write(f,vData->value,strlen(vData->value)*sizeof(char));
  }
  close(f);
  //exit(-1);
  addToMap(outputs->content,"storage",pszDataSource);

  /*
   * Create an empty map, set name, default size and extent
   */
  mapObj *myMap=msNewMapObj();
  free(myMap->name);
  myMap->name=strdup("ZOO-Project_WXS_Server");
  msMapSetSize(myMap,2048,2048);
  msMapSetExtent(myMap,-1,-1,1,1);
  
  /*
   * Set imagepath and imageurl using tmpPath and tmpUrl from main.cfg
   */
  map *tmp1=getMapFromMaps(conf,"main","tmpPath");
  myMap->web.imagepath=strdup(tmp1->value);
  tmp1=getMapFromMaps(conf,"main","tmpUrl");
  myMap->web.imageurl=strdup(tmp1->value);
  
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
      return NULL;

  tmp1=getMapFromMaps(conf,"main","dataPath");
  char *tmpPath=(char*)malloc((13+strlen(tmp1->value))*sizeof(char));
  sprintf(tmpPath,"%s/symbols.sym",tmp1->value);
  msInitSymbolSet(&myMap->symbolset);
  myMap->symbolset.filename=strdup(tmpPath);
  free(tmpPath);

  map* sid=getMapFromMaps(conf,"lenv","sid");
  char *mapPath=
    (char*)malloc((16+strlen(outputs->name)+strlen(tmp1->value))*sizeof(char));
  sprintf(mapPath,"%s/%s_%s.map",tmp1->value,outputs->name,sid->value);
  msSaveMap(myMap,mapPath);
  msFreeMap(myMap);
}

#endif
