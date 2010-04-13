/**
 * Author : Gérald FENOY
 *
 *  Copyright 2008-2009 GeoLabs SARL. All rights reserved.
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

/**
 * Specific includes
 */
#include "fcgio.h"
#include "fcgi_config.h" 
#include "fcgi_stdio.h"
#include <sys/types.h>
#include <unistd.h>
extern "C" {
#include "cgic.h"
}

#include "service_internal.h"
extern "C" {
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}

#ifndef STRTOK_R
char *
strtok_r(char *s1, const char *s2, char **lasts)
{
  char *ret;

  if (s1 == NULL)
    s1 = *lasts;
  while(*s1 && strchr(s2, *s1))
    ++s1;
  if(*s1 == '\0')
    return NULL;
  ret = s1;
  while(*s1 && !strchr(s2, *s1))
    ++s1;
  if(*s1)
    *s1++ = '\0';
  *lasts = s1;
  return ret;
}

#endif

xmlNodeSet* extractFromDoc(xmlDocPtr,char*);
int runRequest(map*);

using namespace std;

char* strtoupper(char* str)
{
  int leng=strlen(str);
  for(int i=0; i<leng; i++)
    if (97<=str[i]&&str[i]<=122)//a-z
      str[i]-=32;
  return str;
}

char* strtolower(char* str)
{
  int leng=strlen(str);
  for(int i=0; i<leng; i++)
    if (65<=str[i]&&str[i]<=90)//A-Z
      str[i]+=32;
  return str;
}

char* remplace(char* delim,char* rep,char* source){
  if(strcmp(source,"")==0){
#ifdef DEBUG
    //char *tmp="RETURN NULL !\n";
    //printf((void*)tmp);
#endif
    return "NULL";
  }

  char *_token; 
  char *origin=strdup(source);
  char result[1024]="";
  _token = strtok(source, delim);
#ifdef DEBUG
  fprintf(stderr,"\nREPLACE TOKEN (%s == %s => %d)\n ",_token,origin,strcmp(_token,origin));
#endif
  if(strcmp(_token,origin)!=0)
    while(_token!=NULL){
      sprintf(result,"%s%s%s",result,_token,rep);
      _token = strtok (NULL, delim);
      if(_token==NULL)
	result[strlen(result)-strlen(rep)]=0;
#ifdef DEBUG
      fprintf(stderr,"\n\nRESULT(%s)\n\n",result);
#endif
    }
  else
    return origin;
  
  return strdup(result);
}


char *colors[] = {
  "red", "green", "blue"
};

#define colorsTotal 3
#define TRUE -1
#define FALSE -1
static bool started=FALSE;
static bool isStarted=FALSE;

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
  
  /**
   * Read the main configuration file
   */
  if(!isStarted){
    maps* m;
    m=(maps*)malloc(sizeof(maps*));  
    conf_read("main.cfg",m);
#ifdef REAL_FCGI
#ifdef DEBUG
    printf("ok passed");
#endif    
#endif    
    isStarted=TRUE;
  }

#ifdef DEBUG
  char textb[23];
  sprintf(textb,"\n%s\n%s\n",cgiRemoteAddr,cgiRequestMethod);
  fprintf(stderr,"****%s\n%s\n***",textb,cgiQueryString);
