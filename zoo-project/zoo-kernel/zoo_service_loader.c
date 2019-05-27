/*
 * Author : Gérald FENOY
 *
 *  Copyright 2008-2013 GeoLabs SARL. All rights reserved.
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

#ifdef META_DB
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

#include "cgic.h"

#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "ulinet.h"

#include <libintl.h>
#include <locale.h>
#include <string.h>

#include "service.h"

#include "service_internal.h"
#include "server_internal.h"
#include "response_print.h"
#include "request_parser.h"
#include "sqlapi.h"
#ifdef WIN32
#include "caching.h"
#endif

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

#ifdef USE_RUBY
#include "service_internal_ruby.h"
#endif

#ifdef USE_PERL
#include "service_internal_perl.h"
#endif

#ifdef USE_MONO
#include "service_internal_mono.h"
#endif

#ifdef USE_CALLBACK
#include "service_json.h"
#include "service_callback.h"
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

#ifndef WIN32
extern char **environ;
#endif


#ifdef WIN32
extern "C"
{
  __declspec (dllexport) char *strcasestr (char const *a, char const *b)
#ifndef USE_MS
  {
    char *x = zStrdup (a);
    char *y = zStrdup (b);

      x = _strlwr (x);
      y = _strlwr (y);
    char *pos = strstr (x, y);
    char *ret = pos == NULL ? NULL : (char *) (a + (pos - x));
      free (x);
      free (y);
      return ret;
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
 * Replace a char by another one in a string
 *
 * @param str the string to update
 * @param toReplace the char to replace
 * @param toReplaceBy the char that will be used
 */
void
translateChar (char *str, char toReplace, char toReplaceBy)
{
  int i = 0, len = strlen (str);
  for (i = 0; i < len; i++)
    {
      if (str[i] == toReplace)
        str[i] = toReplaceBy;
    }
}

/**
 * Dump back the final file fbkp1 to fbkp
 *
 * @param m the conf maps containing the main.cfg settings
 * @param fbkp the string corresponding to the name of the file
 * @param fbkp1 the string corresponding to the name of the file
 */
