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
#include "fcgio.h"
#include "fcgi_config.h" 
#include "fcgi_stdio.h"
#include <sys/types.h>
#include <unistd.h>
#include "service_internal.h"

extern "C" {
#include "cgic.h"
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}

xmlXPathObjectPtr extractFromDoc(xmlDocPtr,char*);
int runRequest(map*);

using namespace std;

/* ************************************************************************* */

int errorException(maps *m, const char *message, const char *errorcode) 
{
  map* errormap = createMap("text", message);
  addToMap(errormap,"code", errorcode);
  printExceptionReportResponse(m,errormap);
  freeMap(&errormap);
  free(errormap);
  return -1;
}

/* ************************************************************************* */

#ifndef STRTOK_R
char *
strtok_r (char *s1, const char *s2, char **lasts)
{
  char *ret;

  if (s1 == NULL)
    s1 = *lasts;
  while (*s1 && strchr(s2, *s1))
    ++s1;
  if (*s1 == '\0')
    return NULL;
  ret = s1;
  while (*s1 && !strchr(s2, *s1))
    ++s1;
  if (*s1)
    *s1++ = '\0';
  *lasts = s1;
  return ret;
}

#endif

#define TRUE 1
#define FALSE -1

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
#endif
  
#ifdef DEBUG
  fprintf (stderr, "Addr:%s\n", cgiRemoteAddr); 
  fprintf (stderr, "RequestMethod:%s\n", cgiRequestMethod); 
  fprintf (stderr, "Request: %s\n", cgiQueryString);
#endif

  map* tmpMap=NULL;

  if(strncmp(cgiContentType,"text/xml",8)==0){
    char *buffer=new char[cgiContentLength+1];
    if(fread(buffer,1,cgiContentLength,cgiIn)){
      buffer[cgiContentLength]=0;
      tmpMap=createMap("request",buffer);
    }else{
      /* Here we have to return an error message ... */
      fprintf(stderr, "Unable to read cgi content in zoo_loader.c line %i\n", __LINE__);
      return 1; 
    }
    delete[]buffer;
  }
  else{
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

  if(strncasecmp(cgiRequestMethod,"post",4)==0 && getMap(tmpMap,"request")==NULL){
    char *tmpKey=strdup(tmpMap->name);
    char *tmpValue=strdup(tmpMap->value);
    freeMap(&tmpMap);
    free(tmpMap);
    char* tmpValueFinal=(char*) malloc((strlen(tmpKey)+strlen(tmpValue)+2)*sizeof(char));
    sprintf(tmpValueFinal,"%s=%s",tmpKey,tmpValue);
    tmpMap=createMap("request",tmpValueFinal);
    free(tmpValueFinal);
  }
  /**
   * In case that the POST method was used, then check if params came in XML
   * format else try to use the attribute "request" which should be the only 
   * one.
   */
  if(strncasecmp(cgiRequestMethod,"post",4)==0 || 
     (count(tmpMap)==1 && strncmp(tmpMap->value,"<",1)==0)){
    /**
     * First include the MetaPath and the ServiceProvider default parameters
     * (which should be always available in GET params so in cgiQueryString)
     */
    char *str1;
    str1=cgiQueryString;
    /**
     * Store the original XML request in xrequest map
     */
    map* t1=getMap(tmpMap,"request");
    if(t1!=NULL){
      addToMap(tmpMap,"xrequest",t1->value);
      xmlInitParser();
      xmlDocPtr doc = xmlParseMemory(t1->value,cgiContentLength);
      xmlNodePtr cur = xmlDocGetRootElement(doc);
      char *tval;
      tval=NULL;
      tval = (char*) xmlGetProp(cur,BAD_CAST "service");
      if(tval!=NULL)
	addToMap(tmpMap,"service",tval);
      tval=NULL;
      tval = (char*) xmlGetProp(cur,BAD_CAST "language");
      if(tval!=NULL)
	addToMap(tmpMap,"language",tval);
      
      char* requests[3];
      requests[0]="GetCapabilities";
      requests[1]="DescribeProcess";
      requests[2]="Execute";
      for(int j=0;j<3;j++){
	char tt[35];
	sprintf(tt,"/*[local-name()='%s']",requests[j]);
	xmlXPathObjectPtr reqptr=extractFromDoc(doc,tt);
	if(reqptr!=NULL){
	  xmlNodeSet* req=reqptr->nodesetval;
#ifdef DEBUG
	  fprintf(stderr,"%i",req->nodeNr);
#endif
	  if(req!=NULL && req->nodeNr==1){
	    t1->value=requests[j];
	    j=2;
	  }
	  xmlXPathFreeObject(reqptr);
	}
	//xmlFree(req);
      }
      if(strncasecmp(t1->value,"GetCapabilities",15)==0){
	xmlXPathObjectPtr versptr=extractFromDoc(doc,"/*/*/*[local-name()='Version']");
	xmlNodeSet* vers=versptr->nodesetval;
	xmlChar* content=xmlNodeListGetString(doc, vers->nodeTab[0]->xmlChildrenNode,1);
	addToMap(tmpMap,"version",(char*)content);
	xmlXPathFreeObject(versptr);
	//xmlFree(vers);
	xmlFree(content);
      }else{
	tval=NULL;
	tval = (char*) xmlGetProp(cur,BAD_CAST "version");
	if(tval!=NULL)
	  addToMap(tmpMap,"version",tval);
	xmlFree(tval);
	tval = (char*) xmlGetProp(cur,BAD_CAST "language");
	if(tval!=NULL)
	  addToMap(tmpMap,"language",tval);
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
		char *tmp=strdup(identifiers);
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
	//xmlFree(id);
      }
      xmlFree(tval);
      xmlFreeDoc(doc);
      xmlCleanupParser();
    }
  }

  runRequest(tmpMap);

  /** 
   * Required but can't be made after executing a process using POST requests.
   */
  if(strncasecmp(cgiRequestMethod,"post",4)!=0 && count(tmpMap)!=1 && tmpMap!=NULL){
    freeMap(&tmpMap);
    free(tmpMap);
  }
  return 0;

}
