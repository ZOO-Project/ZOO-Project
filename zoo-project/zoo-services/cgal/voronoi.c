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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
//#include <CGAL/Triangulation_euclidean_traits_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
//#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <CGAL/Triangulation_face_base_2.h>

#include <fstream>

#include "cpl_minixml.h"
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "service.h"
#include "cgal_service.h"

typedef CGAL::Delaunay_triangulation_2<Kernel>  Triangulation;
typedef Triangulation::Face_iterator  Face_iterator;
typedef Triangulation::Edge_iterator  Edge_iterator;
typedef Triangulation::Vertex_circulator Vertex_circulator;

extern "C" {

  int Voronoi(maps*& conf,maps*& inputs,maps*& outputs){
    //#ifdef DEBUG
    fprintf(stderr,"\nService internal print\nStarting\n");
    //#endif
    //return SERVICE_FAILED;
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    int bufferDistance;
    map* tmpm=NULL;
    tmpm=getMapFromMaps(inputs,"InputPoints","value");

    fprintf(stderr," **** %s %d\n",__FILE__,__LINE__);
    fflush(stderr);

    OGRRegisterAll();

    std::vector<Pointz> points;
    if(int res=parseInput(conf,inputs,&points,"/vsimem/tmp")!=SERVICE_SUCCEEDED){
      fprintf(stderr," **** %s %d\n",__FILE__,__LINE__);
      fflush(stderr);
      return SERVICE_FAILED;
    }
    fprintf(stderr," **** %s %d\n",__FILE__,__LINE__);
    fflush(stderr);
    
    Triangulation T;
    T.insert(points.begin(), points.end());

    //OGRRegisterAll();
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
    map* tmpMap=getMapFromMaps(outputs,"Result","mimeType");
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
    map* tpath=getMapFromMaps(conf,"main","tmpPath");
    char *pszDestDataSource=(char*)malloc(100);
    char **papszDSCO=NULL;
    sprintf(pszDestDataSource,"/vsimem/result_%d.json",tpath->value,getpid());
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
    
    OGRLayer *poDstLayer = poODS->CreateLayer( "Result", NULL,wkbLineString,NULL);
    if( poDstLayer == NULL ){
      setMapInMaps(conf,"lenv","message","Layer creation failed.\n");
      return SERVICE_FAILED;
    }


    int ns = 0;
    int nr = 0;
    int nf = 0;
    Face_iterator fit =T.faces_begin();
    FILE* f=fopen("/tmp/toto.log","w+");
    for ( ; fit !=T.faces_end(); ++fit) {
      fprintf(stderr," *** %s %d %d %d\n",__FILE__,__LINE__,nf,fit.info());
      fflush(stderr);
      
      nf++;
    }
    fclose(f);
    Edge_iterator eit =T.edges_begin();
    for ( ; eit !=T.edges_end(); ++eit) {
      CGAL::Object o = T.dual(eit);
      if (const Kernel::Segment_2 *tmp=CGAL::object_cast<Kernel::Segment_2>(&o)) {
	const Pointz p1=tmp->source();
	const Pointz p2=tmp->target();
	fprintf(stderr,"P1 %d %d | P2 %d %d\n",p1.x(),p1.y(),p2.x(),p2.y());
#ifdef DEBUG
	fprintf(stderr,"P1 %d %d | P2 %d %d\n",p1.x(),p1.y(),p2.x(),p2.y());
#endif
	OGRFeatureH hFeature = OGR_F_Create( OGR_L_GetLayerDefn( poDstLayer ) );
	OGRGeometryH currLine=OGR_G_CreateGeometry(wkbLineString);
	OGR_G_AddPoint_2D(currLine,p1.x(),p1.y());
	OGR_G_AddPoint_2D(currLine,p2.x(),p2.y());
	OGR_F_SetGeometry( hFeature, currLine ); 
	OGR_G_DestroyGeometry(currLine);
	if( OGR_L_CreateFeature( poDstLayer, hFeature ) != OGRERR_NONE ){
	  setMapInMaps(conf,"lenv","message","Failed to create feature in file.\n");
	  return SERVICE_FAILED;
	}
	OGR_F_Destroy( hFeature );
	++ns ;
      }
      else if (const Kernel::Ray_2 *tmp=CGAL::object_cast<Kernel::Ray_2>(&o)) { 
	const Pointz p1=tmp->source();
	const Pointz p2=tmp->point(2);
	OGRFeatureH hFeature = OGR_F_Create( OGR_L_GetLayerDefn( poDstLayer ) );
	OGRGeometryH currLine=OGR_G_CreateGeometry(wkbLineString);
	OGR_G_AddPoint_2D(currLine,p1.x(),p1.y());
	OGR_G_AddPoint_2D(currLine,p2.x(),p2.y());
	OGR_F_SetGeometry( hFeature, currLine );
	OGR_G_DestroyGeometry(currLine);
	if( OGR_L_CreateFeature( poDstLayer, hFeature ) != OGRERR_NONE ){
	  setMapInMaps(conf,"lenv","message","Failed to create feature in file.\n");
	  return SERVICE_FAILED;
	}
	OGR_F_Destroy( hFeature );
	++nr; 
      }
    }
    OGR_DS_Destroy( poODS );

#ifdef DEBUG
    std::cerr << "The Voronoi diagram has " << ns << " finite edges "
	      << " and " << nr << " rays" << std::endl;
    sprintf(tmp1,"%d finite edges, %d rays",ns,nr);
#endif
    

    char *res1=readVSIFile(conf,pszDestDataSource);
    if(res1==NULL)
      return SERVICE_FAILED;
    
    setMapInMaps(outputs,"Result","value",res1);
    
    if(strcmp(oDriver,"GML")==0)
      setMapInMaps(outputs,"Result","mimeType","text/xml");
    else
      setMapInMaps(outputs,"Result","mimeType","application/json");

    setMapInMaps(outputs,"Result","encoding","UTF-8");
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    //OGRCleanupAll();
    return SERVICE_SUCCEEDED;
  }

}
