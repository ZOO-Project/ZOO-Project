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

  char *base64(const unsigned char *input, int length){
    BIO *bmem, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char *)malloc(bptr->length+1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;

    BIO_free_all(b64);

    return buff;
  }

  OGRGeometryH createGeometryFromGML(maps* conf,char* inputStr){
    xmlInitParser();
    xmlDocPtr doc = xmlParseMemory(inputStr,strlen(inputStr));
    xmlChar *xmlbuff;
    int buffersize;
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;
    char * xpathExpr="/*/*/*/*/*[local-name()='Polygon' or local-name()='MultiPolygon']";
    xpathCtx = xmlXPathNewContext(doc);
    xpathObj = xmlXPathEvalExpression(BAD_CAST xpathExpr,xpathCtx);
    if(!xpathObj->nodesetval){
      map* tmp=createMap("text","Unable to parse Input Polygon");
      addToMap(tmp,"code","InvalidParameterValue");
      printExceptionReportResponse(conf,tmp);
      exit(0);
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
    char *tmp=strdup(strstr((char*)xmlbuff,"?>")+2);
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx); 
    xmlFree(xmlbuff);
    xmlFreeDoc(doc);
    xmlCleanupParser();
    fprintf(stderr,"\nService internal print\n Loading the geometry from GML string ...");
    OGRGeometryH res=OGR_G_CreateFromGML(tmp);
    if(res==NULL){
      map* tmp=createMap("text","Unable to call OGR_G_CreatFromGML");
      addToMap(tmp,"code","NoApplicableCode");
      printExceptionReportResponse(conf,tmp);
      exit(0);
    }
    else
      return OGR_G_CreateFromGML(tmp);
  }

  
  int Delaunay(maps*& conf,maps*& inputs,maps*& outputs){
    fprintf(stderr,"\nService internal print\nStarting\n");
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
    xmlNodeSet* toto=xpathObj->nodesetval;

    if(toto==NULL)
      fprintf(stderr,"IMPOSSIBLE DE CONTINUER !!!!\n");
    char filepath[2048];
    map* tmpMap=getMapFromMaps(conf,"main","tmpPath");
    if(tmpMap!=NULL){
      sprintf(filepath,"%s/varonoi_%d.tmp",tmpMap->value,getpid());
    }
    FILE *fo=fopen(filepath,"w");
    fprintf(stderr,"File Creation (%s) OK\nPrinting %d Points.\n",filepath,toto->nodeNr);
    for(int k=0;k<toto->nodeNr;k++){
      xmlNodePtr cur=toto->nodeTab[k];
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
    fprintf(stderr,"File Close (%s) OK\n",filepath);

    std::ifstream in(filepath);

    std::istream_iterator<Point1> begin(in);
    std::istream_iterator<Point1> end;

    DelaunayTriangulation dt1;
    dt1.insert(begin, end);

    OGRRegisterAll();
    /* -------------------------------------------------------------------- */
    /*      Try opening the output datasource as an existing, writable      */
    /* -------------------------------------------------------------------- */
    OGRDataSource       *poODS;
    
    OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();
    OGRSFDriver          *poDriver = NULL;
    int                  iDriver;

    tmpMap=getMapFromMaps(outputs,"Result","mimeType");
    char *oDriver=(char*)malloc(7*sizeof(char));
    oDriver="GeoJSON";
    if(tmpMap!=NULL){
      if(strcmp(tmpMap->value,"text/xml")==0){
	fprintf(stderr,"USEING GML \n");
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
	
	fprintf(stderr,"{\"type\": \"error\", \"%s\"}",emessage);   
	exit( 1 );
      }

    if( !poDriver->TestCapability( ODrCCreateDataSource ) ){
      char emessage[1024];
      sprintf( emessage,  "%s driver does not support data source creation.\n",
	       "json" );
      fprintf(stderr,"{\"type\": \"error\", \"%s\"}",emessage);   
      exit( 1 );
    }

    /* -------------------------------------------------------------------- */
    /*      Create the output data source.                                  */
    /* -------------------------------------------------------------------- */
    map* tpath=getMapFromMaps(conf,"main","tmpPath");
    char *pszDestDataSource=(char*)malloc(strlen(tpath->value)+20);
    char **papszDSCO=NULL;
    sprintf(pszDestDataSource,"%s/result_%d.json",tpath->value,getpid());
    fprintf(stderr,"\n *%s* \n",pszDestDataSource);
    poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
    if( poODS == NULL ){
      char emessage[1024];      
      sprintf( emessage,  "%s driver failed to create %s\n", 
	       "json", pszDestDataSource );
      fprintf(stderr,"{\"type\": \"error\", \"%s\"}",emessage);   
      exit( 1 );
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
	fprintf(stderr,"{\"type\": \"error\",\"message\": \"%s\"}",emessage);
	return FALSE;
      }
    
    CPLErrorReset();
    
    OGRLayer *poDstLayer = poODS->CreateLayer( "Result", NULL,wkbLineString,NULL);
    if( poDstLayer == NULL ){
      fprintf( stderr,"Layer creation failed.\n" );
      exit( 1 );
    }

    Vertex_circulator vc = dt1.incident_vertices(dt1.infinite_vertex()),
      done(vc);
    if (vc != 0) {
      do {std::cout << vc->point() << std::endl;}while(++vc != done);
    }

    DelaunayTriangulation::Edge_iterator eit=dt1.edges_begin();
    int i=0;
    for ( ; eit !=dt1.edges_end(); ++eit) {
      fprintf(stderr,"Edge %d\n",i);
      i++;
      /*CGAL::Object o = dt1.dual(eit);
      if (const K::Segment_2 *tmp=CGAL::object_cast<K::Segment_2>(&o)) {
	const K::Point_2 p1=tmp->source();
	const K::Point_2 p2=tmp->target();
	//#ifdef DEBUG
	fprintf(stderr,"P1 %d %d | P2 %d %d\n",p1.x(),p1.y(),p2.x(),p2.y());
	//#endif
	}*/
      /*if (const K::Segment_3 *tmp=CGAL::object_cast<K::Segment_3>(eit)) {
	const K::Point_3 p1=tmp->source();
	const K::Point_3 p2=tmp->target();
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
	  fprintf( stderr,"Failed to create feature in file.\n" );
	  exit( 1 );
	  }
	  OGR_F_Destroy( hFeature );*/
    }
  
    int idg=0;
    for (DelaunayTriangulation::Face_iterator fit = dt1.faces_begin(); fit!=
	   dt1.faces_end(); ++fit) {
      int idx = 0;
      int i0, i1, i2;

      OGRFeatureH hFeature = OGR_F_Create( OGR_L_GetLayerDefn( poDstLayer ) );
      OGRGeometryH currLine=OGR_G_CreateGeometry(wkbLineString);
      
      for(i0=0;i0<3;i0++){
	if(!dt1.is_infinite((*fit).vertex(i0)))
	  OGR_G_AddPoint_2D(currLine,(*fit).vertex(i0)->point().x(),
			    (*fit).vertex(i0)->point().y());
	if(i0==2 && !dt1.is_infinite((*fit).vertex(0)))
	  OGR_G_AddPoint_2D(currLine,(*fit).vertex(0)->point().x(),
			    (*fit).vertex(0)->point().y());
      }
      OGR_F_SetGeometry( hFeature, currLine );
      OGR_G_DestroyGeometry(currLine);
      if( OGR_L_CreateFeature( poDstLayer, hFeature ) != OGRERR_NONE ){
	fprintf( stderr,"Failed to create feature in file.\n" );
	exit( 1 );
      }
      OGR_F_Destroy( hFeature );

      /*OGRFeatureH hFeature=NULL;
      OGRGeometryH currLine=NULL;
      if(idg==0) {
	hFeature = OGR_F_Create( OGR_L_GetLayerDefn( poDstLayer ) );
	currLine=OGR_G_CreateGeometry(wkbLineString);      
	}*/

      for (DelaunayTriangulation::Vertex_iterator vit = dt1.vertices_begin(); vit!=
	     dt1.vertices_end(); ++vit) {
	idx++;
	/*if(idg==0)
	  OGR_G_AddPoint_2D(currLine,(*vit).point().x(),
	  (*vit).point().y());*/


	if ((*vit).point() == (*fit).vertex(0)->point()){
	  i0 = idx; 
	  /*OGR_G_AddPoint_2D(currLine,(*fit).vertex(i0)->point().x(),
	    (*fit).vertex(i0)->point().y());**/
	}
	if ((*vit).point() == (*fit).vertex(1)->point()){  
	  i1 = idx; 
	  /*OGR_G_AddPoint_2D(currLine,(*fit).vertex(i1)->point().x(),
	    (*fit).vertex(i1)->point().y());*/
	}
	if ((*vit).point() == (*fit).vertex(2)->point()){  
	  i2 = idx; 
	  /*OGR_G_AddPoint_2D(currLine,(*fit).vertex(i2)->point().x(),
	    (*fit).vertex(i2)->point().y());*/
	}
      }
      /*if(idg==0){
	OGR_F_SetGeometry( hFeature, currLine );
	OGR_G_DestroyGeometry(currLine);
	if( OGR_L_CreateFeature( poDstLayer, hFeature ) != OGRERR_NONE ){
	  fprintf( stderr,"Failed to create feature in file.\n" );
	  exit( 1 );
	}
	OGR_F_Destroy( hFeature );
	idg++;
	}*/

      std::cerr << "f " << i0 << " " << dt1.triangle(fit) << std::endl;
      
    } 
    OGR_DS_Destroy( poODS );
    OGRCleanupAll();

 
    char tmp1[1024];
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Result";

    FILE * fichier=fopen(pszDestDataSource,"r"); 
    struct stat file_status;
    stat(pszDestDataSource, &file_status);
    fprintf(stderr,"%s (%d)",pszDestDataSource,file_status.st_size);
    char *res1=(char *)malloc(file_status.st_size*sizeof(char));
    if(fichier==NULL)
      fprintf(stderr,"Failed to open file %s for reading purpose.\n",pszDestDataSource);
    fread(res1,1,(file_status.st_size)*sizeof(char),fichier);
    res1[strlen(res1)]=0;
    fclose(fichier);
    //unlink(pszDestDataSource);
    //fprintf(stderr,"Read (%s).\n",res1);
    
    outputs->content=createMap("value",res1);
    
    if(strcmp(oDriver,"GML")==0)
      addMapToMap(&outputs->content,createMap("mimeType","text/xml"));
    else
      addMapToMap(&outputs->content,createMap("mimeType","text/plain"));

    addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
    outputs->next=NULL;
    fprintf(stderr,"\nService internal print\n===\n");
    xmlCleanupParser();

    return SERVICE_SUCCEEDED;
  }
  

  int Voronoi(maps*& conf,maps*& inputs,maps*& outputs){
    fprintf(stderr,"\nService internal print\nStarting\n");
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
    xmlNodeSet* toto=xpathObj->nodesetval;

    if(toto==NULL)
      fprintf(stderr,"IMPOSSIBLE DE CONTINUER !!!!\n");
    char filepath[2048];
    map* tmpMap=getMapFromMaps(conf,"main","tmpPath");
    if(tmpMap!=NULL){
      sprintf(filepath,"%s/varonoi_%d.tmp",tmpMap->value,getpid());
    }
    FILE *fo=fopen(filepath,"w");
    fprintf(stderr,"File Creation (%s) OK\nPrinting %d Points.\n",filepath,toto->nodeNr);
    for(int k=0;k<toto->nodeNr;k++){
      xmlNodePtr cur=toto->nodeTab[k];
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
    fprintf(stderr,"File Close (%s) OK\n",filepath);

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
    char *oDriver=(char*)malloc(7*sizeof(char));
    oDriver="GeoJSON";
    if(tmpMap!=NULL){
      if(strcmp(tmpMap->value,"text/xml")==0){
	fprintf(stderr,"USEING GML \n");
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
	
	fprintf(stderr,"{\"type\": \"error\", \"%s\"}",emessage);   
	exit( 1 );
      }

    if( !poDriver->TestCapability( ODrCCreateDataSource ) ){
      char emessage[1024];
      sprintf( emessage,  "%s driver does not support data source creation.\n",
	       "json" );
      fprintf(stderr,"{\"type\": \"error\", \"%s\"}",emessage);   
      exit( 1 );
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
      fprintf(stderr,"{\"type\": \"error\", \"%s\"}",emessage);   
      exit( 1 );
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
	fprintf(stderr,"{\"type\": \"error\",\"message\": \"%s\"}",emessage);
	return FALSE;
      }
    
    CPLErrorReset();
    
    OGRLayer *poDstLayer = poODS->CreateLayer( "Result", NULL,wkbLineString,NULL);
    if( poDstLayer == NULL ){
      fprintf( stderr,"Layer creation failed.\n" );
      exit( 1 );
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
	  fprintf( stderr,"Failed to create feature in file.\n" );
	  exit( 1 );
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
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Result";

    FILE * fichier=fopen(pszDestDataSource,"r"); 
    struct stat file_status;
    stat(pszDestDataSource, &file_status);
    //fprintf(stderr,"%s (%d)",pszDestDataSource,file_status.st_size);
    char *res1=(char *)malloc(file_status.st_size*sizeof(char));
    if(fichier==NULL)
      fprintf(stderr,"Failed to open file %s for reading purpose.\n",pszDestDataSource);
    fread(res1,1,(file_status.st_size)*sizeof(char),fichier);
    res1[strlen(res1)]=0;
    fclose(fichier);
    unlink(pszDestDataSource);
    //fprintf(stderr,"Read (%s).\n",res1);
    
    outputs->content=createMap("value",res1);
    
    if(strcmp(oDriver,"GML")==0)
      addMapToMap(&outputs->content,createMap("mimeType","text/xml"));
    else
      addMapToMap(&outputs->content,createMap("mimeType","text/plain"));

    addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
    outputs->next=NULL;
    fprintf(stderr,"\nService internal print\n===\n");
    xmlCleanupParser();
    return SERVICE_SUCCEEDED;
  }

}
