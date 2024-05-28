/*
 * Author : Gérald FENOY
 *
 *  Copyright 2008-2023 GeoLabs SARL. All rights reserved.
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
 
extern "C" int yylex ();
extern "C" int crlex ();

#if defined(META_DB) || defined(USE_AMQP)
#include "ogrsf_frmts.h"
#if GDAL_VERSION_MAJOR >= 2
#include <gdal_priv.h>
#endif
#endif

#ifdef USE_OTB
#include "service_internal_otb.h"
#endif

#ifdef USE_R
#include "service_internal_r.h"
#endif

#ifdef USE_HPC
#include "service_internal_hpc.h"
#endif

#ifdef USE_PYTHON
#include "service_internal_python.h"
#endif

#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "ulinet.h"

#include <libintl.h>
#include <locale.h>
#include <string.h>

#include "service_internal.h"
#include "server_internal.h"
#include "response_print.h"
#include "request_parser.h"
#include "service.h"


#if defined(WIN32) || defined(USE_JSON)
#include "caching.h"
#endif
#include "sqlapi.h"

#ifdef META_DB
#include "meta_sql.h"
#endif

#ifdef USE_SAGA
#include "service_internal_saga.h"
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

#ifdef USE_NODEJS
#include "service_internal_nodejs.h"
#endif

#ifdef USE_RUBY
#include "service_internal_ruby.h"
#endif

#ifdef USE_PERL
#include "service_internal_perl.h"
#endif

#ifdef USE_MONO
#include "service_internal_mono.h"
#endif

#if defined(USE_CALLBACK) || defined(USE_JSON)
#include "service_callback.h"
#endif

#ifdef USE_JSON
#include "service_json.h"
#include "json_tokener.h"
#include "json_object.h"
#include "json.h"
#endif

#ifdef USE_AMQP
#include "service_internal_amqp.h"
#include <sys/wait.h>
#endif

#include <dirent.h>
#include <signal.h>
#ifndef WIN32
#include <execinfo.h>
#endif
#include <unistd.h>
#ifndef WIN32
#include <dlfcn.h>
#include <libgen.h>
#else
#include <windows.h>
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define pid_t int;
#endif
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include "cgic.h"

#ifndef WIN32
extern char **environ;
#endif

#ifdef WIN32
extern "C"
{
  __declspec (dllexport) char *strcasestr (char const *pccA, char const *pccB)
#ifndef USE_MS
  {
    char *pcX = zStrdup (pccA);
    char *pcY = zStrdup (pccB);

    pcX = _strlwr (pcX);
    pcY = _strlwr (pcY);
    char *pcPos = strstr (pcX, pcY);
    char *pcRet = pcPos == NULL ? NULL : (char *) (pccA + (pos - pcX));
    free (pcX);
    free (pcY);
    return pcRet;
  };
#else
   ;
#endif
}
#endif

/**
 * Translation function for zoo-kernel
 */
#define _(String) dgettext ("zoo-kernel",String)
/**
 * Translation function for zoo-service
 */
#define __(String) dgettext ("zoo-service",String)

#ifdef WIN32
#ifndef PROGRAMNAME
#define PROGRAMNAME "zoo_loader.cgi"
#endif
#endif


/**
 * Close any sql backend connexion and call end_sql.
 *
 * @param pmsConf the maps pointer to the main configuration file
 * @see close_sql,end_sql
 */
void cleanUpSql(maps* pmsConf){
#if defined(META_DB) || defined(RELY_ON_DB)
  map* pmDsNb=getMapFromMaps(pmsConf,"lenv","ds_nb");
  if(pmDsNb!=NULL && atoi(pmDsNb->value)>1)
    close_sql(pmsConf,1);
  close_sql(pmsConf,0);
  end_sql();
#endif
}

/**
 * Replace a char by another one in a string
 *
 * @param pcOrigin the string to update
 * @param cToReplace the char to replace
 * @param cToReplaceBy the char that will be used
 */
void
translateChar (char *pcOrigin, char cToReplace, char cToReplaceBy)
{
  int i = 0, len = strlen (pcOrigin);
  for (i = 0; i < len; i++)
    {
      if (pcOrigin[i] == cToReplace)
        pcOrigin[i] = cToReplaceBy;
    }
}


/**
 * Create PID (SID) file referencing the OS process identifier (service id)
 *
 * @param pmsConf the conf maps containing the main.cfg settings
 * @param pcPath the path to store the sid file
 * @param pcUsid the usid value
 * @param pcExtension the file extension (sid or pid)
 * @param pcValue the value to be stored
 */
int createXidFile(maps* pmsConf,char* pcPath,char* pcUsid,const char* pcExtension,char* pcValue){
  char* pcaPath =
    (char *)
    malloc ((strlen (pcPath) + strlen (pcUsid) +
	     strlen (pcExtension) + 3) * sizeof (char));
  char* pcaName = (char*) malloc(10*sizeof(char));
  sprintf (pcaPath, "%s/%s.%s", pcPath, pcUsid,pcExtension);
  sprintf (pcaName, "file.%s", pcExtension);
  setMapInMaps (pmsConf, "lenv", pcaName, pcaPath);
  FILE* pfFile = fopen (pcaPath, "w+");
  free(pcaPath);
  free(pcaName);
  if(pfFile!=NULL){
    fprintf(pfFile,"%s",pcValue);
    fflush(pfFile);
  }else
    return 1;
  return 0;
}

/**
 * Create the PID and SID files referencing the OS process identifier and the
 * service id, respectively.
 *
 * @param pmsConf the conf maps containing the main.cfg settings
 * @param pcPath the path to store the files
 */
int createSPidFile(maps* pmsConf,char* pcPath){
  char* pcaPid=(char*) malloc(100*sizeof(char));
  map *pmUsid=getMapFromMaps(pmsConf,"lenv","usid");
  map *pmOsid=getMapFromMaps(pmsConf,"lenv","osid");
  sprintf(pcaPid,"%d",zGetpid());
  if(createXidFile(pmsConf,pcPath,pmUsid->value,"sid",pmOsid->value)!=0)
    return 1;
  if(createXidFile(pmsConf,pcPath,pmUsid->value,"pid",pcaPid)!=0){
    free(pcaPid);
    return 1;
  }
  free(pcaPid);
  return 0;
}

#ifndef RELY_ON_DB
// TODO: implement this function in service_internal.c
// Implement user filtering when not relying on DB 
void filterJobByUser(maps* pmsConf,char** pcaClauseFinal,char* pcaClauseDate){
}
#endif

/**
 * Create a _lenv.cfg file containing the lenv map
 *
 * @param pmsConf the conf maps containing the main.cfg settings
 * @param pcPath the path to store the sid file
 * @param pcUsid the usid value
 * @param pcValue the value to be stored
 */
int createLenvFile(maps* pmsConf,char* pcPath,char* pcUsid){
  char *pcaPath =
    (char *) malloc ((strlen (pcPath) + strlen (pcUsid) + 12) * sizeof (char));
  sprintf (pcaPath, "%s/%s_lenv.cfg", pcPath, pcUsid);
  maps* pmsLenv=getMaps(pmsConf,"lenv");
  dumpMapsToFile(pmsLenv,pcaPath,1);
  free(pcaPath);
  return 0;
}

/**
 * Dump back the final file fbkp1 to fbkp
 *
 * @param pmsConf the conf maps containing the main.cfg settings
 * @param fbkp the string corresponding to the name of the file
 * @param fbkp1 the string corresponding to the name of the file
 */
int dumpBackFinalFile(maps* pmsConf,char* fbkp,char* fbkp1)
{
  FILE *f2 = fopen (fbkp1, "rb");
  if(f2==NULL)
    return -1;
#ifndef RELY_ON_DB
  semid lid = getShmLockId (pmsConf, 1);
  if (lid < 0)
    return -1;
  lockShm (lid);
#endif
  FILE *f3 = fopen (fbkp, "wb+");
  if(f3==NULL)
    return -1;
  fseek (f2, 0, SEEK_END);
  long flen = ftell (f2);
  fseek (f2, 0, SEEK_SET);
  char *tmps1 = (char *) malloc ((flen + 1) * sizeof (char));
  fread (tmps1, flen, 1, f2);
  fwrite (tmps1, 1, flen, f3);
  fclose (f2);
  fclose (f3);
  free(tmps1);
  return 1;
}

/**
 * Checks if the zooServicesNamespace map is present in the main map;
 * if it is, the path to the directory where the ZOO-kernel should search for service providers will be updated.
 *
 * @param pmsConf the conf maps containing the main.cfg settings
 * @param oldPath default location where the ZOO-kernel should search for service providers
 * @param newPath location where the ZOO-kernel should search for service providers considering the namespace
 * @param maxSize maximum number of bytes to be used in the newPath buffer.
 */
int getServicesNamespacePath(maps* pmsConf,char* oldPath,char* newPath,int maxSize){
  map *zooServicesNamespaceMap = getMapFromMaps (pmsConf, "zooServicesNamespace", "namespace");
  map *servicesNamespaceParentPath = getMapFromMaps (pmsConf, "servicesNamespace", "path");
  memset(newPath,0,maxSize);

  if (zooServicesNamespaceMap && strcmp(zooServicesNamespaceMap->value,"anonymous") == 0 ){
    if (oldPath){
      snprintf (newPath,maxSize, "%s", oldPath);
    }
  } else if (zooServicesNamespaceMap && zooServicesNamespaceMap->value && servicesNamespaceParentPath && servicesNamespaceParentPath->value){

    char *path=(char*)malloc(1024*sizeof(char));
    snprintf (path,maxSize, "%s/%s", servicesNamespaceParentPath->value,zooServicesNamespaceMap->value);
    if (strstr(oldPath,path)) {
      snprintf (newPath,maxSize, "%s", oldPath);
    } else {
      snprintf(newPath, maxSize, path);
    }
    free(path);
  } else {
    if (oldPath)
      snprintf (newPath,maxSize, "%s", oldPath);
  }
  return 0;
}

/***
 * Checks if the env variable SERVICES_NAMESPACE is set;
 * if it is, the zooServicesNamespace map will be added to the conf map
 *
 * @param pmsConf the conf maps containing the main.cfg settings
 */
int addServicesNamespaceToMap(maps** pmsConf){
  int ret=0;
  int ei = 1;
  char **orig = environ;
  char *s=*orig;
  char* pcNamespaceName=NULL;

  if(orig!=NULL)
    for (; s; ei++ ) {
      // retrieving service workspace
      if(strstr(s,"=")!=NULL && strlen(strstr(s,"="))>1){
	if (strstr(s,"SERVICES_NAMESPACE")!=NULL){
	  char* baseU=strchr(s,'=');
	  if (strlen(baseU)>1) {
	    pcNamespaceName = ++baseU;
#ifdef DEBUG
	    fprintf(stderr,"zooServicesNamespace: %s\n",pcNamespaceName);
#endif

	    // checking if namespace folder exists
	    char *pcaNamespaceFolder = (char *) malloc(1024*sizeof(char));
	    //memset(pcaNamespaceFolder, '\0', 1024);
	    map *servicesNamespaceParentFolder = getMapFromMaps(*pmsConf, "servicesN amespace", "path");
	    if(servicesNamespaceParentFolder!=NULL){
	      sprintf(pcaNamespaceFolder, "%s/%s", servicesNamespaceParentFolder->value, pcNamespaceName);
	      DIR *dir = opendir(pcaNamespaceFolder);
	      if (dir) {
		// creating a zooServicesNamespace map
		// the map will contain the namespace name
		if(getMaps(*pmsConf,"zooServicesNamespace")==NULL){
		  maps *pmsTmp = createMaps("zooServicesNamespace");
		  pmsTmp->content = createMap ("namespace", pcNamespaceName);
		  addMapsToMaps(pmsConf, pmsTmp);
		  freeMaps(&pmsTmp);
		  free(pmsTmp);
		  pmsTmp=NULL;
		}else{
		  //setMapInMaps(*pmsConf,"lenv","namespace",pcNamespaceName);
		  setMapInMaps(*pmsConf,"zooServicesNamespace","namespace",pcNamespaceName);
		}
		//setMapInMaps(pmsConf,"zooServicesNamespace","path",pcaNamespaceFolder);
		closedir(dir);
	      } else {
		map* error=createMap("code","BadRequest");
		addToMap(error,"message",_("The resource is not available"));
		printExceptionReportResponseJ(pmsConf,error);
		ret = 1;
		freeMap(&error);
		free(error);
	      }
	      free(pcaNamespaceFolder);
	    }
	  }
	}
      }
      s = *(orig+ei);
    }
  return ret;
}

/***
 * Updates the rootUrl property in the map m by concatenating the rootHost, the
 * namespace and the rootPath.
 *
 * @param m the conf maps containing the main.cfg and oas.cfg settings
 */
void setRootUrlMap(maps* pmConf){
    char *pcaRootUrl= NULL;
    map *zooServicesNamespaceMap = getMapFromMaps (pmConf,
						   "zooServicesNamespace",
						   "namespace");
    map* pmRootHost=getMapFromMaps(pmConf,"openapi","rootHost");
    map* pmRootPath=getMapFromMaps(pmConf,"openapi","rootPath");
#ifdef DEBUG
    fprintf (stderr, "zooServicesNamespaceMap: %s\n",
	     zooServicesNamespaceMap->value);
    fprintf (stderr, "rootHost: %s\n", pmRootHost->value);
    fprintf (stderr, "rootPath: %s\n", pmRootPath->value);
#endif
    if (zooServicesNamespaceMap!=NULL && zooServicesNamespaceMap->value &&
	strcmp(zooServicesNamespaceMap->value,"generalNamespace") != 0 ){
      if(pmRootHost!=NULL && pmRootPath!=NULL){
        pcaRootUrl=(char*)
	  malloc((strlen(pmRootHost->value)+
		  strlen(pmRootPath->value)+
		  strlen(zooServicesNamespaceMap->value)+13)*sizeof(char));
        sprintf(pcaRootUrl,"%s/%s/%s",
		pmRootHost->value,
		zooServicesNamespaceMap->value,
		pmRootPath->value);
      }else
	if(pmRootPath!=NULL){
	  pcaRootUrl=(char*)
	    malloc(((strlen(pmRootPath->value)+
		     strlen(zooServicesNamespaceMap->value)+13)*sizeof(char)));
	  sprintf(pcaRootUrl,"%s/%s",
		  zooServicesNamespaceMap->value, pmRootPath->value);
	}
    } else {
      if(pmRootHost!=NULL && pmRootPath!=NULL){
        pcaRootUrl=(char*)
	  malloc((strlen(pmRootHost->value)+
		  strlen(pmRootPath->value)+13)*sizeof(char));
        sprintf(pcaRootUrl,"%s/%s",pmRootHost->value, pmRootPath->value);
      }else
	if(pmRootPath!=NULL){
	  pcaRootUrl=(char*)
	    malloc(((strlen(pmRootPath->value)+13)*sizeof(char)));
	  sprintf(pcaRootUrl, "/%s",pmRootPath->value);
      }
    }
    if(pcaRootUrl!=NULL){
#ifdef DEBUG
      fprintf (stderr, "rootUrl: %s\n", pcaRootUrl);
#endif
      setMapInMaps(pmConf,"openapi","rootUrl",pcaRootUrl);
      free(pcaRootUrl);
    }
}

/**
 * Recursivelly parse zcfg starting from the ZOO-Kernel cwd.
 * Call the func function given in arguments after parsing the ZCFG file.
 *
 * @param m the conf maps containing the main.cfg settings
 * @param r the registry containing profiles hierarchy
 * @param n the root XML Node to add the sub-elements
 * @param conf_dir the location of the main.cfg file (basically cwd)
 * @param prefix the current prefix if any, or NULL
 * @param saved_stdout the saved stdout identifier
 * @param level the current level (number of sub-directories to reach the
 * current path)
 * @param func a pointer to a function having 5 parameters
 *  (registry*, maps*, xmlDocPtr, xmlNodePtr, and service*).
 * @see inheritance, readServiceFile
 */
int
_recursReaddirF ( maps * m, registry *r, void* doc1, void* n1, char *conf_dir,
		 char *prefix, int saved_stdout, int level,
		 void (func) (registry *, maps *, void*, void*, service *) )
{

  struct dirent *dp;
  int scount = 0;
  xmlDocPtr doc=(xmlDocPtr) doc1;
  xmlNodePtr n=(xmlNodePtr) n1;
  map* pmContinue=getMapFromMaps(m,"lenv","can_continue");

  if (conf_dir == NULL)
    return 1;
  DIR *dirp = opendir (conf_dir);
  if (dirp == NULL)
    {
      if (level > 0)
        return 1;
      else
        return -1;
    }
  char tmp1[25];
  sprintf (tmp1, "sprefix_%d", level);
  char levels[17];
  sprintf (levels, "%d", level);
  setMapInMaps (m, "lenv", "level", levels);
  while ((dp = readdir (dirp)) != NULL)
    if ((dp->d_type == DT_DIR || dp->d_type == DT_LNK) && dp->d_name[0] != '.'
        && strstr (dp->d_name, ".") == NULL)
      {

        char *tmp =
          (char *) malloc ((strlen (conf_dir) + strlen (dp->d_name) + 2) *
                           sizeof (char));
        sprintf (tmp, "%s/%s", conf_dir, dp->d_name);

        if (prefix != NULL)
          {
            prefix = NULL;
          }
        prefix = (char *) malloc ((strlen (dp->d_name) + 2) * sizeof (char));
        sprintf (prefix, "%s.", dp->d_name);

        //map* tmpMap=getMapFromMaps(m,"lenv",tmp1);

        int res;
        if (prefix != NULL)
          {
            setMapInMaps (m, "lenv", tmp1, prefix);
            char levels1[17];
            sprintf (levels1, "%d", level + 1);
            setMapInMaps (m, "lenv", "level", levels1);
            res =
              _recursReaddirF (m, r, doc, n, tmp, prefix, saved_stdout, level + 1,
                              func);
            sprintf (levels1, "%d", level);
            setMapInMaps (m, "lenv", "level", levels1);
            free (prefix);
            prefix = NULL;
          }
        else
          res = -1;
        free (tmp);
        if (res < 0)
          {
            return res;
          }
      }
    else
      {
        char* extn = strstr(dp->d_name, ".zcfg");
        if(dp->d_name[0] != '.' && extn != NULL && strlen(extn) == 5 && strlen(dp->d_name)>5)
          {
            int t;
            char tmps1[1024];
            memset (tmps1, 0, 1024);
            snprintf (tmps1, 1024, "%s/%s", conf_dir, dp->d_name);

	    char *tmpsn = (char*)malloc((strlen(dp->d_name)-4)*sizeof(char));//zStrdup (dp->d_name);
	    memset (tmpsn, 0, strlen(dp->d_name)-4);
	    snprintf(tmpsn,strlen(dp->d_name)-4,"%s",dp->d_name);
            
            map* import = getMapFromMaps (m, IMPORTSERVICE, tmpsn);
            if (import == NULL || import->value == NULL || zoo_path_compare(tmps1, import->value) != 0 ) { // service is not in [include] block
	      if(compareCnt(m,"serviceCntSkip","eupper") && compareCnt(m,"serviceCntLimit","lower")){
		service *s1 = createService();
		if (s1 == NULL)
		  {
		    zDup2 (saved_stdout, fileno (stdout));
		    errorException (&m, _("Unable to allocate memory"),
				    "InternalError", NULL);
		    return -1;
		  }
#ifdef DEBUG
		fprintf (stderr, "#################\n%s\n#################\n",
			 tmps1);
#endif
		t = readServiceFile (m, tmps1, &s1, tmpsn);
		if ( t < 0 )
		  {
		    fflush(stderr);
		    fflush(stdout);
		    zDup2 (saved_stdout, fileno (stdout));
		    zClose(saved_stdout);
		    if(pmContinue!=NULL && strncasecmp(pmContinue->value,"false",4)==0){
		      map *tmp00 = getMapFromMaps (m, "lenv", "message");
		      char tmp01[1024];
		      if (tmp00 != NULL)
			sprintf (tmp01, _("Unable to parse the ZCFG file: %s (%s)"),
				 dp->d_name, tmp00->value);
		      else
			sprintf (tmp01, _("Unable to parse the ZCFG file: %s."),
				 dp->d_name);
		      errorException (&m, tmp01, "InternalError", NULL);
		    }
		    freeService (&s1);
		    free (s1);
		    free (tmpsn);
		    return -1;
		  }
#ifdef DEBUG
		dumpService (s1);
		fflush (stdout);
		fflush (stderr);
#endif
		if(s1!=NULL)
		  inheritance(r,&s1);
		func (r, m, doc, n, s1);
		freeService (&s1);
		free (s1);
	      }
              scount++;
#ifdef DRU_ENABLED
	      bool bIsDeployUndeployService=serviceIsDRU(m,tmpsn);
	      if(!bIsDeployUndeployService){
#endif
		if(!serviceIsFilter(m,tmpsn)){
		  updateCnt(m,"serviceCnt","incr");
		  updateCnt(m,"serviceCounter","incr");
		}
#ifdef DRU_ENABLED
	      }
	      free (tmpsn);
	      tmpsn=NULL;
#endif
	      if(compareCnt(m,"serviceCntLimit","equal")){
		// In case we are willing to count the number of services, we
		// can still continue and not return any value bellow
		if(getMapFromMaps(m,"lenv","serviceCntNext")==NULL)
		  setMapInMaps(m,"lenv","serviceCntNext","true");
		//(void) closedir (dirp);
		//return 1;
	      }
            }
	    if(tmpsn!=NULL)
	      free(tmpsn);
          }
      }
  (void) closedir (dirp);
  return 1;
}

/**
 * Recursivelly parse zcfg from search path (services namespace and cwd).
 *
 * If the main cofiguration maps (pmsConf) get, in the openapi section, a
 * search_path key taking the value true, then the ZOO-Kernel default
 * search_path used for searching zcfg files (cwd) is used in addition to the
 * authenticated user namespace.
 *
 * @param pmsConf the conf maps containing the main.cfg settings
 * @param r the registry containing profiles hierarchy
 * @param n the root XML Node to add the sub-elements
 * @param conf_dir the location of the main.cfg file (basically cwd)
 * @param prefix the current prefix if any, or NULL
 * @param saved_stdout the saved stdout identifier
 * @param level the current level (number of sub-directories to reach the
 * current path)
 * @param func a pointer to a function having 4 parameters
 *  (registry*, maps*, xmlNodePtr and service*).
 * @see recursReaddirF
 */
int
recursReaddirF ( maps * pmsConf, registry *r, void* doc1, void* n1, char *conf_dir_,
		 char *prefix, int saved_stdout, int level,
		 void (func) (registry *, maps *, void*, void*, service *) )
{
  map* pmHasSearchPath=getMapFromMaps(pmsConf,"main","search_path");
  // if services namespace is present in the map, conf_dir will
  // point to the namespace services path else it will point to
  // the default service path
  char conf_dir[1024];
  int res=0;
  getServicesNamespacePath(pmsConf,conf_dir_,conf_dir,1024);
  if(pmHasSearchPath!=NULL && strncasecmp(pmHasSearchPath->value,"true",4)==0){
    setMapInMaps(pmsConf,"lenv","can_continue","true");
    int res=_recursReaddirF(pmsConf, r, doc1, n1, conf_dir_,prefix, saved_stdout,
			    level,func);
    if(strncmp(conf_dir,conf_dir_,strlen(conf_dir))!=0){

      setMapInMaps(pmsConf,"lenv","can_continue","false");
      res=_recursReaddirF(pmsConf, r, doc1, n1, conf_dir,prefix, saved_stdout,
			  level,func);
    }
  }else{
    setMapInMaps(pmsConf,"lenv","can_continue","false");
    res=_recursReaddirF(pmsConf, r, doc1, n1, conf_dir,prefix, saved_stdout,
			level,func);
  }
  return res;
}


/**
 * When th zcfg file is not found, print error message and cleanup memory
 * Note this function does free the *ppmsConf memory.
 *
 * @param zooRegistry the populated registry
 * @param ppmsConf the maps pointer address to the content of main.cfg file
 * @param zcfg the zcfg file name
 * @param code the string determining the nature of the error
 * @param locator the string determining which parameter the error refer to
 * @param orig the service name
 * @param corig the current service name (in case multiple services was parsed)
 * @param funcError the function used to print the error back
 */
void exitAndCleanUp(registry* zooRegistry, maps** ppmsConf,
		    const char* zcfg,const char* code,const char* locator,
		    char* orig,char* corig,
		    void (funcError) (maps**, map*)){
  maps* m=*ppmsConf;
  map *tmp00 = getMapFromMaps (m, "lenv", "message");
  char tmp01[1024];
  if (tmp00 != NULL)
    sprintf (tmp01,
	     _("Unable to parse the ZCFG file: %s (%s)"),
	     zcfg, tmp00->value);
  else
    sprintf (tmp01,
	     _("Unable to parse the ZCFG file: %s."),
	     zcfg);
  
  map* errormap = createMap("text", tmp01);
  map* tmpMap=getMapFromMaps(m,"main","executionType");
  char* errorCode=(char*)code;
  if(tmpMap!=NULL && strncasecmp(tmpMap->value,"json",4)==0)
    errorCode="NoSuchProcess";

  addToMap(errormap,"code", errorCode);
  addToMap(errormap,"locator", locator);
  funcError(&m,errormap);
  if(zooRegistry!=NULL){
    freeRegistry(&zooRegistry);
    free(zooRegistry);
  }
  free (orig);
  if (corig != NULL)
    free (corig);
  xmlCleanupParser ();
  zooXmlCleanupNs ();
  freeMap(&errormap);
  free(errormap);
  freeMaps(&m);
  free(m);
}


/**
 * Parse the ZOO-Service ZCFG to fill the service datastructure
 *
 * @param zooRegistry the populated registry
 * @param m the maps pointer to the content of main.cfg file
 * @param spService the pointer to the service pointer to be filled
 * @param request_inputs the map pointer for http request inputs
 * @param pcDir the path where the ZCFG files are stored
 * @param cIdentifier the service identifier
 * @param funcError the error function to be used in case of error
 */
