/**
 * Author : Gérald FENOY
 *
 * Copyright 2008-2009 GeoLabs SARL. All rights reserved.
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

#include "cpl_conv.h"
#include "ogr_api.h"
#include "ogr_geometry.h"
#if GDAL_VERSION_MAJOR >= 2
#include <gdal_priv.h>
#endif

#include "cpl_minixml.h"
#include "ogr_api.h"
#include "ogrsf_frmts.h"

#include "geos_c.h"
#include "service.h"
#include "service_internal.h"

extern "C" {
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

/*#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
*/

  void printExceptionReportResponse(maps*,map*);
  char *base64(const char *input, int length);

  OGRGeometryH createGeometryFromGML(maps* conf,char* inputStr){
    xmlInitParser();
    xmlDocPtr doc = xmlParseMemory(inputStr,strlen(inputStr));
    xmlChar *xmlbuff;
    int buffersize;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    const char * xpathExpr="/*/*/*/*/*[local-name()='Polygon' or local-name()='MultiPolygon']";
    xpathCtx = xmlXPathNewContext(doc);
    xpathObj = xmlXPathEvalExpression(BAD_CAST xpathExpr,xpathCtx);
    if(!xpathObj->nodesetval){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse Input Polygon"));
      setMapInMaps(conf,"lenv","code","InvalidParameterValue");
      return NULL;
    }
    int size = (xpathObj->nodesetval) ? xpathObj->nodesetval->nodeNr : 0;
    /**
     * Create a temporary XML document
     */
    xmlDocPtr ndoc = xmlNewDoc(BAD_CAST "1.0");
    /**
     * Only one polygon should be provided so we use it as the root node.
     */
    for(int k=size-1;k>=0;k--){ 
      xmlDocSetRootElement(ndoc, xpathObj->nodesetval->nodeTab[k]);
    }
    xmlDocDumpFormatMemory(ndoc, &xmlbuff, &buffersize, 1);
    char *tmp=(char*)calloc((xmlStrlen(xmlStrstr(xmlbuff,BAD_CAST "?>"))-1),sizeof(char));
    sprintf(tmp,"%s",xmlStrstr(xmlbuff,BAD_CAST "?>")+2);
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFree(xmlbuff);
    xmlFreeDoc(doc);
    xmlFreeDoc(ndoc);
#ifndef WIN32
    xmlCleanupParser();
#endif
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n Loading the geometry from GML string ...");
#endif
    OGRGeometryH res=OGR_G_CreateFromGML(tmp);
    free(tmp);
    if(res==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to call OGR_G_CreatFromGML"));
      return NULL;
    }
    else
      return res;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Simplify(maps*& conf,maps*& inputs,maps*& outputs){
    OGRRegisterAll();

    double tolerance;
    map* tmp0=getMapFromMaps(inputs,"Tolerance","value");
    if(tmp0==NULL){
      tolerance=atof("2.0");
    }
    else
      tolerance=atof(tmp0->value);

    maps* cursor=inputs;
    OGRGeometryH geometry;
    OGRGeometry *res;
    OGRLayer *poDstLayer;
    const char *oDriver1;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset *poODS;
#else
    OGRDataSource *poODS;
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    char filename[1024];
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
    const char *oDriver;
    oDriver="GeoJSON";
    sprintf(filename,"/vsimem/input_%d.json",getpid());
    if(tmp1!=NULL){
      if(strcmp(tmp1->value,"text/xml")==0){
	sprintf(filename,"/vsimem/input_%d.xml",getpid());
	oDriver="GML";
      }
    }
    VSILFILE *ifile=VSIFileFromMemBuffer(filename,(GByte*)tmp->value,strlen(tmp->value),FALSE);
    VSIFCloseL(ifile);

#if GDAL_VERSION_MAJOR >= 2
      GDALDataset *ipoDS =
	(GDALDataset*) GDALOpenEx( filename,
				   GDAL_OF_READONLY | GDAL_OF_VECTOR,
				   NULL, NULL, NULL );
      GDALDriverManager* poR=GetGDALDriverManager();
      GDALDriver          *poDriver = NULL;
#else
      OGRDataSource* ipoDS = 
	OGRSFDriverRegistrar::Open(filename,FALSE);
      OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
      OGRSFDriver          *poDriver = NULL;
#endif
    char pszDestDataSource[100];
    if( ipoDS == NULL )
      {
	fprintf( stderr, "FAILURE:\n"
		 "Unable to open datasource `%s' with the following drivers.\n",
		 filename );
	
	for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >= 2
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
	  }
	char tmp[1024];
	sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
      }
    for( int iLayer = 0; iLayer < ipoDS->GetLayerCount();
	 iLayer++ )
      {
	OGRLayer        *poLayer = ipoDS->GetLayer(iLayer);
	
	if( poLayer == NULL )
	  {
	    fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		     iLayer );
	    char tmp[1024];
	    sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
	  }
	
	OGRFeature  *poFeature;

	int                  iDriver;
	
	map* tmpMap=getMapFromMaps(outputs,"Result","mimeType");
	oDriver1="GeoJSON";
	sprintf(pszDestDataSource,"/vsimem/result_%d.json",getpid());
	if(tmpMap!=NULL){
	  if(strcmp(tmpMap->value,"text/xml")==0){
	    sprintf(pszDestDataSource,"/vsimem/result_%d.xml",getpid());
	    oDriver1="GML";
	  }
	}
	
	for( iDriver = 0;
	     iDriver < poR->GetDriverCount() && poDriver == NULL;
	     iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >=2
	    if( EQUAL(poR->GetDriver(iDriver)->GetDescription(),oDriver1) )
#else
	    if( EQUAL(poR->GetDriver(iDriver)->GetName(),oDriver1) )
#endif
	      {
		poDriver = poR->GetDriver(iDriver);
	      }
	  }
	
	if( poDriver == NULL )
	  {
	    char emessage[8192];
	    sprintf( emessage, "Unable to find driver `%s'.\n", oDriver );
	    sprintf( emessage,  "%sThe following drivers are available:\n",emessage );
	    
	    for( iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	      {
#if GDAL_VERSION_MAJOR >=2
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetDescription() );
#else
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetName() );
#endif
	      }
	    
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	    
	  }
	
#if GDAL_VERSION_MAJOR >=2
	if( !CPLTestBool( CSLFetchNameValueDef(poDriver->GetMetadata(), GDAL_DCAP_CREATE, "FALSE") ) )
#else
	if( !poDriver->TestCapability( ODrCCreateDataSource ) )
#endif
	  {
	    char emessage[1024];
	    sprintf( emessage,  "%s driver does not support data source creation.\n",
	      "json" );
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	  }
	
	char **papszDSCO=NULL;
#if GDAL_VERSION_MAJOR >=2
        poODS = poDriver->Create( pszDestDataSource, 0, 0, 0, GDT_Unknown, papszDSCO );
#else
	poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
