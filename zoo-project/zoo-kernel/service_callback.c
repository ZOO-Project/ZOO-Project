/*
 * Author : Gérald FENOY
 *
 *  Copyright 2017-2019 GeoLabs SARL. All rights reserved.
 *
 * This work was supported by public funds received in the framework of GEOSUD,
 * a project (ANR-10-EQPX-20) of the program "Investissements d'Avenir" managed 
 * by the French National Research Agency
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

#include "service_json.h"
#include "service_internal_ms.h"
#include "sqlapi.h"
#include <pthread.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "service_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Parameter definition to be used for sending parameters to a thread.
   */
  typedef struct {
    maps *conf;      //!< the main configuration file
    map *url;        //!< the callback url maps
    json_object *res;//!< the JSON object to post 
    int step;        //!< the current step [0,6]
    int state;       //!< the current state [0,1]
  } local_params;

  /**
   * Number of threads
   */
  int nbThreads=0;
  /**
   * Current step
   */
  int cStep=0;
  /**
   * Is there any ongoing HTTP request
   */
  int isOngoing=0;
  /**
   * Threads array
   */
  pthread_t* myThreads=NULL;
  /**
   * Steps array
   */
  bool steps[7][2]={
    {false,false},
    {false,false},
    {false,false},
    {false,false},
    {false,false},
    {false,false},
    {false,false}
  };
  /**
   * Arguments array to give to the _invokeCallback thread's function
   */
  local_params** local_arguments;
  
  /**
   * Check if a service name is prohibited, meaning that the Kernel doesn't have
   * to invoke the callback for this specific service.
   *
   * @param conf the main configuration file maps
   * @param serviceName the serviceName
   * @return a bool true if the service is prohibited, false in other case
   */
  bool isProhibited(maps* conf,const char* serviceName){
    map* plist=getMapFromMaps(conf,"callback","prohibited");
    if(plist!=NULL){
      char *tmp=plist->value;
      char *tmpS=strtok(tmp,",");
      while(tmpS!=NULL){
	if(strcmp(serviceName,tmpS)==0)
	  return true;
	tmpS=strtok(NULL,",");
      }
    }
    return false;
  }

  /**
   * Practically invoke the callback, meaning sending the HTTP POST request.
   * 
   * @param args local_params containing all the variables required
   */
  void* _invokeCallback(void* args){
    local_params* arg=(local_params*)args;
    HINTERNET hInternet,res1;
    const struct tm *tm;
    size_t len;
    time_t now;
    char *tmp1;
    map *tmpStatus;
    maps* tmpConf=createMaps("main");
    tmpConf->content=createMap("memory","load");
    hInternet=InternetOpen("ZooWPSClient\0",
			   INTERNET_OPEN_TYPE_PRECONFIG,
			   NULL,NULL, 0);
    if(!CHECK_INET_HANDLE(hInternet)){
      InternetCloseHandle (&hInternet);
      return NULL;
    }
    char *URL=(char*)malloc((strlen(arg->url->value)+5)*sizeof(char));
    sprintf(URL,"%s%d_%d/",arg->url->value,arg->step,arg->state);
    const char* jsonStr=json_object_to_json_string_ext(arg->res,JSON_C_TO_STRING_PLAIN);
    hInternet.waitingRequests[0] = zStrdup(URL);
    free(URL);
#ifdef CALLBACK_DEBUG
    now = time ( NULL );
    tm = localtime ( &now );
    tmp1 = (char*)malloc((TIME_SIZE+1)*sizeof(char));
    len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST PARAMETERS cStep %d %d\n",pthread_self(),__FILE__,__LINE__,cStep,isOngoing);
    fprintf(stderr," * JSON: [%s] \n",jsonStr);
    fprintf(stderr," * URL: %s/ \n\n",hInternet.waitingRequests[0]);
    fprintf(stderr," * DATE: %s/ \n\n",tmp1);
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST PARAMETERS\n",pthread_self(),__FILE__,__LINE__);
    free(tmp1);
#endif
    while( (arg->step!=7 || isOngoing>0) &&
	   ( cStep!=arg->step || (arg->state!=0 && steps[arg->step][0]==false) )
	   ){
      zSleep(100);
    }
    isOngoing=1;
#ifdef CALLBACK_DEBUG
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST START\n\n",pthread_self(),__FILE__,__LINE__);
    int i=0;
    for(i=0;i<7;i++){
      fprintf(stderr,"%d) %d %d\n",i,steps[i][0],steps[i][1]);
    }
#endif
    
    now = time ( NULL );
    tm = localtime ( &now );
    
    tmp1 = (char*)malloc((TIME_SIZE+1)*sizeof(char));
    len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );

