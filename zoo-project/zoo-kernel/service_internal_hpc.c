/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2017 GeoLabs SARL
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
 *
 */

#include "service_internal_hpc.h"
#include "response_print.h"
#include "server_internal.h"
#include "service_callback.h"
#include "mimetypes.h"
#include <sys/un.h>

typedef struct {
  maps* conf;
  char* local_file;
  char* target_file;
} local_params;

/**
 * Add nested outputs to every outputs that is geographic format 
 * @see isGeographic
 * @param s the service current definition
 */ 
void addNestedOutputs(service** s){
  if((*s)==NULL){
    return;
  }    
  if(*s==NULL || (*s)->outputs==NULL || (*s)->content==NULL){
    return;
  }
  elements *out=(*s)->outputs;
  elements* cur=out;
  map* serviceType=getMap((*s)->content,"ServiceType");
  if(strncmp(serviceType->value,"HPC",3)!=0)
    return;
  while(cur!=NULL && cur->defaults!=NULL){
    map* mimeType=getMap(cur->defaults->content,"mimeType");
    map* useMS=getMap(cur->defaults->content,"useMapserver");
    if(mimeType!=NULL && useMS!=NULL && strncasecmp(useMS->value,"true",4)==0){
      int geo=isGeographic(mimeType->value);
      if(geo>0){
	elements *tmp[3]={
	  dupElements(cur),
	  dupElements(cur),
	  dupElements(cur)
	};
	const char *geoLink="wcs_link";
	if(geo==2){
	  geoLink="wfs_link";
	}
	int i=0;
	for(;i<3;i++){
	  if(tmp[i]->next!=NULL){
	    freeElements(&tmp[i]->next);
	    free(tmp[i]->next);
	    tmp[i]->next=NULL;
	  }
	  free(tmp[i]->name);
	  if(tmp[i]->format!=NULL)
	    free(tmp[i]->format);
	  tmp[i]->format=zStrdup("ComplexData");
	  freeMap(&tmp[i]->content);
	  free(tmp[i]->content);
	  tmp[i]->content=NULL;
	  switch(i){
	  case 0:
	    tmp[i]->name=zStrdup("download_link");
	    tmp[i]->content=createMap("Title",_("Download link"));
	    addToMap(tmp[i]->content,"Abstract",_("The download link"));
	    addToMap(tmp[i]->defaults->content,"useMapserver","false");
	    if(tmp[i]->supported!=NULL){
	      freeIOType(&tmp[i]->supported);
	      free(tmp[i]->supported);
	      tmp[i]->supported=NULL;
	    }
	    break;
	  case 1:
	    tmp[i]->name=zStrdup("wms_link");
	    tmp[i]->content=createMap("Title",_("WMS link"));
	    addToMap(tmp[i]->content,"Abstract",_("The WMS link"));
	    if(tmp[i]->supported!=NULL && tmp[i]->supported->next!=NULL){
	      freeIOType(&tmp[i]->supported->next);
	      free(tmp[i]->supported->next);
	      tmp[i]->supported->next=NULL;
	    }else{
	      if(tmp[i]->supported!=NULL)
		addToMap(tmp[i]->supported->content,"useMapserver","true");
	      addToMap(tmp[i]->defaults->content,"useMapserver","true");
	    }
	    break;
	  case 2:
	    if(geo==2){
	      tmp[i]->name=zStrdup("wfs_link");
	      tmp[i]->content=createMap("Title",_("WFS link"));
	      addToMap(tmp[i]->content,"Abstract",_("The WFS link"));
	    }else{
	      tmp[i]->name=zStrdup("wcs_link");
	      tmp[i]->content=createMap("Title",_("WCS link"));
	      addToMap(tmp[i]->content,"Abstract",_("The WCS link"));
	    }
	    if(tmp[i]->supported!=NULL && tmp[i]->supported->next!=NULL &&
	       tmp[i]->supported->next->content!=NULL){
	      freeIOType(&tmp[i]->supported);
	      free(tmp[i]->supported);
	      tmp[i]->supported=NULL;
	      tmp[i]->supported=createIoType();
	      iotype* cnext=cur->supported->next;
	      tmp[i]->supported->content=createMap(cnext->content->name,cnext->content->value);
	      addMapToMap(&tmp[i]->supported->content,cnext->content->next);
	      addToMap(tmp[i]->supported->content,"useMapserver","true");
	    }else
	      addToMap(tmp[i]->defaults->content,"useMapserver","true");
	    break;
	  }
	}
	addToElements(&cur->child,tmp[0]);
	addToElements(&cur->child,tmp[1]);
	addToElements(&cur->child,tmp[2]);
	free(cur->format);
	cur->format=NULL;
	if(cur->defaults!=NULL){
	  freeIOType(&cur->defaults);
	  free(cur->defaults);
	  cur->defaults=NULL;
	}
	if(cur->supported!=NULL){
	  freeIOType(&cur->supported);
	  free(cur->supported);
	  cur->supported=NULL;
	}
	freeElements(&tmp[2]);
	free(tmp[2]);
	freeElements(&tmp[1]);
	free(tmp[1]);
	freeElements(&tmp[0]);
	free(tmp[0]);
	//addToMap(cur->content,"internal","true");
      }      
    }else{
      if(mimeType!=NULL){
        elements *tmp=dupElements(cur);
        tmp->name=zStrdup("download_link");
        tmp->content=createMap("Title",_("Download link"));
        addToMap(tmp->content,"Abstract",_("The download link"));
        addToMap(tmp->defaults->content,"useMapserver","false");
        if(tmp->supported!=NULL){
          freeIOType(&tmp->supported);
          free(tmp->supported);
          tmp->supported=NULL;
        }
        addToElements(&cur->child,tmp);
        free(cur->format);
        cur->format=NULL;
        if(cur->defaults!=NULL){
          freeIOType(&cur->defaults);
          free(cur->defaults);
          cur->defaults=NULL;
        }
        if(cur->supported!=NULL){
          freeIOType(&cur->supported);
          free(cur->supported);
          cur->supported=NULL;
        }
        freeElements(&tmp);
        free(tmp);
      }
    }
    cur=cur->next;
  }
  //dumpElements((*s)->outputs);
}