#endif
	if( poODS == NULL ){
	  char emessage[1024];      
	  sprintf( emessage,  "%s driver failed to create %s\n", 
		   "json", pszDestDataSource );
	  setMapInMaps(conf,"lenv","message",emessage);
	  return SERVICE_FAILED;
	}
	
	if( !poODS->TestCapability( ODsCCreateLayer ) )
	  {
	    char emessage[1024];
	    sprintf( emessage, 
		     "Layer %s not found, and CreateLayer not supported by driver.", 
		     "Result" );
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	  }
	
	poDstLayer = poODS->CreateLayer( "Result", NULL,wkbUnknown,NULL);
	if( poDstLayer == NULL ){
	  setMapInMaps(conf,"lenv","message","Layer creation failed.\n");
	  return SERVICE_FAILED;
	}
	
	OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
	int iField;
	int hasMmField=0;
	
	for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
	  {
	    OGRFieldDefn *tmp=poFDefn->GetFieldDefn(iField);
            if (iField >= 0)
                poDstLayer->CreateField( poFDefn->GetFieldDefn(iField) );
            else
            {
                fprintf( stderr, "Field '%s' not found in source layer.\n", 
                        iField );
		return SERVICE_FAILED;
            }
	  }

	while(TRUE){
	  OGRFeature      *poDstFeature = NULL;
	  poFeature = poLayer->GetNextFeature();
	  if( poFeature == NULL )
	    break;
	  if(poFeature->GetGeometryRef() != NULL){
	    poDstFeature = OGRFeature::CreateFeature( poDstLayer->GetLayerDefn() );
	    if( poDstFeature->SetFrom( poFeature, TRUE ) != OGRERR_NONE )
	      {
		char tmpMsg[1024];
		sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			 poFeature->GetFID(), poFDefn->GetName() );
		
		OGRFeature::DestroyFeature( poFeature );
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    geometry=poFeature->GetGeometryRef();
#if GDAL_VERSION_MAJOR == 1 && GDAL_VERSION_MINOR < 11
	    GEOSGeometry* ggeometry=((OGRGeometry *) geometry)->exportToGEOS();
	    GEOSGeometry* gres=GEOSTopologyPreserveSimplify(ggeometry,tolerance);
	    if(gres!=NULL)
	      res=(OGRGeometry*)OGRGeometryFactory::createFromGEOS(gres);
#else
	    res=((OGRGeometry *) geometry)->SimplifyPreserveTopology(tolerance);
#endif
	    if(poDstFeature->SetGeometryDirectly(res) != OGRERR_NONE )
	      {
		char tmpMsg[1024];
		sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			 poFeature->GetFID(), poFDefn->GetName() );
		
		OGRFeature::DestroyFeature( poFeature );
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRFeature::DestroyFeature( poFeature );
	    if( poDstLayer->CreateFeature( poDstFeature ) != OGRERR_NONE )
	      {		
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRFeature::DestroyFeature( poDstFeature );
#if GDAL_VERSION_MAJOR == 1 && GDAL_VERSION_MINOR < 11
	    GEOSGeom_destroy( ggeometry);
	    GEOSGeom_destroy( gres);
#endif
	  }
	}
      }

    delete poODS;
    delete ipoDS;

    char *res1=readVSIFile(conf,pszDestDataSource);
    if(res1==NULL)
      return SERVICE_FAILED;
    setMapInMaps(outputs,"Result","value",res1);
    free(res1);

    OGRCleanupAll();
    dumpMaps(outputs);
    return SERVICE_SUCCEEDED;

}

int applyOne(maps*& conf,maps*& inputs,maps*& outputs,OGRGeometry* (OGRGeometry::*myFunc)() const,const char* schema){
    OGRRegisterAll();

    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    OGRLayer *poDstLayer;
    const char *oDriver1;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset *poODS;
#else
    OGRDataSource *poODS;
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    char filename[1024];
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
    const char *oDriver;
    oDriver="GeoJSON";
    sprintf(filename,"/vsimem/input_%d.json",getpid());
    if(tmp1!=NULL){
      if(strcmp(tmp1->value,"text/xml")==0){
	sprintf(filename,"/vsimem/input_%d.xml",getpid());
	oDriver="GML";
      }
    }
    VSILFILE *ifile=VSIFileFromMemBuffer(filename,(GByte*)tmp->value,strlen(tmp->value),FALSE);
    VSIFCloseL(ifile);
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset *ipoDS =
      (GDALDataset*) GDALOpenEx( filename,
				 GDAL_OF_READONLY | GDAL_OF_VECTOR,
				 NULL, NULL, NULL );
    GDALDriverManager* poR=GetGDALDriverManager();
    GDALDriver          *poDriver = NULL;
#else
    OGRDataSource* ipoDS = 
      OGRSFDriverRegistrar::Open(filename,FALSE);
    OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
    OGRSFDriver          *poDriver = NULL;
#endif
    char pszDestDataSource[100];
    if( ipoDS == NULL )
      {
	fprintf( stderr, "FAILURE:\n"
		 "Unable to open datasource `%s' with the following drivers.\n",
		 filename );
	char emessage[1024];	
	for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >=2
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetDescription() );
		fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetName() );
		fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
	  }
	char tmp[1024];
	sprintf(tmp,"Unable to open datasource `%s' with the following drivers.\n%s",filename,emessage);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
      }

    for( int iLayer = 0; iLayer < ipoDS->GetLayerCount();
	 iLayer++ )
      {
	OGRLayer        *poLayer = ipoDS->GetLayer(iLayer);
	
	if( poLayer == NULL )
	  {
	    fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		     iLayer );
	    char tmp[1024];
	    sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
	  }
	
	OGRFeature  *poFeature;

	/* -------------------------------------------------------------------- */
	/*      Try opening the output datasource as an existing, writable      */
	/* -------------------------------------------------------------------- */
	
	int                  iDriver;
	
	map* tmpMap=getMapFromMaps(outputs,"Result","mimeType");
	oDriver1="GeoJSON";
	sprintf(pszDestDataSource,"/vsimem/result_%d.json",getpid());
	if(tmpMap!=NULL){
	  if(strcmp(tmpMap->value,"text/xml")==0){
	    sprintf(pszDestDataSource,"/vsimem/result_%d.xml",getpid());
	    oDriver1="GML";
	  }
	}
	
	for( iDriver = 0;
	     iDriver < poR->GetDriverCount() && poDriver == NULL;
	     iDriver++ )
	  {

	    if( 
#if GDAL_VERSION_MAJOR >=2
		EQUAL(poR->GetDriver(iDriver)->GetDescription(),oDriver1)
#else
		EQUAL(poR->GetDriver(iDriver)->GetName(),oDriver1)
#endif
		)
	      {
		poDriver = poR->GetDriver(iDriver);
	      }
	  }
	
	if( poDriver == NULL )
	  {
	    char emessage[8192];
	    sprintf( emessage, "Unable to find driver `%s'.\n", oDriver );
	    sprintf( emessage,  "%sThe following drivers are available:\n",emessage );
	    for( iDriver = 0;iDriver < poR->GetDriverCount();iDriver++ )	    
	      {
#if GDAL_VERSION_MAJOR >=2
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetDescription() );
#else
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetName() );
#endif
	      }
	    
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	    
	  }
	
