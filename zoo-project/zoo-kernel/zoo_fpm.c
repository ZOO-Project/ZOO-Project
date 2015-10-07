/**
 * Author : David Saggiorato
 * 
 *  Copyright 2008-2015 GeoLabs SARL. All rights reserved.
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
#ifndef WIN32
/*
#include "fcgio.h"
#include "fcgi_config.h"
*/
//#include <stdio.h>
#include <unistd.h>
#include <fcgiapp.h>
#endif
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include "service_internal.h"

extern "C"
{
#include <string.h>
#include "logger.h"
#include <stdlib.h>
#include <glib.h>
#include <sys/stat.h>
#include <ctype.h>

}
#include "service.h"
#include "response_print.h"
#include "zoo_zcfg.h"
#include "zoo_json.h"
#include "zoo_amqp.h"
//#include "zoo_sql.h"
#include "server_internal.h"
#include "request_parser.h"
#include "logger.h"

void
loadServiceAndRun (maps ** myMap, service * s1, map * request_inputs,
                   maps ** inputs, maps ** ioutputs, int *eres,FCGX_Stream *out, FCGX_Stream *err);


using namespace std;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE -1
#endif


int
main (int argc, char *argv[])
{
 int debug_flag = 0;
 int background_flag = 0;
 char *file_value = NULL;
 int index;
 int c;
 opterr = 0;
 while ((c = getopt (argc, argv, "dbhf:")) != -1)
      switch (c)
      {
      case 'd':
        debug_flag = 1;
        break;
      case 'b':
        background_flag = 1;
        break;
      case 'h':
        fprintf(stderr,"TODO: need to print help\n");
        fflush(stderr);
        return 0;
      case 'f':
        file_value = optarg;
        break;
      case '?':
        if (optopt == 'f')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
        return 1;
      default:
        abort ();
      }

  maps *conf;
  conf = (maps *) malloc (MAPS_SIZE);
  
  int ret = conf_read (file_value, conf);
  if ( ret == 2){
    //a verifier mais conf_read ne renvoie jamais 0
    fprintf(stderr,"Erreur lors de la lecture de %s\n",file_value);
    return 1;
  }

  char *rootDir;
  map *m_rootDir = getMapFromMaps (conf, "server", "rootDir");
  if (m_rootDir == NULL){
    fprintf(stderr,"Configuration error: rootDir");
    return 2;
  }
  else {
   rootDir = (char*)malloc((strlen(m_rootDir->value) +1)*sizeof(char*));
   strncpy(rootDir,m_rootDir->value,strlen(m_rootDir->value));
   rootDir[strlen(m_rootDir->value)] = '\0';
   //freeMap(&m_rootDir);
  }

char *regDir;
  map *m_regDir = getMapFromMaps (conf, "main", "registry");
  if (m_regDir == NULL){
    fprintf(stderr,"Configuration error: regDir");
    return 2;
  }
  else {
   regDir = (char*)malloc((strlen(m_regDir->value) +1)*sizeof(char*));
   strncpy(regDir,m_regDir->value,strlen(m_regDir->value));
   regDir[strlen(m_regDir->value)] = '\0';
  }
  
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

  int max_requests;
  map *m_max_requests = getMapFromMaps (conf, "server", "max_requests");
  if (m_max_requests == NULL){
    fprintf(stderr,"Configuration error: max_requests");
    return 2;
  }
  else {
    max_requests = atoi(m_max_requests->value);
    //freeMap(&m_max_requests);
    if (max_requests == 0){
        fprintf(stderr,"Configuration error: max_requests");
        return 2;
    }
  }

  int id_user;
  map *m_user = getMapFromMaps (conf, "server", "uid");
  if (m_user == NULL){
    fprintf(stderr,"Configuration error: id_user");
    return 2;
  }
  else {
    id_user = atoi(m_user->value);
    //freeMap(&m_user);
    if (id_user == 0){
        fprintf(stderr,"Configuration error: id_user");
        return 2;
    }
  }


  int id_group;
  map *m_group = getMapFromMaps (conf, "server", "gid");
  if (m_group == NULL){
    fprintf(stderr,"Configuration error: gid");
    return 2;
  }
  else {
    id_group = atoi(m_group->value);
    //freeMap(&m_group);
    if (id_group == 0){
        fprintf(stderr,"Configuration error: id_group");
        return 2;
    }
  }


  char * amqp_host;
  map * m_amqp_host = getMapFromMaps (conf, "rabbitmq", "host");
  if (m_amqp_host == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] host");
    return 2;
  }
  else {
    amqp_host = (char *)malloc((strlen(m_amqp_host->value) +1)*sizeof(char*));
    strncpy(amqp_host,m_amqp_host->value,strlen(m_amqp_host->value));
    amqp_host[strlen(m_amqp_host->value)] = '\0';
 }

  int amqp_port;
  map *m_amqp_port = getMapFromMaps (conf, "rabbitmq", "port");
  if (m_amqp_port == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] port");
    return 2;
  }
  else {
    amqp_port = atoi(m_amqp_port->value);
    if (amqp_port == 0){
        fprintf(stderr,"Configuration error: [rabbitmq] port");
        return 2;
    }
  }

  char * amqp_user;
  map * m_amqp_user = getMapFromMaps (conf, "rabbitmq", "user");
  if (m_amqp_user == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] user");
    return 2;
  }
  else {
    amqp_user = (char *)malloc((strlen(m_amqp_user->value) +1)*sizeof(char*));
    strncpy(amqp_user,m_amqp_user->value,strlen(m_amqp_user->value));
    amqp_user[strlen(m_amqp_user->value)] = '\0';
 }

  char * amqp_passwd;
  map * m_amqp_passwd = getMapFromMaps (conf, "rabbitmq", "passwd");
  if (m_amqp_passwd == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] passwd");
    return 2;
  }
  else {
    amqp_passwd = (char *)malloc((strlen(m_amqp_passwd->value) +1)*sizeof(char*));
    strncpy(amqp_passwd,m_amqp_passwd->value,strlen(m_amqp_passwd->value));
    amqp_passwd[strlen(m_amqp_passwd->value)] = '\0';
 }

  char * amqp_exchange;
  map * m_amqp_exchange = getMapFromMaps (conf, "rabbitmq", "exchange");
  if (m_amqp_exchange == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] exchange");
    return 2;
  }
  else {
    amqp_exchange = (char *)malloc((strlen(m_amqp_exchange->value) +1)*sizeof(char*));
    strncpy(amqp_exchange,m_amqp_exchange->value,strlen(m_amqp_exchange->value));
    amqp_exchange[strlen(m_amqp_exchange->value)] = '\0';
 }

  char * amqp_routingkey;
  map * m_amqp_routingkey = getMapFromMaps (conf, "rabbitmq", "routingkey");
  if (m_amqp_routingkey == NULL){
    fprintf(stderr,"Configuration error: [amqp] routingkey");
    return 2;
  }
  else {
    amqp_routingkey = (char *)malloc((strlen(m_amqp_routingkey->value) +1)*sizeof(char*));
    strncpy(amqp_routingkey,m_amqp_routingkey->value,strlen(m_amqp_routingkey->value));
    amqp_routingkey[strlen(m_amqp_routingkey->value)] = '\0';
 }

  char * amqp_queue;
  map * m_amqp_queue = getMapFromMaps (conf, "rabbitmq", "queue");
  if (m_amqp_queue == NULL){
    fprintf(stderr,"Configuration error: [rabbitmq] queue");
    return 2;
  }
  else {
    amqp_queue = (char *)malloc((strlen(m_amqp_queue->value) +1)*sizeof(char*));
    strncpy(amqp_queue,m_amqp_queue->value,strlen(m_amqp_queue->value));
    amqp_queue[strlen(m_amqp_queue->value)] = '\0';
 }


  /*
  ret = setgid(id_group);
  if (ret != 0){
    fprintf(stderr,"Change gid error\n");
    return 3;
  }

  ret = setuid(id_user);
  if (ret != 0){
    fprintf(stderr,"Change uid error\n");
    return 3;
  }
*/


  init_services_conf (rootDir,regDir);
  init_amqp(amqp_host,amqp_port,amqp_user, amqp_passwd, amqp_exchange, amqp_routingkey,amqp_queue);





  int fork_status = fork();
  if (fork_status == 0){
    //child
    int master_async= getpid();
    int i;
    int count_request = 0;
    for (i = 0; i< async_worker; i++){
        fork_status = fork();
        if (fork_status == 0){
            fprintf(stderr,"child async %d \n",getpid());
            fflush(stderr);
            break;
        }
    }
    
    json_object *msg_obj;
    json_object *maps_obj;
    maps * map_c;
    json_object *req_format_jobj;
    maps * request_input_real_format;
    json_object *req_jobj;
    map * request_inputs;
    json_object *outputs_jobj;
    maps * request_output_real_format;

    char *msg;
    int c;
    int eres;
    char * service_identifier;
    service * s1 = NULL;
    
    HINTERNET hInternet;
    HINTERNET res;
    hInternet = InternetOpen (
			    "ZooWPSClient\0",
			    INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    
    while(1){
        /* mode asynchrone */
        if( master_async != getpid()){
            /*traitement des requetes de la queue */
            bind_amqp();
            init_consumer();
            while(1){
                
                c = consumer_amqp(&msg);
                if (c == 0)
                    break;
                msg_obj = json_tokener_parse(msg);
                
                free(msg);
                maps_obj = json_object_object_get(msg_obj,"maps");

                map_c = jsontomaps(maps_obj);

                req_format_jobj = json_object_object_get(msg_obj,"request_input_real_format");
                request_input_real_format = jsontomaps(req_format_jobj);

                req_jobj = json_object_object_get(msg_obj,"request_inputs");
                request_inputs = jsontomap(req_jobj);

                outputs_jobj = json_object_object_get(msg_obj,"request_output_real_format");
                request_output_real_format = jsontomaps(outputs_jobj);
                
                json_object_put(msg_obj);

                /* traitemement du message */
                
                /* Recherche des references */
                
                map * m_identifier = getMap (request_inputs, "Identifier");
                
                service_identifier = zStrdup (m_identifier->value);

                s1 = search_service (service_identifier);
                dumpService(s1);
                free(service_identifier);
                
                validateRequest(&map_c,s1,request_inputs, &request_input_real_format,&request_output_real_format,&hInternet,NULL);
                
                
                dumpMaps(request_input_real_format);

                loadServiceAndRun(&map_c, s1,request_inputs,&request_input_real_format, &request_output_real_format, &eres,NULL,NULL);
                /*
                if (eres == SERVICE_SUCCEEDED) {
                    outputResponse (s1,request_input_real_format,request_output_real_format,request_inputs, 0, map_c, eres,NULL,NULL);
                }
                */
                    
                dumpMaps(request_output_real_format);
                fprintf(stderr,"################################################################\n");
                dumpMaps(map_c);

                //outputResponse (s1,request_input_real_format,request_output_real_format,request_inputs, 0, map_c, eres,NULL,NULL);

                
                freeMaps(&map_c);
                map_c= NULL;
               
                freeMaps(&request_input_real_format);
                request_input_real_format = NULL;

                dumpMap(request_inputs);
                freeMap(&request_inputs);
                request_inputs = NULL;
                
                //dumpMaps(request_output_real_format);
                //freeMaps(&request_output_real_format);
                //request_output_real_format = NULL;
                consumer_ack_amqp(c);
                

            }
            close_amqp();
            
        }
        else {
            wait(0);
            fprintf(stderr,"Master async %d\n",getpid());
            fprintf(stderr,"New async Child\n");
            fflush(stderr);
            exit(1);
            fork();
        }
    }

  }
   while(1);
  return 0;
}
 