/**
 * Acquire a read lock on every files used as input for executing a service.
 * @param conf the main configuration file map
 * @return 0 if every file can be locked, -1 if one lock has failed.
 */
int addReadLocks(maps** conf){
  map* queueLengthMap=getMapFromMaps(*conf,"uploadQueue","length");
  maps* queueMaps=getMaps(*conf,"uploadQueue");
  if(queueLengthMap!=NULL){
    int cnt=atoi(queueLengthMap->value);
    int i=0;
    for(i=0;i<cnt;i++){
      map* argv[2]={
	getMapArray(queueMaps->content,"input",i),
	getMapArray(queueMaps->content,"localPath",i)
      };
      zooLock* lck;
      if((lck=lockFile(*conf,argv[1]->value,'r'))==NULL){
	const char* templateStr=_("Unable to lock the file for %s in read mode.");
	char *tmpMessage=(char*)malloc((strlen(templateStr)+strlen(argv[0]->value)+1)*sizeof(char));
	sprintf(tmpMessage,templateStr,argv[0]->value);
	setMapInMaps(*conf,"lenv","message",tmpMessage);
	free(tmpMessage);
	return -1;
      }else{
	if(zoo_file_locks_cnt==0){
	  zoo_file_locks=(zooLock**)malloc(sizeof(zooLock*));
	}
	else{
	  zoo_file_locks=(zooLock**)realloc(zoo_file_locks,(zoo_file_locks_cnt+1)*sizeof(zooLock*));
	}
	zoo_file_locks[zoo_file_locks_cnt]=lck;
	zoo_file_locks_cnt++;
      }
    }
  }
  return 0;
}

/**
 * Remove all read locks set for files used as input for executing the service.
 * @param conf the main configuration maps pointer
 * @return 0 in case of success, -1 if any error occured. In case of error, one
 * can refer to the message map array from the lenv section.
 */
int removeReadLocks(maps** conf){
  int res=0;
  int nberr=0;
  map* queueLengthMap=getMapFromMaps(*conf,"uploadQueue","length");
  maps* queueMaps=getMaps(*conf,"uploadQueue");
  if(queueLengthMap!=NULL){
    int cnt=atoi(queueLengthMap->value);
    int i=0;
    for(i=0;i<cnt;i++){
      if(unlockFile(*conf,zoo_file_locks[i])<1){
	map* argv=getMapArray(queueMaps->content,"input",i);
	const char* templateStr=_("Unable to unlock the file for %s after execution.");
	char *tmpMessage=(char*)malloc((strlen(templateStr)+strlen(argv->value)+1)*sizeof(char));
	sprintf(tmpMessage,templateStr,argv->value);
	maps* lenv=getMaps(*conf,"lenv");
	setMapArray(lenv->content,"message",nberr,tmpMessage);
	free(tmpMessage);
	res=-1;
	nberr++;
      }
    }
  }
  free(zoo_file_locks);
  return res;
}

/**
 * Get the section name depending on number of features and/or pixels of each
 * inputs and the threshold defined in a section.
 * It supposes that your inputs has been published using MapServer support, 
 * implying that the number of features (nb_features), respectively pixels 
 * (nb_pixels), are defined. The section, identified by confId, should contain
 * preview_max_features and preview_max_pixels defining the threshold values.
 * @param conf the main configuration file maps pointer
 * @param inputs the inputs maps pointer
 * @param confId the section identifier
 * @return "preview_conf" in case the numbers are lower than the threshold,
 * "fullres_conf" in other cases.
 */
char* getConfiguration(maps** conf,maps** inputs,const char* confId){
  maps* input=*inputs;
  map* max_pixels=getMapFromMaps(*conf,confId,"preview_max_pixels");
  map* max_features=getMapFromMaps(*conf,confId,"preview_max_features");
  int i_max_pixels=atoi(max_pixels->value);
  int i_max_features=atoi(max_features->value);
  while(input!=NULL && input->content!=NULL){
    map* tmpMap=getMap(input->content,"geodatatype");
    if(tmpMap!=NULL){
      map* currentNb;
      if(strcasecmp(tmpMap->value,"raster")==0 ){
	currentNb=getMap(input->content,"nb_pixels");
	if(atoi(currentNb->value)>i_max_pixels)
	  return "fullres_conf";
      }else{
	if(strcasecmp(tmpMap->value,"vector")==0 ){
	  currentNb=getMap(input->content,"nb_features");
	  if(atoi(currentNb->value)>i_max_features)
	    return "fullres_conf";
	}
      }
    }
    input=input->next;
  }
  return "preview_conf";
}

/**
 * Load and run a HPC Application corresponding to the service.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 * @return SERVICE_SUCCEEDED in case of success, -1 or SERVICE_FAILED when failing.
 */
