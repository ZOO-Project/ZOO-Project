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
#include "geos_c.h"
#include "service.h"
#include "service_internal.h"

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

  void printExceptionReportResponse(maps*,map*);
  char *base64(const char *input, int length);
  int errorException(maps *m, const char *message, const char *errorcode);

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
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
    double tolerance;
    map* tmp0=getMapFromMaps(cursor,"Tolerance","value");
    if(tmp0==NULL){
      tolerance=atof("2.0");
    }
    else
      tolerance=atof(tmp0->value);
#ifdef DEBUG
    fprintf(stderr,"Tolerance for Simplify %f",tolerance);
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
    if(tmp1!=NULL){
      if(strncmp(tmp1->value,"text/js",7)==0 ||
	 strncmp(tmp1->value,"application/json",16)==0)
        geometry=OGR_G_CreateGeometryFromJson(tmp->value);
      else
        geometry=createGeometryFromGML(conf,tmp->value);
    }
    else{
      setMapInMaps(conf,"lenv","message",_ss("Unable to find any geometry for InputPolygon"));
      return SERVICE_FAILED;
    }
    if(geometry==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
#ifdef DEBUG
    fprintf(stderr,"Create GEOSGeometry object");
#endif
    GEOSGeometry* ggeometry=((OGRGeometry *) geometry)->exportToGEOS();
    GEOSGeometry* gres=GEOSTopologyPreserveSimplify(ggeometry,tolerance);
    res=(OGRGeometryH)OGRGeometryFactory::createFromGEOS(gres);
    tmp1=getMapFromMaps(outputs,"Result","mimeType");
    if(tmp1!=NULL){
      if(strncmp(tmp1->value,"text/js",7)==0 ||
	 strncmp(tmp1->value,"application/json",16)==0){
	char *tmpS=OGR_G_ExportToJson(res);
	setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
	setMapInMaps(outputs,"Result","mimeType","text/plain");
	setMapInMaps(outputs,"Result","encoding","UTF-8");
	free(tmpS);
#endif
      }
      else{
	char *tmpS=OGR_G_ExportToGML(res);
	setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
	setMapInMaps(outputs,"Result","mimeType","text/xml");
	setMapInMaps(outputs,"Result","encoding","UTF-8");
	setMapInMaps(outputs,"Result","schema","http://fooa/gml/3.1.0/polygon.xsd");
	free(tmpS);
#endif
      }
    }else{
      char *tmpS=OGR_G_ExportToJson(res);
      setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
      setMapInMaps(outputs,"Result","mimeType","text/plain");
      setMapInMaps(outputs,"Result","encoding","UTF-8");
      free(tmpS);
#endif
    }
    outputs->next=NULL;
    //GEOSFree(ggeometry);
    //GEOSFree(gres);
    OGR_G_DestroyGeometry(res);
    OGR_G_DestroyGeometry(geometry);
    return SERVICE_SUCCEEDED;
  }


  int applyOne(maps*& conf,maps*& inputs,maps*& outputs,OGRGeometryH (*myFunc)(OGRGeometryH),char* schema){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print\n");
#endif
    maps* cursor=inputs;
    OGRGeometryH geometry,res;
#ifdef DEBUG
    dumpMaps(cursor);
#endif
    map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
    if(!tmp){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
#ifdef DEBUG
    fprintf(stderr,"Service internal print \n");
    dumpMaps(inputs);
    fprintf(stderr,"/Service internal print \n");
#endif
    map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
#ifdef DEBUG
    fprintf(stderr,"Service internal print \n");
    dumpMap(tmp1);
    fprintf(stderr,"/Service internal print \n");
#endif
    if(tmp1!=NULL){
      if(strncmp(tmp1->value,"text/js",7)==0 ||
	 strncmp(tmp1->value,"application/json",7)==0)
        geometry=OGR_G_CreateGeometryFromJson(tmp->value);
      else
        geometry=createGeometryFromGML(conf,tmp->value);
    }
    else
      geometry=createGeometryFromGML(conf,tmp->value);
    if(geometry==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse the input geometry from InputPolygon"));
      return SERVICE_FAILED;
    }
    res=(*myFunc)(geometry);
#ifdef DEBUG
    fprintf(stderr,"Service internal print \n");
    dumpMaps(outputs);
    fprintf(stderr,"/Service internal print \n");
#endif
    map *tmp_2=getMapFromMaps(outputs,"Result","mimeType");
#ifdef DEBUG
    fprintf(stderr,"Service internal print \n");
    dumpMap(tmp_2);
    fprintf(stderr,"/Service internal print \n");
#endif
    if(tmp_2!=NULL){
      if(strncmp(tmp_2->value,"text/js",7)==0 ||
	 strncmp(tmp_2->value,"application/json",16)==0){
	char *tmpS=OGR_G_ExportToJson(res);
	setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
	setMapInMaps(outputs,"Result","mimeType","text/plain");
	setMapInMaps(outputs,"Result","encoding","UTF-8");
	free(tmpS);
#endif
      }
      else{
	char *tmpS=OGR_G_ExportToGML(res);
	setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
	setMapInMaps(outputs,"Result","mimeType","text/xml");
	setMapInMaps(outputs,"Result","encoding","UTF-8");
	setMapInMaps(outputs,"Result","schema",schema);
	free(tmpS);
#endif
      }
    }else{
      char *tmpS=OGR_G_ExportToJson(res);
      setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
      setMapInMaps(outputs,"Result","mimeType","text/plain");
      setMapInMaps(outputs,"Result","encoding","UTF-8");
      free(tmpS);
#endif
    }
    //outputs->next=NULL;
#ifdef DEBUG
    dumpMaps(outputs);
    fprintf(stderr,"\nService internal print\n===\n");
#endif
    OGR_G_DestroyGeometry(res);
    OGR_G_DestroyGeometry(geometry);
    //CPLFree(res);
    //CPLFree(geometry);
#ifdef DEBUG
    fprintf(stderr,"Service internal print \n");
    dumpMaps(outputs);
    fprintf(stderr,"/Service internal print \n");
#endif
    return SERVICE_SUCCEEDED;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
int Buffer(maps*& conf,maps*& inputs,maps*& outputs){
   OGRGeometryH geometry,res;
   map* tmp=getMapFromMaps(inputs,"InputPolygon","value");
   if(tmp==NULL){
     setMapInMaps(conf,"lenv","message",_ss("Unable to fetch input geometry"));
     return SERVICE_FAILED;
   }else
     if(strlen(tmp->value)<=0){
       setMapInMaps(conf,"lenv","message",_ss("Unable to fetch input geometry"));
       return SERVICE_FAILED;
     }
   map* tmp1=getMapFromMaps(inputs,"InputPolygon","mimeType");
   if(strncmp(tmp1->value,"application/json",16)==0)
     geometry=OGR_G_CreateGeometryFromJson(tmp->value);
   else
     geometry=createGeometryFromGML(conf,tmp->value);
   if(geometry==NULL){
     setMapInMaps(conf,"lenv","message",_ss("Unable to parse input geometry"));
     return SERVICE_FAILED;
   }
   double bufferDistance;
   tmp=getMapFromMaps(inputs,"BufferDistance","value");
   if(tmp==NULL){
     bufferDistance=atof("10.0");
   }
   else
     bufferDistance=atof(tmp->value);
   res=OGR_G_Buffer(geometry,bufferDistance,30);
   dumpMap(tmp);
   tmp1=getMapFromMaps(outputs,"Result","mimeType");
   dumpMap(tmp);
   if(strncmp(tmp1->value,"application/json",16)==0){
     char *tmpS=OGR_G_ExportToJson(res);
     setMapInMaps(outputs,"Result","value",tmpS);
     dumpMap(tmp);
#ifndef WIN32
     setMapInMaps(outputs,"Result","mimeType","text/plain");
     setMapInMaps(outputs,"Result","encoding","UTF-8");
     free(tmpS);
#endif
   }
   else{
     char *tmpS=OGR_G_ExportToGML(res);
     setMapInMaps(outputs,"Result","value",tmpS);
     dumpMap(tmp);
#ifndef WIN32
     free(tmpS);
     setMapInMaps(outputs,"Result","mimeType","text/xml");
     setMapInMaps(outputs,"Result","encoding","UTF-8");
     setMapInMaps(outputs,"Result","schema","http://fooa/gml/3.1.0/polygon.xsd");
#endif
   }
   //outputs->next=NULL;
   OGR_G_DestroyGeometry(geometry);
   OGR_G_DestroyGeometry(res);
   return SERVICE_SUCCEEDED;
}

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Boundary(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOne(conf,inputs,outputs,&OGR_G_GetBoundary,"http://fooa/gml/3.1.0/polygon.xsd");
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int ConvexHull(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOne(conf,inputs,outputs,&OGR_G_ConvexHull,"http://fooa/gml/3.1.0/polygon.xsd");
  }


  OGRGeometryH MY_OGR_G_Centroid(OGRGeometryH hTarget){
    OGRGeometryH res;
    res=OGR_G_CreateGeometryFromJson("{\"type\": \"Point\", \"coordinates\": [0,0] }");
    OGRwkbGeometryType gtype=OGR_G_GetGeometryType(hTarget);
    if(gtype!=wkbPolygon){
      hTarget=OGR_G_ConvexHull(hTarget);
    }
    int c=OGR_G_Centroid(hTarget,res);
    return res;
  }

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Centroid(maps*& conf,maps*& inputs,maps*& outputs){
    return applyOne(conf,inputs,outputs,&MY_OGR_G_Centroid,"http://fooa/gml/3.1.0/point.xsd");
  }

  int applyTwo(maps*& conf,maps*& inputs,maps*& outputs,OGRGeometryH (*myFunc)(OGRGeometryH,OGRGeometryH)){
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1\n");
    fflush(stderr);
    fprintf(stderr,"\nService internal print1\n");
    dumpMaps(inputs);
    fprintf(stderr,"\nService internal print1\n");
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
    if(geometry1==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse input geometry for InputEntity1."));
#ifdef DEBUG
      fprintf(stderr,"SERVICE FAILED !\n");
#endif
      return SERVICE_FAILED;
    }
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1 InputEntity1\n");
#endif
    {
      map* tmp=getMapFromMaps(inputs,"InputEntity2","value");
      map* tmp1=getMapFromMaps(inputs,"InputEntity2","mimeType");
#ifdef DEBUG
      fprintf(stderr,"MY MAP \n[%s] - %i\n",tmp1->value,strncmp(tmp1->value,"application/json",16));
      //dumpMap(tmp);
      fprintf(stderr,"MY MAP\n");
      fprintf(stderr,"\nService internal print1 InputEntity2\n");
#endif
      if(tmp1!=NULL){
        if(strncmp(tmp1->value,"application/json",16)==0){
#ifdef DEBUG
	  fprintf(stderr,"\nService internal print1 InputEntity2 as JSON\n");
#endif
      	  geometry2=OGR_G_CreateGeometryFromJson(tmp->value);
	}
	else{
#ifdef DEBUG
	  fprintf(stderr,"\nService internal print1 InputEntity2 as GML\n");
#endif
	  geometry2=createGeometryFromGML(conf,tmp->value);
	}
      }
      else
      	geometry2=createGeometryFromGML(conf,tmp->value);
#ifdef DEBUG
      fprintf(stderr,"\nService internal print1 InputEntity2 PreFinal\n");
#endif
    }
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1 InputEntity2 Final\n");
#endif
    if(geometry2==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse input geometry for InputEntity2."));
#ifdef DEBUG
      fprintf(stderr,"SERVICE FAILED !\n");
#endif
      return SERVICE_FAILED;
    }
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1\n");
#endif
    res=(*myFunc)(geometry1,geometry2);
#ifdef DEBUG
    fprintf(stderr,"\nService internal print1\n");
#endif    
    /* nuova parte */
    map* tmp2=getMapFromMaps(outputs,"Result","mimeType");
    if(strncmp(tmp2->value,"application/json",16)==0){
      char *tmpS=OGR_G_ExportToJson(res);
      setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
      setMapInMaps(outputs,"Result","mimeType","text/plain");
      setMapInMaps(outputs,"Result","encoding","UTF-8");
      free(tmpS);
#endif
    }
    else{
      char *tmpS=OGR_G_ExportToGML(res);
      setMapInMaps(outputs,"Result","value",tmpS);
#ifndef WIN32
      setMapInMaps(outputs,"Result","mimeType","text/xml");
      setMapInMaps(outputs,"Result","encoding","UTF-8");
      setMapInMaps(outputs,"Result","schema","http://fooa/gml/3.1.0/polygon.xsd");
      free(tmpS);
#endif
    }
    
    /* vecchia da togliere */
    /*
    char *tmpS=OGR_G_ExportToJson(res);
    setMapInMaps(outputs,"Result","value",tmpS);
    setMapInMaps(outputs,"Result","mimeType","text/plain");
    setMapInMaps(outputs,"Result","encoding","UTF-8");
    free(tmpS);
    */
    OGR_G_DestroyGeometry(geometry1);
    OGR_G_DestroyGeometry(geometry2);
    OGR_G_DestroyGeometry(res);
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

#ifdef WIN32
  __declspec(dllexport)
#endif
  int Union(maps*& conf,maps*& inputs,maps*& outputs){
    return applyTwo(conf,inputs,outputs,&OGR_G_Union);
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
    res=OGR_G_GetArea(geometry);
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