int dumpBackFinalFile(maps* m,char* fbkp,char* fbkp1)
{
  FILE *f2 = fopen (fbkp1, "rb");
#ifndef RELY_ON_DB
  semid lid = getShmLockId (m, 1);
  if (lid < 0)
    return -1;
  lockShm (lid);
#endif
  FILE *f3 = fopen (fbkp, "wb+");
  free (fbkp);
  fseek (f2, 0, SEEK_END);
  long flen = ftell (f2);
  fseek (f2, 0, SEEK_SET);
  char *tmps1 = (char *) malloc ((flen + 1) * sizeof (char));
  fread (tmps1, flen, 1, f2);
#ifdef WIN32
  /* knut: I think this block can be dropped; pchr may be NULL if result is not in XML format
  char *pchr=strrchr(tmps1,'>');
  flen=strlen(tmps1)-strlen(pchr)+1;
  tmps1[flen]=0;
  */
#endif
  fwrite (tmps1, 1, flen, f3);
  fclose (f2);
  fclose (f3);
  free(tmps1);
  return 1;
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
 * @param func a pointer to a function having 4 parameters 
 *  (registry*, maps*, xmlNodePtr and service*).
 * @see inheritance, readServiceFile
 */
int
recursReaddirF ( maps * m, registry *r, xmlDocPtr doc, xmlNodePtr n, char *conf_dir, 
		 char *prefix, int saved_stdout, int level, 
		 void (func) (registry *, maps *, xmlDocPtr, xmlNodePtr, service *) )
{
  struct dirent *dp;
  int scount = 0;

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
              recursReaddirF (m, r, doc, n, tmp, prefix, saved_stdout, level + 1,
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
        if(dp->d_name[0] != '.' && extn != NULL && strlen(extn) == 5 && strlen(dp->d_name)>6)
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
              service *s1 = createService();
              if (s1 == NULL)
                {
                  zDup2 (saved_stdout, fileno (stdout));
                  errorException (m, _("Unable to allocate memory"),
                                  "InternalError", NULL);
                  return -1;
                }
  #ifdef DEBUG
              fprintf (stderr, "#################\n%s\n#################\n",
                       tmps1);
  #endif
              t = readServiceFile (m, tmps1, &s1, tmpsn);
              free (tmpsn);
              if (t < 0)
                {
                  map *tmp00 = getMapFromMaps (m, "lenv", "message");
                  char tmp01[1024];
                  if (tmp00 != NULL)
                    sprintf (tmp01, _("Unable to parse the ZCFG file: %s (%s)"),
                             dp->d_name, tmp00->value);
                  else
                    sprintf (tmp01, _("Unable to parse the ZCFG file: %s."),
                             dp->d_name);
                  zDup2 (saved_stdout, fileno (stdout));
                  errorException (m, tmp01, "InternalError", NULL);
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
              scount++;
            }
          }
      }
  (void) closedir (dirp);
  return 1;
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
  char tmps1[1024];
  char ntmp[1024];
  maps *m = *myMap;
  maps *request_output_real_format = *ioutputs;
  maps *request_input_real_format = *inputs;
  /**
   * Extract serviceType to know what kind of service should be loaded
   */
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
  if (strlen (r_inputs->value) == 1
      && strncasecmp (r_inputs->value, "C", 1) == 0)
  {
     if (libp != NULL && libp->value != NULL) {
	    r_inputs = getMap (s1->content, "ServiceProvider");
		sprintf (tmps1, "%s/%s", libp->value, r_inputs->value);
	 }
     else {	 
        r_inputs = getMap (request_inputs, "metapath");
        if (r_inputs != NULL)
          sprintf (tmps1, "%s/%s", ntmp, r_inputs->value);
        else
          sprintf (tmps1, "%s/", ntmp);
	  
        char *altPath = zStrdup (tmps1);
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
                  errorException (m, tmpMsg, "InternalError", NULL);
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
	  errorException(m,tmps,"InternalError",NULL);
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
      errorException (m, tmpv, "InternalError", NULL);
      *eres = -1;
    }
  *myMap = m;
  *ioutputs = request_output_real_format;  
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
      errorException( m, _("Unable to cache HTTP POST Execute request."), "InternalError", NULL);  
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
  if (!CreateProcess (NULL,     // No module name (use command line)
                      TEXT (tmp),       // Command line
                      NULL,     // Process handle not inheritable
                      NULL,     // Thread handle not inheritable
                      FALSE,    // Set handle inheritance to FALSE
                      CREATE_NO_WINDOW, // Apache won't wait until the end
                      NULL,     // Use parent's environment block
                      NULL,     // Use parent's starting directory 
                      &si,      // Pointer to STARTUPINFO struct
                      &pi)      // Pointer to PROCESS_INFORMATION struct
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
 
#ifndef USE_GDB
#ifndef WIN32
  signal (SIGCHLD, SIG_IGN);
#endif  
  signal (SIGSEGV, sig_handler);
  signal (SIGTERM, sig_handler);
  signal (SIGINT, sig_handler);
  signal (SIGILL, sig_handler);
  signal (SIGFPE, sig_handler);
  signal (SIGABRT, sig_handler);
#endif

  
  map *r_inputs = NULL;
  map *request_inputs = *inputs;
  //fprintf(stderr,"%s \n",json_object_to_json_string_ext(mapToJson(request_inputs),JSON_C_TO_STRING_PLAIN));
  
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
  m->child=NULL;
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
#ifdef ETC_DIR
#ifndef WIN32
  getcwd (ntmp, 1024);
#else
  _getcwd (ntmp, 1024);
#endif
#endif

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
   */
  FILE *fstde = NULL;
  map *fstdem = getMapFromMaps (m, "main", "logPath");
  if (fstdem != NULL)
    fstde = freopen (fstdem->value, "a+", stderr);

  r_inputs = getMap (request_inputs, "language");
  if (r_inputs == NULL)
    r_inputs = getMap (request_inputs, "AcceptLanguages");
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
          errorException (m, tmp, "InvalidParameterValue", "language");
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
      setMapInMaps(m,"main","serverAddress",tmpUrl);
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

  //Check for minimum inputs
  map* version=getMap(request_inputs,"version");
  if(version==NULL)
    version=getMapFromMaps(m,"main","version");
  setMapInMaps(m,"main","rversion",version->value);
  int vid=getVersionId(version->value);
  if(vid<0)
    vid=0;
  map* err=NULL;
  const char **vvr=(const char**)requests[vid];
  checkValidValue(request_inputs,&err,"request",vvr,1);
  const char *vvs[]={
    "WPS",
    NULL
  };
  if(err!=NULL){
    checkValidValue(request_inputs,&err,"service",(const char**)vvs,1);
    printExceptionReportResponse (m, err);
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
      if(strstr(version1->value,schemas[1][0])!=NULL)
	addToMap(request_inputs,"version",schemas[1][0]);
      else
	addToMap(request_inputs,"version",version1->value);
      version=getMap(request_inputs,"version");
      setMapInMaps(m,"main","rversion",version->value);
      vid=getVersionId(version->value);
    }
  }
  if(err!=NULL){
    printExceptionReportResponse (m, err);
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

  maps *request_output_real_format = NULL;
  map *tmpm = getMapFromMaps (m, "main", "serverAddress");
  if (tmpm != NULL)
    SERVICE_URL = zStrdup (tmpm->value);
  else
    SERVICE_URL = zStrdup (DEFAULT_SERVICE_URL);



  service *s1;
  int scount = 0;
#ifdef DEBUG
  dumpMap (r_inputs);
#endif
  char conf_dir[1024];
  int t;
  char tmps1[1024];

  r_inputs = NULL;
  r_inputs = getMap (request_inputs, "metapath");
  map* cwdMap0=getMapFromMaps(m,"main","servicePath");
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
      errorException (m, message->value,
		      type->value, NULL);
      return 0;
    }
#ifndef WIN32
    zDup2 (saved_stdout, fileno (stdout));
    zClose(saved_stdout);
#endif
  }

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
#ifdef RELY_ON_DB
	  map* dsNb=getMapFromMaps(m,"lenv","ds_nb");
	  if(dsNb!=NULL && atoi(dsNb->value)>1)
	    close_sql(m,1);
	  close_sql(m,0);
#endif
	  
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
	    errorException (m, _("The specified path does not exist."),
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

	    char *orig = zStrdup (r_inputs->value);

	    int saved_stdout = zDup (fileno (stdout));
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
#ifdef USE_HPC
		      addNestedOutputs(&svc);
#endif

		      printDescribeProcessForProcess(zooRegistry, m, doc, n, svc);
		      freeService(&svc);
		      free(svc);                             
		    }
		    zcfg = zcfg->next;
		  }            
		}
  
		if (int res =
		    recursReaddirF (m, zooRegistry, doc, n, conf_dir, NULL, saved_stdout, 0,
				    printDescribeProcessForProcess) < 0)
		  return res;
