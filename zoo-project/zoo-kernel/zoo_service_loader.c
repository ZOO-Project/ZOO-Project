/**
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

#define length(x) (sizeof(x) / sizeof(x[0]))

extern "C" int yylex();
extern "C" int crlex();

#include "cgic.h"

extern "C" {
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}

#include "ulinet.h"

#include <libintl.h>
#include <locale.h>
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

#ifdef USE_RUBY
#include "service_internal_ruby.h"
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define pid_t int;
#endif
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

#ifdef WIN32
extern "C" {
  __declspec(dllexport) char *strcasestr(char const *a, char const *b)
#ifndef USE_MS
 { 
  char *x=zStrdup(a); 
  char *y=zStrdup(b); 
  
  x=_strlwr(x); 
  y=_strlwr(y); 
  char *pos = strstr(x, y); 
  char *ret = pos == NULL ? NULL : (char *)(a + (pos-x)); 
  free(x); 
  free(y); 
  return ret; 
 };
#else
  ;
#endif
}
#endif

#define _(String) dgettext ("zoo-kernel",String)
#define __(String) dgettext ("zoo-service",String)

extern   int getServiceFromFile(maps*,const char*,service**);

int readServiceFile(maps* conf, char* file,service** service,char *name){
  int t=getServiceFromFile(conf,file,service);
#ifdef YAML
  if(t<0){
    t=getServiceFromYAML(conf,file,service,name);
  }
#endif
  return t;
}

void translateChar(char* str,char toReplace,char toReplaceBy){
  int i=0,len=strlen(str);
  for(i=0;i<len;i++){
    if(str[i]==toReplace)
      str[i]=toReplaceBy;
  }
}

/**
 * Create (or append to) an array valued maps
 * value = "["",""]"
 */
int appendMapsToMaps(maps* m,maps* mo,maps* mi,elements* elem){
  maps* tmpMaps=getMaps(mo,mi->name);
  map* tmap=getMapType(tmpMaps->content);
  elements* el=getElements(elem,mi->name);
  int hasEl=1;
  if(el==NULL)
    hasEl=-1;
  if(tmap==NULL){
    if(hasEl>0)
      tmap=getMapType(el->defaults->content);      
  }

  map* testMap=NULL;
  if(hasEl>0){
    testMap=getMap(el->content,"maxOccurs");
  }else{
    testMap=createMap("maxOccurs","unbounded");
  }

  if(testMap!=NULL){
    if(strncasecmp(testMap->value,"unbounded",9)!=0 && atoi(testMap->value)>1){
      if(addMapsArrayToMaps(&mo,mi,tmap->name)<0){
	char emsg[1024];
	sprintf(emsg,_("You set maximum occurences for <%s> as %i but you tried to use it more than the limit you set. Please correct your ZCFG file or your request."),mi->name,atoi(testMap->value));
	errorException(m,emsg,"InternalError",NULL);
	return -1;
      }
    }else{
      if(strncasecmp(testMap->value,"unbounded",9)==0){
	if(hasEl<0){
	  freeMap(&testMap);
	  free(testMap);
	}
	if(addMapsArrayToMaps(&mo,mi,tmap->name)<0){
	  char emsg[1024];
	  map* tmpMap=getMap(mi->content,"length");
	  sprintf(emsg,_("ZOO-Kernel was unable to load your data for %s position %s."),mi->name,tmpMap->value);
	  errorException(m,emsg,"InternalError",NULL);
	  return -1;
	}
      }
      else{
	char emsg[1024];
	sprintf(emsg,_("You set maximum occurences for <%s> to one but you tried to use it more than once. Please correct your ZCFG file or your request."),mi->name);
	errorException(m,emsg,"InternalError",NULL);
	return -1;
      }
    }
  }
  return 0;
}

int recursReaddirF(maps* m,xmlNodePtr n,char *conf_dir,char *prefix,int saved_stdout,int level,void (func) (maps*,xmlNodePtr,service*)){
  struct dirent *dp;
  int scount=0;

  if(conf_dir==NULL)
    return 1;
  DIR *dirp = opendir(conf_dir);
  if(dirp==NULL){
    if(level>0)
      return 1;
    else
      return -1;
  }
  char tmp1[25];
  sprintf(tmp1,"sprefix_%d",level);
  char levels[17];
  sprintf(levels,"%d",level);
  setMapInMaps(m,"lenv","level",levels);
  while ((dp = readdir(dirp)) != NULL)
    if((dp->d_type==DT_DIR || dp->d_type==DT_LNK) && dp->d_name[0]!='.' && strstr(dp->d_name,".")==NULL){

      char *tmp=(char*)malloc((strlen(conf_dir)+strlen(dp->d_name)+2)*sizeof(char));
      sprintf(tmp,"%s/%s",conf_dir,dp->d_name);

      if(prefix!=NULL){
	prefix=NULL;
      }
      prefix=(char*)malloc((strlen(dp->d_name)+2)*sizeof(char));
      sprintf(prefix,"%s.",dp->d_name);
      
      //map* tmpMap=getMapFromMaps(m,"lenv",tmp1);
      
      int res;
      if(prefix!=NULL){
	setMapInMaps(m,"lenv",tmp1,prefix);
	char levels1[17];
	sprintf(levels1,"%d",level+1);
	setMapInMaps(m,"lenv","level",levels1);
	res=recursReaddirF(m,n,tmp,prefix,saved_stdout,level+1,func);
	sprintf(levels1,"%d",level);
	setMapInMaps(m,"lenv","level",levels1);
	free(prefix);
	prefix=NULL;
      }else
	res=-1;
      free(tmp);
      if(res<0){
	return res;
      }
    }
    else{
      if(dp->d_name[0]!='.' && strstr(dp->d_name,".zcfg")!=0){
	int t;
	char tmps1[1024];
	memset(tmps1,0,1024);
	snprintf(tmps1,1024,"%s/%s",conf_dir,dp->d_name);
	service* s1=(service*)malloc(SERVICE_SIZE);
	if(s1 == NULL){ 
	  dup2(saved_stdout,fileno(stdout));
	  errorException(m, _("Unable to allocate memory."),"InternalError",NULL);
	  return -1;
	}
#ifdef DEBUG
	fprintf(stderr,"#################\n%s\n#################\n",tmps1);
#endif
	t=readServiceFile(m,tmps1,&s1,dp->d_name);

	if(t<0){
	  dumpMaps(m);
	  map* tmp00=getMapFromMaps(m,"lenv","message");
	  char tmp01[1024];
	  if(tmp00!=NULL)
	    sprintf(tmp01,_("Unable to parse the ZCFG file: %s (%s)"),dp->d_name,tmp00->value);
	  else
	    sprintf(tmp01,_("Unable to parse the ZCFG file: %s."),dp->d_name);
	  dup2(saved_stdout,fileno(stdout));
	  errorException(m, tmp01,"InternalError",NULL);
	  freeMaps(&m);
	  free(m);
	  return -1;
	}
#ifdef DEBUG
	dumpService(s1);
	fflush(stdout);
	fflush(stderr);
#endif
	func(m,n,s1);
	freeService(&s1);
	free(s1);
	scount++;
      }
    }
  (void)closedir(dirp);
  return 1;
}

xmlXPathObjectPtr extractFromDoc(xmlDocPtr doc,const char* search){
  xmlXPathContextPtr xpathCtx;
  xmlXPathObjectPtr xpathObj;
  xpathCtx = xmlXPathNewContext(doc);
  xpathObj = xmlXPathEvalExpression(BAD_CAST search,xpathCtx);
  xmlXPathFreeContext(xpathCtx);
  return xpathObj;
}

void donothing(int sig){
#ifdef DEBUG
  fprintf(stderr,"Signal %d after the ZOO-Kernel returned result !\n",sig);
#endif
  exit(0);
}

void sig_handler(int sig){
  char tmp[100];
  const char *ssig;
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
  sprintf(tmp,_("ZOO Kernel failed to process your request receiving signal %d = %s"),sig,ssig);
  errorException(NULL, tmp, "InternalError",NULL);
#ifdef DEBUG
  fprintf(stderr,"Not this time!\n");
#endif
  exit(0);
}