int _fetchService(registry* zooRegistry,maps* m,service** spService, map* request_inputs,char* pcDir,char* cIdentifier,void (funcError) (maps**, map*)){
  char* tmps1=(char*)malloc(1024*sizeof(char));
  map *r_inputs=NULL;
  service* s1=*spService;
  map* pmExecutionType=getMapFromMaps(m,"main","executionType");
  map* pmContinue=getMapFromMaps(m,"lenv","can_continue");
  map* import = getMapFromMaps (m, IMPORTSERVICE, cIdentifier); 
  if (import != NULL && import->value != NULL) { 
    strncpy(tmps1, import->value, 1024);
    setMapInMaps (m, "lenv", "Identifier", cIdentifier);
    setMapInMaps (m, "lenv", "oIdentifier", cIdentifier);
  } 
  else {

    snprintf (tmps1, 1024, "%s/%s.zcfg", pcDir, cIdentifier);
#ifdef DEBUG
    fprintf (stderr, "Trying to load %s\n", tmps1);
#endif
    if (strstr (cIdentifier, ".") != NULL)
      {
	setMapInMaps (m, "lenv", "oIdentifier", cIdentifier);
	char *identifier = zStrdup (cIdentifier);
	parseIdentifier (m, pcDir, identifier, tmps1);
	map *tmpMap = getMapFromMaps (m, "lenv", "metapath");
	if (tmpMap != NULL)
	  addToMap (request_inputs, "metapath", tmpMap->value);
	free (identifier);
      }
    else
      {
	setMapInMaps (m, "lenv", "oIdentifier", cIdentifier);
	setMapInMaps (m, "lenv", "Identifier", cIdentifier);
      }
  }

  r_inputs = getMapFromMaps (m, "lenv", "Identifier");

#ifdef META_DB
  int metadb_id=_init_sql(m,"metadb");
  //FAILED CONNECTING DB
  if(getMapFromMaps(m,"lenv","dbIssue")!=NULL || metadb_id<=0){
    fprintf(stderr,"ERROR CONNECTING METADB\n");
  }
  if(metadb_id>0){
    *spService=extractServiceFromDb(m,cIdentifier,0);
    close_sql(m,metadb_id-1);
    s1=*spService;
  }
  if(s1!=NULL){
    inheritance(zooRegistry,spService);
#ifdef USE_HPC_NESTEDOUTPUTS
    addNestedOutputs(spService);
#endif
    if(zooRegistry!=NULL){
      freeRegistry(&zooRegistry);
      free(zooRegistry);
    }
  }else /* Not found in MetaDB */{
#endif
    *spService = createService();
    if (*spService == NULL)
      {
	freeMaps (&m);
	free (m);
	if(zooRegistry!=NULL){
	  freeRegistry(&zooRegistry);
	  free(zooRegistry);
	}
	map* error=createMap("code","InternalError");
	if(pmExecutionType!=NULL && strncasecmp(pmExecutionType->value,"xml",3)==0){
	    addToMap(error,"locator", "NULL");
	    addToMap(error,"text",_("Unable to allocate memory"));
	}
	else{
	  addToMap(error,"message",_("Unable to allocate memory"));
	}

	setMapInMaps(m,"lenv","status_code","404 Bad Request");
	funcError(&m,error);
	freeMap(&error);
	free(error);
	free(tmps1);
	return 1; /*errorException (&m, _("Unable to allocate memory"),
		    "InternalError", NULL);*/
      }

    int saved_stdout = zDup (fileno (stdout));
    zDup2 (fileno (stderr), fileno (stdout));
    int t = readServiceFile (m, tmps1, spService, cIdentifier);
    if(t>=0){
      inheritance(zooRegistry,spService);
#ifdef USE_HPC_NESTEDOUTPUTS
      addNestedOutputs(spService);
#endif
    }
    if(zooRegistry!=NULL){
      freeRegistry(&zooRegistry);
      free(zooRegistry);
    }
    fflush(stderr);
    fflush (stdout);
    zDup2 (saved_stdout, fileno (stdout));
    zClose (saved_stdout);
    if ( t < 0 )
      {
	r_inputs = getMapFromMaps (m, "lenv", "oIdentifier");
	if( r_inputs!=NULL &&
	    (pmContinue!=NULL && strncasecmp(pmContinue->value,"false",5)==0) ){
	  char *tmpMsg = (char *) malloc (2048 + strlen (r_inputs->value));
	  sprintf (tmpMsg,
		   _("The value for <identifier> seems to be wrong (%s). Please specify one of the processes in the list returned by a GetCapabilities request."),
		   r_inputs->value);
	  map* error=createMap("code","NoSuchProcess");
	  if(pmExecutionType!=NULL && strncasecmp(pmExecutionType->value,"xml",3)==0){
	    addToMap(error,"locator", "identifier");
	    addToMap(error,"text",tmpMsg);
	    addToMap(error,"code","InvalidParameterValue");
	  }
	  else{
	    addToMap(error,"message",tmpMsg);
	  }
	  //setMapInMaps(conf,"lenv","status_code","404 Bad Request");
#ifdef LOG_CONSOLE_ENABLED
    logConsoleMessage(tmpMsg);
#endif
	  funcError(&m,error);

	  //errorException (&m, tmpMsg, "InvalidParameterValue", "identifier");
	  free (tmpMsg);
	  free (*spService);
	  freeMap(&error);
	  free(error);
	  free(tmps1);
	  //freeMaps (&m);
	  //free (m);
	  return 1;
	}
	free (*spService);
	free(tmps1);
	return 1;
      }
#ifdef META_DB
  }
#endif
  free(tmps1);
  return 0;
}

/**
 * Parse the ZOO-Service ZCFG to fill the service datastructure
 *
 * @param zooRegistry the populated registry
 * @param m the maps pointer to the content of main.cfg file
 * @param spService the pointer to the service pointer to be filled
 * @param request_inputs the map pointer for http request inputs
 * @param pcDir the path where the ZCFG files are stored
 * @param cIdentifier the service identifier
 * @param funcError the error function to be used in case of error
 */
int fetchService(registry* zooRegistry,maps* m,service** spService, map* request_inputs,char* pcDir,char* cIdentifier,void (funcError) (maps**, map*)){
  map* pmHasSearchPath=getMapFromMaps(m,"main","search_path");
  // if services namespace is present in the map, conf_dir will
  // point to the namespace services path else it will point to
  // the default service path
  char conf_dir[1024];
  int res=0;
  getServicesNamespacePath(m,pcDir,conf_dir,1024);
  if(pmHasSearchPath!=NULL && strncasecmp(pmHasSearchPath->value,"true",4)==0){
    setMapInMaps(m,"lenv","can_continue","true");
    res=_fetchService(zooRegistry, m, spService, request_inputs, conf_dir,
			  cIdentifier, funcError);
    if(res!=0 && strncmp(conf_dir,pcDir,strlen(pcDir))!=0){
      setMapInMaps(m,"lenv","can_continue","false");
      res=_fetchService(zooRegistry, m, spService, request_inputs, pcDir,
			cIdentifier, funcError);
    }
  }else{
    setMapInMaps(m,"lenv","can_continue","false");
    res=_fetchService(zooRegistry, m, spService, request_inputs, conf_dir,
		      cIdentifier, funcError);
  }
  return res;
}

/**
 * Search services from various possible sources 
 *
 * @param zopRegistry the populated registry
 * @param m the maps pointer to the content of main.cfg file
 * @param r_inputs the service(s) name(s)
 * @param func the function used to print the result back
 * @param doc the xml document or NULL (for json)
 * @param n the xmlNode of JSON object pointer to the current element
 * @param conf_dir the directory where the main.cfg has been found
 * @param request_inputs the map pointer to the request KVP if any
 * @param funcError the function used to print the error back
 * @return 0 in case of success, 1 otherwise
 * @see exitAndCleanUp
 */
int _fetchServicesForDescription(registry* zooRegistry, maps** pmsConf, char* r_inputs,
				void (func) (registry *, maps *, void*, void*, service *),
				void* doc, void* n, char *conf_dir, map* request_inputs,
				void (funcError) (maps**, map*) ){
  char *orig = zStrdup (r_inputs);
  maps* m=*pmsConf;
  service* s1=NULL;
  int saved_stdout = zDup (fileno (stdout));
  int t;
  int scount = 0;
  struct dirent *dp;
  map* pmContinue=getMapFromMaps(m,"lenv","can_continue");


  zDup2 (fileno (stderr), fileno (stdout));  
  if (strcasecmp ("all", orig) == 0)
    {
      maps* imports = getMaps(m, IMPORTSERVICE); 
      if (imports != NULL) {       
	map* zcfg = imports->content;
            
	while (zcfg != NULL) {
	  if (zcfg->value != NULL) {
	    service* svc = (service*) malloc(SERVICE_SIZE);
	    if (svc == NULL || readServiceFile(m, zcfg->value, &svc, zcfg->name) < 0) {
	      // pass over silently
	      zcfg = zcfg->next;
	      continue;
	    }
	    inheritance(zooRegistry, &svc);
#ifdef USE_HPC_NESTEDOUTPUTS
	    addNestedOutputs(&svc);
#endif

	    func(zooRegistry, m, doc, n, svc);
	    freeService(&svc);
	    free(svc);                             
	  }
	  zcfg = zcfg->next;
	}            
      }
      if (int res =
	  _recursReaddirF (m, zooRegistry, doc, n, conf_dir, NULL, saved_stdout, 0,
			  func) < 0){
	zDup2 (saved_stdout, fileno (stdout));
	return res;
      }
#ifdef META_DB
      fetchServicesFromDb(zooRegistry,m,doc,n,func,0);
#endif
    }
  else
    {
      DIR *dirp = opendir (conf_dir);
      char *saveptr;
      char *tmps = strtok_r (orig, ",", &saveptr);

      char buff[256];
      char buff1[1024];
      while (tmps != NULL)
	{
	  int hasVal = -1;
	  char *corig = zStrdup (tmps);
	  map* import = getMapFromMaps (m, IMPORTSERVICE, corig);   
	  if (import != NULL && import->value != NULL) 
	    {
#ifdef META_DB
	      int metadb_id=_init_sql(m,"metadb");
	      service* s2=NULL;
	      if(metadb_id>0){
		s2=extractServiceFromDb(m,import->name,0);
		close_sql(m,metadb_id-1);
	      }
	      if(s2==NULL){
#endif
		s1 = createService();
		t = readServiceFile (m, import->value, &s1, import->name);

		if (t < 0) // failure reading zcfg
		  {
		    zDup2 (saved_stdout, fileno (stdout));
#ifdef META_DB
		    close_sql(m,metadb_id-1);
#endif
		    if(pmContinue!=NULL && strncasecmp(pmContinue->value,"false",5)==0)
		      exitAndCleanUp(zooRegistry, &m,
				     tmps,"InvalidParameterValue","identifier",
				     orig,corig,
				     funcError);
		    if(dirp!=NULL)
		      closedir (dirp);
                    return 1;
		  }
#ifdef DEBUG
		dumpService (s1);
#endif
		inheritance(zooRegistry,&s1);
#ifdef USE_HPC_NESTEDOUTPUTS
		addNestedOutputs(&s1);
#endif
		func (zooRegistry, m, doc, n, s1);
		freeService (&s1);
		free (s1);
		s1 = NULL;
		scount++;
		hasVal = 1;                
#ifdef META_DB
	      }
#endif
	    }
	  else if (strstr (corig, ".") != NULL)
	    {
	      parseIdentifier (m, conf_dir, corig, buff1);
	      map *tmpMap = getMapFromMaps (m, "lenv", "metapath");
	      if (tmpMap != NULL)
		addToMap (request_inputs, "metapath", tmpMap->value);
	      map *tmpMapI = getMapFromMaps (m, "lenv", "Identifier");
	      /**
	       * No support for dot in service name stored in metadb!?
	       #ifdef META_DB
	       service* s2=extractServiceFromDb(m,tmpMapI->value,0);
	       if(s2==NULL){
	       #endif
	      */
	      s1 = createService();
	      t = readServiceFile (m, buff1, &s1, tmpMapI->value);
	      if (t < 0)
		{
		  zDup2 (saved_stdout, fileno (stdout));
		  if(pmContinue!=NULL && strncasecmp(pmContinue->value,"false",5)==0)
		    exitAndCleanUp(zooRegistry, &m,
				   tmps,"InvalidParameterValue","identifier",
				   orig,corig,
				   funcError);
		  if(dirp!=NULL)
		    closedir (dirp);
		  return 1;
		}
#ifdef DEBUG
	      dumpService (s1);
#endif
	      inheritance(zooRegistry,&s1);
#ifdef USE_HPC_NESTEDOUTPUTS
	      addNestedOutputs(&s1);
#endif
	      func (zooRegistry, m, doc, n, s1);
	      freeService (&s1);
	      free (s1);
	      s1 = NULL;
	      scount++;
	      hasVal = 1;
	      setMapInMaps (m, "lenv", "level", "0");
	    }
	  else
	    {
#ifdef META_DB
	      int metadb_id=_init_sql(m,"metadb");
	      //FAILED CONNECTING DB
	      if(getMapFromMaps(m,"lenv","dbIssue")!=NULL || metadb_id<=0){
		fprintf(stderr,"ERROR CONNECTING METADB");
	      }
	      service* s2=NULL;
	      if(metadb_id>0){
		s2=extractServiceFromDb(m,corig,0);
		close_sql(m,metadb_id-1);
	      }
	      if(s2!=NULL){
		inheritance(zooRegistry,&s2);
#ifdef USE_HPC_NESTEDOUTPUTS
		addNestedOutputs(&s2);
#endif
		func (zooRegistry,m, doc, n, s2);
		freeService (&s2);
		free (s2);
		s2 = NULL;
		hasVal = 1;
	      }else /*TOTO*/{
#endif
		memset (buff, 0, 256);
		snprintf (buff, 256, "%s.zcfg", corig);
		memset (buff1, 0, 1024);
#ifdef DEBUG
		printf ("\n#######%s\n########\n", buff);
#endif
		while ((dp = readdir (dirp)) != NULL)
		  {
		    if (strcasecmp (dp->d_name, buff) == 0)
		      {
			memset (buff1, 0, 1024);
			snprintf (buff1, 1024, "%s/%s", conf_dir,
				  dp->d_name);
			s1 = createService();
			if (s1 == NULL)
			  {
			    fflush (stdout);
			    fflush (stderr);
			    zDup2 (saved_stdout, fileno (stdout));
			    zClose(saved_stdout);
			    map* errormap = createMap("text", _("Unable to allocate memory"));
			    addToMap(errormap,"code", "InternalError");
			    addToMap(errormap,"locator", "NULL");
			    funcError(&m,errormap);
			    if(dirp!=NULL)
			      closedir (dirp);
			    freeMap(&errormap);
			    free(errormap);
			    return -1;
			  }
#ifdef DEBUG_SERVICE_CONF
			fprintf
			  (stderr,"#################\n(%s) %s\n#################\n",
			   r_inputs->value, buff1);
#endif
			char *tmp0 = zStrdup (dp->d_name);
			tmp0[strlen (tmp0) - 5] = 0;
			t = readServiceFile (m, buff1, &s1, tmp0);
			free (tmp0);
			if (t < 0)
			  {
			    fflush (stdout);
			    fflush (stderr);
			    zDup2 (saved_stdout, fileno (stdout));
			    zClose(saved_stdout);
			    if(pmContinue!=NULL && strncasecmp(pmContinue->value,"false",5)==0)
			      exitAndCleanUp(zooRegistry, &m,
					     buff,"InternalError","NULL",
					     orig,corig,
					     funcError);
			    if(dirp!=NULL)
			      closedir (dirp);
			    return 1;
			  }
#ifdef DEBUG
			dumpService (s1);
#endif
			inheritance(zooRegistry,&s1);
#ifdef USE_HPC_NESTEDOUTPUTS
			addNestedOutputs(&s1);
#endif
			func (zooRegistry,m, doc, n, s1);
			freeService (&s1);
			free (s1);
			s1 = NULL;
			scount++;
			hasVal = 1;
		      }
		  }
#ifdef META_DB
	      }
#endif
	    }		      
	  if (hasVal < 0)
	    {
	      fflush (stdout);
	      fflush (stderr);
	      zDup2 (saved_stdout, fileno (stdout));
	      zClose(saved_stdout);
	      if(pmContinue!=NULL && strncasecmp(pmContinue->value,"false",5)==0)
		  fflush(stderr);
		exitAndCleanUp(zooRegistry, &m,
			       buff,"InvalidParameterValue","Identifier",
			       orig,corig,
			       funcError);
	      fflush(stdout);
	      if(dirp!=NULL)
		closedir (dirp);
	      return 1;
	    }
	  rewinddir (dirp);
	  tmps = strtok_r (NULL, ",", &saveptr);
	  if (corig != NULL)
	    free (corig);
	}
      if(dirp!=NULL)
	closedir (dirp);
    }
  fflush (stdout);
  fflush (stderr);
  zDup2 (saved_stdout, fileno (stdout));
  zClose(saved_stdout);
  free (orig);
  return 0;
}

/**
 * Search services from various possible sources 
 *
 * @param zopRegistry the populated registry
 * @param m the maps pointer to the content of main.cfg file
 * @param r_inputs the service(s) name(s)
 * @param func the function used to print the result back
 * @param doc the xml document or NULL (for json)
 * @param n the xmlNode of JSON object pointer to the current element
 * @param conf_dir the directory where the main.cfg has been found
 * @param request_inputs the map pointer to the request KVP if any
 * @param funcError the function used to print the error back
 * @return 0 in case of success, 1 otherwise
 */
int fetchServicesForDescription(registry* zooRegistry, maps** pmsConf, char* r_inputs,
				void (func) (registry *, maps *, void*, void*, service *),
				void* doc, void* n, char *pcDir, map* request_inputs,
				void (funcError) (maps**, map*) ){
  maps* m=*pmsConf;
  map* pmHasSearchPath=getMapFromMaps(m,"main","search_path");
  // if services namespace is present in the map, conf_dir will
  // point to the namespace services path else it will point to
  // the default service path
  char* pcaConfDir=(char*) malloc(1024*sizeof(char));
  int res=0;
  getServicesNamespacePath(m,pcDir,pcaConfDir,1024);
  if(pmHasSearchPath!=NULL && strncasecmp(pmHasSearchPath->value,"true",4)==0){
    setMapInMaps(m,"lenv","can_continue","true");
    res=_fetchServicesForDescription(zooRegistry, &m, r_inputs, func, doc,
					 n, pcaConfDir, request_inputs, funcError);
    if(res!=0 && strncmp(pcaConfDir,pcDir,strlen(pcDir))!=0){
      setMapInMaps(m,"lenv","can_continue","false");
      res=_fetchServicesForDescription(zooRegistry, &m, r_inputs, func, doc,
				       n, pcDir, request_inputs, funcError);
    }
  }else{
    setMapInMaps(m,"lenv","can_continue","false");
    res=_fetchServicesForDescription(zooRegistry, &m, r_inputs, func, doc,
				     n, pcaConfDir, request_inputs, funcError);
  }
  free(pcaConfDir);
  return res;
}

#ifdef USE_JSON
/**
 * Run every HTTP request to download inputs passed as reference
 *
 * @param conf the maps pointing to the main.cfg file content
 * @param inputs the maps pointing to the inputs provided in the request
 */