int zoo_hpc_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  map* tmp0=getMapFromMaps(*main_conf,"lenv","cwd");
  char *ntmp=tmp0->value;
  map* tmp=NULL;
  int res=-1;
  // Get the configuration id depending on service type and defined thresholds
  // then, set the configId key in the lenv section
  char *serviceType;
  map* mServiceType=getMap(s->content,"confId");
  if(mServiceType!=NULL)
    serviceType=mServiceType->value;
  else
    serviceType="HPC";
  map* tmpPath=getMapFromMaps(*main_conf,"main","tmpPath");
  map* uuid=getMapFromMaps(*main_conf,"lenv","usid");
  map* confMap=getMapFromMaps(*main_conf,serviceType,getConfiguration(main_conf,real_inputs,serviceType));
  char * configurationId=confMap->value;
  setMapInMaps(*main_conf,"lenv","configId",configurationId);
  // Dump lenv maps again after having set the configId ...
  char *flenv =
    (char *)
    malloc ((strlen (tmpPath->value) + 
	     strlen (uuid->value) + 12) * sizeof (char));
  sprintf (flenv, "%s/%s_lenv.cfg", tmpPath->value, uuid->value);
  maps* lenvMaps=getMaps(m,"lenv");
  dumpMapsToFile(lenvMaps,flenv,0);
  free(flenv);

  map* targetPathMap=getMapFromMaps(*main_conf,configurationId,"remote_data_path");
  map* targetPersistentPathMap=getMapFromMaps(*main_conf,configurationId,"remote_persistent_data_path");
  
  pthread_t threads_pool[50];
  // Force the HPC services to be called asynchronously
  map* isAsync=getMapFromMaps(*main_conf,"lenv","async");
  if(isAsync==NULL){
    errorException(*main_conf,_("The synchronous mode is not supported by this type of service"),"NoSuchMode",s->name);
    return -1;
  }

  maps* input=*real_inputs;
  char **parameters=NULL;
  int parameters_cnt=0;
  while(input!=NULL && input->content!=NULL){
    map* isInRequest=getMap(input->content,"inRequest");
    map* minNb=getMap(input->content,"minOccurs");
    if(getMaps(*real_outputs,input->name)==NULL &&
       ( (isInRequest!=NULL && strncasecmp(isInRequest->value,"true",4)==0)
	 || (minNb!=NULL && atoi(minNb->value)>0) ) ){
      parameters_cnt+=1;
      if(parameters_cnt==1)
	parameters=(char**)malloc(parameters_cnt*sizeof(char*));
      else
	parameters=(char**)realloc(parameters,parameters_cnt*sizeof(char*));
      if(getMap(input->content,"mimeType")!=NULL){
	// Input is ComplexData
	if(getMap(input->content,"cache_file")==NULL){
	  // Input data has been passed by value
	  // TODO: publish input through MapServer / use output publication
	  dumpMapsValuesToFiles(main_conf,&input);
	  addToMap(input->content,"toPublish","true");
	  //addToMap(input->content,"useMapserver","true");
	}
	if(getMap(input->content,"cache_file")!=NULL){
	  map* length=getMap(input->content,"length");
	  if(length==NULL){
	    addToMap(input->content,"length","1");
	    length=getMap(input->content,"length");
	  }
	  int len=atoi(length->value);
	  int i=0;
	  for(i=0;i<len;i++){
	    map* tmp=getMapArray(input->content,"cache_file",i);
	    map* origin=getMapArray(input->content,"origin",i);
	    char* targetName=strrchr(tmp->value,'/');
	    char *targetPath;
	    if(origin!=NULL && strcasecmp(origin->value,"SHARED")==0 && targetPersistentPathMap!=NULL){
	      targetPath=(char*)malloc((strlen(targetPersistentPathMap->value)+strlen(targetName)+2)*sizeof(char));
	      sprintf(targetPath,"%s/%s",targetPersistentPathMap->value,targetName);
	    }
	    else{
	      targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
	      sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
	    }
	    setMapArray(input->content,"targetPath",i,targetPath);
	    setMapArray(input->content,"localPath",i,tmp->value);
	    map* tmp1=getMapArray(input->content,"value",i);
	    if(tmp1!=NULL){
	      free(tmp1->value);
	      tmp1->value=strdup("empty");
	    }
	    if(i==0){
	      parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+3)*sizeof(char));
	      sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,targetPath);
	    }else{
	      fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
	      fflush(stderr);
	      char *tmpStr=zStrdup(parameters[parameters_cnt-1]);
	      parameters[parameters_cnt-1]=(char*)realloc(parameters[parameters_cnt-1],(strlen(tmpStr)+strlen(targetPath)+2)*sizeof(char));
	      sprintf(parameters[parameters_cnt-1],"%s %s",tmpStr,targetPath);
	      free(tmpStr);
	      fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
	      fflush(stderr);
	    }
	    free(targetPath);
	  }
	  addToUploadQueue(main_conf,input);
	}else{
	  // ???
	  fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
	  fflush(stderr);
	}
      }else{
	// LitteralData and BboxData 
	if(getMap(input->content,"dataType")!=NULL){
	  // For LitteralData, simply pass the value
          map* length=getMap(input->content,"length");
          if(length!=NULL){
            char* value=NULL;
            int len=atoi(length->value);
            int i=0;
            for(i=0;i<len;i++){
              map* val=getMapArray(input->content,"value",i);
              if(val!=NULL){
                if(value==NULL){
                  value=(char*)malloc((strlen(val->value)+3)*sizeof(char));
                  sprintf(value,"\"%s\"",val->value);
                }
                else{
                  value=(char*)realloc(value,(strlen(value)+strlen(val->value)+4)*sizeof(char));
                  sprintf(value,"%s \"%s\"",value,val->value);
                }
              }
            }
            if(value!=NULL){
              parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(value)+3)*sizeof(char));
              sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,value);
            }
          }else{
            map* val=getMap(input->content,"value");
            parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(val->value)+5)*sizeof(char));
            sprintf(parameters[parameters_cnt-1],"-%s \"%s\"",input->name,val->value);
          }
	}
      }
    }
    input=input->next;
  }