#if GDAL_VERSION_MAJOR >=2
	if( !CPLTestBool( CSLFetchNameValueDef(poDriver->GetMetadata(), GDAL_DCAP_CREATE, "FALSE") ) )
#else
	if( !poDriver->TestCapability( ODrCCreateDataSource ) )
#endif
	{
	  char emessage[1024];
	  sprintf( emessage,  "%s driver does not support data source creation.\n",
		   "json" );
	  setMapInMaps(conf,"lenv","message",emessage);
	  return SERVICE_FAILED;
	}
	
	/* -------------------------------------------------------------------- */
	/*      Create the output data source.                                  */
	/* -------------------------------------------------------------------- */
	//map* tpath=getMapFromMaps(conf,"main","tmpPath");
	char **papszDSCO=NULL;
#if GDAL_VERSION_MAJOR >=2
        poODS = poDriver->Create( pszDestDataSource, 0, 0, 0, GDT_Unknown, papszDSCO );
#else
	poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
#endif
	if( poODS == NULL ){
	  char emessage[1024];      
	  sprintf( emessage,  "%s driver failed to create %s\n", 
		   "json", pszDestDataSource );
	  setMapInMaps(conf,"lenv","message",emessage);
	  return SERVICE_FAILED;
	}
	
	/* -------------------------------------------------------------------- */
	/*      Create the layer.                                               */
	/* -------------------------------------------------------------------- */
	if( !poODS->TestCapability( ODsCCreateLayer ) )
	  {
	    char emessage[1024];
	    sprintf( emessage, 
		     "Layer %s not found, and CreateLayer not supported by driver.", 
		     "Result" );
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	  }
	
	
	poDstLayer = poODS->CreateLayer( "Result", NULL,wkbUnknown,NULL);
	if( poDstLayer == NULL ){
	  setMapInMaps(conf,"lenv","message","Layer creation failed.\n");
	  return SERVICE_FAILED;
	}
	
	OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
	int iField;
	int hasMmField=0;
	
	for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
	  {
	    OGRFieldDefn *tmp=poFDefn->GetFieldDefn(iField);
            if (iField >= 0)
                poDstLayer->CreateField( poFDefn->GetFieldDefn(iField) );
            else
            {
                fprintf( stderr, "Field '%s' not found in source layer.\n", 
                        iField );
		return SERVICE_FAILED;
            }
	  }

	while(TRUE){
	  OGRFeature      *poDstFeature = NULL;
	  poFeature = poLayer->GetNextFeature();
	  if( poFeature == NULL )
	    break;
	  if(poFeature->GetGeometryRef() != NULL){
	    poDstFeature = OGRFeature::CreateFeature( poDstLayer->GetLayerDefn() );
	    if( poDstFeature->SetFrom( poFeature, TRUE ) != OGRERR_NONE )
	      {
		char tmpMsg[1024];
		sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			 poFeature->GetFID(), poFDefn->GetName() );
		
		OGRFeature::DestroyFeature( poFeature );
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    if(poDstFeature->SetGeometryDirectly((poDstFeature->GetGeometryRef()->*myFunc)()) != OGRERR_NONE )
	      {
		char tmpMsg[1024];
		sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			 poFeature->GetFID(), poFDefn->GetName() );
		
		OGRFeature::DestroyFeature( poFeature );
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRFeature::DestroyFeature( poFeature );
	    if( poDstLayer->CreateFeature( poDstFeature ) != OGRERR_NONE )
	      {		
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRFeature::DestroyFeature( poDstFeature );
	  }
	}

      }

    delete poODS;
    delete ipoDS;

    char *res1=readVSIFile(conf,pszDestDataSource);
    if(res1==NULL)
      return SERVICE_FAILED;
    setMapInMaps(outputs,"Result","value",res1);
    free(res1);

    OGRCleanupAll();
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
int Buffer(maps*& conf,maps*& inputs,maps*& outputs){
    OGRRegisterAll();

    double bufferDistance;
    map* tmp0=getMapFromMaps(inputs,"BufferDistance","value");
    if(tmp0==NULL){
      bufferDistance=atof("10.0");
    }
    else
      bufferDistance=atof(tmp0->value);

    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    OGRLayer *poDstLayer;
    const char *oDriver1;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset *poODS;
#else
    OGRDataSource *poODS;
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    char filename[1024];
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
    const char *oDriver;
    oDriver="GeoJSON";
    sprintf(filename,"/vsimem/input_%d.json",getpid());
    if(tmp1!=NULL){
      if(strcmp(tmp1->value,"text/xml")==0){
	sprintf(filename,"/vsimem/input_%d.xml",getpid());
	oDriver="GML";
      }
    }
    VSILFILE *ifile=VSIFileFromMemBuffer(filename,(GByte*)tmp->value,strlen(tmp->value),FALSE);
    VSIFCloseL(ifile);
#if GDAL_VERSION_MAJOR >= 2
      GDALDataset *ipoDS =
	(GDALDataset*) GDALOpenEx( filename,
				   GDAL_OF_READONLY | GDAL_OF_VECTOR,
				   NULL, NULL, NULL );
      GDALDriverManager* poR=GetGDALDriverManager();
      GDALDriver          *poDriver = NULL;
#else
      OGRDataSource* ipoDS = 
	OGRSFDriverRegistrar::Open(filename,FALSE);
      OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
      OGRSFDriver          *poDriver = NULL;
#endif
    char pszDestDataSource[100];
    if( ipoDS == NULL )
      {
	
	fprintf( stderr, "FAILURE:\n"
		 "Unable to open datasource `%s' with the following drivers.\n",
		 filename );
	
	for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >= 2
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
	  }
	char tmp[1024];
	sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
      }
    for( int iLayer = 0; iLayer < ipoDS->GetLayerCount();
	 iLayer++ )
      {
	OGRLayer        *poLayer = ipoDS->GetLayer(iLayer);
	
	if( poLayer == NULL )
	  {
	    fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		     iLayer );
	    char tmp[1024];
	    sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
	  }
	
	OGRFeature  *poFeature;

	/* -------------------------------------------------------------------- */
	/*      Try opening the output datasource as an existing, writable      */
	/* -------------------------------------------------------------------- */
	
	int                  iDriver;
	
	map* tmpMap=getMapFromMaps(outputs,"Result","mimeType");
	oDriver1="GeoJSON";
	sprintf(pszDestDataSource,"/vsimem/result_%d.json",getpid());
	if(tmpMap!=NULL){
	  if(strcmp(tmpMap->value,"text/xml")==0){
	    sprintf(pszDestDataSource,"/vsimem/result_%d.xml",getpid());
	    oDriver1="GML";
	  }
	}
	
	for( iDriver = 0;
	     iDriver < poR->GetDriverCount() && poDriver == NULL;
	     iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >=2
	    if( EQUAL(poR->GetDriver(iDriver)->GetDescription(),oDriver1) )
#else
	    if( EQUAL(poR->GetDriver(iDriver)->GetName(),oDriver1) )
#endif
	      {
		poDriver = poR->GetDriver(iDriver);
	      }
	  }
	
	if( poDriver == NULL )
	  {
	    char emessage[8192];
	    sprintf( emessage, "Unable to find driver `%s'.\n", oDriver );
	    sprintf( emessage,  "%sThe following drivers are available:\n",emessage );
	    
	    for( iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	      {
#if GDAL_VERSION_MAJOR >=2
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetDescription() );
#else
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetName() );
#endif
	      }
	    
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	    
	  }
	
