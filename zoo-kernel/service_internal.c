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

#include "service_internal.h"

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = (char*) malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_decode(char *str) {
  char *pstr = str, *buf = (char*) malloc(strlen(str) + 1), *pbuf = buf;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    } else if (*pstr == '+') { 
      *pbuf++ = ' ';
    } else {
      *pbuf++ = *pstr;
    }
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}


void printProcessResponse1(maps* m,map* request, int pid,service* serv,char* service,int status,maps* inputs,maps* outputs){
  if(getpid()==pid)
    printf("Content-Type: text/xml; charset=utf-8\r\nStatus: 200 OK\r\n\r\n");  
  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc,nc1,nc2,nc3,pseudor;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  int buffersize;
  time_t time1;  
  time(&time1);
  /**
   * Create the document and its temporary root.
   */
  doc = xmlNewDoc(BAD_CAST "1.0");
  pseudor = xmlNewNode(ns, BAD_CAST "root");
  ns_ows=xmlNewNs(pseudor,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");

  maps* tmp_maps=getMaps(m,"main");
  if(tmp_maps!=NULL){
    map* tmp=getMap(tmp_maps->content,"encoding");
    if(tmp!=NULL){
      doc->encoding = xmlCharStrdup(tmp->value);
    }
    else
      doc->encoding = xmlCharStrdup("UTF-8");
  }
  
  n = xmlNewNode(ns, BAD_CAST "ExecuteResponse");  
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/1999/xlink",BAD_CAST "xlink");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
  xmlNewProp(n,BAD_CAST "xsi:schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsExecute_response.xsd");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST "en");
  char tmp[256];
  char url[256];
  memset(tmp,0,256);
  memset(url,0,256);
  if(tmp_maps!=NULL){
    map* tmpm=getMap(tmp_maps->content,"serverAddress");
    map* tmpm1=getMap(tmp_maps->content,"tmpUrl");
    if(tmpm!=NULL && tmpm1!=NULL){
      sprintf(url,"%s/%s/%s_%i.xml",tmpm->value,tmpm1->value,service,pid);
    }
	if(tmpm1!=NULL)
      sprintf(tmp,"%s/",tmpm->value);
  }

  xmlNewProp(n,BAD_CAST "serviceInstance",BAD_CAST tmp);
  if(status!=SERVICE_SUCCEEDED){
    xmlNewProp(n,BAD_CAST "statusLocation",BAD_CAST url);
  }

  nc = xmlNewNode(ns, BAD_CAST "Process");
  map* tmp2=getMap(serv->content,"processVersion");

  if(tmp2!=NULL)
    xmlNewProp(nc,BAD_CAST "wps:processVersion",BAD_CAST tmp2->value);
  
  printDescription(nc,ns_ows,serv->name,serv->content);
  fflush(stderr);

  xmlAddChild(n,nc);

  nc = xmlNewNode(ns, BAD_CAST "Status");
  const struct tm *tm;
  size_t len;
  time_t now;
  char *tmp1;
  
  now = time ( NULL );
  tm = localtime ( &now );

  tmp1 = new char[TIME_SIZE];

  len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );

  switch(status){
  case SERVICE_SUCCEEDED:
    xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessSucceeded");
    break;
  case SERVICE_STARTED:
    xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessStarted");
    xmlNewProp(nc1,BAD_CAST "percentCompleted",BAD_CAST "NEED_SERVICE_ACCESS"); 
    break;
  case SERVICE_ACCEPTED:
    xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessAccepted");
    break;
  case SERVICE_FAILED:
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessFailed");
    break;
  default :
    printf("error code not know : %i\n",status);
    exit(1);
    break;
  }
  xmlAddChild(nc,nc1);
  xmlAddChild(n,nc);

#ifdef DEBUG
  fprintf(stderr,"printProcessResponse 1 161\n");
#endif

  map* lineage=getMap(request,"lineage");
  if(lineage!=NULL){
    nc = xmlNewNode(ns, BAD_CAST "DataInputs");
    int i;
    maps* mcursor=inputs;
    elements* scursor=serv->inputs;
    while(mcursor!=NULL && scursor!=NULL){
      printIOType1(doc,nc,ns,ns_ows,scursor,mcursor,"Input");
      mcursor=mcursor->next;
      scursor=scursor->next;
    }
    xmlAddChild(n,nc);
    
#ifdef DEBUG
    fprintf(stderr,"printProcessResponse 1 177\n");
#endif

    nc = xmlNewNode(ns, BAD_CAST "OutputDefinitions");
    mcursor=outputs;
    scursor=serv->outputs;
    while(mcursor!=NULL /*&& scursor!=NULL*/){
      printOutputDefinitions1(doc,nc,ns,ns_ows,scursor,mcursor,"Output");
      mcursor=mcursor->next;
      //scursor=scursor->next;
    }
    xmlAddChild(n,nc);
  }
#ifdef DEBUG
  fprintf(stderr,"printProcessResponse 1 190\n");
#endif

  /**
   * Display the process output only when requested !
   */
  if(status==SERVICE_SUCCEEDED){
    nc = xmlNewNode(ns, BAD_CAST "ProcessOutputs");
    maps* mcursor=outputs;
    elements* scursor=serv->outputs;
    while(mcursor!=NULL/* && scursor!=NULL*/){
      //xmlAddChild(nc,xmlNewComment(BAD_CAST "Here we need to check for output format from metadata"));
      printIOType1(doc,nc,ns,ns_ows,scursor,mcursor,"Output");
      mcursor=mcursor->next;
      //scursor=scursor->next;
    }
    xmlAddChild(n,nc);
  }
#ifdef DEBUG
  fprintf(stderr,"printProcessResponse 1 202\n");