#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif
#ifdef USE_CALLBACK
  invokeCallback(m,inputs,NULL,1,1);
  invokeCallback(m,inputs,NULL,2,0);
#endif  
  if(getMapFromMaps(m,"lenv","mapError")!=NULL){
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,inputs,NULL,7,0);
#endif
    return -1;
  }
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  dumpMaps(inputs);
#endif

  // Upload data on HPC
  if(runUpload(main_conf)==false){
    errorException (*main_conf, _("Unable to lock the file for upload!"),
		    "InternalError", NULL);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,inputs,NULL,7,0);
#endif
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif
    return -1;
  }
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif
#ifdef USE_CALLBACK
  invokeCallback(m,inputs,NULL,2,1);
#endif
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif

  // Add the filename to generate for every output to parameters
  input=*real_outputs;
#ifdef HPC_DEBUG
  dumpMaps(input);
#endif
  while(input!=NULL){
    // Parse all outputs including inner outputs if required.
    if(input->child==NULL){
      // Name every files that should be produced by the service execution
      map* mime=getMap(input->content,"mimeType");
      char* targetName;
      if(mime!=NULL){
	bool hasExt=false;
	map* fileExt=getFileExtensionMap(mime->value,&hasExt);
	targetName=(char*)malloc((strlen(s->name)+strlen(input->name)+strlen(uuid->value)+strlen(fileExt->value)+11)*sizeof(char));
	sprintf(targetName,"output_%s_%s_%s.%s",s->name,input->name,uuid->value,fileExt->value);
	freeMap(&fileExt);
	free(fileExt);
      }else{
	targetName=(char*)malloc((strlen(s->name)+strlen(input->name)+strlen(uuid->value)+14)*sizeof(char));
	sprintf(targetName,"output_%s_%s_%s.tif",s->name,input->name,uuid->value);
      }
      char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
      sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
      map *tmpUrl=getMapFromMaps(*main_conf,"main","tmpUrl");
      char *targetUrl=(char*)malloc((strlen(tmpUrl->value)+strlen(targetName)+2)*sizeof(char));
      sprintf(targetUrl,"%s/%s",tmpUrl->value,targetName);
      free(targetName);
      setMapInMaps(*real_outputs,input->name,"generated_file",targetPath);
      addToMap(input->content,"generated_url",targetUrl);
      free(targetUrl);
      {
	parameters_cnt+=1;
	if(parameters_cnt==1)
	  parameters=(char**)malloc(parameters_cnt*sizeof(char*));
	else
	  parameters=(char**)realloc(parameters,parameters_cnt*sizeof(char*));
	// We should verify if any optional tag for output is required
	// (i.e. -out output.tiff *int8*), meaning that we should search
	// for a corresponding inputs name.
	map* inValue=getMapFromMaps(*real_inputs,input->name,"value");
	if(inValue!=NULL){
	  parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+strlen(inValue->value)+4)*sizeof(char));
	  sprintf(parameters[parameters_cnt-1],"-%s %s %s",input->name,targetPath,inValue->value);
	}else{
	  parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+3)*sizeof(char));
	  sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,targetPath);
	}
      }
      free(targetPath);
    }// In other case it means we need to return the cache_file as generated_file
    else{
      // Name every files that should be produced by the service execution
      map* mime=getMap(input->child->content,"mimeType");
      char* targetName;
      if(mime!=NULL){
	bool hasExt=false;
	map* fileExt=getFileExtensionMap(mime->value,&hasExt);
	targetName=(char*)malloc((strlen(s->name)+strlen(input->name)+strlen(uuid->value)+strlen(fileExt->value)+11)*sizeof(char));
	sprintf(targetName,"output_%s_%s_%s.%s",s->name,input->name,uuid->value,fileExt->value);
	freeMap(&fileExt);
	free(fileExt);
      }else{
	targetName=(char*)malloc((strlen(s->name)+strlen(input->name)+strlen(uuid->value)+14)*sizeof(char));
	sprintf(targetName,"output_%s_%s_%s.tif",s->name,input->name,uuid->value);
      }
      char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
      sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
      map *tmpUrl=getMapFromMaps(*main_conf,"main","tmpUrl");
      char *targetUrl=(char*)malloc((strlen(tmpUrl->value)+strlen(targetName)+2)*sizeof(char));
      sprintf(targetUrl,"%s/%s",tmpUrl->value,targetName);
      free(targetName);
      addToMap(input->content,"generated_file",targetPath);
      addToMap(input->content,"storage",targetPath);
      addToMap(input->content,"generated_url",targetUrl);
      free(targetUrl);
      if(strcasecmp(input->name,"wms_link")!=0&&
	 strcasecmp(input->name,"wcs_link")!=0 &&
	 strcasecmp(input->name,"wfs_link")!=0){
	parameters_cnt+=1;
	if(parameters_cnt==1)
	  parameters=(char**)malloc(parameters_cnt*sizeof(char*));
	else
	  parameters=(char**)realloc(parameters,parameters_cnt*sizeof(char*));
	// We should verify if any optional tag for output is required
	// (i.e. -out output.tiff *int8*), meaning that we should search
	// for a corresponding inputs name.
	map* inValue=getMapFromMaps(*real_inputs,input->name,"value");
	if(inValue!=NULL){
	  parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+strlen(inValue->value)+4)*sizeof(char));
	  sprintf(parameters[parameters_cnt-1],"-%s %s %s",input->name,targetPath,inValue->value);
	}else{
	  parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+3)*sizeof(char));
	  sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,targetPath);
	}
      }
      free(targetPath);
    }
    input=input->next;
  }
  // Produce the SBATCH File locally
  char *scriptPath=(char*)malloc((strlen(s->name)+strlen(tmpPath->value)+strlen(uuid->value)+10)*sizeof(char));
  sprintf(scriptPath,"%s/zoo_%s_%s.sh",tmpPath->value,s->name,uuid->value);
  setMapInMaps(*main_conf,"lenv","local_script",scriptPath);
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif
#ifdef USE_CALLBACK
  invokeCallback(m,inputs,NULL,3,0);