int loadHttpRequests(maps* conf,maps* inputs){
  // Resolve reference
  // TODO: add error gesture
  // TODO: add nested processes support
  int eres;
  maps* tmpMaps=getMaps(conf,"http_requests");
  if(tmpMaps!=NULL){
    map* lenMap=getMap(tmpMaps->content,"length");
    int len=0;
    int iArrayIndex=0;
    char *pcaPreviousInputName=NULL;
    if(lenMap!=NULL){
      len=atoi(lenMap->value);
    }
    HINTERNET hInternet;
    HINTERNET res;
    if(len>0)
      hInternet = InternetOpen (
#ifndef WIN32
				(LPCTSTR)
#endif
				"ZOO-Project WPS Client\0",
				INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    for(int j=0;j<len;j++){
      map* tmpUrl=getMapArray(tmpMaps->content,"url",j);
      map* tmpInput=getMapArray(tmpMaps->content,"input",j);
      if(tmpInput!=NULL && tmpUrl!=NULL){
	maps* currentMaps=getMaps(inputs,tmpInput->value);
	if(pcaPreviousInputName==NULL)
	  pcaPreviousInputName=zStrdup(tmpInput->value);
	else if(strcmp(pcaPreviousInputName,tmpInput->value)!=0){
	  free(pcaPreviousInputName);
	  pcaPreviousInputName=zStrdup(tmpInput->value);
	  iArrayIndex=0;
	}
	loadRemoteFile(&conf,&currentMaps->content,&hInternet,tmpUrl->value);
	if(getMap(currentMaps->content,"length")!=NULL){
	  if(iArrayIndex==0){
	    addIntToMap(currentMaps->content,"Order",hInternet.nb);
	    addToMap(currentMaps->content,"Reference",tmpUrl->value);
	  }else{
	    addIntToMapArray(currentMaps->content,"Order",iArrayIndex,hInternet.nb);
	    setMapArray(currentMaps->content,"Reference",iArrayIndex,tmpUrl->value);
	  }
	  iArrayIndex++;
	}else{
	  addIntToMap(currentMaps->content,"Order",hInternet.nb);
	  addToMap(currentMaps->content,"Reference",tmpUrl->value);
	}
      }
    }
    if(len>0){
      map* error=NULL;
      runHttpRequests(&conf,&inputs,&hInternet,&error);
      InternetCloseHandle(&hInternet);
    }
    if(pcaPreviousInputName!=NULL)
      free(pcaPreviousInputName);
  }
  return 0;
}
#endif

/**
 * Initialize environment sections, load env, and populate lenv and renv.
 *
 * @param conf the maps pointing to the main.cfg file content
 * @param request_inputs the map pointing to the request KVP 
 * @param cPath a string pointing to the cwd
 * @param request a string pointing to the request key (xrequest or jrequest)
 */
void initAllEnvironment(maps* conf,map* request_inputs,
			const char* cPath,const char* request){
  // Define each env variable in runing environment
  maps *curs = getMaps (conf, "env");
  if (curs != NULL) {
    map *mapcs = curs->content;
    while (mapcs != NULLMAP)
      {
#ifndef WIN32
	setenv (mapcs->name, mapcs->value, 1);
#ifdef DEBUG
	fprintf (stderr, "[ZOO: setenv (%s=%s)]\n", mapcs->name,
		 mapcs->value);
#endif
#else
	if (mapcs->value[strlen (mapcs->value) - 2] == '\r')
	  {
#ifdef DEBUG
	    fprintf (stderr, "[ZOO: Env var finish with \r]\n");
#endif
	    mapcs->value[strlen (mapcs->value) - 1] = 0;
	  }
#ifdef DEBUG
	if (SetEnvironmentVariable (mapcs->name, mapcs->value) == 0)
	  {
	    fflush (stderr);
	    fprintf (stderr, "setting variable... %s\n", "OK");
	  }
	else
	  {
	    fflush (stderr);
	    fprintf (stderr, "setting variable... %s\n", "OK");
	  }
#else
	SetEnvironmentVariable (mapcs->name, mapcs->value);
#endif
	char *toto =
	  (char *)
	  malloc ((strlen (mapcs->name) + strlen (mapcs->value) +
		   2) * sizeof (char));
	sprintf (toto, "%s=%s", mapcs->name, mapcs->value);
	_putenv (toto);
#ifdef DEBUG
	fflush (stderr);
#endif
#endif

#ifdef DEBUG
	fprintf (stderr, "[ZOO: setenv (%s=%s)]\n", mapcs->name,
		 mapcs->value);
	fflush (stderr);
#endif
	mapcs = mapcs->next;
      }
  }

  int eres = SERVICE_STARTED;
  int cpid = zGetpid ();

  // Create a map containing a copy of the request map
  maps *_tmpMaps = createMaps("request");
  addMapToMap(&_tmpMaps->content,request_inputs);
  addMapsToMaps (&conf, _tmpMaps);
  freeMaps (&_tmpMaps);
  free (_tmpMaps);
  /**
   * Initialize the specific [lenv] section which contains runtime variables:
   * 
   *  - usid : it is an universally unique identifier  
   *  - osid : it is an idenfitication number 
   *  - sid : it is the process idenfitication number (OS)
   *  - uusid : it is an universally unique identifier 
   *  - status : value between 0 and 100 to express the  completude of 
   * the operations of the running service 
   *  - message : is a string where you can store error messages, in case 
   * service is failing, or o provide details on the ongoing operation.
   *  - cwd : the current working directory or servicePath if defined
   *  - soap : is a boolean value, true if the request was contained in a SOAP 
   * Envelop 
   *  - sessid : string storing the session identifier (only when cookie is 
   * used)
   *  - cgiSid : only defined on Window platforms (for being able to identify 
   * the created process)
   *
   */
  _tmpMaps = createMaps("lenv");
  char tmpBuff[100];
  struct ztimeval tp;
  if (zGettimeofday (&tp, NULL) == 0)
    sprintf (tmpBuff, "%i", (cpid + ((int) tp.tv_sec + (int) tp.tv_usec)));
  else
    sprintf (tmpBuff, "%i", (cpid + (int) time (NULL)));
  _tmpMaps->content = createMap ("osid", tmpBuff);
  sprintf (tmpBuff, "%i", cpid);
  addToMap (_tmpMaps->content, "sid", tmpBuff);
  char* tmpUuid=get_uuid();
  addToMap (_tmpMaps->content, "uusid", tmpUuid);
  addToMap (_tmpMaps->content, "usid", tmpUuid);
  free(tmpUuid);
  addToMap (_tmpMaps->content, "status", "0");
  map* cwdMap0=getMapFromMaps(conf,"main","servicePath");
  if(cwdMap0!=NULL){
    addToMap (_tmpMaps->content, "cwd", cwdMap0->value);
    addToMap (_tmpMaps->content, "rcwd", cPath);
  }
  else
    addToMap (_tmpMaps->content, "cwd", cPath);
  addToMap (_tmpMaps->content, "message", _("No message provided"));
  map *ltmp = getMap (request_inputs, "soap");
  if (ltmp != NULL)
    addToMap (_tmpMaps->content, "soap", ltmp->value);
  else
    addToMap (_tmpMaps->content, "soap", "false");

  // Parse the session file and add it to the main maps
  char* originalCookie=NULL;
  if (cgiCookie != NULL && strlen (cgiCookie) > 0)
    {
      int hasValidCookie = -1;
      char *tcook = originalCookie = zStrdup (cgiCookie);
      map *testing = getMapFromMaps (conf, "main", "cookiePrefix");
      parseCookie(&conf,originalCookie);
      map *sessId=getMapFromMaps(conf,"cookies",
				 (testing==NULL?"ID":testing->value));
      if (sessId!=NULL)
	{
	  addToMap (_tmpMaps->content, "sessid", sessId->value);
	  char session_file_path[1024];
	  map *tmpPath = getMapFromMaps (conf, "main", "sessPath");
	  if (tmpPath == NULL)
	    tmpPath = getMapFromMaps (conf, "main", "tmpPath");
	  char *tmp1 = strtok (tcook, ";");
	  if (tmp1 != NULL)
	    sprintf (session_file_path, "%s/sess_%s.cfg", tmpPath->value,
		     sessId->value);
	  else
	    sprintf (session_file_path, "%s/sess_%s.cfg", tmpPath->value,
		     sessId->value);
	  free (tcook);
	  maps *tmpSess = (maps *) malloc (MAPS_SIZE);
	  tmpSess->content = NULL;
	  tmpSess->child = NULL;
	  tmpSess->next = NULL;
	  struct stat file_status;
	  int istat = stat (session_file_path, &file_status);
	  if (istat == 0 && file_status.st_size > 0)
	    {
	      int saved_stdout = zDup (fileno (stdout));
	      zDup2 (fileno (stderr), fileno (stdout));
	      conf_read (session_file_path, tmpSess);
	      addMapsToMaps (&conf, tmpSess);
	      freeMaps (&tmpSess);
	      fflush(stdout);
	      zDup2 (saved_stdout, fileno (stdout));
	      zClose(saved_stdout);
	    }
	  free (tmpSess);
	}
    }
  addMapsToMaps (&conf, _tmpMaps);
  freeMaps (&_tmpMaps);
  free (_tmpMaps);

  maps* bmap=NULL;
#ifdef DEBUG
  dumpMap (request_inputs);
#endif
  int ei = 1;
  
  _tmpMaps = createMaps("renv");

#ifdef WIN32
  LPVOID orig = GetEnvironmentStrings();
  LPTSTR s = (LPTSTR) orig;
  
  while (*s != NULL) {
    char* env = strdup(s);
    char* delim = strchr(env,'=');
    if (delim != NULL) {
      char* val = delim+1;
      *delim = '\0';		
      if(_tmpMaps->content == NULL) {
        _tmpMaps->content = createMap(env,val);
      }
      else {
        addToMap(_tmpMaps->content,env,val);
      }			
    }
    s += strlen(s)+1;
  } 
  FreeEnvironmentStrings((LPCH)orig);	
#else
  char **orig = environ;
  char *s=*orig;
  
  if(orig!=NULL)
    for (; s; ei++ ) {
      if(strstr(s,"=")!=NULL && strlen(strstr(s,"="))>1){
	int len=strlen(s);
	char* tmpName=zStrdup(s);
	char* tmpValue=strstr(s,"=")+1;
	char* tmpName1=(char*)malloc((1+(len-(strlen(tmpValue)+1)))*sizeof(char));
	snprintf(tmpName1,(len-strlen(tmpValue)),"%s",tmpName);
	if(_tmpMaps->content == NULL)
	  _tmpMaps->content = createMap (tmpName1,tmpValue);
	else
	  addToMap (_tmpMaps->content,tmpName1,tmpValue);
	free(tmpName1);
	free(tmpName);
      }
      s = *(orig+ei);
    }  
#endif
  
  if(_tmpMaps->content!=NULL && getMap(_tmpMaps->content,"HTTP_COOKIE")!=NULL){
    addToMap(_tmpMaps->content,"HTTP_COOKIE1",&cgiCookie[0]);
  }
  if(strlen(cgiAuthType)>0){
    addToMap(_tmpMaps->content,"HTTP_AUTHORIZATION",cgiAuthType);
  }
  addMapsToMaps (&conf, _tmpMaps);
  freeMaps (&_tmpMaps);
  free (_tmpMaps);
  map* postRequest=getMap(request_inputs,request);
  if(postRequest!=NULL)
    setMapInMaps (conf, "renv", request, postRequest->value);
#ifdef WIN32
  char *cgiSidL = NULL;
  if (getenv ("CGISID") != NULL)
    addToMap (request_inputs, "cgiSid", getenv ("CGISID"));

  char* usidp;
  if ( (usidp = getenv("USID")) != NULL ) {
    setMapInMaps (conf, "lenv", "usid", usidp);
  }

  map *test1 = getMap (request_inputs, "cgiSid");
  if (test1 != NULL){
    cgiSid = zStrdup(test1->value);
    addToMap (request_inputs, "storeExecuteResponse", "true");
    addToMap (request_inputs, "status", "true");
    setMapInMaps (conf, "lenv", "osid", test1->value);
    //status = getMap (request_inputs, "status");
  }
  test1 = getMap (request_inputs, "usid");
  if (test1 != NULL){
    setMapInMaps (conf, "lenv", "usid", test1->value);
    setMapInMaps (conf, "lenv", "uusid", test1->value);
  }
#endif

}

/**
 * Signal handling function which simply call exit(0).
 *
 * @param sig the signal number
 */
void
donothing (int sig)
{
#ifdef DEBUG
  fprintf (stderr, "Signal %d after the ZOO-Kernel returned result!\n", sig);
#endif
  exit (0);
}

/**
 * Signal handling function which create an ExceptionReport node containing the
 * information message corresponding to the signal number.
 *
 * @param sig the signal number
 */
void
sig_handler (int sig)
{
  
  char tmp[100];
  const char *ssig;
  switch (sig)
    {
    case SIGSEGV:
      ssig = "SIGSEGV";
      break;
    case SIGTERM:
      ssig = "SIGTERM";
      break;
    case SIGINT:
      ssig = "SIGINT";
      break;
    case SIGILL:
      ssig = "SIGILL";
      break;
    case SIGFPE:
      ssig = "SIGFPE";
      break;
    case SIGABRT:
      ssig = "SIGABRT";
      break;
    default:
      ssig = "UNKNOWN";
      break;
    }
  sprintf (tmp,
           _
           ("ZOO Kernel failed to process your request, receiving signal %d = %s "),
           sig, ssig);
  errorException (NULL, tmp, "InternalError", NULL);
#ifdef DEBUG
  fprintf (stderr, "Not this time!\n");
#endif
  exit (0);
}

#ifdef USE_JSON
  /**
   * Signal handling function which create an ExceptionReport node containing the
   * information message corresponding to the signal number.
   *
   * @param sig the signal number
   */
void json_sig_handler (int sig){
  char tmp[100];
  const char *ssig;
  switch (sig)
    {
    case SIGSEGV:
      ssig = "SIGSEGV";
      break;
    case SIGTERM:
      ssig = "SIGTERM";
      break;
    case SIGINT:
      ssig = "SIGINT";
      break;
    case SIGILL:
      ssig = "SIGILL";
      break;
    case SIGFPE:
      ssig = "SIGFPE";
      break;
    case SIGABRT:
      ssig = "SIGABRT";
      break;
    default:
      ssig = "UNKNOWN";
      break;
    }
  sprintf (tmp,
	   _
	   ("ZOO Kernel failed to process your request, receiving signal %d = %s "),
	   sig, ssig);
  map* tmpMap=createMap("decode","suze");
  maps* tmpMaps=createMaps("lenv");
  setMapInMaps(tmpMaps,"lenv","message",tmp);
  setMapInMaps(tmpMaps,"lenv","status","failed");
  printExceptionReportResponseJ(&tmpMaps,tmpMap);
  //errorException (NULL, tmp, "InternalError", NULL);
#ifdef DEBUG
  fprintf (stderr, "Not this time!\n");
#endif
  exit (0);
}
  
#endif

/**
 * Register signals
 *
 * @param a function for handling signals
 * @see donothing
 */
void register_signals(void (func)(int)){
#ifndef USE_GDB
#ifndef WIN32
  signal (SIGCHLD, SIG_IGN);
#endif
  signal (SIGSEGV, func);
  signal (SIGTERM, func);
  signal (SIGINT, func);
  signal (SIGILL, func);
  signal (SIGFPE, func);
  signal (SIGABRT, func);
#endif
}

/**
 * Load a service provider and run the service function.
 *
 * @param myMap the conf maps containing the main.cfg settings
 * @param s1 the service structure
 * @param request_inputs map storing all the request parameters
 * @param inputs the inputs maps
 * @param ioutputs the outputs maps
 * @param eres the result returned by the service execution
 */
void
loadServiceAndRun (maps ** myMap, service * s1, map * request_inputs,
                   maps ** inputs, maps ** ioutputs, int *eres)
{
  char ntmp[1024];
  maps *m = *myMap;
  maps *request_output_real_format = *ioutputs;
  maps *request_input_real_format = *inputs;
  /**
   * Extract serviceType to know what kind of service should be loaded
   */
  char *serviceNamespacePath=NULL;
  if(request_output_real_format!=NULL){
    //memset(serviceNamespacePath,'\0',1024);
    map* zooServicesNamespaceMap= getMapFromMaps(m, "zooServicesNamespace", "namespace");
    map* zooServicesNamespacePathMap=getMapFromMaps(m,"servicesNamespace","path");

    if( zooServicesNamespaceMap && strlen(zooServicesNamespaceMap->value)>0 && zooServicesNamespacePathMap && strlen(zooServicesNamespacePathMap->value)>0){
      serviceNamespacePath=(char*)malloc(1024*sizeof(char));
      sprintf(serviceNamespacePath,"%s/%s",zooServicesNamespacePathMap->value,zooServicesNamespaceMap->value);
      setMapInMaps(m, "lenv","cwd", serviceNamespacePath);
    }
  }
  map *r_inputs = NULL;
  map* cwdMap=getMapFromMaps(m,"main","servicePath");
  if(cwdMap!=NULL){
    sprintf(ntmp,"%s",cwdMap->value);
  }else{
#ifndef WIN32
    getcwd (ntmp, 1024);
#else
    _getcwd (ntmp, 1024);
#endif
  }
  r_inputs = getMap (s1->content, "serviceType");
#ifdef DEBUG
  fprintf (stderr, "LOAD A %s SERVICE PROVIDER \n", r_inputs->value);
  fflush (stderr);
#endif

  map* libp = getMapFromMaps(m, "main", "libPath");

  if (r_inputs!=NULL && strlen (r_inputs->value) == 1
      && strncasecmp (r_inputs->value, "C", 1) == 0)
  {
    char *tmps1=(char*)malloc(1024*sizeof(char));
      if (libp != NULL && libp->value != NULL) {
          r_inputs = getMap (s1->content, "ServiceProvider");
          sprintf (tmps1, "%s/%s", libp->value, r_inputs->value);
#ifdef DEBUG
          fprintf (stderr, "libpath1 + service provider : %s/%s", libp->value, r_inputs->value);
#endif
      }
      else {
          r_inputs = getMap (request_inputs, "metapath");
          if (r_inputs != NULL)
              sprintf (tmps1, "%s/%s", ntmp, r_inputs->value);
          else
              sprintf (tmps1, "%s/", ntmp);

          char *altPath = zStrdup ( (serviceNamespacePath!=NULL)?serviceNamespacePath:tmps1);

          r_inputs = getMap (s1->content, "ServiceProvider");
          sprintf (tmps1, "%s/%s", altPath, r_inputs->value);
          free (altPath);
      }
#ifdef DEBUG
      fprintf (stderr, "Trying to load %s\n", tmps1);
#endif
#ifdef WIN32
      HINSTANCE so =
        LoadLibraryEx (tmps1, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
      void *so = dlopen (tmps1, RTLD_LAZY);
#endif
#ifdef WIN32
      char* errstr = getLastErrorMessage();
#else
      char *errstr;
      errstr = dlerror ();
#endif
#ifdef DEBUG
	  fprintf (stderr, "%s loaded (%s) \n", tmps1, errstr);
#endif
	  free(tmps1);
      if (so != NULL)
        {
#ifdef DEBUG
          fprintf (stderr, "Library loaded %s \n", errstr);
          fprintf (stderr, "Service Shared Object = %s\n", r_inputs->value);
#endif
          r_inputs = getMap (s1->content, "serviceType");
#ifdef DEBUG
          dumpMap (r_inputs);
          fprintf (stderr, "%s\n", r_inputs->value);
          fflush (stderr);
#endif
          if (strncasecmp (r_inputs->value, "C-FORTRAN", 9) == 0)
            {
              r_inputs = getMap (request_inputs, "Identifier");
              char fname[1024];
              sprintf (fname, "%s_", r_inputs->value);
#ifdef DEBUG
              fprintf (stderr, "Try to load function %s\n", fname);
#endif
#ifdef WIN32
              typedef int (CALLBACK * execute_t) (char ***, char ***,
                                                  char ***);
              execute_t execute = (execute_t) GetProcAddress (so, fname);
#else
              typedef int (*execute_t) (char ***, char ***, char ***);
              execute_t execute = (execute_t) dlsym (so, fname);
#endif
#ifdef DEBUG
#ifdef WIN32
			  errstr = getLastErrorMessage();
#else
              errstr = dlerror ();
#endif
              fprintf (stderr, "Function loaded %s\n", errstr);
#endif

              char main_conf[10][30][1024];
              char inputs[10][30][1024];
              char outputs[10][30][1024];
              for (int i = 0; i < 10; i++)
                {
                  for (int j = 0; j < 30; j++)
                    {
                      memset (main_conf[i][j], 0, 1024);
                      memset (inputs[i][j], 0, 1024);
                      memset (outputs[i][j], 0, 1024);
                    }
                }
              mapsToCharXXX (m, (char ***) main_conf);
              mapsToCharXXX (request_input_real_format, (char ***) inputs);
              mapsToCharXXX (request_output_real_format, (char ***) outputs);
              *eres =
                execute ((char ***) &main_conf[0], (char ***) &inputs[0],
                         (char ***) &outputs[0]);
#ifdef DEBUG
              fprintf (stderr, "Function run successfully \n");
#endif
              charxxxToMaps ((char ***) &outputs[0],
                             &request_output_real_format);
            }
          else
            {
#ifdef DEBUG
#ifdef WIN32
	      errstr = getLastErrorMessage();
              fprintf (stderr, "Function %s failed to load because of %s\n",
                       r_inputs->value, errstr);
#endif
#endif
              r_inputs = getMapFromMaps (m, "lenv", "Identifier");
#ifdef DEBUG
              fprintf (stderr, "Try to load function %s\n", r_inputs->value);
#endif
              typedef int (*execute_t) (maps **, maps **, maps **);
#ifdef WIN32
              execute_t execute =
                (execute_t) GetProcAddress (so, r_inputs->value);
#else
              execute_t execute = (execute_t) dlsym (so, r_inputs->value);
#endif

              if (execute == NULL)
                {
#ifdef WIN32
		  errstr = getLastErrorMessage();
#else
                  errstr = dlerror ();
#endif
                  char *tmpMsg =
                    (char *) malloc (2048 + strlen (r_inputs->value));
                  sprintf (tmpMsg,
                           _
                           ("Error occurred while running the %s function: %s"),
                           r_inputs->value, errstr);
                  errorException (&m, tmpMsg, "InternalError", NULL);
                  free (tmpMsg);
#ifdef DEBUG
                  fprintf (stderr, "Function %s error %s\n", r_inputs->value,
                           errstr);
#endif
                  *eres = -1;
                  return;
                }

#ifdef DEBUG
#ifdef WIN32
	      errstr = getLastErrorMessage();
#else
              errstr = dlerror ();
#endif
              fprintf (stderr, "Function loaded %s\n", errstr);
#endif

#ifdef DEBUG
              fprintf (stderr, "Now run the function \n");
              fflush (stderr);
#endif
              *eres =
                execute (&m, &request_input_real_format,
                         &request_output_real_format);
#ifdef DEBUG
              fprintf (stderr, "Function loaded and returned %d\n", eres);
              fflush (stderr);
#endif
            }
#ifdef WIN32
          *ioutputs = dupMaps (&request_output_real_format);
          FreeLibrary (so);
#else
          dlclose (so);
#endif
        }
      else
        {
      /**
       * Unable to load the specified shared library
       */
          char tmps[1024];
#ifdef WIN32
	  errstr = getLastErrorMessage();
#else
	  errstr = dlerror ();
#endif
          sprintf (tmps, _("Unable to load C Library %s"), errstr);
	  errorException(&m,tmps,"InternalError",NULL);
          *eres = -1;
        }
    }
  else

#ifdef USE_HPC
  if (strncasecmp (r_inputs->value, "HPC", 3) == 0)
    {
      *eres =
        zoo_hpc_support (&m, request_inputs, s1,
                            &request_input_real_format,
                            &request_output_real_format);
    }
  else
#endif

#ifdef USE_SAGA
  if (strncasecmp (r_inputs->value, "SAGA", 4) == 0)
    {
      *eres =
        zoo_saga_support (&m, request_inputs, s1,
                            &request_input_real_format,
                            &request_output_real_format);
    }
  else
#endif

#ifdef USE_OTB
  if (strncasecmp (r_inputs->value, "OTB", 3) == 0)
    {
      *eres =
        zoo_otb_support (&m, request_inputs, s1,
                            &request_input_real_format,
                            &request_output_real_format);
    }
  else
#endif
#ifdef USE_PYTHON
  if (strncasecmp (r_inputs->value, "PYTHON", 6) == 0)
    {
      *eres =
        zoo_python_support (&m, request_inputs, s1,
                            &request_input_real_format,
                            &request_output_real_format);	  
    }
  else
#endif

#ifdef USE_R
  if (strncasecmp (r_inputs->value, "R", 6) == 0)
    {	  
      *eres =
        zoo_r_support (&m, request_inputs, s1,
                            &request_input_real_format,
                            &request_output_real_format);
    }
  else
#endif

#ifdef USE_JAVA
  if (strncasecmp (r_inputs->value, "JAVA", 4) == 0)
    {
      *eres =
        zoo_java_support (&m, request_inputs, s1, &request_input_real_format,
                          &request_output_real_format);
    }
  else
#endif

#ifdef USE_PHP
  if (strncasecmp (r_inputs->value, "PHP", 3) == 0)
    {
      *eres =
        zoo_php_support (&m, request_inputs, s1, &request_input_real_format,
                         &request_output_real_format);		
    }
  else
#endif


#ifdef USE_PERL
  if (strncasecmp (r_inputs->value, "PERL", 4) == 0)
    {
      *eres =
        zoo_perl_support (&m, request_inputs, s1, &request_input_real_format,
                          &request_output_real_format);
    }
  else
#endif

#ifdef USE_JS
  if (strncasecmp (r_inputs->value, "JS", 2) == 0)
    {
      *eres =
        zoo_js_support (&m, request_inputs, s1, &request_input_real_format,
                        &request_output_real_format);
    }
  else
#endif

#ifdef USE_NODEJS
  if (strncasecmp (r_inputs->value, "NODEJS", 2) == 0)
    {
      *eres =
        zoo_nodejs_support (&m, request_inputs, s1, &request_input_real_format,
                        &request_output_real_format);
    }
  else
#endif

#ifdef USE_RUBY
  if (strncasecmp (r_inputs->value, "Ruby", 4) == 0)
    {
      *eres =
        zoo_ruby_support (&m, request_inputs, s1, &request_input_real_format,
                          &request_output_real_format);
    }
  else
#endif

#ifdef USE_MONO
  if (strncasecmp (r_inputs->value, "Mono", 4) == 0)
    {
      *eres =
        zoo_mono_support (&m, request_inputs, s1, &request_input_real_format,
                          &request_output_real_format);
    }
  else
#endif

    {
      char tmpv[1024];
      sprintf (tmpv,
               _
               ("Programming Language (%s) set in ZCFG file is not currently supported by ZOO Kernel.\n"),
               r_inputs->value);
      errorException (&m, tmpv, "InternalError", NULL);
      *eres = -1;
    }
  *myMap = m;
  if(request_output_real_format!=NULL)
    *ioutputs = request_output_real_format;
}


/**
 * Set the security flag for the current request.
 * Set [lenv] secured_url to true in case the access should be secured,
 * false in other cases.
 *
 * @param pmsConf the maps pointing to the main.cfg file content
 * @param pcCgiQueryString the string containing the request
 */
void setSecurityFlags(maps* pmsConf,char* pcCgiQueryString){
  maps* pmsCurrentPath=getMaps(pmsConf,(strlen(pcCgiQueryString)>1?(pcCgiQueryString+1):"root"));
  int isSet=-1;
  if(pmsCurrentPath==NULL && strstr(pcCgiQueryString,"execution")!=NULL)
    pmsCurrentPath=getMaps(pmsConf,"processes/{processId}/execution");
  else if(pmsCurrentPath==NULL && strstr(pcCgiQueryString,"processes/")!=NULL)
    pmsCurrentPath=getMaps(pmsConf,"processes/{processId}");
  else if(pmsCurrentPath==NULL && strstr(pcCgiQueryString,"results")!=NULL)
    pmsCurrentPath=getMaps(pmsConf,"jobs/{jobID}/results");
  else if(pmsCurrentPath==NULL && strstr(pcCgiQueryString,"jobs/")!=NULL)
    pmsCurrentPath=getMaps(pmsConf,"jobs/{jobID}");
  if(pmsCurrentPath!=NULL){
    map* pmLength=getMap(pmsCurrentPath->content,"length");
    int iLength=1;
    int iCnt=0;
    if(pmLength!=NULL)
      iLength=atoi(pmLength->value);
    for(;iCnt<iLength;iCnt++){
      map* pmCurrentElement=getMapArray(pmsCurrentPath->content,"method",iCnt);
      if(pmCurrentElement!=NULL && strcasecmp(pmCurrentElement->value,cgiRequestMethod)==0){
	map* pmSecured=getMapArray(pmsCurrentPath->content,"secured",iCnt);
	if(pmSecured!=NULL)
	  setMapInMaps(pmsConf,"lenv","secured_url","true");
	else
	  setMapInMaps(pmsConf,"lenv","secured_url","false");
	return;
      }
    }
  }
  if(isSet==-1)
    setMapInMaps(pmsConf,"lenv","secured_url","false");
}

#define localPrintExceptionJ localPrintException

/**
 * Invoke the execution of the security module in case security is activated
 *
 * @param pmsConf the maps pointing to the main.cfg file content
 * @param pcType the string defining the process to execute ('in' or 'out')
 * @return 0 in case of success, 1 in case or error
 */
int ensureFiltered(maps** pmsConf,const char* pcType){
  int eres=0;
  service* psaService=NULL;
  maps* pmsOsecurity=getMaps(*pmsConf,"osecurity");
  map* pmPath=NULL;
  map* pmName=NULL;
  map* pmRun=NULL;
  int iLen=1;
  char* pcaName=(char*)malloc((strlen(pcType)+8)*sizeof(char));
  sprintf(pcaName,"filter_%s",pcType);
  maps* pmsSection=getMaps(*pmsConf,pcaName);
  if(pmsSection==NULL){
    free(pcaName);
    return 0;
  }
  map* pmLength=getMap(pmsSection->content,"length");
  if(pmLength!=NULL)
    iLen=atoi(pmLength->value);
  if(strcasecmp(pcType,"out")==0){
    fflush(stdout);
    fflush(stderr);
  }
  for(int iCnt=0;iCnt<iLen;iCnt++){
    if(pmsSection!=NULL &&
       ((pmRun=getMapFromMaps(*pmsConf,"lenv",pcaName))==NULL) &&
       ((pmPath=getMapArray(pmsSection->content,"path",iCnt))!=NULL) &&
       ((pmName=getMapArray(pmsSection->content,"service",iCnt))!=NULL)){
      if(fetchService(NULL,*pmsConf,&psaService,NULL,pmPath->value,pmName->value,printExceptionReportResponseJ)!=0){
	fprintf(stderr,"ERROR fetching the service %s %d \n",__FILE__,__LINE__);
	free(pcaName);
	return 1;
      }
      maps* pmsInputs=NULL;
      maps* pmsOutputs=NULL;
      loadServiceAndRun (pmsConf, psaService, NULL,
			 &pmsInputs,
			 &pmsOutputs, &eres);
      freeService(&psaService);
      free(psaService);
      psaService=NULL;
      // Make sure to refresh pmsSection from the pmsConf updated from
      // the previous loadServiceAndRun invocation
      pmsSection=getMaps(*pmsConf,pcaName);
      if(eres!=SERVICE_SUCCEEDED){
	setMapInMaps(*pmsConf,"lenv",pcaName,"true");
	free(pcaName);
	return 1;
      }
    }
  }
  setMapInMaps(*pmsConf,"lenv",pcaName,"true");
  free(pcaName);
  return 0;
}



#ifdef WIN32
/**
 * createProcess function: create a new process after setting some env variables
 */
void
createProcess (maps * m, map * request_inputs, service * s1, char *opts,
               int cpid, maps * inputs, maps * outputs)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory (&si, sizeof (si));
  si.cb = sizeof (si);
  ZeroMemory (&pi, sizeof (pi));
  char *tmp = (char *) malloc ((1024 + cgiContentLength) * sizeof (char));
  char *tmpq = (char *) malloc ((1024 + cgiContentLength) * sizeof (char));
  map *req = getMap (request_inputs, "request");
  map *id = getMap (request_inputs, "identifier");
  map *di = getMap (request_inputs, "DataInputs");

  // The required size for the dataInputsKVP and dataOutputsKVP buffers
  // may exceed cgiContentLength, hence a 2 kb extension. However, a 
  // better solution would be to have getMapsAsKVP() determine the required
  // buffer size before allocating memory.	
  char *dataInputsKVP = getMapsAsKVP (inputs, cgiContentLength + 2048, 0);
  char *dataOutputsKVP = getMapsAsKVP (outputs, cgiContentLength + 2048, 1);
#ifdef DEBUG
  fprintf (stderr, "DATAINPUTSKVP %s\n", dataInputsKVP);
  fprintf (stderr, "DATAOUTPUTSKVP %s\n", dataOutputsKVP);
#endif
  map *sid = getMapFromMaps (m, "lenv", "osid");
  map *usid = getMapFromMaps (m, "lenv", "usid");
  map *r_inputs = getMapFromMaps (m, "main", "tmpPath");
  map *r_inputs1 = getMap (request_inputs, "metapath");
  
  int hasIn = -1;
  if (r_inputs1 == NULL)
    {
      r_inputs1 = createMap ("metapath", "");
      hasIn = 1;
    }
  map *r_inputs2 = getMap (request_inputs, "ResponseDocument");
  if (r_inputs2 == NULL)
    r_inputs2 = getMap (request_inputs, "RawDataOutput");
  map *tmpPath = getMapFromMaps (m, "lenv", "cwd");

  map *tmpReq = getMap (request_inputs, "xrequest");

  if(r_inputs2 != NULL && tmpReq != NULL) {
    const char key[] = "rfile=";
    char* kvp = (char*) malloc((FILENAME_MAX + strlen(key))*sizeof(char));
    char* filepath = kvp + strlen(key);
    strncpy(kvp, key, strlen(key));
    addToCache(m, tmpReq->value, tmpReq->value, "text/xml", strlen(tmpReq->value),
	       filepath, FILENAME_MAX);
    if (filepath == NULL) {
      errorException( &m, _("Unable to cache HTTP POST Execute request."), "InternalError", NULL);
      return;
    }
    sprintf(tmp,"\"metapath=%s&%s&cgiSid=%s&usid=%s\"",
	    r_inputs1->value,kvp,sid->value,usid->value);
    sprintf(tmpq,"metapath=%s&%s",
	    r_inputs1->value,kvp);
    free(kvp);
  }
  else if (r_inputs2 != NULL)
    {
      sprintf (tmp,
               "\"metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s&%s=%s&cgiSid=%s&usid=%s\"",
               r_inputs1->value, req->value, id->value, dataInputsKVP,
               r_inputs2->name, dataOutputsKVP, sid->value, usid->value);
      sprintf (tmpq,
               "metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s&%s=%s",
               r_inputs1->value, req->value, id->value, dataInputsKVP,
               r_inputs2->name, dataOutputsKVP);
    }
  else
    {
      sprintf (tmp,
               "\"metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s&cgiSid=%s&usid=%s\"",
               r_inputs1->value, req->value, id->value, dataInputsKVP,
               sid->value, usid->value);
      sprintf (tmpq,
               "metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s",
               r_inputs1->value, req->value, id->value, dataInputsKVP,
               sid->value);
    }

  if (hasIn > 0)
    {
      freeMap (&r_inputs1);
      free (r_inputs1);
    }
  char *tmp1 = zStrdup (tmp);
  sprintf (tmp, "\"%s\" %s \"%s\"", PROGRAMNAME, tmp1, sid->value);
  free (dataInputsKVP);
  free (dataOutputsKVP);
#ifdef DEBUG
  fprintf (stderr, "REQUEST IS : %s \n", tmp);
#endif

  usid = getMapFromMaps (m, "lenv", "usid");
  if (usid != NULL && usid->value != NULL) {
    SetEnvironmentVariable("USID", TEXT (usid->value));
  }
  SetEnvironmentVariable ("CGISID", TEXT (sid->value));
  SetEnvironmentVariable ("QUERY_STRING", TEXT (tmpq));
  // knut: Prevent REQUEST_METHOD=POST in background process call to cgic:main
  // (process hangs when reading cgiIn):
  SetEnvironmentVariable("REQUEST_METHOD", "GET");
  SetEnvironmentVariable("CONTENT_TYPE", "text/plain");
  
  char clen[1000];
  sprintf (clen, "%d", strlen (tmpq));
  SetEnvironmentVariable ("CONTENT_LENGTH", TEXT (clen));

  // ref. https://msdn.microsoft.com/en-us/library/windows/desktop/ms684863%28v=vs.85%29.aspx
  if (!CreateProcess (NULL,             // No module name (use command line)
                      TEXT (tmp),       // Command line
                      NULL,             // Process handle not inheritable
                      NULL,             // Thread handle not inheritable
                      FALSE,            // Set handle inheritance to FALSE
                      CREATE_NO_WINDOW, // Apache won't wait until the end
                      NULL,             // Use parent's environment block
                      NULL,             // Use parent's starting directory
                      &si,              // Pointer to STARTUPINFO struct
                      &pi)              // Pointer to PROCESS_INFORMATION struct
    )
    {
#ifdef DEBUG
      fprintf (stderr, "CreateProcess failed (%d).\n", GetLastError ());
#endif
      if (tmp != NULL) {
        free(tmp);
      }
      if (tmpq != NULL) {
        free(tmpq);
      }
      return;
    }
  else
    {
#ifdef DEBUG
      fprintf (stderr, "CreateProcess successful (%d).\n\n\n\n",
               GetLastError ());
#endif
    }
  CloseHandle (pi.hProcess);
  CloseHandle (pi.hThread);

  if (tmp != NULL) {
    free(tmp);
  }
  if (tmpq != NULL) {
    free(tmpq);
  }

#ifdef DEBUG
  fprintf (stderr, "CreateProcess finished !\n");
#endif
}
#endif


/**
 * Process the request.
 *
 * @param inputs the request parameters map
 * @return 0 on sucess, other value on failure
 * @see conf_read,recursReaddirF
 */
int
runRequest (map ** inputs)
{
  register_signals(sig_handler);

  
  map *r_inputs = NULL;
  map *request_inputs = *inputs;
#ifdef IGNORE_METAPATH
  addToMap(request_inputs, "metapath", "");
#endif  
  maps *m = NULL;
  char *REQUEST = NULL;
  /**
   * Parsing service specfic configuration file
   */
  m = (maps *) malloc (MAPS_SIZE);
  if (m == NULL)
    {
      return errorException (NULL, _("Unable to allocate memory"),
                             "InternalError", NULL);
    }
  m->content = NULL;
  m->child = NULL;
  m->next = NULL;
  char ntmp[1024];
#ifndef ETC_DIR
#ifndef WIN32
  getcwd (ntmp, 1024);
#else
  _getcwd (ntmp, 1024);
#endif
#else
  sprintf(ntmp,"%s",ETC_DIR);
#endif
  r_inputs = getMapOrFill (&request_inputs, "metapath", "");

  char conf_file[10240];
  snprintf (conf_file, 10240, "%s/%s/main.cfg", ntmp, r_inputs->value);
  if (conf_read (conf_file, m) == 2)
    {
      errorException (NULL, _("Unable to load the main.cfg file."),
                      "InternalError", NULL);
      free (m);
      return 1;
    }
#ifdef DEBUG
  fprintf (stderr, "***** BEGIN MAPS\n");
  dumpMaps (m);
  fprintf (stderr, "***** END MAPS\n");
#endif


  maps* zooServicesNamespaceMap=getMaps(m,"zooServicesNamespace");

  map *getPath = getMapFromMaps (m, "main", "gettextPath");
  if (getPath != NULL)
    {
      bindtextdomain ("zoo-kernel", getPath->value);
      bindtextdomain ("zoo-services", getPath->value);
    }
  else
    {
      bindtextdomain ("zoo-kernel", LOCALEDIR);
      bindtextdomain ("zoo-services", LOCALEDIR);
    }

  /**
   * Manage our own error log file (usefull to separate standard apache debug
   * messages from the ZOO-Kernel ones but also for IIS users to avoid wrong 
   * headers messages returned by the CGI due to wrong redirection of stderr)
   *
   * This default configuration can be changed during the compilation of ZOO-kernel
   * by passing --with-log-console=yes to force logs of service executions on stderr
   */
  FILE *fstde = NULL;
  map *fstdem = getMapFromMaps (m, "main", "logPath");
  if (fstdem != NULL)
    fstde = freopen (fstdem->value, "a+", stderr);

  /**
   * Language gesture
   */
  r_inputs = getMap (request_inputs, "language");
  if (r_inputs == NULL)
    r_inputs = getMap (request_inputs, "AcceptLanguages");
  if (r_inputs == NULL){
    // HTTP_ACCEPT_LANGUAGE
    char* pcAcceptLanguages=getenv("HTTP_ACCEPT_LANGUAGE");
    if(pcAcceptLanguages!=NULL){
      char* saveptr;
      char *tmps = strtok_r (pcAcceptLanguages, ",", &saveptr);
      while(tmps!=NULL){
	if(strstr(tmps,";")==NULL && isValidLang (m, tmps) >=0 ){
	  setMapInMaps(m, "main", "language",tmps);
	  break;
	}
	else{
	  char* saveptr1;
	  char *tmps1 = strtok_r (tmps, ";", &saveptr1);
	  int hasValue=-1;
	  while(tmps1!=NULL && isValidLang (m, tmps)  >=0 ){
	    setMapInMaps(m, "main", "language",tmps1);
	    hasValue=1;
	    break;
	    tmps1 = strtok_r (NULL, ";", &saveptr1);
	  }
	  if(hasValue>0)
	    break;
	}
	tmps = strtok_r (NULL, ",", &saveptr);
      }
    }
  }
  if (r_inputs == NULL)
    r_inputs = getMapFromMaps (m, "main", "language");
  if (r_inputs != NULL)
    {
      if (isValidLang (m, r_inputs->value) < 0)
        {
          char tmp[1024];
          sprintf (tmp,
                   _
                   ("The value %s is not supported for the <language> parameter"),
                   r_inputs->value);
          errorException (&m, tmp, "InvalidParameterValue", "language");
          freeMaps (&m);
          free (m);
          free (REQUEST);
          return 1;

        }
      char *tmp = zStrdup (r_inputs->value);
      setMapInMaps (m, "main", "language", tmp);
#ifdef DEB
      char tmp2[12];
      sprintf (tmp2, "%s.utf-8", tmp);
      translateChar (tmp2, '-', '_');
      setlocale (LC_ALL, tmp2);
#else
      translateChar (tmp, '-', '_');
      setlocale (LC_ALL, tmp);
#endif
#ifndef WIN32
      setenv ("LC_ALL", tmp, 1);
#else
      char tmp1[13];
      sprintf (tmp1, "LC_ALL=%s", tmp);
      _putenv (tmp1);
#endif
      free (tmp);
    }
  else
    {
      setlocale (LC_ALL, "en_US");
#ifndef WIN32
      setenv ("LC_ALL", "en_US", 1);
#else
      char tmp1[13];
      sprintf (tmp1, "LC_ALL=en_US");
      _putenv (tmp1);
#endif
      setMapInMaps (m, "main", "language", "en-US");
    }
  setlocale (LC_NUMERIC, "C");
#ifndef WIN32
  setenv ("LC_NUMERIC", "C", 1);
#else
  char tmp1[17];
  sprintf (tmp1, "LC_NUMERIC=C");
  _putenv (tmp1);
#endif
  bind_textdomain_codeset ("zoo-kernel", "UTF-8");
  textdomain ("zoo-kernel");
  bind_textdomain_codeset ("zoo-services", "UTF-8");
  textdomain ("zoo-services");

  map *lsoap = getMap (request_inputs, "soap");
  if (lsoap != NULL && strcasecmp (lsoap->value, "true") == 0)
    setMapInMaps (m, "main", "isSoap", "true");
  else
    setMapInMaps (m, "main", "isSoap", "false");

  if(strlen(cgiServerName)>0)
    {
      char tmpUrl[1024];

      if ( getenv("HTTPS") != NULL && strncmp(getenv("HTTPS"), "on", 2) == 0 ) {
	// Knut: check if non-empty instead of "on"?
	if ( strncmp(cgiServerPort, "443", 3) == 0 ) { 
	  sprintf(tmpUrl, "https://%s%s", cgiServerName, cgiScriptName);
	}
	else {
	  sprintf(tmpUrl, "https://%s:%s%s", cgiServerName, cgiServerPort, cgiScriptName);
	}
      }
      else {
	if ( strncmp(cgiServerPort, "80", 2) == 0 ) { 
	  sprintf(tmpUrl, "http://%s%s", cgiServerName, cgiScriptName);
	}
	else {
	  sprintf(tmpUrl, "http://%s:%s%s", cgiServerName, cgiServerPort, cgiScriptName);
	}
      }
#ifdef DEBUG
      fprintf(stderr,"*** %s ***\n",tmpUrl);
#endif
      if(getMapFromMaps(m,"main","proxied")==NULL)
	setMapInMaps(m,"main","serverAddress",tmpUrl);
      else
	setMapInMaps(m,"lenv","serverAddress",tmpUrl);
    }

  // CORS Support
  if(strncasecmp(cgiRequestMethod,"OPTIONS",7)==0){
    map* cors=getMapFromMaps(m,"main","cors");
    if(cors!=NULL && strncasecmp(cors->value,"true",4)==0){
      char *encoding=getEncoding(m);
      printHeaders(m);
      printf("Content-Type: text/plain; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
      printf(_("CORS is enabled.\r\n"));
      freeMaps (&m);
      free (m);
      fflush (stdout);
      return 3;
    }
  }

  // Populate the Registry
  char conf_dir_[1024];
  char conf_dir[1024];
  int t;
  char tmps1[1024];
  r_inputs = NULL;
  r_inputs = getMap (request_inputs, "metapath");
  map* cwdMap0=getMapFromMaps(m,"main","servicePath");
  if (r_inputs != NULL)
    if(cwdMap0!=NULL)
      snprintf (conf_dir_, 1024, "%s/%s", cwdMap0->value, r_inputs->value);
    else
      snprintf (conf_dir_, 1024, "%s/%s", ntmp, r_inputs->value);
  else
    if(cwdMap0!=NULL)
      snprintf (conf_dir_, 1024, "%s", cwdMap0->value);
    else
      snprintf (conf_dir_, 1024, "%s", ntmp);

   getServicesNamespacePath(m,conf_dir_,conf_dir,1024);
#ifdef DEBUG
  fprintf (stderr, "conf_dir: %s\n", conf_dir_);
  fprintf (stderr, "new conf_dir: %s\n", conf_dir);
#endif
  map* reg = getMapFromMaps (m, "main", "registry");
  registry* zooRegistry=NULL;
  if(reg!=NULL){
#ifndef WIN32
    int saved_stdout = zDup (fileno (stdout));
    zDup2 (fileno (stderr), fileno (stdout));
#endif
    if(createRegistry (m,&zooRegistry,reg->value)<0){
      map *message=getMapFromMaps(m,"lenv","message");
      map *type=getMapFromMaps(m,"lenv","type");
#ifndef WIN32
      zDup2 (saved_stdout, fileno (stdout));
#endif
      errorException (&m, message->value,
		      type->value, NULL);
      return 0;
    }
#ifndef WIN32
    zDup2 (saved_stdout, fileno (stdout));
    zClose(saved_stdout);
#endif
  }

  int eres = SERVICE_STARTED;
  int cpid = zGetpid ();
  maps* bmap=NULL;
  char *fbkp, *fbkpid, *fbkpres, *fbkp1, *flog;
  FILE *f0, *f1;
  HINTERNET hInternet;
  service *s1=NULL;
  maps *request_output_real_format = NULL;
  maps *request_input_real_format = NULL;

  if((strlen(cgiQueryString)>0 && cgiQueryString[0]=='/') /*&& strstr(cgiAccept,"json")!=NULL*/){
    //
    // OGC API - Processes starts here
    //
#ifndef USE_JSON
    errorException (&m, _("OGC API - Processes is not supported by this ZOO-Kernel, please contact the service provider."), "InternalError", NULL);
    return 1;
#else

#ifndef USE_GDB
#ifndef WIN32
    signal (SIGCHLD, SIG_IGN);
#endif
    signal (SIGSEGV, json_sig_handler);
    signal (SIGTERM, json_sig_handler);
    signal (SIGINT, json_sig_handler);
    signal (SIGILL, json_sig_handler);
    signal (SIGFPE, json_sig_handler);
    signal (SIGABRT, json_sig_handler);
#endif
    setMapInMaps(m,"main","executionType","json");
    char *pcaCgiQueryString=NULL;
    if(strstr(cgiQueryString,"&")!=NULL){
      char tmp='&';
      char *token,*saveptr;
      int iCnt=0;
      token=strtok_r(cgiQueryString,"&",&saveptr);
      while(token!=NULL){
	if(iCnt>0){
	  char *token1,*saveptr1;
	  char *name=NULL;
	  char *value=NULL;
	  token1=strtok_r(token,"=",&saveptr1);
	  while(token1!=NULL){
	    if(name==NULL)
	      name=zStrdup(token1);
	    else
	      value=url_decode(token1);
	    token1=strtok_r(NULL,"=",&saveptr1);
	  }
	  addToMapA(request_inputs,name, value != NULL ? value : "");
	  free(name);
	  free(value);
	  name=NULL;
	  value=NULL;
	}else{
	  pcaCgiQueryString=zStrdup(token);
	}
	iCnt++;
	token=strtok_r(NULL,"&",&saveptr);
      }
    }
    if(pcaCgiQueryString==NULL)
      pcaCgiQueryString=zStrdup(cgiQueryString);

    r_inputs = getMapOrFill (&request_inputs, "metapath", "");
    char conf_file1[10240];
    maps* m1 = (maps *) malloc (MAPS_SIZE);
    m1->content = NULL;
    m1->child = NULL;
    m1->next = NULL;
    snprintf (conf_file1, 10240, "%s/%s/oas.cfg", ntmp, r_inputs->value);
    if (conf_read (conf_file1, m1) == 2)
      {
	errorException (NULL, _("Unable to load the oas.cfg file."),
			"InternalError", NULL);
	free (m1);
	return 1;
      }
    addMapsToMaps(&m,m1);
    freeMaps(&m1);
    free(m1);
    initAllEnvironment(m,request_inputs,ntmp,"jrequest");
    setSecurityFlags(m,pcaCgiQueryString);
    // In case security is activated, then execute the security module
    if(ensureFiltered(&m,"in")!=0){
      maps* pmsTmp=getMaps(m,"lenv");
      map *pmError=NULL;
      if(pmsTmp!=NULL){
	addMapToMap(&pmError,pmsTmp->content);
      }
      if(pmError!=NULL)
	localPrintExceptionJ(&m,pmError);
      else
	localPrintExceptionJ(&m,pmsTmp->content);
      // Cleanup memory
      freeMaps(&m);
      free(m);
      free (REQUEST);
      map* pmTest=getMap(request_inputs,"shouldFree");
      if(pmTest!=NULL){
        freeMap (inputs);
        free (*inputs);
        *inputs=NULL;
        freeMap(&r_inputs);
        free (r_inputs);
        r_inputs=NULL;
      }
      free(pcaCgiQueryString);
      return 1;
    }

    bool bIsSupported=true;
    for(int iCnt=0;OAPIPSupportedContentTypes[iCnt]!=NULL;iCnt++){
      if(strcmp(cgiContentType,OAPIPSupportedContentTypes[iCnt])==0){
        bIsSupported=true;
        break;
      }
      else
        bIsSupported=false;
    }
    if(!bIsSupported && strlen(cgiContentType)>0){
      setMapInMaps(m,"headers","Status","415 Unsupported Media Type");
      errorException (&m, _("The Content-Type specified in the request is not supported by this ZOO-Kernel, please contact the service provider."), "UnsupportedMediaType", "Content-Type");
      return 1;
    }

    map* pmsResponse=getMapFromMaps(m,"lenv","response");
    map* pmsResponseFile=getMapFromMaps(m,"lenv","response_generated_file");
    if(pmsResponse!=NULL || pmsResponseFile!=NULL){
      printHeaders(m);
      if(pmsResponseFile!=NULL){
        FILE* pfResponse=fopen(pmsResponseFile->value,"r");
        zStatStruct f_status;
        int s=zStat(pmsResponseFile->value, &f_status);
        if(s==0 && f_status.st_size>0){
          char* pcaFcontent=(char*)malloc(sizeof(char)*(f_status.st_size+1));
          fread(pcaFcontent,f_status.st_size,sizeof(char),pfResponse);
          fclose(pfResponse);
          fwrite(pcaFcontent,sizeof(char),f_status.st_size,stdout);
          fflush(stdout);
          unlink(pmsResponseFile->value);
          free(pcaFcontent);
        }
      }else{
        pmsResponseFile=getMapFromMaps(m,"lenv","response_generated_size");
        if(pmsResponseFile!=NULL){
          fwrite(pmsResponse->value,sizeof(char),atoi(pmsResponseFile->value),stdout);
          fflush(stdout);
        }else
	  printf(pmsResponse->value);
      }
      return 0;
    }

    setMapInMaps(m,"lenv","request_method",cgiRequestMethod);
#ifdef DRU_ENABLED
    // Routes to OGC API - Processes - Part 2: Deploy, Replace, Undeploy (DRU)
    // retrieves the deploy service provider from main.cfg
    map* deployServiceProvider=getMapFromMaps(m,"servicesNamespace","deploy_service_provider");
    map* undeployServiceProvider=getMapFromMaps(m,"servicesNamespace","undeploy_service_provider");
    //Change REQUEST_TYPE in case of Deploy/Undeploy of a Process
    if(strncasecmp(cgiRequestMethod,"DELETE",6)==0
       && strstr(cgiQueryString,"/processes/")!=NULL){
      if(undeployServiceProvider==NULL){
	    setMapInMaps(m,"lenv","status_code","501");
	    map* error=createMap("code","NotImplemented");
	    addToMap(error,"message",_("The request method used to access the current path is not supported (no service available to undeploy)."));
	    localPrintExceptionJ(&m,error);
	    // TODO: cleanup memory
	    freeMaps(&m);
	    free(m);
	    free (REQUEST);
	    freeMap(&error);
	    free(error);
	    freeMap (inputs);
	    free (*inputs);
	    *inputs=NULL;
	    free(pcaCgiQueryString);
	    return 1;
      }
      free(pcaCgiQueryString);
      pcaCgiQueryString=(char*)malloc((strlen(undeployServiceProvider->value)+22)*sizeof(char));
      sprintf(pcaCgiQueryString,"/processes/%s/execution",undeployServiceProvider->value);
#ifndef USE_AMQP
      setMapInMaps(m,"lenv","noRunSql","false");
#endif
      setMapInMaps(m,"lenv","request_method","POST");
      if(strlen(strstr(cgiQueryString,"/processes/"))>11){
	char* pcaProcessId=zStrdup(strstr(cgiQueryString,"/processes/")+11);
	char newPath[1024];
	getServicesNamespacePath(m,ntmp,newPath,1024);
	fprintf(stderr,"----+++++++--- %s %d %s \n",__FILE__,__LINE__,newPath);
	fprintf(stderr,"----+++++++--- %s %d %s \n",__FILE__,__LINE__,pcaProcessId);
	fflush(stderr);
	setMapInMaps(m,"lenv","deployedServiceId",pcaProcessId);
	char* pcaFileName=(char*)malloc((strlen(newPath)+strlen(pcaProcessId)+7)*sizeof(char));
	sprintf(pcaFileName,"%s/%s.json",newPath,pcaProcessId);
	FILE* pfRequest=fopen(pcaFileName,"rb");
	if(pfRequest==NULL) {
	  setMapInMaps(m,"request","Content-Type","application/cwl");
	  sprintf(pcaFileName,"%s/%s.cwl",newPath,pcaProcessId);
	  pfRequest=fopen(pcaFileName,"rb");
	}else
	  setMapInMaps(m,"request","Content-Type","application/ogcapppkg+json");
	if(pfRequest!=NULL){
	  zStatStruct f_status;
	  int s=zStat(pcaFileName, &f_status);
	  if(f_status.st_size>0){
	    char* pcaTmp=(char*)malloc((f_status.st_size+1)*sizeof(char));
	    fread(pcaTmp,1,f_status.st_size,pfRequest);
	    pcaTmp[f_status.st_size]=0;
	    addToMap(request_inputs,"jrequest",pcaTmp);
	    setMapInMaps(m,"renv","jrequest",pcaTmp);
	    free(pcaTmp);
	    if(strstr(pcaFileName,"cwl")!=NULL){
	      json_object* pjRes=convertCwlToOGCAppPkg(m,request_inputs);
	      if(pjRes!=NULL){
	        const char* jsonStr=json_object_to_json_string_ext(pjRes,JSON_C_TO_STRING_NOSLASHESCAPE);
	        setMapInMaps(m,"renv","jrequest",jsonStr);
	        json_object_put(pjRes);
	      }
	    }
	  }
	  fclose(pfRequest);
	}else{
	  //if(strncasecmp(cgiRequestMethod,"DELETE",6)=0){
      setMapInMaps(m,"headers","Status","403 Forbidden");
      map* error=createMap("code","ImmutableProcess");
      addToMap(error,"message",_("The process cannot be modified."));
      setMapInMaps(m,"headers","Content-Type","application/json;charset=UTF-8");
      localPrintExceptionJ(&m,error);
      //json_object_put(res);
      // TODO: cleanup memory
      freeMaps(&m);
      free(m);
      free (REQUEST);
      freeMap (inputs);
      free (*inputs);
      *inputs=NULL;
      free(pcaCgiQueryString);
      return 1;
	  //}
	}
      }
    } else
      if(((strcmp(cgiQueryString,"/processes")==0 ||
	   strcmp(cgiQueryString,"/processes/")==0) &&
	  strcasecmp(cgiRequestMethod,"post")==0) ||
	 (strcasecmp(cgiRequestMethod,"put")==0 &&
	  strstr(cgiQueryString,"/processes/")!=NULL &&
	  strlen(strstr(cgiQueryString,"/processes/"))>11) ){
	if(deployServiceProvider==NULL){
	  setMapInMaps(m,"lenv","status_code","501");
	  map* error=createMap("code","NotImplemented");
	  addToMap(error,"message",_("The request method used to access the current path is not supported."));
	  localPrintExceptionJ(&m,error);
	  // TODO: cleanup memory
	  freeMaps(&m);
	  free(m);
	  free (REQUEST);
	  freeMap(&error);
	  free(error);
	  freeMap (inputs);
	  free (*inputs);
	  *inputs=NULL;
	  free(pcaCgiQueryString);
	  return 1;
	}
	// TODO: validate that the service is not yet deployed
	free(pcaCgiQueryString);
	pcaCgiQueryString=(char*)malloc((strlen(deployServiceProvider->value)+22)*sizeof(char));
	sprintf(pcaCgiQueryString,"/processes/%s/execution",deployServiceProvider->value);
#ifndef USE_AMQP
	setMapInMaps(m,"lenv","noRunSql","false");
#endif
	map* pmWValue=getMapFromMaps(m,"request","w");
	if(pmWValue!=NULL)
	  setMapInMaps(m,"lenv","workflow_id",pmWValue->value);
	if(strncasecmp(cgiRequestMethod,"PUT",3)==0){
	  setMapInMaps(m,"lenv","request_method","POST");
	  setMapInMaps(m,"lenv","orequest_method","PUT");
	  setMapInMaps(m,"lenv","toDeploy",strstr(cgiQueryString,"/processes/")+11);
	  setMapInMaps(m,"lenv","workflow_id",strstr(cgiQueryString,"/processes/")+11);
	  map* pmToDeploy=getMapFromMaps(m,"lenv","toDeploy");
	  if(pmToDeploy!=NULL){
	    setMapInMaps(m,"lenv","requestType","GetCapabilities");
	    setMapInMaps(m,"lenv","Identifier",pmToDeploy->value);
	    setMapInMaps(m,"lenv","oIdentifier",pmToDeploy->value);
	    json_object *res3=json_object_new_object();
	    json_object *res4=json_object_new_array();
	    //int saved_stdout = zDup (fileno (stdout));
	    int t=fetchServicesForDescription(NULL, &m, pmToDeploy->value,
					      printGetCapabilitiesForProcessJ,
					      (void*) res4, (void*) res3, ntmp,
					      request_inputs,
					      localPrintExceptionJ);
	    //zDup2 (saved_stdout, fileno (stdout));
	    //zClose(saved_stdout);
	    json_object *res5=json_object_array_get_idx(res4,0);
	    if(res5==NULL){
	      map* pmError=createMap("code","NoSuchProcess");
	      addToMap(pmError,"message",_("The process failed to be updated."));
	      localPrintExceptionJ(&m,pmError);
	      // TODO: cleanup memory
	      json_object_put(res4);
	      json_object_put(res3);
	      freeMaps(&m);
	      free(m);
	      free (REQUEST);
	      freeMap (inputs);
	      free (*inputs);
	      *inputs=NULL;
	      free(pcaCgiQueryString);
	      return 1;
	    }
	  }
	}
      }
#endif
    map* pmCgiRequestMethod=getMapFromMaps(m,"lenv","request_method");

    setRootUrlMap(m);
    // Redirect using HTTP Status 301 (cf. rfc2616) in case full_html_support is
    // set to true
    map* pmTmp0=getMapFromMaps(m,"openapi","full_html_support");
    if(strstr(pcaCgiQueryString,".html")==NULL && strstr(cgiAccept,"text/html")!=NULL && pmTmp0!=NULL && strncasecmp(pmTmp0->value,"true",4)==0){
      map* pmTmpUrl=getMapFromMaps(m,"openapi","rootUrl");
      char* pacTmpUrl=NULL;
      if(strcmp(pcaCgiQueryString,"/")!=0){
	pacTmpUrl=(char*)malloc((strlen(pcaCgiQueryString)+strlen(pmTmpUrl->value)+6)*sizeof(char));
	sprintf(pacTmpUrl,"%s%s.html",pmTmpUrl->value,pcaCgiQueryString);
      }
      else{
	pacTmpUrl=(char*)malloc((strlen(pmTmpUrl->value)+6)*sizeof(char));
	sprintf(pacTmpUrl,"%s/index.html",pmTmpUrl->value);
      }
      setMapInMaps(m,"headers","Location",pacTmpUrl);
      setMapInMaps(m,"headers","Status","301 Moved permanently");
      printHeaders(m);
      fflush(stdout);
      free(pacTmpUrl);
      return 1;
    }
    json_object *res=json_object_new_object();
    setMapInMaps(m,"headers","Content-Type","application/json;charset=UTF-8");

    if((strstr(pcaCgiQueryString,"/processes")!=NULL && strncasecmp(pcaCgiQueryString,"/processes",10)!=0) ||
       (strstr(pcaCgiQueryString,"/jobs")!=NULL && strncasecmp(pcaCgiQueryString,"/jobs",5)!=0) ){
	map* error=createMap("code","BadRequest");
	addToMap(error,"message",_("The resource is not available"));
	localPrintExceptionJ(&m,error);
	freeMaps (&m);
	free (m);
	free (REQUEST);
	json_object_put(res);
	freeMap (inputs);
	free (*inputs);
	*inputs=NULL;
	freeMap (&error);
	free (error);
	free(pcaCgiQueryString);
	xmlCleanupParser ();
	zooXmlCleanupNs ();
	return 1;
    }
    if((strncasecmp(pmCgiRequestMethod->value,"post",4)==0 &&
	(strstr(pcaCgiQueryString,"/processes/")==NULL ||
	 strlen(pcaCgiQueryString)<=11))
       ||
       (strncasecmp(pmCgiRequestMethod->value,"DELETE",6)==0 &&
	(strstr(pcaCgiQueryString,"/jobs/")==NULL || strlen(pcaCgiQueryString)<=6)) ){
      setMapInMaps(m,"lenv","status_code","405");
      map* pmaError=createMap("code","InvalidMethod");
      addToMap(pmaError,"message",_("The request method used to access the current path is not supported."));
      localPrintExceptionJ(&m,pmaError);
      json_object_put(res);
      // TODO: cleanup memory
      freeMaps(&m);
      free(m);
      free (REQUEST);
      freeMap(&pmaError);
      free(pmaError);
      freeMap (inputs);
      free (*inputs);
      *inputs=NULL;
      free(pcaCgiQueryString);
      return 1;
    }
    else if(cgiContentLength==1){
      /* - Root url */
      if(strncasecmp(cgiRequestMethod,"GET",3)!=0){
	setMapInMaps(m,"lenv","status_code","405");
	map* pmaError=createMap("code","InvalidMethod");
	const char* pccErr=_("This API does not support the method.");
	addToMap(pmaError,"message",pccErr);
	localPrintExceptionJ(&m,pmaError);
	// TODO: cleanup memory
	freeMaps(&m);
	free(m);
	free (REQUEST);
	freeMap(&pmaError);
	free(pmaError);
	freeMap (inputs);
	free (*inputs);
	*inputs=NULL;
	free(pcaCgiQueryString);
	return 1;
      }
      map* tmpMap=getMapFromMaps(m,"main","serverAddress");
      json_object *res1=json_object_new_array();

      map* pmVal=getMapFromMaps(m,"openapi","links");
      char *orig=zStrdup(pmVal->value);
      char *saveptr;
      char *tmps = strtok_r (orig, ",", &saveptr);
      map* tmpUrl=getMapFromMaps(m,"openapi","rootUrl");
      while(tmps!=NULL){
	maps* tmpMaps=getMaps(m,tmps);
	if(tmpMaps!=NULL){
	  json_object *res2;
	  res2=mapToJson(tmpMaps->content);
	  char* tmpStr=(char*) malloc((strlen(tmpUrl->value)+strlen(tmps)+2)*sizeof(char));
	  sprintf(tmpStr,"%s%s",tmpUrl->value,tmps);
	  json_object_object_add(res2,"href",json_object_new_string(tmpStr));
	  free(tmpStr);
	  json_object_array_add(res1,res2);

	  map* pmTmp=getMap(tmpMaps->content,"title");
	  char *pacTitle=NULL;
	  if(pmTmp!=NULL)
	    pacTitle=zStrdup(pmTmp->value);
	  char* pacTmp=(char*) malloc((strlen(tmps)+6)*sizeof(char));
	  sprintf(pacTmp,"%s.html",tmps);
	  tmpMaps=getMaps(m,pacTmp);
	  if(tmpMaps==NULL && strncasecmp(pacTmp,"/.html",6)==0)
	    tmpMaps=getMaps(m,"/index.html");
	  if(tmpMaps!=NULL){
	    json_object *res3;
	    res3=mapToJson(tmpMaps->content);
	    if(getMap(tmpMaps->content,"title")==NULL && pacTitle!=NULL){
	      json_object_object_add(res3,"title",json_object_new_string(_(pacTitle)));
	    }
	    char* tmpStr=NULL;
	    if(strncasecmp(pacTmp,"/.html",6)==0){
	      tmpStr=(char*) malloc((strlen(tmpUrl->value)+12)*sizeof(char));
	      sprintf(tmpStr,"%s/index.html",tmpUrl->value);
	    }else{map* tmpUrl=getMapFromMaps(m,"openapi","rootUrl");
	      tmpStr=(char*) malloc((strlen(tmpUrl->value)+strlen(tmps)+6)*sizeof(char));
	      sprintf(tmpStr,"%s%s.html",tmpUrl->value,tmps);
	    }
	    json_object_object_add(res3,"href",json_object_new_string(tmpStr));
	    free(tmpStr);
	    json_object_array_add(res1,res3);
	  }
	  free(pacTmp);
	  if(pacTitle!=NULL)
	    free(pacTitle);
	}
	tmps = strtok_r (NULL, ",", &saveptr);
      }
      free(orig);

      map* pmTmp=getMapFromMaps(m,"identification","title");
      if(pmTmp!=NULL)
	json_object_object_add(res,"title",json_object_new_string(_(pmTmp->value)));
      pmTmp=getMapFromMaps(m,"identification","abstract");
      if(pmTmp!=NULL)
	json_object_object_add(res,"description",json_object_new_string(_(pmTmp->value)));
      json_object_object_add(res,"links",res1);
    }else if(strcmp(pcaCgiQueryString,"/conformance")==0){
      /* - /conformance url */
      maps* pmsConformsTo=getMaps(m,"conformsTo");
      map* pmExtentionsLength=getMapFromMaps(m,"conformsTo","extentionsLength");
      int len=0;
      map* pmExtentionUrl=NULL;
      if(pmExtentionsLength!=NULL){
	len=atoi(pmExtentionsLength->value);
      }
      map* rootUrl=getMapFromMaps(m,"conformsTo","rootUrl");
      json_object *res1=json_object_new_array();
      map* length=getMapFromMaps(m,"conformsTo","length");
      for(int kk=0;kk<atoi(length->value);kk++){
	map* tmpMap1=getMapArray(pmsConformsTo->content,"link",kk);
	json_object *res2;
	if(tmpMap1!=NULL){
	  char* tmpStr=NULL;
	  if(getMapArray(pmsConformsTo->content,"extention",kk)!=NULL){
	    map* pmCid=getMapArray(pmsConformsTo->content,"extid",kk);
	    if(pmCid!=NULL){
	      pmExtentionUrl=getMapArray(pmsConformsTo->content,"extentionUrl",atoi(pmCid->value));
	      if(pmExtentionUrl!=NULL){
		tmpStr=(char*) malloc((strlen(pmExtentionUrl->value)+strlen(tmpMap1->value)+1)*sizeof(char));
		sprintf(tmpStr,"%s%s",pmExtentionUrl->value,tmpMap1->value);
	      }
	    }
	  }
	  else{
	    tmpStr=(char*) malloc((strlen(rootUrl->value)+strlen(tmpMap1->value)+1)*sizeof(char));
	    sprintf(tmpStr,"%s%s",rootUrl->value,tmpMap1->value);
	  }
	  json_object_array_add(res1,json_object_new_string(tmpStr));
	  free(tmpStr);
	}
      }
      json_object_object_add(res,"conformsTo",res1);
    }else if(strncasecmp(pcaCgiQueryString,"/api",4)==0){
      if(strstr(pcaCgiQueryString,".html")==NULL)
	produceApi(m,res);
      else{
	char* pcaTmp=(char*)malloc(9*sizeof(char));
	sprintf(pcaTmp,"%s",pcaCgiQueryString+1);
	map* pmTmp=getMapFromMaps(m,pcaTmp,"href");
	free(pcaTmp);
	if(pmTmp!=NULL)
	  setMapInMaps(m,"headers","Location",pmTmp->value);
      }
    }
    // GET method to route /processes or /processes/
    // Returns list of processes
    else if(strncasecmp(pmCgiRequestMethod->value,"get",3)==0 && (strcmp(pcaCgiQueryString,"/processes")==0 || strcmp(pcaCgiQueryString,"/processes/")==0)){
      /* - /processes */
      setMapInMaps(m,"lenv","requestType","desc");
      setMapInMaps(m,"lenv","serviceCnt","0");
      setMapInMaps(m,"lenv","serviceCounter","0");
      map* pmTmp=getMap(request_inputs,"limit");
      if(pmTmp!=NULL)
	setMapInMaps(m,"lenv","serviceCntLimit",pmTmp->value);
      else{
	pmTmp=getMapFromMaps(m,"limitParam","schema_default");
	if(pmTmp!=NULL)
	  setMapInMaps(m,"lenv","serviceCntLimit",pmTmp->value);
      }
      pmTmp=getMap(request_inputs,"skip");
      if(pmTmp!=NULL)
	setMapInMaps(m,"lenv","serviceCntSkip",pmTmp->value);
      json_object *res3=json_object_new_array();
      int saved_stdout = zDup (fileno (stdout));
      zDup2 (fileno (stderr), fileno (stdout));
      if (int res0 =
          recursReaddirF (m, NULL, res3, NULL, ntmp, NULL, saved_stdout, 0,
			  printGetCapabilitiesForProcessJ) < 0) {
      }else{
	fflush(stderr);
	fflush(stdout);
	zDup2 (saved_stdout, fileno (stdout));
      }
      zClose(saved_stdout);
#ifdef META_DB
      int nbServices=fetchServicesFromDb(zooRegistry,m,res3,NULL,printGetCapabilitiesForProcessJ,1);
      // Keep track of the total number of processes
      map* pmCnt=getMapFromMaps(m,"lenv","serviceCnt");
      char acCnt[10];
      if(pmCnt!=NULL)
	sprintf(acCnt,"%d",nbServices+atoi(pmCnt->value));
      else
	sprintf(acCnt,"%d",nbServices);
      setMapInMaps(m,"lenv","serviceCnt",acCnt);
      // We can now close the SQL backend connexion
      close_sql(m,0);
#endif
      json_object_object_add(res,"processes",res3);
      setMapInMaps(m,"lenv","path","processes");
      createNextLinks(m,res);
    }
    else if(strstr(pcaCgiQueryString,"/processes")==NULL && (strstr(pcaCgiQueryString,"/jobs")!=NULL || strstr(pcaCgiQueryString,"/jobs/")!=NULL)){
      /* - /jobs url */
      if(strncasecmp(pmCgiRequestMethod->value,"DELETE",6)==0) {
	char* pcTmp=strstr(pcaCgiQueryString,"/jobs/");
	if(strlen(pcTmp)>6){
	  char* jobId=zStrdup(pcTmp+6);
	  setMapInMaps(m,"lenv","gs_usid",jobId);
	  char* pcaStatusFilePath=json_getStatusFilePath(m);
	  setMapInMaps(m,"lenv","file.statusFile",pcaStatusFilePath);
	  free(pcaStatusFilePath);
	  runDismiss(m,jobId);
	  map* pmError=getMapFromMaps(m,"lenv","error");
	  if(pmError!=NULL && strncasecmp(pmError->value,"true",4)==0){
	    localPrintExceptionJ(&m,getMapFromMaps(m,"lenv","code"));
	    register_signals(donothing);
	    freeService (&s1);
	    free(s1);
	    freeMaps (&m);
	    free (m);
	    free (REQUEST);
	    json_object_put(res);
	    freeMap (inputs);
	    free (*inputs);
	    *inputs=NULL;
	    freeMap (&r_inputs);
	    free (r_inputs);
	    free(jobId);
	    free(pcaCgiQueryString);
	    xmlCleanupParser ();
	    zooXmlCleanupNs ();
	    return 1;
	  }
	  else{
	    setMapInMaps(m,"lenv","gs_location","false");
	    if(res!=NULL)
	      json_object_put(res);
	    res=createStatus(m,SERVICE_DISMISSED);
	  }
	}
      }
      else if(strcasecmp(pmCgiRequestMethod->value,"get")==0){
	/* - /jobs List (GET) */
	if(strncasecmp(pmCgiRequestMethod->value,"get",3)==0 && strlen(pcaCgiQueryString)<=6){
	  map* pmTmp=getMap(request_inputs,"limit");
	  if(pmTmp!=NULL)
	    setMapInMaps(m,"lenv","serviceCntLimit",pmTmp->value);
	  else{
	    pmTmp=getMapFromMaps(m,"limitParam","schema_default");
	    if(pmTmp!=NULL)
	      setMapInMaps(m,"lenv","serviceCntLimit",pmTmp->value);
	  }
	  pmTmp=getMap(request_inputs,"skip");
	  if(pmTmp!=NULL)
	    setMapInMaps(m,"lenv","serviceCntSkip",pmTmp->value);
	  // Build the SQL Clause
	  char* pcaClause=NULL;
	  char* pcaClauseFinal=NULL;
	  for(int k=0;k<2;k++){
	    pmTmp=getMap(request_inputs,statusSearchFields[k]);
	    if(pmTmp!=NULL){
	      char *saveptr;
	      char *tmps = strtok_r(pmTmp->value, ",", &saveptr);
	      int iCnt=0;
	      while (tmps != NULL){
		for(int l=0;l<3;l++){
		  if(strcmp(tmps,oapipStatus[l])==0){
		    tmps=wpsStatus[l];
		    break;
		  }
		}
		maps* pmsLenv=getMaps(m,"lenv");
		if(pcaClause==NULL){
		  pcaClause=(char*)malloc((strlen(statusSearchFieldsReal[k])+strlen(tmps)+10)*sizeof(char));
		  sprintf(pcaClause," (%s=$q$%s$q$",statusSearchFieldsReal[k],tmps);
		  setMapArray(pmsLenv->content,"servicePidFilter",iCnt,tmps);
		}else{
		  char* pcaTmp=zStrdup(pcaClause);
		  pcaClause=(char*)realloc(pcaClause,strlen(statusSearchFieldsReal[k])+strlen(tmps)+strlen(pcaTmp)+12);
		  sprintf(pcaClause,"%s OR %s=$q$%s$q$",pcaTmp,statusSearchFieldsReal[k],tmps);
		  setMapArray(pmsLenv->content,"servicePidFilter",iCnt,tmps);
		  free(pcaTmp);
		}
		tmps = strtok_r (NULL, ",", &saveptr);
		iCnt++;
	      }
	      char* pcaTmp=zStrdup(pcaClause);
	      pcaClause=(char*)realloc(pcaClause,strlen(pcaTmp)+3);
	      sprintf(pcaClause,"%s) ",pcaTmp);
	      free(pcaTmp);
	    }
	    if(pcaClause!=NULL){
	      if(pcaClauseFinal==NULL){
		pcaClauseFinal=(char*)malloc((strlen(pcaClause)+1)*sizeof(char));
		sprintf(pcaClauseFinal,"%s",pcaClause);
	      }else{
		char* pcaTmp=zStrdup(pcaClauseFinal);
		pcaClauseFinal=(char*)realloc(pcaClauseFinal,strlen(pcaTmp)+(strlen(pcaClause)+6)*sizeof(char));
		sprintf(pcaClauseFinal,"%s AND %s",pcaClause,pcaTmp);
		free(pcaTmp);
	      }
	      free(pcaClause);
	      pcaClause=NULL;
	    }
	  }
	  // (min/max)Duration should be set '%s second'::interval
	  char* pcaClauseMin=NULL;
	  pmTmp=getMap(request_inputs,"minDuration");
	  if(pmTmp!=NULL){
	    setMapInMaps(m,"lenv","serviceMinDuration",pmTmp->value);
	    char *saveptr;
	    char *tmps = strtok_r(pmTmp->value, ",", &saveptr);
	    while (tmps != NULL){
	      if(pcaClauseMin==NULL){
		pcaClauseMin=(char*)malloc((strlen(tmps)+52)*sizeof(char));
		sprintf(pcaClauseMin," ('%s second'::interval <= (end_time - creation_time)",tmps);
	      }else{
		char* pcaTmp=zStrdup(pcaClauseMin);
		pcaClauseMin=(char*)realloc(pcaClauseMin,strlen(tmps)+strlen(pcaTmp)+54);
		sprintf(pcaClauseMin,"%s OR '%s second'::interval <= (end_time - creation_time)",pcaTmp,tmps);
		free(pcaTmp);
	      }
	      tmps = strtok_r (NULL, ",", &saveptr);
	    }
	    char* pcaTmp=zStrdup(pcaClauseMin);
	    pcaClauseMin=(char*)realloc(pcaClauseMin,strlen(pcaTmp)+3);
	    sprintf(pcaClauseMin,"%s) ",pcaTmp);
	    free(pcaTmp);
	    if(pcaClauseFinal!=NULL){
	      char* pcaTmp=zStrdup(pcaClauseFinal);
	      pcaClauseFinal=(char*)realloc(pcaClauseFinal,strlen(pcaTmp)+(strlen(pcaClauseMin)+6)*sizeof(char));
	      sprintf(pcaClauseFinal,"%s AND %s",pcaClauseMin,pcaTmp);
	      free(pcaTmp);
	    }else{
	      pcaClauseFinal=(char*)malloc((strlen(pcaClauseMin)+1)*sizeof(char));
	      sprintf(pcaClauseFinal,"%s",pcaClauseMin);
	    }
	    free(pcaClauseMin);
	  }
	  char* pcaClauseMax=NULL;
	  pmTmp=getMap(request_inputs,"maxDuration");
	  if(pmTmp!=NULL){
	    setMapInMaps(m,"lenv","serviceMaxDuration",pmTmp->value);
	    char *saveptr;
	    char *tmps = strtok_r(pmTmp->value, ",", &saveptr);
	    while (tmps != NULL){
	      if(pcaClauseMax==NULL){
		pcaClauseMax=(char*)malloc((strlen(tmps)+52)*sizeof(char));
		sprintf(pcaClauseMax," ('%s second'::interval >= (end_time - creation_time)",tmps);
	      }else{
		char* pcaTmp=zStrdup(pcaClauseMax);
		pcaClauseMax=(char*)realloc(pcaClauseMax,strlen(tmps)+strlen(pcaTmp)+54);
		sprintf(pcaClauseMax,"%s OR '%s second'::interval >= (end_time - creation_time)",pcaTmp,tmps);
		free(pcaTmp);
	      }
	      tmps = strtok_r (NULL, ",", &saveptr);
	    }
	    char* pcaTmp=zStrdup(pcaClauseMax);
	    pcaClauseMax=(char*)realloc(pcaClauseMax,strlen(pcaTmp)+3);
	    sprintf(pcaClauseMax,"%s) ",pcaTmp);
	    free(pcaTmp);
	    if(pcaClauseFinal!=NULL){
	      char* pcaTmp=zStrdup(pcaClauseFinal);
	      pcaClauseFinal=(char*)realloc(pcaClauseFinal,strlen(pcaTmp)+(strlen(pcaClauseMax)+6)*sizeof(char));
	      sprintf(pcaClauseFinal,"%s AND %s",pcaClauseMax,pcaTmp);
	      free(pcaTmp);
	    }else{
	      pcaClauseFinal=(char*)malloc((strlen(pcaClauseMax)+1)*sizeof(char));
	      sprintf(pcaClauseFinal,"%s",pcaClauseMax);
	    }
	    free(pcaClauseMax);
	  }
	  char* pcaClauseType=NULL;
	  pmTmp=getMap(request_inputs,"type");
	  if(pmTmp!=NULL){
	    setMapInMaps(m,"lenv","serviceType",pmTmp->value);
	    char *saveptr;
	    char *tmps = strtok_r(pmTmp->value, ",", &saveptr);
	    while (tmps != NULL){
	      if(strcmp(tmps,"process")==0)
		pcaClauseType=zStrdup(tmps);
	      tmps = strtok_r (NULL, ",", &saveptr);
	    }
	    if(pcaClauseType!=NULL){
	      if(pcaClauseFinal!=NULL){
		char* pcaTmp=zStrdup(pcaClauseFinal);
		pcaClauseFinal=(char*)realloc(pcaClauseFinal,
					      (strlen(pcaTmp)+18)*sizeof(char));
		sprintf(pcaClauseFinal,"%s AND itype='json'",
			pcaTmp);
		free(pcaTmp);
	      }else{
		pcaClauseFinal=(char*)malloc(13*sizeof(char));
		sprintf(pcaClauseFinal,"itype='json'");
	      }
	      free(pcaClauseType);
	    }
	  }
	  // datetime parameter
	  char* pcaClauseDate=NULL;
	  pmTmp=getMap(request_inputs,"datetime");
	  if(pmTmp!=NULL){
	    if(strstr(pmTmp->value,"/")!=NULL){
	      char *saveptr0;
	      char *tmps0 = strtok_r(pmTmp->value, ",", &saveptr0);
	      int iCnt0=0;
	      while(tmps0 != NULL){
		if(iCnt0>0){
		  char *saveptr;
		  char *tmps = strtok_r(tmps0, "/", &saveptr);
		  int iCnt=0;
		  while (tmps != NULL){
		    if(iCnt==0 && tmps0[0]!='/'){
		      if(strlen(tmps)>2){
			pcaClauseDate=(char*)malloc((strlen(tmps)+30+1)*sizeof(char));
			sprintf(pcaClauseDate,"creation_time>='%s'::timestamptz",tmps);
		      }
		    }else{
		      if(strlen(tmps)>2){
			if(pcaClauseDate==NULL){
			  pcaClauseDate=(char*)malloc((strlen(tmps)+30+1)*sizeof(char));
			  sprintf(pcaClauseDate,"creation_time<='%s'::timestamptz",tmps);
			}else{
			  char* pcaTmp=zStrdup(pcaClauseDate);
			  pcaClauseDate=(char*)realloc(pcaClauseDate,(strlen(pcaTmp)+strlen(tmps)+35+1)*sizeof(char));
			  sprintf(pcaClauseDate,"%s AND creation_time<='%s'::timestamptz",pcaTmp,tmps);
			  free(pcaTmp);
			}
		      }
		    }
		    iCnt++;
		    tmps = strtok_r (NULL, "/", &saveptr);
		  }
		}
		iCnt0++;
		tmps0 = strtok_r (NULL, ",", &saveptr0);
	      }
	    }else{
	      char *saveptr0;
	      char *tmps0 = strtok_r(pmTmp->value, ",", &saveptr0);
	      int iCnt0=0;
	      while(tmps0 != NULL){
		if(iCnt0>0){
		  pcaClauseDate=(char*)malloc((strlen(tmps0)+29+1)*sizeof(char));
		  sprintf(pcaClauseDate,"creation_time='%s'::timestamptz",tmps0);
		}
		iCnt0++;
		tmps0 = strtok_r (NULL, ",", &saveptr0);
	      }
	    }
	    if(pcaClauseFinal==NULL)
	      pcaClauseFinal=zStrdup(pcaClauseDate);
	    else{
	      char* pcaTmp=zStrdup(pcaClauseFinal);
	      pcaClauseFinal=(char*)realloc(pcaClauseFinal,
					    (strlen(pcaTmp)+strlen(pcaClauseDate)+6)*sizeof(char));
	      sprintf(pcaClauseFinal,"%s AND %s",
		      pcaTmp,pcaClauseDate);
	      free(pcaTmp);
	    }
	    free(pcaClauseDate);
	  }
	  // Filter jobs list based on the potential user_id
	  filterJobByUser(m,&pcaClauseFinal,pcaClauseDate);
	  if(pcaClauseFinal==NULL){
	    pcaClauseFinal=zStrdup("true");
	  }else{
	    fprintf(stderr,"%s %d %s \n",__FILE__,__LINE__,pcaClauseFinal);
	    fflush(stderr);
	  }
	  map *schema=getMapFromMaps(m,"database","schema");
	  if(pcaClauseFinal!=NULL && schema!=NULL){
	    char* pcaTmp=(char*) malloc((strlen(pcaClauseFinal)+
					 strlen(schema->value)+
					 98+1)
					*sizeof(char));
	    sprintf(pcaTmp,
		    "select replace(replace(array(select ''''||uuid||'''' "
		    "from  %s.services where %s)::text,'{',''),'}','')",
		    schema->value,pcaClauseFinal);
	    free(pcaClauseFinal);
	    char* tmp1=
#ifdef RELY_ON_DB
	      runSqlQuery(m,pcaTmp)
#else
	      NULL
#endif
	      ;
	    free(pcaTmp);
	    if(tmp1!=NULL){
	      setMapInMaps(m,"lenv","selectedJob",tmp1);
	      free(tmp1);
	    }
#ifdef RELY_ON_DB
	    else{
	      setMapInMaps(m,"lenv","selectedJob","-1");
	    }
#endif
	  }else{
	    free(pcaClauseFinal);
	  }
	  if(res!=NULL)
	    json_object_put(res);
	  res=printJobList(m);
	}
	else{
	  char* tmpUrl=strstr(pcaCgiQueryString,"/jobs/");
	  if(tmpUrl!=NULL && strlen(tmpUrl)>6){
	    // TODO: verify that the user is allowed to access the jobId
	    // invoke filterJobByUser?
	    if(strncasecmp(pmCgiRequestMethod->value,"DELETE",6)==0){
	      char* jobId=zStrdup(tmpUrl+6);
	      setMapInMaps(m,"lenv","gs_usid",jobId);
	      setMapInMaps(m,"lenv","file.statusFile",json_getStatusFilePath(m));
	      runDismiss(m,jobId);
	      map* pmError=getMapFromMaps(m,"lenv","error");
	      if(pmError!=NULL && strncasecmp(pmError->value,"true",4)==0){
		localPrintExceptionJ(&m,getMapFromMaps(m,"lenv","code"));
		freeMaps(&m);
		free(m);
		json_object_put(res);
		free(jobId);
		free(pcaCgiQueryString);
		return 1;
	      }
	      else{
		setMapInMaps(m,"lenv","gs_location","false");
		if(res!=NULL)
		  json_object_put(res);
		res=createStatus(m,SERVICE_DISMISSED);
	      }
	    }else{
	      char* jobId=zStrdup(tmpUrl+6);
	      if(strlen(jobId)==36){
		if(res!=NULL)
		  json_object_put(res);
		ensureFiltered(&m,"out");
		res=printJobStatus(m,jobId);
		if(res==NULL)
		  fflush(stdout);
	      }else{
		// In case the service has run, then forward request to target result file
		if(strlen(jobId)>36)
		  jobId[36]=0;
		char *sid=getStatusId(m,jobId);
		if(sid==NULL){
		  map* error=createMap("code","NoSuchJob");
		  addToMap(error,"message",_("The JobID from the request does not match any of the Jobs running on this server"));
		  localPrintExceptionJ(&m,error);
		  free(jobId);
		  freeMap(&error);
		  free(error);
		  json_object_put(res);
		  free(pcaCgiQueryString);
		  map* pmTest=getMap(request_inputs,"shouldFree");
		  if(pmTest!=NULL){
		    freeMap (inputs);
		    free (*inputs);
		    *inputs=NULL;
		    freeMap(&r_inputs);
		    free (r_inputs);
		    r_inputs=NULL;
		  }
		  cleanUpSql(m);
		  freeMaps(&m);
		  free(m);
		  return 1;
		}else{
		  if(isRunning(m,jobId)>0){
		    map* error=createMap("code","ResultNotReady");
		    addToMap(error,"message",_("The job is still running."));
		    localPrintExceptionJ(&m,error);
		    free(jobId);
		    freeMap(&error);
		    free(error);
		    json_object_put(res);
		    free(pcaCgiQueryString);
		    map* pmTest=getMap(request_inputs,"shouldFree");
		    if(pmTest!=NULL){
		      freeMap (inputs);
		      free (*inputs);
		      *inputs=NULL;
		      freeMap(&r_inputs);
		      free (r_inputs);
		      r_inputs=NULL;
		    }
		    cleanUpSql(m);
		    freeMaps(&m);
		    free(m);
		    return 1;
		  }else{
		    char *Url0=getResultPath(m,jobId);
		    zStatStruct f_status;
		    int s=zStat(Url0, &f_status);
		    if(s==0 && f_status.st_size>0){
		      if(f_status.st_size>15){
			json_object* pjoTmp=json_readFile(m,Url0);
			json_object* pjoCode=NULL;
			json_object* pjoMessage=NULL;
			if(pjoTmp!=NULL &&
			   !jsonIsFalse(json_object_object_get_ex(pjoTmp,"code",&pjoCode)) &&
			   !jsonIsFalse(json_object_object_get_ex(pjoTmp,"description",&pjoMessage)) ){
			  map* error=createMap("code",json_object_get_string(pjoCode));
			  addToMap(error,"message",json_object_get_string(pjoMessage));
			  localPrintExceptionJ(&m,error);
			  free(jobId);
			  freeMap(&error);
			  free(error);
			  freeMaps(&m);
			  free(m);
			  json_object_put(res);
			  json_object_put(pjoTmp);
		  	  free(pcaCgiQueryString);
			  return 1;
			}else{

			  map* tmpPath = getMapFromMaps (m, "main", "tmpUrl");
			  Url0=(char*) realloc(Url0,(strlen(tmpPath->value)+
						     //strlen(cIdentifier->value)+
						     strlen(jobId)+8)*sizeof(char));
			  sprintf(Url0,"%s/%s.json",tmpPath->value,jobId);
			  setMapInMaps(m,"headers","Location",Url0);
			  ensureFiltered(&m,"out");
			}
			if(pjoTmp!=NULL)
			  json_object_put(pjoTmp);
			free(Url0);
		      }else{
			// Service Failed
			map* statusInfo=createMap("JobID",jobId);
			readFinalRes(m,jobId,statusInfo);
			{
			  map* pmStatus=getMap(statusInfo,"status");
			  if(pmStatus!=NULL)
			    setMapInMaps(m,"lenv","status",pmStatus->value);
			}
			char* tmpStr=_getStatus(m,jobId);
			if(tmpStr!=NULL && strncmp(tmpStr,"-1",2)!=0){
			  char *tmpStr1=zStrdup(tmpStr);
			  char *tmpStr0=zStrdup(strstr(tmpStr,"|")+1);
			  free(tmpStr);
			  tmpStr1[strlen(tmpStr1)-strlen(tmpStr0)-1]='\0';
			  addToMap(statusInfo,"PercentCompleted",tmpStr1);
			  addToMap(statusInfo,"Message",tmpStr0);
			  setMapInMaps(m,"lenv","PercentCompleted",tmpStr1);
			  setMapInMaps(m,"lenv","Message",tmpStr0);
			  free(tmpStr0);
			  free(tmpStr1);
			}
			map* error=createMap("code","NoApplicableCode");
			addToMap(error,"message",_("The service failed to execute."));
			localPrintExceptionJ(&m,error);
			free(jobId);
			freeMap(&error);
			free(error);
			freeMaps(&m);
			free(m);
			json_object_put(res);
			free(pcaCgiQueryString);
			return 1;
		      }

		    }else{
		      free(Url0);
		      ensureFiltered(&m,"out");
		      runGetStatus(m,jobId,"GetResult");
		      free(jobId);
		      free(sid);
		      json_object_put(res);
		      free(pcaCgiQueryString);
		      map* pmTest=getMap(request_inputs,"shouldFree");
		      if(pmTest!=NULL){
			freeMap (inputs);
			free (*inputs);
			*inputs=NULL;
			freeMap(&r_inputs);
			free (r_inputs);
			r_inputs=NULL;
		      }
		      cleanUpSql(m);
		      freeMaps(&m);
		      free(m);
		      return 1;
		    }
		  }
		}


	      }
	      free(jobId);
	    }
	  }
	}

      }
    }
    else{
      service* s1=NULL;
      int t=0;
      if(strstr(pcaCgiQueryString,"/processes")==NULL && strstr(pcaCgiQueryString,"/processes/")==NULL){
	map* error=createMap("code","BadRequest");
	addToMap(error,"message",_("The resource is not available"));
	//setMapInMaps(conf,"lenv","status_code","404 Bad Request");
	localPrintExceptionJ(&m,error);
	freeMaps (&m);
	free (m);
	free (REQUEST);
	json_object_put(res);
	freeMap (inputs);
	free (*inputs);
	*inputs=NULL;
	freeMap (&error);
	free (error);
	free(pcaCgiQueryString);
	xmlCleanupParser ();
	zooXmlCleanupNs ();
	return 1;
      }
      else if(strcasecmp(pmCgiRequestMethod->value,"post")==0 ){
	/* - /processes/{processId}/execution Execution (POST) */
	eres = SERVICE_STARTED;
	map* req=getMapFromMaps(m,"renv","jrequest");
	bool bBypassed=false;
	if(req==NULL){
	  map* error=createMap("code","BadRequest");
	  addToMap(error,"message",_("The request body is empty"));
	  setMapInMaps(m,"lenv","status_code","400 Bad Request");
	  localPrintExceptionJ(&m,error);
	  freeMaps (&m);
	  free (m);
	  free (REQUEST);
	  json_object_put(res);
	  freeMap (inputs);
	  free (*inputs);
	  *inputs=NULL;
	  freeMap (&error);
	  free (error);
	  free(pcaCgiQueryString);
	  xmlCleanupParser ();
	  zooXmlCleanupNs ();
	  return 1;
	}
	setMapInMaps(m,"request","Content-Type",cgiContentType);
#ifdef DRU_ENABLED
	json_object *jobj = convertCwlToOGCAppPkg(m,request_inputs);
#else
	json_object *jobj = NULL;
#endif
	if(jobj==NULL){
	  const char *mystring = NULL;
	  int slen = 0;
	  enum json_tokener_error jerr;
	  struct json_tokener* tok=json_tokener_new();
	  do {
	    mystring = req->value;
	    slen = strlen(mystring);
	    jobj = json_tokener_parse_ex(tok, mystring, slen);
	  } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
	  if (jerr != json_tokener_success) {
	    map* pamError=createMap("code","InvalidParameterValue");
	    const char* pcTmpErr=json_tokener_error_desc(jerr);
	    const char* pccErr=_("ZOO-Kernel cannot parse your POST data: %s");
	    char* pacMessage=(char*)malloc((strlen(pcTmpErr)+strlen(pccErr)+1)*sizeof(char));
	    sprintf(pacMessage,pccErr,pcTmpErr);
	    addToMap(pamError,"message",pacMessage);
	    localPrintExceptionJ(&m,pamError);
	    fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
	    json_tokener_free(tok);
	    return 1;
	  }
	  if (tok->char_offset < slen){
	    map* pamError=createMap("code","InvalidParameterValue");
	    const char* pcTmpErr="None";
	    const char* pccErr=_("ZOO-Kernel cannot parse your POST data: %s");
	    char* pacMessage=(char*)malloc((strlen(pcTmpErr)+strlen(pccErr)+1)*sizeof(char));
	    sprintf(pacMessage,pccErr,pcTmpErr);
	    addToMap(pamError,"message",pacMessage);
	    localPrintExceptionJ(&m,pamError);
	    fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
	    json_tokener_free(tok);
	    return 1;
	  }
	  json_tokener_free(tok);
	}
#ifdef DRU_ENABLED
	convertOGCAppPkgToExecute(m,request_inputs,&jobj);
#endif
	char* cIdentifier=NULL;
	map* preference=getMapFromMaps(m,"renv","HTTP_PREFER");
	map* mode=getMap(request_inputs,"mode");
	json_object* json_io=NULL;
	if(!jsonIsFalse(json_object_object_get_ex(jobj,"id",&json_io))){
	  cIdentifier=zStrdup(json_object_get_string(json_io));
	}
	else if(strstr(pcaCgiQueryString,"/processes/")!=NULL && strlen(pcaCgiQueryString)>11){
	  cIdentifier=zStrdup(strstr(pcaCgiQueryString,"/processes/")+11);
	  if(strstr(cIdentifier,"execution")!=NULL)
	    cIdentifier[strlen(cIdentifier)-10]=0;
	}
#ifdef DRU_ENABLED
	else if( deployServiceProvider!=NULL ) {
          cIdentifier = deployServiceProvider->value;
	}
	map* pmIsDeployed=getMapFromMaps(m,"lenv","isDeployed");
	bool isAlreadyDeployed=false;
	if(pmIsDeployed!=NULL && strncasecmp(pmIsDeployed->value,"true",4)==0){
	  eres=SERVICE_DEPLOYED;
	  isAlreadyDeployed=true;
	  bBypassed=true;
	  goto printDescription;
	}
#endif
	if(cIdentifier!=NULL)
	  addToMap(request_inputs,"Identifier",cIdentifier);
	if(fetchService(zooRegistry,m,&s1,request_inputs,conf_dir_,cIdentifier,localPrintExceptionJ)!=0){
	  // TODO: cleanup memory
	  register_signals(donothing);
	  freeService(&s1);
	  free(s1);
	  freeMaps(&m);
	  free(m);
	  free(REQUEST);
	  json_object_put(res);
	  freeMap(inputs);
	  free(*inputs);
	  *inputs=NULL;
	  free(pcaCgiQueryString);
	  xmlCleanupParser();
	  zooXmlCleanupNs();
	  return -1;
	}
	free(cIdentifier);
	if((preference!=NULL && strstr(preference->value,"respond-async")!=NULL) ||
	    (mode!=NULL && strncasecmp(mode->value,"async",5)==0))
	  setMapInMaps(m,"lenv","execute_mode","async");
	parseJRequest(m,s1,jobj,request_inputs,&request_input_real_format,&request_output_real_format);
	json_object_put(jobj);
#ifdef DRU_ENABLED
	if(s1!= NULL && s1->content!=NULL){
	  map* pmOpenEOGraph=getMap(s1->content,"serviceSubType");
	  if(pmOpenEOGraph!=NULL){
	    // Only support OpenEO UDP
	    map* pmPwd=getMapFromMaps(m,"auth_env","cwd");
	    fflush(stderr);
	    if(pmPwd!=NULL){
	      // Add a graph extra input and update the serviceProvider
	      char *pcaReference=(char*)malloc((strlen(pmPwd->value)+strlen(s1->name)+13)*sizeof(char));
	      sprintf(pcaReference,"%s/%s_graph.json",pmPwd->value,s1->name);
	      maps *pmaOpenEo=createMaps("OpenEOGraph");
	      pmaOpenEo->content=createMap("cache_file",pcaReference);
	      free(pcaReference);
	      addToMap(pmaOpenEo->content,"mimeType","application/openeo");
	      addMapsToMaps(&request_input_real_format,pmaOpenEo);
	      freeMaps(&pmaOpenEo);
	      free(pmaOpenEo);
	      // Update the service serviceProvider here
	      map* pmServiceProvider=getMap(s1->content,"serviceProvider");
	      free(pmServiceProvider->value);
	      pmServiceProvider->value=zStrdup("OpenEO");
	      free(s1->name);
	      s1->name=zStrdup("run");
	    }
	  }
	}
#endif
	if((preference!=NULL && strstr(preference->value,"respond-async")!=NULL) ||
	   (mode!=NULL && strncasecmp(mode->value,"async",5)==0)) {
	  if(mode==NULL){
	    setMapInMaps(m,"request","mode","async");
	    addToMap(request_inputs,"mode","async");
	    mode=getMap(request_inputs,"mode");
	  }
#ifdef DEBUG
	  fprintf (stderr, "\nPID : %d\n", cpid);
#endif
	  //setMapInMaps(m,"lenv","no-headers","true");
#ifdef USE_AMQP
	  publish_amqp_msg(m,&eres,request_inputs,request_input_real_format,request_output_real_format);
#ifdef RELY_ON_DB
	  init_sql(m);
	  recordServiceStatus(m);
#endif
#ifdef USE_CALLBACK
	  invokeCallback(m,NULL,NULL,0,0);
#endif

	  eres = SERVICE_ACCEPTED;
	  createStatusFile(m,eres);
	  if(preference!=NULL)
	    setMapInMaps(m,"headers","Preference-Applied",preference->value);
	  setMapInMaps(m,"headers","Status","201 Created");
	  map* pmTmp=getMapFromMaps(m,"lenv","usid");
	  if(pmTmp!=NULL){
	    if(res!=NULL)
	      json_object_put(res);
	    res=printJobStatus(m,pmTmp->value);
	  }
#else

#ifndef WIN32
	  int pid = fork ();
#else
	  if (cgiSid == NULL)
	    {
	      createProcess (m, request_inputs, s1, NULL, cpid,
			     request_input_real_format,
			     request_output_real_format);
	      pid = cpid;
	    }
	  else
	    {
	      pid = 0;
	      cpid = atoi (cgiSid);
	      updateStatus(m,0,_("Initializing"));
	    }
#endif
	  if (pid > 0)
	    {

	      //
	      // dady :
	      // set status to SERVICE_ACCEPTED
	      //
#ifdef DEBUG
	      fprintf (stderr, "father pid continue (origin %d) %d ...\n", cpid,
		       zGetpid ());
#endif
	      eres = SERVICE_ACCEPTED;
	      createStatusFile(m,eres);
	      map* pmUsid = getMapFromMaps (m, "lenv", "usid");
	      map* pmOsid = getMapFromMaps (m, "lenv", "osid");
	      map* pmTmpPath = getMapFromMaps (m, "main", "tmpPath");

	      createSPidFile(m,pmTmpPath->value);
	      createLenvFile(m,pmTmpPath->value,pmUsid->value);

	      if(preference!=NULL)
		setMapInMaps(m,"headers","Preference-Applied",preference->value);
	      //invokeBasicCallback(m,SERVICE_ACCEPTED);
	      setMapInMaps(m,"headers","Status","201 Created");
	      map* pmTmp=getMapFromMaps(m,"lenv","usid");
	      if(pmTmp!=NULL){
		if(res!=NULL)
		  json_object_put(res);
		res=printJobStatus(m,pmTmp->value);
	      }
	    }
	  else if (pid == 0)
	    {
	      eres = SERVICE_STARTED;
	      //
	      // son : have to close the stdout, stdin and stderr to let the parent
	      // process answer to http client.
	      //
	      map* oid = getMapFromMaps (m, "lenv", "oIdentifier");
	      map* usid = getMapFromMaps (m, "lenv", "uusid");
	      map* tmpm = getMapFromMaps (m, "lenv", "osid");
	      int cpid = atoi (tmpm->value);
	      pid=cpid;
	      r_inputs = getMapFromMaps (m, "main", "tmpPath");
	      setMapInMaps (m, "lenv", "async","true");
	      map* r_inputs1 = createMap("ServiceName", s1->name);
	      // Create the filename for the result file (.res)
	      fbkpres =
		(char *)
		malloc ((strlen (r_inputs->value) +
			 strlen (usid->value) + 7) * sizeof (char));
	      sprintf (fbkpres, "%s/%s.res", r_inputs->value, usid->value);
	      bmap = createMaps("status");
	      bmap->content=createMap("usid",usid->value);
	      addToMap(bmap->content,"sid",tmpm->value);
	      addIntToMap(bmap->content,"pid",zGetpid());

	      createSPidFile(m,r_inputs->value);

	      fbkp =
		(char *)
		malloc ((strlen (r_inputs->value) +
			 strlen (usid->value) + 7) * sizeof (char));
	      sprintf (fbkp, "%s/%s.json", r_inputs->value,
		       usid->value);
	      setMapInMaps (m, "lenv", "file.responseInit", fbkp);
#ifdef DEBUG
	      fprintf (stderr, "RUN IN BACKGROUND MODE \n");
	      fprintf (stderr, "son pid continue (origin %d) %d ...\n", cpid,
		       zGetpid ());
	      fprintf (stderr, "\nFILE TO STORE DATA %s\n", r_inputs->value);
#endif

#ifndef LOG_CONSOLE_ENABLED
	      flog =
		(char *)
		malloc ((strlen (r_inputs->value) + strlen (oid->value) +
			 strlen (usid->value) + 13) * sizeof (char));
	      sprintf (flog, "%s/%s_%s_error.log", r_inputs->value,
		       oid->value, usid->value);
	      setMapInMaps (m, "lenv", "file.log", flog);
	      freopen (flog, "w+", stderr);
	      fflush (stderr);
#endif
	      f0 = freopen (fbkp, "w+", stdout);
	      rewind (stdout);
#ifndef WIN32
	      fclose (stdin);
#endif
#ifdef RELY_ON_DB
	      init_sql(m);
	      recordServiceStatus(m);
#endif
#ifdef USE_CALLBACK
	      invokeCallback(m,NULL,NULL,0,0);
#endif
	      invokeBasicCallback(m,SERVICE_STARTED);
	      createStatusFile(m,SERVICE_STARTED);
	      fbkp1 =
		(char *)
		malloc ((strlen (r_inputs->value) + strlen (oid->value) +
			 strlen (usid->value) + 15) * sizeof (char));
	      sprintf (fbkp1, "%s/%s_final_%s.json", r_inputs->value,
		       oid->value, usid->value);
	      setMapInMaps (m, "lenv", "file.responseFinal", fbkp1);

	      f1 = freopen (fbkp1, "w+", stdout);

	      map* serviceTypeMap=getMap(s1->content,"serviceType");
	      if(serviceTypeMap!=NULL)
		setMapInMaps (m, "lenv", "serviceType", serviceTypeMap->value);

	      createLenvFile(m,r_inputs->value,usid->value);

	      map* testMap=getMapFromMaps(m,"main","memory");
	      loadHttpRequests(m,request_input_real_format);

	      if(validateRequest(&m,s1,request_inputs, &request_input_real_format,&request_output_real_format,NULL)<0){
			freeMaps(&m);
			free(m);
			freeMaps (&request_input_real_format);
			free (request_input_real_format);
			freeMaps (&request_output_real_format);
			free (request_output_real_format);
			json_object_put(res);
			free(pcaCgiQueryString);
			freeService (&s1);
			free(s1);
			return -1;
		  }
	      loadServiceAndRun (&m, s1, request_inputs,
				 &request_input_real_format,
				 &request_output_real_format, &eres);
	      setMapInMaps(m,"lenv","force","true");
	      createStatusFile(m,eres);
	      setMapInMaps(m,"lenv","force","false");
	      setMapInMaps(m,"lenv","no-headers","true");
	      fflush(stdout);
	      rewind(stdout);
	      if(res!=NULL)
		json_object_put(res);
	      res=printJResult(m,s1,request_output_real_format,eres);
	      const char* jsonStr0=json_object_to_json_string_ext(res,JSON_C_TO_STRING_NOSLASHESCAPE);
	      if(getMapFromMaps(m,"lenv","jsonStr")==NULL)
		setMapInMaps(m,"lenv","jsonStr",jsonStr0);
	      invokeBasicCallback(m,eres);
#ifdef DRU_ENABLED
	      // Fetch infromations from main.cfg again after execution
	      deployServiceProvider=getMapFromMaps(m,"servicesNamespace","deploy_service_provider");
	      undeployServiceProvider=getMapFromMaps(m,"servicesNamespace","undeploy_service_provider");
#endif
	    }
#endif
	}else{
	  // request is synchronous
	  map* pmMutable=getMap(s1->content,"mutable");
	  if(pmMutable!=NULL && strncasecmp(pmMutable->value,"true",4)==0){
	    map* pmError=createMap("code","None");
	    addToMap(pmError,"message",_("The synchronous mode is not allowed for mutable services."));
	    localPrintExceptionJ(&m,pmError);
	    freeMap(&pmError);
	    free(pmError);
	    freeMaps(&m);
	    free(m);
	    freeMaps (&request_input_real_format);
	    free (request_input_real_format);
	    freeMaps (&request_output_real_format);
	    free (request_output_real_format);
	    json_object_put(res);
	    free(pcaCgiQueryString);
	    freeService (&s1);
	    free(s1);
	    return 1;
	  }
	  loadHttpRequests(m,request_input_real_format);
	  if(validateRequest(&m,s1,request_inputs, &request_input_real_format,&request_output_real_format,NULL)<0){
	    freeMaps(&m);
	    free(m);
	    freeMaps (&request_input_real_format);
	    free (request_input_real_format);
	    freeMaps (&request_output_real_format);
	    free (request_output_real_format);
	    json_object_put(res);
	    free(pcaCgiQueryString);
	    freeService (&s1);
	    free(s1);
	    return -1;
	  }
	  loadServiceAndRun (&m,s1,request_inputs,
			     &request_input_real_format,
			     &request_output_real_format,&eres);
	  if(res!=NULL){
	    json_object_put(res);
	    res=NULL;
	  }

#ifdef DRU_ENABLED
	  // Fetch infromations from main.cfg again after execution
	  deployServiceProvider=getMapFromMaps(m,"servicesNamespace","deploy_service_provider");
	  undeployServiceProvider=getMapFromMaps(m,"servicesNamespace","undeploy_service_provider");
	  bool bIsDeploy=(deployServiceProvider!=NULL &&
			  strcmp(deployServiceProvider->value,s1->name)==0);
	  bool bIsUndeploy=(undeployServiceProvider!=NULL &&
			    strcmp(undeployServiceProvider->value,s1->name)==0);
	  if(eres!=SERVICE_DEPLOYED && eres!=SERVICE_UNDEPLOYED &&
	     ( bIsDeploy || bIsUndeploy) ){
	    // Use the correct type DuplicatedProcess
	    // http://www.opengis.net/def/exceptions/ogcapi-processes-2/1.0/duplicated-process
	    // And set response status to 409 Conflict
	    map* pmError=NULL;
	    if(bIsDeploy){
	      setMapInMaps(m,"headers","Status","409 Conflict");
	      map* pmError=createMap("code","DuplicatedProcess");
	      addToMap(pmError,"message",_("A service with the same identifier is already deployed"));
	    }else{
	      pmError=createMap("code","InternalError");
	      addToMap(pmError,"message",_("An error occured when trying to undeploy your service."));
	    }
	    localPrintExceptionJ(&m,pmError);
	    setMapInMaps(m,"lenv","no-headers","true");
	    setMapInMaps(m,"lenv","hasPrinted","true");
	    // TODO: cleanup memory
	    freeMap(&pmError);
	    free(pmError);
	    freeMaps(&m);
	    free(m);
	    freeMaps (&request_input_real_format);
	    free (request_input_real_format);
	    freeMaps (&request_output_real_format);
	    free (request_output_real_format);
	    json_object_put(res);
	    free(pcaCgiQueryString);
	    freeService (&s1);
	    free(s1);
	    return 1;
	  }else
	    if(!bIsDeploy && !bIsUndeploy)
#endif
	      res=printJResult(m,s1,request_output_real_format,eres);
	  if(getMapFromMaps(m,"openapi","ensure_storing_result_every_execute")!=NULL)
	    setMapInMaps(m,"lenv","output_response","true");
	}
	freeService (&s1);
	free(s1);
#ifdef DRU_ENABLED
	if(deployServiceProvider!=NULL && undeployServiceProvider!=NULL){
	printDescription:
	  if(bBypassed){
	    // Fetch informations from main.cfg again after execution
	    deployServiceProvider=getMapFromMaps(m,"servicesNamespace","deploy_service_provider");
	    undeployServiceProvider=getMapFromMaps(m,"servicesNamespace","undeploy_service_provider");
	  }
	  map* pmTmp=getMap(request_inputs,"Identifier");
	  if(pmTmp==NULL)
	    pmTmp=getMapFromMaps(m,"lenv","Identifier");
	  if((eres==SERVICE_DEPLOYED || eres==SERVICE_UNDEPLOYED) && pmTmp!=NULL &&
	     (strcmp(pmTmp->value,deployServiceProvider->value)==0 || strcmp(pmTmp->value,undeployServiceProvider->value)==0) ) {
	    map* pmDeployed=getMapFromMaps(m,"lenv","deployedServiceId");
	    map* pmToDeploy=getMapFromMaps(m,"lenv","toDeploy");
	    map* pmRootUrl=getMapFromMaps(m,"openapi","rootUrl");
	    if(pmToDeploy!=NULL && pmDeployed!=NULL &&
	       strcmp(pmToDeploy->value,pmDeployed->value)!=0){
	      char newPath[1024];
	      getServicesNamespacePath(m,ntmp,newPath,1024);
	      char* pcaFileName=(char*)malloc((strlen(newPath)+strlen(pmDeployed->value)+7)*sizeof(char));
	      sprintf(pcaFileName,"%s/%s.zcfg",newPath,pmDeployed->value);
	      unlink(pcaFileName);
	      free(pcaFileName);
	      pcaFileName=(char*)malloc((strlen(newPath)+strlen(pmDeployed->value)+7)*sizeof(char));
	      sprintf(pcaFileName,"%s/%s.json",newPath,pmDeployed->value);
	      unlink(pcaFileName);
	      free(pcaFileName);
	      map* pmError=createMap("code","NoSuchProcess");
	      addToMap(pmError,"message",_("The process failed to be updated."));
	      localPrintExceptionJ(&m,pmError);
	      pmDeployed=NULL;
	    }
	    if(pmDeployed!=NULL){
	      char newPath[1024];
	      getServicesNamespacePath(m,ntmp,newPath,1024);
	      char* pcaFileName=(char*)malloc((strlen(newPath)+strlen(pmDeployed->value)+7)*sizeof(char));
	      map* pmContentType=getMapFromMaps(m,"request","Content-Type");
	      if(pmContentType!=NULL && strstr(pmContentType->value,"json")!=NULL)
		sprintf(pcaFileName,"%s/%s.json",newPath,pmDeployed->value);
	      else
		sprintf(pcaFileName,"%s/%s.cwl",newPath,pmDeployed->value);
	      if(strcmp(pmTmp->value,deployServiceProvider->value)==0){
#ifdef USE_AMQP
		map* pmIsDeployed=getMapFromMaps(m,"lenv","isDeployed");
		if(pmIsDeployed==NULL || strncasecmp(pmIsDeployed->value,"true",4)!=0){
		  setMapInMaps(m,"lenv","noRunSql","true");
		  maps* pmsOutputsBis=NULL;
		  maps* pmsItem=request_output_real_format;
		  while(pmsItem!=NULL){
		    maps* pmsCurrent=createMaps(pmsItem->name);
		    map* pmContent=pmsItem->content;
		    while(pmContent!=NULL){
		      if(strcasecmp(pmContent->name,"value")!=0 &&
			 strcasecmp(pmContent->name,"size")!=0 )
			if(pmsCurrent->content==NULL)
			  pmsCurrent->content=createMap(pmContent->name,pmContent->value);
			else
			  addToMap(pmsCurrent->content,pmContent->name,pmContent->value);
		      pmContent=pmContent->next;
		    }
		    pmsItem=pmsItem->next;
		  }
		  publish_amqp_msg(m,&eres,request_inputs,request_input_real_format,pmsOutputsBis);
		  freeMaps(&pmsOutputsBis);
		  free(pmsOutputsBis);
		}
#endif
		setMapInMaps(m,"lenv","no-headers","true");
		FILE*  pfRequest=fopen(pcaFileName,"wb+");
		if(pfRequest!=NULL){
		  map* pmJRequest=getMap(request_inputs,"jrequest");
		  if(pmJRequest!=NULL)
		    fwrite(pmJRequest->value,1,strlen(pmJRequest->value),pfRequest);
		  fclose(pfRequest);
		}
		setMapInMaps(m,"lenv","requestType","GetCapabilities");
		setMapInMaps(m,"lenv","Identifier",pmDeployed->value);
		setMapInMaps(m,"lenv","oIdentifier",pmDeployed->value);
		json_object *res3=json_object_new_object();
		json_object *res4=json_object_new_array();
		int t=fetchServicesForDescription(NULL, &m, pmDeployed->value,
						  printGetCapabilitiesForProcessJ,
						  (void*) res4, (void*) res3, ntmp,
						  request_inputs,
						  localPrintExceptionJ);
		json_object *res5=json_object_array_get_idx(res4,0);
		const char* jsonStr=json_object_to_json_string_ext(res5,JSON_C_TO_STRING_NOSLASHESCAPE);
		setMapInMaps(m,"lenv","no-headers","false");
		// In case security is activated, then execute the security module
		/*if(*/ensureFiltered(&m,"out");/*!=0){
		  maps* pmsTmp=getMaps(m,"lenv");
		  printExceptionReportResponseJ(m,pmsTmp->content);
		  // TODO: cleanup memory
		  freeMaps(&m);
		  free(m);
		  free (REQUEST);
		  map* pmTest=getMap(request_inputs,"shouldFree");
		  if(pmTest!=NULL){
		    freeMap (inputs);
		    free (*inputs);
		    *inputs=NULL;
		    freeMap(&r_inputs);
		    free (r_inputs);
		    r_inputs=NULL;
		  }
		  free(pcaCgiQueryString);
		  return 1;
		  }*/
		map* pmORequestMethod=getMapFromMaps(m,"lenv","orequest_method");
		if(pmORequestMethod!=NULL && strncasecmp(pmORequestMethod->value,"put",3)==0)
		  setMapInMaps(m,"headers","Status","204 No Content");
		// Refresh pmRootUrl and pmDeployed from the m maps pmDeployed
		// by the ensureFiltered invocation
		maps* pmsTmp1=getMaps(m,"openapi");
		pmRootUrl=getMap(pmsTmp1->content,"rootUrl");
		pmsTmp1=getMaps(m,"lenv");
		pmDeployed=getMap(pmsTmp1->content,"deployedServiceId");
		// Go back to normal handling
		char* pcaLocation=(char*)malloc((strlen(pmRootUrl->value)+strlen(pmDeployed->value)+12)*sizeof(char));
		sprintf(pcaLocation,"%s/processes/%s",pmRootUrl->value,pmDeployed->value);
		setMapInMaps(m,"headers","Location",pcaLocation);
		free(pcaLocation);
		if(pmORequestMethod!=NULL && strncasecmp(pmORequestMethod->value,"put",3)==0){
		  setMapInMaps(m,"headers","Status","204 No Content");
		}
		else{
		  setMapInMaps(m,"headers","Status","201 Created");
		}
		printHeaders(m);
		if(pmORequestMethod!=NULL && strncasecmp(pmORequestMethod->value,"put",3)==0){
		}
		else{
		  printf(jsonStr);
		  printf("\n");
		  fflush(stdout);
		}
		fflush(stdout);
		json_object_put(res3);
		json_object_put(res4);
		//json_object_put(res5);
		setMapInMaps(m,"lenv","no-headers","true");
		setMapInMaps(m,"lenv","hasPrinted","true");
		//if(res!=NULL)
		//  json_object_put(res);
		res=NULL;
	      }else
		if(pmTmp!=NULL && strcmp(pmTmp->value,undeployServiceProvider->value)==0){
		  unlink(pcaFileName);
		  setMapInMaps(m,"headers","Status","204 No Content");
		  ensureFiltered(&m,"out");
		  printHeaders(m);
		  setMapInMaps(m,"lenv","hasPrinted","true");
		  res=NULL;
		}
	      free(pcaFileName);
	    }else{
	      ensureFiltered(&m,"out");
	      handleDRUError(m);
	      fflush(stdout);
	    }
	  }else{
	    if(pmTmp!=NULL && (strcmp(pmTmp->value,deployServiceProvider->value)==0 || strcmp(pmTmp->value,undeployServiceProvider->value)==0) ){
	      handleDRUError(m);
	      res=NULL;
	    }
	  }
	}
#endif
      }//else error
      else
	if(strstr(pcaCgiQueryString,"/jobs")==NULL && 
	   (strstr(pcaCgiQueryString,"/processes/")+11)!=NULL){
	  /* - /processes/{processId}/ */
	  //DIR *dirp = opendir (ntmp);
	  json_object *res3=json_object_new_object();
	  char *orig = NULL;
	  orig = zStrdup (strstr(pcaCgiQueryString,"/processes/")+11);
	  if(orig[strlen(orig)-1]=='/')
	    orig[strlen(orig)-1]=0;
	  setMapInMaps(m,"lenv","requestType","GetCapabilities");
	  int t=fetchServicesForDescription(NULL, &m, orig,
					    printGetCapabilitiesForProcessJ,
					    NULL, (void*) res3, conf_dir_,
					    request_inputs,
					    localPrintExceptionJ);
	  if(t==1){
	    json_object_put(res);
	    json_object_put(res3);
	    free(orig);
	    free(pcaCgiQueryString);
	    return 1;
	  }
	  json_object_put(res);
	  res=json_object_get(res3);
	  free(orig);
	  json_object_put(res3);
	}else{
	  char* cIdentifier=NULL;
	  if(strstr(pcaCgiQueryString,"/processes/")!=NULL){

	    int len0=strlen(strstr(pcaCgiQueryString,"/processes/")+11);
	    int len1=strlen(pcaCgiQueryString)-strlen(strstr(pcaCgiQueryString,"/job"));
	    cIdentifier=(char*)malloc((len1-10)*sizeof(char));
	    int cnt=0;
	    for(int j=11;j<len1;j++){
	      cIdentifier[cnt]=pcaCgiQueryString[j];
	      cIdentifier[cnt+1]=0;
	      cnt++;
	    }
	    fetchService(zooRegistry,m,&s1,
			 request_inputs,conf_dir_,cIdentifier,localPrintExceptionJ);

	  }
	}

    }
    if(res!=NULL){
        const char* pccResult=json_object_to_json_string_ext(res,JSON_C_TO_STRING_NOSLASHESCAPE);
        setMapInMaps(m,"lenv","json_response_object",pccResult);
    }
    ensureFiltered(&m,"out");
    map* pmHasPrinted=getMapFromMaps(m,"lenv","hasPrinted");
    if(res!=NULL && (pmHasPrinted==NULL || strncasecmp(pmHasPrinted->value,"false",5)==0)){
      if((pmHasPrinted=getMapFromMaps(m,"lenv","no-headers"))==NULL ||  strncasecmp(pmHasPrinted->value,"false",5)==0) {
        printHeaders(m);
        if(getMapFromMaps(m,"headers","Status")==NULL)
          if (eres == 7){
              printf("Status: 204 NoContent \r\n\r\n");
          } else {
              printf("Status: 200 OK \r\n\r\n");
          }
      }
      map* pmResponseObject=getMapFromMaps(m,"lenv","json_response_object");
      if(pmResponseObject!=NULL){
	if(getMapFromMaps(m,"lenv","output_response")!=NULL) {
	  map* pmTmpPath=getMapFromMaps(m,"main","tmpPath");
	  map* pmUuid=getMapFromMaps(m,"lenv","usid");
	  if(pmTmpPath!=NULL && pmUuid!=NULL){

	    // Create the filename for the result file (.res)
	    map* pmSid=getMapFromMaps(m,"lenv","sid");
	    fbkpres =
	      (char *)
	      malloc ((strlen (pmTmpPath->value) +
		       strlen (pmUuid->value) + 7) * sizeof (char));
	    sprintf (fbkpres, "%s/%s.res", pmTmpPath->value, pmUuid->value);
	    bmap = createMaps("status");
	    bmap->content=createMap("usid",pmUuid->value);
	    addToMap(bmap->content,"sid",pmSid->value);
	    addIntToMap(bmap->content,"pid",zGetpid());
	    switch(eres){
	    default:
	    case SERVICE_FAILED:
	      setMapInMaps(bmap,"status","status",wpsStatus[1]);
	      setMapInMaps(m,"lenv","fstate",wpsStatus[1]);
	      break;
	    case SERVICE_SUCCEEDED:
	      setMapInMaps(bmap,"status","status",wpsStatus[0]);
	      setMapInMaps(m,"lenv","fstate",wpsStatus[0]);
	      break;
	    }
	    dumpMapsToFile(bmap,fbkpres,1);
	    free(fbkpres);

	    // Only required to produce the .sid file (pid is only required for
	    // runing services)
	    createXidFile(m,pmTmpPath->value,pmUuid->value,"sid",pmSid->value);
	    createLenvFile(m,pmTmpPath->value,pmUuid->value);

	    char* pcaPath =
	      (char *) malloc ((strlen (pmTmpPath->value) +
				strlen (pmUuid->value) + 7) * sizeof (char));
	    sprintf (pcaPath, "%s/%s.json", pmTmpPath->value, pmUuid->value);
	    FILE* pfResponse=fopen(pcaPath,"w");
	    if(pfResponse!=NULL){
	      fwrite ((void*)pmResponseObject->value, 1, strlen(pmResponseObject->value), pfResponse);
	      fclose(pfResponse);
	    }
	    pmTmpPath=getMapFromMaps(m,"main","tmpUrl");
	    free(pcaPath);
	    pcaPath =
	      (char *)
	      malloc ((strlen (pmTmpPath->value) +
		       strlen (pmUuid->value) + 14) * sizeof (char));
	    sprintf (pcaPath, "%s/%s_status.json",
		     pmTmpPath->value,pmUuid->value);
	    char* pcaLink =
	      (char *) malloc ((strlen (pcaPath) + 16) * sizeof (char));
	    sprintf(pcaLink,"%s; rel=monitor",pcaPath);
	    setMapInMaps(m,"headers","Link",pcaLink);
	    setMapInMaps(m,"headers","Status","200 OK");
	    free(pcaPath);
	    free(pcaLink);
	  }
	}
	printf(pmResponseObject->value);
	printf("\n");
	fflush(stdout);
	if(getMapFromMaps(m,"lenv","output_response")!=NULL){
	  createStatusFile(m,eres);
	}
      }
    }
    if(res!=NULL)
      json_object_put(res);
    free(pcaCgiQueryString);
    map* pmTest=getMap(request_inputs,"shouldFree");
    if(pmTest!=NULL){
      freeMap (inputs);
      free (*inputs);
      *inputs=NULL;
      freeMap(&r_inputs);
      free (r_inputs);
      r_inputs=NULL;
    }
    //return 1;
#endif
  }else{
    //
    // WPS 1.0.0 and 2.0.0 starts here
    //
    setMapInMaps(m,"main","executionType","xml");
    //Check for minimum inputs
    map* version=getMap(request_inputs,"version");
    if(version==NULL)
      version=getMapFromMaps(m,"main","version");
    setMapInMaps(m,"main","rversion",version->value);
    int vid=getVersionId(version->value);
    if(vid<0){
      if((version=getMap(request_inputs,"version"))!=NULL){
	char* pcaMessage=(char*) malloc((strlen(version->value)+41)*sizeof(char));
	sprintf(pcaMessage,"The value for version was not recogized %s",version->value);
	map* pmError=createMap("text",pcaMessage);
	addToMap(pmError,"locator","version");
	addToMap(pmError,"code","VersionNegotiationFailed");
	printExceptionReportResponse(&m,pmError);
	freeMap(&pmError);
	free(pmError);
	free(pcaMessage);
	return 0;
      }
      vid=0;
    }
    map* err=NULL;
    const char **vvr=(const char**)requests[vid];
    checkValidValue(request_inputs,&err,"request",vvr,1);
    const char *vvs[]={
      "WPS",
      NULL
    };
    if(err!=NULL){
      checkValidValue(request_inputs,&err,"service",(const char**)vvs,1);
      printExceptionReportResponse (&m, err);
      freeMap(&err);
      free(err);
      if (count (request_inputs) == 1)
	{
	  freeMap (&request_inputs);
	  free (request_inputs);
	}
      freeMaps (&m);
      free (m);
      return 1;
    }
    checkValidValue(request_inputs,&err,"service",(const char**)vvs,1);

    const char *vvv[]={
      "1.0.0",
      "2.0.0",
      NULL
    };
    r_inputs = getMap (request_inputs, "Request");
    if(r_inputs!=NULL)
      REQUEST = zStrdup (r_inputs->value);
    int reqId=-1;
    if (strncasecmp (REQUEST, "GetCapabilities", 15) != 0){
      checkValidValue(request_inputs,&err,"version",(const char**)vvv,1);
      int j=0;
      for(j=0;j<nbSupportedRequests;j++){
	if(requests[vid][j]!=NULL && requests[vid][j+1]!=NULL){
	  if(j<nbReqIdentifier && strncasecmp(REQUEST,requests[vid][j+1],strlen(requests[vid][j+1]))==0){
	    checkValidValue(request_inputs,&err,"identifier",NULL,1);
	    reqId=j+1;
	    break;
	  }
	  else
	    if(j>=nbReqIdentifier && j<nbReqIdentifier+nbReqJob && 
	       strncasecmp(REQUEST,requests[vid][j+1],strlen(requests[vid][j+1]))==0){
	      checkValidValue(request_inputs,&err,"jobid",NULL,1);
	      reqId=j+1;
	      break;
	    }
	}else
	  break;
      }
    }else{
      checkValidValue(request_inputs,&err,"AcceptVersions",(const char**)vvv,-1);
      map* version1=getMap(request_inputs,"AcceptVersions");
      if(version1!=NULL){
	if(strstr(version1->value,schemas[1][0])!=NULL){
	  addToMap(request_inputs,"version",schemas[1][0]);
	  setMapInMaps(m,"main","rversion",schemas[1][0]);
	}
	else{
	  addToMap(request_inputs,"version",version1->value);
	  setMapInMaps(m,"main","rversion",version1->value);
	}
	version=getMap(request_inputs,"version");
      }
    }
    if(err!=NULL){
      printExceptionReportResponse (&m, err);
      freeMap(&err);
      free(err);
      if (count (request_inputs) == 1)
	{
	  freeMap (&request_inputs);
	  free (request_inputs);
	}
      free(REQUEST);
      freeMaps (&m);
      free (m);
      return 1;
    }

    r_inputs = getMap (request_inputs, "serviceprovider");
    if (r_inputs == NULL)
      {
	addToMap (request_inputs, "serviceprovider", "");
      }

    map *tmpm = getMapFromMaps (m, "main", "serverAddress");
    if (tmpm != NULL)
      SERVICE_URL = zStrdup (tmpm->value);
    else
      SERVICE_URL = zStrdup (DEFAULT_SERVICE_URL);


    int scount = 0;
#ifdef DEBUG
    dumpMap (r_inputs);
#endif

    if (strncasecmp (REQUEST, "GetCapabilities", 15) == 0)
      {
#ifdef DEBUG
	dumpMap (r_inputs);
#endif
	xmlDocPtr doc = xmlNewDoc (BAD_CAST "1.0");
	xmlNodePtr n=printGetCapabilitiesHeader(doc,m,(version!=NULL?version->value:"1.0.0"));
	/**
	 * Here we need to close stdout to ensure that unsupported chars 
	 * has been found in the zcfg and then printed on stdout
	 */
	int saved_stdout = zDup (fileno (stdout));
	zDup2 (fileno (stderr), fileno (stdout));

	maps* imports = getMaps(m, IMPORTSERVICE);
	if (imports != NULL) {       
	  map* zcfg = imports->content;
        
	  while (zcfg != NULL) {
	    if (zcfg->value != NULL) {
	      service* svc = (service*) malloc(SERVICE_SIZE);
	      if (svc == NULL || readServiceFile(m, zcfg->value, &svc, zcfg->name) < 0) {
		// pass over silently
		zcfg = zcfg->next;
		continue;
	      }
	      inheritance(zooRegistry, &svc);
	      printGetCapabilitiesForProcess(zooRegistry, m, doc, n, svc);
	      freeService(&svc);
	      free(svc);                             
	    }
	    zcfg = zcfg->next;
	  }            
	}

	if (int res =		  
	    recursReaddirF (m, zooRegistry, doc, n, conf_dir, NULL, saved_stdout, 0,
			    printGetCapabilitiesForProcess) < 0)
	  {
	    freeMaps (&m);
	    free (m);
	    if(zooRegistry!=NULL){
	      freeRegistry(&zooRegistry);
	      free(zooRegistry);
	    }
	    free (REQUEST);
	    free (SERVICE_URL);
	    fflush (stdout);
	    return res;
	  }
	fflush (stdout);
	zDup2 (saved_stdout, fileno (stdout));
#ifdef META_DB
	fetchServicesFromDb(zooRegistry,m,doc,n,printGetCapabilitiesForProcess,1);
	close_sql(m,0);
#endif      
	printDocument (m, doc, zGetpid ());
	freeMaps (&m);
	free (m);
	if(zooRegistry!=NULL){
	  freeRegistry(&zooRegistry);
	  free(zooRegistry);
	}
	free (REQUEST);
	free (SERVICE_URL);
	fflush (stdout);
	return 0;
      }
    else
      {
	r_inputs = getMap (request_inputs, "JobId");
	if(reqId>nbReqIdentifier){
	  if (strncasecmp (REQUEST, "GetStatus", 9) == 0 ||
	      strncasecmp (REQUEST, "GetResult", 9) == 0){
	    runGetStatus(m,r_inputs->value,REQUEST);
	    cleanUpSql(m);
	    freeMaps (&m);
	    free(m);
	    if(zooRegistry!=NULL){
	      freeRegistry(&zooRegistry);
	      free(zooRegistry);
	    }
	    free (REQUEST);
	    free (SERVICE_URL);
	    return 0;
	  }
	  else
	    if (strncasecmp (REQUEST, "Dismiss", strlen(REQUEST)) == 0){
	      runDismiss(m,r_inputs->value);
	      freeMaps (&m);
	      free (m);
	      if(zooRegistry!=NULL){
		freeRegistry(&zooRegistry);
		free(zooRegistry);
	      }
	      free (REQUEST);
	      free (SERVICE_URL);
	      return 0;
	    
	    }
	  return 0;
	}
	if(reqId<=nbReqIdentifier){
	  r_inputs = getMap (request_inputs, "Identifier");

	  struct dirent *dp;
	  DIR *dirp = opendir (conf_dir);
	  if (dirp == NULL)
	    {
	      errorException (&m, _("The specified path does not exist."),
			      "InternalError", NULL);
	      freeMaps (&m);
	      free (m);
	      if(zooRegistry!=NULL){
		freeRegistry(&zooRegistry);
		free(zooRegistry);
	      }
	      free (REQUEST);
	      free (SERVICE_URL);
	      return 0;
	    }
	  if (strncasecmp (REQUEST, "DescribeProcess", 15) == 0)
	    {
	      /**
	       * Loop over Identifier list
	       */
	      xmlDocPtr doc = xmlNewDoc (BAD_CAST "1.0");
	      r_inputs = NULL;
	      r_inputs = getMap (request_inputs, "version");
#ifdef DEBUG
	      fprintf(stderr," ** DEBUG %s %d \n",__FILE__,__LINE__);
	      fflush(stderr);
#endif
	      xmlNodePtr n = printWPSHeader(doc,m,"DescribeProcess",
					    root_nodes[vid][1],(version!=NULL?version->value:"1.0.0"),1);

	      r_inputs = getMap (request_inputs, "Identifier");

	      int t=fetchServicesForDescription(zooRegistry, &m, r_inputs->value,
					  printDescribeProcessForProcess,
					  (void*) doc, (void*) n, conf_dir,
					  request_inputs,printExceptionReportResponse);
	      if(t!=0){
		xmlFreeDoc (doc);
		closedir (dirp);
		free (REQUEST);
		return 1;
	      }
	      printDocument (m, doc, zGetpid ());
	      freeMaps (&m);
	      free (m);
	      if(zooRegistry!=NULL){
		freeRegistry(&zooRegistry);
		free(zooRegistry);
	      }
	      free (REQUEST);
	      free (SERVICE_URL);
	      fflush (stdout);
	      closedir (dirp);
#ifdef META_DB
	      close_sql(m,0);
	      //end_sql();
#endif
	      return 0;
	    }
	  else if (strncasecmp (REQUEST, "Execute", strlen (REQUEST)) != 0)
	    {
	      map* version=getMapFromMaps(m,"main","rversion");
	      int vid=getVersionId(version->value);	    
	      int len = 0;
	      int j = 0;
	      for(j=0;j<nbSupportedRequests;j++){
		if(requests[vid][j]!=NULL)
		  len+=strlen(requests[vid][j])+2;
		else{
		  len+=4;
		  break;
		}
	      }
	      char *tmpStr=(char*)malloc(len*sizeof(char));
	      int it=0;
	      for(j=0;j<nbSupportedRequests;j++){
		if(requests[vid][j]!=NULL){
		  if(it==0){
		    sprintf(tmpStr,"%s",requests[vid][j]);
		    it++;
		  }else{
		    char *tmpS=zStrdup(tmpStr);
		    if(j+1<nbSupportedRequests && requests[vid][j+1]==NULL){
		      sprintf(tmpStr,"%s and %s",tmpS,requests[vid][j]);
		    }else{
		      sprintf(tmpStr,"%s, %s",tmpS,requests[vid][j]);
		  
		    }
		    free(tmpS);
		  }
		}
		else{
		  len+=4;
		  break;
		}
	      }
	      char* message=(char*)malloc((61+len)*sizeof(char));
	      sprintf(message,"The <request> value was not recognized. Allowed values are %s.",tmpStr);
	      errorException (&m,_(message),"InvalidParameterValue", "request");
#ifdef DEBUG
	      fprintf (stderr, "No request found %s", REQUEST);
#endif
	      closedir (dirp);
	      freeMaps (&m);
	      free (m);
	      if(zooRegistry!=NULL){
		freeRegistry(&zooRegistry);
		free(zooRegistry);
	      }
	      free (REQUEST);
	      free (SERVICE_URL);
	      fflush (stdout);
	      return 0;
	    }
	  closedir (dirp);
	}
      }

    map *postRequest = NULL;
    postRequest = getMap (request_inputs, "xrequest");

    if(vid==1 && postRequest==NULL){
      errorException (&m,_("Unable to run Execute request using the GET HTTP method"),"InvalidParameterValue", "request");
      freeMaps (&m);
      free (m);
      if(zooRegistry!=NULL){
	freeRegistry(&zooRegistry);
	free(zooRegistry);
      }
      free (REQUEST);
      free (SERVICE_URL);
      fflush (stdout);
      return 0;
    }
    s1 = NULL;

    r_inputs = getMap (request_inputs, "Identifier");

    if(fetchService(zooRegistry,m,&s1,request_inputs,conf_dir_,r_inputs->value,printExceptionReportResponse)!=0){
      // Service not found clear memory
      freeMaps(&m);
      free(m);
      if(zooRegistry!=NULL){
	freeRegistry(&zooRegistry);
	free(zooRegistry);
      }
      free (REQUEST);
      free (SERVICE_URL);
      return -1;
    }
  
#ifdef DEBUG
    dumpService (s1);
#endif
    int j;


    /**
     * Create the input and output maps data structure
     */
    int i = 0;
    HINTERNET res;
    hInternet = InternetOpen (
#ifndef WIN32
			      (LPCTSTR)
#endif
			      "ZooWPSClient\0",
			      INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

#ifndef WIN32
    if (!CHECK_INET_HANDLE (hInternet))
      fprintf (stderr, "WARNING : hInternet handle failed to initialize");
#endif
    maps *tmpmaps = request_input_real_format;

    if(parseRequest(&m,&request_inputs,s1,&request_input_real_format,&request_output_real_format,&hInternet)<0){
      freeMaps (&m);
      free (m);
      free (REQUEST);
      free (SERVICE_URL);
      InternetCloseHandle (&hInternet);
      freeService (&s1);
      free (s1);
      return 0;
    }
    //InternetCloseHandle (&hInternet);

    initAllEnvironment(m,request_inputs,ntmp,"xrequest");


#ifdef DEBUG
    dumpMap (request_inputs);
#endif

    map *status = getMap (request_inputs, "status");
    if(vid==0){
      // Need to check if we need to fork to load a status enabled 
      r_inputs = NULL;
      map *store = getMap (request_inputs, "storeExecuteResponse");
      /**
       * 05-007r7 WPS 1.0.0 page 57 :
       * 'If status="true" and storeExecuteResponse is "false" then the service 
       * shall raise an exception.'
       */
      if (status != NULL && strcmp (status->value, "true") == 0 &&
	  store != NULL && strcmp (store->value, "false") == 0)
	{
	  errorException (&m,
			  _
			  ("The status parameter cannot be set to true if storeExecuteResponse is set to false. Please modify your request parameters."),
			  "InvalidParameterValue", "storeExecuteResponse");
	  freeService (&s1);
	  free (s1);
	  freeMaps (&m);
	  free (m);

	  freeMaps (&request_input_real_format);
	  free (request_input_real_format);

	  freeMaps (&request_output_real_format);
	  free (request_output_real_format);

	  free (REQUEST);
	  free (SERVICE_URL);
	  return 1;
	}
      r_inputs = getMap (request_inputs, "storeExecuteResponse");
    }else{
      // Define status depending on the WPS 2.0.0 mode attribute
      status = getMap (request_inputs, "mode");
      map* mode=getMap(s1->content,"mode");
      if(strcasecmp(status->value,"async")==0){
	if(mode!=NULL && strcasecmp(mode->value,"async")==0)
	  addToMap(request_inputs,"status","true");
	else{
	  if(mode!=NULL){
	    // see ref. http://docs.opengeospatial.org/is/14-065/14-065.html#61
	    errorException (&m,_("The process does not permit the desired execution mode."),"NoSuchMode", mode->value);
	    fflush (stdout);
	    freeMaps (&m);
	    free (m);
	    if(zooRegistry!=NULL){
	      freeRegistry(&zooRegistry);
	      free(zooRegistry);
	    }
	    freeMaps (&request_input_real_format);
	    free (request_input_real_format);
	    freeMaps (&request_output_real_format);
	    free (request_output_real_format);
	    free (REQUEST);
	    free (SERVICE_URL);
	    return 0;
	  }else
	    addToMap(request_inputs,"status","true");
	}
      }
      else{
	if(strcasecmp(status->value,"auto")==0){
	  if(mode!=NULL){
	    if(strcasecmp(mode->value,"async")==0)
	      addToMap(request_inputs,"status","false");
	    else
	      addToMap(request_inputs,"status","true");
	  }
	  else
	    addToMap(request_inputs,"status","false");
	}else
	  addToMap(request_inputs,"status","false");
      }
      status = getMap (request_inputs, "status");
    }

    if (status != NULL)
      if (strcasecmp (status->value, "false") == 0)
	status = NULLMAP;
    if (status == NULLMAP)
      {
	if(validateRequest(&m,s1,request_inputs, &request_input_real_format,&request_output_real_format,&hInternet)<0){
	  freeService (&s1);
	  free (s1);
	  freeMaps (&m);
	  free (m);
	  free (REQUEST);
	  free (SERVICE_URL);
	  freeMaps (&request_input_real_format);
	  free (request_input_real_format);
	  freeMaps (&request_output_real_format);
	  free (request_output_real_format);
	  freeMaps (&tmpmaps);
	  free (tmpmaps);
	  return -1;
	}
	map* testMap=getMapFromMaps(m,"main","memory");
	if(testMap==NULL || strcasecmp(testMap->value,"load")!=0)
	  dumpMapsValuesToFiles(&m,&request_input_real_format);
	loadServiceAndRun (&m, s1, request_inputs, &request_input_real_format,
			   &request_output_real_format, &eres);	  

#ifdef META_DB
	close_sql(m,0);      
#endif      
      }
    else
      {
	int pid;
#ifdef DEBUG
	fprintf (stderr, "\nPID : %d\n", cpid);
#endif

#ifdef USE_AMQP

	publish_amqp_msg(m,&eres,request_inputs,request_input_real_format,request_output_real_format);

#ifdef RELY_ON_DB
	init_sql(m);
	recordServiceStatus(m);
#endif
	eres = SERVICE_ACCEPTED;

	map* usid = getMapFromMaps (m, "lenv", "uusid");
	int saved_stdout = zDup (fileno (stdout));
	map* r_inputs1 = createMap("ServiceName", s1->name);
	r_inputs = getMapFromMaps (m, "main", "tmpPath");
	fbkp =
	  (char *)
	  malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		   strlen (usid->value) + 7) * sizeof (char));
	sprintf (fbkp, "%s/%s_%s.xml", r_inputs->value, r_inputs1->value,
		 usid->value);

	f0 = freopen (fbkp, "w+", stdout);
#ifdef USE_CALLBACK
	invokeCallback(m,NULL,NULL,0,0);
#endif
	if(vid==0){
	  //
	  // set status to SERVICE_STARTED and flush stdout to ensure full
	  // content was outputed (the file used to store the ResponseDocument).
	  // Then, rewind stdout to restart writing from the begining of the file.
	  // This way, the data will be updated at the end of the process run.
	  //
	  printProcessResponse (m, request_inputs, -1, s1, r_inputs1->value,
				SERVICE_STARTED, request_input_real_format,
				request_output_real_format);
	  fflush (stdout);
	  recordResponse(m,fbkp);
	}
	zDup2 (saved_stdout, fileno (stdout));

#else

#ifndef WIN32
	pid = fork ();
#else
	if (cgiSid == NULL)
	  {
	    createProcess (m, request_inputs, s1, NULL, cpid,
			   request_input_real_format,
			   request_output_real_format);
	    pid = cpid;
	  }
	else
	  {
	    pid = 0;
	    cpid = atoi (cgiSid);
	    updateStatus(m,0,_("Initializing"));
	  }
#endif
	if (pid > 0)
	  {
	    //
	    // dady :
	    // set status to SERVICE_ACCEPTED
	    //
#ifdef DEBUG
	    fprintf (stderr, "father pid continue (origin %d) %d ...\n", cpid,
		     zGetpid ());
#endif
	    eres = SERVICE_ACCEPTED;
	  }
	else if (pid == 0)
	  {
	    eres = SERVICE_ACCEPTED;
	    //
	    // son : have to close the stdout, stdin and stderr to let the parent
	    // process answer to http client.
	    //
	    map* usid = getMapFromMaps (m, "lenv", "uusid");
	    map* tmpm = getMapFromMaps (m, "lenv", "osid");
	    int cpid = atoi (tmpm->value);
	    pid=cpid;
	    r_inputs = getMapFromMaps (m, "main", "tmpPath");
	    setMapInMaps (m, "lenv", "async","true");
	    map* r_inputs1 = createMap("ServiceName", s1->name);

	    // Create the filename for the result file (.res)
	    fbkpres =
	      (char *)
	      malloc ((strlen (r_inputs->value) +
		       strlen (usid->value) + 7) * sizeof (char));                    
	    sprintf (fbkpres, "%s/%s.res", r_inputs->value, usid->value);
	    bmap = createMaps("status");
	    bmap->content=createMap("usid",usid->value);
	    addToMap(bmap->content,"sid",tmpm->value);
	    addIntToMap(bmap->content,"pid",zGetpid());

	    createSPidFile(m,r_inputs->value);

	    fbkp =
	      (char *)
	      malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		       strlen (usid->value) + 8) * sizeof (char));
	    sprintf (fbkp, "%s/%s_%s.xml", r_inputs->value, r_inputs1->value,
		     usid->value);
	    setMapInMaps (m, "lenv", "file.responseInit", fbkp);

#ifdef DEBUG
	    fprintf (stderr, "RUN IN BACKGROUND MODE \n");
	    fprintf (stderr, "son pid continue (origin %d) %d ...\n", cpid,
		     zGetpid ());
	    fprintf (stderr, "\nFILE TO STORE DATA %s\n", r_inputs->value);
#endif
#ifndef LOG_CONSOLE_ENABLED
	    flog =
	      (char *)
	      malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		       strlen (usid->value) + 13) * sizeof (char));
	    sprintf (flog, "%s/%s_%s_error.log", r_inputs->value,
		     r_inputs1->value, usid->value);
	    setMapInMaps (m, "lenv", "file.log", flog);
	    freopen (flog, "w+", stderr);
	    fflush (stderr);
#endif
	    f0 = freopen (fbkp, "w+", stdout);
	    rewind (stdout);
#ifndef WIN32
	    fclose (stdin);
#endif
#ifdef RELY_ON_DB
	    init_sql(m);
	    recordServiceStatus(m);
#endif
#ifdef USE_CALLBACK
	    invokeCallback(m,NULL,NULL,0,0);
#endif
	    if(vid==0){
	      //
	      // set status to SERVICE_STARTED and flush stdout to ensure full 
	      // content was outputed (the file used to store the ResponseDocument).
	      // Then, rewind stdout to restart writing from the begining of the file.
	      // This way, the data will be updated at the end of the process run.
	      //
	      printProcessResponse (m, request_inputs, cpid, s1, r_inputs1->value,
				    SERVICE_STARTED, request_input_real_format,
				    request_output_real_format);
	      fflush (stdout);
#ifdef RELY_ON_DB
	      recordResponse(m,fbkp);
#endif
	    }

	    fflush (stderr);

	    fbkp1 =
	      (char *)
	      malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		       strlen (usid->value) + 13) * sizeof (char));
	    sprintf (fbkp1, "%s/%s_final_%s.xml", r_inputs->value,
		     r_inputs1->value, usid->value);
	    setMapInMaps (m, "lenv", "file.responseFinal", fbkp1);

	    f1 = freopen (fbkp1, "w+", stdout);

	    map* serviceTypeMap=getMap(s1->content,"serviceType");
	    if(serviceTypeMap!=NULL)
	      setMapInMaps (m, "lenv", "serviceType", serviceTypeMap->value);

	    createLenvFile(m,r_inputs->value,usid->value);
