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

#ifdef __cplusplus
extern "C" {
#endif

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
  bool invokeCallback(maps* m,maps* inputs,maps* outputs,int step,int state){
    map* url=getMapFromMaps(m,"callback","url");
    if(url==NULL)
      return false;

    maps* lenv=getMaps(m,"lenv");

    json_object *res=json_object_new_object();

    map* sid=getMapFromMaps(m,"lenv","usid");
    if(sid!=NULL){
      json_object *jsStr=json_object_new_string(sid->value);
      json_object_object_add(res,"jobid",jsStr);
    }
    switch(step){
    case 0: {
      // Create a new analyze
      maps* lenv=getMaps(m,"lenv");
      sid=getMapFromMaps(m,"lenv","xrequest");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"request_execute_content",jsStr);
      }
      sid=getMapFromMaps(m,"lenv","identifier");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"process_identifier",jsStr);
      }
      break;
    }
    case 1: {
      // Fetching data inputs
      maps* curs=inputs;
      
      char *keys[4][2]={
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
	}
      };
      json_object *res1=json_object_new_object();
      while(curs!=NULL){
	map* tmpMap=getMap(curs->content,"cache_file");
	if(tmpMap!=NULL){
	  addToMap(curs->content,"generated_file",tmpMap->value);
	  tmpMap=getMap(curs->content,"fmimeType");
	  if(tmpMap!=NULL){
	    addToMap(curs->content,"mimeType",tmpMap->value);
	  }
	  setReferenceUrl(m,curs);
	  //outputMapfile(m,curs);
	  dumpMaps(curs);
	}
	json_object *res2=json_object_new_object();
	int i=0;
	int hasRef=-1;
	for(;i<4;i++){
	  sid=getMap(curs->content,keys[i][0]);
	  if(sid!=NULL){
	    json_object *jsStr=json_object_new_string(sid->value);
	    json_object_object_add(res2,keys[i][1],jsStr);
	    if(i==0)
	      hasRef=1;
	  }
	}
	if(hasRef<0)
	  json_object_put(res2);
	else
	  json_object_object_add(res1,curs->name,res2);
	curs=curs->next;
      }
      json_object_object_add(res,"inputs",res1);
      json_object* in=mapsToJson(inputs);
      if(in!=NULL){
	//json_object_object_add(res,"inputs",in);
	json_object_put(in);
      }
      break;
    }
    case 2: {
      // Uploading data input to cluster
      maps* in=getMaps(m,"uploadQueue");
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
      sid=getMapFromMaps(m,"lenv","local_script");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"script",jsStr);
      }
      break;
    }
    case 4: {
      // Submitting job to cluster
      sid=getMapFromMaps(m,"lenv","remote_script");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"script",jsStr);
      }
      break;
    }
    case 5: {
      // Downloading process outputs from cluster
      json_object* in=mapsToJson(outputs);
      if(in!=NULL){
	//json_object_object_add(res,"outputs",in);
	json_object_put(in);
      }
      break;
    }
    case 6: {
      // Finalize HPC
      sid=getMapFromMaps(m,"lenv","local_script");
      if(sid!=NULL){
	json_object *jsStr=json_object_new_string(sid->value);
	json_object_object_add(res,"inputs",jsStr);
      }
      break;
    }
    case 7: {
      // Error or Dismiss
      sid=getMapFromMaps(m,"lenv","message");
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
    fprintf(stderr,"************************* %s %d\n\n",__FILE__,__LINE__);
    fflush(stderr);
    HINTERNET hInternet,res1;
    hInternet=InternetOpen("ZooWPSClient\0",
			   INTERNET_OPEN_TYPE_PRECONFIG,
			   NULL,NULL, 0);
    if(!CHECK_INET_HANDLE(hInternet)){
      InternetCloseHandle (&hInternet);
      return false;
    }
    fprintf(stderr," * JSON: [%s] \n",json_object_to_json_string_ext(res,JSON_C_TO_STRING_PLAIN));
    fprintf(stderr," * URL: %s%d_%d/ \n\n",url->value,step,state);
    fflush(stderr);
    char *URL=(char*)malloc((strlen(url->value)+5)*sizeof(char));
    sprintf(URL,"%s%d_%d/",url->value,step,state);
    hInternet.waitingRequests[0] = zStrdup(URL);
    free(URL);
    const char* jsonStr=json_object_to_json_string_ext(res,JSON_C_TO_STRING_PLAIN);
    res1 = InternetOpenUrl (&hInternet,
		     hInternet.waitingRequests[0],
		     (char*)jsonStr, strlen(jsonStr),
		     INTERNET_FLAG_NO_CACHE_WRITE,
		     0);
    //curl_easy_setopt(hInternet.ihandle[hInternet.nb].handle, CURLOPT_VERBOSE, 1);
    processDownloads(&hInternet);
    char *tmp = (char *) malloc ((hInternet.ihandle[0].nDataLen + 1)
				 * sizeof (char));
    if (tmp == NULL)
      {
	setMapInMaps(m,"lenv","message",_("Unable to allocate memory"));
	setMapInMaps(m,"lenv","code","InternalError");
	return false;
      }
    size_t bRead;
    InternetReadFile (hInternet.ihandle[0],
		      (LPVOID) tmp,
		      hInternet.
		      ihandle[0].nDataLen,
		      &bRead);
    tmp[hInternet.ihandle[0].nDataLen] = 0;
    fprintf(stderr,"Result: \n%s\n\n",tmp);
    fprintf(stderr,"************************* %s %d\n\n",__FILE__,__LINE__);
    fflush(stderr);
    free(tmp);
    json_object_put(res);
    InternetCloseHandle(&hInternet);
    return true;
  }

#ifdef __cplusplus
}
#endif
