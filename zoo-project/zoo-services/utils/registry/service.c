/**
 * Author : Gérald FENOY
 *
 * Copyright 2015 GeoLabs SARL. All rights reserved.
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
#include "server_internal.h"
#include "response_print.h"

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
   * GetFromRegistry ZOO Service :
   * This service is used in the ZOO-Project to browse the profiles registry.
   */
#ifdef WIN32
  __declspec(dllexport)
#endif
  int GetFromRegistry(maps*& conf,maps*& inputs,maps*& outputs){
    const char *params[4 + 1];
    int xmlLoadExtDtdDefaultValue;
    map* tmpMap=NULL,*tmpMmap=NULL, *tmpTmap=NULL;

    map* reg = getMapFromMaps (conf, "main", "registry");
    registry* zooRegistry=NULL;
    if(reg!=NULL){
      int saved_stdout = dup (fileno (stdout));
      dup2 (fileno (stderr), fileno (stdout));
      createRegistry (conf,&zooRegistry,reg->value);
      dup2 (saved_stdout, fileno (stdout));
      close(saved_stdout);
    }else{
      setMapInMaps(conf,"lenv","message",_ss("No registry defined"));
      return SERVICE_FAILED;
    }
    tmpMap=getMapFromMaps(inputs,"id","value");
    if(strstr(tmpMap->value,"/")==NULL){
      setMapInMaps(conf,"lenv","message",_ss("Unable to parse your registry identifier"));
      return SERVICE_FAILED;
    }
    char* tmp=zStrdup(tmpMap->value);
    char* tmp1=zStrdup(strstr(tmpMap->value,"/")+1);
    tmp[strlen(tmp)-strlen(tmp1)-1]=0;
    service* inherited=getServiceFromRegistry(zooRegistry,tmp,tmp1);
    if(inherited!=NULL){
      dumpService(inherited);
      xmlDocPtr doc = xmlNewDoc (BAD_CAST "1.0");
      xmlNodePtr n = printWPSHeader(doc,conf,"DescribeProcess",
				    root_nodes[1][1],"2.0.0",1);
      setMapInMaps(conf,"main","rversion","2.0.0");
      printDescribeProcessForProcess(zooRegistry,conf,doc,n,inherited);
      setMapInMaps(conf,"main","rversion","1.0.0");


      xmlNodePtr n1 = xmlFirstElementChild(xmlFirstElementChild(n));


      xmlDocPtr doc1 = xmlNewDoc (BAD_CAST "1.0");
      int vid=1;
      char *tmp=(char*) malloc((86+strlen("DescribeProcess")+1)*sizeof(char));
      sprintf(tmp,schemas[vid][4],schemas[vid][2],schemas[vid][3],"DescribeProcess");
      xmlNewProp(n1,BAD_CAST "xsi:schemaLocation",BAD_CAST tmp);
      free(tmp);

      n1->ns=xmlCopyNamespaceList(n->ns);
      n1->nsDef=xmlCopyNamespaceList(n->nsDef);
      
      xmlDocSetRootElement(doc1, n1);
      
      xmlChar *xmlbuff;
      int buffersize;
      xmlDocDumpFormatMemoryEnc(doc1, &xmlbuff, &buffersize, "utf-8", 1);
      setMapInMaps(outputs,"Result","value",(char*)xmlbuff);
      xmlFree(xmlbuff);
      xmlFreeDoc(doc);
      xmlFreeDoc(doc1);
      return SERVICE_SUCCEEDED;
    }else{
      if(strcasecmp(tmp,"concept")==0){
	char *file_path=(char*)malloc((strlen(reg->value)+strlen(tmpMap->value)+5)*sizeof(char));
	sprintf(file_path,"%s/%s.txt",reg->value,tmpMap->value);
	FILE* f=fopen(file_path,"r");
	if(f!=NULL){
	  fseek(f, 0, SEEK_END);
	  long count = ftell(f);
	  fseek(f, 0, SEEK_SET);
	  setMapInMaps(outputs,"Result","value","");
	  map* tmpMap=getMapFromMaps(outputs,"Result","value");
	  free(tmpMap->value);
	  tmpMap->value=(char*) malloc((count+1)*sizeof(char));  
	  fread(tmpMap->value,1,count,f);
	  tmpMap->value[count]=0;
	  fclose(f);
	  setMapInMaps(outputs,"Result","mimeType","text/plain");
	  return SERVICE_SUCCEEDED;
	}else{
	  setMapInMaps(conf,"lenv","message",_ss("Unable to access this item in the registry."));
	  return SERVICE_FAILED;
	}
      }
      setMapInMaps(conf,"lenv","message",_ss("Unable to access this item in the registry."));
      return SERVICE_FAILED;
    }
    return SERVICE_SUCCEEDED;
  }

}
