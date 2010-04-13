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

#include "cpl_minixml.h"
#include "ogr_api.h"
#include "service.h"

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
  char *base64(const unsigned char *input, int length);

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
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n Loading the geometry from GML string ...");
#endif
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

  int applyTwo(maps*& conf,maps*& inputs,maps*& outputs,OGRGeometryH (*myFunc)(OGRGeometryH,OGRGeometryH)){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1\n");
#endif
    fflush(stderr);

    maps* cursor=inputs;
    OGRGeometryH geometry1,geometry2;
    OGRGeometryH res;
    {
      map* tmp=getMapFromMaps(inputs,"InputEntity1","value");
      map* tmp1=getMapFromMaps(inputs,"InputEntity1","mimeType");
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"application/json",16)==0)
      	  geometry1=OGR_G_CreateGeometryFromJson(tmp->value);
	else
	  geometry1=createGeometryFromGML(conf,tmp->value);
      }
      else
      	geometry1=createGeometryFromGML(conf,tmp->value);
    }
    {
      map* tmp=getMapFromMaps(inputs,"InputEntity2","value");
      map* tmp1=getMapFromMaps(inputs,"InputEntity2","mimeType");
#ifdef DEBUG
      fprintf(stderr,"MY MAP\n");
      dumpMap(tmp1);
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
    res=(*myFunc)(geometry1,geometry2);
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Result";
    char tmpres[100];
    sprintf(tmpres,"%d",res);
    outputs->content=createMap("value",OGR_G_ExportToJson(res));
    addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
    addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
    outputs->next=NULL;
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Difference(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGR_G_Difference);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int SymDifference(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGR_G_SymmetricDifference);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Intersection(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGR_G_Intersection);
  }

  int applyOne(maps*& conf,maps*& inputs,maps*& outputs,OGRGeometryH (*myFunc)(OGRGeometryH)){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n");
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    int bufferDistance;
#ifdef DEBUG
    dumpMaps(cursor);
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp)
      return SERVICE_FAILED;
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeTime");
    if(tmp1!=NULL){
      if(strncmp(tmp1->value,"text/js",7)==0)
        geometry=OGR_G_CreateGeometryFromJson(tmp->value);
      else
        geometry=createGeometryFromGML(conf,tmp->value);
      }
    else
      geometry=createGeometryFromGML(conf,tmp->value);
    res=(*myFunc)(geometry);
    /**
     * Here we should check what is the requested output.
     * Let's use JSON per default.
     */
    //outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Result";
    if(tmp1!=NULL){
      if(strncmp(tmp1->value,"text/js",7)==0){
        outputs->content=createMap("value",OGR_G_ExportToJson(tmp->value));
	addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
	addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
      }
      else{
        outputs->content=createMap("value",OGR_G_ExportToGML(res));
	addMapToMap(&outputs->content,createMap("mimeType","text/xml"));
	addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
	addMapToMap(&outputs->content,createMap("schema","http://fooa/gml/3.1.0/polygon.xsd"));
      }
    }else{
      outputs->content=createMap("value",OGR_G_ExportToJson(res));
      addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
      addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
    }
    outputs->next=NULL;
#ifdef DEBUG
    dumpMaps(outputs);
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int ConvexHull(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOne(conf,inputs,outputs,&OGR_G_ConvexHull);
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Boundary(maps*& conf,maps*& inputs,maps*& outputs){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\nStarting\n");
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    int bufferDistance;
    xmlInitParser();
    
    if(cursor!=NULL){
      map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
      map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"text/js",7)==0)
      	  geometry=OGR_G_CreateGeometryFromJson(tmp->value);
	else
	  geometry=createGeometryFromGML(conf,tmp->value);
      }
      else
      	geometry=createGeometryFromGML(conf,tmp->value);
    }
    res=OGR_G_GetBoundary(geometry);
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Boundary";
    outputs->content=createMap("value",OGR_G_ExportToJson(res));
    addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
    addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
    /*
    outputs->content=createMap("value",OGR_G_ExportToGML(res));
    addMapToMap(&outputs->content,createMap("mimeType","text/xml"));
    addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
    addMapToMap(&outputs->content,createMap("schema","http://fooa/gml/3.1.0/polygon.xsd"));
    */
    outputs->next=NULL;
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    xmlCleanupParser();
    return SERVICE_SUCCEEDED;
    }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Centroid(maps*& conf,maps*& inputs,maps*& outputs){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n");
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry,res=NULL;
    int bufferDistance;    
    if(cursor!=NULL){
#ifdef DEBUG
      fprintf(stderr,"\nService internal print\n");
      dumpMaps(cursor);
      fprintf(stderr,"\nService internal print\n");
      dumpMaps(inputs);
      fprintf(stderr,"\nService internal print\n");
#endif
      map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
      map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"application/json",16)==0)
      	  geometry=OGR_G_CreateGeometryFromJson(tmp->value);
	else
	  geometry=createGeometryFromGML(conf,tmp->value);
      }
      else
      	geometry=createGeometryFromGML(conf,tmp->value);
    }else{
    /**
     * Print WPSExceptionError message here !!
     */
    }