#ifdef META_DB
		fetchServicesFromDb(zooRegistry,m,doc,n,printDescribeProcessForProcess,0);
		close_sql(m,0);
#endif      

	      }
	    else
	      {
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
			service* s2=extractServiceFromDb(m,import->name,0);
			if(s2==NULL){
#endif
			  s1 = createService();
			  t = readServiceFile (m, import->value, &s1, import->name);
                
			  if (t < 0) // failure reading zcfg
			    {
			      map *tmp00 = getMapFromMaps (m, "lenv", "message");
			      char tmp01[1024];
			      if (tmp00 != NULL)
				sprintf (tmp01, _("Unable to parse the ZCFG file: %s (%s)"), import->value, tmp00->value);
			      else
				sprintf (tmp01, _("Unable to parse the ZCFG file: %s."), import->value);
			      
			      zDup2 (saved_stdout, fileno (stdout));
			      errorException (m, tmp01, "InternalError", NULL);
			      
			      freeMaps (&m);
			      free (m);

			      if(zooRegistry!=NULL){
				freeRegistry(&zooRegistry);
				free(zooRegistry);
			      }
			      free (orig);
			      free (REQUEST);
			      closedir (dirp);
			      //xmlFreeDoc (doc);
			      xmlCleanupParser ();
			      zooXmlCleanupNs ();
                    
			      return 1;
			    }
#ifdef DEBUG
			  dumpService (s1);
#endif
			  inheritance(zooRegistry,&s1);
#ifdef USE_HPC
			  addNestedOutputs(&s1);
#endif
			  printDescribeProcessForProcess (zooRegistry, m, doc, n, s1);
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
			    map *tmp00 = getMapFromMaps (m, "lenv", "message");
			    char tmp01[1024];
			    if (tmp00 != NULL)
			      sprintf (tmp01,
				       _
				       ("Unable to parse the ZCFG file for the following ZOO-Service: %s. Message: %s"),
				       tmps, tmp00->value);
			    else
			      sprintf (tmp01,
				       _
				       ("Unable to parse the ZCFG file for the following ZOO-Service: %s."),
				       tmps);
			    zDup2 (saved_stdout, fileno (stdout));
			    errorException (m, tmp01, "InvalidParameterValue",
					    "identifier");
			    freeMaps (&m);
			    free (m);
			    if(zooRegistry!=NULL){
			      freeRegistry(&zooRegistry);
			      free(zooRegistry);
			    }
			    free (REQUEST);
			    free (corig);
			    free (orig);
			    free (SERVICE_URL);
			    free (s1);
			    closedir (dirp);
			    //xmlFreeDoc (doc);
			    xmlCleanupParser ();
			    zooXmlCleanupNs ();
			    return 1;
			  }
#ifdef DEBUG
			dumpService (s1);
#endif
			inheritance(zooRegistry,&s1);
#ifdef USE_HPC
			addNestedOutputs(&s1);
#endif
			printDescribeProcessForProcess (zooRegistry, m, doc, n, s1);
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
			_init_sql(m,"metadb");
			//FAILED CONNECTING DB
			if(getMapFromMaps(m,"lenv","dbIssue")!=NULL){
			  fprintf(stderr,"ERROR CONNECTING METADB");
			}
			service* s2=extractServiceFromDb(m,corig,0);
			if(s2!=NULL){
			  inheritance(zooRegistry,&s2);
#ifdef USE_HPC
			  addNestedOutputs(&s2);
#endif
			  printDescribeProcessForProcess (zooRegistry,m, doc, n, s2);
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
				      zDup2 (saved_stdout, fileno (stdout));
				      return errorException (m,
							     _
							     ("Unable to allocate memory"),
							     "InternalError",
							     NULL);
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
				      map *tmp00 =
					getMapFromMaps (m, "lenv", "message");
				      char tmp01[1024];
				      if (tmp00 != NULL)
					sprintf (tmp01,
						 _
						 ("Unable to parse the ZCFG file: %s (%s)"),
						 dp->d_name, tmp00->value);
				      else
					sprintf (tmp01,
						 _
						 ("Unable to parse the ZCFG file: %s."),
						 dp->d_name);
				      zDup2 (saved_stdout, fileno (stdout));
				      errorException (m, tmp01, "InternalError",
						      NULL);
				      freeMaps (&m);
				      free (m);
				      if(zooRegistry!=NULL){
					freeRegistry(&zooRegistry);
					free(zooRegistry);
				      }
				      free (orig);
				      free (REQUEST);
				      closedir (dirp);
				      //xmlFreeDoc (doc);
				      xmlCleanupParser ();
				      zooXmlCleanupNs ();
				      return 1;
				    }
#ifdef DEBUG
				  dumpService (s1);
#endif
				  inheritance(zooRegistry,&s1);
#ifdef USE_HPC
				  addNestedOutputs(&s1);
#endif
				  /*json_object* jobj=serviceToJson(s1);
				  const char* jsonStr=json_object_to_json_string_ext(jobj,JSON_C_TO_STRING_PLAIN);
				  fprintf(stderr,"*** %s %d %s \n",__FILE__,__LINE__,jsonStr);*/

				  printDescribeProcessForProcess (zooRegistry,m, doc, n, s1);
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
			map *tmp00 = getMapFromMaps (m, "lenv", "message");
			char tmp01[1024];
			if (tmp00 != NULL)
			  sprintf (tmp01,
				   _("Unable to parse the ZCFG file: %s (%s)"),
				   buff, tmp00->value);
			else
			  sprintf (tmp01,
				   _("Unable to parse the ZCFG file: %s."),
				   buff);
			zDup2 (saved_stdout, fileno (stdout));
			errorException (m, tmp01, "InvalidParameterValue",
					"Identifier");
			freeMaps (&m);
			free (m);
			if(zooRegistry!=NULL){
			  freeRegistry(&zooRegistry);
			  free(zooRegistry);
			}
			free (orig);
			free (REQUEST);
			closedir (dirp);
			if (corig != NULL)
			  free (corig);
			xmlFreeDoc (doc);
			xmlCleanupParser ();
			zooXmlCleanupNs ();
			return 1;
		      }
		    rewinddir (dirp);
		    tmps = strtok_r (NULL, ",", &saveptr);
		    if (corig != NULL)
		      free (corig);
		  }		  
	      }
	    closedir (dirp);
	    fflush (stdout);
	    zDup2 (saved_stdout, fileno (stdout));
	    free (orig);
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
	    errorException (m,_(message),"InvalidParameterValue", "request");
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
    errorException (m,_("Unable to run Execute request using the GET HTTP method"),"InvalidParameterValue", "request");  
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
  map* import = getMapFromMaps (m, IMPORTSERVICE, r_inputs->value); 
  if (import != NULL && import->value != NULL) { 
    strncpy(tmps1, import->value, 1024);
    setMapInMaps (m, "lenv", "Identifier", r_inputs->value);
    setMapInMaps (m, "lenv", "oIdentifier", r_inputs->value);
  } 
  else {
    snprintf (tmps1, 1024, "%s/%s.zcfg", conf_dir, r_inputs->value);
#ifdef DEBUG
    fprintf (stderr, "Trying to load %s\n", tmps1);
#endif
    if (strstr (r_inputs->value, ".") != NULL)
      {
	char *identifier = zStrdup (r_inputs->value);
	parseIdentifier (m, conf_dir, identifier, tmps1);
	map *tmpMap = getMapFromMaps (m, "lenv", "metapath");
	if (tmpMap != NULL)
	  addToMap (request_inputs, "metapath", tmpMap->value);
	free (identifier);
      }
    else
      {
	setMapInMaps (m, "lenv", "Identifier", r_inputs->value);
	setMapInMaps (m, "lenv", "oIdentifier", r_inputs->value);
      }
  }

  r_inputs = getMapFromMaps (m, "lenv", "Identifier");
  