#endif

  xmlAddChild(nr,n);
  xmlDocSetRootElement(doc, n);

  /*
   * Dump the document to a buffer and print it
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  xmlFree(nr);
  xmlFreeDoc(doc);

}


void printProcessResponse(maps* m,int pid,service* serv,char* service,int status,map* inputs,map* outputs){
  
  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc,nc1,nc2,nc3,pseudor;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  int buffersize;
  time_t time1;  
  time(&time1);
  /**
   * Create the document and its temporary root.
   */
  doc = xmlNewDoc(BAD_CAST "1.0");
  pseudor = xmlNewNode(ns, BAD_CAST "root");
  ns_ows=xmlNewNs(pseudor,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");

  maps* tmp_maps=getMaps(m,"main");
  if(tmp_maps!=NULL){
    map* tmp=getMap(tmp_maps->content,"encoding");
    if(tmp!=NULL){
      doc->encoding = xmlCharStrdup(tmp->value);
    }
    else
      doc->encoding = xmlCharStrdup("UTF-8");
  }
  
  n = xmlNewNode(ns, BAD_CAST "ExecuteResponse");  
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/1999/xlink",BAD_CAST "xlink");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
  xmlNewProp(n,BAD_CAST "xsi:schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsExecute_response.xsd");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST "en");
  char tmp[256];
  char url[256];
  memset(tmp,0,256);
  memset(url,0,256);
  if(tmp_maps!=NULL){
    map* tmpm=getMap(tmp_maps->content,"serverAddress");
    if(tmpm!=NULL){
      sprintf(url,"%s/temp/%s_%i.xml",tmpm->value,service,pid);
      sprintf(tmp,"%s/%s",tmpm->value,service);
    }
  }

  //sprintf(tmp,"%s/%s",SERVICE_URL,service);
  xmlNewProp(n,BAD_CAST "serviceInstance",BAD_CAST tmp);
  //memset(tmp,0,256);
  
  
  if(status!=SERVICE_SUCCEEDED){
    xmlNewProp(n,BAD_CAST "statusLocation",BAD_CAST url);
  }


  nc = xmlNewNode(ns, BAD_CAST "Process");
  map* tmp2=getMap(serv->content,"processVersion");

  if(tmp2!=NULL)
    xmlNewProp(nc,BAD_CAST "wps:processVersion",BAD_CAST tmp2->value);
  
  printDescription(nc,ns_ows,serv->name,serv->content);
  fflush(stderr);

  xmlAddChild(n,nc);

  nc = xmlNewNode(ns, BAD_CAST "Status");
  const struct tm *tm;
  size_t len;
  time_t now;
  char *tmp1;
  
  now = time ( NULL );
  tm = localtime ( &now );

  tmp1 = new char[TIME_SIZE];

  len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );

  switch(status){
  case SERVICE_SUCCEEDED:
    xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessSucceeded");
    break;
  case SERVICE_STARTED:
    xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessStarted");
    xmlNewProp(nc1,BAD_CAST "percentCompleted",BAD_CAST "NEED_SERVICE_ACCESS"); 
    break;
  case SERVICE_ACCEPTED:
    xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessSucceeded");
    break;
  case SERVICE_FAILED:
    nc1 = xmlNewNode(ns, BAD_CAST "ProcessFailed");
    break;
  default :
    printf("error code not know : %i\n",status);
    exit(1);
    break;
  }
  xmlAddChild(nc,nc1);
  xmlAddChild(n,nc);

  fprintf(stderr,"printProcessResponse 1\n");

  nc = xmlNewNode(ns, BAD_CAST "DataInputs");
  int i;
  map* mcursor=inputs;
  elements* scursor=serv->inputs;
  while(mcursor!=NULL && scursor!=NULL){
    //xmlAddChild(nc,xmlNewComment(BAD_CAST "Here we need to check for input format from metadata"));
    printIOType(doc,nc,ns,ns_ows,scursor,mcursor,"Input");
    mcursor=mcursor->next;
    scursor=scursor->next;
  }
  xmlAddChild(n,nc);

  fprintf(stderr,"printProcessResponse 1\n");

  nc = xmlNewNode(ns, BAD_CAST "OutputDefinitions");
  mcursor=outputs;
  scursor=serv->outputs;
  //dumpMap(mcursor);
  while(mcursor!=NULL && scursor!=NULL){
    //xmlAddChild(nc,xmlNewComment(BAD_CAST "Here we need to check for output format from metadata"));
    printOutputDefinitions(doc,nc,ns,ns_ows,scursor,mcursor,"Output");
    mcursor=mcursor->next;
    scursor=scursor->next;
  }
  xmlAddChild(n,nc);

  fprintf(stderr,"printProcessResponse 1\n");
  
  nc = xmlNewNode(ns, BAD_CAST "ProcessOutputs");
  mcursor=outputs;
  scursor=serv->outputs;
  while(mcursor!=NULL && scursor!=NULL){
    //xmlAddChild(nc,xmlNewComment(BAD_CAST "Here we need to check for output format from metadata"));
    printIOType(doc,nc,ns,ns_ows,scursor,mcursor,"Output");
    mcursor=mcursor->next;
    scursor=scursor->next;
  }
  xmlAddChild(n,nc);
  fprintf(stderr,"printProcessResponse 1\n");

  xmlAddChild(nr,n);
  xmlDocSetRootElement(doc, n);

  /*
   * Dump the document to a buffer and print it
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  xmlFreeDoc(doc);

}


void printGetCapabilitiesResponse(service** serv,int sc,char* service,maps* m){

  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  int buffersize;
  /**
   * Create the document and its temporary root.
   */
  nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");

  maps* toto1=getMaps(m,"main");
  doc = xmlNewDoc(BAD_CAST "1.0");
  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"encoding");
    if(tmp!=NULL){
      doc->encoding = xmlCharStrdup(tmp->value);
    }
    else
      doc->encoding = xmlCharStrdup("UTF-8");
  }
  else
    doc->encoding = xmlCharStrdup ("UTF-8");

  n = xmlNewNode(ns, BAD_CAST "Capabilities");  
  ns_ows=xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/1999/xlink",BAD_CAST "xlink");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
  xmlNewProp(n,BAD_CAST "xsi:schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsGetCapabilities_response.xsd");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  
  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"version");
    if(tmp!=NULL){
      xmlNewProp(n,BAD_CAST "version",BAD_CAST tmp->value);
    }
    else
      xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  }
  else
    xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");

  char tmp[256];
  
  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceIdentification");
  maps* tmp4=getMaps(m,"identification");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    while(tmp2!=NULL){
      if(strcmp(tmp2->name,"keywords")!=0 &&
	 strcmp(tmp2->name,"serverAddress")!=0 &&
	 strcmp(tmp2->name,"lang")!=0 &&
	 strcmp(tmp2->name,"encoding")!=0 &&
	 strcmp(tmp2->name,"version")!=0){
	tmp2->name[0]=toupper(tmp2->name[0]);
	nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	xmlAddChild(nc,nc1);
      }
      else
	if(strcmp(tmp2->name,"keywords")==0){
	  nc1 = xmlNewNode(ns_ows, BAD_CAST "Keywords");
	  char *toto=tmp2->value;
	  char buff[256];
	  int i=0;
	  int j=0;
	  while(toto[i]){
	    if(toto[i]!=',' && toto[i]!=0){
	      buff[j]=toto[i];
	      buff[j+1]=0;
	      j++;
	    }
	    else{
	      nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	      xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	      xmlAddChild(nc1,nc2);
	      j=0;
	    }
	    i++;
	  }
	  if(strlen(buff)>0){
	    nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	    xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	    xmlAddChild(nc1,nc2);
	  }
	  xmlAddChild(nc,nc1);
	}
      tmp2=tmp2->next;
    }
  }
  else{
    fprintf(stderr,"TMP4 NOT FOUND !!");
    //dumpMaps(tmp4);
    return;
  }
  xmlAddChild(n,nc);

  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceProvider");
  nc3 = xmlNewNode(ns_ows, BAD_CAST "ServiceContact");
  nc4 = xmlNewNode(ns_ows, BAD_CAST "ContactInfo");
  nc5 = xmlNewNode(ns_ows, BAD_CAST "Phone");
  nc6 = xmlNewNode(ns_ows, BAD_CAST "Address");
  tmp4=getMaps(m,"provider");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    while(tmp2!=NULL){
      if(strcmp(tmp2->name,"keywords")!=0 &&
	 strcmp(tmp2->name,"serverAddress")!=0 &&
	 strcmp(tmp2->name,"lang")!=0){
	tmp2->name[0]=toupper(tmp2->name[0]);
	if(strcmp(tmp2->name,"ProviderName")==0 || 
	   strcmp(tmp2->name,"ProviderSite")==0){
	  nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	  xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	  xmlAddChild(nc,nc1);
	}
	else{
	  if(strcmp(tmp2->name,"IndividualName")==0 || 
	     strcmp(tmp2->name,"PositionName")==0){
	    nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	    xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	    xmlAddChild(nc3,nc1);
	  } 
	  else 
	    if(strncmp(tmp2->name,"Phone",5)==0){
	      char *toto=NULL;
	      char *toto1=tmp2->name;
	      toto=strstr(toto1,"Phone");
	      nc1 = xmlNewNode(ns_ows, BAD_CAST toto1+5);
	      xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	      xmlAddChild(nc5,nc1);
	    }
	    else 
	      if(strncmp(tmp2->name,"Address",7)==0){
		char *toto=NULL;
		char *toto1=tmp2->name;
		toto=strstr(toto1,"Address");
		nc1 = xmlNewNode(ns_ows, BAD_CAST toto1+7);
		xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
		xmlAddChild(nc6,nc1);
	      }
	}
      }
      else
	if(strcmp(tmp2->name,"keywords")==0){
	  nc1 = xmlNewNode(ns_ows, BAD_CAST "Keywords");
	  char *toto=tmp2->value;
	  char buff[256];
	  int i=0;
	  int j=0;
	  while(toto[i]){
	    if(toto[i]!=',' && toto[i]!=0){
	      buff[j]=toto[i];
	      buff[j+1]=0;
	      j++;
	    }
	    else{
	      nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	      xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	      xmlAddChild(nc1,nc2);
	      j=0;
	    }
	    i++;
	  }
	  if(strlen(buff)>0){
	    nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	    xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	    xmlAddChild(nc1,nc2);
	  }
	  xmlAddChild(nc,nc1);
	}
      tmp2=tmp2->next;
    }
  }
  else{
    fprintf(stderr,"TMP4 NOT FOUND !!");
    //dumpMaps(tmp4);
  }
  xmlAddChild(nc4,nc5);
  xmlAddChild(nc4,nc6);
  xmlAddChild(nc3,nc4);
  xmlAddChild(nc,nc3);
  xmlAddChild(n,nc);


  nc = xmlNewNode(ns_ows, BAD_CAST "OperationsMetadata");
  char *tmp2[3];
  tmp2[0]="GetCapabilities";
  tmp2[1]="DescribeProcess";
  tmp2[2]="Execute";
  int j=0;


  for(j=0;j<3;j++){
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Operation");
    xmlNewProp(nc1,BAD_CAST "name",BAD_CAST tmp2[j]);
    nc2 = xmlNewNode(ns_ows, BAD_CAST "DCP");
    nc3 = xmlNewNode(ns_ows, BAD_CAST "HTTP");
    nc4 = xmlNewNode(ns_ows, BAD_CAST "Get");
    sprintf(tmp,"%s/%s",SERVICE_URL,service);
    xmlNewProp(nc4,BAD_CAST "xlink:href",BAD_CAST tmp);
    xmlAddChild(nc3,nc4);
    if(j>0){
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Post");
      xmlNewProp(nc4,BAD_CAST "xlink:href",BAD_CAST tmp);
      xmlAddChild(nc3,nc4);
    }
    xmlAddChild(nc2,nc3);
    xmlAddChild(nc1,nc2);    
    xmlAddChild(nc,nc1);    
  }
  xmlAddChild(n,nc);


  nc = xmlNewNode(ns, BAD_CAST "ProcessOfferings");
  /**
   * Need to loop over various operations
   */
  int cursor=0;
  map* tmp1;
  for(cursor=0;cursor<sc;cursor++){
    //dumpService(serv[cursor]);
    if(serv[cursor]->content!=NULL){
      nc1 = xmlNewNode(ns, BAD_CAST "Process");
      tmp1=getMap(serv[cursor]->content,"processVersion");
      if(tmp1!=NULL)
	xmlNewProp(nc1,BAD_CAST "wps:processVersion",BAD_CAST tmp1->value);
      
      printDescription(nc1,ns_ows,serv[cursor]->name,serv[cursor]->content);
      tmp1=serv[cursor]->metadata;
      while(tmp1!=NULL){
	nc2 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
	xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
	xmlAddChild(nc1,nc2);
	tmp1=tmp1->next;
      }
      xmlAddChild(nc,nc1);
      xmlAddChild(n,nc);
    }
  }


  nc1 = xmlNewNode(ns, BAD_CAST "Languages");
  nc2 = xmlNewNode(ns, BAD_CAST "Default");
  nc3 = xmlNewNode(ns, BAD_CAST "Supported");
  
  toto1=getMaps(m,"main");
  if(toto1!=NULL){
    tmp1=getMap(toto1->content,"lang");
    char *toto=tmp1->value;
    char buff[256];
    int i=0;
    int j=0;
    int dcount=0;
    while(toto[i]){
      if(toto[i]!=',' && toto[i]!=0){
	buff[j]=toto[i];
	buff[j+1]=0;
	j++;
      }
      else{
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	if(dcount==0){
	  xmlAddChild(nc2,nc4);
	  xmlAddChild(nc1,nc2);
	  xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST buff);
	  dcount++;
	}
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	xmlAddChild(nc3,nc4);
	j=0;
	buff[j]=0;
      }
      i++;
    }
    if(strlen(buff)>0){
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
      xmlAddChild(nc4,xmlNewText(BAD_CAST buff));	      
      xmlAddChild(nc3,nc4);
    }
  }
  xmlAddChild(nc1,nc3);
  xmlAddChild(n,nc1);
  
  xmlDocSetRootElement(doc, n);
    
  /*
   * Dump the document to a buffer and print it
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  //xmlFreeDoc(doc);

}

xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr doc,char* service,maps* m){

  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;
  //xmlDocPtr doc;
  xmlChar *xmlbuff;
  int buffersize;
  /**
   * Create the document and its temporary root.
   */
  nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");

  maps* toto1=getMaps(m,"main");
  //doc = xmlNewDoc(BAD_CAST "1.0");
  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"encoding");
    if(tmp!=NULL){
      doc->encoding = xmlCharStrdup(tmp->value);
    }
    else
      doc->encoding = xmlCharStrdup("UTF-8");
  }
  else
    doc->encoding = xmlCharStrdup ("UTF-8");

  n = xmlNewNode(ns, BAD_CAST "Capabilities");  
  ns_ows=xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/1999/xlink",BAD_CAST "xlink");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
  xmlNewProp(n,BAD_CAST "xsi:schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsGetCapabilities_response.xsd");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  
  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"version");
    if(tmp!=NULL){
      xmlNewProp(n,BAD_CAST "version",BAD_CAST tmp->value);
    }
    else
      xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  }
  else
    xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");

  char tmp[256];
  
  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceIdentification");
  maps* tmp4=getMaps(m,"identification");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    while(tmp2!=NULL){
      if(strcmp(tmp2->name,"keywords")!=0 &&
	 strcmp(tmp2->name,"serverAddress")!=0 &&
	 strcmp(tmp2->name,"lang")!=0 &&
	 strcmp(tmp2->name,"encoding")!=0 &&
	 strcmp(tmp2->name,"version")!=0){
	tmp2->name[0]=toupper(tmp2->name[0]);
	nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	xmlAddChild(nc,nc1);
      }
      else
	if(strcmp(tmp2->name,"keywords")==0){
	  nc1 = xmlNewNode(ns_ows, BAD_CAST "Keywords");
	  char *toto=tmp2->value;
	  char buff[256];
	  int i=0;
	  int j=0;
	  while(toto[i]){
	    if(toto[i]!=',' && toto[i]!=0){
	      buff[j]=toto[i];
	      buff[j+1]=0;
	      j++;
	    }
	    else{
	      nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	      xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	      xmlAddChild(nc1,nc2);
	      j=0;
	    }
	    i++;
	  }
	  if(strlen(buff)>0){
	    nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	    xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	    xmlAddChild(nc1,nc2);
	  }
	  xmlAddChild(nc,nc1);
	  nc2 = xmlNewNode(ns_ows, BAD_CAST "ServiceType");
	  xmlAddChild(nc2,xmlNewText(BAD_CAST "WPS"));
	  xmlAddChild(nc,nc2);
	  nc2 = xmlNewNode(ns_ows, BAD_CAST "ServiceTypeVersion");
	  xmlAddChild(nc2,xmlNewText(BAD_CAST "1.0.0"));
	  xmlAddChild(nc,nc2);	  
	}
      tmp2=tmp2->next;
    }
  }
  else{
    fprintf(stderr,"TMP4 NOT FOUND !!");
    //dumpMaps(tmp4);
    return NULL;
  }
  xmlAddChild(n,nc);

  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceProvider");
  nc3 = xmlNewNode(ns_ows, BAD_CAST "ServiceContact");
  nc4 = xmlNewNode(ns_ows, BAD_CAST "ContactInfo");
  nc5 = xmlNewNode(ns_ows, BAD_CAST "Phone");
  nc6 = xmlNewNode(ns_ows, BAD_CAST "Address");
  tmp4=getMaps(m,"provider");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    while(tmp2!=NULL){
      if(strcmp(tmp2->name,"keywords")!=0 &&
	 strcmp(tmp2->name,"serverAddress")!=0 &&
	 strcmp(tmp2->name,"lang")!=0){
	tmp2->name[0]=toupper(tmp2->name[0]);
	if(strcmp(tmp2->name,"ProviderName")==0){
	  nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	  xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	  xmlAddChild(nc,nc1);
	}
	else{
	  if(strcmp(tmp2->name,"ProviderSite")==0){
	    nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	    xmlNewProp(nc1,BAD_CAST "xlink:href",BAD_CAST tmp2->value);
	    xmlAddChild(nc,nc1);
	  } 
	  else  
	    if(strcmp(tmp2->name,"IndividualName")==0 || 
	       strcmp(tmp2->name,"PositionName")==0){
	      nc1 = xmlNewNode(ns_ows, BAD_CAST tmp2->name);
	      xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
	      xmlAddChild(nc3,nc1);
	    } 
	    else 
	      if(strncmp(tmp2->name,"Phone",5)==0){
		char *toto=NULL;
		char *toto1=tmp2->name;
		toto=strstr(toto1,"Phone");
		nc1 = xmlNewNode(ns_ows, BAD_CAST toto1+5);
		xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
		xmlAddChild(nc5,nc1);
	      }
	      else 
		if(strncmp(tmp2->name,"Address",7)==0){
		  char *toto=NULL;
		  char *toto1=tmp2->name;
		  toto=strstr(toto1,"Address");
		  nc1 = xmlNewNode(ns_ows, BAD_CAST toto1+7);
		  xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
		  xmlAddChild(nc6,nc1);
		}
	}
      }
      else
	if(strcmp(tmp2->name,"keywords")==0){
	  nc1 = xmlNewNode(ns_ows, BAD_CAST "Keywords");
	  char *toto=tmp2->value;
	  char buff[256];
	  int i=0;
	  int j=0;
	  while(toto[i]){
	    if(toto[i]!=',' && toto[i]!=0){
	      buff[j]=toto[i];
	      buff[j+1]=0;
	      j++;
	    }
	    else{
	      nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	      xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	      xmlAddChild(nc1,nc2);
	      j=0;
	    }
	    i++;
	  }
	  if(strlen(buff)>0){
	    nc2 = xmlNewNode(ns_ows, BAD_CAST "Keyword");
	    xmlAddChild(nc2,xmlNewText(BAD_CAST buff));	      
	    xmlAddChild(nc1,nc2);
	  }
	  xmlAddChild(nc,nc1);
	}
      tmp2=tmp2->next;
    }
  }
  else{
    fprintf(stderr,"TMP4 NOT FOUND !!");
    //dumpMaps(tmp4);
  }
  xmlAddChild(nc4,nc5);
  xmlAddChild(nc4,nc6);
  xmlAddChild(nc3,nc4);
  xmlAddChild(nc,nc3);
  xmlAddChild(n,nc);


  nc = xmlNewNode(ns_ows, BAD_CAST "OperationsMetadata");
  char *tmp2[3];
  tmp2[0]="GetCapabilities";
  tmp2[1]="DescribeProcess";
  tmp2[2]="Execute";
  int j=0;

  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"serverAddress");
    if(tmp!=NULL){
      SERVICE_URL = strdup(tmp->value);
    }
    else
      SERVICE_URL = "not_found";
  }
  else
    SERVICE_URL = "not_found";

  for(j=0;j<3;j++){
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Operation");
    xmlNewProp(nc1,BAD_CAST "name",BAD_CAST tmp2[j]);
    nc2 = xmlNewNode(ns_ows, BAD_CAST "DCP");
    nc3 = xmlNewNode(ns_ows, BAD_CAST "HTTP");
    nc4 = xmlNewNode(ns_ows, BAD_CAST "Get");
    sprintf(tmp,"%s/%s",SERVICE_URL,service);
    xmlNewProp(nc4,BAD_CAST "xlink:href",BAD_CAST tmp);
    xmlAddChild(nc3,nc4);
    if(j>0){
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Post");
      xmlNewProp(nc4,BAD_CAST "xlink:href",BAD_CAST tmp);
      xmlAddChild(nc3,nc4);
    }
    xmlAddChild(nc2,nc3);
    xmlAddChild(nc1,nc2);    
    xmlAddChild(nc,nc1);    
  }
  xmlAddChild(n,nc);

  nc = xmlNewNode(ns, BAD_CAST "ProcessOfferings");
  xmlAddChild(n,nc);



  nc1 = xmlNewNode(ns, BAD_CAST "Languages");
  nc2 = xmlNewNode(ns, BAD_CAST "Default");
  nc3 = xmlNewNode(ns, BAD_CAST "Supported");
  
  toto1=getMaps(m,"main");
  if(toto1!=NULL){
    map* tmp1=getMap(toto1->content,"lang");
    char *toto=tmp1->value;
    char buff[256];
    int i=0;
    int j=0;
    int dcount=0;
    while(toto[i]){
      if(toto[i]!=',' && toto[i]!=0){
	buff[j]=toto[i];
	buff[j+1]=0;
	j++;
      }
      else{
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	if(dcount==0){
	  xmlAddChild(nc2,nc4);
	  xmlAddChild(nc1,nc2);
	  xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST buff);
	  dcount++;
	}
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	xmlAddChild(nc3,nc4);
	j=0;
	buff[j]=0;
      }
      i++;
    }
    if(strlen(buff)>0){
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
      xmlAddChild(nc4,xmlNewText(BAD_CAST buff));	      
      xmlAddChild(nc3,nc4);
    }
  }
  xmlAddChild(nc1,nc3);
  xmlAddChild(n,nc1);
  
  xmlDocSetRootElement(doc, n);

  return nc;
}

