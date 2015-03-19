/**
 * Author : Gérald FENOY
 *
 *  Copyright 2008-2011 GeoLabs SARL. All rights reserved.
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
#include "fcgi_stdio.h"
*/
#include <unistd.h>
#include <fcgiapp.h>
#endif
#include <sys/wait.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include "service_internal.h"
#ifdef WIN32
#include "windows.h"
#define strtok_r strtok_s
#endif

extern "C"
{
#include "cgic.h"
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <sys/stat.h>
#include <ctype.h>

}

#include "service_zcfg.h"
#include "zoo_json.h"
#include "zoo_amqp.h"
#include "zoo_sql.h"
//#include "service_internal.h"


void
loadServiceAndRun (maps ** myMap, service * s1, map * request_inputs,
                   maps ** inputs, maps ** ioutputs, int *eres,FCGX_Stream *out, FCGX_Stream *err);

xmlXPathObjectPtr extractFromDoc (xmlDocPtr, const char *);
int runRequest (map **,struct cgi_env **,FCGX_Request *);

using namespace std;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE -1
#endif


static void PrintEnv(FCGX_Stream *out, char *label, char **envp)
{
    FCGX_FPrintF(out, "%s:<br>\n<pre>\n", label);
    for( ; *envp != NULL; envp++) {
        FCGX_FPrintF(out, "%s\n", *envp);
    }
    FCGX_FPrintF(out, "</pre><p>\n");
}

#define PATH_SOCKET "/tmp/zoo.sock"
#define THREAD_COUNT 50
static int counts[THREAD_COUNT];