#endif

  map* tmp=NULL;
    
  if(strncmp(cgiContentType,"text/xml",8)==0){
    char *buffer=new char[cgiContentLength];
    if(fread(buffer,1,cgiContentLength,cgiIn)){
      buffer[cgiContentLength]=0;
      tmp=createMap("request",buffer);
    }else{
      /* Here we have to return an error message ... */
      return 1; 
    }
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
      if(tmp!=NULL)
	addToMap(tmp,*arrayStep,value);
      else
	tmp=createMap(*arrayStep,value);
      arrayStep++;
    }
    cgiStringArrayFree(array);
  }

  /**
   * In case that the POST method was used, then check if params came in XML
   * format (attribute request should be the once).
   */
  if(strcmp(cgiRequestMethod,mtoupper("post"))==0 && count(tmp)==1){
    /**
     * First include the MetaPath and the ServiceProvider default parameters
     * (which should be always available in GET params so in cgiQueryString)
     */
    char *saveptr1, *saveptr2;
    char *str1, *str2, *token, *subtoken;
    str1=cgiQueryString;
    token=strtok_r(str1,"&",&saveptr1);
    while(token!=NULL){
#ifdef DEBUG
      fprintf(stderr,"%s",token);
#endif
      str2=token;
      subtoken=strtok_r(str2,"=",&saveptr2);
#ifdef DEBUG
      fprintf(stderr,"%s\n",subtoken);
#endif
      char* tmp_name;
      if(subtoken!=NULL){
	tmp_name=subtoken;
#ifdef DEBUG
	fprintf(stderr,"%s",subtoken);
#endif
	subtoken=strtok_r(NULL,"=",&saveptr2);
	if(subtoken!=NULL)
	  addToMap(tmp,tmp_name,subtoken);
	else
	  addToMap(tmp,tmp_name,"");
      }
      token=strtok_r(NULL,"&",&saveptr1);
    }
    /**
     * Store the original XML request in xrequest map
     */
    map* t1=getMap(tmp,"request");
    if(t1!=NULL){
      addToMap(tmp,"xrequest",t1->value);
      xmlInitParser();
      xmlDocPtr doc = xmlParseMemory(t1->value,cgiContentLength);      
      xmlNodePtr cur = xmlDocGetRootElement(doc);
      char *tval;
      tval=NULL;
      tval = (char*) xmlGetProp(cur,BAD_CAST "service");
      if(tval!=NULL)
	addToMap(tmp,"service",tval);
      tval=NULL;
      tval = (char*) xmlGetProp(cur,BAD_CAST "language");
      if(tval!=NULL)
	addToMap(tmp,"language",tval);
      
      char* requests[3];
      requests[0]="GetCapabilities";
      requests[1]="DescribeProcess";
      requests[2]="Execute";
      for(int j=0;j<3;j++){
	char tt[35];
	sprintf(tt,"/*[local-name()='%s']",requests[j]);
	xmlNodeSet* req=extractFromDoc(doc,tt);
#ifdef DEBUG
	fprintf(stderr,"%i",req->nodeNr);
#endif
	if(req->nodeNr==1){
	  t1->value=requests[j];
	  j=2;
	}
	xmlFree(req);
      }
      if(strcmp(mtoupper(t1->value),mtoupper("GetCapabilities"))==0){	
	xmlNodeSet* vers=extractFromDoc(doc,"/*/*/*[local-name()='Version']");
	xmlChar* content=xmlNodeListGetString(doc, vers->nodeTab[0]->xmlChildrenNode,1);
	addToMap(tmp,"version",(char*)content);
	xmlFree(vers);
	xmlFree(content);
      }else{
	tval=NULL;
	tval = (char*) xmlGetProp(cur,BAD_CAST "version");
	if(tval!=NULL)
	  addToMap(tmp,"version",tval);
	xmlFree(tval);
	tval = (char*) xmlGetProp(cur,BAD_CAST "language");
	if(tval!=NULL)
	  addToMap(tmp,"language",tval);
	xmlNodeSet* id=extractFromDoc(doc,"/*/*[local-name()='Identifier']");
	char* identifiers=NULL;
	identifiers=(char*)malloc(cgiContentLength);
	identifiers[0]=0;
	for(int k=0;k<id->nodeNr;k++){
	  xmlChar* content=xmlNodeListGetString(doc, id->nodeTab[k]->xmlChildrenNode,1);
	  if(strlen(identifiers)>0){
	    sprintf(identifiers,"%s,%s",identifiers,content);
	    identifiers[strlen(identifiers)]=0;
	  }
	  else{
	    sprintf(identifiers,"%s",content);
	    identifiers[strlen(identifiers)]=0;
	  }
	  xmlFree(content);
	}
	xmlFree(id);
	addToMap(tmp,"Identifier",identifiers);
      }
      //xmlCleanupParser();
      xmlFree(tval);
    }
  }
  //dumpMap(tmp);
  runRequest(tmp);
  //dumpMap(tmp);

  return 0;

}