#ifdef CALLBACK_DEBUG    
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST START (%s)\n",pthread_self(),__FILE__,__LINE__,tmp1);
    fflush(stderr);
#endif    
    free(tmp1);
    res1 = InternetOpenUrl (&hInternet,
			    hInternet.waitingRequests[0], 
			    (char*)jsonStr, strlen(jsonStr),
			    INTERNET_FLAG_NO_CACHE_WRITE,
			    0,tmpConf);
    AddHeaderEntries(&hInternet,arg->conf);
    //curl_easy_setopt(hInternet.ihandle[hInternet.nb].handle, CURLOPT_VERBOSE, 1);x
    processDownloads(&hInternet);
    freeMaps(&tmpConf);
    free(tmpConf);
    now = time ( NULL );
    tm = localtime ( &now );
    tmp1 = (char*)malloc((TIME_SIZE+1)*sizeof(char));
    len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );
    
#ifdef CALLBACK_DEBUG    
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST END (%s)\n\n",pthread_self(),__FILE__,__LINE__,tmp1);
#endif
    free(tmp1);
    char *tmp = (char *) malloc ((hInternet.ihandle[0].nDataLen + 1)
				 * sizeof (char));
    if (tmp == NULL)
      {
	setMapInMaps(arg->conf,"lenv","message",_("Unable to allocate memory"));
	setMapInMaps(arg->conf,"lenv","code","InternalError");
	return NULL;
      }
    size_t bRead;
    InternetReadFile (hInternet.ihandle[0],
		      (LPVOID) tmp,
		      hInternet.
		      ihandle[0].nDataLen,
		      &bRead);
    tmp[hInternet.ihandle[0].nDataLen] = 0;
    json_object_put(arg->res);
    InternetCloseHandle(&hInternet);
    isOngoing=0;
    if(cStep==0 || cStep==6 || arg->state==1)
      cStep=arg->step+1;
#ifdef CALLBACK_DEBUG
    now = time ( NULL );
    tm = localtime ( &now );
    tmp1 = (char*)malloc((TIME_SIZE+1)*sizeof(char));
    len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );
    fprintf(stderr,"************************* From thread %d %s %d: RESPONSE CONTENT (%s)\n",pthread_self(),__FILE__,__LINE__,tmp1);
    for(i=0;i<7;i++){
      fprintf(stderr,"%d) %d %d\n",i,steps[i][0],steps[i][1]);
    }
    fprintf(stderr,"Result: \n%s\n\n",tmp);
    fprintf(stderr,"************************* From thread %d %s %d\n\n",pthread_self(),__FILE__,__LINE__);
    fflush(stderr);
    free(tmp1);
#endif
    steps[arg->step][arg->state]=true;
    free(tmp);
#ifdef CALLBACK_DEBUG
    fprintf(stderr,"************************* From thread %d %s %d: EXIT\n\n",pthread_self(),__FILE__,__LINE__);
    fflush(stderr);
