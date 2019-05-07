/**
 * Author : Gérald FENOY
 *
 * Copyright 2009-2013 GeoLabs SARL. All rights reserved.
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include "cgal_service.h"
typedef CGAL::Triangulation_vertex_base_with_info_2<unsigned int, Kernel> Vb;
typedef CGAL::Triangulation_data_structure_2<Vb>                       Tds;
typedef CGAL::Delaunay_triangulation_2<Kernel, Tds>                    DelaunayT;

extern "C" {

  int Delaunay(maps*& conf,maps*& inputs,maps*& outputs){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\nStarting\n");
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    int bufferDistance;
    map* tmpm=NULL;
    OGRRegisterAll();

    std::vector<Pointz> points;
    if(int res=parseInput(conf,inputs,&points,"/vsimem/tmp")!=SERVICE_SUCCEEDED)
      return res;

    DelaunayT T;
    T.insert(points.begin(), points.end());

    /* -------------------------------------------------------------------- */
    /*      Try opening the output datasource as an existing, writable      */
    /* -------------------------------------------------------------------- */
#if GDAL_VERSION_MAJOR >= 2
    GDALDataset *poODS;
    GDALDriverManager* poR=GetGDALDriverManager();
    GDALDriver          *poDriver = NULL;
#else
    OGRDataSource       *poODS;    
    OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();
    OGRSFDriver          *poDriver = NULL;
#endif
    int                  iDriver;
    map *tmpMap=getMapFromMaps(outputs,"Result","mimeType");
    const char *oDriver;
    oDriver="GeoJSON";
    if(tmpMap!=NULL){
      if(strcmp(tmpMap->value,"text/xml")==0){
	oDriver="GML";
      }
    }
    
    for( iDriver = 0;
	 iDriver < poR->GetDriverCount() && poDriver == NULL;
	 iDriver++ )
      {
#ifdef DEBUG
#if GDAL_VERSION_MAJOR >= 2
	fprintf(stderr,"D:%s\n",poR->GetDriver(iDriver)->GetDescription());
#else
	fprintf(stderr,"D:%s\n",poR->GetDriver(iDriver)->GetName());
#endif
#endif
	if( EQUAL(
#if GDAL_VERSION_MAJOR >= 2
		  poR->GetDriver(iDriver)->GetDescription()
#else
		  poR->GetDriver(iDriver)->GetName()
#endif
		  ,
		  oDriver) )
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
#if GDAL_VERSION_MAJOR >= 2
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
    char *pszDestDataSource=(char*)malloc(100);
    char **papszDSCO=NULL;
    sprintf(pszDestDataSource,"/vsimem/result_%d",getpid());
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
    
    CPLErrorReset();
    
    OGRLayer *poDstLayer = poODS->CreateLayer( "Result", NULL,wkbPolygon,NULL);
    if( poDstLayer == NULL ){
      setMapInMaps(conf,"lenv","message","Layer creation failed.\n");
      return SERVICE_FAILED;
    }


    for(DelaunayT::Finite_faces_iterator fit = T.finite_faces_begin();
	fit != T.finite_faces_end(); ++fit) {
      DelaunayT::Face_handle face = fit;
      OGRFeatureH hFeature = OGR_F_Create( OGR_L_GetLayerDefn( poDstLayer ) );
      OGRGeometryH hCollection = OGR_G_CreateGeometry( wkbGeometryCollection );
      OGRGeometryH currLine=OGR_G_CreateGeometry(wkbLinearRing);
      OGRGeometryH currPoly=OGR_G_CreateGeometry(wkbPolygon);
      OGR_G_AddPoint_2D(currLine,T.triangle(face)[0].x(),T.triangle(face)[0].y());
      OGR_G_AddPoint_2D(currLine,T.triangle(face)[1].x(),T.triangle(face)[1].y());
      OGR_G_AddPoint_2D(currLine,T.triangle(face)[2].x(),T.triangle(face)[2].y());
      OGR_G_AddPoint_2D(currLine,T.triangle(face)[0].x(),T.triangle(face)[0].y());
      OGR_G_AddGeometryDirectly( currPoly, currLine ); 
      OGR_G_AddGeometryDirectly( hCollection, currPoly ); 
      OGR_F_SetGeometry( hFeature, hCollection ); 
      OGR_G_DestroyGeometry(hCollection);
      if( OGR_L_CreateFeature( poDstLayer, hFeature ) != OGRERR_NONE ){
	setMapInMaps(conf,"lenv","message","Failed to create feature in file.\n");
	return SERVICE_FAILED;
      }
      OGR_F_Destroy( hFeature );
    }
    OGR_DS_Destroy( poODS );

#ifdef DEBUG
    std::cerr << "The Voronoi diagram has " << ns << " finite edges "
	      << " and " << nr << " rays" << std::endl;
#endif

    char *res1=readVSIFile(conf,pszDestDataSource);
    if(res1==NULL)
      return SERVICE_FAILED;

    setMapInMaps(outputs,"Result","value",res1);
    free(res1);

    if(strcmp(oDriver,"GML")==0)
      setMapInMaps(outputs,"Result","mimeType","text/xml");
    else
      setMapInMaps(outputs,"Result","mimeType","application/json");

    setMapInMaps(outputs,"Result","encoding","UTF-8");
    OGRCleanupAll();
    
    return SERVICE_SUCCEEDED;
  }

}