int process(FCGX_Request *request){

      int pid = getpid();
      struct cgi_env *cgi;
      cgi = (struct cgi_env*)malloc(sizeof(struct cgi_env));
      cgiMain_init (NULL, NULL,&cgi,request);
      char *strQuery = NULL;
      if (cgi->cgiQueryString != NULL)
        strQuery = zStrdup (cgi->cgiQueryString);
      map *tmpMap = NULL;

      if (strncmp (cgi->cgiContentType, "text/xml", 8) == 0 ||
          strncasecmp (cgi->cgiRequestMethod, "post", 4) == 0)
        {
          if (cgi->cgiContentLength == 0)
            {
              char *post_data = NULL;
              int i = 0;
              int ch = FCGX_GetChar(request->in);
              while (ch != -1){
                {
                  i++;
                  if (post_data == NULL)
                    {
                    post_data=(char*)malloc(sizeof(char));
                    post_data[i-1] = (char) ch;
                    }
                  else
                    {
                    post_data=(char*)realloc(post_data,i*sizeof(char));
                    post_data[i-1] = (char) ch;
                    }
                  ch = FCGX_GetChar(request->in);
                  if (ch == -1 ){
                    post_data=(char*)realloc(post_data,(i + 1)*sizeof(char));
                    post_data[i] = '\0';
                    }
                }
                cgi->cgiContentLength = i;
              if (post_data == NULL && (strQuery == NULL || strlen (strQuery) == 0))
                {
                  return errorException (NULL,
                                         "ZOO-Kernel failed to process your request cause the request was emtpty.",
                                         "InternalError", NULL,request->out);
                }
              else
                {
                  if (strQuery == NULL || strlen (strQuery) == 0)
                    tmpMap = createMap ("request", post_data);
                }
              if (post_data != NULL)
                free (post_data);
                }
            }
          else
            {
              char *post_data = new char[cgi->cgiContentLength + 1];
              int r = FCGX_GetStr(post_data,cgi->cgiContentLength,request->in);
              if ( r > 0)
                {
                  post_data[r] = '\0';
                  cgi->cgiContentLength = r;
                  tmpMap = createMap ("request", post_data);
                }
              else
                {
                  post_data[0] = '\0';
                  char **array, **arrayStep;
                  if (cgiFormEntries (&array,&cgi) != cgiFormSuccess)
                    {
                      return 1;
                    }
                  arrayStep = array;
                  while (*arrayStep)
                    {
                      char *ivalue = new char[cgi->cgiContentLength];
                      cgiFormStringNoNewlines (*arrayStep, ivalue,
                                               cgi->cgiContentLength,&cgi);
                      char *tmpValueFinal =
                        (char *)
                        malloc ((strlen (*arrayStep) + strlen (ivalue) +
                                 1) * sizeof (char));
                      sprintf (tmpValueFinal, "%s=%s", *arrayStep, ivalue);
                      if (strlen (post_data) == 0)
                        {
                          sprintf (post_data, "%s", tmpValueFinal);
                        }
                      else
                        {
                          char *tmp = zStrdup (post_data);
                          sprintf (post_data, "%s&%s", tmp, tmpValueFinal);
                          free (tmp);
                        }
                      free (tmpValueFinal);
#ifdef DEBUG
                      fprintf (stderr, "(( \n %s \n %s \n ))", *arrayStep,
                               ivalue);
#endif
                      delete[]ivalue;
                      arrayStep++;
                    }
                  if (tmpMap != NULL)
                    addToMap (tmpMap, "request", post_data);
                  else
                    tmpMap = createMap ("request", post_data);
                }
              delete[]post_data;
            }
        }
      else
        {
#ifdef DEBUG
          dumpMap (tmpMap);
#endif
       
       char **array, **arrayStep;
          if (cgiFormEntries (&array,&cgi) != cgiFormSuccess)
            {
              return 1;
            }
          arrayStep = array;
          while (*arrayStep)
            {
              char *value = new char[cgi->cgiContentLength];
              cgiFormStringNoNewlines (*arrayStep, value, cgi->cgiContentLength,&cgi);
#ifdef DEBUG
              fprintf (stderr, "(( \n %s \n %s \n ))", *arrayStep, value);
#endif
              if (tmpMap != NULL)
                addToMap (tmpMap, *arrayStep, value);
              else
                tmpMap = createMap (*arrayStep, value);
              arrayStep++;
              delete[]value;
            }
          cgiStringArrayFree (array);
        }

#ifdef WIN32
      map *tmpReq = getMap (tmpMap, "rfile");
      if (tmpReq != NULL)
        {
          FILE *lf = fopen (tmpReq->value, "r");
          fseek (lf, 0, SEEK_END);
          long flen = ftell (lf);
          fseek (lf, 0, SEEK_SET);
          char *buffer = (char *) malloc ((flen + 1) * sizeof (char));
          fread (buffer, flen, 1, lf);
          fclose (lf);
          addToMap (tmpMap, "request", buffer);
          free (buffer);
          cgiContentLength = flen + 9;
        }
#endif
  /**
   * In case that the POST method was used, then check if params came in XML
   * format else try to use the attribute "request" which should be the only 
   * one.
   */
      if (strncasecmp (cgi->cgiRequestMethod, "post", 4) == 0 ||
          (count (tmpMap) == 1 && strncmp (tmpMap->value, "<", 1) == 0)
#ifdef WIN32
          || tmpReq != NULL
#endif
        )
        {
    /**
     * Store the original XML request in xrequest map
     */
          map *t1 = getMap (tmpMap, "request");
          if (t1 != NULL && strncasecmp (t1->value, "<", 1) == 0)
            {
              addToMap (tmpMap, "xrequest", t1->value);
              xmlInitParser ();
              xmlDocPtr doc = xmlParseMemory (t1->value, cgi->cgiContentLength);
              {
                xmlXPathObjectPtr reqptr = extractFromDoc (doc,
                                                           "/*[local-name()='Envelope']/*[local-name()='Body']/*");
                if (reqptr != NULL)
                  {
                    xmlNodeSet *req = reqptr->nodesetval;
                    if (req != NULL && req->nodeNr == 1)
                      {
                        addToMap (tmpMap, "soap", "true");
                        for (int k = 0; k < req->nodeNr; k++)
                          {
                            //xmlNsPtr ns=xmlNewNs(req->nodeTab[k],BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
                            xmlDocSetRootElement (doc, req->nodeTab[k]);
                            xmlChar *xmlbuff;
                            int buffersize;
                            xmlDocDumpFormatMemoryEnc (doc, &xmlbuff,
                                                       &buffersize, "utf-8",
                                                       1);
                            addToMap (tmpMap, "xrequest", (char *) xmlbuff);
                            xmlFree (xmlbuff);
                          }
                      }
                    xmlXPathFreeObject (reqptr);
                  }
              }

              xmlNodePtr cur = xmlDocGetRootElement (doc);
              char *tval;
              tval = NULL;
              tval = (char *) xmlGetProp (cur, BAD_CAST "service");
              if (tval != NULL)
                {
                  addToMap (tmpMap, "service", tval);
                  xmlFree (tval);
                }
              tval = NULL;
              tval = (char *) xmlGetProp (cur, BAD_CAST "language");
              if (tval != NULL)
                {
                  addToMap (tmpMap, "language", tval);
                  xmlFree (tval);
                }
              const char *requests[3] =
                { "GetCapabilities", "DescribeProcess", "Execute" };
              for (int j = 0; j < 3; j++)
                {
                  char tt[128];
                  sprintf (tt, "/*[local-name()='%s']", requests[j]);
                  xmlXPathObjectPtr reqptr = extractFromDoc (doc, tt);
                  if (reqptr != NULL)
                    {
                      xmlNodeSet *req = reqptr->nodesetval;
#ifdef DEBUG
                      fprintf (stderr, "%i", req->nodeNr);
#endif
                      if (req != NULL && req->nodeNr == 1)
                        {
                          if (t1->value != NULL)
                            free (t1->value);
                          t1->value = zStrdup (requests[j]);
                          j = 2;
                        }
                      xmlXPathFreeObject (reqptr);
                    }
                }
              if (strncasecmp (t1->value, "GetCapabilities", 15) == 0)
                {
                  xmlXPathObjectPtr versptr =
                    extractFromDoc (doc, "/*/*/*[local-name()='Version']");
                  xmlNodeSet *vers = versptr->nodesetval;
                  xmlChar *content = xmlNodeListGetString (doc,
                                                           vers->
                                                           nodeTab
                                                           [0]->xmlChildrenNode,
                                                           1);
                  addToMap (tmpMap, "version", (char *) content);
                  xmlXPathFreeObject (versptr);
                  xmlFree (content);
                }
              else
                {
                  tval = NULL;
                  tval = (char *) xmlGetProp (cur, BAD_CAST "version");
                  if (tval != NULL)
                    {
                      addToMap (tmpMap, "version", tval);
                      xmlFree (tval);
                    }
                  tval = (char *) xmlGetProp (cur, BAD_CAST "language");
                  if (tval != NULL)
                    {
                      addToMap (tmpMap, "language", tval);
                      xmlFree (tval);
                    }
                  xmlXPathObjectPtr idptr =
                    extractFromDoc (doc, "/*/*[local-name()='Identifier']");
                  if (idptr != NULL)
                    {
                      xmlNodeSet *id = idptr->nodesetval;
                      if (id != NULL)
                        {
                          char *identifiers = NULL;
                          identifiers =
                            (char *) calloc (cgi->cgiContentLength, sizeof (char));
                          identifiers[0] = 0;
                          for (int k = 0; k < id->nodeNr; k++)
                            {
                              xmlChar *content = xmlNodeListGetString (doc,
                                                                       id->nodeTab
                                                                       [k]->
                                                                       xmlChildrenNode,
                                                                       1);
                              if (strlen (identifiers) > 0)
                                {
                                  char *tmp = zStrdup (identifiers);
                                  snprintf (identifiers,
                                            strlen (tmp) +
                                            xmlStrlen (content) + 2, "%s,%s",
                                            tmp, content);
                                  free (tmp);
                                }
                              else
                                {
                                  snprintf (identifiers,
                                            xmlStrlen (content) + 1, "%s",
                                            content);
                                }
                              xmlFree (content);
                            }
                          xmlXPathFreeObject (idptr);
                          addToMap (tmpMap, "Identifier", identifiers);
                          free (identifiers);
                        }
                    }
                }
              xmlFreeDoc (doc);
              xmlCleanupParser ();
            }
          else
            {
              freeMap (&tmpMap);
              free (tmpMap);
              tmpMap = createMap ("not_valid", "true");
            }

          char *token, *saveptr;
          token = strtok_r (cgi->cgiQueryString, "&", &saveptr);
          while (token != NULL)
            {
              char *token1, *saveptr1;
              char *name = NULL;
              char *value = NULL;
              token1 = strtok_r (token, "=", &saveptr1);
              while (token1 != NULL)
                {
                  if (name == NULL)
                    name = zStrdup (token1);
                  else
                    value = zStrdup (token1);
                  token1 = strtok_r (NULL, "=", &saveptr1);
                }
              addToMap (tmpMap, name, value);
              free (name);
              free (value);
              name = NULL;
              value = NULL;
              token = strtok_r (NULL, "&", &saveptr);
            }

        }

      if (strncasecmp (cgi->cgiContentType, "multipart/form-data", 19) == 0)
        {
          map *tmp = getMap (tmpMap, "dataInputs");
          if (tmp != NULL)
            {
              addToMap (tmpMap, "dataInputs",
                        strstr (strQuery, "dataInputs=") + 11);
            }
        }

      if (strQuery != NULL)
        free (strQuery);
/*
      json_object *obj;
      maptojson(&obj,tmpMap);
      fprintf(stderr,"%s\n",json_object_to_json_string(obj));
      fflush(stderr);
  */    
      runRequest (&tmpMap,&cgi,request);

  /** 
   * Required but can't be made after executing a process using POST requests.
   */
      if ( /*strncasecmp(cgiRequestMethod,"post",4)!=0 && count(tmpMap)!=1 && */ tmpMap != NULL)
        {
          freeMap (&tmpMap);
          free (tmpMap);
        }
        // a verifier fait planter
      cgiFreeResources (&cgi);

      FCGX_Finish_r(request);
      return 0;
}



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
/* 
 json_object *jobj;
  mapstojson(&jobj,conf);
  fprintf (stderr,"The json object created: %s\n",json_object_to_json_string(jobj));
    freeMaps(&conf);
 
  maps *conf_tmp;
  jsontomaps(jobj,&conf_tmp);
  dumpMaps(conf_tmp);
   return 1;
*/
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

  int req_worker;
  map *m_req_worker = getMapFromMaps (conf, "server", "req_worker");
  if (m_req_worker == NULL){
    fprintf(stderr,"Configuration error: req_worker not found");
    return 2;
  }
  else {
    req_worker=atoi(m_req_worker->value);
    //freeMap(&m_req_worker);
    if (req_worker == 0){
        fprintf(stderr,"Configuration error: req_worker");
        return 2;
    }
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

  map *m_listen = getMapFromMaps (conf, "server", "listen");
  char *listen;
  if (m_listen == NULL){
    fprintf(stderr,"Configuration error: listen not found");
    return 2;
  }
  else {
    listen = (char *)malloc((strlen(m_listen->value) +1)*sizeof(char*));
    strncpy(listen,m_listen->value,strlen(m_listen->value));
    listen[strlen(m_listen->value)] = '\0';
    //freeMap(&m_listen);
  }
  int listen_owner;
  map *m_listen_owner = getMapFromMaps (conf, "server", "listen_owner");
  if (m_listen_owner == NULL){
    fprintf(stderr,"Configuration error: listen_owner");
    return 2;
  }
  else {
    listen_owner = atoi(m_listen_owner->value);
    //freeMap(&m_listen_owner);
    if (listen_owner == 0){
        fprintf(stderr,"Configuration error: listen_owner");
        return 2;
    }
  }

  int listen_group;
  map *m_listen_group = getMapFromMaps (conf, "server", "listen_group");
  if (m_listen_group == NULL){
    fprintf(stderr,"Configuration error: listen_group");
    return 2;
  }
  else {
    listen_group = atoi(m_listen_group->value);
    //freeMap(&m_listen_group);
    if (listen_group == 0){
        fprintf(stderr,"Configuration error: listen_group");
        return 2;
    }
  }

  char * listen_mode;
  map *m_listen_mode = getMapFromMaps (conf, "server", "listen_mode");
  if (m_listen_mode == NULL){
    fprintf(stderr,"Configuration error: listen_mode");
    return 2;
  }
  else {
    listen_mode = (char *)malloc((strlen(m_listen_mode->value) +1)*sizeof(char*));
    strncpy(listen_mode,m_listen_mode->value,strlen(m_listen_mode->value));
    listen_mode[strlen(m_listen_mode->value)] = '\0';
    //freeMap(&m_listen_mode);
  }

  int listen_queue;
  map *m_listen_queue = getMapFromMaps (conf, "server", "listen_queue");
  if (m_listen_queue == NULL){
    fprintf(stderr,"Configuration error: listen_queue");
    return 2;
  }
  else {
    listen_queue = atoi(m_listen_queue->value);
    //freeMap(&m_listen_queue);
    if (listen_queue == 0){
        fprintf(stderr,"Configuration error: listen_queue");
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

  char * status_user;
  map * m_status_user = getMapFromMaps (conf, "status", "user");
  if (m_status_user == NULL){
    fprintf(stderr,"Configuration error: [status] user");
    return 2;
  }
  else {
    status_user = (char *)malloc((strlen(m_status_user->value) +1)*sizeof(char*));
    strncpy(status_user,m_status_user->value,strlen(m_status_user->value));
    status_user[strlen(m_status_user->value)] = '\0';
  }


  char * status_passwd;
  map * m_status_passwd = getMapFromMaps (conf, "status", "passwd");
  if (m_status_passwd == NULL){
    fprintf(stderr,"Configuration error: [status] passwd");
    return 2;
  }
  else {
    status_passwd = (char *)malloc((strlen(m_status_passwd->value) +1)*sizeof(char*));
    strncpy(status_passwd,m_status_passwd->value,strlen(m_status_passwd->value));
    status_passwd[strlen(m_status_passwd->value)] = '\0';
  }

  char * status_bdd;
  map * m_status_bdd = getMapFromMaps (conf, "status", "bdd");
  if (m_status_bdd == NULL){
    fprintf(stderr,"Configuration error: [status] bdd");
    return 2;
  }
  else {
    status_bdd = (char *)malloc((strlen(m_status_bdd->value) +1)*sizeof(char*));
    strncpy(status_bdd,m_status_bdd->value,strlen(m_status_bdd->value));
    status_bdd[strlen(m_status_bdd->value)] = '\0';
  }

  char * status_host;
  map * m_status_host = getMapFromMaps (conf, "status", "host");
  if (m_status_host == NULL){
    fprintf(stderr,"Configuration error: [status] host");
    return 2;
  }
  else {
    status_host = (char *)malloc((strlen(m_status_host->value) +1)*sizeof(char*));
    strncpy(status_host,m_status_host->value,strlen(m_status_host->value));
    status_host[strlen(m_status_host->value)] = '\0';
  }

  int status_port;
  map *m_status_port = getMapFromMaps (conf, "status", "port");
  if (m_status_port == NULL){
    fprintf(stderr,"Configuration error: [status] port");
    return 2;
  }
  else {
    status_port = atoi(m_status_port->value);
    if (status_port == 0){
        fprintf(stderr,"Configuration error: [status] port");
        return 2;
    }
  }
  init_sql(status_host,status_user,status_passwd,status_bdd,status_port);

  int sock = FCGX_OpenSocket(listen, listen_queue); 
  init_services_conf (rootDir);
  
  ret = chown(listen, listen_owner, listen_group); 
  if (ret != 0){
    fprintf(stderr,"Change owner error on : %s\n",listen);
    return 3;
  }
  
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

  init_amqp(amqp_host,amqp_port,amqp_user, amqp_passwd, amqp_exchange, amqp_routingkey,amqp_queue);


  int fork_status = fork();
  if (fork_status == 0){
    //child
    int master_sync= getpid();
    fprintf(stderr,"Master sync%d\n",getpid());
    FCGX_Init();
    FCGX_Request request;
    FCGX_InitRequest(&request, sock, 0);
    int i;
    int count_request = 0;
    for (i = 0; i< req_worker; i++){
        fork_status = fork();
        if (fork_status == 0){
            fprintf(stderr,"child sync %d \n",getpid());
            fflush(stderr);
            break;
        }
    }
    while(1){
        /* mode synchrone */
        if (master_sync != getpid()){
            while(FCGX_Accept_r(&request) == 0){
                process(&request);
                count_request ++;
                if (count_request >= max_requests){
                    fprintf(stderr,"Max request stop process\n");
                    fflush(stderr);
                    exit(0);
                }
            }
        }
        else {
            wait(0);
            fprintf(stderr,"Master sync %d\n",getpid());
            fprintf(stderr,"New sync Child\n");
            fflush(stderr);
            fork();
        }
    }
  }
  else {
   int master_async = getpid();
   fprintf(stderr,"Master async %d\n",master_async);
    int fork_s;
    int j;
    for (j = 0; j< async_worker; j++){
        fork_s = fork();
        if (fork_s == 0){
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
                maps* tmp=request_input_real_format;
                HINTERNET hInternet = InternetOpen ((LPCTSTR) "ZooWPSClient\0",INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
                while(tmp!=NULL){
                    map * tmp_map = getMap(tmp->content,"xlink:href");
                    if (tmp_map != NULL){
                        if (loadRemoteFile(&map_c, &tmp_map, &hInternet,tmp_map->value) < 0) {
                            /* passer le status failed dans la base de donnÃ©e */
                            fprintf(stderr,"Erreur de chargement \n");
                        }
                    }
                    tmp=tmp->next;
                }
                runHttpRequests (&map_c, &request_input_real_format, &hInternet);
                InternetCloseHandle (&hInternet);
                free(tmp); 
                map * uuid = getMapFromMaps(map_c,"lenv","usid");
                if (uuid != NULL)
                    start_job(uuid->value);
                map *t=createMap("background","1");
                maps * lenv = getMaps(map_c,"lenv");
                addMapToMap(&lenv->content,t);
                freeMap(&t);
                free(t);
                
                map * m_identifier = getMap (request_inputs, "Identifier");
                
                service_identifier = zStrdup (m_identifier->value);

                s1 = search_service (service_identifier);
                free(service_identifier);
                

                //dumpMaps(request_input_real_format);

                loadServiceAndRun(&map_c, s1,request_inputs,&request_input_real_format, &request_output_real_format, &eres,NULL,NULL);
                if (eres == SERVICE_SUCCEEDED) {
                    outputResponse (s1,request_input_real_format,request_output_real_format,request_inputs, 0, map_c, eres,NULL,NULL);
                }
                    
                    
                //dumpMaps(request_output_real_format);
                //fprintf(stderr,"################################################################\n");
                //dumpMaps(map_c);

                outputResponse (s1,request_input_real_format,request_output_real_format,request_inputs, 0, map_c, eres,NULL,NULL);

                
                freeMaps(&map_c);
                map_c= NULL;
               
                freeMaps(&request_input_real_format);
                request_input_real_format = NULL;

                //dumpMap(request_inputs);
                freeMap(&request_inputs);
                request_inputs = NULL;
                
                //dumpMaps(request_output_real_format);
                freeMaps(&request_output_real_format);
                request_output_real_format = NULL;
                consumer_ack_amqp(c);
                

            }
            close_amqp();
            



            
        }
        else {
            wait(0);
            fprintf(stderr,"Master async %d\n",getpid());
            fprintf(stderr,"New async Child\n");
            fflush(stderr);
            fork();
        }
    }

  }
  
  return 0;
}
 