void loadServiceAndRun(maps **myMap,service* s1,map* request_inputs,maps **inputs,maps** ioutputs,int* eres){
  char tmps1[1024];
  char ntmp[1024];
  maps *m=*myMap;
  maps *request_output_real_format=*ioutputs;
  maps *request_input_real_format=*inputs;
  /**
   * Extract serviceType to know what kind of service should be loaded
   */
  map* r_inputs=NULL;
#ifndef WIN32
  getcwd(ntmp,1024);
#else
  _getcwd(ntmp,1024);
#endif
  r_inputs=getMap(s1->content,"serviceType");
#ifdef DEBUG
  fprintf(stderr,"LOAD A %s SERVICE PROVIDER \n",r_inputs->value);
  fflush(stderr);
#endif
  if(strlen(r_inputs->value)==1 && strncasecmp(r_inputs->value,"C",1)==0){
    r_inputs=getMap(request_inputs,"metapath");
    if(r_inputs!=NULL)
      sprintf(tmps1,"%s/%s",ntmp,r_inputs->value);
    else
      sprintf(tmps1,"%s/",ntmp);
    char *altPath=zStrdup(tmps1);
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
#ifdef WIN32
    DWORD errstr;
    errstr = GetLastError();
#else
    char *errstr;
    errstr = dlerror();
#endif
#ifdef DEBUG
    fprintf(stderr,"%s loaded (%d) \n",tmps1,errstr);
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
	*eres=execute((char***)&main_conf[0],(char***)&inputs[0],(char***)&outputs[0]);
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
	r_inputs=getMapFromMaps(m,"lenv","Identifier");
#ifdef DEBUG
	fprintf(stderr,"Try to load function %s\n",r_inputs->value);
#endif
	typedef int (*execute_t)(maps**,maps**,maps**);
#ifdef WIN32
	execute_t execute=(execute_t)GetProcAddress(so,r_inputs->value); 
#else
	execute_t execute=(execute_t)dlsym(so,r_inputs->value);
#endif

	if(execute==NULL){
#ifdef WIN32
	  errstr = GetLastError();
#else
	  errstr = dlerror();
#endif
	  char *tmpMsg=(char*)malloc(2048+strlen(r_inputs->value));
	  sprintf(tmpMsg,_("Error occured while running the %s function: %s"),r_inputs->value,errstr);
	  errorException(m, tmpMsg, "InternalError",NULL);
	  free(tmpMsg);
#ifdef DEBUG
	  fprintf(stderr,"Function %s error %s\n",r_inputs->value,errstr);
#endif
	  *eres=-1;
	  return;
	}

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
	*eres=execute(&m,&request_input_real_format,&request_output_real_format);
#ifdef DEBUG
	fprintf(stderr,"Function loaded and returned %d\n",eres);
	fflush(stderr);
#endif
      }
#ifdef WIN32
      *ioutputs=dupMaps(&request_output_real_format);
      FreeLibrary(so);
#else
      dlclose(so);
#endif
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
      sprintf(tmps,_("C Library can't be loaded %s"),errstr);
      map* tmps1=createMap("text",tmps);
      printExceptionReportResponse(m,tmps1);
      *eres=-1;
      freeMap(&tmps1);
      free(tmps1);
    }
  }
  else
#ifdef USE_PYTHON
    if(strncasecmp(r_inputs->value,"PYTHON",6)==0){
      *eres=zoo_python_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
    }
    else
#endif
	
#ifdef USE_JAVA
      if(strncasecmp(r_inputs->value,"JAVA",4)==0){
	*eres=zoo_java_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
      }
      else
#endif

#ifdef USE_PHP
	if(strncasecmp(r_inputs->value,"PHP",3)==0){
	  *eres=zoo_php_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	}
	else
#endif
	    
	    
#ifdef USE_PERL
          if(strncasecmp(r_inputs->value,"PERL",4)==0){
            *eres=zoo_perl_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
          }
          else
#endif

#ifdef USE_JS
	    if(strncasecmp(r_inputs->value,"JS",2)==0){
	      *eres=zoo_js_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
	    }
	    else
#endif

#ifdef USE_RUBY
          if(strncasecmp(r_inputs->value,"Ruby",4)==0){
            *eres=zoo_ruby_support(&m,request_inputs,s1,&request_input_real_format,&request_output_real_format);
          }
          else
#endif

	      {
		char tmpv[1024];
		sprintf(tmpv,_("Programming Language (%s) set in ZCFG file is not currently supported by ZOO Kernel.\n"),r_inputs->value);
		map* tmps=createMap("text",tmpv);
		printExceptionReportResponse(m,tmps);
		*eres=-1;
	      }
  *myMap=m;
  *ioutputs=request_output_real_format;
}


#ifdef WIN32
/**
 * createProcess function: create a new process after setting some env variables
 */
void createProcess(maps* m,map* request_inputs,service* s1,char* opts,int cpid, maps* inputs,maps* outputs){
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory( &si, sizeof(si) );
  si.cb = sizeof(si);
  ZeroMemory( &pi, sizeof(pi) );
  char *tmp=(char *)malloc((1024+cgiContentLength)*sizeof(char));
  char *tmpq=(char *)malloc((1024+cgiContentLength)*sizeof(char));
  map *req=getMap(request_inputs,"request");
  map *id=getMap(request_inputs,"identifier");
  map *di=getMap(request_inputs,"DataInputs");

  char *dataInputsKVP=getMapsAsKVP(inputs,cgiContentLength,0);
  char *dataOutputsKVP=getMapsAsKVP(outputs,cgiContentLength,1);
#ifdef DEBUG
  fprintf(stderr,"DATAINPUTSKVP %s\n",dataInputsKVP);
  fprintf(stderr,"DATAOUTPUTSKVP %s\n",dataOutputsKVP);
#endif
  map *sid=getMapFromMaps(m,"lenv","sid");
  map* r_inputs=getMapFromMaps(m,"main","tmpPath");
  map* r_inputs1=getMap(request_inputs,"metapath");
  int hasIn=-1;
  if(r_inputs1==NULL){
    r_inputs1=createMap("metapath","");
    hasIn=1;
  }
  map* r_inputs2=getMap(request_inputs,"ResponseDocument");
  if(r_inputs2==NULL)
    r_inputs2=getMap(request_inputs,"RawDataOutput");
  map *tmpPath=getMapFromMaps(m,"lenv","cwd");

  map *tmpReq=getMap(request_inputs,"xrequest");
  if(r_inputs2!=NULL){
    sprintf(tmp,"\"metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s&%s=%s&cgiSid=%s\"",r_inputs1->value,req->value,id->value,dataInputsKVP,r_inputs2->name,dataOutputsKVP,sid->value);
    sprintf(tmpq,"metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s&%s=%s",r_inputs1->value,req->value,id->value,dataInputsKVP,r_inputs2->name,dataOutputsKVP);
  }
  else{
    sprintf(tmp,"\"metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s&cgiSid=%s\"",r_inputs1->value,req->value,id->value,dataInputsKVP,sid->value);
    sprintf(tmpq,"metapath=%s&request=%s&service=WPS&version=1.0.0&Identifier=%s&DataInputs=%s",r_inputs1->value,req->value,id->value,dataInputsKVP,sid->value);
  }
  
  if(hasIn>0){
    freeMap(&r_inputs1);
    free(r_inputs1);
  }
  char *tmp1=zStrdup(tmp);
  sprintf(tmp,"\"zoo_loader.cgi\" %s \"%s\"",tmp1,sid->value);
  
  free(dataInputsKVP);
  free(dataOutputsKVP);
#ifdef DEBUG
  fprintf(stderr,"REQUEST IS : %s \n",tmp);
#endif
  SetEnvironmentVariable("CGISID",TEXT(sid->value));
  SetEnvironmentVariable("QUERY_STRING",TEXT(tmpq));
  char clen[1000];
  sprintf(clen,"%d",strlen(tmpq));
  SetEnvironmentVariable("CONTENT_LENGTH",TEXT(clen));
  
  if( !CreateProcess( NULL,             // No module name (use command line)
		      TEXT(tmp),        // Command line
		      NULL,             // Process handle not inheritable
		      NULL,             // Thread handle not inheritable
		      FALSE,            // Set handle inheritance to FALSE
		      CREATE_NO_WINDOW, // Apache won't wait until the end
		      NULL,             // Use parent's environment block
		      NULL,             // Use parent's starting directory 
		      &si,              // Pointer to STARTUPINFO struct
		      &pi )             // Pointer to PROCESS_INFORMATION struct
      ) 
    { 
#ifdef DEBUG
      fprintf( stderr, "CreateProcess failed (%d).\n", GetLastError() );
#endif
      return ;
    }else{
#ifdef DEBUG
    fprintf( stderr, "CreateProcess successfull (%d).\n\n\n\n", GetLastError() );
#endif
  }
  CloseHandle( pi.hProcess );
  CloseHandle( pi.hThread );
#ifdef DEBUG
  fprintf(stderr,"CreateProcess finished !\n");
#endif
}
#endif

