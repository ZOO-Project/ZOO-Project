/**
 * Author : David Saggiorato
 * 
 *  Copyright 2015 GeoLabs SARL. All rights reserved.
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

#define MALLOC_CHECK_ 0
#define MALLOC_CHECK 0

/**
 * Specific includes
 */
#include "fcgio.h"
#include "fcgi_config.h" 
#include "fcgi_stdio.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "service_internal.h"
#include "response_print.h"

extern "C" {
#include "cgic.h"
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
}


#include "service_internal.h"
#include "server_internal.h"
#include "request_parser.h"


#ifdef USE_JSON
#include "service_json.h"
#include "json_tokener.h"
#endif

#include "service_internal_amqp.h"


int runAsyncRequest(maps**,map**,map**,json_object *);

using namespace std;

int cgiInit(){
  fprintf(stderr,"ZOO-Kernel initialization %s %d ... \n",__FILE__,__LINE__);
  fflush(stderr);
  return 0;
}

/**
 * Main entry point for cgic.
 * @return 0 on sucess.
 */
int cgiMain(){
  /*}
    int
    main (int argc, char *argv[])
    {*/
  int debug_flag = 0;
  int background_flag = 0;
  char *file_value = cgiQueryString;
  int index;
  int c;
  opterr = 0;
  
  maps *conf=NULL;
  conf = (maps *) malloc (MAPS_SIZE);
  conf->content = NULL;
  conf->child = NULL;
  conf->next = NULL;
  int ret = conf_read (file_value, conf);
  if ( ret == 2){
    //a verifier mais conf_read ne renvoie jamais 0
    fprintf(stderr,_("Unable to load the %s file.\n"),file_value);
    return 1;
  }

  char ntmp[1024];
#ifndef ETC_DIR
#ifndef WIN32
    getcwd (ntmp, 1024);
#else
    _getcwd (ntmp, 1024);
#endif

    //  zGetcwd (ntmp, 1024);
#else
  sprintf(ntmp,"%s",ETC_DIR);
#endif
  char conf_file1[10240];
  snprintf (conf_file1, 10240, "%s/oas.cfg", ntmp);
  maps *oapi = (maps *) malloc (MAPS_SIZE);
  oapi->content = NULL;
  oapi->child = NULL;
  oapi->next = NULL;
  ret = conf_read (conf_file1, oapi);
  if ( ret == 2){
    fprintf(stderr,_("Unable to load the %s file.\n"),file_value);
    return 1;
  }
  addMapsToMaps(&conf,oapi);
  //dumpMaps(conf);
  
  int async_worker;
  map *m_async_worker = getMapFromMaps (conf, "server", "async_worker");  
  if (m_async_worker == NULL){
    fprintf(stderr,"Configuration error: async_worker not found");
    return 2;
  }
  else {
    async_worker = atoi(m_async_worker->value);
    //freeMap(&m_async_worker);
    if (async_worker == 0){
      fprintf(stderr,"Configuration error: req_worker");
      return 2;
    }
  }

  init_amqp(conf);
 
  int fork_status = fork();
  if (fork_status == 0){
    //child
    int master_async= getpid();
    int i;
    int count_request = 0;
  
    for (i = 0; i< async_worker; i++){
      fork_status = fork();
      if (fork_status == 0){
	fprintf(stderr,"# %d child async \n",getpid());
	fflush(stderr);
	break;
      }
    }
    
    json_object *msg_obj;
    json_object *maps_obj;
    maps * map_c;
    json_object *req_format_jobj;
    maps * request_input_real_format;
    json_object *lenv_jobj;
    json_object *req_jobj;
    map * request_inputs;
    json_object *outputs_jobj;
    maps * request_output_real_format;

    char *msg;
    int c;
    int eres;
    char * service_identifier;

    while(1){
      /* mode asynchrone */
      if( master_async != getpid()){
      reinit:
	/*traitement des requetes de la queue */
	bind_amqp();
	bind_queue();
	while(1){
	  init_consumer();
	  c = consumer_amqp(&msg);
	  if (c == 0)
	    break;
	  int consumed = consumer_ack_amqp(c);
	  if(consumed<0){
	    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	    fprintf(stderr,"Unable to acknowledge the message!\n");
	    fprintf(stderr,"# %d +++++++++++++++++++++++++++ %s %d \n",getpid(),__FILE__,__LINE__);
	    fflush(stderr);
	    return -1;
	  }

	  msg_obj = json_tokener_parse(msg);

#ifdef AMQP_DEBUG
	  fprintf(stderr,"##########################################@@\n",getpid());
	  fprintf(stderr,"# MSG TO TREAT:  %s\n",msg);
	  fprintf(stderr,"##########################################@@\n",getpid());
#endif //AMQP_DEBUG

	  json_object_object_get_ex(msg_obj,"request_inputs",&req_jobj);
	  map* mpReq=jsonToMap(req_jobj);

	  json_object_object_get_ex(msg_obj,"main_lenv",&lenv_jobj);
	  map* mpLenv=jsonToMap(lenv_jobj);

	  free(msg);
	  
	  runAsyncRequest(&conf,&mpLenv,&mpReq,msg_obj);

	  exit(0);
	}
	close_amqp();
      }
      else {
	fprintf(stderr,"##########################################@@\n",getpid());
	fprintf(stderr,"# Master async %d\n",getpid());
	fprintf(stderr,"# New async Child\n");
	fprintf(stderr,"##########################################@@\n",getpid());
	fflush(stderr);
	int pid=fork();
	if(pid==0)
	  goto reinit;
	else
	  wait(0);
      }
      zSleep(1);
    }
    
  }
  else
    wait(0);
  return 0;
}
 
