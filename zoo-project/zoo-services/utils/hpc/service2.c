/**
 * Author : Gérald FENOY
 *
 * Copyright 2017 GeoLabs SARL. All rights reserved.
 *
 * This work was supported by public funds received in the framework of GEOSUD,
 * a project (ANR-10-EQPX-20) of the program "Investissements d'Avenir" managed
 * by the French National Research Agency
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


#include "service.h"
#include "service_internal.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include <dirent.h>
#include <string.h>
#include <cerrno>

extern "C" {

  /** 
   * Retrieve the error message which occured when trying to remove a file
   * (return values can be EACCES, EBUSY, EFAULT, EIO, EISDIR, ELOOP, 
   * ENAMETOOLONG, ENOENT, ENOMEM, ENOTDIR, EPERM, EROFS, UNKNOWN).
   *
   * @param errn
   * @result a string corresponding to the error number 
   */ 
  char* fetchErrno(int errn){
   switch(errn){
    case EACCES:
	return "EACCES";
    case EBUSY:
	return "EBUSY";
    case EFAULT:
	return "EFAULT";
    case EIO:
	return "EIO";
    case EISDIR:
	return "EISDIR";
    case ELOOP:
	return "ELOOP";
    case ENAMETOOLONG:
	return "ENAMETOOLONG";
    case ENOENT:
	return "ENOENT";
    case ENOMEM:
	return "ENOMEM";
    case ENOTDIR:
	return "ENOTDIR";
    case EPERM:
	return "EPERM";
    case EROFS:
	return "EROFS";
    default:
	return "UNKNOWN";
   }
  }

  /**
   * Try to delete a cache file.
   * 
   * @param storage the full path to store the file
   * @param filename the filename to store
   * @param ext the extention of the file
   * @return 0 in case of success
   */
  int tryDeleteCacheFile(const char* storage,const char* filename,const char* ext){
    char *filename_noext=(char*)malloc((strlen(filename)-3)*sizeof(char));
    snprintf(filename_noext,strlen(filename)-4,"%s",filename);
    char* filename_full=(char*)malloc((strlen(filename)+1)*sizeof(char));
    sprintf(filename_full,"%s.%s",filename_noext,ext);
    char* fullpath=(char*)malloc((strlen(storage)+strlen(filename_full)+2)*sizeof(char));
    sprintf(fullpath,"%s/%s",storage,filename_full);
    if(unlink(fullpath)==0){
      // TODO store the filename_full in the deletedfiles
      fprintf(stderr,"#### DeleteData #### %s %d %s has been successfully deleted\n",__FILE__,__LINE__,filename_full);
    }else{
      fprintf(stderr,"#### DeleteData #### unable to delete %s \n",fullpath);
    }
    free(fullpath);
    free(filename_full);
    return 0;
  }  

  /**
   * Try to delete a data file.
   * 
   * @param storage the full path to store the file
   * @param filename the filename to store
   * @return 0 in case of success
   */
  int tryDeleteDataFile(const char* storage,const char* filename){
    char* fullpath=NULL;
    if(filename!=NULL){
      fullpath=(char*)malloc((strlen(storage)+strlen(filename)+2)*sizeof(char));
      sprintf(fullpath,"%s/%s",storage,filename);
    }
    else
      fullpath=zStrdup(storage);
    if(unlink(fullpath)==0){
      // TODO store the filename_full in the deletedfiles
      fprintf(stderr,"#### DeleteData #### %s %d %s has been successfully deleted\n",__FILE__,__LINE__,(filename!=NULL && strlen(filename)>0?filename:fullpath));
    }else{
      fprintf(stderr,"#### DeleteData #### unable to delete %s %s \n",fullpath,fetchErrno(errno));
    }
    free(fullpath);
    return 0;
  }  
  
  
  /**
   * DeleteData ZOO-Service :
   * This service is used in the ZOO-Project to delete the data file
   * associated with input or output.
   */
  ZOO_DLL_EXPORT int DeleteData(maps*& conf,maps*& inputs,maps*& outputs){
    map* dataPath=getMapFromMaps(conf,"main","dataPath");
    map* tmpPath=getMapFromMaps(conf,"main","tmpPath");
    map* cacheDir=getMapFromMaps(conf,"main","cacheDir");
    map* jobid=getMapFromMaps(inputs,"jobid","value");
    map* filename=getMapFromMaps(inputs,"filename","value");
    map* ioname=getMapFromMaps(inputs,"ioname","value");
    char tmp0[4];
    sprintf(tmp0,"%c%c%c",filename->value[strlen(filename->value)-3],filename->value[strlen(filename->value)-2],filename->value[strlen(filename->value)-1]);
    char *cfilename=NULL;
    if(strcasecmp(tmp0,"zca")==0){
      cfilename=(char*) malloc((strlen(filename->value)+strlen(cacheDir->value)+2)*sizeof(char));
      sprintf(cfilename,"%s/%s",cacheDir->value,filename->value);
    }
    else{
      cfilename=(char*) malloc((strlen(filename->value)+strlen(tmpPath->value)+2)*sizeof(char));
      sprintf(cfilename,"%s/%s",tmpPath->value,filename->value);
    }
    zooLock* lck=lockFile(conf,cfilename,'w');
    char** deletedfiles;
    if(lck!=NULL){
      if(strcasecmp(tmp0,"zca")==0){
	// Read the zcp file to verify if it comes from a shared source
	char *filename_noext=(char*)malloc((strlen(filename->value)-3)*sizeof(char));
	snprintf(filename_noext,strlen(filename->value)-4,"%s",filename->value);
	char* filename_full=(char*)malloc((strlen(filename->value)+1)*sizeof(char));
	sprintf(filename_full,"%s.zcp",filename_noext);
	char* fullpath=(char*)malloc((strlen(cacheDir->value)+strlen(filename_full)+2)*sizeof(char));
	sprintf(fullpath,"%s/%s",cacheDir->value,filename_full);
	FILE* f0=fopen(fullpath,"rb");
	char *fcontent=NULL;
	if(f0!=NULL){
	  long flen;
	  fseek (f0, 0, SEEK_END);
	  flen = ftell (f0);
	  fseek (f0, 0, SEEK_SET);
	  fcontent = (char *) malloc ((flen + 1) * sizeof (char));
	  fread(fcontent,flen,1,f0);
	  fcontent[flen]=0;
	  fclose(f0);
	}
	if(fcontent!=NULL && strcasecmp(fcontent,"SHARED")!=0){
	  // Delete associated zcm, zcp and maps files
	  tryDeleteCacheFile(cacheDir->value,filename->value,"zca");
	  tryDeleteCacheFile(cacheDir->value,filename->value,"zcm");
	  tryDeleteCacheFile(cacheDir->value,filename->value,"zcp");
	  tryDeleteCacheFile(tmpPath->value,filename->value,"maps");
	  // Delete <input>_<sid>.map
	  char* datafile=(char*)malloc((strlen(jobid->value)+strlen(ioname->value)+19)*sizeof(char));
	  sprintf(datafile,"%s_%s.map",ioname->value,jobid->value);
	  tryDeleteDataFile(dataPath->value,datafile);
	  free(datafile);
	}else{
	  setMapInMaps(conf,"lenv","message",_ss("The file you try to delete is a shared ressource and cannot be deleted by nature."));
	  unlockFile(conf,lck);
	  return SERVICE_FAILED;
	}
	setMapInMaps(outputs,"Result","value",_ss("The input data has been correclty removed"));
      }else{
	char tmp1[8];
	snprintf(tmp1,7,"%s",filename->value);
	if(strcasecmp(tmp1,"output")==0 || strstr(filename->value,".zca")!=NULL || strcasecmp(tmp1,"input_")==0){
	  tryDeleteDataFile(tmpPath->value,filename->value);
          char *tmp=zStrdup(filename->value);
	  tmp[strlen(tmp)-strlen(strrchr(tmp,'.'))]=0;
	  char *mapsfile=(char*)malloc((strlen(tmp)+6)*sizeof(char));
	  sprintf(mapsfile,"%s.maps",tmp);
          char* mapPath=(char*)malloc((strlen(tmpPath->value)+strlen(mapsfile)+2)*sizeof(char));
          sprintf(mapPath,"%s/%s",tmpPath->value,mapsfile);
	  FILE* myMapsfile=fopen(mapPath,"r");
          if(myMapsfile!=NULL){
	    char *buffer=(char*)malloc(1024*sizeof(char));
	    while(fgets(buffer, 1024, myMapsfile) != NULL){
	      buffer[strlen(buffer)-1]=0;
	      tryDeleteDataFile(buffer,NULL);
	    }
	    free(buffer);
	    fclose(myMapsfile);
          }
          tryDeleteDataFile(mapPath,NULL);
          free(mapPath);
          free(mapsfile);
	  // Delete ZOO_DATA_<output>_<sid>.data and <output>_<sid>.map
	  char* datafile=(char*)malloc((strlen(jobid->value)+strlen(ioname->value)+19)*sizeof(char));
	  sprintf(datafile,"ZOO_DATA_%s_%s.data",ioname->value,jobid->value);
	  tryDeleteDataFile(dataPath->value,datafile);
	  free(datafile);
	  datafile=(char*)malloc((strlen(jobid->value)+strlen(ioname->value)+19)*sizeof(char));
	  sprintf(datafile,"%s_%s.map",ioname->value,jobid->value);
	  tryDeleteDataFile(dataPath->value,datafile);
	  free(datafile);
	  char* webServices[3]={"wms","wfs","wcs"};
	  int i=0;
	  // Delete ZOO_DATA_<ws>_link_<sid>.data and <ws>_link_<sid>.map
	  // with <ws> is the corresponding OGC web service (wms,wfs,wcs)
	  for(i=0;i<3;i++){
	    datafile=(char*)malloc((strlen(jobid->value)+24)*sizeof(char));
	    sprintf(datafile,"ZOO_DATA_%s_link_%s.data",webServices[i],jobid->value);
	    tryDeleteDataFile(dataPath->value,datafile);
	    free(datafile);
	    datafile=(char*)malloc((strlen(jobid->value)+14)*sizeof(char));
	    sprintf(datafile,"%s_link_%s.map",webServices[i],jobid->value);
	    tryDeleteDataFile(dataPath->value,datafile);
	    free(datafile);
	  }
	  setMapInMaps(outputs,"Result","value",_ss("The output data has been correclty removed"));
	}else{
	  setMapInMaps(conf,"lenv","message",_ss("The file you try to delete is nor an input, nor an output."));
	  unlockFile(conf,lck);
	  return SERVICE_FAILED;
	}
      }
      unlockFile(conf,lck);
    }
    else{
      setMapInMaps(conf,"lenv","message",_ss("Failed to acquire lock for deletion, please try again later."));
      return SERVICE_FAILED;
    }
    return SERVICE_SUCCEEDED;
  }

}
