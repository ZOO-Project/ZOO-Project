/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2015 GeoLabs SARL
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
 * See Ref: http://hg.orfeo-toolbox.org/OTB/ Copyright
 * Some parts of this code are derived from ITK. See ITKCopyright.txt for 
 * details.
 */

#include "service_internal_hpc.h"
#include "response_print.h"
#include "server_internal.h"
#include <sys/un.h>

void appendOutputParameters(maps* input,char** parameters,int* cnt,service* s,map* uuid,map* targetPathMap){
  while(input!=NULL){
    if(input->child==NULL){
      *cnt+=1;
      if(*cnt==1)
	parameters=(char**)malloc((*cnt)*sizeof(char*));
      else
	parameters=(char**)realloc(parameters,(*cnt)*sizeof(char*));
      // Name every files that should be produced by the service execution
      char* targetName=(char*)malloc((strlen(s->name)+strlen(input->name)+strlen(uuid->value)+10)*sizeof(char));
      sprintf(targetName,"output_%s_%s_%s",s->name,input->name,uuid->value);
      // TODO: We should verify if any optional tag for output is required (i.e. -out output.tiff int8)
      char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
      sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
      parameters[*cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+3)*sizeof(char));
      sprintf(parameters[*cnt-1],"-%s %s",input->name,targetPath);
    }else
      appendOutputParameters(input->child,parameters,cnt,s,uuid,targetPathMap);
    input=input->next;
  }
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
  map* targetPathMap=getMapFromMaps(*main_conf,"HPC","storagePath");
  map* tmpPath=getMapFromMaps(*main_conf,"main","tmpPath");
  map* uuid=getMapFromMaps(*main_conf,"lenv","usid");

  sleep(5);

  maps* input=*real_inputs;
  char **parameters=NULL;
  int parameters_cnt=0;
  while(input!=NULL){
    parameters_cnt+=1;
    if(parameters_cnt==1)
      parameters=(char**)malloc(parameters_cnt*sizeof(char*));
    else
      parameters=(char**)realloc(parameters,parameters_cnt*sizeof(char*));
    if(getMap(input->content,"mimeType")!=NULL){
      // Input is ComplexData
      SSHCON *test=ssh_connect(*main_conf);
      dumpMaps(getMaps(*main_conf,"lenv"));
      if(test==NULL){	
	sleep(3600);
	return -1;
      }
      if(getMap(input->content,"cache_file")==NULL){
	// Input data has been passed by value
	// TODO: store data remotely
	// TODO: publish input through MapServer / use output publication
	dumpMapsValuesToFiles(main_conf,&input);
      }
      if(getMap(input->content,"cache_file")!=NULL){
	// Input data passed by reference or by value
	map* tmp=getMap(input->content,"cache_file");
	char* targetName=strrchr(tmp->value,'/');
	char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
	sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
	ssh_copy(*main_conf,tmp->value,targetPath);
	parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+3)*sizeof(char));
	sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,targetPath);
	free(targetPath);
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
	sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,val->value);
      }
      
    }
    input=input->next;
  }

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
      char* targetName=(char*)malloc((strlen(s->name)+strlen(input->name)+strlen(uuid->value)+10)*sizeof(char));
      sprintf(targetName,"output_%s_%s_%s",s->name,input->name,uuid->value);
      // TODO: We should verify if any optional tag for output is required (i.e. -out output.tiff *int8*)
      // TODO: Add support for Array values
      char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
      sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
      setMapInMaps(*real_outputs,input->name,"generated_file",targetPath);
      parameters[parameters_cnt-1]=(char*)malloc((strlen(input->name)+strlen(targetPath)+3)*sizeof(char));
      sprintf(parameters[parameters_cnt-1],"-%s %s",input->name,targetPath);
    }
    input=input->next;
  }
  
  // Produce the SBATCH File locally
  char *scriptPath=(char*)malloc((strlen(s->name)+strlen(tmpPath->value)+strlen(uuid->value)+9)*sizeof(char));
  sprintf(scriptPath,"%s/zoo_%s_%s.sh",tmpPath->value,s->name,uuid->value);
  FILE* scriptFile=fopen(scriptPath,"w+");
  maps* hpc_opts=getMaps(*main_conf,"sbatch_options");
  map* hpc_opts_content=hpc_opts->content;
  map* headerMap=getMapFromMaps(*main_conf,"HPC","header");
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
      fprintf(scriptFile,"%s\n### --- ZOO-Service HEADER end --- ###",fcontent);
      free(fcontent);
    }else
      fprintf(scriptFile,"#!/bin/bash\n\n### *** Default ZOO-Service HEADER (no header found) *** ###\n\n");
  }else
    fprintf(scriptFile,"#!/bin/bash\n\n### *** Default ZOO-Service HEADER *** ###\n\n");
  
  while(hpc_opts_content!=NULL){
    fprintf(scriptFile,"#SBATCH --%s=%s\n",hpc_opts_content->name,hpc_opts_content->value);
    hpc_opts_content=hpc_opts_content->next;
  }
  fprintf(scriptFile,"#SBATCH --job-name=ZOO-Project_%s_%s\n\n",uuid->value,s->name);
  map* mods=getMap(s->content,"hpcModules");
  if(mods!=NULL)
    fprintf(scriptFile,"#SBATCH --export=MODULES=%s\n",mods->value);
  else
    fprintf(scriptFile,"#SBATCH --export=MODULES=\n");
  map* sp=getMap(s->content,"serviceProvider");
  // Require to produce the command line to be executed
  fprintf(scriptFile,"\n\necho \"Job started at: $(date)\"\n");
  fprintf(scriptFile,"echo \"Running service: [%s]\"\n",sp->value);
  fprintf(scriptFile,"%s ",sp->value);
  for(int i=0;i<parameters_cnt;i++){
    fprintf(scriptFile," %s",parameters[i]);
    //free(parameters[i]);
  }
  free(parameters);
  fprintf(scriptFile,"\n");
  fprintf(scriptFile,"echo \"Job finished at: $(date)\"\n");
  
  fflush(scriptFile);
  fclose(scriptFile);

  // Upload the SBATCH File to the remote host
  targetPathMap=getMapFromMaps(*main_conf,"HPC","executePath");
  if(targetPathMap==NULL){
    setMapInMaps(*main_conf,"lenv","message",_("There is no executePath defined in you HPC section!"));
    return SERVICE_FAILED;
  }
  char* targetName=strrchr(scriptPath,'/');
  char *targetPath=(char*)malloc((strlen(targetPathMap->value)+strlen(targetName)+2)*sizeof(char));
  sprintf(targetPath,"%s/%s",targetPathMap->value,targetName);
  SSHCON *test=ssh_connect(*main_conf);
  ssh_copy(*main_conf,scriptPath,targetPath);
  
  // Execute the SBATCH script remotely
  map* subStr=getMapFromMaps(*main_conf,"HPC","subStr");
  //char *command=(char*)malloc((strlen(targetPath)+strlen(targetPathMap->value)+strlen(subStr->value)+strlen(uuid->value)+37)*sizeof(char));
  char *command=(char*)malloc((strlen(targetPath)+strlen(targetPathMap->value)+strlen(subStr->value)+strlen(uuid->value)+137)*sizeof(char));
  //sprintf(command,"ls # %s 2> %s/error_%s.log | sed \"s:%s::g\"",targetPath,targetPathMap->value,uuid->value,subStr->value);
  sprintf(command,"sbatch %s 2> %s/error_%s.log | sed \"s:%s::g\"",targetPath,targetPathMap->value,uuid->value,subStr->value);
  if(ssh_exec(*main_conf,command)==0){
    // The sbatch command has failed!
    // Download the error log file from the HPC server
    char tmpS[1024];
    free(command);
    command=(char*)malloc((strlen(targetPathMap->value)+strlen(uuid->value)+22)*sizeof(char));
    sprintf(command,"%s/error_%s.log",targetPathMap->value,uuid->value);
    targetName=strrchr(command,'/');
    free(targetPath);
    targetPath=(char*)malloc((strlen(tmpPath->value)+strlen(targetName)+2)*sizeof(char));
    sprintf(targetPath,"%s/%s",tmpPath->value,targetName);
    if(ssh_fetch(*main_conf,targetPath,command)==0){
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
    sprintf(tmpS, "Cannot execute the HPC ZOO-Service %s: %s", s->name, tmpPath->value);
    errorException(m,tmpS,"NoApplicableCode",NULL);
    free(command);
    free(targetPath);
    ssh_close(*main_conf);
    sleep(120);
    return -1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  int rc, cl, fd = socket(AF_UNIX, SOCK_STREAM, 0);
  char buf[100];
  char *sname=(char*)malloc((strlen(tmpPath->value)+strlen(uuid->value)+19));
  sprintf(sname,"%s/.wait_socket_%s.sock",tmpPath->value,uuid->value);
  strncpy(addr.sun_path, sname, sizeof(addr.sun_path)-1);
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    sleep(120);
    return -1;
  }
  if (listen(fd, 5) == -1) {
    perror("listen error");
    sleep(120);
    return -1;
  }
  /*fd_set master, read_fds;
  int fdmax;
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(fd, &master);
  if (select(fd+1, &master, NULL, NULL, NULL) == -1) {
    perror("select");
    sleep(120);
    return -1;
  }
  if (FD_ISSET(fd, &master)) {*/
    if ( (cl = accept(fd, NULL, NULL)) == -1) {
      perror("accept error");
      sleep(120);
      return -1;
    }else{
      int hasPassed=-1;
      /*FD_SET(cl,&read_fds);
      if (select(cl+1, &read_fds, NULL, NULL, NULL) == -1) {
	perror("select");
	sleep(120);
	return -1;
	}*/
      while ( (rc=read(cl,buf,10)) ) {	  
	if(rc==0){
	  sleep(120);
	  return -1;
	}else{
	  if(rc<0){
	    perror("read");
	    sleep(120);
	    return -1;
	  }
	}
	hasPassed=1;
	res=atoi(buf);
	unlink(sname);
	if(res==3){
	  input=*real_outputs;
	  while(input!=NULL){
	    // TODO: parse all outputs including inner outputs if required.
	    if(input->child==NULL){
	      //map* dataPath=getMapFromMaps(*main_conf,"main","dataPath");
	      map* generatedFile=getMap(input->content,"generated_file");
	      if(generatedFile!=NULL){
		char* filename=strrchr(generatedFile->value,'/');
		char* targetPath=(char*)malloc((strlen(tmpPath->value)+strlen(filename)+2)*sizeof(char));
		sprintf(targetPath,"%s/%s",tmpPath->value,filename);
		test=ssh_connect(*main_conf);
		if(ssh_fetch(*main_conf,targetPath,generatedFile->value)==0){
		  setMapInMaps(*real_outputs,input->name,"generated_file",targetPath);
		}else{
		  char *tmpStr=(char*)malloc((strlen(filename)+strlen(_("Unable to fetch the remote file for %s"))+1)*sizeof(char));
		  sprintf(tmpStr,_("Unable to fetch the remote file for %s"),filename);
		  setMapInMaps(*main_conf,"lenv","message",tmpStr);
		  free(tmpStr);
		  return SERVICE_FAILED;
		}
	      }
	    }/*else{
	     // Generate the nested outputs based on each input value
	     if(getMaps(*real_inputs,input->name)!=NULL && getMapFromMaps(*real_inputs,input->name,"mimeType")!=NULL){
	     // Input was ComplexeData
	     maps* output=getMaps(*real_outputs,input->name);
	     map* cache=getMapFromMaps(*real_inputs,input->name,"cache_file");
	     setMapInMaps(output->child,"download","generated_file",cache->value);
	     setMapInMaps(output->child,"WCS_LINK","generated_file",cache->value);
	     setMapInMaps(output->child,"WCS_LINK","useMs","true");
	     setMapInMaps(output->child,"WMS_LINK","generated_file",cache->value);
	     setMapInMaps(output->child,"WMS_LINK","useMs","true");
	     }
	     }*/
	    input=input->next;
	  }
	}
      }
      if(hasPassed<0){
	perror("Failed to read");
	setMapInMaps(*main_conf,"lenv","message",_("Unable to parse the value returned by remote execution"));
	sleep(120);
	return SERVICE_FAILED;
      }
    }
    //}
  ssh_close(*main_conf);
  //sleep(120);
  return res;
}

    