#ifdef META_DB
  int metadb_id=_init_sql(m,"metadb");
  //FAILED CONNECTING DB
  if(getMapFromMaps(m,"lenv","dbIssue")!=NULL || metadb_id<0){
    fprintf(stderr,"ERROR CONNECTING METADB\n");
  }
  if(metadb_id>=0)
    s1=extractServiceFromDb(m,r_inputs->value,0);
  //close_sql(m,0);
  if(s1!=NULL){
    inheritance(zooRegistry,&s1);
#ifdef USE_HPC
    addNestedOutputs(&s1);
#endif
    if(zooRegistry!=NULL){
      freeRegistry(&zooRegistry);
      free(zooRegistry);
    }
  }else /* Not found in MetaDB */{
#endif
    s1 = createService();
    if (s1 == NULL)
      {
	freeMaps (&m);
	free (m);
	if(zooRegistry!=NULL){
	  freeRegistry(&zooRegistry);
	  free(zooRegistry);
	}
	free (REQUEST);
	free (SERVICE_URL);
	return errorException (m, _("Unable to allocate memory"),
			       "InternalError", NULL);
      }

    int saved_stdout = zDup (fileno (stdout));
    zDup2 (fileno (stderr), fileno (stdout));
    t = readServiceFile (m, tmps1, &s1, r_inputs->value);
    if(t>=0){
      inheritance(zooRegistry,&s1);
#ifdef USE_HPC
      addNestedOutputs(&s1);
#endif
    }
    if(zooRegistry!=NULL){
      freeRegistry(&zooRegistry);
      free(zooRegistry);
    }
    fflush (stdout);
    zDup2 (saved_stdout, fileno (stdout));
    if (t < 0)
      {
	char *tmpMsg = (char *) malloc (2048 + strlen (r_inputs->value));
	sprintf (tmpMsg,
		 _
		 ("The value for <identifier> seems to be wrong (%s). Please specify one of the processes in the list returned by a GetCapabilities request."),
		 r_inputs->value);
	errorException (m, tmpMsg, "InvalidParameterValue", "identifier");
	free (tmpMsg);
	free (s1);
	freeMaps (&m);
	free (m);
	free (REQUEST);
	free (SERVICE_URL);
	return 0;
      }
    zClose (saved_stdout);
#ifdef META_DB
  }
