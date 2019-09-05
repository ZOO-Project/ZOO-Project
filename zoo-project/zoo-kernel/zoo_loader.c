/*
 * Author : Gérald FENOY
 *
 *  Copyright 2008-2011 GeoLabs SARL. All rights reserved.
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

#define MALLOC_CHECK_ 0
#define MALLOC_CHECK 0

/**
 * Specific includes
 */
#include "fcgio.h"
#include "fcgi_config.h" 
#include "fcgi_stdio.h"
#include <sys/types.h>
#include <unistd.h>
#include "service_internal.h"
#include "response_print.h"

extern "C" {
#include "cgic.h"
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}

#ifdef WIN32
#include "windows.h"
#define strtok_r strtok_s
#endif

#include "service_internal.h"
#include "request_parser.h"

int runRequest(map**);

using namespace std;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE -1
#endif

int cgiInit(){
  fprintf(FCGI_stderr,"ZOO-Kernel initialization %s %d ... \n",__FILE__,__LINE__);
  fflush(FCGI_stderr);
  return 0;
}

/**
 * Main entry point for cgic.
 * @return 0 on sucess.
 */
int cgiMain(){
  /**
   * We'll use cgiOut as the default output (stdout) to produce plain text 
   * response.
   */
  dup2(fileno(cgiOut),fileno(stdout));
#ifdef DEBUG
  fprintf(cgiOut,"Content-Type: text/plain; charset=utf-8\r\nStatus: 200 OK\r\n\r\n");
  fprintf(cgiOut,"Welcome on ZOO verbose debuging mode \r\n\r\n");
  fflush(cgiOut);
  fprintf (stderr, "Addr:%s\n", cgiRemoteAddr); 
  fprintf (stderr, "RequestMethod: (%s) %d %d\n", cgiRequestMethod,strncasecmp(cgiRequestMethod,"post",4),strncmp(cgiContentType,"text/xml",8)==0 || strncasecmp(cgiRequestMethod,"post",4)==0); 
  fprintf (stderr, "Request: %s\n", cgiQueryString);
  fprintf (stderr, "ContentType: %s\n", cgiContentType);
  fprintf (stderr, "ContentLength: %d\n", cgiContentLength);
  fflush(stderr);
#endif
  
  char *strQuery=NULL;
  if(cgiQueryString!=NULL)
    strQuery=zStrdup(cgiQueryString);
  map* tmpMap=NULL;

  if(strncmp(cgiContentType,"application/json",16)==0 &&
     strncasecmp(cgiRequestMethod,"post",4)==0){
       char *buffer=new char[2];
       char *res=NULL;
       int r=0;
       int len=0;
       while((r=fread(buffer,sizeof(char),1,cgiIn))>0){
	 fprintf(stderr,"%s",buffer);
	 buffer[1]=0;
	 if(res==NULL){
	   res=(char*)malloc(2*sizeof(char));
	   sprintf(res,"%s",buffer);
	 }
	 else{
	   res=(char*)realloc(res,(len+2)*sizeof(char));
	   memcpy(res + len, buffer, sizeof(char));
	   res[len+1]=0;
	 }
	 len+=1;
       }
       delete[] buffer;
       tmpMap=createMap("jrequest",res);
       free(res);
  }else if(strncmp(cgiContentType,"text/xml",8)==0 ||
     strncasecmp(cgiRequestMethod,"post",4)==0){
    if(cgiContentLength==0){

       char *buffer=new char[2];
       char *res=NULL;
       int r=0;
       while((r=fread(buffer,sizeof(char),1,cgiIn))){
	 buffer[1]=0;
	 if(res==NULL){
	   res=(char*)malloc(2*sizeof(char));
	   sprintf(res,"%s",buffer);
	 }
	 else{
	   res=(char*)realloc(res,(cgiContentLength+2)*sizeof(char));
	   memcpy(res + cgiContentLength, buffer, sizeof(char));
	   res[cgiContentLength+1]=0;
	 }
	 cgiContentLength+=r;
       }
       delete[] buffer;
       if(res!=NULL && (strQuery==NULL || strlen(strQuery)==0))
	 tmpMap=createMap("request",res);
       if(res!=NULL)
	 free(res);
    }else{  
      char *buffer=new char[cgiContentLength+1];
      if(fread(buffer,sizeof(char),cgiContentLength,cgiIn)>0){
	buffer[cgiContentLength]=0;
	tmpMap=createMap("request",buffer);
      }else{
	buffer[0]=0;
	char **array, **arrayStep;
	if (cgiFormEntries(&array) != cgiFormSuccess) {
	  return 1;
	}
	arrayStep = array;
	while (*arrayStep) {
	  char *ivalue=new char[cgiContentLength];
	  cgiFormStringNoNewlines(*arrayStep, ivalue, cgiContentLength);
	  char* tmpValueFinal=(char*) malloc((strlen(*arrayStep)+strlen(ivalue)+2)*sizeof(char));	
	  sprintf(tmpValueFinal,"%s=%s",*arrayStep,ivalue);

	  if(strlen(buffer)==0){		
	    sprintf(buffer,"%s",tmpValueFinal);
	  }else{		
	    char *tmp=zStrdup(buffer);
	    sprintf(buffer,"%s&%s",tmp,tmpValueFinal);
	    free(tmp);
	  }	  
	  free(tmpValueFinal);
#ifdef DEBUG
	  fprintf(stderr,"(( \n %s \n %s \n ))",*arrayStep,ivalue);
#endif
	  delete[]ivalue;
	  arrayStep++;
	}	
	if(tmpMap!=NULL)
	  addToMap(tmpMap,"request",buffer);
	else
	  tmpMap=createMap("request",buffer);
      }
      delete[]buffer;
    }	
  }
  else{	  
#ifdef DEBUG
    dumpMap(tmpMap);
#endif
    char **array, **arrayStep;
    if (cgiFormEntries(&array) != cgiFormSuccess) {
      return 1;
    }
    arrayStep = array;
    while (*arrayStep) {
      char *value=new char[cgiContentLength];
      cgiFormStringNoNewlines(*arrayStep, value, cgiContentLength);
#ifdef DEBUG
      fprintf(stderr,"(( \n %s \n %s \n ))",*arrayStep,value);
#endif
      if(tmpMap!=NULL)
        addToMap(tmpMap,*arrayStep,value);
      else
        tmpMap=createMap(*arrayStep,value);
      arrayStep++;
      delete[]value;
    }
    cgiStringArrayFree(array);
  }
 
#ifdef WIN32
  map *tmpReq=getMap(tmpMap,"rfile");  
  if(tmpReq!=NULL){	  	  
    FILE *lf=fopen(tmpReq->value,"r");	
    fseek(lf,0,SEEK_END);
    long flen=ftell(lf);
    fseek(lf,0,SEEK_SET);
    char *buffer=(char*)malloc((flen+1)*sizeof(char));
    fread(buffer,flen,1,lf);
    char *pchr=strrchr(buffer,'>');
    cgiContentLength=strlen(buffer)-strlen(pchr)+1;
    buffer[cgiContentLength]=0;
    fclose(lf);
    addToMap(tmpMap,"request",buffer);
    free(buffer);
  }
#endif
  /**
   * In case that the POST method was used, then check if params came in XML
   * format else try to use the attribute "request" which should be the only 
   * one.
   */
  if(strncasecmp(cgiRequestMethod,"post",4)==0 || 
     (count(tmpMap)==1 && strncmp(tmpMap->value,"<",1)==0) 
#ifdef WIN32
     ||tmpReq!=NULL
#endif
     ){
    /**
     * Store the original XML request in xrequest map
     */
    map* t1=getMap(tmpMap,"request");
    if(t1!=NULL && strncasecmp(t1->value,"<",1)==0) {
      addToMap(tmpMap,"xrequest",t1->value);
      xmlInitParser();
      xmlDocPtr doc = xmlReadMemory(t1->value, cgiContentLength, "input_request.xml", NULL, XML_PARSE_RECOVER);
      {
	xmlXPathObjectPtr reqptr=extractFromDoc(doc,"/*[local-name()='Envelope']/*[local-name()='Body']/*");
	if(reqptr!=NULL){
	  xmlNodeSet* req=reqptr->nodesetval;
	  if(req!=NULL && req->nodeNr==1){
	    addToMap(tmpMap,"soap","true");
	    for(int k=0;k < req->nodeNr;k++){
	      xmlDocSetRootElement(doc, req->nodeTab[k]);
	      xmlChar *xmlbuff;
	      int buffersize;
	      xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, "utf-8", 1);
	      addToMap(tmpMap,"xrequest",(char*)xmlbuff);
	      xmlFree(xmlbuff);
	    }
	  }
	  xmlXPathFreeObject(reqptr);
	}
      }

      xmlNodePtr cur = xmlDocGetRootElement(doc);
      char *tval;
      tval=NULL;
      tval = (char*) xmlGetProp(cur,BAD_CAST "service");
      if(tval!=NULL){
	addToMap(tmpMap,"service",tval);
	xmlFree(tval);
      }
      tval=NULL;
      tval = (char*) xmlGetProp(cur,BAD_CAST "language");
      if(tval!=NULL){
	addToMap(tmpMap,"language",tval);
	xmlFree(tval);
      }
      const char* requests[6]={"GetCapabilities","DescribeProcess","Execute","GetStatus","GetResult","Dismiss"};
      for(int j=0;j<6;j++){
	char tt[128];
	sprintf(tt,"/*[local-name()='%s']",requests[j]);
	xmlXPathObjectPtr reqptr=extractFromDoc(doc,tt);
	if(reqptr!=NULL){
	  xmlNodeSet* req=reqptr->nodesetval;
#ifdef DEBUG
	  fprintf(stderr,"%i",req->nodeNr);
#endif
	  if(req!=NULL && req->nodeNr==1){
	    if(t1->value!=NULL)
	      free(t1->value);
	    t1->value=zStrdup(requests[j]);
	    j=5;
	  }
	  xmlXPathFreeObject(reqptr);
	}
      }
      if(strncasecmp(t1->value,"GetCapabilities",15)==0){
	xmlXPathObjectPtr versptr=extractFromDoc(doc,"/*/*/*[local-name()='Version']");
	xmlNodeSet* vers=versptr->nodesetval;
	if(vers!=NULL && vers->nodeTab!=NULL && vers->nodeTab[0]!=NULL){
	  xmlChar* content=xmlNodeListGetString(doc, vers->nodeTab[0]->xmlChildrenNode,1);
	  addToMap(tmpMap,"version",(char*)content);
	  xmlFree(content);
	}
	if(cur->ns){
	  addToMap(tmpMap,"wps_schemas",(char*)cur->ns->href);
	  int j=0;
	  for(j=0;j<2;j++)
	    if(strncasecmp(schemas[j][2],(char*)cur->ns->href,strlen(schemas[j][2]))==0){
	      char vers[6];
	      sprintf(vers,"%d.0.0",j+1);
	      addToMap(tmpMap,"version",(char*)vers);
	    }
	}
	xmlXPathFreeObject(versptr);
      }else{
	tval=NULL;
	tval = (char*) xmlGetProp(cur,BAD_CAST "version");
	if(tval!=NULL){
	  addToMap(tmpMap,"version",tval);
	  xmlFree(tval);
	}
	tval = (char*) xmlGetProp(cur,BAD_CAST "language");
	if(tval!=NULL){
	  addToMap(tmpMap,"language",tval);
	  xmlFree(tval);
	}
	xmlXPathObjectPtr idptr=extractFromDoc(doc,"/*/*[local-name()='Identifier']");
	if(idptr!=NULL){
	  xmlNodeSet* id=idptr->nodesetval;
	  if(id!=NULL){
	    char* identifiers=NULL;
	    identifiers=(char*)calloc(cgiContentLength,sizeof(char));
	    identifiers[0]=0;
	    for(int k=0;k<id->nodeNr;k++){
	      xmlChar* content=xmlNodeListGetString(doc, id->nodeTab[k]->xmlChildrenNode,1);
	      if(strlen(identifiers)>0){
		char *tmp=zStrdup(identifiers);
		snprintf(identifiers,strlen(tmp)+xmlStrlen(content)+2,"%s,%s",tmp,content);
		free(tmp);
	      }
	      else{
		snprintf(identifiers,xmlStrlen(content)+1,"%s",content);
	      }
	      xmlFree(content);
	    }
	    xmlXPathFreeObject(idptr);
	    if(identifiers[0]!=0)
	      addToMap(tmpMap,"Identifier",identifiers);
	    free(identifiers);
	  }
	}
	if(getMap(tmpMap,"Identifier")==NULL){
	  idptr=extractFromDoc(doc,"/*/*[local-name()='JobID']");
	  if(idptr!=NULL){
	    xmlNodeSet* id=idptr->nodesetval;
	    if(id!=NULL){
	      char* identifiers=NULL;
	      identifiers=(char*)calloc(cgiContentLength,sizeof(char));
	      identifiers[0]=0;
	      for(int k=0;k<id->nodeNr;k++){
		  xmlChar* content=xmlNodeListGetString(doc, id->nodeTab[k]->xmlChildrenNode,1);
		  if(strlen(identifiers)>0){
		    char *tmp=zStrdup(identifiers);
		    snprintf(identifiers,strlen(tmp)+xmlStrlen(content)+2,"%s,%s",tmp,content);
		    free(tmp);
		  }
		  else{
		    snprintf(identifiers,xmlStrlen(content)+1,"%s",content);
		  }
		  xmlFree(content);
	      }
	      xmlXPathFreeObject(idptr);
	      if(identifiers[0]!=0)
		addToMap(tmpMap,"JobID",identifiers);
	      free(identifiers);
	    }
	  }
	}
      }
      xmlFreeDoc(doc);
      xmlCleanupParser();
    }else{
      if(tmpMap!=NULL){	
	if(getMap(tmpMap,"jrequest")==NULL){
	  freeMap(&tmpMap);
	  free(tmpMap);
	  tmpMap=createMap("not_valid","true");
	}
      }else
	tmpMap=createMap("not_valid","true");
    }

    char *token,*saveptr;
    token=strtok_r(cgiQueryString,"&",&saveptr);
    while(token!=NULL){
      char *token1,*saveptr1;
      char *name=NULL;
      char *value=NULL;
      token1=strtok_r(token,"=",&saveptr1);
      while(token1!=NULL){
        if(name==NULL)
          name=zStrdup(token1);
        else
          value=zStrdup(token1);
        token1=strtok_r(NULL,"=",&saveptr1);
      }	  
      //addToMap(tmpMap,name,value);
      /* knut: strtok(_r) ignores delimiter bytes at start and end of string; 
       * it will return non-empty string or NULL, e.g. "metapath=" yields value=NULL.
       * This modification sets value="" instead of NULL.
       */
      addToMap(tmpMap,name, value != NULL ? value : "");
      free(name);
      free(value);
      name=NULL;
      value=NULL;
      token=strtok_r(NULL,"&",&saveptr);
    }
  }
  
  if(strncasecmp(cgiContentType,"multipart/form-data",19)==0){
    map* tmp=getMap(tmpMap,"dataInputs");
    if(tmp!=NULL){
      if(strcasestr(strQuery,"dataInputs=")!=NULL)
	addToMap(tmpMap,"dataInputs",strcasestr(strQuery,"dataInputs=")+11);
      else
	addToMap(tmpMap,"dataInputs","None");
    }
  }

  if(strQuery!=NULL)
    free(strQuery);

  runRequest(&tmpMap);

  if(tmpMap!=NULL){
    freeMap(&tmpMap);
    free(tmpMap);
  }
  return 0;

}