#if GDAL_VERSION_MAJOR >=2
	if( !CPLTestBool( CSLFetchNameValueDef(poDriver->GetMetadata(), GDAL_DCAP_CREATE, "FALSE") ) )
#else
	if( !poDriver->TestCapability( ODrCCreateDataSource ) )
#endif
	{
	  char emessage[1024];
	  sprintf( emessage,  "%s driver does not support data source creation.\n",
		   "json" );
	  setMapInMaps(conf,"lenv","message",emessage);
	  return SERVICE_FAILED;
	}
	
	/* -------------------------------------------------------------------- */
	/*      Create the output data source.                                  */
	/* -------------------------------------------------------------------- */
	//map* tpath=getMapFromMaps(conf,"main","tmpPath");
	char **papszDSCO=NULL;
#if GDAL_VERSION_MAJOR >=2
        poODS = poDriver->Create( pszDestDataSource, 0, 0, 0, GDT_Unknown, papszDSCO );
#else
	poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
#endif
	if( poODS == NULL ){
	  char emessage[1024];      
	  sprintf( emessage,  "%s driver failed to create %s\n", 
		   "json", pszDestDataSource );
	  setMapInMaps(conf,"lenv","message",emessage);
	  return SERVICE_FAILED;
	}
	
	/* -------------------------------------------------------------------- */
	/*      Create the layer.                                               */
	/* -------------------------------------------------------------------- */
	if( !poODS->TestCapability( ODsCCreateLayer ) )
	  {
	    char emessage[1024];
	    sprintf( emessage, 
		     "Layer %s not found, and CreateLayer not supported by driver.", 
		     "Result" );
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	  }

	poDstLayer = poODS->CreateLayer( "Result", NULL,wkbUnknown,NULL);
	if( poDstLayer == NULL ){
	  setMapInMaps(conf,"lenv","message","Layer creation failed.\n");
	  return SERVICE_FAILED;
	}
	
	OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
	int iField;
	int hasMmField=0;
	
	for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
	  {
	    OGRFieldDefn *tmp=poFDefn->GetFieldDefn(iField);
            if (iField >= 0)
                poDstLayer->CreateField( poFDefn->GetFieldDefn(iField) );
            else
            {
                fprintf( stderr, "Field '%s' not found in source layer.\n", 
                        iField );
		return SERVICE_FAILED;
            }
	  }

	while(TRUE){
	  OGRFeature      *poDstFeature = NULL;
	  poFeature = poLayer->GetNextFeature();
	  if( poFeature == NULL )
	    break;
	  if(poFeature->GetGeometryRef() != NULL){
	    poDstFeature = OGRFeature::CreateFeature( poDstLayer->GetLayerDefn() );
	    if( poDstFeature->SetFrom( poFeature, TRUE ) != OGRERR_NONE )
	      {
		char tmpMsg[1024];
		sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			 poFeature->GetFID(), poFDefn->GetName() );
		
		OGRFeature::DestroyFeature( poFeature );
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    if(poDstFeature->SetGeometryDirectly(poDstFeature->GetGeometryRef()->Buffer(bufferDistance,30)) != OGRERR_NONE )
	      {
		char tmpMsg[1024];
		sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			 poFeature->GetFID(), poFDefn->GetName() );
		
		OGRFeature::DestroyFeature( poFeature );
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRFeature::DestroyFeature( poFeature );
	    if( poDstLayer->CreateFeature( poDstFeature ) != OGRERR_NONE )
	      {		
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRFeature::DestroyFeature( poDstFeature );
	  }
	}

      }

    delete poODS;
    delete ipoDS;

    char *res1=readVSIFile(conf,pszDestDataSource);
    if(res1==NULL)
      return SERVICE_FAILED;
    setMapInMaps(outputs,"Result","value",res1);
    free(res1);

    OGRCleanupAll();
    return SERVICE_SUCCEEDED;

}

#ifdef WIN32
  __declspec(dllexport)
#endif
int Centroid(maps*& conf,maps*& inputs,maps*& outputs){
    OGRRegisterAll();

    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    OGRLayer *poDstLayer;
    const char *oDriver1;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset *poODS;
#else
    OGRDataSource *poODS;
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    char filename[1024];
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
    const char *oDriver;
    oDriver="GeoJSON";
    sprintf(filename,"/vsimem/input_%d.json",getpid());
    if(tmp1!=NULL){
      if(strcmp(tmp1->value,"text/xml")==0){
	sprintf(filename,"/vsimem/input_%d.xml",getpid());
	oDriver="GML";
      }
    }
    VSILFILE *ifile=VSIFileFromMemBuffer(filename,(GByte*)tmp->value,strlen(tmp->value),FALSE);
    VSIFCloseL(ifile);
#if GDAL_VERSION_MAJOR >= 2
      GDALDataset *ipoDS =
	(GDALDataset*) GDALOpenEx( filename,
				   GDAL_OF_READONLY | GDAL_OF_VECTOR,
				   NULL, NULL, NULL );
      GDALDriverManager* poR=GetGDALDriverManager();
      GDALDriver          *poDriver = NULL;
#else
      OGRDataSource* ipoDS = 
	OGRSFDriverRegistrar::Open(filename,FALSE);
      OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
      OGRSFDriver          *poDriver = NULL;
#endif
    char pszDestDataSource[100];
    if( ipoDS == NULL )
      {
	
	fprintf( stderr, "FAILURE:\n"
		 "Unable to open datasource `%s' with the following drivers.\n",
		 filename );
	
	for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >= 2
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
	  }
	char tmp[1024];
	sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
      }
    for( int iLayer = 0; iLayer < ipoDS->GetLayerCount();
	 iLayer++ )
      {
	OGRLayer        *poLayer = ipoDS->GetLayer(iLayer);
	
	if( poLayer == NULL )
	  {
	    fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		     iLayer );
	    char tmp[1024];
	    sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
	  }
	
	OGRFeature  *poFeature;

	/* -------------------------------------------------------------------- */
	/*      Try opening the output datasource as an existing, writable      */
	/* -------------------------------------------------------------------- */
	
	int                  iDriver;
	
	map* tmpMap=getMapFromMaps(outputs,"Result","mimeType");
	oDriver1="GeoJSON";
	sprintf(pszDestDataSource,"/vsimem/result_%d.json",getpid());
	if(tmpMap!=NULL){
	  if(strcmp(tmpMap->value,"text/xml")==0){
	    sprintf(pszDestDataSource,"/vsimem/result_%d.xml",getpid());
	    oDriver1="GML";
	  }
	}
	
	for( iDriver = 0;
	     iDriver < poR->GetDriverCount() && poDriver == NULL;
	     iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >=2
	    if( EQUAL(poR->GetDriver(iDriver)->GetDescription(),oDriver1) )
#else
	    if( EQUAL(poR->GetDriver(iDriver)->GetName(),oDriver1) )
#endif
	      {
		poDriver = poR->GetDriver(iDriver);
	      }
	  }
	
	if( poDriver == NULL )
	  {
	    char emessage[8192];
	    sprintf( emessage, "Unable to find driver `%s'.\n", oDriver );
	    sprintf( emessage,  "%sThe following drivers are available:\n",emessage );
	    
	    for( iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	      {
#if GDAL_VERSION_MAJOR >=2
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetDescription() );
#else
		sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetName() );
#endif
	      }
	    
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	    
	  }
	