#ifdef USE_CALLBACK
	    invokeCallback(m,request_input_real_format,NULL,1,0);
#endif
	    if(validateRequest(&m,s1,request_inputs, &request_input_real_format,&request_output_real_format,&hInternet)<0){
	      freeService (&s1);
	      free (s1);
	      fflush (stdout);
	      fflush (stderr);
	      fclose (f0);
	      fclose (f1);
	      if(dumpBackFinalFile(m,fbkp,fbkp1)<0)
		return -1;
#ifndef RELY_ON_DB
	      dumpMapsToFile(bmap,fbkpres,1);
	      removeShmLock (m, 1);
#else
	      recordResponse(m,fbkp1);
#ifdef USE_CALLBACK
	      invokeCallback(m,NULL,NULL,7,0);
#endif
#endif
	      map *pmPid=getMapFromMaps(m,"lenv","file.pid");
	      if(pmPid!=NULL)
		zUnlink (pmPid->value);
	      unhandleStatus (m);
#ifdef RELY_ON_DB
#ifdef META_DB
	      cleanupCallbackThreads();
	      close_sql(m,1);
#endif
	      close_sql(m,0);
#endif
	      freeMaps (&m);
	      free (m);
	      free (REQUEST);
	      free (SERVICE_URL);
	      freeMaps (&request_input_real_format);
	      free (request_input_real_format);
	      freeMaps (&request_output_real_format);
	      free (request_output_real_format);
	      freeMaps (&tmpmaps);
	      free (tmpmaps);
	      return -1;
	    }
	    if(getMapFromMaps(m,"lenv","mapError")!=NULL){
	      setMapInMaps(m,"lenv","message",_("Issue with geographic data"));
#ifdef USE_CALLBACK
	      invokeCallback(m,NULL,NULL,7,0);
#endif
	      eres=-1;//SERVICE_FAILED;
	    }else{
	      map* testMap=getMapFromMaps(m,"main","memory");
	      if(testMap==NULL || strcasecmp(testMap->value,"load")!=0)
		dumpMapsValuesToFiles(&m,&request_input_real_format);
	      loadServiceAndRun (&m, s1, request_inputs,
				 &request_input_real_format,
				 &request_output_real_format, &eres);
	    }
	  }
	else
	  {
	    /**
	     * error server don't accept the process need to output a valid 
	     * error response here !!!
	     */
	    eres = -1;
	    errorException (&m, _("Unable to run the child process properly"),
			    "InternalError", NULL);
	  }