#endif
  
#ifdef DEBUG
  dumpService (s1);
#endif
  int j;


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
  maps *request_input_real_format = NULL;
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
  
  // Define each env variable in runing environment
  maps *curs = getMaps (m, "env");
  if (curs != NULL)
    {
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
	errorException (m,
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
	  errorException (m,_("The process does not permit the desired execution mode."),"NoSuchMode", mode->value);  
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

  int eres = SERVICE_STARTED;
  int cpid = zGetpid ();

  // Create a map containing a copy of the request map
  maps *_tmpMaps = createMaps("request");
  addMapToMap(&_tmpMaps->content,request_inputs);
  addMapsToMaps (&m, _tmpMaps);
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
  if(cwdMap0!=NULL){
    addToMap (_tmpMaps->content, "cwd", cwdMap0->value);
    addToMap (_tmpMaps->content, "rcwd", ntmp);
  }
  else
    addToMap (_tmpMaps->content, "cwd", ntmp);
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
      map *testing = getMapFromMaps (m, "main", "cookiePrefix");
      parseCookie(&m,originalCookie);
      map *sessId=getMapFromMaps(m,"cookies",(testing==NULL?"ID":testing->value));
      if (sessId!=NULL)
        {
	  addToMap (_tmpMaps->content, "sessid", sessId->value);
          char session_file_path[1024];
          map *tmpPath = getMapFromMaps (m, "main", "sessPath");
          if (tmpPath == NULL)
            tmpPath = getMapFromMaps (m, "main", "tmpPath");
          char *tmp1 = strtok (tcook, ";");
          if (tmp1 != NULL)
            sprintf (session_file_path, "%s/sess_%s.cfg", tmpPath->value,
                     sessId->value);
          else
            sprintf (session_file_path, "%s/sess_%s.cfg", tmpPath->value,
                     sessId->value);
          free (tcook);
          maps *tmpSess = (maps *) malloc (MAPS_SIZE);
	  tmpSess->child=NULL;
          struct stat file_status;
          int istat = stat (session_file_path, &file_status);
          if (istat == 0 && file_status.st_size > 0)
            {
	      int saved_stdout = zDup (fileno (stdout));
	      zDup2 (fileno (stderr), fileno (stdout));
              conf_read (session_file_path, tmpSess);
              addMapsToMaps (&m, tmpSess);
              freeMaps (&tmpSess);
	      fflush(stdout);
	      zDup2 (saved_stdout, fileno (stdout));
	      zClose(saved_stdout);
            }
	  free (tmpSess);
        }
    }
  addMapsToMaps (&m, _tmpMaps);
  freeMaps (&_tmpMaps);
  free (_tmpMaps);
  maps* bmap=NULL;
#ifdef DEBUG
  dumpMap (request_inputs);
#endif
  int ei = 1;
  
#ifdef WIN32
  LPVOID orig = GetEnvironmentStrings();
  LPTSTR s = (LPTSTR) orig;
#else
  char **orig = environ;
  char *s=*orig;
#endif

  _tmpMaps = createMaps("renv");
if(orig!=NULL)
  for (; 
#ifdef WIN32
       *s;
       s++
#else
       s;
       ei++
#endif
       ) {
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
#ifndef WIN32
    s = *(orig+ei);
#endif
  }
  if(_tmpMaps->content!=NULL && getMap(_tmpMaps->content,"HTTP_COOKIE")!=NULL){
    addToMap(_tmpMaps->content,"HTTP_COOKIE1",&cgiCookie[0]);
  }
  addMapsToMaps (&m, _tmpMaps);
  freeMaps (&_tmpMaps);
  free (_tmpMaps);
#ifdef WIN32
  FreeEnvironmentStrings((LPCH)orig);
#endif
  if(postRequest!=NULL)
    setMapInMaps (m, "renv", "xrequest", postRequest->value);
  //dumpMaps(m);
#ifdef WIN32
  char *cgiSidL = NULL;
  if (getenv ("CGISID") != NULL)
    addToMap (request_inputs, "cgiSid", getenv ("CGISID"));

  char* usidp;
  if ( (usidp = getenv("USID")) != NULL ) {
    setMapInMaps (m, "lenv", "usid", usidp);
  }

  map *test1 = getMap (request_inputs, "cgiSid");
  if (test1 != NULL){
    cgiSid = zStrdup(test1->value);
    addToMap (request_inputs, "storeExecuteResponse", "true");
    addToMap (request_inputs, "status", "true");
    setMapInMaps (m, "lenv", "osid", test1->value);
    status = getMap (request_inputs, "status");
  }
  test1 = getMap (request_inputs, "usid");
  if (test1 != NULL){
    setMapInMaps (m, "lenv", "usid", test1->value);
    setMapInMaps (m, "lenv", "uusid", test1->value);
  }
#endif

  char *fbkp, *fbkpid, *fbkpres, *fbkp1, *flog;
  FILE *f0, *f1;
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
	  
	  // Create PID file referencing the OS process identifier
	  fbkpid =
	    (char *)
	    malloc ((strlen (r_inputs->value) +
		     strlen (usid->value) + 7) * sizeof (char));
	  sprintf (fbkpid, "%s/%s.pid", r_inputs->value, usid->value);
	  setMapInMaps (m, "lenv", "file.pid", fbkpid);

	  f0 = freopen (fbkpid, "w+",stdout);
	  printf("%d",zGetpid());
	  fflush(stdout);

	  // Create SID file referencing the semaphore name
	  fbkp =
	    (char *)
	    malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		     strlen (usid->value) + 7) * sizeof (char));
	  sprintf (fbkp, "%s/%s.sid", r_inputs->value, usid->value);
	  setMapInMaps (m, "lenv", "file.sid", fbkp);
	  FILE* f2 = freopen (fbkp, "w+",stdout);
	  printf("%s",tmpm->value);
	  fflush(f2);
	  free(fbkp);

	  fbkp =
	    (char *)
	    malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		     strlen (usid->value) + 7) * sizeof (char));
	  sprintf (fbkp, "%s/%s_%s.xml", r_inputs->value, r_inputs1->value,
		   usid->value);
	  setMapInMaps (m, "lenv", "file.responseInit", fbkp);
	  flog =
	    (char *)
	    malloc ((strlen (r_inputs->value) + strlen (r_inputs1->value) +
		     strlen (usid->value) + 13) * sizeof (char));
	  sprintf (flog, "%s/%s_%s_error.log", r_inputs->value,
		   r_inputs1->value, usid->value);
	  setMapInMaps (m, "lenv", "file.log", flog);
