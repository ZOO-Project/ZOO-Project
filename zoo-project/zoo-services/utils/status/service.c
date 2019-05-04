/**
 * Author : Gérald FENOY
 *
 * Copyright 2008-2009 GeoLabs SARL. All rights reserved.
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
extern "C" {

  /**
   * GetStatus ZOO Service :
   * This service is used in the ZOO-Project to get information about Services
   * running as background tasks. The service will first get the XML document 
   * cached by the ZOO-Kernel before calling effectively the Service, then 
   * will access the shared memory space created by the Kernel to extract the 
   * current status of the running Service. Using a simple XSL file it will 
   * finally produce the final ExecuteResponse including the updated 
   * percentCompleted attribute of the ProcessStarted node of the cached
   * document if any (so if the Service is currently running) else it will 
   * return the final ExecuteResponse stored on the Server file system.
   */
  ZOO_DLL_EXPORT int GetStatus(maps*& conf,maps*& inputs,maps*& outputs){	  
    const char *params[4 + 1];
    int xmlLoadExtDtdDefaultValue;
    map* tmpMap=NULL,*tmpMmap=NULL, *tmpTmap=NULL;

    tmpMap=getMapFromMaps(inputs,"sid","value");
    tmpTmap=getMapFromMaps(conf,"main","tmpPath");
    tmpMmap=getMapFromMaps(conf,"main","dataPath");
    if(tmpMmap==NULL)
      tmpMmap=tmpTmap;
    xmlInitParser();
    int hasFile=-1;
    char xslFileName[1024];
    char* mem=_getStatusFile(conf,tmpMap->value);
    if(mem==NULL){
      char tmp[1024];
      snprintf(tmp,1024,_ss("GetStatus was unable to find any cache file for Service ID %s."),tmpMap->value);
      setMapInMaps(conf,"lenv","message",tmp);
      return SERVICE_FAILED;
    }
    map* tmpMap1=getMapFromMaps(conf,"lenv","semaphore");
    if(tmpMap1!=NULL && strcasecmp(tmpMap1->value,"Created")==0)
      removeShmLock(conf,1);
    sprintf(xslFileName,"%s/updateStatus.xsl",tmpMmap->value);
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 0;
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;
    cur = xsltParseStylesheetFile(BAD_CAST xslFileName);
    doc = xmlParseMemory(mem,strlen(mem));
    //doc = xmlParseFile(fileName);
    if(cur!=NULL && doc!=NULL){
      /**
       * Parse Status to extract Status / Message
       */
      char *tmpStr=_getStatus(conf,tmpMap->value);
      //#ifdef DEBUG
      fprintf(stderr,"DEBUG: %s \n",tmpStr);
      //#endif
      if(tmpStr!=NULL && strncmp(tmpStr,"-1",2)!=0){
	char *tmpStr1=strdup(tmpStr);
	char *tmpStr0=strdup(strstr(tmpStr,"|")+1);
	free(tmpStr);
	tmpStr1[strlen(tmpStr1)-strlen(tmpStr0)-1]='\0';
	char *tmpStrFinal=(char*)malloc((strlen(tmpStr0)+11)*sizeof(char));
	sprintf(tmpStrFinal,"string(\"%s\")",tmpStr0);
	params[0]="value";
	params[1]=tmpStr1;
	params[2]="message";
	params[3]=tmpStrFinal;
	params[4]=NULL;
	res = xsltApplyStylesheet(cur, doc, params);
	xmlChar *xmlbuff;
	int buffersize;
	xmlDocDumpFormatMemory(res, &xmlbuff, &buffersize, 1);
	setMapInMaps(outputs,"Result","value",(char*)xmlbuff);
	xmlFree(xmlbuff);
	free(tmpStr1);
	free(tmpStr0);
	free(tmpStrFinal);
      }else{
	xmlChar *xmlbuff;
	int buffersize;
	xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
	setMapInMaps(outputs,"Result","value",(char*)xmlbuff);
	xmlFree(xmlbuff);
      }
    }
    else{
      char tmp[1024];
      sprintf(tmp,_ss("ZOO GetStatus Service was unable to parse the cache xml file available for the Service ID %s."),tmpMap->value);
      setMapInMaps(conf,"lenv","message",tmp);
      return SERVICE_FAILED;
    }
    return SERVICE_SUCCEEDED;
  }


  /**
   * longProcess ZOO Service :
   * Simple Service which just loop over 100 times then return a welcome message
   * string, at each step the service will sleep for one second.
   */
#ifdef WIN32
  __declspec(dllexport)
#endif
  int longProcess(maps*& conf,maps*& inputs,maps*& outputs){
    int i=0;
    while(i<100){
      char message[10];
      sprintf(message,"Step %d",i);
      fprintf (stderr, "RUN IN BACKGROUND MODE %s %d \n",__FILE__,__LINE__);
      fflush(stderr);
      updateStatus(conf,i,message);
#ifndef WIN32
      sleep(1);
#else
      Sleep(1000);
#endif
      i+=5;
    }
    setOutputValue(outputs,"Result",(char*)"\"Long process run successfully\"",-1);
    return SERVICE_SUCCEEDED;
  }

}
