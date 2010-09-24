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

#define length(x) (sizeof(x) / sizeof(x[0]))

extern "C" int yylex();
extern "C" int crlex();


extern "C" {
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}

#include "cgic.h"
#include "ulinet.h"

#include <string.h>

#include "service.h"
#include "service_internal.h"


#ifdef USE_PYTHON
#include "service_internal_python.h"
#endif

#ifdef USE_JAVA
#include "service_internal_java.h"
#endif

#ifdef USE_PHP
#include "service_internal_php.h"
#endif

#ifdef USE_JS
#include "service_internal_js.h"
#endif

#ifdef USE_PERL
#include "service_internal_perl.h"
#endif



#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#ifndef WIN32
#include <dlfcn.h>
#include <libgen.h>
#else
#include <windows.h>
#include <direct.h>
#endif
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

xmlXPathObjectPtr extractFromDoc(xmlDocPtr doc,char* search){
  xmlXPathContextPtr xpathCtx;
  xmlXPathObjectPtr xpathObj;
  xpathCtx = xmlXPathNewContext(doc);
  xpathObj = xmlXPathEvalExpression(BAD_CAST search,xpathCtx);
  xmlXPathFreeContext(xpathCtx);
  return xpathObj;
}

void sig_handler(int sig){
  char tmp[100];
  char *ssig;
  switch(sig){
  case SIGSEGV:
    ssig="SIGSEGV";
    break;
  case SIGTERM:
    ssig="SIGTERM";
    break;
  case SIGINT:
    ssig="SIGINT";
    break;
  case SIGILL:
    ssig="SIGILL";
    break;
  case SIGFPE:
    ssig="SIGFPE";
    break;
  case SIGABRT:
    ssig="SIGABRT";
    break;
  default:
    ssig="UNKNOWN";
    break;
  }
  sprintf(tmp,"ZOO Kernel failed to process your request receiving signal %d = %s",sig,ssig);
  errorException(NULL, tmp, "InternalError");
#ifdef DEBUG
  fprintf(stderr,"Not this time!\n");
#endif
  exit(0);
}