#ifdef DEBUG
    dumpMaps(outputs);
#endif
    res=OGR_G_CreateGeometryFromJson("{\"type\": \"Point\", \"coordinates\": [0,0] }");
    OGRwkbGeometryType gtype=OGR_G_GetGeometryType(geometry);
    if(gtype!=wkbPolygon){
    	fprintf(stderr,"\n\nGeometry Type is not Polygon, let us use the ConvexHull !\n\n");
    	geometry=OGR_G_ConvexHull(OGR_G_Clone(geometry));
    }else{
    	fprintf(stderr,"\n\nGeometry Type is Polygon, thanks !\n\n");
    }
    int c=OGR_G_Centroid(geometry,res);
#ifdef DEBUG
    fprintf(stderr,"\n\nC VALUE : %d\n\n",c);
#endif
    map* tmp=getMapFromMaps(outputs,"Result","mimeType");
#ifdef DEBUG
    dumpMaps(outputs);
#endif
    if(tmp!=NULL){
#ifdef DEBUG
      dumpMap(tmp);
#endif
      if(strncmp(mtoupper(tmp->value),mtoupper("application/json"),16)==0){
	outputs->content=createMap("value",OGR_G_ExportToJson(res));
	addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
	addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
      }
      else if(strncmp(mtoupper(tmp->value),mtoupper("text/xml"),8)==0){
	/*xmlInitParser();
	xmlDocPtr doc = xmlParseMemory(tmp->value,strlen(tmp->value));
	xmlChar *xmlbuff;
	int buffersize;*/
	outputs->content=createMap("value",OGR_G_ExportToGML(res));
	addMapToMap(&outputs->content,createMap("mimeType","text/xml"));
	map* tmp1=getMapFromMaps(outputs,"Result","encoding");
	if(tmp1!=NULL)
	  addMapToMap(&outputs->content,createMap("encoding",tmp1->value));
	else
	  addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
	/*xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
	outputs->content=createMap("value",OGR_G_ExportToGML(res));
	xmlFree(xmlbuff);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	*/
      }
    else{
      outputs=(maps*)malloc(sizeof(maps*));
      outputs->name="Result";
      outputs->content=createMap("value",OGR_G_ExportToJson(res));
      addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
      addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
      outputs->next=NULL;
    }
    outputs->next=NULL;
    }
    else{
      outputs=(maps*)malloc(sizeof(maps*));
      outputs->name="Result";
      outputs->content=createMap("value",OGR_G_ExportToJson(res));
      addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
      addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
      outputs->next=NULL;
    }
    outputs->next=NULL;
#ifdef DEBUG
    dumpMaps(outputs);
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Buffer(maps*& conf,maps*& inputs,maps*& outputs){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n");
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    int bufferDistance;    
    if(cursor!=NULL){
#ifdef DEBUG
      fprintf(stderr,"\nService internal print\n");
      dumpMaps(cursor);
      fprintf(stderr,"\nService internal print\n");
      dumpMaps(inputs);
      fprintf(stderr,"\nService internal print\n");
#endif
      map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
      map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"application/json",16)==0)
      	  geometry=OGR_G_CreateGeometryFromJson(tmp->value);
	else
	  geometry=createGeometryFromGML(conf,tmp->value);
      }
      else
      	geometry=createGeometryFromGML(conf,tmp->value);
    }
    if(cursor!=NULL){
      map* tmp=getMapFromMaps(cursor,"BufferDistance","value");
      if(tmp==NULL){
	bufferDistance=10;
      }
      else
	bufferDistance=atoi(tmp->value);
#ifdef DEBUG
      fprintf(stderr,"\nService internal print (BufferDistance value: %i)\n",bufferDistance);
#endif
    }
#ifdef DEBUG
    dumpMaps(outputs);
#endif
    map* tmp=getMapFromMaps(outputs,"BufferedPolygon","mimeType");
    res=OGR_G_Buffer(geometry,bufferDistance,30);
#ifdef DEBUG
    dumpMap(tmp);