#endif
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif
  FILE* scriptFile=fopen(scriptPath,"w+");
  map* headerMap=getMapFromMaps(*main_conf,configurationId,"jobscript_header");
  if(headerMap!=NULL){
    // Use the header file if defined in the HPC section of the main.cfg file
    struct stat f_status;
    int s=stat(headerMap->value, &f_status);
    if(s==0){
      char* fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(headerMap->value,"rb");
      fread(fcontent,f_status.st_size,1,f);
      int fsize=f_status.st_size;
      fcontent[fsize]=0;
      fclose(f);
      fprintf(scriptFile,"%s\n### --- ZOO-Service HEADER end --- ###\n\n",fcontent);
      free(fcontent);
    }else
      fprintf(scriptFile,"#!/bin/bash\n\n### *** Default ZOO-Service HEADER (no header found) *** ###\n\n");
  }else
    fprintf(scriptFile,"#!/bin/bash\n\n### *** Default ZOO-Service HEADER *** ###\n\n");
  maps* hpc_opts=getMaps(*main_conf,configurationId);
  if(hpc_opts!=NULL){
    map* hpc_opts_content=hpc_opts->content;
    while(hpc_opts_content!=NULL){
      if(strncasecmp(hpc_opts_content->name,"sbatch_options_",15)==0)
	fprintf(scriptFile,"#SBATCH --%s=%s\n",strstr(hpc_opts_content->name,"sbatch_options_")+15,hpc_opts_content->value);
      hpc_opts_content=hpc_opts_content->next;
    }
  }
  fprintf(scriptFile,"#SBATCH --job-name=ZOO-Project_%s_%s\n\n",uuid->value,s->name);
  map* mods=getMap(s->content,"hpcModules");
  if(mods!=NULL)
    fprintf(scriptFile,"#SBATCH --export=MODULES=%s\n",mods->value);

  map* bodyMap=getMapFromMaps(*main_conf,configurationId,"jobscript_body");
  if(bodyMap!=NULL){
    // Use the header file if defined in the HPC section of the main.cfg file
    struct stat f_status;
    int s=stat(bodyMap->value, &f_status);
    if(s==0){
      char* fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(bodyMap->value,"rb");
      fread(fcontent,f_status.st_size,1,f);
      int fsize=f_status.st_size;
      fcontent[fsize]=0;
      fclose(f);
      fprintf(scriptFile,"%s\n### --- ZOO-Service BODY end --- ###\n\n",fcontent);
      free(fcontent);
    }else
      fprintf(scriptFile,"\n### *** Default ZOO-Service BODY (no body found) *** ###\n\n");
  }else
    fprintf(scriptFile,"\n### *** Default ZOO-Service BODY *** ###\n\n");

  map* sp=getMap(s->content,"serviceProvider");
  
  // Require to produce the command line to be executed
  fprintf(scriptFile,"\n\necho \"Job started at: $(date)\"\n");
  fprintf(scriptFile,"echo \"Running service: [%s]\"\n",sp->value);
  fprintf(scriptFile,"%s ",sp->value);
  for(int i=0;i<parameters_cnt;i++){
    fprintf(scriptFile," %s",parameters[i]);
  }
  for(int i=parameters_cnt-1;i>=0;i--){
    free(parameters[i]);
  }
  free(parameters);
  fprintf(scriptFile,"\n");
  fprintf(scriptFile,"echo \"Job finished at: $(date)\"\n");
  map* footerMap=getMapFromMaps(*main_conf,configurationId,"jobscript_footer");
  if(footerMap!=NULL){
    // Use the footer file if defined in the HPC section of the main.cfg file
    struct stat f_status;
    int s=stat(footerMap->value, &f_status);
    if(s==0){
      char* fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
      FILE* f=fopen(footerMap->value,"rb");
      fread(fcontent,f_status.st_size,1,f);
      int fsize=f_status.st_size;
      fcontent[fsize]=0;
      fclose(f);
      char* ffcontent=(char*)malloc((strlen(fcontent)+(3*strlen(uuid->value))+1)*sizeof(char));
      sprintf(ffcontent,fcontent,uuid->value,uuid->value,uuid->value);
      fprintf(scriptFile,"%s\n### --- ZOO-Service FOOTER end --- ###\n\n",ffcontent);
      free(fcontent);
    }else
      fprintf(scriptFile,"### *** Default ZOO-Service FOOTER (footer file failed to load) *** ###\n\n");
  }else
      fprintf(scriptFile,"### *** Default ZOO-Service FOOTER (no footer found) *** ###\n\n");
  fflush(scriptFile);
  fclose(scriptFile);
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif
#ifdef USE_CALLBACK
  invokeCallback(m,inputs,NULL,3,1);