#if GDAL_VERSION_MAJOR >=2
	if( !CPLTestBool( CSLFetchNameValueDef(poDriver->GetMetadata(), GDAL_DCAP_CREATE, "FALSE") ) )
#else
	if( !poDriver->TestCapability( ODrCCreateDataSource ) )
#endif
	{
	  char emessage[1024];
	  sprintf( emessage,  "%s driver does not support data source creation.\n",
		   "json" );
	  setMapInMaps(conf,"lenv","message",emessage);
	  return SERVICE_FAILED;
	}
	
	/* -------------------------------------------------------------------- */
	/*      Create the output data source.                                  */
	/* -------------------------------------------------------------------- */
	//map* tpath=getMapFromMaps(conf,"main","tmpPath");
	char **papszDSCO=NULL;
#if GDAL_VERSION_MAJOR >=2
        poODS = poDriver->Create( pszDestDataSource, 0, 0, 0, GDT_Unknown, papszDSCO );
#else
	poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
#endif
	if( poODS == NULL ){
	  char emessage[1024];      
	  sprintf( emessage,  "%s driver failed to create %s\n", 
		   "json", pszDestDataSource );
	  setMapInMaps(conf,"lenv","message",emessage);
	  return SERVICE_FAILED;
	}
	
	/* -------------------------------------------------------------------- */
	/*      Create the layer.                                               */
	/* -------------------------------------------------------------------- */
	if( !poODS->TestCapability( ODsCCreateLayer ) )
	  {
	    char emessage[1024];
	    sprintf( emessage, 
		     "Layer %s not found, and CreateLayer not supported by driver.", 
		     "Result" );
	    setMapInMaps(conf,"lenv","message",emessage);
	    return SERVICE_FAILED;
	  }

	poDstLayer = poODS->CreateLayer( "Result", NULL,wkbUnknown,NULL);
	if( poDstLayer == NULL ){
	  setMapInMaps(conf,"lenv","message","Layer creation failed.\n");
	  return SERVICE_FAILED;
	}
	
	OGRFeatureDefn *poFDefn = poLayer->GetLayerDefn();
	int iField;
	int hasMmField=0;
	
	for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
	  {
	    OGRFieldDefn *tmp=poFDefn->GetFieldDefn(iField);
            if (iField >= 0)
                poDstLayer->CreateField( poFDefn->GetFieldDefn(iField) );
            else
            {
                fprintf( stderr, "Field '%s' not found in source layer.\n", 
                        iField );
		return SERVICE_FAILED;
            }
	  }

	while(TRUE){
	  OGRFeature      *poDstFeature = NULL;
	  poFeature = poLayer->GetNextFeature();
	  if( poFeature == NULL )
	    break;
	  if(poFeature->GetGeometryRef() != NULL){
	    poDstFeature = OGRFeature::CreateFeature( poDstLayer->GetLayerDefn() );
	    if( poDstFeature->SetFrom( poFeature, TRUE ) != OGRERR_NONE )
	      {
		char tmpMsg[1024];
		sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			 poFeature->GetFID(), poFDefn->GetName() );
		
		OGRFeature::DestroyFeature( poFeature );
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRPoint* poPoint=new OGRPoint();
	    poDstFeature->GetGeometryRef()->Centroid(poPoint);
	    if(poDstFeature->SetGeometryDirectly(poPoint)!= OGRERR_NONE ){
	      char tmpMsg[1024];
	      sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
		       poFeature->GetFID(), poFDefn->GetName() );
	      
	      OGRFeature::DestroyFeature( poFeature );
	      OGRFeature::DestroyFeature( poDstFeature );
	      return SERVICE_FAILED;
	    }
	    OGRFeature::DestroyFeature( poFeature );
	    if( poDstLayer->CreateFeature( poDstFeature ) != OGRERR_NONE )
	      {		
		OGRFeature::DestroyFeature( poDstFeature );
		return SERVICE_FAILED;
	      }
	    OGRFeature::DestroyFeature( poDstFeature );
	  }
	}

      }

    delete poODS;
    delete ipoDS;

    char *res1=readVSIFile(conf,pszDestDataSource);
    if(res1==NULL)
      return SERVICE_FAILED;
    setMapInMaps(outputs,"Result","value",res1);
    free(res1);

    OGRCleanupAll();
    return SERVICE_SUCCEEDED;

}

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Boundary(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOne(conf,inputs,outputs,&OGRGeometry::Boundary,"http://fooa/gml/3.1.0/polygon.xsd");
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int ConvexHull(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOne(conf,inputs,outputs,&OGRGeometry::ConvexHull,"http://fooa/gml/3.1.0/polygon.xsd");
  }

#if GDAL_VERSION_MAJOR >= 2
GDALDataset*
#else
OGRDataSource* 
#endif
  loadEntity(maps* conf,maps* inputs,char **filename,const char **oDriver,const char *entity,int iter){
    map* tmp=getMapFromMaps(inputs,entity,"value");
    map* tmp1=getMapFromMaps(inputs,entity,"mimeType");
    *oDriver="GeoJSON";
    sprintf(*filename,"/vsimem/input_%d.json",getpid()+iter);
    if(tmp1!=NULL){
      if(strcmp(tmp1->value,"text/xml")==0){
	sprintf(*filename,"/vsimem/input_%d.xml",getpid()+iter);
	*oDriver="GML";
      }
    }
    VSILFILE *ifile=VSIFileFromMemBuffer(*filename,(GByte*)tmp->value,strlen(tmp->value),FALSE);
    VSIFCloseL(ifile);
#if GDAL_VERSION_MAJOR >= 2
    return (GDALDataset*) GDALOpenEx( *filename,
				      GDAL_OF_READONLY | GDAL_OF_VECTOR,
				      NULL, NULL, NULL );
#else
    return OGRSFDriverRegistrar::Open(*filename,FALSE);    
#endif
  }

  int applyOneBool(maps*& conf,maps*& inputs,maps*& outputs,OGRBoolean (OGRGeometry::*myFunc)() const){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n");
#endif
    OGRRegisterAll();

    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    OGRLayer *poDstLayer;
    const char *oDriver1;
    OGRDataSource       *poODS;
#ifdef DEBUG
    dumpMaps(cursor);
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    char filename[1024];
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
    const char *oDriver;
    oDriver="GeoJSON";
    sprintf(filename,"/vsimem/input_%d.json",getpid());
    if(tmp1!=NULL){
      if(strcmp(tmp1->value,"text/xml")==0){
	sprintf(filename,"/vsimem/input_%d.xml",getpid());
	oDriver="GML";
      }
    }
    VSILFILE *ifile=VSIFileFromMemBuffer(filename,(GByte*)tmp->value,strlen(tmp->value),FALSE);
    VSIFCloseL(ifile);

#if GDAL_VERSION_MAJOR >= 2
      GDALDataset *ipoDS =
	(GDALDataset*) GDALOpenEx( filename,
				   GDAL_OF_READONLY | GDAL_OF_VECTOR,
				   NULL, NULL, NULL );
      GDALDriverManager* poR=GetGDALDriverManager();
      GDALDriver          *poDriver = NULL;
#else
      OGRDataSource* ipoDS = 
	OGRSFDriverRegistrar::Open(filename,FALSE);
      OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
      OGRSFDriver          *poDriver = NULL;
#endif
    char pszDestDataSource[100];
    if( ipoDS == NULL )
      {	
	fprintf( stderr, "FAILURE:\n"
		 "Unable to open datasource `%s' with the following drivers.\n",
		 filename );
	
	for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >= 2
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
	  }
	char tmp[1024];
	sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
      }
    for( int iLayer = 0; iLayer < ipoDS->GetLayerCount();
	 iLayer++ )
      {
	OGRLayer        *poLayer = ipoDS->GetLayer(iLayer);
	
	if( poLayer == NULL )
	  {
	    fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		     iLayer );
	    char tmp[1024];
	    sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
	  }
	
	OGRFeature  *poFeature;


	while(TRUE){
	  OGRFeature      *poDstFeature = NULL;
	  poFeature = poLayer->GetNextFeature();
	  if( poFeature == NULL )
	    break;
	  if(poFeature->GetGeometryRef() != NULL){
	    if((poFeature->GetGeometryRef()->*myFunc)()==0){
	      setMapInMaps(outputs,"Result","value","false");
	      OGRFeature::DestroyFeature( poFeature );
	      delete ipoDS;
	      return SERVICE_SUCCEEDED;
	    }
	  }
	  OGRFeature::DestroyFeature( poFeature );
	}

      }

    delete ipoDS;
    setMapInMaps(outputs,"Result","value","true");

    OGRCleanupAll();
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int IsSimple(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOneBool(conf,inputs,outputs,&OGRGeometry::IsSimple);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int IsClosed(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOneBool(conf,inputs,outputs,&OGRGeometry::IsRing);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int IsValid(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOneBool(conf,inputs,outputs,&OGRGeometry::IsValid);
  }

  
  int applyTwo(maps*& conf,maps*& inputs,maps*& outputs,OGRGeometry* (OGRGeometry::*myFunc)(const OGRGeometry*) const){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n");
#endif
    OGRRegisterAll();

    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    OGRLayer *poDstLayer;
    //const char *oDriver1;
#ifdef DEBUG
    dumpMaps(cursor);
#endif

    char *filename=(char*)malloc(1024*sizeof(char));
    const char *oDriver1;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset*
#else
    OGRDataSource* 
#endif
      ipoDS1 = loadEntity(conf,inputs,&filename,&oDriver1,"InputEntity1",1);
    char *filename1=(char*)malloc(1024*sizeof(char));
    const char *oDriver2;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset*
#else
    OGRDataSource* 
#endif
      ipoDS2 = loadEntity(conf,inputs,&filename1,&oDriver2,"InputEntity2",2);
    const char *oDriver3;
    char pszDestDataSource[100];
#if GDAL_VERSION_MAJOR >= 2
      GDALDriverManager* poR=GetGDALDriverManager();
      GDALDriver          *poDriver = NULL;
      GDALDataset *poODS;
#else
      OGRDataSource       *poODS;
      OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
      OGRSFDriver          *poDriver = NULL;
#endif
    if( ipoDS1 == NULL || ipoDS2 == NULL )
      {
	
	fprintf( stderr, "FAILURE:\n"
		 "Unable to open datasource `%s' with the following drivers.\n",
		 filename );
	
	for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >= 2
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
	  }
	char tmp[1024];
	if( ipoDS1 == NULL )
	  sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename);
	if( ipoDS2 == NULL )
	  sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename1);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
      }
    for( int iLayer = 0; iLayer < ipoDS1->GetLayerCount();
	 iLayer++ )
      {
	OGRLayer        *poLayer1 = ipoDS1->GetLayer(iLayer);
	
	if( poLayer1 == NULL )
	  {
	    fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		     iLayer );
	    char tmp[1024];
	    sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
	  }

	for( int iLayer1 = 0; iLayer1 < ipoDS2->GetLayerCount();
	     iLayer1++ )
	  {
	    OGRLayer        *poLayer2 = ipoDS2->GetLayer(iLayer1);
	    
	    if( poLayer1 == NULL )
	      {
		fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
			 iLayer1 );
		char tmp[1024];
		sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer1);
		setMapInMaps(conf,"lenv","message",tmp);
		return SERVICE_FAILED;
	      }
	
	    OGRFeature  *poFeature1,*poFeature2;

	    /* -------------------------------------------------------------------- */
	    /*      Try opening the output datasource as an existing, writable      */
	    /* -------------------------------------------------------------------- */
	    
	    int                  iDriver;
	    
	    map* tmpMap=getMapFromMaps(outputs,"Result","mimeType");
	    oDriver3="GeoJSON";
	    sprintf(pszDestDataSource,"/vsimem/result_%d.json",getpid());
	    if(tmpMap!=NULL){
	      if(strcmp(tmpMap->value,"text/xml")==0){
		sprintf(pszDestDataSource,"/vsimem/result_%d.xml",getpid());
		oDriver3="GML";
	      }
	    }

	    for( iDriver = 0;
		 iDriver < poR->GetDriverCount() && poDriver == NULL;
		 iDriver++ )
	      {
#if GDAL_VERSION_MAJOR >=2
		if( EQUAL(poR->GetDriver(iDriver)->GetDescription(),oDriver3) )
#else
		if( EQUAL(poR->GetDriver(iDriver)->GetName(),oDriver3) )
#endif
		  {
		    poDriver = poR->GetDriver(iDriver);
		  }
	      }
	    
	    if( poDriver == NULL )
	      {
		char emessage[8192];
		sprintf( emessage, "Unable to find driver `%s'.\n", oDriver1 );
		sprintf( emessage,  "%sThe following drivers are available:\n",emessage );
		
		for( iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
		  {
#if GDAL_VERSION_MAJOR >= 2
		    sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetDescription() );
#else
		    sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetName() );
#endif
		  }
		
		setMapInMaps(conf,"lenv","message",emessage);
		return SERVICE_FAILED;
		
	      }
	    
