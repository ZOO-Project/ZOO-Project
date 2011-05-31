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
#ifdef WIN32
#include <windows.h>
#endif

extern "C" {
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include <dirent.h>
#include "service_internal.h"

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
#ifdef WIN32
  __declspec(dllexport)
#endif
  int GetStatus(maps*& conf,maps*& inputs,maps*& outputs){
    const char *params[2 + 1];
    int xmlLoadExtDtdDefaultValue;
    map* tmpMap=NULL,*tmpMmap=NULL, *tmpTmap=NULL;
    tmpMap=getMapFromMaps(inputs,"sid","value");
    tmpTmap=getMapFromMaps(conf,"main","tmpPath");
    tmpMmap=getMapFromMaps(conf,"main","dataPath");
    xmlInitParser();
    struct dirent *dp;
    DIR *dirp = opendir(tmpTmap->value);
    char fileName[1024],xslFileName[1024];
    int hasFile=-1;
    if(dirp!=NULL){
      char tmp[128];
      sprintf(tmp,"_%s.xml",tmpMap->value);
      while ((dp = readdir(dirp)) != NULL)
	if(strstr(dp->d_name,tmp)!=0){
	  sprintf(fileName,"%s/%s",tmpTmap->value,dp->d_name);
	  hasFile=1;
	}
    }else{
      char tmp[1024];
      snprintf(tmp,1024,_ss("GetStatus was unable to use the tmpPath value set in main.cfg file as directory %s."),tmpTmap->value);
      setMapInMaps(conf,"lenv","message",tmp);
      return SERVICE_FAILED;
    }
    if(hasFile<0){
      char tmp[1024];
      snprintf(tmp,1024,_ss("GetStatus was unable to find any cache file for Service ID %s."),tmpMap->value);
      setMapInMaps(conf,"lenv","message",tmp);
      return SERVICE_FAILED;
    }
    sprintf(xslFileName,"%s/updateStatus.xsl",tmpMmap->value);
    xmlSubstituteEntitiesDefault(1);
    xmlLoadExtDtdDefaultValue = 0;
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, res;
    cur = xsltParseStylesheetFile(BAD_CAST xslFileName);
    doc = xmlParseFile(fileName);
    if(cur!=NULL && doc!=NULL){
      params[0]="value";
      params[1]=getStatus(atoi(tmpMap->value));
      params[2]=NULL;
      res = xsltApplyStylesheet(cur, doc, params);
      xmlChar *xmlbuff;
      int buffersize;
      xmlDocDumpFormatMemory(res, &xmlbuff, &buffersize, 1);
      setMapInMaps(outputs,"Result","value",(char*)xmlbuff);
      setMapInMaps(outputs,"Result","mimeType","text/xml");
      setMapInMaps(outputs,"Result","encoding","UTF-8");
      xmlFree(xmlbuff);
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
      char tmp[4];
      sprintf(tmp,"%i",i);
      map* tmpMap=NULL;
      tmpMap=getMapFromMaps(conf,"lenv","sid");
      if(tmpMap!=NULL)
	fprintf(stderr,"Status %s %s\n",tmpMap->value,tmp);
      setMapInMaps(conf,"lenv","status",tmp);
      updateStatus(conf);
#ifndef WIN32
      sleep(1);
#else
      Sleep(1000);
#endif
      i+=5;
    }
    setMapInMaps(outputs,"Result","value","\"Running long process successfully\"");
    return SERVICE_SUCCEEDED;
  }

}