#ifdef DEBUG
	  fprintf (stderr, "RUN IN BACKGROUND MODE \n");
	  fprintf (stderr, "son pid continue (origin %d) %d ...\n", cpid,
		   zGetpid ());
	  fprintf (stderr, "\nFILE TO STORE DATA %s\n", r_inputs->value);
#endif
	  freopen (flog, "w+", stderr);
	  fflush (stderr);
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

	  char *flenv =
	    (char *)
	    malloc ((strlen (r_inputs->value) + 
		     strlen (usid->value) + 12) * sizeof (char));
	  sprintf (flenv, "%s/%s_lenv.cfg", r_inputs->value, usid->value);
	  maps* lenvMaps=getMaps(m,"lenv");
	  dumpMapsToFile(lenvMaps,flenv,0);
	  free(flenv);

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
	    zUnlink (fbkpid);
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
	  errorException (m, _("Unable to run the child process properly"),
			  "InternalError", NULL);
        }
    }
	
#ifdef DEBUG
  fprintf (stderr, "RUN IN BACKGROUND MODE %s %d \n",__FILE__,__LINE__);
  dumpMaps (request_output_real_format);
  fprintf (stderr, "RUN IN BACKGROUND MODE %s %d \n",__FILE__,__LINE__);
#endif
  fflush(stdout);
  rewind(stdout);

  //fprintf(stderr,"%s %d %d\n",__FILE__,__LINE__,eres);  
  if (eres != -1)
    outputResponse (s1, request_input_real_format,
                    request_output_real_format, request_inputs,
                    cpid, m, eres);
  fflush (stdout);
  
  /**
   * Ensure that if error occurs when freeing memory, no signal will return
   * an ExceptionReport document as the result was already returned to the 
   * client.
   */