int runRequest(map** inputs)
{
  
#ifndef USE_GDB
  (void) signal(SIGSEGV,sig_handler);
  (void) signal(SIGTERM,sig_handler);
  (void) signal(SIGINT,sig_handler);
  (void) signal(SIGILL,sig_handler);
  (void) signal(SIGFPE,sig_handler);
  (void) signal(SIGABRT,sig_handler);
#endif

  map* r_inputs=NULL;
  map* request_inputs=*inputs;
  maps* m=NULL;
  char* REQUEST=NULL;
  /**
   * Parsing service specfic configuration file
   */
  m=(maps*)malloc(MAPS_SIZE);
  if(m == NULL){
    return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
  }
  char ntmp[1024];
#ifndef WIN32
  getcwd(ntmp,1024);
#else
  _getcwd(ntmp,1024);
#endif
  r_inputs=getMapOrFill(&request_inputs,"metapath","");


  char conf_file[10240];
  snprintf(conf_file,10240,"%s/%s/main.cfg",ntmp,r_inputs->value);
  if(conf_read(conf_file,m)==2){
    errorException(NULL, _("Unable to load the main.cfg file."),"InternalError",NULL);
    free(m);
    return 1;
  }
#ifdef DEBUG
  fprintf(stderr, "***** BEGIN MAPS\n"); 
  dumpMaps(m);
  fprintf(stderr, "***** END MAPS\n");
#endif

  map *getPath=getMapFromMaps(m,"main","gettextPath");
  if(getPath!=NULL){
    bindtextdomain ("zoo-kernel",getPath->value);
    bindtextdomain ("zoo-services",getPath->value);    
  }else{
    bindtextdomain ("zoo-kernel","/usr/share/locale/");
    bindtextdomain ("zoo-services","/usr/share/locale/");
  }


  /**
   * Manage our own error log file (usefull to separate standard apache debug
   * messages from the ZOO-Kernel ones but also for IIS users to avoid wrong 
   * headers messages returned by the CGI due to wrong redirection of stderr)
   */
  FILE * fstde=NULL;
  map* fstdem=getMapFromMaps(m,"main","logPath");
  if(fstdem!=NULL)
    fstde = freopen(fstdem->value, "a+", stderr) ;

  r_inputs=getMap(request_inputs,"language");
  if(r_inputs==NULL)
    r_inputs=getMapFromMaps(m,"main","language");
  if(r_inputs!=NULL){
    if(isValidLang(m,r_inputs->value)<0){
      char tmp[1024];
      sprintf(tmp,_("The value %s is not supported for the <language> parameter"),
	      r_inputs->value);
      errorException(m, tmp,"InvalidParameterValue","language");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      return 1;

    }
    char *tmp=zStrdup(r_inputs->value);
    setMapInMaps(m,"main","language",tmp);
#ifdef DEB
    char tmp2[12];
    sprintf(tmp2,"%s.utf-8",tmp);
    translateChar(tmp2,'-','_');
    setlocale (LC_ALL, tmp2);
#else
    translateChar(tmp,'-','_');
    setlocale (LC_ALL, tmp);
#endif
#ifndef WIN32
    setenv("LC_ALL",tmp,1);
#else
    char tmp1[12];
    sprintf(tmp1,"LC_ALL=%s",tmp);
    putenv(tmp1);
#endif
    free(tmp);
  }
  else{
    setlocale (LC_ALL, "en_US");
#ifndef WIN32
    setenv("LC_ALL","en_US",1);
#else
    char tmp1[12];
    sprintf(tmp1,"LC_ALL=en_US");
    putenv(tmp1);
#endif
    setMapInMaps(m,"main","language","en-US");
  }
  setlocale (LC_NUMERIC, "en_US");
  bind_textdomain_codeset("zoo-kernel","UTF-8");
  textdomain("zoo-kernel");
  bind_textdomain_codeset("zoo-services","UTF-8");
  textdomain("zoo-services");

  map* lsoap=getMap(request_inputs,"soap");
  if(lsoap!=NULL && strcasecmp(lsoap->value,"true")==0)
    setMapInMaps(m,"main","isSoap","true");
  else
    setMapInMaps(m,"main","isSoap","false");

  if(strlen(cgiServerName)>0){
    char tmpUrl[1024];
    if(strncmp(cgiServerPort,"80",2)==0){
      sprintf(tmpUrl,"http://%s%s",cgiServerName,cgiScriptName);
    }else{
      sprintf(tmpUrl,"http://%s:%s%s",cgiServerName,cgiServerPort,cgiScriptName);
    }
#ifdef DEBUG
    fprintf(stderr,"*** %s ***\n",tmpUrl);
#endif
    setMapInMaps(m,"main","serverAddress",tmpUrl);
  }

  /**
   * Check for minimum inputs
   */
  r_inputs=getMap(request_inputs,"Request");
  if(request_inputs==NULL || r_inputs==NULL){ 
    errorException(m, _("Parameter <request> was not specified"),"MissingParameterValue","request");
    if(count(request_inputs)==1){
      freeMap(&request_inputs);
      free(request_inputs);
    }
    freeMaps(&m);
    free(m);
    return 1;
  }
  else{
    REQUEST=zStrdup(r_inputs->value);
    if(strncasecmp(r_inputs->value,"GetCapabilities",15)!=0
       && strncasecmp(r_inputs->value,"DescribeProcess",15)!=0
       && strncasecmp(r_inputs->value,"Execute",7)!=0){ 
      errorException(m, _("Unenderstood <request> value. Please check that it was set to GetCapabilities, DescribeProcess or Execute."), "OperationNotSupported",r_inputs->value);
      freeMaps(&m);
      free(m);
      free(REQUEST);
      return 1;
    }
  }
  r_inputs=NULL;
  r_inputs=getMap(request_inputs,"Service");
  if(r_inputs==NULLMAP){
    errorException(m, _("Parameter <service> was not specified"),"MissingParameterValue","service");
    freeMaps(&m);
    free(m);
    free(REQUEST);
    return 1;
  }else{
    if(strcasecmp(r_inputs->value,"WPS")!=0){
      errorException(m, _("Unenderstood <service> value, WPS is the only acceptable value."), "InvalidParameterValue","service");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      return 1;
    }
  }
  if(strncasecmp(REQUEST,"GetCapabilities",15)!=0){
    r_inputs=getMap(request_inputs,"Version");
    if(r_inputs==NULL){ 
      errorException(m, _("Parameter <version> was not specified"),"MissingParameterValue","version");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      return 1;
    }else{
      if(strcasecmp(r_inputs->value,"1.0.0")!=0){
	errorException(m, _("Unenderstood <version> value, 1.0.0 is the only acceptable value."), "InvalidParameterValue","service");
	freeMaps(&m);
	free(m);
	free(REQUEST);
	return 1;
      }
    } 
  }else{
    r_inputs=getMap(request_inputs,"AcceptVersions");
    if(r_inputs!=NULL){ 
      if(strncmp(r_inputs->value,"1.0.0",5)!=0){
      errorException(m, _("Unenderstood <AcceptVersions> value, 1.0.0 is the only acceptable value."), "VersionNegotiationFailed",NULL);
	freeMaps(&m);
	free(m);
	free(REQUEST);
	return 1;
      }
    }
  }

  r_inputs=getMap(request_inputs,"serviceprovider");
  if(r_inputs==NULL){
    addToMap(request_inputs,"serviceprovider","");
  }

  maps* request_output_real_format=NULL;
  map* tmpm=getMapFromMaps(m,"main","serverAddress");
  if(tmpm!=NULL)
    SERVICE_URL=zStrdup(tmpm->value);
  else
    SERVICE_URL=zStrdup(DEFAULT_SERVICE_URL);

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
#ifdef DEBUG
    dumpMap(r_inputs);
#endif
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    r_inputs=NULL;
    r_inputs=getMap(request_inputs,"ServiceProvider");
    xmlNodePtr n;
    if(r_inputs!=NULL)
      n = printGetCapabilitiesHeader(doc,r_inputs->value,m);
    else
      n = printGetCapabilitiesHeader(doc,"",m);
    /**
     * Here we need to close stdout to ensure that not supported chars 
     * has been found in the zcfg and then printed on stdout
     */
    int saved_stdout = dup(fileno(stdout));
    dup2(fileno(stderr),fileno(stdout));
    if(int res=recursReaddirF(m,n,conf_dir,NULL,saved_stdout,0,printGetCapabilitiesForProcess)<0){
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      fflush(stdout);
      return res;
    }
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
      errorException(m, _("Mandatory <identifier> was not specified"),"MissingParameterValue","identifier");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      return 0;
    }

    struct dirent *dp;
    DIR *dirp = opendir(conf_dir);
    if(dirp==NULL){
      errorException(m, _("The specified path path doesn't exist."),"InvalidParameterValue",conf_dir);
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
      
      char *orig=zStrdup(r_inputs->value);

      int saved_stdout = dup(fileno(stdout));
      dup2(fileno(stderr),fileno(stdout));
      if(strcasecmp("all",orig)==0){
	if(int res=recursReaddirF(m,n,conf_dir,NULL,saved_stdout,0,printDescribeProcessForProcess)<0)
	  return res;
      }
      else{
	char *saveptr;
	char *tmps=strtok_r(orig,",",&saveptr);
	
	char buff[256];
	char buff1[1024];
	while(tmps!=NULL){
	  int hasVal=-1;
	  char *corig=strdup(tmps);
	  if(strstr(corig,".")!=NULL){
	    parseIdentifier(m,conf_dir,corig,buff1);
	    s1=(service*)malloc(SERVICE_SIZE);
	    t=readServiceFile(m,buff1,&s1,corig);

	    if(t<0){
	      map* tmp00=getMapFromMaps(m,"lenv","message");
	      char tmp01[1024];
	      if(tmp00!=NULL)
		sprintf(tmp01,_("Unable to parse the ZCFG file: %s (%s)"),dp->d_name,tmp00->value);
	      else
		sprintf(tmp01,_("Unable to parse the ZCFG file: %s."),dp->d_name);
	      dup2(saved_stdout,fileno(stdout));
	      errorException(m, tmp01,"InternalError",NULL);
	      freeMaps(&m);
	      free(m);
	      free(REQUEST);
	      return 1;
	    }
#ifdef DEBUG
	    dumpService(s1);
#endif
	    printDescribeProcessForProcess(m,n,s1);
	    freeService(&s1);
	    free(s1);
	    s1=NULL;
	    scount++;
	    setMapInMaps(m,"lenv","level","0");
	  }
	  
	  memset(buff,0,256);
	  snprintf(buff,256,"%s.zcfg",corig);
	  free(corig);
	  memset(buff1,0,1024);
#ifdef DEBUG
	  printf("\n#######%s\n########\n",buff);
#endif
	  while ((dp = readdir(dirp)) != NULL){
	    if( strcasecmp(dp->d_name,buff)==0 ){
	      memset(buff1,0,1024);
	      snprintf(buff1,1024,"%s/%s",conf_dir,dp->d_name);
	      s1=(service*)malloc(SERVICE_SIZE);
	      if(s1 == NULL){
		dup2(saved_stdout,fileno(stdout));
		return errorException(m, _("Unable to allocate memory."),"InternalError",NULL);
	      }
#ifdef DEBUG
	      printf("#################\n(%s) %s\n#################\n",r_inputs->value,buff1);
#endif
	      char *tmp0=zStrdup(dp->d_name);
	      tmp0[strlen(tmp0)-5]=0;
	      t=readServiceFile(m,buff1,&s1,tmp0);
	      free(tmp0);
	      if(t<0){
		map* tmp00=getMapFromMaps(m,"lenv","message");
		char tmp01[1024];
		if(tmp00!=NULL)
		  sprintf(tmp01,_("Unable to parse the ZCFG file: %s (%s)"),dp->d_name,tmp00->value);
		else
		  sprintf(tmp01,_("Unable to parse the ZCFG file: %s."),dp->d_name);
		dup2(saved_stdout,fileno(stdout));
		errorException(m, tmp01,"InternalError",NULL);
		freeMaps(&m);
		free(m);
		free(orig);
		free(REQUEST);
		closedir(dirp);
		xmlFreeDoc(doc);
		return 1;
	      }
#ifdef DEBUG
	      dumpService(s1);
#endif
	      printDescribeProcessForProcess(m,n,s1);
	      freeService(&s1);
	      free(s1);
	      s1=NULL;
	      scount++;
	      hasVal=1;
	    }
	  }
	  if(hasVal<0){
	    map* tmp00=getMapFromMaps(m,"lenv","message");
	    char tmp01[1024];
	    if(tmp00!=NULL)
	      sprintf(tmp01,_("Unable to parse the ZCFG file: %s (%s)"),buff,tmp00->value);
	    else
	      sprintf(tmp01,_("Unable to parse the ZCFG file: %s."),buff);
	    dup2(saved_stdout,fileno(stdout));
	    errorException(m, tmp01,"InvalidParameterValue","Identifier");
	    freeMaps(&m);
	    free(m);
	    free(orig);
	    free(REQUEST);
	    closedir(dirp);
	    xmlFreeDoc(doc);
	    return 1;
	  }
	  rewinddir(dirp);
	  tmps=strtok_r(NULL,",",&saveptr);
	}
      }
      closedir(dirp);
      fflush(stdout);
      dup2(saved_stdout,fileno(stdout));
      free(orig);
      printDocument(m,doc,getpid());
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      fflush(stdout);
      return 0;
    }
    else
      if(strncasecmp(REQUEST,"Execute",strlen(REQUEST))!=0){
	errorException(m, _("Unenderstood <request> value. Please check that it was set to GetCapabilities, DescribeProcess or Execute."), "InvalidParameterValue","request");
#ifdef DEBUG
	fprintf(stderr,"No request found %s",REQUEST);
#endif	
	closedir(dirp);
	freeMaps(&m);
	free(m);
	free(REQUEST);
	free(SERVICE_URL);
	fflush(stdout);
	return 0;
      }
    closedir(dirp);
  }
  
  s1=NULL;
  s1=(service*)malloc(SERVICE_SIZE);
  if(s1 == NULL){
    freeMaps(&m);
    free(m);
    free(REQUEST);
    free(SERVICE_URL);
    return errorException(m, _("Unable to allocate memory."),"InternalError",NULL);
  }
  r_inputs=getMap(request_inputs,"MetaPath");
  if(r_inputs!=NULL)
    snprintf(tmps1,1024,"%s/%s",ntmp,r_inputs->value);
  else
    snprintf(tmps1,1024,"%s/",ntmp);
  r_inputs=getMap(request_inputs,"Identifier");
  char *ttmp=zStrdup(tmps1);
  snprintf(tmps1,1024,"%s/%s.zcfg",ttmp,r_inputs->value);
  free(ttmp);