#if GDAL_VERSION_MAJOR >= 2
	    if( !CPLTestBool( CSLFetchNameValueDef(poDriver->GetMetadata(), GDAL_DCAP_CREATE, "FALSE") ) )
#else
	    if( !poDriver->TestCapability( ODrCCreateDataSource ) )
#endif
	      {
		char emessage[1024];
		sprintf( emessage,  "%s driver does not support data source creation.\n",
			 "json" );
		setMapInMaps(conf,"lenv","message",emessage);
		return SERVICE_FAILED;
	      }
	    
	    /* -------------------------------------------------------------------- */
	    /*      Create the output data source.                                  */
	    /* -------------------------------------------------------------------- */
	    //map* tpath=getMapFromMaps(conf,"main","tmpPath");
	    char **papszDSCO=NULL;
#if GDAL_VERSION_MAJOR >= 2
	    poODS = poDriver->Create( pszDestDataSource, 0, 0, 0, GDT_Unknown, papszDSCO );
#else
	    poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
#endif
	    if( poODS == NULL ){
	      char emessage[1024];      
	      sprintf( emessage,  "%s driver failed to create %s\n", 
		       "json", pszDestDataSource );
	      setMapInMaps(conf,"lenv","message",emessage);
	      return SERVICE_FAILED;
	    }
	    
	    /* -------------------------------------------------------------------- */
	    /*      Create the layer.                                               */
	    /* -------------------------------------------------------------------- */
	    if( !poODS->TestCapability( ODsCCreateLayer ) )
	      {
		char emessage[1024];
		sprintf( emessage, 
			 "Layer %s not found, and CreateLayer not supported by driver.", 
			 "Result" );
		setMapInMaps(conf,"lenv","message",emessage);
		return SERVICE_FAILED;
	      }
	    
	    //CPLErrorReset();
	    
	    poDstLayer = poODS->CreateLayer( "Result", NULL,wkbUnknown,NULL);
	    if( poDstLayer == NULL ){
	      setMapInMaps(conf,"lenv","message","Layer creation failed.\n");
	      return SERVICE_FAILED;
	    }
	    
	    OGRFeatureDefn *poFDefn = poLayer2->GetLayerDefn();
	    int iField;
	    int hasMmField=0;
	    
	    for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
	      {
		OGRFieldDefn *tmp=poFDefn->GetFieldDefn(iField);
		if (iField >= 0)
		  poDstLayer->CreateField( poFDefn->GetFieldDefn(iField) );
		else
		  {
		    fprintf( stderr, "Field '%s' not found in source layer.\n", 
			     iField );
		    return SERVICE_FAILED;
		  }
	      }
	    
	    while(TRUE){
	      OGRFeature      *poDstFeature = NULL;
	      poFeature1 = poLayer1->GetNextFeature();
	      if( poFeature1 == NULL )
		break;
	      while(TRUE){
		poFeature2 = poLayer2->GetNextFeature();
		if( poFeature2 == NULL )
		  break;

		if(poFeature1->GetGeometryRef() != NULL && poFeature2->GetGeometryRef() != NULL){
		  poDstFeature = OGRFeature::CreateFeature( poDstLayer->GetLayerDefn() );
		  if( poDstFeature->SetFrom( poFeature2, TRUE ) != OGRERR_NONE )
		    {
		      char tmpMsg[1024];
		      sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			       poFeature2->GetFID(), poFDefn->GetName() );
		      
		      OGRFeature::DestroyFeature( poFeature1 );
		      OGRFeature::DestroyFeature( poFeature2 );
		      OGRFeature::DestroyFeature( poDstFeature );
		      return SERVICE_FAILED;
		    }
		  if(poDstFeature->SetGeometryDirectly((poFeature1->GetGeometryRef()->*myFunc)(poFeature2->GetGeometryRef())) != OGRERR_NONE )
		    {
		      char tmpMsg[1024];
		      sprintf( tmpMsg,"Unable to translate feature %ld from layer %s.\n",
			       poFeature2->GetFID(), poFDefn->GetName() );
		      
		      OGRFeature::DestroyFeature( poFeature1 );
		      OGRFeature::DestroyFeature( poFeature2 );
		      OGRFeature::DestroyFeature( poDstFeature );
		      return SERVICE_FAILED;
		    }
		  OGRFeature::DestroyFeature( poFeature2 );
		  if(!poDstFeature->GetGeometryRef()->IsEmpty())
		    if( poDstLayer->CreateFeature( poDstFeature ) != OGRERR_NONE )
		      {		
			OGRFeature::DestroyFeature( poDstFeature );
			return SERVICE_FAILED;
		      }
		  OGRFeature::DestroyFeature( poDstFeature );
		}
	      }
	    }
	    OGRFeature::DestroyFeature( poFeature1 );
	  }
      }

    delete poODS;
    delete ipoDS1;
    delete ipoDS2;
    free(filename);
    free(filename1);

    char *res1=readVSIFile(conf,pszDestDataSource);
    if(res1==NULL)
      return SERVICE_FAILED;
    setMapInMaps(outputs,"Result","value",res1);
    free(res1);
    OGRCleanupAll();
    return SERVICE_SUCCEEDED;
  }
  