int runRequest(map* request_inputs)
{

  (void) signal(SIGSEGV,sig_handler);
  (void) signal(SIGTERM,sig_handler);
  (void) signal(SIGINT,sig_handler);
  (void) signal(SIGILL,sig_handler);
  (void) signal(SIGFPE,sig_handler);
  (void) signal(SIGABRT,sig_handler);

  map* r_inputs=NULL,*tmps=NULL;
  maps* m=NULL;
  int argc=count(request_inputs);

  char* REQUEST=NULL;
  /**
   * Parsing service specfic configuration file
   */
  m=(maps*)calloc(1,MAPS_SIZE);
  if(m == NULL){
    return errorException(m, "Unable to allocate memory.", "InternalError");
  }
  char ntmp[1024];
#ifndef WIN32
  getcwd(ntmp,1024);
#else
  _getcwd(ntmp,1024);
#endif
  r_inputs=getMap(request_inputs,"metapath");
  if(r_inputs==NULL){
    if(request_inputs==NULL)
      request_inputs=createMap("metapath","");
    else
      addToMap(request_inputs,"metapath","");
#ifdef DEBUG
    fprintf(stderr,"ADD METAPATH\n");
    dumpMap(request_inputs);
#endif
    r_inputs=getMap(request_inputs,"metapath");
  }
  char conf_file[10240];
  snprintf(conf_file,10240,"%s/%s/main.cfg",ntmp,r_inputs->value);
  conf_read(conf_file,m);
#ifdef DEBUG
  fprintf(stderr, "***** BEGIN MAPS\n"); 
  dumpMaps(m);
  fprintf(stderr, "***** END MAPS\n");
#endif

  /**
   * Check for minimum inputs
   */
  r_inputs=getMap(request_inputs,"Request");
  if(request_inputs==NULL || r_inputs==NULL){ 
    errorException(m, "Parameter <request> was not specified","MissingParameterValue");
    freeMaps(&m);
    free(m);
    freeMap(&request_inputs);
    free(request_inputs);
    return 1;
  }
  else{
    REQUEST=strdup(r_inputs->value);
    if(strncasecmp(r_inputs->value,"GetCapabilities",15)!=0
       && strncasecmp(r_inputs->value,"DescribeProcess",15)!=0
       && strncasecmp(r_inputs->value,"Execute",7)!=0){ 
      errorException(m, "Unenderstood <request> value. Please check that it was set to GetCapabilities, DescribeProcess or Execute.", "InvalidParameterValue");
      freeMaps(&m);
      free(m);
      freeMap(&request_inputs);
      free(request_inputs);
      free(REQUEST);
      return 1;
    }
  }
  r_inputs=NULL;
  r_inputs=getMap(request_inputs,"Service");
  if(r_inputs==NULLMAP){
    errorException(m, "Parameter <service> was not specified","MissingParameterValue");
    freeMaps(&m);
    free(m);
    free(REQUEST);
    return 1;
  }
  if(strncasecmp(REQUEST,"GetCapabilities",15)!=0){
    r_inputs=getMap(request_inputs,"Version");
    if(r_inputs==NULL){ 
      errorException(m, "Parameter <version> was not specified","MissingParameterValue");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      return 1;
    }
  }

  r_inputs=getMap(request_inputs,"serviceprovider");
  if(r_inputs==NULL){
    addToMap(request_inputs,"serviceprovider","");
  }

  map* outputs=NULL;
  maps* request_output_real_format=NULL;
  map* tmpm=getMapFromMaps(m,"main","serverAddress");
  if(tmpm!=NULL)
    SERVICE_URL=strdup(tmpm->value);
  else
    SERVICE_URL=DEFAULT_SERVICE_URL;

  service* s[100];
  service* s1;
  int scount=0;

#ifdef DEBUG
  dumpMap(r_inputs);
#endif
  char conf_dir[1024];
  int t;
  char tmps1[1024];

  r_inputs=NULL;
  r_inputs=getMap(request_inputs,"metapath");
  if(r_inputs!=NULL)
    snprintf(conf_dir,1024,"%s/%s",ntmp,r_inputs->value);
  else
    snprintf(conf_dir,1024,"%s",ntmp);

  if(strncasecmp(REQUEST,"GetCapabilities",15)==0){
    int i=0;
    struct dirent *dp;
#ifdef DEBUG
    dumpMap(r_inputs);
#endif
    DIR *dirp = opendir(conf_dir);
    if(dirp==NULL){
      return errorException(m, "The specified path doesn't exist.","InvalidParameterValue");
    }
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    r_inputs=NULL;
    r_inputs=getMap(request_inputs,"ServiceProvider");
    xmlNodePtr n;
    //dumpMap(request_inputs);
    if(r_inputs!=NULL)
      n = printGetCapabilitiesHeader(doc,r_inputs->value,m);
    else
      n = printGetCapabilitiesHeader(doc,"",m);
    /**
     * Strange, here we need to close stdout to ensure that no uneeded 
     * char will be printed (parser issue ?)
     */
    int saved_stdout = dup(fileno(stdout));
    dup2(fileno(stderr),fileno(stdout));
    while ((dp = readdir(dirp)) != NULL)
      if(strstr(dp->d_name,".zcfg")!=0){
	memset(tmps1,0,1024);
	snprintf(tmps1,1024,"%s/%s",conf_dir,dp->d_name);
	s1=(service*)calloc(1,SERVICE_SIZE);
	if(s1 == NULL){ 
	  return errorException(m, "Unable to allocate memory.","InternalError");
	}
#ifdef DEBUG
	fprintf(stderr,"#################\n%s\n#################\n",tmps1);
#endif
	t=getServiceFromFile(tmps1,&s1);
#ifdef DEBUG
	dumpService(s1);
	fflush(stdout);
	fflush(stderr);
#endif
	printGetCapabilitiesForProcess(m,n,s1);
	freeService(&s1);
	free(s1);
	scount++;
      }
    (void)closedir(dirp);
    fflush(stdout);
    dup2(saved_stdout,fileno(stdout));
    printDocument(m,doc,getpid());
    freeMaps(&m);
    free(m);
    free(REQUEST);
    free(SERVICE_URL);
    fflush(stdout);
    return 0;
  }
  else{
    r_inputs=getMap(request_inputs,"Identifier");
    if(r_inputs==NULL 
       || strlen(r_inputs->name)==0 || strlen(r_inputs->value)==0){ 
      errorException(m, "Mandatory <identifier> was not specified","MissingParameterValue");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      return 0;
    }

    struct dirent *dp;
    DIR *dirp = opendir(conf_dir);
    if(dirp==NULL){
      errorException(m, "The specified path path doesn't exist.","InvalidParameterValue");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      return 0;
    }
    if(strncasecmp(REQUEST,"DescribeProcess",15)==0){
      /**
       * Loop over Identifier list
       */
      xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
      r_inputs=NULL;
      r_inputs=getMap(request_inputs,"ServiceProvider");

      xmlNodePtr n;
      if(r_inputs!=NULL)
	n = printDescribeProcessHeader(doc,r_inputs->value,m);
      else
	n = printDescribeProcessHeader(doc,"",m);

      r_inputs=getMap(request_inputs,"Identifier");
      char *tmps=strtok(r_inputs->value,",");
      
      char buff[256];
      char buff1[1024];
      int i=0;
      int j=0;
      int end=-1;
      int saved_stdout = dup(fileno(stdout));
      dup2(fileno(stderr),fileno(stdout));
      while(tmps){
	memset(buff,0,256);
	snprintf(buff,256,"%s.zcfg",tmps);
	memset(buff1,0,1024);
#ifdef DEBUG
	fprintf(stderr,"\n#######%s\n########\n",buff1);
#endif
	while ((dp = readdir(dirp)) != NULL)
	  if(strcmp(dp->d_name,buff)==0){
	    memset(buff1,0,1024);
	    snprintf(buff1,1024,"%s/%s",conf_dir,dp->d_name);
	    //s1=(service*)malloc(sizeof(service*));
	    s1=(service*)calloc(1,SERVICE_SIZE);
	    if(s1 == NULL){
	      return errorException(m, "Unable to allocate memory.","InternalError");
	    }
#ifdef DEBUG
	    fprintf(stderr,"#################\n%s\n#################\n",buff1);
#endif
	    t=getServiceFromFile(buff1,&s1);
#ifdef DEBUG
	    dumpService(s1);
#endif
	    printDescribeProcessForProcess(m,n,s1,1);
	    freeService(&s1);
	    free(s1);
	    scount++;
	  }
	rewinddir(dirp);
	tmps=strtok(NULL,",");
      }
      closedir(dirp);
      fflush(stdout);
      dup2(saved_stdout,fileno(stdout));
      printDocument(m,doc,getpid());
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      fflush(stdout);
      //xmlFree(n);
#ifndef LINUX_FREE_ISSUE
      if(s1)
	free(s1);
#endif
      return 0;
    }
    else
      if(strncasecmp(REQUEST,"Execute",strlen(REQUEST))!=0){
	errorException(m, "Unenderstood <request> value. Please check that it was set to GetCapabilities, DescribeProcess or Execute.", "InvalidParameterValue");
#ifdef DEBUG
	fprintf(stderr,"No request found %s",REQUEST);
#endif	
	closedir(dirp);
	free(s);
	return 0;
      }
    closedir(dirp);
  }
  
  s1=NULL;
  s1=(service*)calloc(1,SERVICE_SIZE);
  if(s1 == NULL){
    return errorException(m, "Unable to allocate memory.","InternalError");
  }
  r_inputs=getMap(request_inputs,"MetaPath");
  if(r_inputs!=NULL)
    snprintf(tmps1,1024,"%s/%s",ntmp,r_inputs->value);
  else
    snprintf(tmps1,1024,"%s/",ntmp);
  r_inputs=getMap(request_inputs,"Identifier");
  char *ttmp=strdup(tmps1);
  snprintf(tmps1,1024,"%s/%s.zcfg",ttmp,r_inputs->value);
  free(ttmp);
#ifdef DEBUG
  fprintf(stderr,"Trying to load %s\n", tmps1);
#endif
  int saved_stdout = dup(fileno(stdout));
  dup2(fileno(stderr),fileno(stdout));
  t=getServiceFromFile(tmps1,&s1);
  fflush(stdout);
  dup2(saved_stdout,fileno(stdout));
  if(t==22){
    errorException(m, "The value for <indetifier> seems to be wrong. Please, ensure that the process exsits using the GetCapabilities request.", "InvalidParameterValue");
    exit(0);
  }
  //s[0]=s1;

#ifdef DEBUG
  dumpService(s1);
#endif
  map* inputs=NULL;
  elements* c_inputs=s1->inputs;
  int j;
  
  /**
   * Create the input maps data structure
   */
  int i=0;
  HINTERNET hInternet;
  HINTERNET res;
  hInternet=InternetOpen(
#ifndef WIN32
			 (LPCTSTR)
#endif
			 "ZooWPSClient\0",
			 INTERNET_OPEN_TYPE_PRECONFIG,
			 NULL,NULL, 0);

#ifndef WIN32
  if(!CHECK_INET_HANDLE(hInternet))
    fprintf(stderr,"WARNING : hInternet handle failed to initialize");
#endif
  maps* request_input_real_format=NULL;
  maps* tmpmaps = request_input_real_format;
  map* postRequest=NULL;
  postRequest=getMap(request_inputs,"xrequest");
  if(postRequest==NULLMAP){
    /**
     * Parsing outputs provided as KVP
     */
    r_inputs=NULL;
#ifdef DEBUG
    fprintf(stderr,"OUTPUT Parsing ... \n");
#endif
    r_inputs=getMap(request_inputs,"ResponseDocument");	
    if(r_inputs==NULL) r_inputs=getMap(request_inputs,"RawDataOutput");
    
    //#ifdef DEBUG
    fprintf(stderr,"OUTPUT Parsing ... \n");
    //#endif
    if(r_inputs!=NULL){
      //#ifdef DEBUG
      fprintf(stderr,"OUTPUT Parsing start now ... \n");
      //#endif
      char current_output_as_string[10240];
      char cursor_output[10240];
      char *cotmp=strdup(r_inputs->value);
      snprintf(cursor_output,10240,"%s",cotmp);
      free(cotmp);
      j=0;
      map* request_kvp_outputs=NULL;
	
      /**
       * Put each Output into the outputs_as_text array
       */
      char * pToken;
      maps* tmp_output=NULL;
#ifdef DEBUG
      fprintf(stderr,"OUTPUT [%s]\n",cursor_output);
#endif
      pToken=strtok(cursor_output,";");
      char** outputs_as_text=(char**)calloc(128,sizeof(char*));
      if(outputs_as_text == NULL) {
	return errorException(m, "Unable to allocate memory", "InternalError");
      }
      i=0;
      while(pToken!=NULL){
#ifdef DEBUG
	fprintf(stderr,"***%s***\n",pToken);
	fflush(stderr);
	fprintf(stderr,"***%s***\n",pToken);
#endif
	outputs_as_text[i]=(char*)calloc(strlen(pToken)+1,sizeof(char));
	if(outputs_as_text[i] == NULL) {
	  return errorException(m, "Unable to allocate memory", "InternalError");
	}
	snprintf(outputs_as_text[i],strlen(pToken)+1,"%s",pToken);
	pToken = strtok(NULL,";");
	i++;
      }
      for(j=0;j<i;j++){
	char *tmp=strdup(outputs_as_text[j]);
	free(outputs_as_text[j]);
	char *tmpc;
	tmpc=strtok(tmp,"@");
	int k=0;
	while(tmpc!=NULL){
	  if(k==0){
	    if(tmp_output==NULL){
	      tmp_output=(maps*)calloc(1,MAPS_SIZE);
	      if(tmp_output == NULL){
		return errorException(m, "Unable to allocate memory.", "InternalError");
	      }
	      tmp_output->name=strdup(tmpc);
	      tmp_output->content=NULL;
	      tmp_output->next=NULL;
	    }
	  }
	  else{
	    char *tmpv=strstr(tmpc,"=");
	    char tmpn[256];
	    memset(tmpn,0,256);
	    strncpy(tmpn,tmpc,(strlen(tmpc)-strlen(tmpv))*sizeof(char));
	    tmpn[strlen(tmpc)-strlen(tmpv)]=0;
#ifdef DEBUG
	    fprintf(stderr,"OUTPUT DEF [%s]=[%s]\n",tmpn,tmpv+1);
#endif
	    if(tmp_output->content==NULL){
	      tmp_output->content=createMap(tmpn,tmpv+1);
	      tmp_output->content->next=NULL;
	    }
	    else
	      addToMap(tmp_output->content,tmpn,tmpv+1);
	  }
	  k++;
#ifdef DEBUG
	  fprintf(stderr,"***%s***\n",tmpc);
#endif
	  tmpc=strtok(NULL,"@");
	}
	if(request_output_real_format==NULL)
	  request_output_real_format=dupMaps(&tmp_output);
	else
	  addMapsToMaps(&request_output_real_format,tmp_output);
	freeMaps(&tmp_output);
	free(tmp_output);
	tmp_output=NULL;
#ifdef DEBUG
	dumpMaps(tmp_output);
	fflush(stderr);
#endif
	//tmp_output=tmp_output->next;
	free(tmp);
      }
      free(outputs_as_text);
    }


    /**
     * Parsing inputs provided as KVP
     */
    r_inputs=getMap(request_inputs,"DataInputs");
#ifdef DEBUG
    fprintf(stderr,"DATA INPUTS [%s]\n",r_inputs->value);
#endif
    char current_input_as_string[40960];
    char cursor_input[40960];
    if(r_inputs!=NULL)
      snprintf(cursor_input,40960,"%s",r_inputs->value);
    else{
      errorException(m, "Parameter <DataInputs> was not specified","MissingParameterValue");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      return 0;
    }
    j=0;
    map* request_kvp_inputs=NULL;
  
    /**
     * Put each DataInputs into the inputs_as_text array
     */
    char * pToken;
    pToken=strtok(cursor_input,";");
    char** inputs_as_text=(char**)calloc(100,sizeof(char*));
    if(inputs_as_text == NULL){
      return errorException(m, "Unable to allocate memory.", "InternalError");
    }
    i=0;
    while(pToken!=NULL){
#ifdef DEBUG
      fprintf(stderr,"***%s***\n",pToken);
#endif
      fflush(stderr);
#ifdef DEBUG
      fprintf(stderr,"***%s***\n",pToken);
#endif
      inputs_as_text[i]=(char*)calloc(strlen(pToken)+1,sizeof(char));
      snprintf(inputs_as_text[i],strlen(pToken)+1,"%s",pToken);
      if(inputs_as_text[i] == NULL){
	return errorException(m, "Unable to allocate memory.", "InternalError");
      }
      pToken = strtok(NULL,";");
      i++;
    }

    for(j=0;j<i;j++){
      char *tmp=strdup(inputs_as_text[j]);
      free(inputs_as_text[j]);
      char *tmpc;
      tmpc=strtok(tmp,"@");
      while(tmpc!=NULL){
#ifdef DEBUG
	fprintf(stderr,"***\n***%s***\n",tmpc);
#endif
	char *tmpv=strstr(tmpc,"=");
	char tmpn[256];
	memset(tmpn,0,256);
	strncpy(tmpn,tmpc,(strlen(tmpc)-strlen(tmpv))*sizeof(char));
	tmpn[strlen(tmpc)-strlen(tmpv)]=0;
	int cnt=0;
#ifdef DEBUG
	fprintf(stderr,"***\n*** %s = %s ***\n",tmpn,tmpv+1);
#endif
	if(tmpmaps==NULL){
	  tmpmaps=(maps*)calloc(1,MAPS_SIZE);
	  if(tmpmaps == NULL){
	    return errorException(m, "Unable to allocate memory.", "InternalError");
	  }
	  tmpmaps->name=strdup(tmpn);
	  tmpmaps->content=createMap("value",tmpv+1);
	  tmpmaps->next=NULL;
	}
	tmpc=strtok(NULL,"@");
	while(tmpc!=NULL){
#ifdef DEBUG
	  fprintf(stderr,"*** KVP NON URL-ENCODED \n***%s***\n",tmpc);
#endif
	  char *tmpv1=strstr(tmpc,"=");
#ifdef DEBUG
	  fprintf(stderr,"*** VALUE NON URL-ENCODED \n***%s***\n",tmpv1+1);
#endif
	  char tmpn1[1024];
	  memset(tmpn1,0,1024);
	  strncpy(tmpn1,tmpc,strlen(tmpc)-strlen(tmpv1));
	  tmpn1[strlen(tmpc)-strlen(tmpv1)]=0;
#ifdef DEBUG
	  fprintf(stderr,"*** NAME NON URL-ENCODED \n***%s***\n",tmpn1);
	  fprintf(stderr,"*** VALUE NON URL-ENCODED \n***%s***\n",tmpv1+1);
#endif
	  if(strcmp(tmpn1,"xlink:href")!=0)
	    addToMap(tmpmaps->content,tmpn1,tmpv1+1);
	  else{
#ifdef DEBUG
	    fprintf(stderr,"REQUIRE TO DOWNLOAD A FILE FROM A SERVER : url(%s)\n",tmpv1+1);
#endif
#ifndef WIN32
	    if(CHECK_INET_HANDLE(hInternet))
#endif
	      {
		res=InternetOpenUrl(hInternet,tmpv1+1,NULL,0,
				    INTERNET_FLAG_NO_CACHE_WRITE,0);
#ifdef DEBUG
		fprintf(stderr,"(%s) content-length : %d,,res.nDataAlloc %d \n",
			tmpv1+1,res.nDataAlloc,res.nDataLen);
#endif
		char* tmpContent=(char*)calloc((res.nDataLen+1),sizeof(char));
		if(tmpContent == NULL){
		  return errorException(m, "Unable to allocate memory.", "InternalError");
		}
		size_t dwRead;
		InternetReadFile(res, (LPVOID)tmpContent,res.nDataLen, &dwRead);
		map* tmpMap=getMap(tmpmaps->content,"value");
		if(tmpMap!=NULL)
		  tmpMap->value=strdup(tmpContent);
		free(tmpContent);
	      }
	    addToMap(tmpmaps->content,tmpn1,tmpv1+1);
	  }
	  tmpc=strtok(NULL,"@");
	}
#ifdef DEBUG
	dumpMaps(tmpmaps);
	fflush(stderr);
#endif
	if(request_input_real_format==NULL)
	  request_input_real_format=dupMaps(&tmpmaps);
	else
	  addMapsToMaps(&request_input_real_format,tmpmaps);
	/*freeMap(&tmpmaps->content);
	free(tmpmaps->content);
	tmpmaps->content=NULL;*/
	freeMaps(&tmpmaps);
	free(tmpmaps);
	  //}
	tmpmaps=NULL;
	free(tmp);
      }
    }
    free(inputs_as_text);
  }
  else {
    /**
     * Parse XML request
     */ 
    xmlInitParser();
#ifdef DEBUG
    fflush(stderr);
    fprintf(stderr,"BEFORE %s\n",postRequest->value);
    fflush(stderr);
#endif
    xmlDocPtr doc =
      xmlParseMemory(postRequest->value,cgiContentLength);
#ifdef DEBUG
    fprintf(stderr,"AFTER\n");
    fflush(stderr);
#endif
    xmlNodePtr cur = xmlDocGetRootElement(doc);
    /**
     * Parse every Input in DataInputs node.
     */
    maps* tempMaps=NULL;
    xmlXPathObjectPtr tmpsptr=extractFromDoc(doc,"/*/*/*[local-name()='Input']");
    xmlNodeSet* tmps=tmpsptr->nodesetval;
#ifdef DEBUG
    fprintf(stderr,"*****%d*****\n",tmps->nodeNr);
#endif
    for(int k=0;k<tmps->nodeNr;k++){
      maps *tmpmaps=NULL;
      xmlNodePtr cur=tmps->nodeTab[k];
      if(tmps->nodeTab[k]->type == XML_ELEMENT_NODE) {
	/**
	 * A specific Input node.
	 */
#ifdef DEBUG
	fprintf(stderr, "= element 0 node \"%s\"\n", cur->name);
#endif
	xmlNodePtr cur2=cur->children;
	while(cur2){
	  while(cur2->type!=XML_ELEMENT_NODE)
	    cur2=cur2->next;
	  /**
	   * Indentifier
	   */
	  if(xmlStrncasecmp(cur2->name,BAD_CAST "Identifier",xmlStrlen(cur2->name))==0){
	    xmlChar *val= xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
	    if(tmpmaps==NULL){
	      tmpmaps=(maps*)calloc(1,MAPS_SIZE);
	      if(tmpmaps == NULL){
		return errorException(m, "Unable to allocate memory.", "InternalError");
	      }
	      tmpmaps->name=strdup((char*)val);
	      tmpmaps->content=NULL;
	      tmpmaps->next=NULL;
	    }
	    xmlFree(val);
	  }
	  /**
	   * Title, Asbtract
	   */
	  if(xmlStrncasecmp(cur2->name,BAD_CAST "Title",xmlStrlen(cur2->name))==0 ||
	     xmlStrncasecmp(cur2->name,BAD_CAST "Abstract",xmlStrlen(cur2->name))==0){
	    xmlChar *val=
	      xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
	    if(tmpmaps==NULL){
	      tmpmaps=(maps*)calloc(1,MAPS_SIZE);
	      if(tmpmaps == NULL){
		return errorException(m, "Unable to allocate memory.", "InternalError");
	      }
	      tmpmaps->name="missingIndetifier";
	      tmpmaps->content=createMap((char*)cur2->name,(char*)val);
	      tmpmaps->next=NULL;
	    }
	    else{
	      if(tmpmaps->content!=NULL)
		addToMap(tmpmaps->content,
			 (char*)cur2->name,(char*)val);
	      else
		tmpmaps->content=
		  createMap((char*)cur2->name,(char*)val);
	    }
#ifdef DEBUG
	    dumpMaps(tmpmaps);
#endif
	    xmlFree(val);
	  }
	  /**
	   * InputDataFormChoice (Reference or Data ?) 
	   */
	  if(xmlStrcasecmp(cur2->name,BAD_CAST "Reference")==0){
	    /**
	     * Get every attribute from a Reference node
	     * mimeType, encoding, schema, href, method
	     * Header and Body gesture should be added here
	     */
#ifdef DEBUG
	    fprintf(stderr,"REFERENCE\n");
#endif
	    map* referenceMap=NULL;
	    char *refs[5];
	    refs[0]="mimeType";
	    refs[1]="encoding";
	    refs[2]="schema";
	    refs[3]="method";
	    refs[4]="href";
	    char*url;
	    for(int l=0;l<5;l++){
#ifdef DEBUG
	      fprintf(stderr,"*** %s ***",refs[l]);
#endif
	      xmlChar *val=xmlGetProp(cur2,BAD_CAST refs[l]);
	      if(val!=NULL && xmlStrlen(val)>0){
		if(tmpmaps->content!=NULL)
		  addToMap(tmpmaps->content,refs[l],(char*)val);
		else
		  tmpmaps->content=createMap(refs[l],(char*)val);
		map* ltmp=getMap(tmpmaps->content,"method");
		if(l==4){
		  if(!(ltmp!=NULL && strcmp(ltmp->value,"POST")==0)
		     && CHECK_INET_HANDLE(hInternet)){
		    res=InternetOpenUrl(hInternet,(char*)val,NULL,0,
					INTERNET_FLAG_NO_CACHE_WRITE,0);
		    char* tmpContent=
		      (char*)calloc((res.nDataLen+1),sizeof(char));
		    if(tmpContent == NULL){
		      return errorException(m, "Unable to allocate memory.", "InternalError");
		    }
		    size_t dwRead;
		    InternetReadFile(res, (LPVOID)tmpContent,
				     res.nDataLen, &dwRead);
		    tmpContent[res.nDataLen]=0;
		    addToMap(tmpmaps->content,"value",tmpContent);
		  }
		}
	      }
#ifdef DEBUG
	      fprintf(stderr,"%s\n",val);
#endif
	      xmlFree(val);
	    }
#ifdef POST_DEBUG
	    fprintf(stderr,"Parse Header and Body from Reference \n");
#endif
	    xmlNodePtr cur3=cur2->children;
	    hInternet.header=NULL;
	    while(cur3){
	      if(xmlStrcasecmp(cur3->name,BAD_CAST "Header")==0 ){
		xmlNodePtr cur4=cur3;
		char *tmp=new char[cgiContentLength];
		char *ha[2];
		ha[0]="key";
		ha[1]="value";
		int hai;
		char *has;
		char *key;
		for(hai=0;hai<2;hai++){
		  xmlChar *val=xmlGetProp(cur3,BAD_CAST ha[hai]);
#ifdef POST_DEBUG
		  fprintf(stderr,"%s = %s\n",ha[hai],(char*)val);
#endif
		  if(hai==0){
		    key=(char*)calloc((1+strlen((char*)val)),sizeof(char));
		    snprintf(key,1+strlen((char*)val),"%s",(char*)val);
		  }else{
		    has=(char*)calloc((3+strlen((char*)val)+strlen(key)),sizeof(char));
		    if(has == NULL){
		      return errorException(m, "Unable to allocate memory.", "InternalError");
		    }
		    snprintf(has,(3+strlen((char*)val)+strlen(key)),"%s: %s",key,(char*)val);
#ifdef POST_DEBUG
		    fprintf(stderr,"%s\n",has);
#endif
		  }
		}
		hInternet.header=curl_slist_append(hInternet.header, has);
		//free(has);
	      }
	      else{
#ifdef POST_DEBUG
		fprintf(stderr,"Try to fetch the body part of the request ...\n");
#endif
		if(xmlStrcasecmp(cur3->name,BAD_CAST "Body")==0 ){
#ifdef POST_DEBUG
		  fprintf(stderr,"Body part found !!!\n",(char*)cur3->content);
#endif
		  char *tmp=new char[cgiContentLength];
		  memset(tmp,0,cgiContentLength);
		  xmlNodePtr cur4=cur3->children;
		  while(cur4!=NULL){
		    xmlDocPtr bdoc = xmlNewDoc(BAD_CAST "1.0");
		    bdoc->encoding = xmlCharStrdup ("UTF-8");
		    xmlDocSetRootElement(bdoc,cur4);
		    xmlChar* btmps;
		    int bsize;
		    xmlDocDumpMemory(bdoc,&btmps,&bsize);
#ifdef POST_DEBUG
		    fprintf(stderr,"Body part found !!! %s %s\n",tmp,(char*)btmps);
#endif
		    if(btmps!=NULL)
		      sprintf(tmp,"%s",(char*)btmps);
		    xmlFreeDoc(bdoc);
		    cur4=cur4->next;
		  }
		  map *btmp=getMap(tmpmaps->content,"href");
		  if(btmp!=NULL){
#ifdef POST_DEBUG
		    fprintf(stderr,"%s %s\n",btmp->value,tmp);
		    curl_easy_setopt(hInternet.handle, CURLOPT_VERBOSE, 1);
#endif
		    res=InternetOpenUrl(hInternet,btmp->value,tmp,strlen(tmp),
					INTERNET_FLAG_NO_CACHE_WRITE,0);
		    char* tmpContent = (char*)calloc((res.nDataLen+1),sizeof(char));
		    if(tmpContent == NULL){
		      return errorException(m, "Unable to allocate memory.", "InternalError");
		    }
		    size_t dwRead;
		    InternetReadFile(res, (LPVOID)tmpContent,
				     res.nDataLen, &dwRead);
		    tmpContent[res.nDataLen]=0;
		    if(hInternet.header!=NULL)
		      curl_slist_free_all(hInternet.header);
		    addToMap(tmpmaps->content,"value",tmpContent);
#ifdef POST_DEBUG
		    fprintf(stderr,"DL CONTENT : (%s)\n",tmpContent);
#endif
		  }
		}
		else
		  if(xmlStrcasecmp(cur3->name,BAD_CAST "BodyReference")==0 ){
		    xmlChar *val=xmlGetProp(cur3,BAD_CAST "href");
		    HINTERNET bInternet,res1;
		    bInternet=InternetOpen(
#ifndef WIN32
					   (LPCTSTR)
#endif
					   "ZooWPSClient\0",
					   INTERNET_OPEN_TYPE_PRECONFIG,
					   NULL,NULL, 0);
		    if(!CHECK_INET_HANDLE(bInternet))
		      fprintf(stderr,"WARNING : hInternet handle failed to initialize");
#ifdef POST_DEBUG
		    curl_easy_setopt(bInternet.handle, CURLOPT_VERBOSE, 1);
#endif
		    res1=InternetOpenUrl(bInternet,(char*)val,NULL,0,
					 INTERNET_FLAG_NO_CACHE_WRITE,0);
		    char* tmp=
		      (char*)calloc((res1.nDataLen+1),sizeof(char));
		    if(tmp == NULL){
		      return errorException(m, "Unable to allocate memory.", "InternalError");
		    }
		    size_t bRead;
		    InternetReadFile(res1, (LPVOID)tmp,
				     res1.nDataLen, &bRead);
		    tmp[res1.nDataLen]=0;
		    InternetCloseHandle(bInternet);
		    map *btmp=getMap(tmpmaps->content,"href");
		    if(btmp!=NULL){
#ifdef POST_DEBUG
		      fprintf(stderr,"%s %s\n",btmp->value,tmp);
		      curl_easy_setopt(hInternet.handle, CURLOPT_VERBOSE, 1);
#endif
		      res=InternetOpenUrl(hInternet,btmp->value,tmp,
					  strlen(tmp),
					  INTERNET_FLAG_NO_CACHE_WRITE,0);
		      char* tmpContent = (char*)calloc((res.nDataLen+1),sizeof(char));
		      if(tmpContent == NULL){
			return errorException(m, "Unable to allocate memory.", "InternalError");
		      }
		      size_t dwRead;
		      InternetReadFile(res, (LPVOID)tmpContent,
				       res.nDataLen, &dwRead);
		      tmpContent[res.nDataLen]=0;
		      if(hInternet.header!=NULL)
			curl_slist_free_all(hInternet.header);
		      addToMap(tmpmaps->content,"value",tmpContent);
#ifdef POST_DEBUG
		      fprintf(stderr,"DL CONTENT : (%s)\n",tmpContent);
#endif
		    }
		  }
	      }
	      cur3=cur3->next;
	    }
#ifdef POST_DEBUG
	    fprintf(stderr,"Header and Body was parsed from Reference \n");
#endif
#ifdef DEBUG
	    dumpMap(tmpmaps->content);
	    fprintf(stderr, "= element 2 node \"%s\" = (%s)\n", 
		    cur2->name,cur2->content);
#endif
	  }
	  else if(xmlStrcasecmp(cur2->name,BAD_CAST "Data")==0){
#ifdef DEBUG
	    fprintf(stderr,"DATA\n");
#endif
	    xmlNodePtr cur4=cur2->children;
	    while(cur4){
	      while(cur4->type!=XML_ELEMENT_NODE)
		cur4=cur4->next;

	      if(xmlStrcasecmp(cur4->name, BAD_CAST "LiteralData")==0){
		/**
		 * Get every attribute from a LiteralData node
		 * dataType , uom
		 */
		char *lits[2];
		lits[0]="dataType";
		lits[1]="uom";
		for(int l=0;l<2;l++){
#ifdef DEBUG
		  fprintf(stderr,"*** LiteralData %s ***",lits[l]);
#endif
		  xmlChar *val=xmlGetProp(cur4,BAD_CAST lits[l]);
		  if(val!=NULL && strlen((char*)val)>0){
		    if(tmpmaps->content!=NULL)
		      addToMap(tmpmaps->content,lits[l],(char*)val);
		    else
		      tmpmaps->content=createMap(lits[l],(char*)val);
		  }
#ifdef DEBUG
		  fprintf(stderr,"%s\n",val);
#endif
		  xmlFree(val);
		}
	      }
	      else if(xmlStrcasecmp(cur4->name, BAD_CAST "ComplexData")==0){
		/**
		 * Get every attribute from a Reference node
		 * mimeType, encoding, schema
		 */
		char *coms[3];
		coms[0]="mimeType";
		coms[1]="encoding";
		coms[2]="schema";
		for(int l=0;l<3;l++){
#ifdef DEBUG
		  fprintf(stderr,"*** ComplexData %s ***",coms[l]);
#endif
		  xmlChar *val=xmlGetProp(cur4,BAD_CAST coms[l]);
		  if(val!=NULL && strlen((char*)val)>0){
		    if(tmpmaps->content!=NULL)
		      addToMap(tmpmaps->content,coms[l],(char*)val);
		    else
		      tmpmaps->content=createMap(coms[l],(char*)val);
		  }
#ifdef DEBUG
		  fprintf(stderr,"%s\n",val);
#endif
		  xmlFree(val);
		}
	      }
	      xmlChar* mv=xmlNodeListGetString(doc,cur4->xmlChildrenNode,1);
	      addToMap(tmpmaps->content,"value",(char*)mv);
	      xmlFree(mv);
	      cur4=cur4->next;
	    }
	  }
#ifdef DEBUG
	  fprintf(stderr,"cur2 next \n");
	  fflush(stderr);
#endif
	  cur2=cur2->next;
	}
#ifdef DEBUG
	fprintf(stderr,"ADD MAPS TO REQUEST MAPS !\n");
	fflush(stderr);
#endif
	addMapsToMaps(&request_input_real_format,tmpmaps);
	
#ifdef DEBUG
	fprintf(stderr,"******TMPMAPS*****\n");
	dumpMaps(tmpmaps);
	fprintf(stderr,"******REQUESTMAPS*****\n");
	dumpMaps(request_input_real_format);
#endif
	tmpmaps=tmpmaps->next;
	      
      }
#ifdef DEBUG
      dumpMaps(tmpmaps); 
#endif
    }
#ifdef DEBUG
    fprintf(stderr,"Search for response document node\n");
#endif
    xmlXPathFreeObject(tmpsptr);
    //xmlFree(tmps);
    tmpsptr=extractFromDoc(doc,"/*/*/*[local-name()='ResponseDocument']");
    tmps=tmpsptr->nodesetval;
#ifdef DEBUG
    fprintf(stderr,"*****%d*****\n",tmps->nodeNr);
#endif
    for(int k=0;k<tmps->nodeNr;k++){
      addToMap(request_inputs,"ResponseDocument","");
      request_output_real_format;
      maps *tmpmaps=NULL;
      xmlNodePtr cur=tmps->nodeTab[k];
      if(cur->type == XML_ELEMENT_NODE) {
	/**
	 * A specific responseDocument node.
	 */
	if(tmpmaps==NULL){
	  tmpmaps=(maps*)calloc(1,MAPS_SIZE);
	  if(tmpmaps == NULL){
	    return errorException(m, "Unable to allocate memory.", "InternalError");
	  }
	  tmpmaps->name="unknownIdentifier";
	  tmpmaps->next=NULL;
	}
	/**
	 * Get every attribute from a LiteralData node
	 * storeExecuteResponse, lineage, status
	 */
	char *ress[3];
	ress[0]="storeExecuteResponse";
	ress[1]="lineage";
	ress[2]="status";
	xmlChar *val;
	for(int l=0;l<3;l++){
#ifdef DEBUG
	  fprintf(stderr,"*** %s ***\t",ress[l]);
#endif
	  val=xmlGetProp(cur,BAD_CAST ress[l]);
	  if(val!=NULL && strlen((char*)val)>0){
	    if(tmpmaps->content!=NULL)
	      addToMap(tmpmaps->content,ress[l],(char*)val);
	    else
	      tmpmaps->content=createMap(ress[l],(char*)val);
	    addToMap(request_inputs,ress[l],(char*)val);
	  }
#ifdef DEBUG
	  fprintf(stderr,"%s\n",val);
#endif
	  xmlFree(val);
	}
	xmlNodePtr cur1=cur->children;
	while(cur1){
	  if(xmlStrncasecmp(cur1->name,BAD_CAST "Output",xmlStrlen(cur1->name))==0){
	    /**
	     * Get every attribute from a Output node
	     * mimeType, encoding, schema, uom, asReference
	     */
	    char *outs[5];
	    outs[0]="mimeType";
	    outs[1]="encoding";
	    outs[2]="schema";
	    outs[3]="uom";
	    outs[4]="asReference";
	    for(int l=0;l<5;l++){
#ifdef DEBUG
	      fprintf(stderr,"*** %s ***\t",outs[l]);
#endif
	      val=xmlGetProp(cur1,BAD_CAST outs[l]);
	      if(val!=NULL && strlen((char*)val)>0){
		if(tmpmaps->content!=NULL)
		  addToMap(tmpmaps->content,outs[l],(char*)val);
		else
		  tmpmaps->content=createMap(outs[l],(char*)val);
	      }
#ifdef DEBUG
	      fprintf(stderr,"%s\n",val);
#endif
	      xmlFree(val);
	    }
	    
	    xmlNodePtr cur2=cur1->children;
	    while(cur2){
	      /**
	       * Indentifier
	       */
	      if(xmlStrncasecmp(cur2->name,BAD_CAST "Identifier",xmlStrlen(cur2->name))==0){
		xmlChar *val=
		  xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
		if(tmpmaps==NULL){
		  tmpmaps=(maps*)calloc(1,MAPS_SIZE);
		  if(tmpmaps == NULL){
		    return errorException(m, "Unable to allocate memory.", "InternalError");
		  }
		  tmpmaps->name=strdup((char*)val);
		  tmpmaps->content=NULL;
		  tmpmaps->next=NULL;
		}
		else
		  tmpmaps->name=strdup((char*)val);;
		xmlFree(val);
	      }
	      /**
	       * Title, Asbtract
	       */
	      if(xmlStrncasecmp(cur2->name,BAD_CAST "Title",xmlStrlen(cur2->name))==0 ||
		 xmlStrncasecmp(cur2->name,BAD_CAST "Abstract",xmlStrlen(cur2->name))==0){
		xmlChar *val=
		  xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
		if(tmpmaps==NULL){
		  tmpmaps=(maps*)calloc(1,MAPS_SIZE);
		  if(tmpmaps == NULL){
		    return errorException(m, "Unable to allocate memory.", "InternalError");
		  }
		  tmpmaps->name="missingIndetifier";
		  tmpmaps->content=createMap((char*)cur2->name,(char*)val);
		  tmpmaps->next=NULL;
		}
		else{
		  if(tmpmaps->content!=NULL)
		    addToMap(tmpmaps->content,
			     (char*)cur2->name,(char*)val);
		  else
		    tmpmaps->content=
		      createMap((char*)cur2->name,(char*)val);
		}
		xmlFree(val);
	      }
	      cur2=cur2->next;
	    }
	  }
	  cur1=cur1->next;
	}
      }
      //xmlFree(cur);
      if(request_output_real_format==NULL)
	request_output_real_format=tmpmaps;
      else
	addMapsToMaps(&request_output_real_format,tmpmaps);
#ifdef DEBUG
      dumpMaps(tmpmaps);
#endif
    }
    xmlXPathFreeObject(tmpsptr);
    //xmlFree(tmps);
    tmpsptr=extractFromDoc(doc,"/*/*/*[local-name()='RawDataOutput']");
    tmps=tmpsptr->nodesetval;
#ifdef DEBUG
    fprintf(stderr,"*****%d*****\n",tmps->nodeNr);
#endif
    for(int k=0;k<tmps->nodeNr;k++){
      addToMap(request_inputs,"RawDataOutput","");
      xmlNodePtr cur1=tmps->nodeTab[k];
      xmlChar *val;
      /**
       * Get every attribute from a Output node
       * mimeType, encoding, schema, uom, asReference
       */
      char *outs[4];
      outs[0]="mimeType";
      outs[1]="encoding";
      outs[2]="schema";
      outs[3]="uom";
      for(int l=0;l<4;l++){
#ifdef DEBUG
	fprintf(stderr,"*** %s ***\t",outs[l]);
#endif
	val=xmlGetProp(cur1,BAD_CAST outs[l]);
	if(val!=NULL && strlen((char*)val)>0){
	  if(tmpmaps==NULL){
	    tmpmaps=(maps*)calloc(1,MAPS_SIZE);
	    if(tmpmaps == NULL){
	      return errorException(m, "Unable to allocate memory.", "InternalError");
	    }
	    tmpmaps->name="unknownIdentifier";
	    tmpmaps->content=createMap(outs[l],(char*)val);
	    tmpmaps->next=NULL;
	  }
	  else
	    addToMap(tmpmaps->content,outs[l],(char*)val);
	}
#ifdef DEBUG
	fprintf(stderr,"%s\n",val);
#endif
	xmlFree(val);
      }
	    
      xmlNodePtr cur2=cur1->children;
      while(cur2){
	/**
	 * Indentifier
	 */
	if(xmlStrncasecmp(cur2->name,BAD_CAST "Identifier",xmlStrlen(cur2->name))==0){
	  val=
	    xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
	  if(tmpmaps==NULL){
	    tmpmaps=(maps*)calloc(1,MAPS_SIZE);
	    if(tmpmaps == NULL){
	      return errorException(m, "Unable to allocate memory.", "InternalError");
	    }
	    tmpmaps->name=strdup((char*)val);
	    tmpmaps->content=NULL;
	    tmpmaps->next=NULL;
	  }
	  else
	    tmpmaps->name=strdup((char*)val);;
	  xmlFree(val);
	}
	cur2=cur2->next;
      }
      if(request_output_real_format==NULL)
	request_output_real_format=tmpmaps;
      else
	addMapsToMaps(&request_output_real_format,tmpmaps);
#ifdef DEBUG
      dumpMaps(tmpmaps);
#endif
    }
    xmlXPathFreeObject(tmpsptr);
    //xmlFree(tmps);
    xmlCleanupParser();
  }

  //if(CHECK_INET_HANDLE(hInternet))
  InternetCloseHandle(hInternet);

#ifdef DEBUG
  fprintf(stderr,"\n%i\n",i);
  dumpMaps(request_input_real_format);
  dumpMaps(request_output_real_format);
  dumpMap(request_inputs);
#endif

  /**
   * Ensure that each requested arguments are present in the request
   * DataInputs and ResponseDocument / RawDataOutput
   */  
  char *dfv=addDefaultValues(&request_input_real_format,s1->inputs,m,"inputs");
  if(strcmp(dfv,"")!=0){
    char tmps[1024];
    snprintf(tmps,1024,"The <%s> argument was not specified in DataInputs but defined as requested in ZOO ServicesProvider configuration file, please correct your query or the ZOO Configuration file.",dfv);
    map* tmpe=createMap("text",tmps);
    addToMap(tmpe,"code","MissingParameterValue");
    printExceptionReportResponse(m,tmpe);
    freeMap(&tmpe);
    free(tmpe);
    freeMaps(&m);
    free(m);
    free(REQUEST);
    freeMaps(&request_input_real_format);
    free(request_input_real_format);
    freeMaps(&request_output_real_format);
    free(request_output_real_format);
    freeMaps(&tmpmaps);
    free(tmpmaps);
    return 1;
  }
  addDefaultValues(&request_output_real_format,s1->outputs,m,"outputs");

#ifdef DEBUG
  fprintf(stderr,"REQUEST_INPUTS\n");
  dumpMaps(request_input_real_format);
  fprintf(stderr,"REQUEST_OUTPUTS\n");
  dumpMaps(request_output_real_format);
#endif

  maps* curs=getMaps(m,"env");
  if(curs!=NULL){
    map* mapcs=curs->content;
    while(mapcs!=NULLMAP){
#ifndef WIN32
      setenv(mapcs->name,mapcs->value,1);
#else
#ifdef DEBUG
      fprintf(stderr,"[ZOO: setenv (%s=%s)]\n",mapcs->name,mapcs->value);
#endif
      if(mapcs->value[strlen(mapcs->value)-2]=='\r'){
#ifdef DEBUG
	fprintf(stderr,"[ZOO: Env var finish with \r]\n");
#endif
	mapcs->value[strlen(mapcs->value)-1]=0;
      }
#ifdef DEBUG
      fflush(stderr);
      fprintf(stderr,"setting variable... %s\n",
#endif
	      SetEnvironmentVariable(mapcs->name,mapcs->value)
#ifdef DEBUG
	      ? "OK" : "FAILED");
#else
      ;
#endif
#ifdef DEBUG
      fflush(stderr);
#endif
#endif
#ifdef DEBUG
      fprintf(stderr,"[ZOO: setenv (%s=%s)]\n",mapcs->name,mapcs->value);
      fflush(stderr);
#endif
      mapcs=mapcs->next;
    }
  }
  
#ifdef DEBUG
  dumpMap(request_inputs);
#endif

  /**
   * Need to check if we need to fork to load a status enabled 
   */
  r_inputs=NULL;
  r_inputs=getMap(request_inputs,"storeExecuteResponse");
  int eres=SERVICE_STARTED;
  int cpid=getpid();
#ifdef DEBUG
  dumpMap(request_inputs);
#endif

  if(r_inputs!=NULL)
    if(strcasecmp(r_inputs->value,"false")==0)
      r_inputs=NULL;
  if(r_inputs==NULLMAP){
    /**
     * Extract serviceType to know what kind of service shoudl be loaded
     */
    r_inputs=NULL;
    r_inputs=getMap(s1->content,"serviceType");
#ifdef DEBUG
    fprintf(stderr,"LOAD A %s SERVICE PROVIDER IN NORMAL MODE \n",r_inputs->value);
    fflush(stderr);
#endif
    if(strncasecmp(r_inputs->value,"C",1)==0){
      r_inputs=getMap(request_inputs,"metapath");
      if(r_inputs!=NULL)
	sprintf(tmps1,"%s/%s",ntmp,r_inputs->value);
      else
	sprintf(tmps1,"%s/",ntmp);
      char *altPath=strdup(tmps1);
      r_inputs=getMap(s1->content,"ServiceProvider");
      sprintf(tmps1,"%s/%s",altPath,r_inputs->value);
      free(altPath);
#ifdef DEBUG
      fprintf(stderr,"Trying to load %s\n",tmps1);
#endif
#ifdef WIN32
      HINSTANCE so = LoadLibraryEx(tmps1,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
#else
      void* so = dlopen(tmps1, RTLD_LAZY);
#endif
#ifdef DEBUG
#ifdef WIN32
      DWORD errstr;
      errstr = GetLastError();
      fprintf(stderr,"%s loaded (%d) \n",tmps1,errstr);
#else
      char *errstr;
      errstr = dlerror();
#endif
#endif

      if( so != NULL ) {
#ifdef DEBUG
	fprintf(stderr,"Library loaded %s \n",errstr);
	fprintf(stderr,"Service Shared Object = %s\n",r_inputs->value);
#endif
	r_inputs=getMap(s1->content,"serviceType");
#ifdef DEBUG
	dumpMap(r_inputs);
	fprintf(stderr,"%s\n",r_inputs->value);
	fflush(stderr);
#endif
	if(strncasecmp(r_inputs->value,"C-FORTRAN",9)==0){
#ifdef WIN32
	  //Strange return value needed here !
	  return 1;
#endif
	  r_inputs=getMap(request_inputs,"Identifier");
	  char fname[1024];
	  sprintf(fname,"%s_",r_inputs->value);
#ifdef DEBUG
	  fprintf(stderr,"Try to load function %s\n",fname);
#endif
#ifdef WIN32
	  typedef int (CALLBACK* execute_t)(char***,char***,char***);
	  execute_t execute=(execute_t)GetProcAddress(so,fname);
#else
	  typedef int (*execute_t)(char***,char***,char***);
	  execute_t execute=(execute_t)dlsym(so,fname);
#endif
#ifdef DEBUG
#ifdef WIN32
	  errstr = GetLastError();
#else
	  errstr = dlerror();
#endif
	  fprintf(stderr,"Function loaded %s\n",errstr);
#endif	

	  char main_conf[10][30][1024];
	  char inputs[10][30][1024];
	  char outputs[10][30][1024];
	  for(int i=0;i<10;i++){
	    for(int j=0;j<30;j++){
	      memset(main_conf[i][j],0,1024);
	      memset(inputs[i][j],0,1024);
	      memset(outputs[i][j],0,1024);
	    }
	  }
	  mapsToCharXXX(m,(char***)main_conf);
	  mapsToCharXXX(request_input_real_format,(char***)inputs);
	  mapsToCharXXX(request_output_real_format,(char***)outputs);
	  eres=execute((char***)&main_conf[0],(char***)&inputs[0],(char***)&outputs[0]);
#ifdef DEBUG
	  fprintf(stderr,"Function run successfully \n");
#endif
	  charxxxToMaps((char***)&outputs[0],&request_output_real_format);
	}else{
#ifdef DEBUG
#ifdef WIN32
	  errstr = GetLastError();
	  fprintf(stderr,"Function %s failed to load because of %d\n",r_inputs->value,errstr);
#endif
#endif
	  r_inputs=getMap(request_inputs,"Identifier");
#ifdef DEBUG
	  fprintf(stderr,"Try to load function %s\n",r_inputs->value);
#endif
	  typedef int (*execute_t)(maps**,maps**,maps**);
#ifdef WIN32
	  execute_t execute=(execute_t)GetProcAddress(so,r_inputs->value); 
#ifdef DEBUG
	  errstr = GetLastError();
	  fprintf(stderr,"Function %s failed to load because of %d\n",r_inputs->value,errstr);
#endif
#else
	  execute_t execute=(execute_t)dlsym(so,r_inputs->value);
#endif

#ifdef DEBUG
#ifdef WIN32
	  errstr = GetLastError();
#else
	  errstr = dlerror();
#endif
	  fprintf(stderr,"Function loaded %s\n",errstr);
#endif	

#ifdef DEBUG
	  fprintf(stderr,"Now run the function \n");
	  fflush(stderr);
#endif
	  eres=execute(&m,&request_input_real_format,&request_output_real_format);
#ifdef DEBUG
	  fprintf(stderr,"Function loaded and returned %d\n",eres);
	  fflush(stderr);
#endif
	}
	dlclose(so);
      } else {
	/**
	 * Unable to load the specified shared library
	 */
	char tmps[1024];
#ifdef WIN32
	DWORD errstr = GetLastError();
#else
	char* errstr = dlerror();
#endif
	sprintf(tmps,"C Library can't be loaded %s \n",errstr);
	map* tmps1=createMap("text",tmps);
	printExceptionReportResponse(m,tmps1);
	exit(1);
      }
    }
    else{
#ifdef USE_PYTHON
      if(strncasecmp(r_inputs->value,"PYTHON",6)==0){
	eres=zoo_python_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
      }
      else
#endif
	
#ifdef USE_JAVA
	if(strncasecmp(r_inputs->value,"JAVA",4)==0){
	  eres=zoo_java_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	}
	else
#endif

#ifdef USE_PHP
	  if(strncasecmp(r_inputs->value,"PHP",3)==0){
	    eres=zoo_php_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	  }
	  else
#endif


#ifdef USE_PERL
          if(strncasecmp(r_inputs->value,"PERL",4)==0){
            eres=zoo_perl_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
          }
          else
#endif

#ifdef USE_JS
	    if(strncasecmp(r_inputs->value,"JS",2)==0){
	      eres=zoo_js_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	    }
	    else
#endif
	      {
		char tmpv[1024];
		sprintf(tmpv,"Programming Language (%s) set in ZCFG file is not currently supported by ZOO Kernel.\n",r_inputs->value);
		map* tmps=createMap("text",tmpv);
		printExceptionReportResponse(m,tmps);
		return(-1);
	      }
    }
  }
  else{

    pid_t   pid;
#ifdef DEBUG
    fprintf(stderr,"\nPID : %d\n",cpid);
#endif

#ifndef WIN32
    pid = fork ();
#else
    pid = 0;
#endif
    if (pid > 0) {
      /**
       * dady :
       * set status to SERVICE_ACCEPTED
       */
#ifdef DEBUG
      fprintf(stderr,"father pid continue (origin %d) %d ...\n",cpid,getpid());
#endif
      eres=SERVICE_ACCEPTED;
    }else if (pid == 0) {
      /**
       * son : have to close the stdout, stdin and stderr to let the parent
       * process answer to http client.
       */
      r_inputs=getMapFromMaps(m,"main","tmpPath");
      map* r_inputs1=getMap(s1->content,"ServiceProvider");
      char* fbkp=(char*)malloc((strlen(r_inputs->value)+strlen(r_inputs1->value)+16)*sizeof(char));
      sprintf(fbkp,"%s/%s_%d.xml",r_inputs->value,r_inputs1->value,cpid);
      char* flog=(char*)malloc((strlen(r_inputs->value)+strlen(r_inputs1->value)+22)*sizeof(char));
      sprintf(flog,"%s/%s_%d_error.log",r_inputs->value,r_inputs1->value,cpid);
      fprintf(stderr,"File to use as backup %s\n",fbkp);
#ifdef DEBUG
      fprintf(stderr,"RUN IN BACKGROUND MODE \n");
      fprintf(stderr,"son pid continue (origin %d) %d ...\n",cpid,getpid());
      fprintf(stderr,"\nFILE TO STORE DATA %s\n",r_inputs->value);
#endif
      freopen(fbkp , "w+", stdout);
      fclose(stdin);
      freopen(flog,"w+",stderr);
      free(fbkp);
      free(flog);
      /**
       * set status to SERVICE_STARTED and flush stdout to ensure full 
       * content was outputed (the file used to store the ResponseDocument).
       * The rewind stdout to restart writing from the bgining of the file,
       * this way the data will be updated at the end of the process run.
       */
      printProcessResponse(m,request_inputs,cpid,
			    s1,r_inputs->value,SERVICE_STARTED,
			    request_input_real_format,
			    request_output_real_format);
      fflush(stdout);
      rewind(stdout);
      /**
       * Extract serviceType to know what kind of service shoudl be loaded
       */
      r_inputs=NULL;
      r_inputs=getMap(s1->content,"serviceType");
#ifdef DEBUG
      fprintf(stderr,"LOAD A %s SERVICE PROVIDER IN BACKGROUND MODE \n",r_inputs->value);
#endif

      if(strncasecmp(r_inputs->value,"C",1)==0){

	r_inputs=getMap(request_inputs,"metapath");
	if(r_inputs!=NULL){
	  sprintf(tmps1,"%s/%s",ntmp,r_inputs->value);
	  r_inputs=getMap(s1->content,"ServiceProvider");
	  if(r_inputs!=NULL)
	    sprintf(tmps1,"%s/%s",strdup(tmps1),r_inputs->value);
	}else{
	  sprintf(tmps1,"%s/",ntmp);
	}
	  
  
#ifdef DEBUG
	fprintf(stderr,"Trying to load %s\n",tmps1);
#endif
#ifdef WIN32
	HINSTANCE so = LoadLibraryEx(tmps1,NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
#else
	void* so = dlopen(tmps1, RTLD_LAZY);
#endif
#ifdef WIN32
	DWORD errstr;
	errstr = GetLastError();
#else
	char *errstr;
	errstr = dlerror();
#endif
	if( so != NULL ) {
	  r_inputs=getMap(s1->content,"serviceType");
#ifdef DEBUG
	  fprintf(stderr,"r_inputs->value = %s\n",r_inputs->value);
#endif
	  if(strncasecmp(r_inputs->value,"C-FORTRAN",9)==0){
	    r_inputs=getMap(request_inputs,"Identifier");
#ifdef DEBUG
	    fprintf(stderr,"Try to load function %s\n",r_inputs->value);
#endif
	    typedef int (*execute_t)(char***,char***,char***);
	    char fname[1024];
	    sprintf(fname,"%s_",r_inputs->value);
#ifdef DEBUG
	    fprintf(stderr,"Try to load function %s\n",fname);
#endif
#ifdef WIN32
	    execute_t execute=(execute_t)GetProcAddress(so,fname); 
#else
	    execute_t execute=(execute_t)dlsym(so,fname);
#endif
#ifdef DEBUG
#ifdef WIN32
	    errstr = GetLastError();
#else
	    errstr = dlerror();
#endif
#endif	
	    char main_conf[10][10][1024];
	    char inputs[10][10][1024];
	    char outputs[10][10][1024];
	    for(int i=0;i<10;i++){
	      for(int j=0;j<10;j++){
		memset(main_conf[i][j],0,1024);
		memset(inputs[i][j],0,1024);
		memset(outputs[i][j],0,1024);
	      }
	    }
	    mapsToCharXXX(m,(char***)main_conf);
	    mapsToCharXXX(request_input_real_format,(char***)inputs);
	    //mapsToCharXXX(request_output_real_format,(char***)outputs);
	    eres=execute((char***)&main_conf[0],(char***)&inputs[0],(char***)&outputs[0]);
	    charxxxToMaps((char***)&outputs[0],&request_output_real_format);

	  }else{

	    typedef int (*execute_t)(maps**,maps**,maps**);
#ifdef DEBUG
	    fprintf(stderr,"Library loaded %s \n",errstr);
#endif
	    r_inputs=getMap(request_inputs,"Identifier");
#ifdef DEBUG
	    fprintf(stderr,"Try to load function %s\n",r_inputs->value);
#endif
#ifdef WIN32
	    execute_t execute=(execute_t)GetProcAddress(so,r_inputs->value); 
#else
	    execute_t execute=(execute_t)dlsym(so,r_inputs->value);
#endif
#ifdef DEBUG
#ifdef WIN32
	    errstr = GetLastError();
#else
	    errstr = dlerror();
#endif
	    fprintf(stderr,"Function loaded %s\n",errstr);
#endif	
	    /**
	     * set the status code value returned by the service function itself
	     */
	    eres=execute(&m,&request_input_real_format,&request_output_real_format);
	  }
	  dlclose(so);
	} else {
	  /**
	   * Unable to load the requested C Library
	   */
	  char tmps2[1024];
	  sprintf(tmps1,"C Library can't be loaded %s \n",errstr);
	  map* tmps=createMap("text",tmps1);
	  printExceptionReportResponse(m,tmps);
	  freeMap(&tmps);
	  free(tmps);
	  exit(1);
	}
      } else{

#ifdef USE_PYTHON
	if(strncasecmp(r_inputs->value,"PYTHON",6)==0){
	  eres=zoo_python_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	}
	else
#endif

#ifdef USE_JAVA
	  if(strncasecmp(r_inputs->value,"JAVA",4)==0){
	    eres=zoo_java_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	  }
	  else
#endif
	    
#ifdef USE_PHP
	    if(strncasecmp(r_inputs->value,"PHP",3)==0){
	      eres=zoo_php_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	    }
	    else
#endif
	      
#ifdef USE_PERL
          if(strncasecmp(r_inputs->value,"PERL",4)==0){
            eres=zoo_perl_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
          }
          else
#endif
#ifdef USE_JS
	      if(strncasecmp(r_inputs->value,"JS",2)==0){
		eres=zoo_js_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	      }
	      else
#endif
		{
		  char tmpv[1024];
		  sprintf(tmpv,"Programming Language (%s) set in ZCFG file is not currently supported by ZOO Kernel.\n",r_inputs->value);
		  map* tmps=createMap("text",tmpv);
		  printExceptionReportResponse(m,tmps);
		  return -1;
		}
      }
  
      //res=execute(&m,&request_input_real_format,&request_output_real_format);
    } else {
      /**
       * error server don't accept the process need to output a valid 
       * error response here !!!
       */
    }
      	
  }

#ifdef DEBUG
  dumpMaps(request_output_real_format);
  fprintf(stderr,"Function loaded and returned %d\n",eres);
  fflush(stderr);
#endif
  if(eres!=-1)
    outputResponse(s1,request_input_real_format,
		   request_output_real_format,request_inputs,
		   cpid,m,eres);

  //if(getpid()==cpid){
  freeService(&s1);
  free(s1);
  freeMaps(&m);
  free(m);
  freeMaps(&tmpmaps);
  free(tmpmaps);
  
  freeMaps(&request_input_real_format);
  free(request_input_real_format);
  
  //freeMap(&request_inputs);
  //free(request_inputs);
    
  /* The following is requested but get issue using with Python support :/ */
  /* freeMaps(&request_output_real_format);
     free(request_output_real_format);
  */
  
  free(REQUEST);
  free(SERVICE_URL);
#ifdef DEBUG
  fprintf(stderr,"Processed response \n");
  fflush(stdout);
  fflush(stderr);
#endif
    //}

  return 0;
}
