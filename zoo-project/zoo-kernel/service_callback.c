/*
 * Author : Gérald FENOY
 *
 *  Copyright 2017 GeoLabs SARL. All rights reserved.
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

#include "service_callback.h"
#include "service_json.h"
#include "service_internal_ms.h"
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

  int nbThreads=0;
  int cStep=0;
  pthread_t* myThreads=NULL;
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
   * Verify if the URL should use a shared cache or not.
   *
   * In case the security section contains a key named "shared", then if the
   * domain listed in the shared key are contained in the url given as parameter
   * then it return "SHARED" in other cases, it returns "OTHER".
   *
   * @param conf the main configuration file maps
   * @param url the URL to evaluate
   * @return a string "SHARED" in case the host is in a domain listed in the
   * shared key, "OTHER" in other cases.
   */
  char* getProvenance(maps* conf,const char* url){
    map* sharedCache=getMapFromMaps(conf,"security","shared");
    if(sharedCache!=NULL){
      char *res=NULL;
      char *hosts=sharedCache->value;
      char *curs=strtok(hosts,",");
      while(curs!=NULL){
	if(strstr(url,curs)==NULL)
	  res="OTHER";
	else{
	  res="SHARED";
	  return res;
	}
      }
      return res;
    }
    return "OTHER";
  }

  /**
   * Practically invoke the callback, meaning sending the HTTP POST request.
   * 
   * @param args local_params containing all the variables required
   */
  void* _invokeCallback(void* args){
    local_params* arg=(local_params*)args;
    HINTERNET hInternet,res1;
    hInternet=InternetOpen("ZooWPSClient\0",
			   INTERNET_OPEN_TYPE_PRECONFIG,
			   NULL,NULL, 0);
    if(!CHECK_INET_HANDLE(hInternet)){
      InternetCloseHandle (&hInternet);
      return false;
    }
    char *URL=(char*)malloc((strlen(arg->url->value)+5)*sizeof(char));
    sprintf(URL,"%s%d_%d/",arg->url->value,arg->step,arg->state);
    const char* jsonStr=json_object_to_json_string_ext(arg->res,JSON_C_TO_STRING_PLAIN);
    hInternet.waitingRequests[0] = zStrdup(URL);
    free(URL);
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST PARAMETERS\n",pthread_self(),__FILE__,__LINE__);
    fprintf(stderr," * JSON: [%s] \n",jsonStr);
    fprintf(stderr," * URL: %s/ \n\n",hInternet.waitingRequests[0]);
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST PARAMETERS\n",pthread_self(),__FILE__,__LINE__);
    while( cStep!=7 &&
	   ( cStep!=arg->step || (arg->state!=0 && steps[arg->step][0]==false) )
	   ){
      sleep(1);
    }
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST START\n\n",pthread_self(),__FILE__,__LINE__);
    int i=0;
    for(i=0;i<7;i++){
      fprintf(stderr,"%d) %d %d\n",i,steps[i][0],steps[i][1]);
      fflush(stderr);
    }
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST START\n",pthread_self(),__FILE__,__LINE__);
    fflush(stderr);
    res1 = InternetOpenUrl (&hInternet,
			    hInternet.waitingRequests[0], 
			    (char*)jsonStr, strlen(jsonStr),
			    INTERNET_FLAG_NO_CACHE_WRITE,
			    0);
    AddHeaderEntries(&hInternet,arg->conf);
    //curl_easy_setopt(hInternet.ihandle[hInternet.nb].handle, CURLOPT_VERBOSE, 1);x
    processDownloads(&hInternet);
    fprintf(stderr,"************************* From thread %d %s %d: REQUEST END\n\n",pthread_self(),__FILE__,__LINE__);
    fflush(stderr);
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
    if(cStep==0 || cStep==6 || arg->state==1)
      cStep=arg->step+1;
    steps[arg->step][arg->state]=true;
    fprintf(stderr,"************************* From thread %d %s %d: RESPONSE CONTENT\n",pthread_self(),__FILE__,__LINE__);
    for(i=0;i<7;i++){
      fprintf(stderr,"%d) %d %d\n",i,steps[i][0],steps[i][1]);
    }
    fprintf(stderr,"Result: \n%s\n\n",tmp);
    fprintf(stderr,"************************* From thread %d %s %d\n\n",pthread_self(),__FILE__,__LINE__);
    fflush(stderr);
    free(tmp);
    free(args);
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

    len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );
    json_object *jsStr0=json_object_new_string(tmp1);
    json_object_object_add(res,"datetime",jsStr0);

    switch(step){
    case 0: {
      // Create a new analyze
      maps* lenv=getMaps(conf,"lenv");
      sid=getMapFromMaps(conf,"lenv","xrequest");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"request_execute_content",jsStr);
      }
      sid=getMapFromMaps(conf,"lenv","identifier");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"process_identifier",jsStr);
      }
      break;
    }
    case 1: {
      // Fetching data inputs
      maps* curs=inputs;
      char *keys[8][2]={
	{
	  "href",
	  "ref"
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
	  "ref_wcs_link",
	  "ref_wcs_link"
	},
	{
	  "ref_wfs_link",
	  "ref_wfs_link"
	},
	{
	  "geodatatype",
	  "datatype"
	}	
      };
      json_object *res1=json_object_new_object();
      while(curs!=NULL){
	map* tmpMap=getMap(curs->content,"cache_file");
	sid=getMap(curs->content,"ref_wms_link");
	json_object *res2=json_object_new_object();
	if(tmpMap!=NULL && sid==NULL){
	  addToMap(curs->content,"generated_file",tmpMap->value);
	  struct stat buf;
	  char timeStr[ 100 ] = "";
	  if (stat(tmpMap->value, &buf)==0){
	    strftime(timeStr, 100, "%d-%m-%Y %H:%M:%S", localtime( &buf.st_mtime));
	    json_object *jsStr=json_object_new_string(timeStr);
	    json_object_object_add(res2,"creation_date",jsStr);
	  }
	  tmpMap=getMap(curs->content,"fmimeType");
	  if(tmpMap!=NULL){
	    addToMap(curs->content,"mimeType",tmpMap->value);
	  }
	  setReferenceUrl(conf,curs);
	  //outputMapfile(conf,curs);
	  dumpMaps(curs);
	}
	int i=0;
	int hasRef=-1;
	for(;i<8;i++){
	  sid=getMap(curs->content,keys[i][0]);
	  if(sid!=NULL){
	    json_object *jsStr=json_object_new_string(sid->value);
	    json_object_object_add(res2,keys[i][1],jsStr);
	    if(i==0){
	      hasRef=1;
	      json_object *jsStr1=json_object_new_string(getProvenance(conf,url->value));
	      json_object_object_add(res2,"dataOrigin",jsStr1);
	    }
	  }
	}
	if(hasRef<0)
	  json_object_put(res2);
	else
	  json_object_object_add(res1,curs->name,res2);
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
	  json_object *res2=json_object_new_object();
	  int limit=atoi(length->value);
	  int i=0;
	  maps* uploadQueue=getMaps(in,"uploadQueue");
	  map* tmp=uploadQueue->content;
	  for(;i<limit;i++){
	    map* tmp0=getMapArray(tmp,"input",i);
	    map* tmp1=getMapArray(tmp,"localPath",i);
	    map* tmp2=getMapArray(tmp,"targetPath",i);
	    if(tmp0!=NULL && tmp1!=NULL && tmp2!=NULL){
	      json_object *jsStr=json_object_new_string(tmp1->value);
	      json_object_object_add(res2,"local_path",jsStr);
	      jsStr=json_object_new_string(tmp2->value);
	      json_object_object_add(res2,"target_path",jsStr);
	      json_object_object_add(res1,tmp0->value,res2);
	    }
	  }
	  json_object_object_add(res,"inputs",res1);
	}
	//json_object_object_add(res,"inputs",in);
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
      //json_object* in=mapsToJson(outputs);
      dumpMaps(outputs);
      maps* curs=outputs;
      char *keys[8][2]={
	{
	  "Reference",
	  "ref"
	},
	{
	  "storage",
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
	  "ref_wcs_link",
	  "ref_wcs_link"
	},
	{
	  "ref_wfs_link",
	  "ref_wfs_link"
	},
	{
	  "geodatatype",
	  "datatype"
	}	
      };
      json_object *res1=json_object_new_object();
      while(curs!=NULL){	
	map* tmpMap=getMap(curs->content,"cache_file");
	sid=getMap(curs->content,"ref_wms_link");
	json_object *res2=json_object_new_object();
	int i=0;
	int hasRef=-1;
	for(;i<8;i++){
	  sid=getMap(curs->content,keys[i][0]);
	  if(sid!=NULL){
	    json_object *jsStr=json_object_new_string(sid->value);
	    json_object_object_add(res2,keys[i][1],jsStr);
	    if(i==0)
	      hasRef=1;
	  }
	}
	if(hasRef>0)
	  json_object_object_add(res1,curs->name,res2);
	else{
	  maps* curs0=curs->child;
	  while(curs0!=NULL){
	    json_object *res3=json_object_new_object();
	    int i0=0;
	    int hasRef0=-1;
	    for(;i0<8;i0++){
	      sid=getMap(curs0->content,keys[i0][0]);
	      if(sid!=NULL){
		json_object *jsStr=json_object_new_string(sid->value);
		json_object_object_add(res3,keys[i0][1],jsStr);
		//if(i0==0)
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
      sid=getMapFromMaps(conf,"lenv","local_script");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"inputs",jsStr);
      }
      break;
    }
    case 7: {
      // Error or Dismiss
      sid=getMapFromMaps(conf,"lenv","message");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"message",jsStr);
      }
      json_object *jsStr=json_object_new_string("failed");
      json_object_object_add(res,"wps_status",jsStr);
      break;
    }
    others: {
	break;
      }
    }
   
    local_params* argumentsA=(local_params*)malloc(MAPS_SIZE+MAP_SIZE+sizeof(json_object*)+(2*sizeof(int)));
    argumentsA->conf=conf;
    argumentsA->url=url;
    argumentsA->res=res;
    argumentsA->step=step;
    argumentsA->state=state;
    //pthread_t p1;
    if(myThreads==NULL)
      myThreads=(pthread_t*)malloc((nbThreads+1)*sizeof(pthread_t));
    else
      myThreads=(pthread_t*)realloc(myThreads,(nbThreads+1)*sizeof(pthread_t));
    if(pthread_create(&myThreads[nbThreads], NULL, _invokeCallback, (void*)argumentsA)==-1){
      setMapInMaps(conf,"lenv","message",_("Unable to create a new thread"));
      return false;
    }
    //free(argumentsA);
    nbThreads++;
    return true;
  }

  void cleanupCallbackThreads(){
    int i=0;
    for(i=0;i<nbThreads;i++){
      pthread_join(myThreads[i],NULL);
    }
    free(myThreads);
  }

#ifdef __cplusplus
}
#endif