#ifdef WIN32
  __declspec(dllexport)
#endif
  int Difference(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGRGeometry::Difference);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int SymDifference(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGRGeometry::SymDifference);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Intersection(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGRGeometry::Intersection);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Union(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGRGeometry::Union);
  }

  int applyTwoBool(maps*& conf,maps*& inputs,maps*& outputs,OGRBoolean (OGRGeometry::*myFunc)(const OGRGeometry*) const){
    OGRRegisterAll();

    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    OGRLayer *poDstLayer;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset *poODS;
#else
    OGRDataSource *poODS;
#endif

    char *filename=(char*)malloc(1024*sizeof(char));
    const char *oDriver1;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset*
#else
    OGRDataSource*
#endif
      ipoDS1 = loadEntity(conf,inputs,&filename,&oDriver1,"InputEntity1",1);

    char *filename1=(char*)malloc(1024*sizeof(char));
    const char *oDriver2;
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset*
#else
    OGRDataSource*
#endif
      ipoDS2 = loadEntity(conf,inputs,&filename1,&oDriver2,"InputEntity2",2);
    const char *oDriver3;
    char pszDestDataSource[100];
#if GDAL_VERSION_MAJOR >= 2
      GDALDriverManager* poR=GetGDALDriverManager();
      GDALDriver          *poDriver = NULL;
#else
      OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
      OGRSFDriver          *poDriver = NULL;
#endif

    if( ipoDS1 == NULL || ipoDS2 == NULL )
      {
	fprintf( stderr, "FAILURE:\n"
		 "Unable to open datasource `%s' with the following drivers.\n",
		 filename );
	
	for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
	  {
#if GDAL_VERSION_MAJOR >= 2
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
	  }
	char tmp[1024];
	if( ipoDS1 == NULL )
	  sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename);
	if( ipoDS2 == NULL )
	  sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename1);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
      }
    for( int iLayer = 0; iLayer < ipoDS1->GetLayerCount();
	 iLayer++ )
      {
	OGRLayer        *poLayer1 = ipoDS1->GetLayer(iLayer);
	
	if( poLayer1 == NULL )
	  {
	    fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		     iLayer );
	    char tmp[1024];
	    sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
	  }

	for( int iLayer1 = 0; iLayer1 < ipoDS2->GetLayerCount();
	     iLayer1++ )
	  {
	    OGRLayer        *poLayer2 = ipoDS2->GetLayer(iLayer1);
	    
	    if( poLayer1 == NULL )
	      {
		fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
			 iLayer1 );
		char tmp[1024];
		sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer1);
		setMapInMaps(conf,"lenv","message",tmp);
		return SERVICE_FAILED;
	      }
	
	    OGRFeature  *poFeature1,*poFeature2;


	    while(TRUE){
	      OGRFeature      *poDstFeature = NULL;
	      poFeature1 = poLayer1->GetNextFeature();
	      if( poFeature1 == NULL )
		break;
	      while(TRUE){
		poFeature2 = poLayer2->GetNextFeature();
		if( poFeature2 == NULL )
		  break;
		if(poFeature1->GetGeometryRef() != NULL && poFeature2->GetGeometryRef() != NULL){
		  if((poFeature1->GetGeometryRef()->*myFunc)(poFeature2->GetGeometryRef())==0){
		    setMapInMaps(outputs,"Result","value","false");
		    OGRFeature::DestroyFeature( poFeature1 );
		    OGRFeature::DestroyFeature( poFeature2 );
		    delete ipoDS1;
		    delete ipoDS2;
		    free(filename);
		    free(filename1);
		    return SERVICE_SUCCEEDED;
		  }
		}
		OGRFeature::DestroyFeature( poFeature2 );
	      }
	      OGRFeature::DestroyFeature( poFeature1 );
	    }
	  }
      }

    delete ipoDS1;
    delete ipoDS2;
    free(filename);
    free(filename1);

    setMapInMaps(outputs,"Result","value","true");

    OGRCleanupAll();
    return SERVICE_SUCCEEDED;
  }