#ifdef DEBUG
  fprintf(stderr,"Trying to load %s\n", tmps1);
#endif
  if(strstr(r_inputs->value,".")!=NULL){
    char *identifier=zStrdup(r_inputs->value);
    parseIdentifier(m,conf_dir,identifier,tmps1);
    map* tmpMap=getMapFromMaps(m,"lenv","metapath");
    if(tmpMap!=NULL)
      addToMap(request_inputs,"metapath",tmpMap->value);
    free(identifier);
  }else
    setMapInMaps(m,"lenv","Identifier",r_inputs->value);
  int saved_stdout = dup(fileno(stdout));
  dup2(fileno(stderr),fileno(stdout));
  t=readServiceFile(m,tmps1,&s1,r_inputs->value);
  fflush(stdout);
  dup2(saved_stdout,fileno(stdout));
  if(t<0){
    char *tmpMsg=(char*)malloc(2048+strlen(r_inputs->value));
    sprintf(tmpMsg,_("The value for <indetifier> seems to be wrong (%s). Please, ensure that the process exist using the GetCapabilities request."),r_inputs->value);
    errorException(m, tmpMsg, "InvalidParameterValue","identifier");
    free(tmpMsg);
    free(s1);
    freeMaps(&m);
    free(m);
    free(REQUEST);
    free(SERVICE_URL);
    return 0;
  }
  close(saved_stdout);

#ifdef DEBUG
  dumpService(s1);
#endif
  int j;
  

  /**
   * Create the input and output maps data structure
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
    
#ifdef DEBUG
    fprintf(stderr,"OUTPUT Parsing ... \n");
#endif
    if(r_inputs!=NULL){
#ifdef DEBUG
      fprintf(stderr,"OUTPUT Parsing start now ... \n");
#endif
      char cursor_output[10240];
      char *cotmp=zStrdup(r_inputs->value);
      snprintf(cursor_output,10240,"%s",cotmp);
      free(cotmp);
      j=0;
	
      /**
       * Put each Output into the outputs_as_text array
       */
      char * pToken;
      maps* tmp_output=NULL;
#ifdef DEBUG
      fprintf(stderr,"OUTPUT [%s]\n",cursor_output);
