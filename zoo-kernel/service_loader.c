/**
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2010 GeoLabs SARL
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

/**
 * Environment variables definitions.
 * All the following fixed values should be extracted from a configuration main
 * configuration file for all the potential services.
 */
/**
 * END "Environment variables definitions"
 */
#define length(x) (sizeof(x) / sizeof(x[0]))

extern "C" int yylex();
extern "C" int crlex();

#include <string.h>

#include "service.h"
#include "service_internal.h"
#include "service_internal_python.h"

#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#ifndef WIN32
#include <dlfcn.h>
#include <libgen.h>
#endif
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

void sigint_handler(int sig){
    printf("Not this time!\n");
}

int main(int argc, char *argv[])
{
  if (argc < 4){
    printf( "Usage: %s <servicename> <directory>|<definition_file> <REQUEST> [<functionname> <param_1>[...<param_n>]]\n", basename(argv[0]) );
    return 1;
  }
 
  map* outputs=NULL;
  /**
   * Parsing inputs (need a loop over all files in the service path !!)
   */
  maps* m;
  m=(maps*)malloc(MAP_SIZE);
  conf_read("main.cfg",m);
  map* tmpm=getMapFromMaps(m,"main","serverAddress");
  int toto=count(tmpm);
  //printf(" - %i \n",toto);

  if(tmpm!=NULL)
    SERVICE_URL=strdup(tmpm->value);
  else
    SERVICE_URL=DEFAULT_SERVICE_URL;

  service* s[100];
  service* s1;
  int scount=0;

  if(strcmp(argv[3],"GetCapabilities")==0){
    int i=0;
    struct dirent *dp;
    DIR *dirp = opendir(argv[1]);
    int t;
    xmlDocPtr doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNodePtr n = printGetCapabilitiesHeader(doc,argv[2],m);
    
    int saved_stdout = dup(fileno(stdout));
    stdout = freopen("/dev/null" , "w" , stdout);
    while ((dp = readdir(dirp)) != NULL)
      if(strstr(dp->d_name,".zcfg")!=0){
	char toto1[1024];
	sprintf(toto1,"%s%s",argv[1],dp->d_name);
	char *toto=toto1;
	s1=(service*)malloc(sizeof(char*)+(MAP_SIZE*2)+(2*ELEMENTS_SIZE));
	//s[scount]=(service*)malloc(sizeof(service*));
	//#ifdef DEBUG
	fprintf(stderr,"#################\n%s\n#################\n",toto1);
	//printf("|(1)");
	//#endif
	t=getServiceFromFile(toto1,&s1);
	
	//printf("|(2)");
	printGetCapabilitiesForProcess(m,n,s1);
	/**
	 * Valgrind told us that there is an issue regarding a 
	 * "conditional jump or move depends on uninitialised value(s)" for
	 * freeIOType
	 */
	//freeService(&s1);
	scount++;
      }
    char buf[20];
    sprintf(buf, "/dev/fd/%d", saved_stdout);
    stdout = freopen(buf , "w" , stdout);

    printDocument(doc);
    fflush(stdout);
    free(m);
    return 0;
  }
  else{
    s1=(service*)malloc(sizeof(char*)+(MAP_SIZE*2)+(2*ELEMENTS_SIZE));
    //s[0]=(service*)malloc(sizeof(service*));
    int t=getServiceFromFile(argv[1],&s1);
    if(strcmp(argv[3],"DescribeProcess")==0){
      printDescribeProcessResponse(s1,argv[2]);
      //dumpMaps(m);
      //free(s1);
      return 0;
    }
    else
      if(strcmp(argv[3],"Execute")!=0){
	fprintf(stderr,"");
	//free(s);
	return 0;
      }
  }
  //dumpService(s);
  s[0]=s1;
  map* inputs=NULL;
  elements* c_inputs=s1->inputs;
  int j;
  for(j=0;j<argc-5;j++){
    //dumpElements(c_inputs);
    if(inputs!=NULL)
      addToMap(inputs,c_inputs->name,argv[j+5]);
    else
      inputs=createMap(c_inputs->name,argv[j+5]);
    if(c_inputs->next!=NULL || j+1>=argc-5)
      c_inputs=c_inputs->next;
    else{
      map* tmps=createMap("text","ERROR you provided more inputs than requested.");
      printExceptionReportResponse(m,tmps);
      //printf("ERROR you provided more inputs than requested.");
      return -1;
    }
#ifdef DEBUG
    printf("ARGV1 %d %s\n",j,inputs->value);
#endif
  }

#ifdef DEBUG
  dumpMap(inputs);
#endif

  const struct tm *tm;
  size_t len;
  time_t now;
  char *sDate;
  
  now = time ( NULL );
  tm = localtime ( &now );

  sDate = new char[TIME_SIZE];

  len = strftime ( sDate, TIME_SIZE, "%d-%B-%YT%I:%M:%SZ", tm );

#ifdef DEBUG
  printf("Trying to load %s\n", argv[2]);
#endif
  void* so = dlopen(argv[2], RTLD_LAZY);
  char *errstr;
  errstr = dlerror();
  if( so != NULL ) {
    typedef int (*execute_t)(map**,map**);
#ifdef DEBUG
    printf("Library loaded %s \n",errstr);
#endif
    execute_t execute=(execute_t)dlsym(so,argv[4]);
#ifdef DEBUG
    errstr = dlerror();
    printf("Function loaded %s\n",errstr);
#endif	

    /**
     * Need to check if we need to fork to load a status enabled 
     */
    char _toto[10];
    sprintf(_toto,"input_%i",argc-5);
    map* toto=getMap(inputs,_toto);
    if(strcmp(argv[argc-1],"bg")!=0){
#ifdef DEBUG
      printf("RUN IN NORMAL MODE \n");
#endif
      int res=execute(&inputs,&outputs);
#ifdef DEBUG
      printf("RUNNED IN NORMAL MODE \n");
      dumpMap(inputs);
      dumpMap(outputs);
#endif
      printProcessResponse(m,getpid(),s[0],argv[2],res,inputs,outputs);
    }
    else{
      pid_t   pid;
      int cpid=getpid();
      pid = fork ();
      if (pid > 0) {
	/**
	 * dady :
	 * set status to SERVICE_ACCEPTED
	 */
	printProcessResponse(m,pid,s[0],argv[2],SERVICE_ACCEPTED,inputs,outputs);
      }else if (pid == 0) {
	/* son */
	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
	  printf("signal");
	  map* tmps=createMap("text","father received sigint.");
	  printExceptionReportResponse(m,tmps);
	  exit(1);
	}
#ifdef DEBUG
	printf("RUN IN BACKGROUND MODE \n");
#endif
	char tmp1[256];
	sprintf(tmp1,"service/temp/%s_%d.xml",argv[2],getpid());
	stdout = freopen(tmp1 , "w+" , stdout);
	/**
	 * set status to SERVICE_STARTED
	 */
	printProcessResponse(m,getpid(),s[0],argv[2],SERVICE_STARTED,inputs,outputs);
	fflush(stdout);
	rewind(stdout);
	int t=execute(&inputs,&outputs);
	/**
	 * set status to status code returned by the service function
	 */
	printProcessResponse(m,getpid(),s[0],argv[2],t,inputs,outputs);
      } else {
	/* error */
      }
    }
#ifdef DEBUG
    errstr = dlerror();
    printf("Function successfully loaded %s, unloading now.\n",errstr);
#endif
    dlclose(so);
  }
  else {
#ifdef DEBUG
    printf("C Library can't be loaded %s \n",errstr);
#endif
    python_support(m,s[0],argc,argv,inputs,outputs);
  }
  return 0;
}