#endif
      }
	
#ifdef DEBUG
    fprintf (stderr, "RUN IN BACKGROUND MODE %s %d \n",__FILE__,__LINE__);
    dumpMaps (request_output_real_format);
    fprintf (stderr, "RUN IN BACKGROUND MODE %s %d \n",__FILE__,__LINE__);
#endif
    fflush(stdout);
    rewind(stdout);

    if (eres != -1){
      outputResponse (s1, request_input_real_format,
		      request_output_real_format, request_inputs,
		      cpid, m, eres);
	}
    fflush (stdout);
  }
  /**
   * Ensure that if error occurs when freeing memory, no signal will return
   * an ExceptionReport document as the result was already returned to the 
   * client.
   */
  register_signals(donothing);

  if (((int) zGetpid ()) != cpid || cgiSid != NULL)
    {
      if (eres == SERVICE_SUCCEEDED)
#ifdef USE_CALLBACK
	invokeCallback(m,NULL,request_output_real_format,5,1);
#endif
      fflush(stderr);
      fflush(stdout);

      fclose (stdout);

      fclose (f0);

      fclose (f1);

      if(dumpBackFinalFile(m,fbkp,fbkp1)<0)
	return -1;
      map *pmPid=getMapFromMaps(m,"lenv","file.pid");
      if(pmPid!=NULL)
	zUnlink (pmPid->value);
      switch(eres){
      default:
      case SERVICE_FAILED:
	setMapInMaps(bmap,"status","status",wpsStatus[1]);
	setMapInMaps(m,"lenv","fstate",wpsStatus[1]);
	break;
      case SERVICE_SUCCEEDED:
	setMapInMaps(bmap,"status","status",wpsStatus[0]);
	setMapInMaps(m,"lenv","fstate",wpsStatus[0]);
	break;
      }      
#ifndef RELY_ON_DB
      dumpMapsToFile(bmap,fbkpres,1);
      removeShmLock (m, 1);
#else
      recordResponse(m,fbkp1);
#ifdef USE_CALLBACK
      if (eres == SERVICE_SUCCEEDED)
	invokeCallback(m,NULL,request_output_real_format,6,0);
#endif
#endif
      freeMaps(&bmap);
      free(bmap);
      zUnlink (fbkp1);
      unhandleStatus (m);
#if defined(USE_JSON) || defined(USE_CALLBACK)
      cleanupCallbackThreads();
#endif

      cleanUpSql(m);
      free(fbkpres);
      free (fbkp1);
      if(cgiSid!=NULL)
	free(cgiSid);
      map* tMap=getMapFromMaps(m,"main","executionType");
      if(tMap!=NULL && strncasecmp(tMap->value,"xml",3)==0)
	InternetCloseHandle (&hInternet);
#ifdef DEBUG
      fprintf (stderr, "RUN IN BACKGROUND MODE %s %d \n",__FILE__,__LINE__);
#endif
#ifndef LOG_CONSOLE_ENABLED
      fflush(stderr);
      fclose (stderr);
      zUnlink (flog);
      free (flog);
#endif
    }
  else{
    //InternetCloseHandle (&hInternet);
    cleanUpSql(m);
  }

  if(s1!=NULL){
    freeService (&s1);
    free (s1);
  }
  freeMaps (&m);
  free (m);
  freeMaps (&request_input_real_format);
  free (request_input_real_format);

  freeMaps (&request_output_real_format);
  free (request_output_real_format);

  free (REQUEST);
  free (SERVICE_URL);