#endif
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif

  // Upload the SBATCH File to the remote host
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif
#ifdef USE_CALLBACK
  invokeCallback(m,inputs,NULL,4,0);
#endif
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
#endif
  targetPathMap=getMapFromMaps(*main_conf,configurationId,"remote_work_path");
  if(targetPathMap==NULL){
    setMapInMaps(*main_conf,"lenv","message",_("There is no remote_work_path defined in your section!"));
    setMapInMaps(*main_conf,"lenv","status","failed");
    errorException (*main_conf, _("There is no remote_work_path defined in your section!"),
		    "InternalError", NULL);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    return SERVICE_FAILED;
  }
  char* targetName=strrchr(scriptPath,'/');
  char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
  sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
  setMapInMaps(*main_conf,"lenv","remote_script",targetPath);
  SSHCON *test=ssh_connect(*main_conf);
  int copy0=ssh_copy(*main_conf,scriptPath,targetPath,ssh_get_cnt(*main_conf));
  unlink(scriptPath);
  free(scriptPath);
  if(copy0!=true){
    setMapInMaps(*main_conf,"lenv","message",_("Unable to upload the script"));
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
    errorException(*main_conf,_("Unable to upload the script"),"NoApplicableCode",NULL);
    return -1;
  }
  // Execute the SBATCH script remotely
  addReadLocks(main_conf);
  map* subStr=getMapFromMaps(*main_conf,configurationId,"sbatch_substr");
  char *command=(char*)malloc((strlen(targetPath)+strlen(targetPathMap->value)+strlen(subStr->value)+strlen(uuid->value)+137)*sizeof(char));
  sprintf(command,"sbatch %s 2> %s/error_%s.log | sed \"s:%s::g\"",targetPath,targetPathMap->value,uuid->value,subStr->value);
  if(ssh_exec(*main_conf,command,ssh_get_cnt(m))<=0){
    // The sbatch command has failed!
    // Download the error log file from the HPC server
    char tmpS[1024];
    free(command);
    command=(char*)malloc((strlen(targetPathMap->value)+strlen(uuid->value)+11)*sizeof(char));
    sprintf(command,"%s/error_%s.log",targetPathMap->value,uuid->value);
    targetName=strrchr(command,'/');
    free(targetPath);
    targetPath=(char*)malloc((strlen(tmpPath->value)+strlen(targetName)+2)*sizeof(char));
    sprintf(targetPath,"%s/%s",tmpPath->value,targetName);
    if(ssh_fetch(*main_conf,targetPath,command,ssh_get_cnt(m))==0){
      struct stat f_status;
      int ts=stat(targetPath, &f_status);
      if(ts==0) {
	char* fcontent = NULL;
	fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
	FILE* f=fopen(targetPath,"rb");
	fread(fcontent,f_status.st_size,1,f);
	int fsize=f_status.st_size;
	fcontent[fsize]=0;
	fclose(f);
	setMapInMaps(*main_conf,"lenv","message",fcontent);
	free(fcontent);
      }else
	setMapInMaps(*main_conf,"lenv","message",_("No message provided"));
    }else
      setMapInMaps(*main_conf,"lenv","message",_("Unable to fetch the remote error log file"));
    tmpPath=getMapFromMaps(m,"lenv","message");
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    sprintf(tmpS, "Cannot execute the HPC ZOO-Service %s using %s: %s", s->name, configurationId, tmpPath->value);
    errorException(*main_conf,tmpS,"NoApplicableCode",NULL);
    free(command);
    free(targetPath);
    ssh_close(*main_conf);
    removeReadLocks(main_conf);
    return -1;
  }
  free(targetPath);
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif
#ifdef USE_CALLBACK
  invokeCallback(m,NULL,NULL,4,1);