#endif
      pToken=strtok(cursor_output,";");
      char** outputs_as_text=(char**)malloc(128*sizeof(char*));
      if(outputs_as_text == NULL) {
	return errorException(m, _("Unable to allocate memory"), "InternalError",NULL);
      }
      i=0;
      while(pToken!=NULL){
#ifdef DEBUG
	fprintf(stderr,"***%s***\n",pToken);
	fflush(stderr);
	fprintf(stderr,"***%s***\n",pToken);
#endif
	outputs_as_text[i]=(char*)malloc((strlen(pToken)+1)*sizeof(char));
	if(outputs_as_text[i] == NULL) {
	  return errorException(m, _("Unable to allocate memory"), "InternalError",NULL);
	}
	snprintf(outputs_as_text[i],strlen(pToken)+1,"%s",pToken);
	pToken = strtok(NULL,";");
	i++;
      }
      for(j=0;j<i;j++){
	char *tmp=zStrdup(outputs_as_text[j]);
	free(outputs_as_text[j]);
	char *tmpc;
	tmpc=strtok(tmp,"@");
	int k=0;
	while(tmpc!=NULL){
	  if(k==0){
	    if(tmp_output==NULL){
	      tmp_output=(maps*)malloc(MAPS_SIZE);
	      if(tmp_output == NULL){
		return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
	      }
	      tmp_output->name=zStrdup(tmpc);
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
    char cursor_input[40960];
    if(r_inputs!=NULL)
      snprintf(cursor_input,40960,"%s",r_inputs->value);
    else{
      errorException(m, _("Parameter <DataInputs> was not specified"),"MissingParameterValue","DataInputs");
      freeMaps(&m);
      free(m);
      free(REQUEST);
      free(SERVICE_URL);
      InternetCloseHandle(&hInternet);
      freeService(&s1);
      free(s1);
      return 0;
    }
    j=0;
  
    /**
     * Put each DataInputs into the inputs_as_text array
     */
    char *tmp1=zStrdup(cursor_input);
    char * pToken;
    pToken=strtok(cursor_input,";");
    if(pToken!=NULL && strncasecmp(pToken,tmp1,strlen(tmp1))==0){
      char* tmp2=url_decode(tmp1);
      snprintf(cursor_input,(strlen(tmp2)+1)*sizeof(char),"%s",tmp2);
      free(tmp2);
      pToken=strtok(cursor_input,";");
    }
    free(tmp1);

    char** inputs_as_text=(char**)malloc(100*sizeof(char*));
    if(inputs_as_text == NULL){
      return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
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
      inputs_as_text[i]=(char*)malloc((strlen(pToken)+1)*sizeof(char));
      snprintf(inputs_as_text[i],strlen(pToken)+1,"%s",pToken);
      if(inputs_as_text[i] == NULL){
	return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
      }
      pToken = strtok(NULL,";");
      i++;
    }

    for(j=0;j<i;j++){
      char *tmp=zStrdup(inputs_as_text[j]);
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
	if(tmpv!=NULL){
	  strncpy(tmpn,tmpc,(strlen(tmpc)-strlen(tmpv))*sizeof(char));
	  tmpn[strlen(tmpc)-strlen(tmpv)]=0;
	}
	else{
	  strncpy(tmpn,tmpc,strlen(tmpc)*sizeof(char));
	  tmpn[strlen(tmpc)]=0;
	}
#ifdef DEBUG
	fprintf(stderr,"***\n*** %s = %s ***\n",tmpn,tmpv+1);
#endif
	if(tmpmaps==NULL){
	  tmpmaps=(maps*)malloc(MAPS_SIZE);
	  if(tmpmaps == NULL){
	    return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
	  }
	  tmpmaps->name=zStrdup(tmpn);
	  if(tmpv!=NULL){
	    char *tmpvf=url_decode(tmpv+1);
	    tmpmaps->content=createMap("value",tmpvf);
	    free(tmpvf);
	  }
	  else
	    tmpmaps->content=createMap("value","Reference");
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
	  if(tmpv1!=NULL){
	    strncpy(tmpn1,tmpc,strlen(tmpc)-strlen(tmpv1));
	    tmpn1[strlen(tmpc)-strlen(tmpv1)]=0;
	    addToMap(tmpmaps->content,tmpn1,tmpv1+1);
	  }
	  else{
	    strncpy(tmpn1,tmpc,strlen(tmpc));
	    tmpn1[strlen(tmpc)]=0;
	    map* lmap=getLastMap(tmpmaps->content);
	    char *tmpValue=(char*)malloc((strlen(tmpv)+strlen(tmpc)+1)*sizeof(char));
	    sprintf(tmpValue,"%s@%s",tmpv+1,tmpc);
	    free(lmap->value);
	    lmap->value=zStrdup(tmpValue);
	    free(tmpValue);
	    tmpc=strtok(NULL,"@");
	    continue;
	  }
#ifdef DEBUG
	  fprintf(stderr,"*** NAME NON URL-ENCODED \n***%s***\n",tmpn1);
	  fprintf(stderr,"*** VALUE NON URL-ENCODED \n***%s***\n",tmpv1+1);
#endif
	  if(strcmp(tmpn1,"xlink:href")!=0)
	    addToMap(tmpmaps->content,tmpn1,tmpv1+1);
	  else
	    if(tmpv1!=NULL){
	      char *tmpx2=url_decode(tmpv1+1);
	      if(strncasecmp(tmpx2,"http://",7)!=0 &&
		 strncasecmp(tmpx2,"ftp://",6)!=0 &&
		 strncasecmp(tmpx2,"https://",8)!=0){
		char emsg[1024];
		sprintf(emsg,_("Unable to find a valid protocol to download the remote file %s"),tmpv1+1);
		errorException(m,emsg,"InternalError",NULL);
		freeMaps(&m);
		free(m);
		free(REQUEST);
		free(SERVICE_URL);
		InternetCloseHandle(&hInternet);
		freeService(&s1);
		free(s1);
		return 0;
	      }
#ifdef DEBUG
	      fprintf(stderr,"REQUIRE TO DOWNLOAD A FILE FROM A SERVER : url(%s)\n",tmpv1+1);
#endif
	      addToMap(tmpmaps->content,tmpn1,tmpx2);	      
#ifndef WIN32
	      if(CHECK_INET_HANDLE(hInternet))
#endif
		{
		  if(loadRemoteFile(&m,&tmpmaps->content,&hInternet,tmpx2)<0){
		    freeMaps(&m);
		    free(m);
		    free(REQUEST);
		    free(SERVICE_URL);
		    InternetCloseHandle(&hInternet);
		    freeService(&s1);
		    free(s1);
		    return 0;
		  }
		}
	      free(tmpx2);
	      addToMap(tmpmaps->content,"Reference",tmpv1+1);
	    }
	  tmpc=strtok(NULL,"@");
	}
#ifdef DEBUG
	dumpMaps(tmpmaps);
	fflush(stderr);
#endif
	if(request_input_real_format==NULL)
	  request_input_real_format=dupMaps(&tmpmaps);
	else{
	  maps* testPresence=getMaps(request_input_real_format,tmpmaps->name);
	  if(testPresence!=NULL){
	    elements* elem=getElements(s1->inputs,tmpmaps->name);
	    if(elem!=NULL){
	      if(appendMapsToMaps(m,request_input_real_format,tmpmaps,elem)<0){
		freeMaps(&m);
		free(m);
		free(REQUEST);
		free(SERVICE_URL);
		InternetCloseHandle(&hInternet);
		freeService(&s1);
		free(s1);
		return 0;
	      }
	    }
	  }
	  else
	    addMapsToMaps(&request_input_real_format,tmpmaps);
	}
	freeMaps(&tmpmaps);
	free(tmpmaps);
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
    /**
     * Parse every Input in DataInputs node.
     */
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
	while(cur2!=NULL){
	  while(cur2!=NULL && cur2->type!=XML_ELEMENT_NODE)
	    cur2=cur2->next;
	  if(cur2==NULL)
	    break;
	  /**
	   * Indentifier
	   */
	  if(xmlStrncasecmp(cur2->name,BAD_CAST "Identifier",xmlStrlen(cur2->name))==0){
	    xmlChar *val= xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
	    if(tmpmaps==NULL){
	      tmpmaps=(maps*)malloc(MAPS_SIZE);
	      if(tmpmaps == NULL){
		return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
	      }
	      tmpmaps->name=zStrdup((char*)val);
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
	      tmpmaps=(maps*)malloc(MAPS_SIZE);
	      if(tmpmaps == NULL){
		return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
	      }
	      tmpmaps->name=zStrdup("missingIndetifier");
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
	    const char *refs[5]={"mimeType","encoding","schema","method","href"};
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
		  if(!(ltmp!=NULL && strncmp(ltmp->value,"POST",4)==0)
		     && CHECK_INET_HANDLE(hInternet)){
		    if(loadRemoteFile(&m,&tmpmaps->content,&hInternet,(char*)val)!=0){
		      freeMaps(&m);
		      free(m);
		      free(REQUEST);
		      free(SERVICE_URL);
		      InternetCloseHandle(&hInternet);
		      freeService(&s1);
		      free(s1);
		      return 0;
		    }
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
	    /*	    HINTERNET hInternetP;
	    hInternetP=InternetOpen(
#ifndef WIN32
				   (LPCTSTR)
#endif
				   "ZooWPSClient\0",
				   INTERNET_OPEN_TYPE_PRECONFIG,
				   NULL,NULL, 0);*/
	    //hInternet.ihandle[hInternet.nb].header=NULL;
	    while(cur3!=NULL){
	      while(cur3!=NULL && cur3->type!=XML_ELEMENT_NODE)
		cur3=cur3->next;
	      if(cur3==NULL)
		break;
	      if(xmlStrcasecmp(cur3->name,BAD_CAST "Header")==0 ){
		const char *ha[2];
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
		    key=zStrdup((char*)val);
		  }else{
		    has=(char*)malloc((4+xmlStrlen(val)+strlen(key))*sizeof(char));
		    if(has == NULL){
		      return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
		    }
		    snprintf(has,(3+xmlStrlen(val)+strlen(key)),"%s: %s",key,(char*)val);
		    free(key);
#ifdef POST_DEBUG
		    fprintf(stderr,"%s\n",has);
#endif
		  }
		  xmlFree(val);
		}
		hInternet.ihandle[hInternet.nb].header=curl_slist_append(hInternet.ihandle[hInternet.nb].header, has);
		if(has!=NULL)
		  free(has);
	      }
	      else{
#ifdef POST_DEBUG
		fprintf(stderr,"Try to fetch the body part of the request ...\n");
#endif
		if(xmlStrcasecmp(cur3->name,BAD_CAST "Body")==0 ){
#ifdef POST_DEBUG
		  fprintf(stderr,"Body part found !!!\n",(char*)cur3->content);
#endif
		  char *tmp=(char*)malloc(cgiContentLength+1*sizeof(char));
		  memset(tmp,0,cgiContentLength);
		  xmlNodePtr cur4=cur3->children;
		  while(cur4!=NULL){
		    while(cur4->type!=XML_ELEMENT_NODE)
		      cur4=cur4->next;
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
		    xmlFree(btmps);
		    cur4=cur4->next;
		    xmlFreeDoc(bdoc);
		  }
		  map *btmp=getMap(tmpmaps->content,"href");
		  if(btmp!=NULL){
#ifdef POST_DEBUG
		    fprintf(stderr,"%s %s\n",btmp->value,tmp);
		    curl_easy_setopt(hInternet.handle, CURLOPT_VERBOSE, 1);
#endif
		    hInternet.waitingRequests[hInternet.nb]=strdup(tmp);
		    InternetOpenUrl(&hInternet,btmp->value,hInternet.waitingRequests[hInternet.nb],strlen(hInternet.waitingRequests[hInternet.nb]),
					INTERNET_FLAG_NO_CACHE_WRITE,0);
		  }
		  free(tmp);
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
		    if(!CHECK_INET_HANDLE(hInternet))
		      fprintf(stderr,"WARNING : hInternet handle failed to initialize");
#ifdef POST_DEBUG
		    curl_easy_setopt(bInternet.handle, CURLOPT_VERBOSE, 1);
#endif
		    bInternet.waitingRequests[0]=strdup((char*)val);
		    res1=InternetOpenUrl(&bInternet,bInternet.waitingRequests[0],NULL,0,
					 INTERNET_FLAG_NO_CACHE_WRITE,0);
		    processDownloads(&bInternet);
		    char* tmp=
		      (char*)malloc((bInternet.ihandle[0].nDataLen+1)*sizeof(char));
		    if(tmp == NULL){
		      return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
		    }
		    size_t bRead;
		    InternetReadFile(bInternet.ihandle[0], (LPVOID)tmp,
				     bInternet.ihandle[0].nDataLen, &bRead);
		    tmp[bInternet.ihandle[0].nDataLen]=0;
		    InternetCloseHandle(&bInternet);
		    map *btmp=getMap(tmpmaps->content,"href");
		    if(btmp!=NULL){
#ifdef POST_DEBUG
		      fprintf(stderr,"%s %s\n",btmp->value,tmp);
#endif
		      hInternet.waitingRequests[hInternet.nb]=strdup(tmp);
		      res=InternetOpenUrl(&hInternet,btmp->value,hInternet.waitingRequests[hInternet.nb],
					  strlen(hInternet.waitingRequests[hInternet.nb]),
					  INTERNET_FLAG_NO_CACHE_WRITE,0);
		    }
		    free(tmp);
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
	    while(cur4!=NULL){
	      while(cur4!=NULL &&cur4->type!=XML_ELEMENT_NODE)
		cur4=cur4->next;
	      if(cur4==NULL)
		break;
	      if(xmlStrcasecmp(cur4->name, BAD_CAST "LiteralData")==0){
		/**
		 * Get every attribute from a LiteralData node
		 * dataType , uom
		 */
		char *list[2];
		list[0]=zStrdup("dataType");
		list[1]=zStrdup("uom");
		for(int l=0;l<2;l++){
#ifdef DEBUG
		  fprintf(stderr,"*** LiteralData %s ***",list[l]);
#endif
		  xmlChar *val=xmlGetProp(cur4,BAD_CAST list[l]);
		  if(val!=NULL && strlen((char*)val)>0){
		    if(tmpmaps->content!=NULL)
		      addToMap(tmpmaps->content,list[l],(char*)val);
		    else
		      tmpmaps->content=createMap(list[l],(char*)val);
#ifdef DEBUG
		    fprintf(stderr,"%s\n",val);
#endif
		  }
		  xmlFree(val);
		  free(list[l]);		  
		}
	      }
	      else if(xmlStrcasecmp(cur4->name, BAD_CAST "ComplexData")==0){
		/**
		 * Get every attribute from a Reference node
		 * mimeType, encoding, schema
		 */
		const char *coms[3]={"mimeType","encoding","schema"};
		for(int l=0;l<3;l++){
#ifdef DEBUG
		  fprintf(stderr,"*** ComplexData %s ***\n",coms[l]);
#endif
		  xmlChar *val=xmlGetProp(cur4,BAD_CAST coms[l]);
		  if(val!=NULL && strlen((char*)val)>0){
		    if(tmpmaps->content!=NULL)
		      addToMap(tmpmaps->content,coms[l],(char*)val);
		    else
		      tmpmaps->content=createMap(coms[l],(char*)val);
#ifdef DEBUG
		    fprintf(stderr,"%s\n",val);
#endif
		  }
		  xmlFree(val);
		}
	      }

	      map* test=getMap(tmpmaps->content,"encoding");
	      if(test==NULL){
		if(tmpmaps->content!=NULL)
		  addToMap(tmpmaps->content,"encoding","utf-8");
		else
		  tmpmaps->content=createMap("encoding","utf-8");
		test=getMap(tmpmaps->content,"encoding");
	      }

	      if(strcasecmp(test->value,"base64")!=0){
		xmlChar* mv=xmlNodeListGetString(doc,cur4->xmlChildrenNode,1);
		map* ltmp=getMap(tmpmaps->content,"mimeType");
		if(mv==NULL || 
		   (xmlStrcasecmp(cur4->name, BAD_CAST "ComplexData")==0 &&
		    (ltmp==NULL || strncasecmp(ltmp->value,"text/xml",8)==0) )){
		  xmlDocPtr doc1=xmlNewDoc(BAD_CAST "1.0");
		  int buffersize;
		  xmlNodePtr cur5=cur4->children;
		  while(cur5!=NULL && cur5->type!=XML_ELEMENT_NODE && cur5->type!=XML_CDATA_SECTION_NODE)
		    cur5=cur5->next;
		  if(cur5!=NULL && cur5->type!=XML_CDATA_SECTION_NODE){
		    xmlDocSetRootElement(doc1,cur5);
		    xmlDocDumpFormatMemoryEnc(doc1, &mv, &buffersize, "utf-8", 1);
		    char size[1024];
		    sprintf(size,"%d",buffersize);
		    addToMap(tmpmaps->content,"size",size);
		    xmlFreeDoc(doc1);
		  }
		}
		if(mv!=NULL){
		  addToMap(tmpmaps->content,"value",(char*)mv);
		  xmlFree(mv);
		}
	      }else{
		xmlChar* tmp=xmlNodeListGetRawString(doc,cur4->xmlChildrenNode,0);
		addToMap(tmpmaps->content,"value",(char*)tmp);
		map* tmpv=getMap(tmpmaps->content,"value");
		char *res=NULL;
		char *curs=tmpv->value;
		for(int i=0;i<=strlen(tmpv->value)/64;i++) {
		  if(res==NULL)
		    res=(char*)malloc(67*sizeof(char));
		  else
		    res=(char*)realloc(res,(((i+1)*65)+i)*sizeof(char));
		  int csize=i*65;
		  strncpy(res + csize,curs,64);
		  if(i==xmlStrlen(tmp)/64)
		    strcat(res,"\n\0");
		  else{
		    strncpy(res + (((i+1)*64)+i),"\n\0",2);
		    curs+=64;
		  }
		}
		free(tmpv->value);
		tmpv->value=zStrdup(res);
		free(res);
		xmlFree(tmp);
	      }
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

	{
	  maps* testPresence=getMaps(request_input_real_format,tmpmaps->name);
	  if(testPresence!=NULL){
	    elements* elem=getElements(s1->inputs,tmpmaps->name);
	    if(elem!=NULL){
	      if(appendMapsToMaps(m,request_input_real_format,tmpmaps,elem)<0){
		freeMaps(&m);
		free(m);
		free(REQUEST);
		free(SERVICE_URL);
		InternetCloseHandle(&hInternet);
		freeService(&s1);
		free(s1);
		return 0;
	      }
	    }
	  }
	  else
	    addMapsToMaps(&request_input_real_format,tmpmaps);
	}

#ifdef DEBUG
	fprintf(stderr,"******TMPMAPS*****\n");
	dumpMaps(tmpmaps);
	fprintf(stderr,"******REQUESTMAPS*****\n");
	dumpMaps(request_input_real_format);
#endif
	freeMaps(&tmpmaps);
	free(tmpmaps);
	tmpmaps=NULL;	      
      }
#ifdef DEBUG
      dumpMaps(tmpmaps); 
#endif
    }
#ifdef DEBUG
    fprintf(stderr,"Search for response document node\n");
#endif
    xmlXPathFreeObject(tmpsptr);
    
    tmpsptr=extractFromDoc(doc,"/*/*/*[local-name()='ResponseDocument']");
    bool asRaw=false;
    tmps=tmpsptr->nodesetval;
    if(tmps->nodeNr==0){
      tmpsptr=extractFromDoc(doc,"/*/*/*[local-name()='RawDataOutput']");
      tmps=tmpsptr->nodesetval;
      asRaw=true;
    }
#ifdef DEBUG
    fprintf(stderr,"*****%d*****\n",tmps->nodeNr);
#endif
    for(int k=0;k<tmps->nodeNr;k++){
      if(asRaw==true)
	addToMap(request_inputs,"RawDataOutput","");
      else
	addToMap(request_inputs,"ResponseDocument","");
      maps *tmpmaps=NULL;
      xmlNodePtr cur=tmps->nodeTab[k];
      if(cur->type == XML_ELEMENT_NODE) {
	/**
	 * A specific responseDocument node.
	 */
	if(tmpmaps==NULL){
	  tmpmaps=(maps*)malloc(MAPS_SIZE);
	  if(tmpmaps == NULL){
	    return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
	  }
	  tmpmaps->name=zStrdup("unknownIdentifier");
	  tmpmaps->content=NULL;
	  tmpmaps->next=NULL;
	}
	/**
	 * Get every attribute: storeExecuteResponse, lineage, status
	 */
	const char *ress[3]={"storeExecuteResponse","lineage","status"};
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
	while(cur1!=NULL && cur1->type != XML_ELEMENT_NODE)
	  cur1=cur1->next;
	int cur1cnt=0;
	while(cur1){
	  /**
	   * Indentifier
	   */
	  if(xmlStrncasecmp(cur1->name,BAD_CAST "Identifier",xmlStrlen(cur1->name))==0){
	    xmlChar *val=
	      xmlNodeListGetString(doc,cur1->xmlChildrenNode,1);
	    if(tmpmaps==NULL){
	      tmpmaps=(maps*)malloc(MAPS_SIZE);
	      if(tmpmaps == NULL){
		return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
	      }
	      tmpmaps->name=zStrdup((char*)val);
	      tmpmaps->content=NULL;
	      tmpmaps->next=NULL;
	    }
	    else{
	      //free(tmpmaps->name);
	      tmpmaps->name=zStrdup((char*)val);
	    }
	    if(asRaw==true)
	      addToMap(request_inputs,"RawDataOutput",(char*)val);
	    else{
	      if(cur1cnt==0)
		addToMap(request_inputs,"ResponseDocument",(char*)val);
	      else{
		map* tt=getMap(request_inputs,"ResponseDocument");
		char* tmp=zStrdup(tt->value);
		free(tt->value);
		tt->value=(char*)malloc((strlen(tmp)+strlen((char*)val)+1)*sizeof(char));
		sprintf(tt->value,"%s;%s",tmp,(char*)val);
		free(tmp);
	      }
	    }
	    cur1cnt+=1;
	    xmlFree(val);
	  }
	  /**
	   * Title, Asbtract
	   */
	  else if(xmlStrncasecmp(cur1->name,BAD_CAST "Title",xmlStrlen(cur1->name))==0 ||
		  xmlStrncasecmp(cur1->name,BAD_CAST "Abstract",xmlStrlen(cur1->name))==0){
	    xmlChar *val=
	      xmlNodeListGetString(doc,cur1->xmlChildrenNode,1);
	    if(tmpmaps==NULL){
	      tmpmaps=(maps*)malloc(MAPS_SIZE);
	      if(tmpmaps == NULL){
		return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
	      }
	      tmpmaps->name=zStrdup("missingIndetifier");
	      tmpmaps->content=createMap((char*)cur1->name,(char*)val);
	      tmpmaps->next=NULL;
	    }
	    else{
	      if(tmpmaps->content!=NULL)
		addToMap(tmpmaps->content,(char*)cur1->name,(char*)val);
	      else
		tmpmaps->content=createMap((char*)cur1->name,(char*)val);
	    }
	    xmlFree(val);
	  }
	  else if(xmlStrncasecmp(cur1->name,BAD_CAST "Output",xmlStrlen(cur1->name))==0){
	    /**
	     * Get every attribute from a Output node
	     * mimeType, encoding, schema, uom, asReference
	     */
	    const char *outs[5]={"mimeType","encoding","schema","uom","asReference"};
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
	    while(cur2!=NULL && cur2->type != XML_ELEMENT_NODE)
	      cur2=cur2->next;
	    while(cur2){
	      /**
	       * Indentifier
	       */
	      if(xmlStrncasecmp(cur2->name,BAD_CAST "Identifier",xmlStrlen(cur2->name))==0){
		xmlChar *val=
		  xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
		if(tmpmaps==NULL){
		  tmpmaps=(maps*)malloc(MAPS_SIZE);
		  if(tmpmaps == NULL){
		    return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
		  }
		  tmpmaps->name=zStrdup((char*)val);
		  tmpmaps->content=NULL;
		  tmpmaps->next=NULL;
		}
		else{
		  if(tmpmaps->name!=NULL)
		    free(tmpmaps->name);
		  tmpmaps->name=zStrdup((char*)val);;
		}
		xmlFree(val);
	      }
	      /**
	       * Title, Asbtract
	       */
	      else if(xmlStrncasecmp(cur2->name,BAD_CAST "Title",xmlStrlen(cur2->name))==0 ||
		      xmlStrncasecmp(cur2->name,BAD_CAST "Abstract",xmlStrlen(cur2->name))==0){
		xmlChar *val=
		  xmlNodeListGetString(doc,cur2->xmlChildrenNode,1);
		if(tmpmaps==NULL){
		  tmpmaps=(maps*)malloc(MAPS_SIZE);
		  if(tmpmaps == NULL){
		    return errorException(m, _("Unable to allocate memory."), "InternalError",NULL);
		  }
		  tmpmaps->name=zStrdup("missingIndetifier");
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
	      while(cur2!=NULL && cur2->type != XML_ELEMENT_NODE)
		cur2=cur2->next;
	    }
	  }
	  cur1=cur1->next;
	  while(cur1!=NULL && cur1->type != XML_ELEMENT_NODE)
	    cur1=cur1->next;
	}
      }
      if(request_output_real_format==NULL)
	request_output_real_format=dupMaps(&tmpmaps);
      else
	addMapsToMaps(&request_output_real_format,tmpmaps);
      if(tmpmaps!=NULL){
	freeMaps(&tmpmaps);
	free(tmpmaps);
	tmpmaps=NULL;
      }
    }
    xmlXPathFreeObject(tmpsptr);
    xmlFreeDoc(doc);
    xmlCleanupParser();
  }
  
  runHttpRequests(&m,&request_input_real_format,&hInternet);

  //  if(CHECK_INET_HANDLE(hInternet))
  InternetCloseHandle(&hInternet);

#ifdef DEBUG
  fprintf(stderr,"\n%d\n",__LINE__);
  fflush(stderr);
  dumpMaps(request_input_real_format);
  dumpMaps(request_output_real_format);
  dumpMap(request_inputs);
  fprintf(stderr,"\n%d\n",__LINE__);
  fflush(stderr);
#endif

  /**
   * Ensure that each requested arguments are present in the request
   * DataInputs and ResponseDocument / RawDataOutput
   */
  char *dfv=addDefaultValues(&request_input_real_format,s1->inputs,m,0);
  maps *ptr=request_input_real_format;
  while(ptr!=NULL){
    map *tmp0=getMap(ptr->content,"size");
    map *tmp1=getMap(ptr->content,"maximumMegabytes");
    if(tmp1!=NULL && tmp0!=NULL){
      float i=atof(tmp0->value)/1048576.0;
      if(i>=atoi(tmp1->value)){
	char tmps[1024];
	map* tmpe=createMap("code","FileSizeExceeded");
	snprintf(tmps,1024,_("The <%s> parameter has a limited size (%sMB) defined in ZOO ServicesProvider configuration file but the reference you provided exceed this limitation (%fMB), please correct your query or the ZOO Configuration file."),ptr->name,tmp1->value,i);
	addToMap(tmpe,"locator",ptr->name);
	addToMap(tmpe,"text",tmps);
	printExceptionReportResponse(m,tmpe);
	freeService(&s1);
	free(s1);
	freeMap(&tmpe);
	free(tmpe);
	freeMaps(&m);
	free(m);
	free(REQUEST);
	free(SERVICE_URL);
	freeMaps(&request_input_real_format);
	free(request_input_real_format);
	freeMaps(&request_output_real_format);
	free(request_output_real_format);
	freeMaps(&tmpmaps);
	free(tmpmaps);
	return 1;
      }
    }
    ptr=ptr->next;
  }

  char *dfv1=addDefaultValues(&request_output_real_format,s1->outputs,m,1);
  if(strcmp(dfv1,"")!=0 || strcmp(dfv,"")!=0){
    char tmps[1024];
    map* tmpe=createMap("code","MissingParameterValue");
    if(strcmp(dfv,"")!=0){
      snprintf(tmps,1024,_("The <%s> argument was not specified in DataInputs but defined as requested in ZOO ServicesProvider configuration file, please correct your query or the ZOO Configuration file."),dfv);
      addToMap(tmpe,"locator",dfv);
    }
    else if(strcmp(dfv1,"")!=0){
      snprintf(tmps,1024,_("The <%s> argument was specified as Output identifier but not defined in the ZOO Configuration File. Please, correct your query or the ZOO Configuration File."),dfv1);
      addToMap(tmpe,"locator",dfv1);
    }
    addToMap(tmpe,"text",tmps);
    printExceptionReportResponse(m,tmpe);
    freeService(&s1);
    free(s1);
    freeMap(&tmpe);
    free(tmpe);
    freeMaps(&m);
    free(m);
    free(REQUEST);
    free(SERVICE_URL);
    freeMaps(&request_input_real_format);
    free(request_input_real_format);
    freeMaps(&request_output_real_format);
    free(request_output_real_format);
    freeMaps(&tmpmaps);
    free(tmpmaps);
    return 1;
  }
  maps* tmpReqI=request_input_real_format;
  while(tmpReqI!=NULL){
    char name[1024];
    if(getMap(tmpReqI->content,"isFile")!=NULL){
      if (cgiFormFileName(tmpReqI->name, name, sizeof(name)) == cgiFormSuccess) {
	int BufferLen=1024;
	cgiFilePtr file;
	int targetFile;
	char storageNameOnServer[2048];
	char fileNameOnServer[64];
	char contentType[1024];
	char buffer[1024];
	char *tmpStr=NULL;
	int size;
	int got,t;
	map *path=getMapFromMaps(m,"main","tmpPath");
	cgiFormFileSize(tmpReqI->name, &size);
	cgiFormFileContentType(tmpReqI->name, contentType, sizeof(contentType));
	if (cgiFormFileOpen(tmpReqI->name, &file) == cgiFormSuccess) {
	  t=-1;
	  while(1){
	    tmpStr=strstr(name+t+1,"\\");
	    if(NULL==tmpStr)
	      tmpStr=strstr(name+t+1,"/");
	    if(NULL!=tmpStr)
	      t=(int)(tmpStr-name);
	    else
	      break;
	  }
	  strcpy(fileNameOnServer,name+t+1);
	  
	  sprintf(storageNameOnServer,"%s/%s",path->value,fileNameOnServer);
#ifdef DEBUG
	  fprintf(stderr,"Name on server %s\n",storageNameOnServer);
	  fprintf(stderr,"fileNameOnServer: %s\n",fileNameOnServer);
#endif
	  targetFile = open (storageNameOnServer,O_RDWR|O_CREAT|O_TRUNC,S_IRWXU|S_IRGRP|S_IROTH);
	  if(targetFile<0){
#ifdef DEBUG
	    fprintf(stderr,"could not create the new file,%s\n",fileNameOnServer);	    
#endif
	  }else{
	    while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess){
	      if(got>0)
		write(targetFile,buffer,got);
	    }
	  }
	  addToMap(tmpReqI->content,"lref",storageNameOnServer);
	  cgiFormFileClose(file);
	  close(targetFile);
#ifdef DEBUG
	  fprintf(stderr,"File \"%s\" has been uploaded",fileNameOnServer);
#endif
	}
      }
    }
    tmpReqI=tmpReqI->next;
  }

  ensureDecodedBase64(&request_input_real_format);

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
      fprintf(stderr,"setting variable... %s\n",(
#endif
	      SetEnvironmentVariable(mapcs->name,mapcs->value)
#ifdef DEBUG
	      ==0)? "OK" : "FAILED");
#else
      ;
#endif
      char* toto=(char*)malloc((strlen(mapcs->name)+strlen(mapcs->value)+2)*sizeof(char));
      sprintf(toto,"%s=%s",mapcs->name,mapcs->value);
      putenv(toto);
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
  map* store=getMap(request_inputs,"storeExecuteResponse");
  map* status=getMap(request_inputs,"status");
  /**
   * 05-007r7 WPS 1.0.0 page 57 :
   * 'If status="true" and storeExecuteResponse is "false" then the service 
   * shall raise an exception.'
   */
  if(status!=NULL && strcmp(status->value,"true")==0 && 
     store!=NULL && strcmp(store->value,"false")==0){
    errorException(m, _("Status cannot be set to true with storeExecuteResponse to false. Please, modify your request parameters."), "InvalidParameterValue","storeExecuteResponse");
    freeService(&s1);
    free(s1);
    freeMaps(&m);
    free(m);
    
    freeMaps(&request_input_real_format);
    free(request_input_real_format);
    
    freeMaps(&request_output_real_format);
    free(request_output_real_format);
    
    free(REQUEST);
    free(SERVICE_URL);
    return 1;
  }
  r_inputs=getMap(request_inputs,"storeExecuteResponse");
  int eres=SERVICE_STARTED;
  int cpid=getpid();

  /**
   * Initialize the specific [lenv] section which contains runtime variables:
   * 
   *  - usid : it is an unique identification number 
   *  - sid : it is the process idenfitication number (OS)
   *  - status : value between 0 and 100 to express the  completude of 
   * the operations of the running service 
   *  - message : is a string where you can store error messages, in case 
   * service is failing, or o provide details on the ongoing operation.
   *  - cwd : is the current working directory
   *  - soap : is a boolean value, true if the request was contained in a SOAP 
   * Envelop 
   *  - sessid : string storing the session identifier (only when cookie is 
   * used)
   *  - cgiSid : only defined on Window platforms (for being able to identify 
   * the created process)
   *
   */
  maps *_tmpMaps=(maps*)malloc(MAPS_SIZE);
  _tmpMaps->name=zStrdup("lenv");
  char tmpBuff[100];
  sprintf(tmpBuff,"%i",(cpid+(int)time(NULL)));
  _tmpMaps->content=createMap("usid",tmpBuff);
  _tmpMaps->next=NULL;
  sprintf(tmpBuff,"%i",cpid);
  addToMap(_tmpMaps->content,"sid",tmpBuff);
  addToMap(_tmpMaps->content,"status","0");
  addToMap(_tmpMaps->content,"cwd",ntmp);
  addToMap(_tmpMaps->content,"message",_("No message provided"));
  map* ltmp=getMap(request_inputs,"soap");
  if(ltmp!=NULL)
    addToMap(_tmpMaps->content,"soap",ltmp->value);
  else
    addToMap(_tmpMaps->content,"soap","false");
  if(cgiCookie!=NULL && strlen(cgiCookie)>0){
    int hasValidCookie=-1;
    char *tcook=zStrdup(cgiCookie);
    char *tmp=NULL;
    map* testing=getMapFromMaps(m,"main","cookiePrefix");
    if(testing==NULL){
      tmp=zStrdup("ID=");
    }else{
      tmp=(char*)malloc((strlen(testing->value)+2)*sizeof(char));
      sprintf(tmp,"%s=",testing->value);
    }
    if(strstr(cgiCookie,";")!=NULL){
      char *token,*saveptr;
      token=strtok_r(cgiCookie,";",&saveptr);
      while(token!=NULL){
	if(strcasestr(token,tmp)!=NULL){
	  if(tcook!=NULL)
	    free(tcook);
	  tcook=zStrdup(token);
	  hasValidCookie=1;
	}
	token=strtok_r(NULL,";",&saveptr);
      }
    }else{
      if(strstr(cgiCookie,"=")!=NULL && strcasestr(cgiCookie,tmp)!=NULL){
	tcook=zStrdup(cgiCookie);
	hasValidCookie=1;
      }
      if(tmp!=NULL){
	free(tmp);
      }
    }
    if(hasValidCookie>0){
      addToMap(_tmpMaps->content,"sessid",strstr(tcook,"=")+1);
      char session_file_path[1024];
      map *tmpPath=getMapFromMaps(m,"main","sessPath");
      if(tmpPath==NULL)
	tmpPath=getMapFromMaps(m,"main","tmpPath");
      char *tmp1=strtok(tcook,";");
      if(tmp1!=NULL)
	sprintf(session_file_path,"%s/sess_%s.cfg",tmpPath->value,strstr(tmp1,"=")+1);
      else
	sprintf(session_file_path,"%s/sess_%s.cfg",tmpPath->value,strstr(cgiCookie,"=")+1);
      free(tcook);
      maps *tmpSess=(maps*)malloc(MAPS_SIZE);
      struct stat file_status;
      int istat = stat(session_file_path, &file_status);
      if(istat==0 && file_status.st_size>0){
	conf_read(session_file_path,tmpSess);
	addMapsToMaps(&m,tmpSess);
	freeMaps(&tmpSess);
	free(tmpSess);
      }
    }
  }
  addMapsToMaps(&m,_tmpMaps);
  freeMaps(&_tmpMaps);
  free(_tmpMaps);
  
#ifdef DEBUG
  dumpMap(request_inputs);
#endif
#ifdef WIN32
  char *cgiSidL=NULL;
  if(getenv("CGISID")!=NULL)
    addToMap(request_inputs,"cgiSid",getenv("CGISID"));
  map* test1=getMap(request_inputs,"cgiSid");
  if(test1!=NULL){
    cgiSid=test1->value;
    addToMap(request_inputs,"storeExecuteResponse","true");
    addToMap(request_inputs,"status","true");
    setMapInMaps(m,"lenv","sid",test1->value);
    status=getMap(request_inputs,"status");
  }
#endif
  char *fbkp,*fbkp1;
  FILE *f0,*f1;
  if(status!=NULL)
    if(strcasecmp(status->value,"false")==0)
      status=NULLMAP;
  if(status==NULLMAP){
    loadServiceAndRun(&m,s1,request_inputs,&request_input_real_format,&request_output_real_format,&eres);
  }
  else{
    int   pid;
#ifdef DEBUG
    fprintf(stderr,"\nPID : %d\n",cpid);
#endif

#ifndef WIN32
    pid = fork ();
#else
    if(cgiSid==NULL){
      createProcess(m,request_inputs,s1,NULL,cpid,request_input_real_format,request_output_real_format);
      pid = cpid;
    }else{
      pid=0;
      cpid=atoi(cgiSid);
    }
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
      fbkp=(char*)malloc((strlen(r_inputs->value)+strlen(r_inputs1->value)+1024)*sizeof(char));
      sprintf(fbkp,"%s/%s_%d.xml",r_inputs->value,r_inputs1->value,cpid);
      char* flog=(char*)malloc((strlen(r_inputs->value)+strlen(r_inputs1->value)+1024)*sizeof(char));
      sprintf(flog,"%s/%s_%d_error.log",r_inputs->value,r_inputs1->value,cpid);
#ifdef DEBUG
      fprintf(stderr,"RUN IN BACKGROUND MODE \n");
      fprintf(stderr,"son pid continue (origin %d) %d ...\n",cpid,getpid());
      fprintf(stderr,"\nFILE TO STORE DATA %s\n",r_inputs->value);
#endif
      freopen(flog, "w+", stderr);
      f0=freopen(fbkp , "w+", stdout);
#ifndef WIN32
      fclose(stdin);
#endif
      free(flog);
      /**
       * set status to SERVICE_STARTED and flush stdout to ensure full 
       * content was outputed (the file used to store the ResponseDocument).
       * The rewind stdout to restart writing from the bgining of the file,
       * this way the data will be updated at the end of the process run.
       */
      printProcessResponse(m,request_inputs,cpid,s1,r_inputs1->value,SERVICE_STARTED,
			   request_input_real_format,request_output_real_format);
#ifndef WIN32
      fflush(stdout);
      rewind(stdout);
#else
#endif
      fbkp1=(char*)malloc((strlen(r_inputs->value)+strlen(r_inputs1->value)+1024)*sizeof(char));
      sprintf(fbkp1,"%s/%s_final_%d.xml",r_inputs->value,r_inputs1->value,cpid);
      f1=freopen(fbkp1 , "w+", stdout);
      loadServiceAndRun(&m,s1,request_inputs,&request_input_real_format,&request_output_real_format,&eres);
    } else {
      /**
       * error server don't accept the process need to output a valid 
       * error response here !!!
       */
      eres=-1;
      errorException(m, _("Unable to run the child process properly"), "InternalError",NULL);
    }
  }

#ifdef DEBUG
  dumpMaps(request_output_real_format);
#endif
  if(eres!=-1)
    outputResponse(s1,request_input_real_format,
		   request_output_real_format,request_inputs,
		   cpid,m,eres);
  fflush(stdout);
  /**
   * Ensure that if error occurs when freeing memory, no signal will return
   * an ExceptionReport document as the result was already returned to the 
   * client.
   */
#ifndef USE_GDB
  (void) signal(SIGSEGV,donothing);
  (void) signal(SIGTERM,donothing);
  (void) signal(SIGINT,donothing);
  (void) signal(SIGILL,donothing);
  (void) signal(SIGFPE,donothing);
  (void) signal(SIGABRT,donothing);
#endif
  if(((int)getpid())!=cpid || cgiSid!=NULL){
    fclose(stdout);
    fclose(stderr);
    unhandleStatus(m);
    /**
     * Dump back the final file fbkp1 to fbkp
     */
    fclose(f0);
    fclose(f1);
    FILE* f2=fopen(fbkp1,"rb");
    FILE* f3=fopen(fbkp,"wb+");
    free(fbkp);
    fseek(f2,0,SEEK_END);
    long flen=ftell(f2);
    fseek(f2,0,SEEK_SET);
    char *tmps1=(char*)malloc((flen+1)*sizeof(char));
    fread(tmps1,flen,1,f2);
    fwrite(tmps1,1,flen,f3);
    fclose(f2);
    fclose(f3);
    unlink(fbkp1);
    free(fbkp1);
    free(tmps1);
  }

  freeService(&s1);
  free(s1);
  freeMaps(&m);
  free(m);
  
  freeMaps(&request_input_real_format);
  free(request_input_real_format);
  
  freeMaps(&request_output_real_format);
  free(request_output_real_format);
  
  free(REQUEST);
  free(SERVICE_URL);
#ifdef DEBUG
  fprintf(stderr,"Processed response \n");
  fflush(stdout);
  fflush(stderr);
#endif

  return 0;
}