#ifdef DEBUG
  fprintf (stderr, "Processed response \n");
  fflush (stdout);
  fflush (stderr);
#endif

  if (((int) zGetpid ()) != cpid || cgiSid != NULL)
    {
      exit (0);
    }

  return 0;
}


#ifdef USE_AMQP
/**
 * Process the request asyncrhonously.
 *
 * @param conf the main configuration maps 
 * @param lenv the lenv map 
 * @param inputs the request parameters map 
 * @return 0 on sucess, other value on failure
 */
int
runAsyncRequest (maps** iconf, map ** lenv, map ** irequest_inputs,json_object *msg_obj)
{
  register_signals(sig_handler);
  maps* conf=*iconf;
  map* request_inputs=*irequest_inputs;
  map* dsNb=getMapFromMaps(conf,"lenv","ds_nb");
  int metadb_id=0;
  //if(dsNb==NULL || strncasecmp(dsNb->value,"-1",2)==0){
#ifdef META_DB
  metadb_id=_init_sql(conf,"metadb");
#endif
  int iSqlCon=init_sql(conf);
    //}
  map *uusid=getMap(*lenv,"usid");
  map *schema=getMapFromMaps(conf,"database","schema");

  char* sqlQueryTmp0="SELECT %s.checkAvailableExecutionSlot('%s','%s',%d);";
  char* sqlQuery0=(char*)malloc(((2*strlen(schema->value))+
				 strlen(uusid->value)+strlen(sqlQueryTmp0)+129)*sizeof(char));
  sprintf(sqlQuery0,sqlQueryTmp0,schema->value,schema->value,uusid->value,getpid());
  OGRLayer *res=fetchSql(conf,iSqlCon-1,sqlQuery0);
  free(sqlQuery0);
  if(res!=NULL){
    OGRFeature  *poFeature = NULL;
    const char *tmp1;
    poFeature = res->GetNextFeature();
    if( poFeature != NULL ){
      while( poFeature != NULL ){
	if(atoi(poFeature->GetFieldAsString( 0 ))==0){
	  fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	  fprintf(stderr,"No need to run the async request, it has been taken by another worker!\n");
	  fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	  fflush(stderr);
	}else{

	  int forkId=fork();
	  if(forkId==0){
	    char ntmp[1024];
#ifndef ETC_DIR
#ifndef WIN32
	    getcwd (ntmp, 1024);
#else
	    _getcwd (ntmp, 1024);
#endif
#else
	    sprintf(ntmp,"%s",ETC_DIR);
#endif
	    map* r_inputs = getMapOrFill (&request_inputs, "metapath", "");
	    setMapInMaps(conf,"lenv","usid",uusid->value);
	    setMapInMaps(conf,"lenv","uusid",uusid->value);

	    maps* lconf=dupMaps(iconf);
	    // Reset metapath
	    addToMap(request_inputs,"metapath","");
	    setMapInMaps(lconf,"lenv","metapath","");
	    setMapInMaps(lconf,"lenv","no-headers","true");
	    maps* pmsTmp=getMaps(lconf,"lenv");

	    // Define auth_env section in case we find fpm_user in the lenv
	    // map coming from the message received
	    map* pmUserEnv=getMap(*lenv,"fpm_user");
	    if(pmUserEnv!=NULL){
	      setMapInMaps(lconf,"env","SERVICES_NAMESPACE",pmUserEnv->value);
	      setMapInMaps(lconf,"zooServicesNamespace","namespace",pmUserEnv->value);
	      maps* pmsaUserEnv=createMaps("auth_env");
	      pmsaUserEnv->content=createMap("user",pmUserEnv->value);
	      pmUserEnv=getMap(*lenv,"fpm_cwd");
	      if(pmUserEnv!=NULL){
		addToMap(pmsaUserEnv->content,"cwd",pmUserEnv->value);
	      }
	      addMapsToMaps(&lconf,pmsaUserEnv);
	      freeMaps(&pmsaUserEnv);
	      free(pmsaUserEnv);
	    }
	    // Determine the executionType for this run
	    if(getMap(request_inputs,"jrequest")!=NULL){
	      setMapInMaps(lconf,"main","executionType","json");
	      initAllEnvironment(lconf,request_inputs,ntmp,"jrequest");
	      register_signals(json_sig_handler);
	    }
	    else{
	      setMapInMaps(lconf,"main","executionType","xml");
	      initAllEnvironment(lconf,request_inputs,ntmp,"xrequest");
	    }
	    // Update every lenv map and add them to the main conf maps lenv section
	    map* pmTmp0=*lenv;
	    while(pmTmp0!=NULL){
	      setMapInMaps(lconf,"lenv",pmTmp0->name,pmTmp0->value);
	      pmTmp0=pmTmp0->next;
	    }
	    map* pmSubscribers;
	    json_object *req_subscribers;
	    if(json_object_object_get_ex(msg_obj,"main_subscriber",&req_subscribers)!=FALSE){
	      pmSubscribers=jsonToMap(req_subscribers);
	      if(pmSubscribers!=NULL){
		maps* pmsaTmp=createMaps("subscriber");
		pmsaTmp->content=pmSubscribers;
		addMapsToMaps(&lconf,pmsaTmp);
		freeMaps(&pmsaTmp);
		free(pmsaTmp);
	      }
	    }
	    map* pmListSections=getMapFromMaps(lconf,"servicesNamespace","sections_list");
	    if(pmListSections!=NULL) {
	      char* pcaListSections=zStrdup(pmListSections->value);
	      char *pcPtr;
	      char *pcCurrent = strtok_r (pcaListSections, ",", &pcPtr);
	      while(pcCurrent!=NULL){
		json_object* pjoValues;
		char* pcaKey=(char*)malloc(strlen(pcCurrent)+6);
		sprintf(pcaKey,"main_%s",pcCurrent);
		if(json_object_object_get_ex(msg_obj,pcaKey,&pjoValues)!=FALSE){
		  map* pmaTmp=jsonToMap(pjoValues);
		  if(pmaTmp!=NULL){
		    maps* pmsaTmp=createMaps(pcCurrent);
		    pmsaTmp->content=pmaTmp;
		    addMapsToMaps(&lconf,pmsaTmp);
		    freeMaps(&pmsaTmp);
		    free(pmsaTmp);
		  }
		}
		free(pcaKey);
		pcCurrent = strtok_r (NULL, ",", &pcPtr);
	      }
	      free(pcaListSections);
	    }

	    pmsTmp=getMaps(lconf,"lenv");
#ifdef USE_CALLBACK
	    invokeCallback(lconf,NULL,NULL,0,0);
#endif
	    invokeBasicCallback(lconf,SERVICE_STARTED);

	    // Populate the Registry
	    char conf_dir[1024];
	    int t;
	    char tmps1[1024];
	    r_inputs = NULL;
	    r_inputs = getMap (request_inputs, "metapath");
	    map* cwdMap0=getMapFromMaps(lconf,"main","servicePath");
	    if (r_inputs != NULL)
	      if(cwdMap0!=NULL)
		snprintf (conf_dir, 1024, "%s/%s", cwdMap0->value, r_inputs->value);
	      else
		snprintf (conf_dir, 1024, "%s/%s", ntmp, r_inputs->value);
	    else
	      if(cwdMap0!=NULL)
		snprintf (conf_dir, 1024, "%s", cwdMap0->value);
	      else
		snprintf (conf_dir, 1024, "%s", ntmp);
	    map* reg = getMapFromMaps (lconf, "main", "registry");
	    registry* zooRegistry=NULL;
	    if(reg!=NULL){
#ifndef WIN32
	      int saved_stdout = zDup (fileno (stdout));
	      zDup2 (fileno (stderr), fileno (stdout));
#endif
	      if(createRegistry (lconf,&zooRegistry,reg->value)<0){
		map *message=getMapFromMaps(lconf,"lenv","message");
		map *type=getMapFromMaps(lconf,"lenv","type");
#ifndef WIN32
		zDup2 (saved_stdout, fileno (stdout));
#endif
		return errorException (&lconf, message->value,
				       type->value, NULL);
	      }
#ifndef WIN32
	      zDup2 (saved_stdout, fileno (stdout));
	      zClose(saved_stdout);
#endif
	    }
	    r_inputs = getMap (request_inputs, "Identifier");
	    // TODO: Remove the following as we are already defining the
	    // Identifier from the client ZOO-Kernel sending the message
	    if(r_inputs==NULL){
	      // JSON
	      map* pmTmp=request_inputs;
	      while(pmTmp!=NULL){
		if(strstr(pmTmp->name,"processes")!=NULL){
		  char* pczTmp=strstr(pmTmp->name,"processes")+2;
		  pczTmp[strlen(pczTmp)-11]=0;
		  addToMap(request_inputs,"Identifier",pczTmp);
		  r_inputs = getMap (request_inputs, "Identifier");
		  break;
		}
		pmTmp=pmTmp->next;
	      }
	    }
	    service *s1=NULL;
	    // Reset metapath
	    addToMap(request_inputs,"metapath","");
	    setMapInMaps(lconf,"lenv","metapath","");
	    if(fetchService(zooRegistry,lconf,&s1,request_inputs,conf_dir,r_inputs->value,printExceptionReportResponse)!=0){
	      // TODO: cleanup memory
	      freeMaps(&lconf);
	      free(lconf);
	      return -1;
	    }

	    /**
	     * Create the input and output maps data structure
	     */
	    int i = 0;
	    HINTERNET hInternet;
	    HINTERNET res;
	    hInternet = InternetOpen (
#ifndef WIN32
				      (LPCTSTR)
#endif
				      "ZooWPSClient\0",
				      INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
#ifndef WIN32
	    if (!CHECK_INET_HANDLE (hInternet))
	      fprintf (stderr, "WARNING : hInternet handle failed to initialize");
#endif
	    json_object *reqi_format_jobj;
	    maps *request_input_real_format = NULL;
	    json_object *reqo_format_jobj;
	    maps *request_output_real_format = NULL;
	    maps *tmpmaps = request_input_real_format;
	    int eres;
	    map* pmExecutionType=getMapFromMaps(lconf,"main","executionType");

	    if(json_object_object_get_ex(msg_obj,"request_input_real_format",&reqi_format_jobj)!=FALSE)
	      request_input_real_format=jsonToMaps(reqi_format_jobj);

	    if(json_object_object_get_ex(msg_obj,"request_output_real_format",&reqo_format_jobj)!=FALSE)
	      request_output_real_format=jsonToMaps(reqo_format_jobj);

	    map* version=getMap(request_inputs,"version");
	    if(version==NULL)
	      version=getMapFromMaps(lconf,"main","version");
	    if(version!=NULL)
	      setMapInMaps(lconf,"main","rversion",version->value);

	    int vid=0;
	    if(version!=NULL)
	      vid=getVersionId(version->value);
	    if(vid<0)
	      vid=0;

	    char *fbkp, *fbkpid, *fbkpres, *fbkp1, *flog;
	    FILE *f0, *f1;
	    int pid;

	    dsNb=getMapFromMaps(conf,"lenv", "ds_nb");
	    setMapInMaps(lconf, "lenv", "ds_nb",dsNb->value);
	    map* usid = getMapFromMaps (lconf, "lenv", "uusid");
	    map* tmpm = getMap(*lenv, "osid");
	    setMapInMaps(lconf, "lenv", "osid",tmpm->value);
	    tmpm = getMapFromMaps (lconf, "lenv", "osid");

	    int cpid = atoi (tmpm->value);
	    pid=cpid;
	    r_inputs = getMapFromMaps (lconf, "main", "tmpPath");
	    setMapInMaps (lconf, "lenv", "async","true");
	    map* r_inputs1 = createMap("ServiceName", s1->name);

	    // Create the filename for the result file (.res)
	    fbkpres =
	      (char *)
	      malloc ((strlen (r_inputs->value) +
		       strlen (usid->value) + 7) * sizeof (char));
	    sprintf (fbkpres, "%s/%s.res", r_inputs->value, usid->value);
	    maps* bmap = createMaps("status");
	    bmap->content=createMap("usid",usid->value);
	    addToMap(bmap->content,"sid",tmpm->value);
	    addIntToMap(bmap->content,"pid",zGetpid());

	    createSPidFile(lconf,r_inputs->value);

	    int iNbChars=7;
	    if(pmExecutionType!=NULL && strncasecmp(pmExecutionType->value,"xml",3)==0)
	      iNbChars=8;
	    fbkp =
	      (char *)
	      malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		       strlen (usid->value) + 7) * sizeof (char));
	    sprintf (fbkp, "%s/%s_%s.%s", r_inputs->value, r_inputs1->value,
		     usid->value, pmExecutionType->value);
	    setMapInMaps (lconf, "lenv", "file.responseInit", fbkp);

#ifdef DEBUG
	    fprintf (stderr, "RUN IN BACKGROUND MODE \n");
	    fprintf (stderr, "son pid continue (origin %d) %d ...\n", cpid,
		     zGetpid ());
	    fprintf (stderr, "\nFILE TO STORE DATA %s\n", r_inputs->value);
#endif

#ifndef LOG_CONSOLE_ENABLED
	    flog =
	      (char *)
	      malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		       strlen (usid->value) + 13) * sizeof (char));
	    sprintf (flog, "%s/%s_%s_error.log", r_inputs->value,
		     r_inputs1->value, usid->value);
	    setMapInMaps (lconf, "lenv", "file.log", flog);
	    freopen (flog, "w+", stderr);
	    fflush (stderr);
#endif
	    f0 = freopen (fbkp, "w+", stdout);
	    rewind (stdout);

#ifndef WIN32
	    fclose (stdin);
#endif
#ifdef USE_CALLBACK
	    invokeCallback(lconf,NULL,NULL,0,0);
#endif
	    if(vid==0 && (pmExecutionType!=NULL && strncasecmp(pmExecutionType->value,"xml",3)==0)){
	      //
	      // set status to SERVICE_STARTED and flush stdout to ensure full
	      // content was outputed (the file used to store the ResponseDocument).
	      // Then, rewind stdout to restart writing from the begining of the file.
	      // This way, the data will be updated at the end of the process run.
	      //
	      printProcessResponse (lconf, request_inputs, cpid, s1, r_inputs1->value,
				    SERVICE_STARTED, request_input_real_format,
				    request_output_real_format);
	      fflush (stdout);
#ifdef RELY_ON_DB
	      recordResponse(lconf,fbkp);
#endif
	    }

	    fflush (stderr);

	    // Set Headers from lenv coming form the message
	    // TODO: append current headers by fetching the renv from the
	    // message?
	    map* pmCursor=*lenv;
	    while(pmCursor!=NULL){
	      if(strncasecmp(pmCursor->name,"rb_headers_",11)==0){
		setMapInMaps(lconf,"lenv",pmCursor->name,pmCursor->value);
	      }
	      pmCursor=pmCursor->next;
	    }
	    // Add the request section in the main conf maps
	    pmCursor=*irequest_inputs;
	    while(pmCursor!=NULL){
	      setMapInMaps(lconf,"request",pmCursor->name,pmCursor->value);
	      pmCursor=pmCursor->next;
	    }

	    map* pmExtraSupportedCodes=getMapFromMaps(lconf,"main","extra_supported_codes");
	    if(pmExtraSupportedCodes!=NULL){
	      char* pcaTmp=(char*)malloc((strlen(pmExtraSupportedCodes->value)+5)*sizeof(char));
	      sprintf(pcaTmp,"%s,201",pmExtraSupportedCodes->value);
	      setMapInMaps(lconf,"main","extra_supported_codes",pcaTmp);
	      free(pcaTmp);
	    }
	    else
	      setMapInMaps(lconf,"main","extra_supported_codes","201");
	    parseInputHttpRequests(lconf,request_input_real_format,&hInternet);

	    fbkp1 =
	      (char *)
	      malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		       strlen (usid->value) + 13) * sizeof (char));
	    sprintf (fbkp1, "%s/%s_final_%s.%s", r_inputs->value,
		     r_inputs1->value, usid->value, pmExecutionType->value);
	    setMapInMaps (lconf, "lenv", "file.responseFinal", fbkp1);

	    f1 = freopen (fbkp1, "w+", stdout);

	    map* serviceTypeMap=getMap(s1->content,"serviceType");
	    if(serviceTypeMap!=NULL)
	      setMapInMaps (lconf, "lenv", "serviceType", serviceTypeMap->value);
	    createLenvFile(lconf,r_inputs->value,usid->value);

#ifdef USE_CALLBACK
	    invokeCallback(lconf,request_input_real_format,NULL,1,0);
#endif
	    if(validateRequest(&lconf,s1,request_inputs, &request_input_real_format,&request_output_real_format,&hInternet)<0){
	      freeService (&s1);
	      free (s1);
	      fflush (stdout);
	      fflush (stderr);
	      fclose (f0);
	      fclose (f1);
	      if(dumpBackFinalFile(lconf,fbkp,fbkp1)<0)
		return -1;
#ifndef RELY_ON_DB
	      dumpMapsToFile(bmap,fbkpres,1);
	      removeShmLock (lconf, 1);
#else
	      recordResponse(lconf,fbkp1);
#ifdef USE_CALLBACK
	      invokeCallback(lconf,NULL,NULL,7,0);
#endif
#endif
	      map *pmPid=getMapFromMaps(lconf,"lenv","file.pid");
	      if(pmPid!=NULL)
		zUnlink (pmPid->value);
	      unhandleStatus (conf);
#ifdef RELY_ON_DB
#ifdef META_DB
	      cleanupCallbackThreads();
	      close_sql(lconf,1);
#endif
	      close_sql(lconf,0);
#endif
	      freeMaps (&request_input_real_format);
	      free (request_input_real_format);
	      freeMaps (&request_output_real_format);
	      free (request_output_real_format);
	      freeMaps (&tmpmaps);
	      free (tmpmaps);
	      return -1;
	    }
	    if(getMapFromMaps(lconf,"lenv","mapError")!=NULL){
	      setMapInMaps(lconf,"lenv","message",_("Issue with geographic data"));
#ifdef USE_CALLBACK
	      invokeCallback(lconf,NULL,NULL,7,0);
#endif
	      eres=-1;//SERVICE_FAILED;
	    }else{
	      map* testMap=getMapFromMaps(lconf,"main","memory");
	      if(testMap==NULL || strcasecmp(testMap->value,"load")!=0){
		dumpMapsValuesToFiles(&lconf,&request_input_real_format);
	      }
	      // TODO: set size to 0 for every outputs
	      //setMapInMaps(request_output_real_format,"Result","size","0");
	      loadServiceAndRun (&lconf, s1, request_inputs,
				 &request_input_real_format,
				 &request_output_real_format, &eres);
	    }
	    pmExecutionType=getMapFromMaps(lconf,"main","executionType");
	    if (eres != -1){
	      if(pmExecutionType!=NULL && strncasecmp(pmExecutionType->value,"xml",3)==0)
		outputResponse (s1, request_input_real_format,
				request_output_real_format, request_inputs,
				cpid, lconf, eres);
	      else{
		setMapInMaps(lconf,"lenv","force","true");
		createStatusFile(lconf,eres);
		setMapInMaps(lconf,"lenv","force","false");
		setMapInMaps(lconf,"lenv","no-headers","true");
		fflush(stdout);
		rewind(stdout);
		json_object* res=printJResult(lconf,s1,request_output_real_format,eres);
		const char* jsonStr0=json_object_to_json_string_ext(res,JSON_C_TO_STRING_NOSLASHESCAPE);
		if(getMapFromMaps(lconf,"lenv","jsonStr")==NULL)
		  setMapInMaps(lconf,"lenv","jsonStr",jsonStr0);
		invokeBasicCallback(lconf,eres);
		map* pmHasPrinted=getMapFromMaps(lconf,"lenv","hasPrinted");
		if(res!=NULL && (pmHasPrinted==NULL || strncasecmp(pmHasPrinted->value,"false",5)==0)){
		  if(getMapFromMaps(lconf,"lenv","no-headers")==NULL){
		    printHeaders(lconf);
		    printf("Status: 200 OK \r\n\r\n");
		  }
		  const char* jsonStr=json_object_to_json_string_ext(res,JSON_C_TO_STRING_NOSLASHESCAPE);
		  printf(jsonStr);
		  printf("\n");
		  fflush(stdout);
		}
		if(res!=NULL)
		  json_object_put(res);
	      }
	    }
	    fflush (stdout);

	    fflush(stderr);

	    fclose (stdout);

	    fclose (f0);
	    fclose (f1);

	    if(dumpBackFinalFile(lconf,fbkp,fbkp1)<0)
	      return -1;
	    map *pmPid=getMapFromMaps(lconf,"lenv","file.pid");
	    if(pmPid!=NULL)
	      zUnlink (pmPid->value);
	    switch(eres){
	    default:
	    case SERVICE_FAILED:
	      setMapInMaps(bmap,"status","status",wpsStatus[1]);
	      setMapInMaps(lconf,"lenv","fstate",wpsStatus[1]);
	      break;
	    case SERVICE_SUCCEEDED:
	      setMapInMaps(bmap,"status","status",wpsStatus[0]);
	      setMapInMaps(lconf,"lenv","fstate",wpsStatus[0]);
	      break;
	    }
#ifndef RELY_ON_DB
	    dumpMapsToFile(bmap,fbkpres,1);
	    removeShmLock (lconf, 1);
#else
	    recordResponse(lconf,fbkp1);
#ifdef USE_CALLBACK
	    if (eres == SERVICE_SUCCEEDED)
	      invokeCallback(lconf,NULL,request_output_real_format,6,0);
#endif
#endif
	    // Detect if there is a service_logs section
	    maps* pmsLogs=getMaps(lconf,"service_logs");
	    if(pmsLogs!=NULL){
	      map* pmTmpPath=getMapFromMaps(conf,"main","tmpPath");
	      map* pmUsid=getMapFromMaps(conf,"lenv","usid");
	      char *pcaPath =
		(char *) malloc ((strlen (pmTmpPath->value) + strlen (pmUsid->value) + 12) * sizeof (char));
	      sprintf (pcaPath, "%s/%s_logs.cfg", pmTmpPath->value, pmUsid->value);
	      dumpMapsToFile(pmsLogs,pcaPath,1);
	      free(pcaPath);
	    }
	    freeMaps(&bmap);
	    free(bmap);
	    zUnlink (fbkp1);
	    unhandleStatus (lconf);
	    //zSleep(100000);
	    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	    fprintf(stderr,"This worker can now exit!\n");
	    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	    char* sqlQuery2=(char*)malloc(((strlen(schema->value))+
					   strlen(uusid->value)+36)*sizeof(char));
	    sprintf(sqlQuery2,"DELETE FROM %s.workers WHERE uuid='%s'",schema->value,uusid->value);
	    OGRLayer *res2=fetchSql(lconf,iSqlCon-1,sqlQuery2);
	    free(sqlQuery2);
	    freeMaps(&lconf);
	    free(lconf);
	  }else{
	    if(forkId<0){
	      fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	      fprintf(stderr,_("Unable to create a new process, please try later!\n"));
	      fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	      fflush(stderr);
	    }else
	      wait(0);
	  }
	}
	poFeature = res->GetNextFeature();
      }
    }
    cleanFetchSql(conf,iSqlCon-1,res);
  }else{
    fprintf(stderr,"Concurrent access on the workers table imply that there is no need to take this request into account!\n");
    fflush(stderr);
  }
  fflush(stderr);
#ifdef META_DB
  close_sql(conf,1);
#endif
  close_sql(conf,0);
  setMapInMaps(conf,"lenv","ds_nb","-1");
  end_sql();
  return 0;
}
#endif