void printGetCapabilitiesForProcess(maps* m,xmlNodePtr nc,service* serv){

  xmlNsPtr ns,ns_ows;
  xmlNodePtr nr,n,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;

  nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  ns_ows=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/ows/1.1.0",BAD_CAST "ows");

  int cursor=0;
  map* tmp1;
  if(serv->content!=NULL){
    nc1 = xmlNewNode(ns, BAD_CAST "Process");
    tmp1=getMap(serv->content,"processVersion");
    if(tmp1!=NULL)
      xmlNewProp(nc1,BAD_CAST "wps:processVersion",BAD_CAST tmp1->value);
    
    printDescription(nc1,ns_ows,serv->name,serv->content);
    tmp1=serv->metadata;
    while(tmp1!=NULL){
      nc2 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
      char toto[1024];
      sprintf(toto,"xlink:%s",tmp1->name);
      xmlNewProp(nc2,BAD_CAST toto,BAD_CAST tmp1->value);
      xmlAddChild(nc1,nc2);
      tmp1=tmp1->next;
    }
    xmlAddChild(nc,nc1);
  }
  
}

void printDocument(xmlDocPtr doc){
  rewind(stdout);
  printf("Content-Type: text/xml; charset=utf-8\r\nStatus: 200 OK\r\n\r\n");
  fflush(stdout);
  xmlChar *xmlbuff;
  int buffersize;
  /*
   * Dump the document to a buffer and print it on stdout
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  xmlFreeDoc(doc);
}

void printGetCapabilitiesDocument(xmlDocPtr doc){
  printf("Content-Type: text/xml; charset=utf-8\r\nStatus: 200 OK\r\n\r\n");
  xmlChar *xmlbuff;
  int buffersize;
  /*
   * Dump the document to a buffer and print it on stdout
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);

}
  

void printDescribeProcessResponse(service* serv,char* service){

  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  int buffersize;
  /**
   * Create the document and its temporary root.
   */
  nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://schemas.opengis.net/wps/1.0.0",BAD_CAST "wps");

  doc = xmlNewDoc(BAD_CAST "1.0");
  doc->encoding = xmlCharStrdup ("UTF-8");

  n = xmlNewNode(ns, BAD_CAST "ProcessDescriptions");  
  ns_ows=xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/1999/xlink",BAD_CAST "xlink");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
  xmlNewProp(n,BAD_CAST "xsi:schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  char tmp[256];
  
  nc = xmlNewNode(NULL, BAD_CAST "ProcessDescription");
  char *tmp4[3];
  tmp4[0]="processVersion";
  tmp4[1]="storeSupported";
  tmp4[2]="statusSupported";
  int j=0;
  map* tmp1=NULL;
  for(j=0;j<3;j++){
    tmp1=getMap(serv->content,tmp4[j]);
    if(tmp1!=NULL){
      if(j==0)
	xmlNewProp(nc,BAD_CAST "wps:processVersion",BAD_CAST tmp1->value);      
      else
	xmlNewProp(nc,BAD_CAST tmp4[j],BAD_CAST tmp1->value);      
    }
    else{
      if(j>0)
	xmlNewProp(nc,BAD_CAST tmp4[j],BAD_CAST "false");      
    }
  }
  
  printDescription(nc,ns_ows,serv->name,serv->content);

  tmp1=serv->metadata;
  while(tmp1!=NULL){
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
    xmlNewProp(nc1,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    xmlAddChild(nc,nc1);
    tmp1=tmp1->next;
  }

  tmp1=getMap(serv->content,"Profile");
  if(tmp1!=NULL){
    nc1 = xmlNewNode(ns, BAD_CAST "Profile");
    xmlAddChild(nc1,xmlNewText(BAD_CAST tmp1->value));
    xmlAddChild(nc,nc1);
  }

  nc1 = xmlNewNode(NULL, BAD_CAST "DataInputs");
  
  elements* e=serv->inputs;
  while(e!=NULL){
    nc2 = xmlNewNode(NULL, BAD_CAST "Input");
    tmp1=getMap(e->content,"minOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }
    tmp1=getMap(e->content,"maxOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }

    printDescription(nc2,ns_ows,e->name,e->content);


    nc3 = xmlNewNode(NULL, BAD_CAST e->format);
    iotype* _tmp=e->defaults;
    if(_tmp!=NULL){
      if(strcmp(e->format,"LiteralData")!=0){
	nc4 = xmlNewNode(NULL, BAD_CAST "Default");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      else{
	nc4 = xmlNewNode(NULL, BAD_CAST "UOMs");
	nc5 = xmlNewNode(NULL, BAD_CAST "Default");
      }
      tmp1=_tmp->content;
      while(tmp1!=NULL){
#ifdef DEBUG
	printf("DATATYPE DEFAULT ? %s\n",tmp1->name);
#endif
	if(strcmp(tmp1->name,"DataType")==0){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  char tmp[1024];
	  sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	  xmlNewProp(nc6,BAD_CAST "ows:reference",BAD_CAST tmp);
	  xmlAddChild(nc3,nc6);
	  tmp1=tmp1->next;
	  continue;
	}
	if(strcmp(tmp1->name,"asReference")!=0 && 
	   strcmp(tmp1->name,"DataType")!=0 && 
	   strcmp(tmp1->name,"value")!=0){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  xmlAddChild(nc5,nc6);
	}
	else{
	  if(strcmp(tmp1->name,"value")==0){
	    nc6 = xmlNewNode(NULL, BAD_CAST "DefaultValue");
	    xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	    xmlAddChild(nc3,nc6);
	  }
	}
	tmp1=tmp1->next;
      }
      xmlAddChild(nc4,nc5);
      xmlAddChild(nc3,nc4);
    }
    xmlAddChild(nc2,nc3);
    
    _tmp=e->supported;
    if(_tmp!=NULL){
      if(strcmp(e->format,"LiteralData")!=0){
	nc4 = xmlNewNode(NULL, BAD_CAST "Supported");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      else{
	nc5 = xmlNewNode(NULL, BAD_CAST "Supported");
	
      }
      tmp1=_tmp->content;
      while(tmp1!=NULL){
	/*if(strcmp(e->format,"LiteralData")==0)
	  xmlAddChild(nc5,nc6);*/
	nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	xmlAddChild(nc5,nc6);
	tmp1=tmp1->next;
      }
      xmlAddChild(nc4,nc5);
      if(strcmp(e->format,"LiteralData")!=0){
	xmlAddChild(nc3,nc4);
      }
    }
    if(strcmp(e->format,"LiteralData")!=0){
      xmlAddChild(nc2,nc3);
    }
    xmlAddChild(nc1,nc2);
    
    
    e=e->next;
  }
  xmlAddChild(nc,nc1);

  nc1 = xmlNewNode(NULL, BAD_CAST "ProcessOutputs");
  
  e=serv->outputs;
  while(e!=NULL){
    nc2 = xmlNewNode(NULL, BAD_CAST "Output");
    tmp1=getMap(e->content,"minOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }
    tmp1=getMap(e->content,"maxOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }

    printDescription(nc2,ns_ows,e->name,e->content);

    nc3 = xmlNewNode(ns, BAD_CAST e->format);
    fprintf(stderr,"LITERALDATA ? %s",e->format);
    iotype* _tmp=e->defaults;
    if(_tmp!=NULL){
      if(strcmp(e->format,"LiteralData")!=0 &&
	 strcmp(e->format,"LiteralOutput")!=0){
	nc4 = xmlNewNode(NULL, BAD_CAST "Default");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }   
      else{
	nc4 = xmlNewNode(NULL, BAD_CAST "UOMs");
	nc5 = xmlNewNode(NULL, BAD_CAST "Default");
      }

      /*nc4 = xmlNewNode(NULL, BAD_CAST "Default");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      */
      tmp1=_tmp->content;
      while(tmp1!=NULL){
#ifdef DEBUG
	printf("DATATYPE DEFAULT ? %s\n",tmp1->name);
	fprintf(stderr,"DEBUG DATATYPE !!\n");
	dumpMap(tmp1);
#endif
	if(strcmp(tmp1->name,"DataType")==0){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  char tmp[1024];
	  sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	  xmlNewProp(nc6,BAD_CAST "ows:reference",BAD_CAST tmp);
	  xmlAddChild(nc5,nc6);
	  tmp1=tmp1->next;
	  continue;
	}
	if(strcmp(tmp1->name,"asReference")!=0 && strcmp(tmp1->name,"DataType")!=0){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  xmlAddChild(nc5,nc6);
	}
	tmp1=tmp1->next;
      }
      xmlAddChild(nc4,nc5);
      xmlAddChild(nc3,nc4);	    
    }
    _tmp=e->supported;
    while(_tmp!=NULL){
      nc4 = xmlNewNode(NULL, BAD_CAST "Supported");
      nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      tmp1=_tmp->content;
      while(tmp1!=NULL){
#ifdef DEBUG
	printf("DATATYPE SUPPORTED ? %s\n",tmp1->name);
#endif
	if(strcmp(tmp1->name,"asReference")!=0 && strcmp(tmp1->name,"DataType")!=0){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  xmlAddChild(nc5,nc6);
	}
	tmp1=tmp1->next;
      }
      _tmp=_tmp->next;
      xmlAddChild(nc4,nc5);
      xmlAddChild(nc3,nc4);
    }
    xmlAddChild(nc2,nc3);

    xmlAddChild(nc3,nc4);
      
    
    xmlAddChild(nc2,nc3);
    
    xmlAddChild(nc1,nc2);
    
    e=e->next;
  }
  xmlAddChild(nc,nc1);

  xmlAddChild(n,nc);

  xmlDocSetRootElement(doc, n);
    
  /*
   * Dump the document to a buffer and print it
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  //xmlFreeDoc(doc);

}

xmlNodePtr printDescribeProcessHeader(xmlDocPtr doc,char* service,maps* m){

  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  //xmlDocPtr doc;
  xmlNodePtr n,nr;
  xmlChar *xmlbuff;
  int buffersize;
  /**
   * Create the document and its temporary root.
   */
  nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://schemas.opengis.net/wps/1.0.0",BAD_CAST "wps");

  /*doc = xmlNewDoc(BAD_CAST "1.0");
    doc->encoding = xmlCharStrdup ("UTF-8");*/

  n = xmlNewNode(ns, BAD_CAST "ProcessDescriptions");  
  ns_ows=xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/1999/xlink",BAD_CAST "xlink");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
  xmlNewProp(n,BAD_CAST "xsi:schemaLocation",BAD_CAST "http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
  xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST "en");

  xmlDocSetRootElement(doc, n);

  return n;
}

void printDescribeProcessForProcess(maps* m,xmlNodePtr nc,service** serv,int sc){
  xmlNsPtr ns,ns_ows,ns_xlink,ns_xsi;
  xmlNodePtr nr,n,nc1,nc2,nc3,nc4,nc5,nc6,pseudor;

  char tmp[256];
  n=nc;
  nr = xmlNewNode(ns, BAD_CAST "root");
  
  ns=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/wps/1.0.0",BAD_CAST "wps");
  ns_ows=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/ows/1.1.0",BAD_CAST "ows");

  nc = xmlNewNode(NULL, BAD_CAST "ProcessDescription");
  char *tmp4[3];
  tmp4[0]="processVersion";
  tmp4[1]="storeSupported";
  tmp4[2]="statusSupported";
  int j=0;
  map* tmp1=NULL;
  for(j=0;j<3;j++){
    tmp1=getMap(serv[0]->content,tmp4[j]);
    if(tmp1!=NULL){
      if(j==0)
	xmlNewProp(nc,BAD_CAST "wps:processVersion",BAD_CAST tmp1->value);      
      else
	xmlNewProp(nc,BAD_CAST tmp4[j],BAD_CAST tmp1->value);      
    }
    else{
      if(j>0)
	xmlNewProp(nc,BAD_CAST tmp4[j],BAD_CAST "false");      
    }
  }
  
  printDescription(nc,ns_ows,serv[0]->name,serv[0]->content);

  tmp1=serv[0]->metadata;
  while(tmp1!=NULL){
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
    char toto[1024];
    sprintf(toto,"xlink:%s",tmp1->name);
    xmlNewProp(nc1,BAD_CAST toto,BAD_CAST tmp1->value);
    xmlAddChild(nc,nc1);
    tmp1=tmp1->next;
  }

  tmp1=getMap(serv[0]->content,"Profile");
  if(tmp1!=NULL){
    nc1 = xmlNewNode(ns, BAD_CAST "Profile");
    xmlAddChild(nc1,xmlNewText(BAD_CAST tmp1->value));
    xmlAddChild(nc,nc1);
  }

  nc1 = xmlNewNode(NULL, BAD_CAST "DataInputs");
  
  elements* e=serv[0]->inputs;
  while(e!=NULL){
    nc2 = xmlNewNode(NULL, BAD_CAST "Input");
    tmp1=getMap(e->content,"minOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }
    tmp1=getMap(e->content,"maxOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }

    printDescription(nc2,ns_ows,e->name,e->content);


    nc3 = xmlNewNode(NULL, BAD_CAST e->format);
    iotype* _tmp=e->defaults;
    int datatype=0;
    if(_tmp!=NULL){
      if(strcmp(e->format,"LiteralData")!=0){
	nc4 = xmlNewNode(NULL, BAD_CAST "Default");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      else{
	nc4 = xmlNewNode(NULL, BAD_CAST "UOMs");
	nc5 = xmlNewNode(NULL, BAD_CAST "Default");
	datatype=1;
      }
      tmp1=_tmp->content;
      int default1=0;
      xmlNodePtr nc7;
      while(tmp1!=NULL){
#ifdef DEBUG
	printf("DATATYPE DEFAULT ? %s\n",tmp1->name);
#endif
	if(strcmp(tmp1->name,"DataType")==0){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  char tmp[1024];
	  sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	  xmlNewProp(nc6,BAD_CAST "ows:reference",BAD_CAST tmp);
	  xmlAddChild(nc3,nc6);
	  tmp1=tmp1->next;
	  continue;
	}
	if(strcmp(tmp1->name,"asReference")!=0 && 
	   strcmp(tmp1->name,"DataType")!=0 && 
	   strcmp(tmp1->name,"value")!=0){
	  if(datatype==0)
	  nc6 = xmlNewNode(NULL, BAD_CAST tmp1->name);
	  else
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  xmlAddChild(nc5,nc6);
	}
	else{
	  if(strcmp(tmp1->name,"value")==0){
	    nc7 = xmlNewNode(NULL, BAD_CAST "DefaultValue");
	    xmlAddChild(nc7,xmlNewText(BAD_CAST tmp1->value));
	    default1=1;
	  }
	}
	tmp1=tmp1->next;
      }
      xmlAddChild(nc4,nc5);
      xmlAddChild(nc3,nc4);
      if(datatype==1){
		xmlAddChild(nc3,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
		if(default1>0)
	    xmlAddChild(nc3,nc7);
      }
    }
    xmlAddChild(nc2,nc3);
    
    _tmp=e->supported;
    if(_tmp!=NULL){
      if(datatype==0){
	nc4 = xmlNewNode(NULL, BAD_CAST "Supported");
	nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      else{
	nc5 = xmlNewNode(NULL, BAD_CAST "Supported");
      }
      tmp1=_tmp->content;
      while(tmp1!=NULL){
	/*if(strcmp(e->format,"LiteralData")==0)
	  xmlAddChild(nc5,nc6);*/
	if(datatype==0)
	nc6 = xmlNewNode(NULL, BAD_CAST tmp1->name);
	else
	nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	xmlAddChild(nc5,nc6);
	tmp1=tmp1->next;
      }
      if(datatype==0){
        xmlAddChild(nc4,nc5);
	xmlAddChild(nc3,nc4);
      }else{
      	xmlAddChild(nc4,nc5);
	//xmlAddChild(nc3,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
	}
    }
    if(strcmp(e->format,"LiteralData")!=0){
      xmlAddChild(nc2,nc3);
    }
    xmlAddChild(nc1,nc2);
    
    
    e=e->next;
  }
  xmlAddChild(nc,nc1);

  nc1 = xmlNewNode(NULL, BAD_CAST "ProcessOutputs");
  
  e=serv[0]->outputs;
  while(e!=NULL){
    nc2 = xmlNewNode(NULL, BAD_CAST "Output");
    tmp1=getMap(e->content,"minOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }
    tmp1=getMap(e->content,"maxOccurs");
    if(tmp1){
      xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
    }

    printDescription(nc2,ns_ows,e->name,e->content);

    if(strcmp(mtoupper(e->format),"LITERALDATA")==0)
      nc3 = xmlNewNode(NULL, BAD_CAST "LiteralOutput");
    else
      if(strcmp(mtoupper(e->format),"COMPLEXDATA")==0)
	nc3 = xmlNewNode(NULL, BAD_CAST "ComplexOutput");
      else
	nc3 = xmlNewNode(NULL, BAD_CAST e->format);
    iotype* _tmp=e->defaults;
    int datatype=0;
    if(_tmp!=NULL){
     if(strcmp(e->format,"LiteralOutput")==0 ||
	strcmp(e->format,"LiteralData")==0){
     	datatype=1;
	nc4 = xmlNewNode(NULL, BAD_CAST "UOMs");
	nc5 = xmlNewNode(NULL, BAD_CAST "Default");
     }
      else{
      nc4 = xmlNewNode(NULL, BAD_CAST "Default");
      nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      tmp1=_tmp->content;
      while(tmp1!=NULL){
#ifdef DEBUG
	printf("DATATYPE DEFAULT ? %s\n",tmp1->name);
#endif
	if(strcmp(tmp1->name,"DataType")==0){
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  char tmp[1024];
	  sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	  xmlNewProp(nc6,BAD_CAST "ows:reference",BAD_CAST tmp);
	  xmlAddChild(nc3,nc6);
	  tmp1=tmp1->next;
	  datatype=1;
	  continue;
	}
	if(strcmp(tmp1->name,"asReference")!=0 && strcmp(tmp1->name,"DataType")!=0){
	  if(datatype==0)
	  nc6 = xmlNewNode(NULL, BAD_CAST tmp1->name);
	  else
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  xmlAddChild(nc5,nc6);
	}
	tmp1=tmp1->next;
      }
      xmlAddChild(nc4,nc5);
      xmlAddChild(nc3,nc4);	    
    }
    _tmp=e->supported;
    while(_tmp!=NULL){
    if(datatype==0){
      nc4 = xmlNewNode(NULL, BAD_CAST "Supported");
      nc5 = xmlNewNode(NULL, BAD_CAST "Format");
      }
      else
      nc5 = xmlNewNode(NULL, BAD_CAST "Supported");
      tmp1=_tmp->content;
      while(tmp1!=NULL){
#ifdef DEBUG
	printf("DATATYPE SUPPORTED ? %s\n",tmp1->name);
#endif
	if(strcmp(tmp1->name,"asReference")!=0 && strcmp(tmp1->name,"DataType")!=0){
	  if(datatype==0)
	  nc6 = xmlNewNode(NULL, BAD_CAST tmp1->name);
	  else
	  nc6 = xmlNewNode(ns_ows, BAD_CAST tmp1->name);
	  xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	  xmlAddChild(nc5,nc6);
	}
	tmp1=tmp1->next;
      }
      _tmp=_tmp->next;
      if(datatype==0){
         xmlAddChild(nc4,nc5);
         xmlAddChild(nc3,nc4);
      }else
      xmlAddChild(nc4,nc5);
    }
    xmlAddChild(nc2,nc3);

    xmlAddChild(nc3,nc4);
      
    
    xmlAddChild(nc2,nc3);
    
    xmlAddChild(nc1,nc2);
    
    e=e->next;
  }
  xmlAddChild(nc,nc1);

  xmlAddChild(n,nc);

}

void printDescribeProcessDocument(xmlDocPtr doc){
  xmlChar *xmlbuff;
  int buffersize;
  /*
   * Dump the document to a buffer and print it
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  //xmlFreeDoc(doc);

}
//printGetCapabilitiesHeader

void printOutputDefinitions1(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,maps* m,char* type){
  xmlNodePtr nc1;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=NULL;  
  if(e!=NULL && e->defaults!=NULL)
    tmp=e->defaults->content;
  else{
    /*
    dumpElements(e);
    */
    return;
  }
  while(tmp!=NULL){
    if(strcmp(mtoupper(tmp->name),"MIMETYPE")==0
       || strcmp(mtoupper(tmp->name),"ENCODING")==0
       || strcmp(mtoupper(tmp->name),"SCHEMA")==0
       || strcmp(mtoupper(tmp->name),"UOM")==0)
    xmlNewProp(nc1,BAD_CAST tmp->name,BAD_CAST tmp->value);
    tmp=tmp->next;
  }
  tmp=getMap(e->defaults->content,"asReference");
  if(tmp==NULL)
    xmlNewProp(nc1,BAD_CAST "asReference",BAD_CAST "false");

  tmp=e->content;

  printDescription(nc1,ns_ows,m->name,e->content);

  xmlAddChild(nc,nc1);

}

void printOutputDefinitions(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,map* m,char* type){
  xmlNodePtr nc1,nc2,nc3;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=NULL;  
  if(e!=NULL && e->defaults!=NULL)
    tmp=e->defaults->content;
  else{
    /*
    dumpElements(e);
    */
    return;
  }
  while(tmp!=NULL){
    xmlNewProp(nc1,BAD_CAST tmp->name,BAD_CAST tmp->value);
    tmp=tmp->next;
  }
  tmp=getMap(e->defaults->content,"asReference");
  if(tmp==NULL)
    xmlNewProp(nc1,BAD_CAST "asReference",BAD_CAST "false");

  tmp=e->content;

  printDescription(nc1,ns_ows,m->name,e->content);

  xmlAddChild(nc,nc1);

}

void printIOType1(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,maps* m,char* type){
  xmlNodePtr nc1,nc2,nc3;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=e->content;
#ifdef DEBUG
  dumpMap(tmp);
  dumpElements(e);
#endif
  nc2=xmlNewNode(ns_ows, BAD_CAST "Identifier");
  xmlAddChild(nc2,xmlNewText(BAD_CAST m->name));
  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);
  nc2=xmlNewNode(ns_ows, BAD_CAST tmp->name);
  xmlAddChild(nc2,xmlNewText(BAD_CAST tmp->value));
  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);
  tmp=tmp->next;

  /**
   * IO type Reference or full Data ?
   */
#ifdef DEBUG
  fprintf(stderr,"FORMAT %s %s\n",e->format,mtoupper(e->format));
#endif
  nc2=xmlNewNode(ns_wps, BAD_CAST "Data");
  if(strcmp(mtoupper(e->format),"LITERALOUTPUT")==0)
    nc3=xmlNewNode(ns_wps, BAD_CAST "LiteralData");
  else
    if(strcmp(mtoupper(e->format),"COMPLEXOUTPUT")==0)
      nc3=xmlNewNode(ns_wps, BAD_CAST "ComplexData");
    else
      nc3=xmlNewNode(ns_wps, BAD_CAST e->format);
  tmp=m->content;
  while(tmp!=NULL){
    if(strcmp(tmp->name,"value")!=0)
      xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
    tmp=tmp->next;
    xmlAddChild(nc2,nc3);
  }
  tmp=getMap(e->defaults->content,"mimeType");
  map* tmp1=getMap(m->content,"encoding");
  map* tmp2=getMap(m->content,"mimeType");
  map* toto=getMap(m->content,"value");
  if((tmp1!=NULL && strncmp(tmp1->value,"base64",6)==0)
     || (tmp2!=NULL && (strncmp(tmp2->value,"image/",6)==0
			|| strncmp(tmp2->value,"application/",6)==0)) ){
    map* rs=getMap(m->content,"size");
    if(rs==NULL){
      char tmp1[1024];
      sprintf(tmp1,"%d",strlen(toto->value));
      rs=createMap("z",tmp1);
    }
    xmlAddChild(nc3,xmlNewText(BAD_CAST base64((const unsigned char*)toto->value,atoi(rs->value))));
  }
  else if(tmp!=NULL){
    if(strcmp(tmp->value,"text/js")==0)
      xmlAddChild(nc3,xmlNewCDataBlock(doc,BAD_CAST toto->value,strlen(toto->value)));
    else
      xmlAddChild(nc3,xmlNewText(BAD_CAST toto->value));
    xmlAddChild(nc2,nc3);
  }
  else
    xmlAddChild(nc3,xmlNewText(BAD_CAST toto->value));

  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);

}

void printIOType(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,map* m,char* type){
  xmlNodePtr nc1,nc2,nc3;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=e->content;
  nc2=xmlNewNode(ns_ows, BAD_CAST "Identifier");
  xmlAddChild(nc2,xmlNewText(BAD_CAST m->name));
  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);
  nc2=xmlNewNode(ns_ows, BAD_CAST tmp->name);
  xmlAddChild(nc2,xmlNewText(BAD_CAST tmp->value));
  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);
  tmp=tmp->next;

  /**
   * IO type Reference or full Data ?
   */
#ifdef DEBUG
  printf("FORMAT %s\n",e->format);
#endif
  nc2=xmlNewNode(ns_wps, BAD_CAST "Data");
  nc3=xmlNewNode(ns_wps, BAD_CAST e->format);
  tmp=e->defaults->content;
  while(tmp!=NULL){
    xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
    tmp=tmp->next;
    xmlAddChild(nc2,nc3);
  }
  tmp=getMap(e->defaults->content,"mimeType");
  if(tmp!=NULL){
    if(strcmp(tmp->value,"text/js")==0)
      xmlAddChild(nc3,xmlNewCDataBlock(doc,BAD_CAST m->value,strlen(m->value)));
    else
      xmlAddChild(nc3,xmlNewText(BAD_CAST m->value));
    xmlAddChild(nc2,nc3);
  }
  else
    xmlAddChild(nc3,xmlNewText(BAD_CAST m->value));

  xmlAddChild(nc1,nc2);
  xmlAddChild(nc,nc1);

}

void printDescription(xmlNodePtr root,xmlNsPtr ns_ows,char* identifier,map* amap){
  xmlNodePtr nc2 = xmlNewNode(ns_ows, BAD_CAST "Identifier");
  xmlAddChild(nc2,xmlNewText(BAD_CAST identifier));
  xmlAddChild(root,nc2);
  map* tmp=amap;
  char *tmp2[2];
  tmp2[0]="Title";
  tmp2[1]="Abstract";
  int j=0;
  for(j=0;j<2;j++){
    map* tmp1=getMap(tmp,tmp2[j]);
    if(tmp1!=NULL){
      nc2 = xmlNewNode(ns_ows, BAD_CAST tmp2[j]);
      xmlAddChild(nc2,xmlNewText(BAD_CAST tmp1->value));
      xmlAddChild(root,nc2);
    }
  }
}

void printExceptionReportResponse(maps* m,map* s){
  
  printf("Content-Type: text/xml; charset=utf-8\r\nStatus: 200 OK\r\n\r\n");  
  int buffersize;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  xmlNsPtr ns=NULL;
  xmlNodePtr nr = xmlNewNode(ns, BAD_CAST "root");
  ns=xmlNewNs(nr,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");

  maps* toto1=getMaps(m,"main");
  doc = xmlNewDoc(BAD_CAST "1.0");
  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"encoding");
  //map* tmp=getMapFromMaps(m,"main","encoding");
    if(tmp!=NULL){
      doc->encoding = xmlCharStrdup(tmp->value);
    }
    else
      doc->encoding = xmlCharStrdup("UTF-8");
  }
  else
    doc->encoding = xmlCharStrdup ("UTF-8");

  xmlNodePtr n = xmlNewNode(ns, BAD_CAST "ExceptionReport");  
  xmlNsPtr ns_ows=xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/1999/xlink",BAD_CAST "xlink");
  xmlNewNs(n,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
  xmlNewProp(n,BAD_CAST "xsi:schemaLocation",BAD_CAST "http://www.opengis.net/ows/1.1 http://schemas.opengis.net/ows/1.1.0/owsExceptionReport.xsd");
  xmlNewProp(n,BAD_CAST "xml:lan",BAD_CAST "en");
  xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");

  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"version");
    if(tmp!=NULL){
      xmlNewProp(n,BAD_CAST "version",BAD_CAST tmp->value);
    }
    else
      xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");
  }
  else
    xmlNewProp(n,BAD_CAST "version",BAD_CAST "1.0.0");


  xmlNodePtr nc = xmlNewNode(ns, BAD_CAST "Exception");  

  map* tmp=getMap(s,"code");
  if(tmp!=NULL)
    xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST tmp->value);
  else
    xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST "NoApplicableCode");

  tmp=getMap(s,"text");
  xmlNodePtr nc1 = xmlNewNode(ns, BAD_CAST "ExceptionText");
  if(tmp!=NULL){
    xmlAddChild(nc1,xmlNewText(BAD_CAST tmp->value));
  }
  else
    xmlAddChild(nc1,xmlNewText(BAD_CAST "No debug message available"));
  
  xmlAddChild(nc,nc1);
  xmlAddChild(n,nc);
  xmlDocSetRootElement(doc, n);

  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);
  printf((char *) xmlbuff);
  xmlFree(xmlbuff);
  xmlFree(ns_ows);
  xmlFree(nr);
  xmlFree(nc1);
  xmlFree(nc);
  xmlFree(n);
  xmlFree(ns);
  xmlFree(doc);
}


void outputResponse(service* s,maps* request_inputs,maps* request_outputs,
		    map* request_inputs1,int cpid,maps* m,int res){
#ifdef DEBUG
  dumpMaps(request_inputs);
  dumpMaps(request_outputs);
  fprintf(stderr,"printProcessResponse\n");
#endif
  map* toto=getMap(request_inputs1,"RawDataOutput");
  int asRaw=0;
  if(toto!=NULL)
    asRaw=1;
   
  if(asRaw==0){
    map *r_inputs=getMap(s->content,"serviceProvider");
#ifdef DEBUG
    fprintf(stderr,"SERVICE : %s\n",r_inputs->value);
    dumpMaps(m);
#endif
    printProcessResponse1(m,request_inputs1,cpid,
			  s,r_inputs->value,res,
			  request_inputs,
			  request_outputs);
  }
  else{
    /**
     * We get the first output only !!
     */
    char mime[1024];
    map* mi=getMap(request_outputs->content,"mimeType");
#ifdef DEBUG
    fprintf(stderr,"SERVICE OUTPUTS\n");
    dumpMaps(request_outputs);
    fprintf(stderr,"SERVICE OUTPUTS\n");
#endif
    map* en=getMap(request_outputs->content,"encoding");
    if(mi!=NULL && en!=NULL)
      sprintf(mime,
	      "Content-Type: %s; charset=%s\r\nStatus: 200 OK\r\n\r\n",
	      mi->value,en->value);
    else
      if(mi!=NULL)
	sprintf(mime,
		"Content-Type: %s; charset=UTF-8\r\nStatus: 200 OK\r\n\r\n",
		mi->value);
      else
	sprintf(mime,"Content-Type: text/plain; charset=utf-8\r\nStatus: 200 OK\r\n\r\n");
    printf(mime);
    toto=getMapFromMaps(request_outputs,
			request_outputs->name,"value");
    if(mi!=NULL && strncmp(mi->value,"image",5)==0){
      map* rs=getMapFromMaps(request_outputs,
			     request_outputs->name,"size");
      fwrite(toto->value,atoi(rs->value),1,stdout);
    }
    else
      printf(toto->value);
#ifdef DEBUG
    dumpMap(toto);
#endif
  }
}

char *base64(const unsigned char *input, int length)
{
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, length);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  char *buff = (char *)malloc(bptr->length);
  memcpy(buff, bptr->data, bptr->length-1);
  buff[bptr->length-1] = 0;

  BIO_free_all(b64);

  fprintf(stderr,"BASE64 [%s] \n",buff);
  return buff;
}

void addDefaultValues(maps** out,elements* in,maps* m,char* type){
  elements* tmpInputs=in;
  maps* out1=*out;
  while(tmpInputs!=NULL){
    maps *tmpMaps=getMaps(out1,tmpInputs->name);
    if(tmpMaps==NULL){
      map* tmpMap1=getMap(tmpInputs->content,"minOccurs");
      if(strcmp(type,"inputs")==0)
	if(tmpMap1!=NULL && atoi(tmpMap1->value)>=1){
	  char tmps[1024];
	  sprintf(tmps,"The <%s> argument was not specified in DataInputs but defined as requested in ZOO ServicesProvider configuration file, please correct your query or the ZOO Configuration file.",tmpInputs->name);
	  map* tmpe=createMap("text",tmps);
	  addToMap(tmpe,"code","MissingParameterValue");
	  printExceptionReportResponse(m,tmpe);
	  exit(1);
	}
      tmpMaps=(maps*)malloc(MAPS_SIZE);
      tmpMaps->name=strdup((char*)tmpInputs->name);
      tmpMaps->content=NULL;
      iotype* tmpIoType=tmpInputs->defaults;
      while(tmpIoType!=NULL){
	tmpMaps->content=tmpIoType->content;
	tmpMaps->content->next;
	tmpIoType=tmpIoType->next;
      }
      map *tmpMap=getMap(tmpMaps->content,"value");
      if(tmpMap==NULL)
	addToMap(tmpMaps->content,"value","NULL");
      tmpMaps->next=NULL;
      dumpMaps(tmpMaps);
      if(out1==NULL){
	out1=(maps*)malloc(MAPS_SIZE);
	out1->name=tmpMaps->name;
	out1->content=tmpMaps->content;
	out1->next=NULL;
	*out=out1;
      }
      else
	addMapsToMaps(&out1,tmpMaps);
    }
    else{
      map* tmpContent=tmpInputs->defaults->content;
      while(tmpContent!=NULL){
	map* tmpIn=getMap(out1->content,tmpContent->name);
	if(tmpIn==NULL)
	  if(out1->content==NULL)
	    out1->content=createMap(tmpContent->name,tmpContent->value);
	  else
	    addToMap(out1->content,tmpContent->name,tmpContent->value);
	tmpContent=tmpContent->next;
      }
    }
    tmpInputs=tmpInputs->next;
  }
}
