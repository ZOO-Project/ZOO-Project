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
}

#include "service_zcfg.h"
//#include "service_internal.h"

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
      //PrintEnv(request.err, "Request environment", request.envp); 
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
  maps *main_config;
  int max_requests, start_servers;
  start_servers = 10;
  max_requests= 100;
  main_config = (maps *) malloc (MAP_SIZE);
  conf_read ("main.cfg", main_config);
  char ntmp[1024];
#ifndef WIN32
  getcwd (ntmp, 1024);
#else
  _getcwd (ntmp, 1024);
#endif
  char *rootDir = "/var/www/zoo-wps/cgi-bin";
  int fork_status = fork();
  if (fork_status == 0){
    //child
    int forker_pid = getpid();
    init_services_conf (rootDir);
    int sock = FCGX_OpenSocket(PATH_SOCKET, 1000);
    FCGX_Init();
    FCGX_Request request;
    FCGX_InitRequest(&request, sock, 0);
    int i;
    int count_request = 0;
    for (i = 0; i< start_servers; i++){
        fork_status = fork();
        if (fork_status == 0){
            fprintf(stderr,"child %d \n",i);
            fflush(stderr);
            break;
        }
    }
    while(1){
        if (forker_pid != getpid()){
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
            fprintf(stderr,"new child\n");
            fflush(stderr);
            fork();
        }
    }
  }
  else {
  
  while(1);



  }
  
  return 0;
}
 