#endif
    pthread_exit(NULL);
  }
  
  /**
   * Invoke the callback in case there is a [callback] section containing a url parameter
   * 
   * @param m the maps containing the main configuration file definitions
   * @param inputs the inputs defined in the request (can be null if not yet initialized)
   * @param inputs the outputs provided in the request (can be null if not yet initialized)
   * @param step the step number, steps are defined as: 
   *  0: Analyze creation
   *  1: Fetching Data Inputs 
   *  2: Uploading data inputs to cluster
   *  3: Creating Job Script
   *  4: Submitting Job to Cluster
   *  5: Downloading processed output from cluster
   *  6: Finalize
   *  7: Dismiss or Error
   * @param state 0 in case the step starts, 1 when it ends
   * @return bool true in case of success, false in other cases
   */
  bool invokeCallback(maps* conf,maps* inputs,maps* outputs,int step,int state){
    map* url=getMapFromMaps(conf,"callback","url");
    if(url==NULL)
      return false;
      
    maps* lenv=getMaps(conf,"lenv");
    map* sname=getMap(lenv->content,"identifier");
    if(sname!=NULL && isProhibited(conf,sname->value))
      return false;
      
    json_object *res=json_object_new_object();

    map* sid=getMapFromMaps(conf,"lenv","usid");
    if(sid!=NULL){
      json_object *jsStr=json_object_new_string(sid->value);
      json_object_object_add(res,"jobid",jsStr);
    }
    const struct tm *tm;
    size_t len;
    time_t now;
    char *tmp1;
    map *tmpStatus;
  
    now = time ( NULL );
    tm = localtime ( &now );

    tmp1 = (char*)malloc((TIME_SIZE+1)*sizeof(char));
    len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%H:%M:%SZ", tm );
    json_object *jsStr0=json_object_new_string(tmp1);
    json_object_object_add(res,"datetime",jsStr0);
    free(tmp1);
    
    switch(step){
    case 0: {
      // Create a new analyze
      maps* lenv=getMaps(conf,"lenv");
      sid=getMapFromMaps(conf,"renv","xrequest");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"request_execute_content",jsStr);
      }
      sid=getMapFromMaps(conf,"lenv","identifier");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"process_identifier",jsStr);
      }
      // Save the Execute request on disk 
      map* tmpPath=getMapFromMaps(conf,"main","tmpPath");
      map* req=getMapFromMaps(conf,"renv","xrequest");
      sid=getMapFromMaps(conf,"lenv","usid");
      char* executePath=(char*)malloc((strlen(tmpPath->value)+strlen(sid->value)+14)*sizeof(char));
      sprintf(executePath,"%s/execute_%s.xml",tmpPath->value,sid->value);
      FILE* saveExecute=fopen(executePath,"wb");
      fwrite(req->value,1,strlen(req->value)*sizeof(char),saveExecute);
      fflush(saveExecute);
      fclose(saveExecute);
      setMapInMaps(conf,"lenv","execute_file",executePath);
      free(executePath);
      break;
    }
      
    case 1: {
      // Update the execute request stored on disk at step 0,0 to modify the references used.
      if(state==1){
	maps* curs=inputs;
	xmlInitParser();
	map* xmlPath=getMapFromMaps(conf,"lenv","execute_file");
	while(curs!=NULL){
	  map* length=getMap(curs->content,"length");
	  map* useMS=getMap(curs->content,"useMapserver");
	  if(length==NULL){
	    addToMap(curs->content,"length","1");
	    length=getMap(curs->content,"length");
	  }
	  int len=atoi(length->value);
	  for(int ii=0;ii<len;ii++){
	    if(getMapArray(curs->content,"byValue",ii)!=NULL && getMapArray(curs->content,"mimeType",ii)!=NULL && useMS!=NULL && strncasecmp(useMS->value,"true",4)==0){
	      map* tmpMap=getMapArray(curs->content,"value",ii);
	      char tmpStr[100];
	      sprintf(tmpStr,"%d",strlen(tmpMap->value));
	      setMapArray(curs->content,"size",ii,tmpStr);
	      tmpMap=getMapArray(curs->content,"mimeType",ii);
	      setMapArray(curs->content,"fmimeType",ii,tmpMap->value);
	      tmpMap=getMapArray(curs->content,"cache_file",ii);
	      setMapArray(curs->content,"generated_file",ii,tmpMap->value);
	      setMapArray(curs->content,"storage",ii,tmpMap->value);
	      setReferenceUrl(conf,curs);
	      addIntToMap(curs->content,"published_id",ii+1);
	      const char *params[7];
	      int xmlLoadExtDtdDefaultValue;
	      int hasFile=-1;
	      map* xslPath=getMapFromMaps(conf,"callback","template");
	      map* filePath=getMapArray(curs->content,"ref_wfs_link",ii);
	      if(filePath==NULL)
		filePath=getMap(curs->content,"ref_wcs_link");
	      char* inputName=curs->name;
	      if(xslPath==NULL || xmlPath==NULL || filePath==NULL)
		break;
	      char *tmpParam=(char*)malloc((strlen(curs->name)+11)*sizeof(char));
	      char *tmpParam1=(char*)malloc((strlen(filePath->value)+11)*sizeof(char));
	      char tmpParam2[16];
	      sprintf(tmpParam2,"string(\"%d\")",ii);
	      setMapArray(curs->content,"href",ii,filePath->value);
	      setMapArray(curs->content,"xlink:href",ii,filePath->value);
	      tmpMap=getMapArray(curs->content,"cache_url",ii);
	      if(tmpMap!=NULL)
		setMapArray(curs->content,"xlink:href",ii,tmpMap->value);
	      else
		setMapArray(curs->content,"xlink:href",ii,filePath->value);
	      sprintf(tmpParam,"string(\"%s\")",curs->name);
	      sprintf(tmpParam1,"string(\"%s\")",filePath->value);
	      sprintf(tmpParam2,"string(\"%d\")",ii);
	      params[0]="attr";
	      params[1]=tmpParam;
	      params[2]="value";
	      params[3]=tmpParam1;//filePath->value;
	      params[4]="cnt";
	      params[5]=tmpParam2;
	      params[6]=NULL;
	      fprintf(stderr, "## XSLT PARAMETERS ATTR: %s VALUE: %s INDEX: %s\n",
		      tmpParam,tmpParam1,tmpParam2);
	      fflush(stderr);
	      xmlSubstituteEntitiesDefault(1);
	      xmlLoadExtDtdDefaultValue = 0;
	      xsltStylesheetPtr cur = NULL;
	      xmlDocPtr doc, res;
	      cur = xsltParseStylesheetFile(BAD_CAST xslPath->value);
	      doc = xmlParseFile(xmlPath->value);
	      fflush(stderr);
	      res = xsltApplyStylesheet(cur, doc, params);
	      xmlChar *xmlbuff;
	      int buffersize;
	      xmlDocDumpFormatMemory(res, &xmlbuff, &buffersize, 1);
	      // Store the executeRequest in file again
	      free(tmpParam);
	      free(tmpParam1);
	      fprintf(stderr," # Request / XSLT: %s\n",xmlbuff);
	      fflush(stderr);
	      FILE* saveExecute=fopen(xmlPath->value,"wb");
	      if(saveExecute!=NULL){
		fwrite(xmlbuff,1,buffersize,saveExecute);
		fflush(saveExecute);
		fclose(saveExecute);
	      }
	      xmlFree(xmlbuff);
	      xmlFreeDoc(doc);
	      xsltFreeStylesheet(cur);
	    }
	  }
	  addIntToMap(curs->content,"published_id",0);
	  curs=curs->next;
	}
	xmlCleanupParser();
	FILE* f0=fopen(xmlPath->value,"rb");
	if(f0!=NULL){
	  long flen;
	  char *fcontent;
	  fseek (f0, 0, SEEK_END);
	  flen = ftell (f0);
	  fseek (f0, 0, SEEK_SET);
	  fcontent = (char *) malloc ((flen + 1) * sizeof (char));
	  fread(fcontent,flen,1,f0);
	  fcontent[flen]=0;
	  fclose(f0);
	  map *schema=getMapFromMaps(conf,"database","schema");
	  map* sid=getMapFromMaps(conf,"lenv","usid");
	  char *req=(char*)malloc((flen+strlen(schema->value)+strlen(sid->value)+66)*sizeof(char));
	  sprintf(req,"UPDATE %s.services set request_execute_content=$$%s$$ WHERE uuid=$$%s$$",schema->value,fcontent,sid->value);
#ifdef RELY_ON_DB
	  execSql(conf,1,req);
#endif
	  free(fcontent);
	  free(req);
	}
      }

      // Fetching data inputs
      maps* curs=inputs;
      dumpMaps(curs);
      const char *keys[11][2]={
	{
	  "xlink:href",
	  "ref_download_link"
	},
	{
	  "cache_file",
	  "cachefile"
	},
	{
	  "fmimeType",
	  "mimetype"
	},
	{
	  "size",
	  "size"
	},
	{
	  "ref_wms_link",
	  "ref_wms_link"
	},
	{
	  "ref_wfs_link",
	  "ref_wfs_link"
	},
	{
	  "ref_wcs_link",
	  "ref_wcs_link"
	},
	{
	  "ref_wcs_link",
	  "ref_wcs_link"
	},
	{
	  "ref_wcs_preview_link",
	  "ref_wcs_preview_link"
	},
	{
	  "geodatatype",
	  "datatype"
	},
	{
	  "wgs84_extent",
	  "boundingbox"
	}	
      };
      json_object *res1=json_object_new_object();
      while(curs!=NULL){
	if(getMap(curs->content,"length")==NULL){
	  addToMap(curs->content,"length","1");
	}
	map* length=getMap(curs->content,"length");
	int len=atoi(length->value);
	json_object *res3;
	int hasRef=-1;
	for(int ii=0;ii<len;ii++){
	  map* tmpMap=getMapArray(curs->content,"cache_file",ii);
	  sid=getMapArray(curs->content,"ref_wms_link",ii);
	  json_object *res2=json_object_new_object();
	  if(tmpMap!=NULL){
	    if(sid==NULL){
	      setMapArray(curs->content,"generated_file",ii,tmpMap->value);
	      setMapArray(curs->content,"storage",ii,tmpMap->value);
	    }
	    struct stat buf;
	    char timeStr[ 100 ] = "";
	    if (stat(tmpMap->value, &buf)==0){
	      strftime(timeStr, 100, "%d-%m-%Y %H:%M:%S", localtime( &buf.st_mtime));
	      json_object *jsStr=json_object_new_string(timeStr);
	      json_object_object_add(res2,"creation_date",jsStr);
	    }
	    tmpMap=getMapArray(curs->content,"fmimeType",ii);
	    if(tmpMap!=NULL){
	      setMapArray(curs->content,"mimeType",ii,tmpMap->value);
	    }
	    setReferenceUrl(conf,curs);
	  }else{	  
	  }
	  addIntToMap(curs->content,"published_id",ii+1);
	  int i=0;
	  for(;i<11;i++){
	    sid=getMapArray(curs->content,keys[i][0],ii);
	    if(sid!=NULL){
	      json_object *jsStr=json_object_new_string(sid->value);
	      json_object_object_add(res2,keys[i][1],jsStr);
	      if(i==0){
		hasRef=1;
		json_object *jsStr1=json_object_new_string(getProvenance(conf,sid->value));
		json_object_object_add(res2,"dataOrigin",jsStr1);
	      }
	    }
	  }
	  if(len>1){
	    if(ii==0)
	      res3=json_object_new_array();
	    json_object_array_add(res3,res2);
	  }else
	    res3=res2;
	}
	if(hasRef<0)
	  json_object_put(res3);
	else{
	  json_object_object_add(res1,curs->name,json_object_get(res3));
	  json_object_put(res3);
	}
	addIntToMap(curs->content,"published_id",0);
	curs=curs->next;
      }
      json_object_object_add(res,"inputs",res1);
      break;
    }
      
    case 2: {
      // Uploading data input to cluster
      maps* in=getMaps(conf,"uploadQueue");
      if(in!=NULL){
	maps* curs=in;
	map* length=getMapFromMaps(in,"uploadQueue","length");
	if(length!=NULL){
	  json_object *res1=json_object_new_object();
	  int limit=atoi(length->value);
	  int i=0;
	  maps* uploadQueue=getMaps(in,"uploadQueue");
	  map* tmp=uploadQueue->content;
	  for(;i<limit;i++){
	    map* tmp0=getMapArray(tmp,"input",i);
	    map* tmp1=getMapArray(tmp,"localPath",i);
	    map* tmp2=getMapArray(tmp,"targetPath",i);
	    if(tmp0!=NULL && tmp1!=NULL && tmp2!=NULL){
	      json_object *res2=json_object_new_object();
	      json_object *jsStr=json_object_new_string(tmp1->value);
	      json_object_object_add(res2,"local_path",jsStr);
	      jsStr=json_object_new_string(tmp2->value);
	      json_object_object_add(res2,"target_path",jsStr);
	      json_object *res4=json_object_object_get(res1,tmp0->value);
	      if(json_object_is_type(res4,json_type_null)){
		json_object_object_add(res1,tmp0->value,res2);
	      }else{
		if(json_object_is_type(res4,json_type_object) && !json_object_is_type(res4, json_type_array)){
		  json_object *res3=json_object_new_array();
		  json_object_array_add(res3,json_object_get(res4));
		  json_object_array_add(res3,res2);
		  json_object_object_del(res1,tmp0->value);
		  json_object_object_add(res1,tmp0->value,res3);
		}else
		  json_object_array_add(res4,res2);
	      }
	    }
	  }
	  json_object_object_add(res,"inputs",res1);
	}
      }
      break;
    }
      
    case 3: {
      // Generating job script
      sid=getMapFromMaps(conf,"lenv","local_script");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"script",jsStr);
      }
      break;
    }
      
    case 4: {
      // Submitting job to cluster
      sid=getMapFromMaps(conf,"lenv","remote_script");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"script",jsStr);
      }
      break;
    }
      
    case 5: {
      // Downloading process outputs from cluster
      maps* curs=outputs;
      dumpMaps(curs);
      const char *keys[10][2]={
	{
	  "Reference",
	  "ref"
	},
	{
	  "generated_file",
	  "cachefile"
	},
	{
	  "mimeType",
	  "mimetype"
	},
	{
	  "size",
	  "size"
	},
	{
	  "geodatatype",
	  "datatype"
	},
	{
	  "wgs84_extent",
	  "boundingbox"
	},
	{
	  "ref_wms_link",
	  "ref_wms_link"
	},
	{
	  "ref_wcs_link",
	  "ref_wcs_link"
	},
	{
	  "ref_wcs_preview_link",
	  "ref_wcs_preview_link"
	},
	{
	  "ref_wfs_link",
	  "ref_wfs_link"
	}	
      };
      const char* specifics[5][2]={
	{
	  "download_link",
	  "ref_download_link"
	},
	{
	  "wms_link",
	  "ref_wms_link"
	},
	{
	  "wfs_link",
	  "ref_wfs_link"
	},
	{
	  "wcs_link",
	  "ref_wcs_link"
	},
	{
	  "wcs_link",
	  "ref_wcs_preview_link"
	}
      };
      json_object *res1=json_object_new_object();
      while(curs!=NULL){	
	json_object *res2=json_object_new_object();
	int i=0;
	int hasRef=-1;
	for(;i<10;i++){
	  sid=getMap(curs->content,keys[i][0]);
	  if(sid!=NULL){
	    json_object *jsStr=json_object_new_string(sid->value);
	    json_object_object_add(res2,keys[i][1],jsStr);
	    if(i==0){
	      hasRef=1;
	      json_object_object_add(res2,"ref_download_link",jsStr);
	    }
	    if(i==1){
	      struct stat buf;
	      char timeStr[ 100 ] = "";
	      if (stat(sid->value, &buf)==0){
		strftime(timeStr, 100, "%d-%m-%Y %H:%M:%S", localtime( &buf.st_mtime));
		json_object *jsStr=json_object_new_string(timeStr);
		json_object_object_add(res2,"creation_date",jsStr);
	      }
	    }
	  }
	}
	if(hasRef>0)
	  json_object_object_add(res1,curs->name,res2);
	else{
	  maps* curs0=curs->child;
	  int i=0;
	  int bypass=-1;
	  for(i=0;i<5;i++){
	    maps* specificMaps;
	    if((specificMaps=getMaps(curs0,specifics[i][0]))!=NULL){
	      int hasRef0=-1;
	      int i0=0;
	      for(;i0<6;i0++){
		sid=getMap(specificMaps->content,keys[i0][0]);
		if(sid!=NULL){
		  json_object *jsStr=json_object_new_string(sid->value);
		  if(i0==0){
		    json_object_object_add(res2,specifics[i][1],jsStr);
		  }
		  else
		    json_object_object_add(res2,keys[i0][1],jsStr);
		  hasRef0=1;
		  bypass=1;
		  if(i==1){
		    struct stat buf;
		    char timeStr[ 100 ] = "";
		    if (stat(sid->value, &buf)==0){
		      strftime(timeStr, 100, "%d-%m-%Y %H:%M:%S", localtime( &buf.st_mtime));
		      json_object *jsStr=json_object_new_string(timeStr);
		      json_object_object_add(res2,"creation_date",jsStr);
		    }
		  }
		}
	      }	      
	    }
	  }
	  if(bypass<0)
	    while(curs0!=NULL){
	      json_object *res3=json_object_new_object();
	      int i0=0;
	      int hasRef0=-1;
	      for(;i0<10;i0++){
		sid=getMap(curs0->content,keys[i0][0]);
		if(sid!=NULL){
		  json_object *jsStr=json_object_new_string(sid->value);
		  json_object_object_add(res3,keys[i0][1],jsStr);
		  hasRef0=1;
		}
	      }
	      if(hasRef0<0)
		json_object_put(res3);
	      else
		json_object_object_add(res2,curs0->name,res3);
	      curs0=curs0->next;
	    }
	  json_object_object_add(res1,curs->name,res2);
	}
	curs=curs->next;
      }
      json_object_object_add(res,"outputs",res1);
      break;
    }
      
    case 6: {
      // Finalize HPC
      const char *keys[6][2]={
	{
	  //"SubmitTime",
	  "Submit",
	  "hpc_submission_date"
	},
	{
	  "JobId",
	  "hpc_job_identifier"
	},
	{
	  "JobName",
	  "hpc_job_name"
	},
	{
	  //"StartTime",
	  "Start",
	  "hpc_start_date"
	},
	{
	  //"EndTime",
	  "End",
	  "hpc_end_date"
	},
	{
	  //"JobState",
	  "State",
	  "hpc_status"
	}	
      };
      int i=0;
      if(getMaps(conf,"henv")!=NULL){
	for(i=0;i<6;i++){
	  sid=getMapFromMaps(conf,"henv",keys[i][0]);
	  if(sid!=NULL){
	    json_object *jsStr=json_object_new_string(sid->value);
	    json_object_object_add(res,keys[i][1],jsStr);
	  }
	}
      }
      if((sid=getMapFromMaps(conf,"henv","billing_nb_cpu"))!=NULL){
        json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"hpc_cpu_usage",jsStr);
      }else{
	json_object *jsStr=json_object_new_string("1");
	json_object_object_add(res,"hpc_cpu_usage",jsStr);
      }
      json_object *jsStr=json_object_new_string("succeeded");
      json_object_object_add(res,"wps_status",jsStr);
      break;
    }
      
    case 7: {
      // Error or Dismiss
      sid=getMapFromMaps(conf,"lenv","message");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"message",jsStr);
      }
      json_object *jsStr;
      if(state==1)
	jsStr=json_object_new_string("dismissed");
      else
	jsStr=json_object_new_string("failed");
      json_object_object_add(res,"wps_status",jsStr);
      break;
    }
    others: {
	break;
      }
    }

    if(local_arguments==NULL)
      local_arguments=(local_params**)malloc(sizeof(local_params*));
    else
      local_arguments=(local_params**)realloc(local_arguments,(nbThreads+1)*sizeof(local_params*));
    local_arguments[nbThreads]=(local_params*)malloc(MAPS_SIZE+MAP_SIZE+sizeof(json_object*)+(2*sizeof(int)));	
    local_arguments[nbThreads]->conf=conf;
    local_arguments[nbThreads]->url=url;
    local_arguments[nbThreads]->res=res;
    local_arguments[nbThreads]->step=step;
    local_arguments[nbThreads]->state=state;
    if(myThreads==NULL)
      myThreads=(pthread_t*)malloc((nbThreads+1)*sizeof(pthread_t));
    else
      myThreads=(pthread_t*)realloc(myThreads,(nbThreads+1)*sizeof(pthread_t));
    if(pthread_create(&myThreads[nbThreads], NULL, _invokeCallback, (void*)local_arguments[nbThreads])==-1){
      setMapInMaps(conf,"lenv","message",_("Unable to create a new thread"));
      return false;
    }
    nbThreads++;
    return true;
  }

  /**
   * Wait for the threads to end then, clean used memory.
   */
  void cleanupCallbackThreads(){
    int i=0;
    for(i=0;i<nbThreads;i++){
      pthread_join(myThreads[i],NULL);
      free(local_arguments[i]);
    }
    free(local_arguments);
    free(myThreads);
  }

#ifdef __cplusplus
}
#endif