#endif
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif
  free(command);
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  int rc, cl, fd = socket(AF_UNIX, SOCK_STREAM, 0);
  char *sname=(char*)malloc((strlen(tmpPath->value)+strlen(uuid->value)+20));
  sprintf(sname,"%s/.wait_socket_%s.sock",tmpPath->value,uuid->value);
  strncpy(addr.sun_path, sname, sizeof(addr.sun_path)-1);
  
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    setMapInMaps(*main_conf,"lenv","message",_("Unable to bind socket!"));
    errorException (*main_conf, _("Unable to bind socket!"),
		    "InternalError", NULL);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
    removeReadLocks(main_conf);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    return -1;
  }
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif
  if (listen(fd, 5) == -1) {
    setMapInMaps(*main_conf,"lenv","message",_("Listen error"));
    errorException (*main_conf, _("Listen error"),
		    "InternalError", NULL);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
    removeReadLocks(main_conf);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    return -1;
  }
  if ( (cl = accept(fd, NULL, NULL)) == -1) {
    setMapInMaps(*main_conf,"lenv","message",_("Accept error"));
    errorException (*main_conf, _("Accept error"),
		    "InternalError", NULL);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
#ifdef USE_CALLBACK
    invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
    removeReadLocks(main_conf);
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    return -1;
  }else{
#ifdef HPC_DEBUG
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
#endif
    int hasPassed=-1;
    char buf[11];
    memset(&buf,0,11);
    while ( (rc=read(cl,buf,10)) ) {	  
      if(rc==0){
	setMapInMaps(*main_conf,"lenv","message",_("Read closed"));
	errorException (*main_conf, _("Read closed"),
			"InternalError", NULL);
#ifdef HPC_DEBUG
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
#endif
#ifdef USE_CALLBACK
	invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
	removeReadLocks(main_conf);
#ifdef HPC_DEBUG
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
#endif
	return -1;
      }else{
	if(rc<0){
	  setMapInMaps(*main_conf,"lenv","message",_("Read error"));
	  errorException (*main_conf, _("Read error"),
			  "InternalError", NULL);
#ifdef HPC_DEBUG
	  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	  fflush(stderr);
#endif
#ifdef USE_CALLBACK
	  invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
	  removeReadLocks(main_conf);
#ifdef HPC_DEBUG
	  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	  fflush(stderr);
#endif
	  return -1;
	}
      }
      hasPassed=1;
      res=atoi(buf);
      unlink(sname);
      free(sname);
      removeReadLocks(main_conf);
  
      if(res==3){
#ifdef HPC_DEBUG
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
#endif
#ifdef USE_CALLBACK
	invokeCallback(m,NULL,outputs,5,0);
#endif
#ifdef HPC_DEBUG
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
#endif

	// Read informations provided by FinalizeHPC as a configuration file
	// then, remove the file.
	map* jobid=getMapFromMaps(*main_conf,"lenv","usid");
	map* tmpPath=getMapFromMaps(*main_conf,"main","tmpPath");
	char *filePath=(char*)malloc((strlen(tmpPath->value)+strlen(jobid->value)+15)*sizeof(char));
	sprintf(filePath,"%s/exec_status_%s",tmpPath->value,jobid->value);
	maps* lm = (maps *) malloc (MAPS_SIZE);
	lm->child=NULL;
	lm->next=NULL;
	int saved_stdout = dup (fileno (stdout));
	dup2 (fileno (stderr), fileno (stdout));
	conf_read(filePath,lm);
	//dumpMaps(m);
	fflush(stdout);
	dup2 (saved_stdout, fileno (stdout));
	close(saved_stdout);
	unlink(filePath);
	free(filePath);
	addMapsToMaps(main_conf,lm);
	freeMaps(&lm);
	free(lm);

	input=*real_outputs;
	while(input!=NULL){
	  if(input->child==NULL){
	    map* generatedFile=getMap(input->content,"generated_file");
	    if(generatedFile!=NULL){
	      char* filename=strrchr(generatedFile->value,'/');
	      char* targetPath=(char*)malloc((strlen(tmpPath->value)+strlen(filename)+2)*sizeof(char));
	      sprintf(targetPath,"%s/%s",tmpPath->value,filename);
	      test=ssh_connect(*main_conf);
	      if(ssh_fetch(*main_conf,targetPath,generatedFile->value,ssh_get_cnt(m))==0){
		setMapInMaps(*real_outputs,input->name,"generated_file",targetPath);
		free(targetPath);
	      }else{
		map* hpcStdErr=getMapFromMaps(*main_conf,"henv","StdErr");
		// Added for using sacct in place of scontrol
		char *sourcePath=NULL;
		if(hpcStdErr!=NULL){
		  sourcePath=(char*)malloc((strlen(targetPathMap->value)+strlen(hpcStdErr->value)+2)*sizeof(char));
		  sprintf(sourcePath,"%s/%s",targetPathMap->value,hpcStdErr->value);
		}
                if(hpcStdErr!=NULL && sourcePath!=NULL && ssh_fetch(*main_conf,targetPath,sourcePath,ssh_get_cnt(m))==0){
		  free(sourcePath);
		  struct stat f_status;
		  int ts=stat(targetPath, &f_status);
		  if(ts==0) {
		    char* fcontent = NULL;
		    fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
		    FILE* f=fopen(targetPath,"rb");
		    fread(fcontent,f_status.st_size,1,f);
		    int fsize=f_status.st_size;
		    fcontent[fsize]=0;
		    fclose(f);
		    setMapInMaps(*main_conf,"lenv","message",fcontent);
		    free(fcontent);
		  }else{
		    char *tmpStr=(char*)malloc((strlen(targetPath)+strlen(_("Unable to fetch the remote file for %s"))+1)*sizeof(char));
		    sprintf(tmpStr,_("Unable to fetch the remote file for %s"),targetPath);
		    setMapInMaps(*main_conf,"lenv","message",tmpStr);
		    free(tmpStr);
		  }
                }else{
                  char *tmpStr=(char*)malloc((strlen(filename)+strlen(_("Unable to fetch the remote file for %s"))+1)*sizeof(char));
                  sprintf(tmpStr,_("Unable to fetch the remote file for %s"),filename);
                  setMapInMaps(*main_conf,"lenv","message",tmpStr);
                  free(tmpStr);
                }
#ifdef USE_CALLBACK
		invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
		return SERVICE_FAILED;
	      }
	    }
	  }else{
	    map* generatedFile=getMap(input->content,"generated_file");
	    map* generatedUrl=getMap(input->content,"generated_url");
	    if(generatedFile!=NULL){
	      char* filename=strrchr(generatedFile->value,'/');
	      char* targetPath=(char*)malloc((strlen(tmpPath->value)+strlen(filename)+2)*sizeof(char));
	      sprintf(targetPath,"%s/%s",tmpPath->value,filename);
	      test=ssh_connect(*main_conf);
	      if(ssh_fetch(*main_conf,targetPath,generatedFile->value,ssh_get_cnt(m))==0){
		maps* tmp=getMaps(*real_outputs,input->name);
		char serviceName[9];
		freeMap(&tmp->content);
		free(tmp->content);
		tmp->content=NULL;
		maps* output=getMaps(*real_outputs,input->name);
		setMapInMaps(output->child,"download_link","generated_file",targetPath);
		setMapInMaps(output->child,"download_link","generated_url",generatedUrl->value);
		setMapInMaps(output->child,"download_link","storage",targetPath);
		setMapInMaps(output->child,"download_link","useMapserver","false");
		setMapInMaps(output->child,"download_link","replicateStorageNext","true");
		setMapInMaps(output->child,"download_link","asReference","true");
		setMapInMaps(output->child,"download_link","inRequest","true");
		setMapInMaps(output->child,"wms_link","generated_file",targetPath);
		setMapInMaps(output->child,"wms_link","storage",targetPath);
		setMapInMaps(output->child,"wms_link","useMapserver","true");
		setMapInMaps(output->child,"wms_link","msOgc","WMS");
		setMapInMaps(output->child,"wms_link","requestedMimeType","image/png");
		setMapInMaps(output->child,"wms_link","asReference","true");
		if(getMaps(output->child,"wcs_link")!=NULL){
		  sprintf(serviceName,"wcs_link");
		  setMapInMaps(output->child,"wcs_link","msOgc","WCS");
		}else{
		  sprintf(serviceName,"wfs_link");
		  setMapInMaps(output->child,"wfs_link","msOgc","WFS");
                }
		setMapInMaps(output->child,serviceName,"storage",targetPath);
		setMapInMaps(output->child,serviceName,"generated_file",targetPath);
		setMapInMaps(output->child,serviceName,"useMapserver","true");
		setMapInMaps(output->child,serviceName,"asReference","true");
	      }else{
		map* hpcStdErr=getMapFromMaps(*main_conf,"henv","StdErr");
		char *sourcePath=NULL;
		if(hpcStdErr!=NULL){
		  dumpMap(hpcStdErr);
		  sourcePath=(char*)malloc((strlen(targetPathMap->value)+strlen(hpcStdErr->value)+2)*sizeof(char));
		  sprintf(sourcePath,"%s/%s",targetPathMap->value,hpcStdErr->value);
		}
                if(hpcStdErr!=NULL && sourcePath!=NULL && ssh_fetch(*main_conf,targetPath,sourcePath,ssh_get_cnt(m))==0){
		  free(sourcePath);
		  struct stat f_status;
		  int ts=stat(targetPath, &f_status);
		  if(ts==0) {
		    char* fcontent = NULL;
		    fcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
		    FILE* f=fopen(targetPath,"rb");
		    fread(fcontent,f_status.st_size,1,f);
		    int fsize=f_status.st_size;
		    fcontent[fsize]=0;
		    fclose(f);
		    setMapInMaps(*main_conf,"lenv","message",fcontent);
		    free(fcontent);
		  }else{
		    char *tmpStr=(char*)malloc((strlen(targetPath)+strlen(_("Unable to fetch the remote file for %s"))+1)*sizeof(char));
		    sprintf(tmpStr,_("Unable to fetch the remote file for %s"),targetPath);
		    setMapInMaps(*main_conf,"lenv","message",tmpStr);
		    free(tmpStr);
		  }
		}else{
		  char *tmpStr=(char*)malloc((strlen(sourcePath)+strlen(_("Unable to fetch the remote file for %s"))+1)*sizeof(char));
		  sprintf(tmpStr,_("Unable to fetch the remote file for %s"),sourcePath);
		  setMapInMaps(*main_conf,"lenv","message",tmpStr);
		  free(tmpStr);
		}
#ifdef USE_CALLBACK
                invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
                return SERVICE_FAILED;
	      }
	      free(targetPath);
	    }
	  }
	  input=input->next;
	}

      }else{
	// Try to access remotely to the log file and return a more relevant error message
	setMapInMaps(*main_conf,"lenv","message",_("HPC Execution failed!"));
	errorException (*main_conf, _("HPC Execution failed!"),
			"InternalError", NULL);
#ifdef HPC_DEBUG
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
#endif
#ifdef USE_CALLBACK
	invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
#ifdef HPC_DEBUG
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
#endif
      }
    }
    if(hasPassed<0){
      perror("Failed to read");
      setMapInMaps(*main_conf,"lenv","message",_("Unable to parse the value returned by remote execution"));
      errorException (*main_conf, _("Unable to parse the value returned by remote execution"),
		      "InternalError", NULL);
#ifdef HPC_DEBUG
      fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
      fflush(stderr);
#endif
#ifdef USE_CALLBACK
      invokeCallback(*main_conf,NULL,NULL,7,0);
#endif
#ifdef HPC_DEBUG
      fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
      fflush(stderr);
#endif
      return SERVICE_FAILED;
    }
  }
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif
  ssh_close(*main_conf);
#ifdef HPC_DEBUG
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
#endif
  return res;
}
