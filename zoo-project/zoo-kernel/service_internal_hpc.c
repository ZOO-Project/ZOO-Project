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
  if(*s==NULL || (*s)->outputs==NULL){
    return;
  }
  elements *out=(*s)->outputs;
  elements* cur=out;
  map* serviceType=getMap((*s)->content,"ServiceType");
  if(strncmp(serviceType->value,"HPC",3)!=0)
    return;
  while(cur!=NULL && cur->defaults!=NULL){
    map* mimeType=getMap(cur->defaults->content,"mimeType");
    if(mimeType!=NULL){
      int geo=isGeographic(mimeType->value);
      if(geo>0){
	elements *tmp[3]={
	  dupElements(cur),
	  dupElements(cur),
	  dupElements(cur)
	};
	char *geoLink="wcs_link";
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
	  cur->defaults=NULL;
	}
	if(cur->supported!=NULL){
	  freeIOType(&cur->supported);
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
    }
    cur=cur->next;
  }
  //dumpElements((*s)->outputs);
}

/**
 * Load and run a HPC Application corresponding to the service.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 */
int zoo_hpc_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  map* tmp0=getMapFromMaps(*main_conf,"lenv","cwd");
  char *ntmp=tmp0->value;
  map* tmp=NULL;
  int res=-1;
  char *serviceType;
  map* mServiceType=getMap(s->content,"serviceType");
  if(mServiceType!=NULL)
    serviceType=mServiceType->value;
  else
    serviceType="HPC";
  map* targetPathMap=getMapFromMaps(*main_conf,serviceType,"storagePath");
  map* tmpPath=getMapFromMaps(*main_conf,"main","tmpPath");
  map* uuid=getMapFromMaps(*main_conf,"lenv","usid");
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
    if(getMaps(*real_outputs,input->name)==NULL){
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
	    char* targetName=strrchr(tmp->value,'/');
	    char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
	    sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
	    setMapArray(input->content,"targetPath",i,targetPath);
	    setMapArray(input->content,"localPath",i,tmp->value);
	    addToUploadQueue(main_conf,input);
	    if(i==0){
	      parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+3)*sizeof(char));
	      sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,targetPath);
	    }else{
	      char *tmpStr=zStrdup(parameters[parameters_cnt-1]);
	      parameters[parameters_cnt-1]=(char*)realloc(parameters[parameters_cnt-1],(strlen(tmpStr)+strlen(targetPath)+2)*sizeof(char));
	      sprintf(parameters[parameters_cnt-1],"%s %s",tmpStr,targetPath);
	      free(tmpStr);
	    }
	    free(targetPath);
	  }
	}else{
	  // ???
	  fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
	  fflush(stderr);
	}
      }else{
	// LitteralData and BboxData 
	if(getMap(input->content,"dataType")!=NULL){
	  // For LitteralData, simply pass the value
	  map* val=getMap(input->content,"value");
	  parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(val->value)+3)*sizeof(char));
	  sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,val->value);
	}
      }
    }
    input=input->next;
  }

  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  invokeCallback(m,inputs,NULL,2,0);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);

  // Upload data on HPC
  runUpload(main_conf);
  
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  invokeCallback(m,inputs,NULL,2,1);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);

  // Add the filename to generate for every output to the parameters
  input=*real_outputs;
  // TODO: fix appendOutputParameters
  //appendOutputParameters(input,parameters,&parameters_cnt,s,uuid,targetPathMap);
  while(input!=NULL){
    // TODO: parse all outputs including inner outputs if required.
    if(input->child==NULL){
      parameters_cnt+=1;
      if(parameters_cnt==1)
	parameters=(char**)malloc(parameters_cnt*sizeof(char*));
      else
	parameters=(char**)realloc(parameters,parameters_cnt*sizeof(char*));
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
      setMapInMaps(*real_outputs,input->name,"generated_file",targetPath);
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
      free(targetPath);
    }// In other case it means we need to return the cache_file as generated_file
    else{
      parameters_cnt+=1;
      if(parameters_cnt==1)
	parameters=(char**)malloc(parameters_cnt*sizeof(char*));
      else
	parameters=(char**)realloc(parameters,parameters_cnt*sizeof(char*));
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
      addToMap(input->content,"generated_file",targetPath);
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
      free(targetPath);
    }
    input=input->next;
  }
  
  // Produce the SBATCH File locally
  char *scriptPath=(char*)malloc((strlen(s->name)+strlen(tmpPath->value)+strlen(uuid->value)+10)*sizeof(char));
  sprintf(scriptPath,"%s/zoo_%s_%s.sh",tmpPath->value,s->name,uuid->value);
  setMapInMaps(*main_conf,"lenv","local_script",scriptPath);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
  invokeCallback(m,inputs,NULL,3,0);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
  FILE* scriptFile=fopen(scriptPath,"w+");
  map* headerMap=getMapFromMaps(*main_conf,serviceType,"header");
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
  maps* hpc_opts=getMaps(*main_conf,"sbatch_options");
  if(hpc_opts!=NULL){
    map* hpc_opts_content=hpc_opts->content;
    while(hpc_opts_content!=NULL){
      fprintf(scriptFile,"#SBATCH --%s=%s\n",hpc_opts_content->name,hpc_opts_content->value);
      hpc_opts_content=hpc_opts_content->next;
    }
  }
  fprintf(scriptFile,"#SBATCH --job-name=ZOO-Project_%s_%s\n\n",uuid->value,s->name);
  map* mods=getMap(s->content,"hpcModules");
  if(mods!=NULL)
    fprintf(scriptFile,"#SBATCH --export=MODULES=%s\n",mods->value);

  map* bodyMap=getMapFromMaps(*main_conf,serviceType,"body");
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
      fprintf(scriptFile,"#!/bin/bash\n\n### *** Default ZOO-Service BODY (no body found) *** ###\n\n");
  }else
    fprintf(scriptFile,"#!/bin/bash\n\n### *** Default ZOO-Service BODY *** ###\n\n");

  
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
  fflush(scriptFile);
  fclose(scriptFile);
  
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  invokeCallback(m,inputs,NULL,3,1);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);

  // Upload the SBATCH File to the remote host
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  invokeCallback(m,inputs,NULL,4,0);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  targetPathMap=getMapFromMaps(*main_conf,serviceType,"executePath");
  if(targetPathMap==NULL){
    setMapInMaps(*main_conf,"lenv","message",_("There is no executePath defined in you HPC section!"));
    return SERVICE_FAILED;
  }
  char* targetName=strrchr(scriptPath,'/');
  char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
  sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
  setMapInMaps(*main_conf,"lenv","remote_script",targetPath);
  SSHCON *test=ssh_connect(*main_conf);
  ssh_copy(*main_conf,scriptPath,targetPath,ssh_get_cnt(*main_conf));
  
  // Execute the SBATCH script remotely
  map* subStr=getMapFromMaps(*main_conf,"HPC","subStr");
  char *command=(char*)malloc((strlen(targetPath)+strlen(targetPathMap->value)+strlen(subStr->value)+strlen(uuid->value)+137)*sizeof(char));
  sprintf(command,"sbatch %s 2> %s/error_%s.log | sed \"s:%s::g\"",targetPath,targetPathMap->value,uuid->value,subStr->value);
  if(ssh_exec(*main_conf,command,ssh_get_cnt(m))==0){
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
    tmpPath=getMapFromMaps(*main_conf,"lenv","message");
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
    invokeCallback(m,NULL,NULL,7,1);
    fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    fflush(stderr);
    sprintf(tmpS, "Cannot execute the HPC ZOO-Service %s: %s", s->name, tmpPath->value);
    errorException(m,tmpS,"NoApplicableCode",NULL);
    free(command);
    free(targetPath);
    ssh_close(*main_conf);
    sleep(120);
    return -1;
  }
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
  invokeCallback(m,NULL,NULL,4,1);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
  free(command);

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  int rc, cl, fd = socket(AF_UNIX, SOCK_STREAM, 0);
  char *sname=(char*)malloc((strlen(tmpPath->value)+strlen(uuid->value)+20));
  sprintf(sname,"%s/.wait_socket_%s.sock",tmpPath->value,uuid->value);
  strncpy(addr.sun_path, sname, sizeof(addr.sun_path)-1);
  
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    sleep(120);
    return -1;
  }
  if (listen(fd, 5) == -1) {
    setMapInMaps(*main_conf,"lenv","message",_("Listen error"));
    return -1;
  }
  if ( (cl = accept(fd, NULL, NULL)) == -1) {
    setMapInMaps(*main_conf,"lenv","message",_("Accept error"));
    return -1;
  }else{
    int hasPassed=-1;
    char buf[11];
    memset(&buf,0,11);
    while ( (rc=read(cl,buf,10)) ) {	  
      if(rc==0){
	sleep(1);
	setMapInMaps(*main_conf,"lenv","message",_("Read closed"));
	invokeCallback(m,NULL,NULL,7,1);
	return -1;
      }else{
	if(rc<0){
	  setMapInMaps(*main_conf,"lenv","message",_("Read error"));
	  invokeCallback(m,NULL,NULL,7,1);
	  return -1;
	}
      }
      hasPassed=1;
      res=atoi(buf);
      unlink(sname);
      //free(sname);

      if(res==3){
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
	invokeCallback(m,NULL,outputs,5,0);
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
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
		char *tmpStr=(char*)malloc((strlen(filename)+strlen(_("Unable to fetch the remote file for %s"))+1)*sizeof(char));
		sprintf(tmpStr,_("Unable to fetch the remote file for %s"),filename);
		setMapInMaps(*main_conf,"lenv","message",tmpStr);
		free(tmpStr);
		return SERVICE_FAILED;
	      }
	    }	    
	  }else{
	    fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
	    fflush(stderr);
	    map* generatedFile=getMap(input->content,"generated_file");
	    if(generatedFile!=NULL){
	      char* filename=strrchr(generatedFile->value,'/');
	      char* targetPath=(char*)malloc((strlen(tmpPath->value)+strlen(filename)+2)*sizeof(char));
	      sprintf(targetPath,"%s/%s",tmpPath->value,filename);
	      test=ssh_connect(*main_conf);
	      if(ssh_fetch(*main_conf,targetPath,generatedFile->value,ssh_get_cnt(m))==0){
		maps* tmp=getMaps(*real_outputs,input->name);
		freeMap(&tmp->content);
		free(tmp->content);
		tmp->content=NULL;
		maps* output=getMaps(*real_outputs,input->name);
		setMapInMaps(output->child,"download_link","generated_file",targetPath);
		setMapInMaps(output->child,"download_link","useMapserver","false");
		setMapInMaps(output->child,"WMS_LINK","generated_file",targetPath);
		setMapInMaps(output->child,"WMS_LINK","useMapserver","true");
		setMapInMaps(output->child,"WCS_LINK","generated_file",targetPath);
		setMapInMaps(output->child,"WCS_LINK","useMapserver","true");
	      }
	    }
	  }
	  input=input->next;
	}
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
	invokeCallback(m,NULL,outputs,5,1);
	fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
	fflush(stderr);
      }
      //free(buf);
    }
    if(hasPassed<0){
      perror("Failed to read");
      setMapInMaps(*main_conf,"lenv","message",_("Unable to parse the value returned by remote execution"));
      sleep(120);
      return SERVICE_FAILED;
    }
  }
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
  ssh_close(*main_conf);
  fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
  fflush(stderr);
  return res;
}

    