#ifndef USE_GDB
  signal (SIGSEGV, donothing);
  signal (SIGTERM, donothing);
  signal (SIGINT, donothing);
  signal (SIGILL, donothing);
  signal (SIGFPE, donothing);
  signal (SIGABRT, donothing);
#endif

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
      zUnlink (fbkpid);
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
      if (eres == SERVICE_SUCCEEDED)
#ifdef USE_CALLBACK
	invokeCallback(m,NULL,request_output_real_format,6,0);
#endif
#endif
      freeMaps(&bmap);
      free(bmap);
      zUnlink (fbkp1);
      unhandleStatus (m);
#ifdef RELY_ON_DB
#ifdef META_DB
      cleanupCallbackThreads();
      close_sql(m,1);
#endif
      close_sql(m,0);
      end_sql();
#endif
      free(fbkpid);
      free(fbkpres); 
      free (fbkp1);
      if(cgiSid!=NULL)
	free(cgiSid);
      //InternetCloseHandle (&hInternet);
      fprintf (stderr, "RUN IN BACKGROUND MODE %s %d \n",__FILE__,__LINE__);
      fflush(stderr);
      fclose (stderr);
      zUnlink (flog);
      free (flog);
    }
  else{
    //InternetCloseHandle (&hInternet);  
#ifdef META_DB
    close_sql(m,0);
#endif
  }
  
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