#ifdef WIN32
  __declspec(dllexport)
#endif
  int Equals(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,(OGRBoolean (OGRGeometry::*)(const OGRGeometry *) const)&OGRGeometry::Equals);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Disjoint(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,&OGRGeometry::Disjoint);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Touches(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,&OGRGeometry::Touches);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Crosses(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,&OGRGeometry::Crosses);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Within(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,&OGRGeometry::Within);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Contains(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,&OGRGeometry::Contains);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Overlaps(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,&OGRGeometry::Overlaps);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Intersects(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwoBool(conf,inputs,outputs,(OGRBoolean (OGRGeometry::*)(const OGRGeometry *) const)&OGRGeometry::Intersects);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Distance(maps*& conf,maps*& inputs,maps*& outputs){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1\n");
#endif
    fflush(stderr);
    maps* cursor=inputs;
    OGRGeometryH geometry1,geometry2;
    double res;
    {
      map* tmp=getMapFromMaps(inputs,"InputEntity1","value");
      map* tmp1=getMapFromMaps(inputs,"InputEntity1","mimeType");
#ifdef DEBUG
      fprintf(stderr,"MY MAP\n");
      dumpMap(tmp1);
      dumpMaps(inputs);
      fprintf(stderr,"MY MAP\n");
#endif
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"application/json",16)==0)
      	  geometry1=OGR_G_CreateGeometryFromJson(tmp->value);
	else
	  geometry1=createGeometryFromGML(conf,tmp->value);
      }
      else
      	geometry1=createGeometryFromGML(conf,tmp->value);
    }
    if(geometry1==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse input geometry for InputEntity1."));
      fprintf(stderr,"SERVICE FAILED !\n");
      return SERVICE_FAILED;
    }
    {
      map* tmp=getMapFromMaps(inputs,"InputEntity2","value");
      map* tmp1=getMapFromMaps(inputs,"InputEntity2","mimeType");
#ifdef DEBUG
      fprintf(stderr,"MY MAP\n");
      dumpMap(tmp1);
      dumpMaps(inputs);
      fprintf(stderr,"MY MAP\n");
#endif
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"application/json",16)==0)
      	  geometry2=OGR_G_CreateGeometryFromJson(tmp->value);
	else
	  geometry2=createGeometryFromGML(conf,tmp->value);
      }
      else
      	geometry2=createGeometryFromGML(conf,tmp->value);
    }
    if(geometry2==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse input geometry for InputEntity2."));
      fprintf(stderr,"SERVICE FAILED !\n");
      return SERVICE_FAILED;
    }
    res=OGR_G_Distance(geometry1,geometry2);    
    char tmpres[100];
    sprintf(tmpres,"%f",res);
    setMapInMaps(outputs,"Distance","value",tmpres);
    setMapInMaps(outputs,"Distance","dataType","float");
#ifdef DEBUG
    dumpMaps(outputs);
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int GetArea(maps*& conf,maps*& inputs,maps*& outputs){
    fprintf(stderr,"GETAREA \n");
    double res;
    /**
     * Extract Geometry from the InputPolygon value
     */
    OGRGeometryH geometry;
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(tmp==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    fprintf(stderr,"geometry creation %s \n",tmp->value);
    geometry=createGeometryFromGML(conf,tmp->value);
    if(geometry==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    fprintf(stderr,"geometry created %s \n",tmp->value);
    res=OGR_G_Area(geometry);
    fprintf(stderr,"area %d \n",res);
    /**
     * Filling the outputs
     */
    char tmp1[100];
    sprintf(tmp1,"%f",res);
    setMapInMaps(outputs,"Area","value",tmp1);
    setMapInMaps(outputs,"Area","dataType","float");
#ifdef DEBUG
    dumpMaps(outputs);
#endif
    return SERVICE_SUCCEEDED;
  }

}