#endif
    if(tmp!=NULL){
#ifdef DEBUG
      dumpMap(tmp);
#endif
      if(strncmp(mtoupper(tmp->value),mtoupper("application/json"),16)==0){
	outputs->content=createMap("value",OGR_G_ExportToJson(res));
	addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
	addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
      }
      else if(strcmp(mtoupper(tmp->value),mtoupper("text/xml"))==0){
	xmlInitParser();
	xmlDocPtr doc = xmlParseMemory(tmp->value,strlen(tmp->value));
	xmlChar *xmlbuff;
	int buffersize;
	outputs->content=createMap("value",OGR_G_ExportToGML(res));
	addMapToMap(&outputs->content,createMap("mimeType","text/xml"));
	map* tmp1=getMapFromMaps(outputs,"BufferedPolygon","encoding");
	if(tmp1!=NULL)
	  addMapToMap(&outputs->content,createMap("encoding",tmp1->value));
	else
	  addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
	xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
	outputs->content=createMap("value",OGR_G_ExportToGML(res));
	xmlFree(xmlbuff);
	xmlFreeDoc(doc);
	xmlCleanupParser();
      }
    else{
      outputs=(maps*)malloc(sizeof(maps*));
      outputs->name="BufferedPolygon";
      outputs->content=createMap("value",OGR_G_ExportToJson(res));
      addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
      addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
      outputs->next=NULL;
    }
    outputs->next=NULL;
    }
    else{
      outputs=(maps*)malloc(sizeof(maps*));
      outputs->name="BufferedPolygon";
      outputs->content=createMap("value",OGR_G_ExportToJson(res));
      addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
      addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
      outputs->next=NULL;
    }
    outputs->next=NULL;
#ifdef DEBUG
    dumpMaps(outputs);
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Union(maps*& conf,maps*& inputs,maps*& outputs){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1\n");
    fflush(stderr);
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry1,geometry2;
    OGRGeometryH res;
    {
      map* tmp=getMapFromMaps(inputs,"InputEntity1","value");
      map* tmp1=getMapFromMaps(inputs,"InputEntity1","mimeType");
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"application/json",16)==0)
      	  geometry1=OGR_G_CreateGeometryFromJson(tmp->value);
	else
	  geometry1=createGeometryFromGML(conf,tmp->value);
      }
      else
      	geometry1=createGeometryFromGML(conf,tmp->value);
    }
    {
      map* tmp=getMapFromMaps(inputs,"InputEntity2","value");
      map* tmp1=getMapFromMaps(inputs,"InputEntity2","mimeType");
#ifdef DEBUG
      fprintf(stderr,"MY MAP\n");
      dumpMap(tmp1);
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
    res=OGR_G_Union(geometry1,geometry2);
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Result";
    char tmpres[100];
    sprintf(tmpres,"%d",res);
    outputs->content=createMap("value",OGR_G_ExportToJson(res));
    addMapToMap(&outputs->content,createMap("mimeType","text/plain"));
    addMapToMap(&outputs->content,createMap("encoding","UTF-8"));
    outputs->next=NULL;
    return SERVICE_SUCCEEDED;
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
    res=OGR_G_Distance(geometry1,geometry2);
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Distance";
    char tmpres[100];
    sprintf(tmpres,"%d",res);
    outputs->content=createMap("value",tmpres);
    addMapToMap(&outputs->content,createMap("datatype","float"));
    outputs->next=NULL;
#ifdef DEBUG
    dumpMaps(outputs);
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    return SERVICE_SUCCEEDED;
  }

  int GetArea(maps*& conf,maps*& inputs,maps*& outputs){
    fprintf(stderr,"GETAREA \n");
    double res;
    /**
     * Extract Geometry from the InputEntity1 value
     */
    OGRGeometryH geometry1;
    map* tmp=getMapFromMaps(inputs,"InputEntity1","value");
    fprintf(stderr,"geometry creation %s \n",tmp->value);
    geometry1=createGeometryFromGML(conf,tmp->value);
    fprintf(stderr,"geometry created %s \n",tmp->value);
    res=OGR_G_GetArea(geometry1);
    fprintf(stderr,"area %d \n",res);
    /**
     * Creating the outputs
     */
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="Area";
    char tmp1[100];
    sprintf(tmp1,"%d",res);
    outputs->content=createMap("value",tmp1);
    addMapToMap(&outputs->content,createMap("datatype","float"));
    outputs->next=NULL;
#ifdef DEBUG
    dumpMaps(outputs);
#endif
    return SERVICE_SUCCEEDED;
  }

}
