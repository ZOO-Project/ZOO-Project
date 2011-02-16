#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_euclidean_traits_xy_3.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <CGAL/Triangulation_face_base_2.h>

#include <fstream>

#include "cpl_minixml.h"
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "service.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

typedef CGAL::Delaunay_triangulation_2<K>  Triangulation;
typedef Triangulation::Edge_iterator  Edge_iterator;
typedef Triangulation::Point          Point;

typedef CGAL::Constrained_Delaunay_triangulation_2<K> CDT;
typedef CDT::Point Point;
typedef CDT::Vertex_handle Vertex_handle;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangulation_euclidean_traits_xy_3<K>  Gt;
typedef CGAL::Delaunay_triangulation_2<Gt> DelaunayTriangulation;

typedef Triangulation::Vertex_circulator Vertex_circulator;

typedef K::Point_3   Point1;

extern "C" {
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

  xmlNodeSet* extractFromDoc(xmlDocPtr,char*);
  void printExceptionReportResponse(maps*,map*);

  int Voronoi(maps*& conf,maps*& inputs,maps*& outputs){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\nStarting\n");
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    int bufferDistance;
    xmlInitParser();
    map* tmpm=NULL;
    tmpm=getMapFromMaps(inputs,"InputPoints","value");
    
    xmlInitParser();
    xmlDocPtr doc =
      xmlParseMemory(tmpm->value,strlen(tmpm->value));
    xmlNodePtr cur = xmlDocGetRootElement(doc);
    /**
     * Parse every Input in DataInputs node.
     */
    maps* tempMaps=NULL;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    xpathCtx = xmlXPathNewContext(doc);
    xpathObj = xmlXPathEvalExpression(BAD_CAST "/*/*[local-name()='featureMember']/*/*/*[local-name()='Point']/*[local-name()='coordinates']",xpathCtx);
    xmlXPathFreeContext(xpathCtx); 
    xmlNodeSet* nSet=xpathObj->nodesetval;

    if(nSet==NULL){
      setMapInMaps(conf,"lenv","message","Unable to continue !!!");
      return SERVICE_FAILED;
    }
    char filepath[2048];
    map* tmpMap=getMapFromMaps(conf,"main","tmpPath");
    if(tmpMap!=NULL){
      sprintf(filepath,"%s/varonoi_%d.tmp",tmpMap->value,getpid());
    }
    FILE *fo=fopen(filepath,"w");
#ifdef DEBUG
    fprintf(stderr,"File Creation (%s) OK\nPrinting %d Points.\n",filepath,nSet->nodeNr);
#endif
    for(int k=0;k<nSet->nodeNr;k++){
      xmlNodePtr cur=nSet->nodeTab[k];
      char *val=
	(char*)xmlNodeListGetString(doc,cur->xmlChildrenNode,1);
      char *tmp=strstr(val,",");
      char tmp1[1024];
      strncpy(tmp1,val,strlen(val)-strlen(tmp));
      tmp1[strlen(val)-strlen(tmp)]=0;
      char buff[1024];
      sprintf(buff,"%s %s\n",tmp1,tmp+1);
      fwrite(buff,1,strlen(buff)*sizeof(char),fo);
    }
    fclose(fo);
#ifdef DEBUG
    fprintf(stderr,"File Close (%s) OK\n",filepath);
#endif

    std::ifstream in(filepath);
    std::istream_iterator<Point> begin(in);
    std::istream_iterator<Point> end;
    Triangulation T;
    T.insert(begin, end);

    OGRRegisterAll();
    /* -------------------------------------------------------------------- */
    /*      Try opening the output datasource as an existing, writable      */
    /* -------------------------------------------------------------------- */
    OGRDataSource       *poODS;
    
    OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();
    OGRSFDriver          *poDriver = NULL;
    int                  iDriver;

    tmpMap=getMapFromMaps(outputs,"Result","mimeType");
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
	fprintf(stderr,"D:%s\n",poR->GetDriver(iDriver)->GetName());
#endif
	if( EQUAL(poR->GetDriver(iDriver)->GetName(),oDriver) )
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
	    sprintf( emessage,  "%s  -> `%s'\n", emessage, poR->GetDriver(iDriver)->GetName() );
	  }

	setMapInMaps(conf,"lenv","message",emessage);
	return SERVICE_FAILED;

      }

    if( !poDriver->TestCapability( ODrCCreateDataSource ) ){
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
    char *pszDestDataSource=(char*)malloc(strlen(tpath->value)+20);
    char **papszDSCO=NULL;
    sprintf(pszDestDataSource,"%s/result_%d.json",tpath->value,getpid());
    poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
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
    Edge_iterator eit =T.edges_begin();
    for ( ; eit !=T.edges_end(); ++eit) {
      CGAL::Object o = T.dual(eit);
      if (const K::Segment_2 *tmp=CGAL::object_cast<K::Segment_2>(&o)) {
	const K::Point_2 p1=tmp->source();
	const K::Point_2 p2=tmp->target();
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
      else if (CGAL::object_cast<K::Ray_2>(&o)) {++nr;}
    }
    OGR_DS_Destroy( poODS );
    OGRCleanupAll();

#ifdef DEBUG
    std::cerr << "The Voronoi diagram has " << ns << " finite edges "
	      << " and " << nr << " rays" << std::endl;
    sprintf(tmp1,"%d finite edges, %d rays",ns,nr);
#endif
    
    char tmp1[1024];

    FILE * fichier=fopen(pszDestDataSource,"r"); 
    struct stat file_status;
    stat(pszDestDataSource, &file_status);
    char *res1=(char *)malloc(file_status.st_size*sizeof(char));
    if(fichier==NULL){
      char tmp[1024];
      sprintf(tmp,"Failed to open file %s for reading purpose.\n",
	      pszDestDataSource);
      setMapInMaps(conf,"lenv","message",tmp);
      return SERVICE_FAILED;
    }
    fread(res1,1,(file_status.st_size)*sizeof(char),fichier);
    res1[strlen(res1)]=0;
    fclose(fichier);
    unlink(pszDestDataSource);
    
    setMapInMaps(outputs,"Result","value",res1);
    
    if(strcmp(oDriver,"GML")==0)
      setMapInMaps(outputs,"Result","mimeType","text/xml");
    else
      setMapInMaps(outputs,"Result","mimeType","text/plain");

    setMapInMaps(outputs,"Result","encoding","UTF-8");
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    xmlCleanupParser();
    return SERVICE_SUCCEEDED;
  }

}
