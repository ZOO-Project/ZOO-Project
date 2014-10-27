/**
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
#ifndef WIN32
#include "fcgio.h"
#include "fcgi_config.h" 
#include "fcgi_stdio.h"
#endif
#include <sys/types.h>
#include <unistd.h>
#include "service_internal.h"
#ifdef WIN32
#include "windows.h"
#define strtok_r strtok_s
#endif

extern "C" {
#include "cgic.h"
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}

#include "service_internal.h"

xmlXPathObjectPtr extractFromDoc(xmlDocPtr,const char*);
int runRequest(map**);

using namespace std;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE -1
#endif

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

  if(strncmp(cgiContentType,"text/xml",8)==0 || 
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
       if(res==NULL && (strQuery==NULL || strlen(strQuery)==0)){
	 return errorException(NULL,"ZOO-Kernel failed to process your request cause the request was emtpty.","InternalError",NULL);
       }else{
	 if(strQuery==NULL || strlen(strQuery)==0)
	   tmpMap=createMap("request",res);
       }
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
	  char* tmpValueFinal=(char*) malloc((strlen(*arrayStep)+strlen(ivalue)+1)*sizeof(char));
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
    fclose(lf);
    addToMap(tmpMap,"request",buffer);
    free(buffer);
    cgiContentLength=flen+9;
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
    if(t1!=NULL && strncasecmp(t1->value,"<",1)==0){
      addToMap(tmpMap,"xrequest",t1->value);
      xmlInitParser();
      xmlDocPtr doc = xmlParseMemory(t1->value,cgiContentLength);
      {
	xmlXPathObjectPtr reqptr=extractFromDoc(doc,"/*[local-name()='Envelope']/*[local-name()='Body']/*");
	if(reqptr!=NULL){
	  xmlNodeSet* req=reqptr->nodesetval;
	  if(req!=NULL && req->nodeNr==1){
	    addToMap(tmpMap,"soap","true");
	    for(int k=0;k < req->nodeNr;k++){
	      //xmlNsPtr ns=xmlNewNs(req->nodeTab[k],BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
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
      const char* requests[3]={"GetCapabilities","DescribeProcess","Execute"};
      for(int j=0;j<3;j++){
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
	    j=2;
	  }
	  xmlXPathFreeObject(reqptr);
	}
      }
      if(strncasecmp(t1->value,"GetCapabilities",15)==0){
	xmlXPathObjectPtr versptr=extractFromDoc(doc,"/*/*/*[local-name()='Version']");
	xmlNodeSet* vers=versptr->nodesetval;
	xmlChar* content=xmlNodeListGetString(doc, vers->nodeTab[0]->xmlChildrenNode,1);
	addToMap(tmpMap,"version",(char*)content);
	xmlXPathFreeObject(versptr);
	xmlFree(content);
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
	    addToMap(tmpMap,"Identifier",identifiers);
	    free(identifiers);
	  }
	}
      }
      xmlFreeDoc(doc);
      xmlCleanupParser();
    }else{
      freeMap(&tmpMap);
      free(tmpMap);
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
      addToMap(tmpMap,name,value);
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
      addToMap(tmpMap,"dataInputs",strstr(strQuery,"dataInputs=")+11);
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
