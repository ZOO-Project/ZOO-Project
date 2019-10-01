/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2015 GeoLabs SARL
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

#ifdef USE_MS
#include "service_internal_ms.h"
#else
#include "cpl_vsi.h"
#endif
#ifdef USE_CALLBACK
#include "service_callback.h"
#endif
#include "service.h"
#include "service_internal.h"
#include "server_internal.h"
#include "response_print.h"

#ifndef WIN32
#include <dlfcn.h>
#endif

#include "mimetypes.h"

/**
 * Add prefix to the service name.
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param level the map containing the level information
 * @param serv the service structure created from the zcfg file
 */
void addPrefix(maps* conf,map* level,service* serv){
  if(level!=NULL){
    char key[25];
    char* prefix=NULL;
    int clevel=atoi(level->value);
    int cl=0;
    for(cl=0;cl<clevel;cl++){
      sprintf(key,"sprefix_%d",cl);
      map* tmp2=getMapFromMaps(conf,"lenv",key);
      if(tmp2!=NULL){
	if(prefix==NULL)
	  prefix=zStrdup(tmp2->value);
	else{
	  int plen=strlen(prefix);
	  prefix=(char*)realloc(prefix,(plen+strlen(tmp2->value)+2)*sizeof(char));
	  memcpy(prefix+plen,tmp2->value,strlen(tmp2->value)*sizeof(char));
	  prefix[plen+strlen(tmp2->value)]=0;
	}
      }
    }
    if(prefix!=NULL){
      char* tmp0=zStrdup(serv->name);
      free(serv->name);
      serv->name=(char*)malloc((strlen(prefix)+strlen(tmp0)+1)*sizeof(char));
      sprintf(serv->name,"%s%s",prefix,tmp0);
      free(tmp0);
      free(prefix);
      prefix=NULL;
    }
  }
}

/**
 * Print the HTTP headers based on a map.
 * 
 * @param m the map containing the headers information
 */
void printHeaders(maps* m){
  maps *_tmp=getMaps(m,"headers");
  if(_tmp!=NULL){
    map* _tmp1=_tmp->content;
    while(_tmp1!=NULL){
      printf("%s: %s\r\n",_tmp1->name,_tmp1->value);
      _tmp1=_tmp1->next;
    }
  }
}

/**
 * Add a land attribute to a XML node
 *
 * @param n the XML node to add the attribute
 * @param m the map containing the language key to add as xml:lang
 */
void addLangAttr(xmlNodePtr n,maps *m){
  map *tmpLmap=getMapFromMaps(m,"main","language");
  if(tmpLmap!=NULL)
    xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST tmpLmap->value);
  else
    xmlNewProp(n,BAD_CAST "xml:lang",BAD_CAST "en-US");
}

/**
 * Replace the first letter by its upper case version in a new char array
 *
 * @param tmp the char*
 * @return a new char* with first letter in upper case
 * @warning be sure to free() the returned string after use
 */
char *zCapitalize1(char *tmp){
  char *res=zStrdup(tmp);
  if(res[0]>=97 && res[0]<=122)
    res[0]-=32;
  return res;
}

/**
 * Replace all letters by their upper case version in a new char array
 *
 * @param tmp the char*
 * @return a new char* with first letter in upper case
 * @warning be sure to free() the returned string after use
 */
char *zCapitalize(char *tmp){
  int i=0;
  char *res=zStrdup(tmp);
  for(i=0;i<strlen(res);i++)
    if(res[i]>=97 && res[i]<=122)
      res[i]-=32;
  return res;
}

/**
 * Search for an existing XML namespace in usedNS.
 * 
 * @param name the name of the XML namespace to search
 * @return the index of the XML namespace found or -1 if not found.
 */
int zooXmlSearchForNs(const char* name){
  int i;
  int res=-1;
  for(i=0;i<nbNs;i++)
    if(strncasecmp(name,nsName[i],strlen(nsName[i]))==0){
      res=i;
      break;
    }
  return res;
}

/**
 * Add an XML namespace to the usedNS if it was not already used.
 * 
 * @param nr the xmlNodePtr to attach the XML namspace (can be NULL)
 * @param url the url of the XML namespace to add
 * @param name the name of the XML namespace to add
 * @return the index of the XML namespace added.
 */
int zooXmlAddNs(xmlNodePtr nr,const char* url,const char* name){
#ifdef DEBUG
  fprintf(stderr,"zooXmlAddNs %d %s \n",nbNs,name);
#endif
  int currId=-1;
  if(nbNs==0){
    nbNs++;
    currId=0;
    nsName[currId]=zStrdup(name);
    usedNs[currId]=xmlNewNs(nr,BAD_CAST url,BAD_CAST name);
  }else{
    currId=zooXmlSearchForNs(name);
    if(currId<0){
      nbNs++;
      currId=nbNs-1;
      nsName[currId]=zStrdup(name);
      usedNs[currId]=xmlNewNs(nr,BAD_CAST url,BAD_CAST name);
    }
  }
  return currId;
}

/**
 * Free allocated memory to store used XML namespace.
 */
void zooXmlCleanupNs(){
  int j;
#ifdef DEBUG
  fprintf(stderr,"zooXmlCleanup %d\n",nbNs);
#endif
  for(j=nbNs-1;j>=0;j--){
#ifdef DEBUG
    fprintf(stderr,"zooXmlCleanup %d\n",j);
#endif
    if(j==0)
      xmlFreeNs(usedNs[j]);
    free(nsName[j]);
    nbNs--;
  }
  nbNs=0;
}

/**
 * Add a XML document to the iDocs.
 * 
 * @param value the string containing the XML document
 * @return the index of the XML document added.
 */
int zooXmlAddDoc(const char* value){
  int currId=0;
  nbDocs++;
  currId=nbDocs-1;
  iDocs[currId]=xmlParseMemory(value,strlen(value));
  return currId;
}

/**
 * Free allocated memort to store XML documents
 */
void zooXmlCleanupDocs(){
  int j;
  for(j=nbDocs-1;j>=0;j--){
    xmlFreeDoc(iDocs[j]);
  }
  nbDocs=0;
}

/**
 * Generate a SOAP Envelope node when required (if the isSoap key of the [main]
 * section is set to true).
 * 
 * @param conf the conf maps containing the main.cfg settings
 * @param n the node used as children of the generated soap:Envelope
 * @return the generated soap:Envelope (if isSoap=true) or the input node n 
 *  (when isSoap=false)
 */
xmlNodePtr soapEnvelope(maps* conf,xmlNodePtr n){
  map* soap=getMapFromMaps(conf,"main","isSoap");
  if(soap!=NULL && strcasecmp(soap->value,"true")==0){
    int lNbNs=nbNs;
    nsName[lNbNs]=zStrdup("soap");
    usedNs[lNbNs]=xmlNewNs(NULL,BAD_CAST "http://www.w3.org/2003/05/soap-envelope",BAD_CAST "soap");
    nbNs++;
    xmlNodePtr nr = xmlNewNode(usedNs[lNbNs], BAD_CAST "Envelope");
    nsName[nbNs]=zStrdup("soap");
    usedNs[nbNs]=xmlNewNs(nr,BAD_CAST "http://www.w3.org/2003/05/soap-envelope",BAD_CAST "soap");
    nbNs++;
    nsName[nbNs]=zStrdup("xsi");
    usedNs[nbNs]=xmlNewNs(nr,BAD_CAST "http://www.w3.org/2001/XMLSchema-instance",BAD_CAST "xsi");
    nbNs++;
    xmlNsPtr ns_xsi=usedNs[nbNs-1];
    xmlNewNsProp(nr,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.w3.org/2003/05/soap-envelope http://www.w3.org/2003/05/soap-envelope");
    xmlNodePtr nr1 = xmlNewNode(usedNs[lNbNs], BAD_CAST "Body");
    xmlAddChild(nr1,n);
    xmlAddChild(nr,nr1);
    return nr;
  }else
    return n;
}

/**
 * Generate a WPS header.
 * 
 * @param doc the document to add the header
 * @param m the conf maps containing the main.cfg settings
 * @param req the request type (GetCapabilities,DescribeProcess,Execute)
 * @param rname the root node name
 * @return the generated wps:rname xmlNodePtr (can be wps: Capabilities, 
 *  wps:ProcessDescriptions,wps:ExecuteResponse)
 */
xmlNodePtr printWPSHeader(xmlDocPtr doc,maps* m,const char* req,const char* rname,const char* version,int reqId){

  xmlNsPtr ns,ns_xsi;
  xmlNodePtr n;

  int vid=getVersionId(version);

  int wpsId=zooXmlAddNs(NULL,schemas[vid][2],"wps");
  ns=usedNs[wpsId];
  n = xmlNewNode(ns, BAD_CAST rname);
  zooXmlAddNs(n,schemas[vid][1],"ows");
  xmlNewNs(n,BAD_CAST schemas[vid][2],BAD_CAST "wps");
  zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
  ns_xsi=usedNs[xsiId];
  char *tmp=(char*) malloc((86+strlen(req)+1)*sizeof(char));
  sprintf(tmp,schemas[vid][4],schemas[vid][2],schemas[vid][3],req);
  xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST tmp);
  free(tmp);
  if(vid==0 || reqId==0){
    xmlNewProp(n,BAD_CAST "service",BAD_CAST "WPS");
    xmlNewProp(n,BAD_CAST "version",BAD_CAST schemas[vid][0]);
  }
  if(vid==0)
    addLangAttr(n,m);
  xmlNodePtr fn=soapEnvelope(m,n);
  xmlDocSetRootElement(doc, fn);
  return n;
}

void addLanguageNodes(maps* conf,xmlNodePtr n,xmlNsPtr ns,xmlNsPtr ns_ows){
  xmlNodePtr nc1,nc2,nc3,nc4;
  map* version=getMapFromMaps(conf,"main","rversion");
  int vid=getVersionId(version->value);
  if(vid==1)
    nc1 = xmlNewNode(ns_ows, BAD_CAST "Languages");
  else{
    nc1 = xmlNewNode(ns, BAD_CAST "Languages");
    nc2 = xmlNewNode(ns, BAD_CAST "Default");
    nc3 = xmlNewNode(ns, BAD_CAST "Supported");
  }

  maps* tmp=getMaps(conf,"main");
  if(tmp!=NULL){
    map* tmp1=getMap(tmp->content,"lang");
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
	if(dcount==0){
	  if(vid==0){
	    nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	    xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	    xmlAddChild(nc2,nc4);
	    xmlAddChild(nc1,nc2);
	  }
	  dcount++;
	}
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
	xmlAddChild(nc4,xmlNewText(BAD_CAST buff));
	if(vid==0)
	  xmlAddChild(nc3,nc4);
	else
	  xmlAddChild(nc1,nc4);
	j=0;
	buff[j]=0;
      }
      i++;
    }
    if(strlen(buff)>0){
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Language");
      xmlAddChild(nc4,xmlNewText(BAD_CAST buff));	      
      if(vid==0)
	xmlAddChild(nc3,nc4);
      else
	xmlAddChild(nc1,nc4);
    }
  }
  if(vid==0)
    xmlAddChild(nc1,nc3);
  xmlAddChild(n,nc1);
}

/**
 * Generate a Capabilities header.
 * 
 * @param doc the document to add the header
 * @param m the conf maps containing the main.cfg settings
 * @return the generated wps:ProcessOfferings xmlNodePtr 
 */
xmlNodePtr printGetCapabilitiesHeader(xmlDocPtr doc,maps* m,const char* version="1.0.0"){

  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr n,nc,nc1,nc2,nc3,nc4,nc5,nc6;
  n = printWPSHeader(doc,m,"GetCapabilities","Capabilities",version,0);
  maps* toto1=getMaps(m,"main");
  char tmp[256];
  map* v=getMapFromMaps(m,"main","rversion");
  int vid=getVersionId(v->value);

  int wpsId=zooXmlAddNs(NULL,schemas[vid][2],"wps");
  ns=usedNs[wpsId];
  int xlinkId=zooXmlAddNs(NULL,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];
  int owsId=zooXmlAddNs(NULL,schemas[vid][1],"ows");
  ns_ows=usedNs[owsId];

  nc = xmlNewNode(ns_ows, BAD_CAST "ServiceIdentification");
  maps* tmp4=getMaps(m,"identification");
  if(tmp4!=NULL){
    map* tmp2=tmp4->content;
    const char *orderedFields[5];
    orderedFields[0]="Title";
    orderedFields[1]="Abstract";
    orderedFields[2]="Keywords";
    orderedFields[3]="Fees";
    orderedFields[4]="AccessConstraints";
    int oI=0;
    for(oI=0;oI<5;oI++)
      if((tmp2=getMap(tmp4->content,orderedFields[oI]))!=NULL){
	if(strcasecmp(tmp2->name,"abstract")==0 ||
	   strcasecmp(tmp2->name,"title")==0 ||
	   strcasecmp(tmp2->name,"accessConstraints")==0 ||
	   strcasecmp(tmp2->name,"fees")==0){
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
	    map* tmpv=getMapFromMaps(m,"main","rversion");
	    xmlAddChild(nc2,xmlNewText(BAD_CAST tmpv->value));
	    xmlAddChild(nc,nc2);
	  }
	tmp2=tmp2->next;
      }
  }
  else{
    fprintf(stderr,"TMP4 NOT FOUND !!");
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
    const char *tmpAddress[6];
    tmpAddress[0]="addressDeliveryPoint";
    tmpAddress[1]="addressCity";
    tmpAddress[2]="addressAdministrativeArea";
    tmpAddress[3]="addressPostalCode";
    tmpAddress[4]="addressCountry";
    tmpAddress[5]="addressElectronicMailAddress";
    const char *tmpPhone[2];
    tmpPhone[0]="phoneVoice";
    tmpPhone[1]="phoneFacsimile";
    const char *orderedFields[12];
    orderedFields[0]="providerName";
    orderedFields[1]="providerSite";
    orderedFields[2]="individualName";
    orderedFields[3]="positionName";
    orderedFields[4]=tmpPhone[0];
    orderedFields[5]=tmpPhone[1];
    orderedFields[6]=tmpAddress[0];
    orderedFields[7]=tmpAddress[1];
    orderedFields[8]=tmpAddress[2];
    orderedFields[9]=tmpAddress[3];
    orderedFields[10]=tmpAddress[4];
    orderedFields[11]=tmpAddress[5];
    int oI=0;
    for(oI=0;oI<12;oI++)
      if((tmp2=getMap(tmp4->content,orderedFields[oI]))!=NULL){
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
	      xmlNewNsProp(nc1,ns_xlink,BAD_CAST "href",BAD_CAST tmp2->value);
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
		  int j;
		  for(j=0;j<2;j++)
		    if(strcasecmp(tmp2->name,tmpPhone[j])==0){
		      char *tmp4=tmp2->name;
		      nc1 = xmlNewNode(ns_ows, BAD_CAST tmp4+5);
		      xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
		      xmlAddChild(nc5,nc1);
		    }
		}
		else 
		  if(strncmp(tmp2->name,"Address",7)==0){
		    int j;
		    for(j=0;j<6;j++)
		      if(strcasecmp(tmp2->name,tmpAddress[j])==0){
			char *tmp4=tmp2->name;
			nc1 = xmlNewNode(ns_ows, BAD_CAST tmp4+7);
			xmlAddChild(nc1,xmlNewText(BAD_CAST tmp2->value));
			xmlAddChild(nc6,nc1);
		      }
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
  }
  xmlAddChild(nc4,nc5);
  xmlAddChild(nc4,nc6);
  xmlAddChild(nc3,nc4);
  xmlAddChild(nc,nc3);
  xmlAddChild(n,nc);


  nc = xmlNewNode(ns_ows, BAD_CAST "OperationsMetadata");

  int j=0;

  if(toto1!=NULL){
    map* tmp=getMap(toto1->content,"serverAddress");
    if(tmp!=NULL){
      SERVICE_URL = zStrdup(tmp->value);
    }
    else
      SERVICE_URL = zStrdup("not_defined");
  }
  else
    SERVICE_URL = zStrdup("not_defined");

  for(j=0;j<nbSupportedRequests;j++){
    if(requests[vid][j]==NULL)
      break;
    else{
      nc1 = xmlNewNode(ns_ows, BAD_CAST "Operation");
      xmlNewProp(nc1,BAD_CAST "name",BAD_CAST requests[vid][j]);
      nc2 = xmlNewNode(ns_ows, BAD_CAST "DCP");
      nc3 = xmlNewNode(ns_ows, BAD_CAST "HTTP");
      if(vid!=1 || j!=2){
	nc4 = xmlNewNode(ns_ows, BAD_CAST "Get");
	xmlNewNsProp(nc4,ns_xlink,BAD_CAST "href",BAD_CAST SERVICE_URL);
	xmlAddChild(nc3,nc4);
      }
      nc4 = xmlNewNode(ns_ows, BAD_CAST "Post");
      xmlNewNsProp(nc4,ns_xlink,BAD_CAST "href",BAD_CAST SERVICE_URL);
      xmlAddChild(nc3,nc4);
      xmlAddChild(nc2,nc3);
      xmlAddChild(nc1,nc2);
      xmlAddChild(nc,nc1);
    }
  }
  xmlAddChild(n,nc);

  if(vid==1)
    addLanguageNodes(m,n,ns,ns_ows);
  free(SERVICE_URL);

  nc = xmlNewNode(ns, BAD_CAST root_nodes[vid][0]);
  xmlAddChild(n,nc);

  if(vid==0)
    addLanguageNodes(m,n,ns,ns_ows);

  return nc;
}

/**
 * Generate a wps:Process node for a servie and add it to a given node.
 * 
 * @param reg the profiles registry
 * @param m the conf maps containing the main.cfg settings
 * @param registry the profile registry if any
 * @param nc the XML node to add the Process node
 * @param serv the service structure created from the zcfg file
 * @return the generated wps:ProcessOfferings xmlNodePtr 
 */
void printGetCapabilitiesForProcess(registry *reg, maps* m,void* doc0,void* nc0,service* serv){
  xmlNsPtr ns,ns_ows,ns_xml,ns_xlink;
  xmlDocPtr doc=(xmlDocPtr) doc0;
  xmlNodePtr nc=(xmlNodePtr) nc0;
  xmlNodePtr n=NULL,nc1,nc2,nc3;
  map* version=getMapFromMaps(m,"main","rversion");
  int vid=getVersionId(version->value);
  // Initialize or get existing namespaces
  int wpsId=zooXmlAddNs(NULL,schemas[vid][2],"wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,schemas[vid][1],"ows");
  ns_ows=usedNs[owsId];
  int xmlId=zooXmlAddNs(NULL,"http://www.w3.org/XML/1998/namespace","xml");
  ns_xml=usedNs[xmlId];
  int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];
  map* tmp1;
  if(serv->content!=NULL){
    nc1 = xmlNewNode(ns, BAD_CAST capabilities[vid][0]);
    int i=1;
    int limit=3;
    if(vid==1){
      ns=NULL;
      limit=7;
    }
    nc3=NULL;
    map* sType=getMap(serv->content,"serviceType");
    for(;i<limit;i+=2){
      if(capabilities[vid][i]==NULL)
	break;
      else{
	tmp1=getMap(serv->content,capabilities[vid][i]);
	if(tmp1!=NULL){
	  if(vid==1 && i==1 && strlen(tmp1->value)<5){
	    char *val=(char*)malloc((strlen(tmp1->value)+5)*sizeof(char));
	    sprintf(val,"%s.0.0",tmp1->value);
	    xmlNewNsProp(nc1,ns,BAD_CAST capabilities[vid][i],BAD_CAST val);
	    free(val);
	  }
	  else
	    xmlNewNsProp(nc1,ns,BAD_CAST capabilities[vid][i],BAD_CAST tmp1->value);
	}
	else{
	  if(i==3 && vid==1 && sType!=NULL && strstr(sType->value,"HPC")!=NULL)
	    xmlNewNsProp(nc1,ns,BAD_CAST capabilities[vid][i],BAD_CAST "async-execute dismiss");
	  else
	    xmlNewNsProp(nc1,ns,BAD_CAST capabilities[vid][i],BAD_CAST capabilities[vid][i+1]);
	}
      }
    }
    map* tmp3=getMapFromMaps(m,"lenv","level");
    addPrefix(m,tmp3,serv);
    printDescription(nc1,ns_ows,serv->name,serv->content,vid);
    tmp1=serv->metadata;

    addMetadata(tmp1,doc,nc1,ns_ows,ns_xlink,vid);
    tmp1=serv->additional_parameters;
    int fromDb=-1;
    map* test=getMap(serv->content,"fromDb");
    if(test!=NULL && strncasecmp(test->value,"true",4)==0)
      fromDb=1;
    addAdditionalParameters(tmp1,doc,nc1,ns_ows,ns_xlink,fromDb);

    if(nc3!=NULL)
      xmlAddChild(nc1,nc3);
    xmlAddChild(nc,nc1);
  }

}

/**
 * Attach attributes to a ProcessDescription or a ProcessOffering node.
 * 
 * @param n the XML node to attach the attributes to
 * @param ns the XML namespace to create the attributes
 * @param content the servive main content created from the zcfg file
 * @param vid the version identifier (0 for 1.0.0 and 1 for 2.0.0)
 * @param serviceType string containing the current service type
 */
void attachAttributes(xmlNodePtr n,xmlNsPtr ns,map* content,int vid,map* serviceType){
  int limit=7;
  for(int i=1;i<limit;i+=2){
    map* tmp1=getMap(content,capabilities[vid][i]);
    if(tmp1!=NULL){
      if(vid==1 && i==1 && strlen(tmp1->value)<5){
	char *val=(char*)malloc((strlen(tmp1->value)+5)*sizeof(char));
	sprintf(val,"%s.0.0",tmp1->value);
	xmlNewNsProp(n,ns,BAD_CAST capabilities[vid][i],BAD_CAST val);
	free(val);
      }
      else{
	if(vid==0 && i>=2)
	  xmlNewProp(n,BAD_CAST capabilities[vid][i],BAD_CAST tmp1->value);
	else
	  xmlNewNsProp(n,ns,BAD_CAST capabilities[vid][i],BAD_CAST tmp1->value);
      }
    }
    else{
      if(i==3 && vid==1 && serviceType!=NULL && strstr(serviceType->value,"HPC")!=NULL)
	xmlNewNsProp(n,ns,BAD_CAST capabilities[vid][i],BAD_CAST "async-execute dismiss");
      else
	if(vid==0 && i>=2)
	  xmlNewProp(n,BAD_CAST capabilities[vid][i],BAD_CAST capabilities[vid][i+1]);
	else
	  xmlNewNsProp(n,ns,BAD_CAST capabilities[vid][i],BAD_CAST capabilities[vid][i+1]);
    }
  }
}

/**
 * Add a Metadata node to any existing node.
 * @param meta the map defining the additional parameters
 * @param doc the XML document used
 * @param nb the node to add the additional parameters
 * @param ns_ows the OWS namespace
 * @param ns_xlink the xlink namespace
 * @param vid the version of WPS to use (0 for 1.0.0 and 1 for 2.0)
 */
void addMetadata(map* meta,xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_ows,xmlNsPtr ns_xlink,int vid){
  int hasTitle=-1;
  int hasValue=-1;
  xmlNodePtr nc1;
  map* oMeta=meta;
  int isAdditionalParameters=-1;
  int level=0;
  map* test=getMap(meta,"title");
  if(test!=NULL)
    level+=1;
  test=getMap(meta,"href");
  if(test!=NULL)
    level+=1;
  test=getMap(meta,"role");
  if(test!=NULL)
    level+=1;
  if(count(oMeta)>level+1)
    isAdditionalParameters=1;
  char *ctitle=NULL;
  while(meta!=NULL){
    if(hasTitle<0)
      if(isAdditionalParameters<0)
	nc1 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
      else
	if(hasValue<0)
	  nc1 = xmlNewNode(ns_ows, BAD_CAST "AdditionalParameters");
    if(strncasecmp(meta->name,"title",5)==0 ||
       strcasecmp(meta->name,"href")==0 ||
       strcasecmp(meta->name,"role")==0 ){
      int index=5;
      if(strncasecmp(meta->name,"title",5)==0){
	index=6;
	hasTitle=1;
	if(ctitle!=NULL && strcasecmp(meta->value,ctitle)!=0){
	  xmlAddChild(nc,nc1);
	  nc1 = xmlNewNode(ns_ows, BAD_CAST "AdditionalParameters");
	  free(ctitle);
	  ctitle=NULL;
	}
	if(ctitle==NULL){
	  char *tmp=(char*)malloc((strlen(meta->name)+1)*sizeof(char));
	  snprintf(tmp,index,"%s",meta->name);
	  xmlNewNsProp(nc1,ns_xlink,BAD_CAST tmp,BAD_CAST meta->value);
	  free(tmp);
	}	  
	if(ctitle!=NULL)
	  free(ctitle);
	ctitle=zStrdup(meta->value);
      }
    }else{
      xmlNodePtr nc2 = xmlNewNode(ns_ows, BAD_CAST "AdditionalParameter");
      xmlNodePtr nc3 = xmlNewNode(ns_ows, BAD_CAST "Name");
      xmlAddChild(nc3,xmlNewText(BAD_CAST meta->name));
      xmlNodePtr nc4 = xmlNewNode(ns_ows, BAD_CAST "Value");
      xmlAddChild(nc4,xmlNewText(BAD_CAST meta->value));
      xmlAddChild(nc2,nc3);
      xmlAddChild(nc2,nc4);
      xmlAddChild(nc1,nc2);
      hasTitle=-1;
    }
    meta=meta->next;
    if(hasTitle<0){
      hasValue=1;
      if(isAdditionalParameters){
	if(vid==0){
	  meta=NULL;
	  break;
	}else
	  xmlAddChild(nc,nc1);
      }
    }
  }
  if(oMeta!=NULL && hasValue<0 && nc1!=NULL){
    xmlAddChild(nc,nc1);
  }
}

/**
 * Add AdditionalParameters nodes to any existing node.
 * @param meta the map defining the additional parameters
 * @param doc the XML document used
 * @param nb the node to add the additional parameters
 * @param ns_ows the OWS namespace
 * @param ns_xlink the xlink namespace
 * @param fromDb 1 if the metadata has been extracted from the metadb, 
 * 0 otherwise
 */
void addAdditionalParameters(map* meta,xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_ows,xmlNsPtr ns_xlink,int fromDb){
  int hasTitle=-1;
  int hasValue=-1;
  int toAddAtEnd=-1;
  int cnt=0;
  xmlNodePtr* ncr=NULL;
  xmlNodePtr nc1;
  map* oMeta=meta;
  int isAdditionalParameters=-1;
  int level=0;
  map* test=getMap(meta,"title");
  map* otitle=getMap(meta,"title");
  map* length=getMap(meta,"length");
  int len=0;
  char *ctitle=NULL;
    
  if(test!=NULL)
    level+=1;
  test=getMap(meta,"href");
  if(test!=NULL)
    level+=1;
  test=getMap(meta,"role");
  if(test!=NULL)
    level+=1;
  if(count(oMeta)>level+1)
    isAdditionalParameters=1;

  while(meta!=NULL){
    if(hasTitle<0 && hasValue<0){
      nc1 = xmlNewNode(ns_ows, BAD_CAST "AdditionalParameters");
    }
    if(strncasecmp(meta->name,"title",5)==0 ||
       strcasecmp(meta->name,"href")==0 ||
       strcasecmp(meta->name,"role")==0 ){
      int index=5;
      if(strncasecmp(meta->name,"title",5)==0){
	index=6;
	hasTitle=1;
	if(ctitle!=NULL && strcasecmp(meta->value,ctitle)!=0){
	  xmlNodePtr ncTmp = xmlDocCopyNodeList(doc,nc1);
	  xmlAddChild(nc,ncTmp);
	  xmlFreeNode(nc1);
	  toAddAtEnd=1;
	  cnt++;
	  nc1 = xmlNewNode(ns_ows, BAD_CAST "AdditionalParameters");
	  free(ctitle);
	  ctitle=NULL;
	}
	if(ctitle==NULL){
	  char *tmp=(char*)malloc((strlen(meta->name)+1)*sizeof(char));
	  snprintf(tmp,index,"%s",meta->name);
	  xmlNewNsProp(nc1,ns_xlink,BAD_CAST tmp,BAD_CAST meta->value);
	  free(tmp);
	}	  
	if(ctitle!=NULL)
	  free(ctitle);
	ctitle=zStrdup(meta->value);
      }else{
	xmlNewNsProp(nc1,ns_xlink,BAD_CAST meta->name,BAD_CAST meta->value);
      }
    }else{
      if(strncasecmp(meta->name,"length",6)!=0 && strncasecmp(meta->name,"fromDb",6)!=0){
	xmlNodePtr nc2 = xmlNewNode(ns_ows, BAD_CAST "AdditionalParameter");
	xmlNodePtr nc3 = xmlNewNode(ns_ows, BAD_CAST "Name");
	xmlAddChild(nc3,xmlNewText(BAD_CAST meta->name));
	xmlAddChild(nc2,nc3);
	if(fromDb<0){
	  char *mptr;
	  char* meta_values=strtok_r(meta->value,",",&mptr);
	  while(meta_values!=NULL){
	    xmlNodePtr nc4 = xmlNewNode(ns_ows, BAD_CAST "Value");
	    xmlAddChild(nc4,xmlNewText(BAD_CAST meta_values));
	    xmlAddChild(nc2,nc4);
	    meta_values=strtok_r(NULL,",",&mptr);
	  }
	}else{
	  xmlNodePtr nc4 = xmlNewNode(ns_ows, BAD_CAST "Value");
	  xmlAddChild(nc4,xmlNewCDataBlock(doc,BAD_CAST meta->value,strlen(meta->value)));
	  xmlAddChild(nc2,nc4);
	}
	xmlAddChild(nc1,nc2);
      }
      hasTitle=-1;
    }
    meta=meta->next;
    if(hasTitle<0){
      //xmlAddChild(nc,nc1);
      hasValue=1;
    }/*else
       if(ctitle!=NULL)
       free(ctitle);*/
  }
  if(length!=NULL)
    len=atoi(length->value);
  if(otitle!=NULL)
    len=1;
  if(cnt<len){
    xmlAddChild(nc,nc1);
    free(ctitle);
  }
}

/**
 * Add the ows:Metadata nodes relative to the profile registry
 *
 * @param n the XML node to add the ows:Metadata
 * @param ns_ows the ows XML namespace
 * @param ns_xlink the ows xlink namespace
 * @param reg the profile registry
 * @param main_conf the map containing the main configuration content
 * @param serv the service 
 */
void addInheritedMetadata(xmlNodePtr n,xmlNsPtr ns_ows,xmlNsPtr ns_xlink,registry* reg,maps* main_conf,service* serv){
  int vid=1;
  map* tmp1=getMap(serv->content,"extend");
  if(tmp1==NULL)
    tmp1=getMap(serv->content,"concept");
  if(tmp1!=NULL){
    map* level=getMap(serv->content,"level");
    if(level!=NULL){
      xmlNodePtr nc1 = xmlNewNode(ns_ows, BAD_CAST "Metadata");
      char* ckey=level->value;
      if(strncasecmp(level->value,"profile",7)==0)
	ckey=(char*)"generic";
      if(strncasecmp(level->value,"generic",7)==0)
	ckey=(char*)"concept";
      service* inherited=getServiceFromRegistry(reg,ckey,tmp1->value);
      if(inherited!=NULL){
	addInheritedMetadata(n,ns_ows,ns_xlink,reg,main_conf,inherited);
      }
      char cschema[71];
      sprintf(cschema,"%s%s",schemas[vid][7],ckey);
      map* regUrl=getMapFromMaps(main_conf,"main","registryUrl");
      map* regExt=getMapFromMaps(main_conf,"main","registryExt");
      char* registryUrl=(char*)malloc((strlen(regUrl->value)+strlen(ckey)+
				       (regExt!=NULL?strlen(regExt->value)+1:0)+
				       strlen(tmp1->value)+2)*sizeof(char));
      if(regExt!=NULL)
	sprintf(registryUrl,"%s%s/%s.%s",regUrl->value,ckey,tmp1->value,regExt->value);
      else
	sprintf(registryUrl,"%s%s/%s",regUrl->value,ckey,tmp1->value);
      xmlNewNsProp(nc1,ns_xlink,BAD_CAST "role",BAD_CAST cschema);
      xmlNewNsProp(nc1,ns_xlink,BAD_CAST "href",BAD_CAST registryUrl);
      free(registryUrl);
      xmlAddChild(n,nc1);
    }
  }
}

/**
 * Generate a ProcessDescription node for a servie and add it to a given node.
 * 
 * @param reg the profile registry
 * @param m the conf maps containing the main.cfg settings
 * @param nc the XML node to add the Process node
 * @param serv the servive structure created from the zcfg file
 * @return the generated wps:ProcessOfferings xmlNodePtr 
 */
void printDescribeProcessForProcess(registry *reg, maps* m,void* doc0,void* nc0,service* serv){
  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr n,nc1;
  xmlDocPtr doc=(xmlDocPtr) doc0;
  xmlNodePtr nc=(xmlNodePtr) nc0;
  xmlNodePtr nc2 = NULL;
  map* version=getMapFromMaps(m,"main","rversion");
  int vid=getVersionId(version->value);
  int fromDb=-1;
  map* serviceType=getMap(serv->content,"serviceType");
  map* test=getMap(serv->content,"fromDb");
  if(test!=NULL && strncasecmp(test->value,"true",4)==0)
    fromDb=1;

  n=nc;
  int wpsId=zooXmlAddNs(NULL,schemas[vid][3],"wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,schemas[vid][1],"ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(NULL,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];
  map* tmp1=NULL;

  if(vid==0){
    nc = xmlNewNode(NULL, BAD_CAST "ProcessDescription");
    attachAttributes(nc,ns,serv->content,vid,NULL);
  }
  else{
    nc2 = xmlNewNode(ns, BAD_CAST "ProcessOffering");
    // In case mode was defined in the ZCFG file then restrict the 
    // jobControlOptions value to this value. The dismiss is always 
    // supported whatever you can set in the ZCFG file.
    // cf. http://docs.opengeospatial.org/is/14-065/14-065.html#47 (Table 30)
    map* mode=getMap(serv->content,"mode");
    if(mode!=NULL){
      if( strncasecmp(mode->value,"sync",strlen(mode->value))==0 ||
	  strncasecmp(mode->value,"async",strlen(mode->value))==0 ){
	char toReplace[22];
	sprintf(toReplace,"%s-execute dismiss",mode->value);
	addToMap(serv->content,capabilities[vid][3],toReplace);
      }
    }
    attachAttributes(nc2,NULL,serv->content,vid,serviceType);
    map* level=getMap(serv->content,"level");
    if(level!=NULL && strcasecmp(level->value,"generic")==0)
      nc = xmlNewNode(ns, BAD_CAST "GenericProcess");
    else
      nc = xmlNewNode(ns, BAD_CAST "Process");
  }
  
  tmp1=getMapFromMaps(m,"lenv","level");
  addPrefix(m,tmp1,serv);
  printDescription(nc,ns_ows,serv->name,serv->content,vid);

  if(vid==0){
    tmp1=serv->metadata;
    while(tmp1!=NULL){
      addMetadata(tmp1,doc,nc,ns_ows,ns_xlink,vid);
      tmp1=tmp1->next;
    }
    tmp1=getMap(serv->content,"Profile");
    if(tmp1!=NULL && vid==0){
      nc1 = xmlNewNode(ns, BAD_CAST "Profile");
      xmlAddChild(nc1,xmlNewText(BAD_CAST tmp1->value));
      xmlAddChild(nc,nc1);
    }
  }else{
    tmp1=serv->metadata;
    addMetadata(tmp1,doc,nc,ns_ows,ns_xlink,vid);
    addInheritedMetadata(nc,ns_ows,ns_xlink,reg,m,serv);
    tmp1=serv->additional_parameters;
    if(vid!=0)
      addAdditionalParameters(tmp1,doc,nc,ns_ows,ns_xlink,fromDb);
  }

  if(serv->inputs!=NULL){
    elements* e=serv->inputs;
    if(vid==0){
      nc1 = xmlNewNode(NULL, BAD_CAST "DataInputs");
      printFullDescription(doc,1,e,"Input",ns,ns_ows,nc1,vid,fromDb,NULL);
      xmlAddChild(nc,nc1);
    }
    else{
      printFullDescription(doc,1,e,"wps:Input",ns,ns_ows,nc,vid,fromDb,NULL);
    }
  }

  elements* e=serv->outputs;
  if(vid==0){
    nc1 = xmlNewNode(NULL, BAD_CAST "ProcessOutputs");
    printFullDescription(doc,0,e,"Output",ns,ns_ows,nc1,vid,fromDb,NULL);
    xmlAddChild(nc,nc1);
  }
  else{
    printFullDescription(doc,0,e,"wps:Output",ns,ns_ows,nc,vid,fromDb,serviceType);
  }
  if(vid==0)
    xmlAddChild(n,nc);
  else if (nc2 != NULL) {	  
    xmlAddChild(nc2,nc);
    xmlAddChild(n,nc2);
  }

}

/**
 * Generate the required XML tree for the detailled metadata information of 
 * inputs or outputs
 *
 * @param in 1 in case of inputs, 0 for outputs
 * @param elem the elements structure containing the metadata information
 * @param type the name ("Input" or "Output") of the XML node to create
 * @param ns_ows the ows XML namespace
 * @param ns_ows the ows XML namespace
 * @param nc1 the XML node to use to add the created tree
 * @param vid the WPS version id (0 for 1.0.0, 1 for 2.0.0)
 * @param fromDb 1 in case the metadata comes from the DB, -1 in other cases
 * @param serviceType the serviceType found in the ZCFG file or the DB
 */
void printFullDescription(xmlDocPtr doc,int in,elements *elem,const char* type,xmlNsPtr ns,xmlNsPtr ns_ows,xmlNodePtr nc1,int vid,int fromDb,const map* serviceType){
  xmlNsPtr ns1=NULL;
  if(vid==1)
    ns1=ns;

  xmlNodePtr nc2,nc3,nc4,nc5,nc6,nc7,nc8,nc9,nc55=NULL,nc56;
  elements* e=elem;
  nc9=NULL;
  map* tmp1=NULL;
  while(e!=NULL){
    int default1=0;
    int isAnyValue=1;
    nc2 = xmlNewNode(NULL, BAD_CAST type);
    // Extract min/max Occurence information
    if(strstr(type,"Input")!=NULL){
      tmp1=getMap(e->content,"minOccurs");
      if(tmp1!=NULL){
	xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
      }else
	xmlNewProp(nc2,BAD_CAST "minOccurs",BAD_CAST "0");
      tmp1=getMap(e->content,"maxOccurs");
      if(tmp1!=NULL){
	if(strcasecmp(tmp1->value,"unbounded")!=0)
	  xmlNewProp(nc2,BAD_CAST tmp1->name,BAD_CAST tmp1->value);
	else
	  xmlNewProp(nc2,BAD_CAST "maxOccurs",BAD_CAST "1000");
      }else
	xmlNewProp(nc2,BAD_CAST "maxOccurs",BAD_CAST "1");
      if((tmp1=getMap(e->content,"maximumMegabytes"))!=NULL){
	if(vid==0)
	  xmlNewProp(nc2,BAD_CAST "maximumMegabytes",BAD_CAST tmp1->value);
      }
    }

    printDescription(nc2,ns_ows,e->name,e->content,vid);

    if(e->format!=NULL){
#ifdef USE_HPC	    
    DEFAULT_OUT:
#endif
      const char orderedFields[13][14]={
	"mimeType",
	"encoding",
	"schema",
	"dataType",
	"uom",
	"CRS",
	"AllowedValues",
	"range",
	"rangeMin",
	"rangeMax",
	"rangeClosure",
	"rangeSpace"
      };

      //Build the (Literal/Complex/BoundingBox)Data node
      if(strncmp(type,"Output",6)==0){
	if(strncasecmp(e->format,"LITERAL",7)==0)
	  nc3 = xmlNewNode(ns1, BAD_CAST "LiteralOutput");
	else if(strncasecmp(e->format,"COMPLEXDATA",strlen(e->format))==0)
	  nc3 = xmlNewNode(ns1, BAD_CAST "ComplexOutput");
	else if(strncasecmp(e->format,"BOUNDINGBOXDATA",strlen(e->format))==0)
	  nc3 = xmlNewNode(ns1, BAD_CAST "BoundingBoxOutput");
	else
	  nc3 = xmlNewNode(ns1, BAD_CAST e->format);
      }else{
	if(strncasecmp(e->format,"LITERALDATA",strlen(e->format))==0 ||
	   strncasecmp(e->format,"LITERALOUTPUT",strlen(e->format))==0){
	  nc3 = xmlNewNode(ns1, BAD_CAST "LiteralData");
	}
	else if(strncasecmp(e->format,"COMPLEXDATA",strlen(e->format))==0)
	  nc3 = xmlNewNode(ns1, BAD_CAST "ComplexData");
	else if(strncasecmp(e->format,"BOUNDINGBOXDATA",strlen(e->format))==0)
	  nc3 = xmlNewNode(ns1, BAD_CAST "BoundingBoxData");
	else
	  nc3 = xmlNewNode(ns1, BAD_CAST e->format);
      }

      iotype* _tmp0=NULL;
      iotype* _tmp=e->defaults;
      int datatype=0;
      bool hasUOM=false;
      bool hasUOM1=false;
      if(_tmp!=NULL){
	if(strcmp(e->format,"LiteralOutput")==0 ||
	   strcmp(e->format,"LiteralData")==0){
	  datatype=1;
	  if(vid==1){
	    nc4 = xmlNewNode(ns1, BAD_CAST "Format");
	    xmlNewProp(nc4,BAD_CAST "mimeType",BAD_CAST "text/plain");
	    xmlNewProp(nc4,BAD_CAST "default",BAD_CAST "true");
	    xmlAddChild(nc3,nc4);
	    nc5 = xmlNewNode(NULL, BAD_CAST "LiteralDataDomain");
	    nc55 = xmlNewNode(NULL, BAD_CAST "LiteralDataDomain");
	    xmlNewProp(nc5,BAD_CAST "default",BAD_CAST "true");
	  }
	  else{
	    nc4 = xmlNewNode(NULL, BAD_CAST "UOMs");
	    nc5 = xmlNewNode(NULL, BAD_CAST "Default");
	  }
	}
	else if(strcmp(e->format,"BoundingBoxOutput")==0 ||
		strcmp(e->format,"BoundingBoxData")==0){
	  datatype=2;
	  if(vid==0)
	    nc5 = xmlNewNode(NULL, BAD_CAST "Default");
	  else{
	    xmlNodePtr nc6 = xmlNewNode(ns1, BAD_CAST "Format");
	    xmlNewProp(nc6,BAD_CAST "mimeType",BAD_CAST "text/xml");
	    xmlNewProp(nc6,BAD_CAST "default",BAD_CAST "true");
	    xmlAddChild(nc3,nc6);
	    nc5 = xmlNewNode(NULL, BAD_CAST "SupportedCRS");
	  }
	}
	else{
	  if(vid==0)
	    nc4 = xmlNewNode(NULL, BAD_CAST "Default");
	  nc5 = xmlNewNode(ns1, BAD_CAST "Format");
	  if(vid==1){
	    xmlNewProp(nc5,BAD_CAST "default",BAD_CAST "true");
	    int oI=0;
	    for(oI=0;oI<3;oI++)
	      if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
		xmlNewProp(nc5,BAD_CAST orderedFields[oI],BAD_CAST tmp1->value);
	      }
	  }
	}

	if((tmp1=getMap(e->content,"maximumMegabytes"))!=NULL){
	  if(vid==1)
	    xmlNewProp(nc5,BAD_CAST "maximumMegabytes",BAD_CAST tmp1->value);
	}


	tmp1=_tmp->content;

	if(vid==0)
	  if((tmp1=getMap(_tmp->content,"DataType"))!=NULL){
	    nc8 = xmlNewNode(ns_ows, BAD_CAST "DataType");
	    xmlAddChild(nc8,xmlNewText(BAD_CAST tmp1->value));
	    char tmp[1024];
	    sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	    xmlNewNsProp(nc8,ns_ows,BAD_CAST "reference",BAD_CAST tmp);
	    if(vid==0)
	      xmlAddChild(nc3,nc8);
	    else
	      xmlAddChild(nc5,nc8);
	    datatype=1;
	  }

	bool isInput=false;
	if(strncmp(type,"Input",5)==0 || strncmp(type,"wps:Input",9)==0){
	  isInput=true;
	  if((tmp1=getMap(_tmp->content,"AllowedValues"))!=NULL){
	    nc6 = xmlNewNode(ns_ows, BAD_CAST "AllowedValues");
	    char *token,*saveptr1;
	    token=strtok_r(tmp1->value,",",&saveptr1);
	    while(token!=NULL){
	      nc7 = xmlNewNode(ns_ows, BAD_CAST "Value");
	      char *tmps=zStrdup(token);
	      tmps[strlen(tmps)]=0;
	      nc8 = xmlNewText(BAD_CAST tmps);
	      xmlAddChild(nc7,nc8);
	      free(tmps);
	      xmlAddChild(nc6,nc7);
	      token=strtok_r(NULL,",",&saveptr1);
	    }
	    if(getMap(_tmp->content,"range")!=NULL ||
	       getMap(_tmp->content,"rangeMin")!=NULL ||
	       getMap(_tmp->content,"rangeMax")!=NULL ||
	       getMap(_tmp->content,"rangeClosure")!=NULL )
	      goto doRange;
	    if(vid==0)
	      xmlAddChild(nc3,nc6);
	    else
	      xmlAddChild(nc5,nc6);
	    isAnyValue=-1;
	  }

	  tmp1=getMap(_tmp->content,"range");
	  if(tmp1==NULL)
	    tmp1=getMap(_tmp->content,"rangeMin");
	  if(tmp1==NULL)
	    tmp1=getMap(_tmp->content,"rangeMax");
	
	  if(tmp1!=NULL && isAnyValue==1){
	    nc6 = xmlNewNode(ns_ows, BAD_CAST "AllowedValues");
	  doRange:
	  
	    /**
	     * Range: Table 46 OGC Web Services Common Standard
	     */
	    nc8 = xmlNewNode(ns_ows, BAD_CAST "Range");
	  
	    map* tmp0=getMap(tmp1,"range");
	    if(tmp0!=NULL){
	      char* pToken;
	      char* orig=zStrdup(tmp0->value);
	      /**
	       * RangeClosure: Table 47 OGC Web Services Common Standard
	       */
	      const char *tmp="closed";
	      if(orig[0]=='[' && orig[strlen(orig)-1]=='[')
		tmp="closed-open";
	      else
		if(orig[0]==']' && orig[strlen(orig)-1]==']')
		  tmp="open-closed";
		else
		  if(orig[0]==']' && orig[strlen(orig)-1]=='[')
		    tmp="open";
	      xmlNewNsProp(nc8,ns_ows,BAD_CAST "rangeClosure",BAD_CAST tmp);
	      pToken=strtok(orig,",");
	      int nci0=0;
	      while(pToken!=NULL){
		char *tmpStr=(char*) malloc((strlen(pToken))*sizeof(char));
		if(nci0==0){
		  nc7 = xmlNewNode(ns_ows, BAD_CAST "MinimumValue");
		  strncpy( tmpStr, pToken+1, strlen(pToken)-1 );
		  tmpStr[strlen(pToken)-1] = '\0';
		}else{
		  nc7 = xmlNewNode(ns_ows, BAD_CAST "MaximumValue");
		  const char* bkt;
		  if ( ( bkt = strchr(pToken, '[') ) != NULL || ( bkt = strchr(pToken, ']') ) != NULL ){
		    strncpy( tmpStr, pToken, bkt - pToken );
		    tmpStr[bkt - pToken] = '\0';
		  }
		}
		xmlAddChild(nc7,xmlNewText(BAD_CAST tmpStr));
		free(tmpStr);
		xmlAddChild(nc8,nc7);
		nci0++;
		pToken = strtok(NULL,",");
	      }		    
	      if(getMap(tmp1,"rangeSpacing")==NULL){
		nc7 = xmlNewNode(ns_ows, BAD_CAST "Spacing");
		xmlAddChild(nc7,xmlNewText(BAD_CAST "1"));
		xmlAddChild(nc8,nc7);
	      }
	      free(orig);
	    }else{
	    
	      tmp0=getMap(tmp1,"rangeMin");
	      if(tmp0!=NULL){
		nc7 = xmlNewNode(ns_ows, BAD_CAST "MinimumValue");
		xmlAddChild(nc7,xmlNewText(BAD_CAST tmp0->value));
		xmlAddChild(nc8,nc7);
	      }else{
		nc7 = xmlNewNode(ns_ows, BAD_CAST "MinimumValue");
		xmlAddChild(nc8,nc7);
	      }
	      tmp0=getMap(tmp1,"rangeMax");
	      if(tmp0!=NULL){
		nc7 = xmlNewNode(ns_ows, BAD_CAST "MaximumValue");
		xmlAddChild(nc7,xmlNewText(BAD_CAST tmp0->value));
		xmlAddChild(nc8,nc7);
	      }else{
		nc7 = xmlNewNode(ns_ows, BAD_CAST "MaximumValue");
		xmlAddChild(nc8,nc7);
	      }
	      tmp0=getMap(tmp1,"rangeSpacing");
	      if(tmp0!=NULL){
		nc7 = xmlNewNode(ns_ows, BAD_CAST "Spacing");
		xmlAddChild(nc7,xmlNewText(BAD_CAST tmp0->value));
		xmlAddChild(nc8,nc7);
	      }
	      tmp0=getMap(tmp1,"rangeClosure");
	      if(tmp0!=NULL){
		const char *tmp="closed";
		if(strcasecmp(tmp0->value,"co")==0)
		  tmp="closed-open";
		else
		  if(strcasecmp(tmp0->value,"oc")==0)
		    tmp="open-closed";
		  else
		    if(strcasecmp(tmp0->value,"o")==0)
		      tmp="open";
		xmlNewNsProp(nc8,ns_ows,BAD_CAST "rangeClosure",BAD_CAST tmp);
	      }else
		xmlNewNsProp(nc8,ns_ows,BAD_CAST "rangeClosure",BAD_CAST "closed");
	    }
	    if(_tmp0==NULL){
	      xmlAddChild(nc6,nc8);
	      _tmp0=e->supported;
	      if(_tmp0!=NULL &&
		 (getMap(_tmp0->content,"range")!=NULL ||
		  getMap(_tmp0->content,"rangeMin")!=NULL ||
		  getMap(_tmp0->content,"rangeMax")!=NULL ||
		  getMap(_tmp0->content,"rangeClosure")!=NULL )){
		tmp1=_tmp0->content;
		goto doRange;
	      }
	    }else{
	      _tmp0=_tmp0->next;
	      if(_tmp0!=NULL){
		xmlAddChild(nc6,nc8);
		if(getMap(_tmp0->content,"range")!=NULL ||
		   getMap(_tmp0->content,"rangeMin")!=NULL ||
		   getMap(_tmp0->content,"rangeMax")!=NULL ||
		   getMap(_tmp0->content,"rangeClosure")!=NULL ){
		  tmp1=_tmp0->content;
		  goto doRange;
		}
	      }
	    }
	    xmlAddChild(nc6,nc8);
	    if(vid==0)
	      xmlAddChild(nc3,nc6);
	    else{
	      xmlAddChild(nc5,nc6);
	      xmlAddChild(nc55,nc6);
	    }
	    isAnyValue=-1;
	  }
	
	}
      
	int oI=0;
	/*if(vid==0)*/ {
	  for(oI=0;oI<12;oI++)
	    if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
#ifdef DEBUG
	      printf("DATATYPE DEFAULT ? %s\n",tmp1->name);
#endif
	      if(strcmp(tmp1->name,"asReference")!=0 &&
		 strncasecmp(tmp1->name,"DataType",8)!=0 &&
		 strcasecmp(tmp1->name,"extension")!=0 &&
		 strcasecmp(tmp1->name,"value")!=0 &&
		 strcasecmp(tmp1->name,"AllowedValues")!=0 &&
		 strncasecmp(tmp1->name,"range",5)!=0){
		if(datatype!=1){
		  if(datatype==2 && vid==1){
		    nc9 = xmlNewNode(ns, BAD_CAST "SupportedCRS");
		    xmlNewProp(nc9,BAD_CAST "default",BAD_CAST "true");
		  }
		  else{
		    char *tmp2=zCapitalize1(tmp1->name);
		    nc9 = xmlNewNode(NULL, BAD_CAST tmp2);
		    free(tmp2);
		  }
		}
		else{
		  char *tmp2=zCapitalize(tmp1->name);
		  nc9 = xmlNewNode(ns_ows, BAD_CAST tmp2);
		  free(tmp2);
		}
		xmlAddChild(nc9,xmlNewText(BAD_CAST tmp1->value));
		if(vid==0 || oI>=3){
		  if(vid==0 || oI!=4){
		    if(datatype==2 && vid==1)
		      xmlAddChild(nc3,nc9);
		    else
		      xmlAddChild(nc5,nc9);
		  }
		}
		else{
		  xmlFreeNode(nc9);
		}
		if(strcasecmp(tmp1->name,"uom")==0)
		  hasUOM1=true;
		hasUOM=true;
	      }
	    }
	}
	if(datatype!=2){
	  if(hasUOM==true){
	    if(vid==0){
	      xmlAddChild(nc4,nc5);
	      xmlAddChild(nc3,nc4);
	    }
	    else{
	      xmlAddChild(nc3,nc5);
	    }
	  }else{
	    if(hasUOM1==false && vid==0){
	      if(nc5!=NULL)
		xmlFreeNode(nc5);
	      if(datatype==1){
		xmlFreeNode(nc4);
	      }
	    }
	    else{
	      xmlAddChild(nc3,nc5);
	    }
	  }
	}else{
	  if(vid==0)
	    xmlAddChild(nc3,nc5);
	  else
	    xmlFreeNode(nc5);
	}
      
	if(datatype==0 && default1<0){
	  xmlFreeNode(nc5);
	  if(datatype!=2){
	    xmlFreeNode(nc4);
	  }
	}


	if((isInput || vid==1) && datatype==1 &&
	   getMap(_tmp->content,"AllowedValues")==NULL &&
	   getMap(_tmp->content,"range")==NULL &&
	   getMap(_tmp->content,"rangeMin")==NULL &&
	   getMap(_tmp->content,"rangeMax")==NULL &&
	   getMap(_tmp->content,"rangeClosure")==NULL ){
	  tmp1=getMap(_tmp->content,"dataType");
	  // We were tempted to define default value for boolean as {true,false}
	  if(tmp1!=NULL && strcasecmp(tmp1->value,"boolean")==0){
	    nc6 = xmlNewNode(ns_ows, BAD_CAST "AllowedValues");
	    nc7 = xmlNewNode(ns_ows, BAD_CAST "Value");
	    xmlAddChild(nc7,xmlNewText(BAD_CAST "true"));
	    xmlAddChild(nc6,nc7);
	    nc7 = xmlNewNode(ns_ows, BAD_CAST "Value");
	    xmlAddChild(nc7,xmlNewText(BAD_CAST "false"));
	    xmlAddChild(nc6,nc7);
	    if(vid==0)
	      xmlAddChild(nc3,nc6);
	    else{
	      xmlAddChild(nc5,nc6);
	      xmlAddChild(nc55,nc6);
	    }
	  }
	  else
	    if(vid==0)
	      xmlAddChild(nc3,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
	    else{
	      xmlAddChild(nc5,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
	      xmlAddChild(nc55,xmlNewNode(ns_ows, BAD_CAST "AnyValue"));
	    }
	}

	if(vid==1){
	  if((tmp1=getMap(_tmp->content,"DataType"))!=NULL){
	    nc8 = xmlNewNode(ns_ows, BAD_CAST "DataType");
	    xmlAddChild(nc8,xmlNewText(BAD_CAST tmp1->value));
	    char tmp[1024];
	    sprintf(tmp,"http://www.w3.org/TR/xmlschema-2/#%s",tmp1->value);
	    xmlNewNsProp(nc8,ns_ows,BAD_CAST "reference",BAD_CAST tmp);
	    if(vid==0)
	      xmlAddChild(nc3,nc8);
	    else{
	      xmlAddChild(nc5,nc8);
	      xmlAddChild(nc55,nc8);
	    }
	    datatype=1;
	  }
	  if(hasUOM==true){
	    tmp1=getMap(_tmp->content,"uom");
	    if(tmp1!=NULL){
	      xmlAddChild(nc5,nc9);
	    }
	  }
	  if(e->defaults!=NULL && (tmp1=getMap(e->defaults->content,"value"))!=NULL){
	    nc7 = xmlNewNode(ns_ows, BAD_CAST "DefaultValue");
	    xmlAddChild(nc7,xmlNewText(BAD_CAST tmp1->value));
	    xmlAddChild(nc5,nc7);
	    xmlAddChild(nc55,nc7);
	  }
	}

	map* metadata=e->metadata;
	xmlNodePtr n=NULL;
	int xlinkId=zooXmlAddNs(n,"http://www.w3.org/1999/xlink","xlink");
	xmlNsPtr ns_xlink=usedNs[xlinkId];

	addMetadata(metadata,doc,nc2,ns_ows,ns_xlink,vid);
	if(vid!=0)
	  addAdditionalParameters(e->additional_parameters,doc,nc2,ns_ows,ns_xlink,fromDb);

      }

      _tmp=e->supported;
      if(_tmp==NULL && datatype==0)
	_tmp=e->defaults;

      int hasSupported=-1;

      while(_tmp!=NULL){
	
	if(hasSupported<0){
	  if(datatype==0){
	    if(vid==0)
	      nc4 = xmlNewNode(NULL, BAD_CAST "Supported");
	    nc5 = xmlNewNode(ns1, BAD_CAST "Format");
	    if(vid==1){
	      int oI=0;
	      for(oI=0;oI<3;oI++)
		if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
		  xmlNewProp(nc5,BAD_CAST orderedFields[oI],BAD_CAST tmp1->value);
		}
	    }
	  }
	  else
	    if(vid==0)
	      nc5 = xmlNewNode(NULL, BAD_CAST "Supported");
	  hasSupported=0;
	}else
	  if(datatype==0){
	    nc5 = xmlNewNode(ns1, BAD_CAST "Format");
	    if(vid==1){
	      int oI=0;
	      for(oI=0;oI<3;oI++)
		if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
		  xmlNewProp(nc5,BAD_CAST orderedFields[oI],BAD_CAST tmp1->value);
		}
	    }
	  }

	if((tmp1=getMap(e->content,"maximumMegabytes"))!=NULL){
	  if(vid==1)
	    xmlNewProp(nc5,BAD_CAST "maximumMegabytes",BAD_CAST tmp1->value);
	}

	tmp1=_tmp->content;
	int oI=0;
	for(oI=0;oI<6;oI++)
	  if((tmp1=getMap(_tmp->content,orderedFields[oI]))!=NULL){
#ifdef DEBUG
	    printf("DATATYPE SUPPORTED ? %s\n",tmp1->name);
#endif
	    if(strcmp(tmp1->name,"asReference")!=0 && 
	       strcmp(tmp1->name,"value")!=0 && 
	       strcmp(tmp1->name,"DataType")!=0 &&
	       strcasecmp(tmp1->name,"extension")!=0){
	      if(datatype!=1){
		if(datatype==2 && vid==1){
		  nc6 = xmlNewNode(ns, BAD_CAST "SupportedCRS");
		}
		else{
		  char *tmp2=zCapitalize1(tmp1->name);
		  nc6 = xmlNewNode(NULL, BAD_CAST tmp2);
		  free(tmp2);
		}
	      }
	      else{
		char *tmp2=zCapitalize(tmp1->name);
		nc6 = xmlNewNode(ns_ows, BAD_CAST tmp2);
		free(tmp2);
	      }
	      if(datatype==2){
		char *tmpv,*tmps;
		tmps=strtok_r(tmp1->value,",",&tmpv);
		while(tmps){
		  xmlAddChild(nc6,xmlNewText(BAD_CAST tmps));
		  tmps=strtok_r(NULL,",",&tmpv);
		  if(tmps){
		    char *tmp2=zCapitalize1(tmp1->name);
		    nc6 = xmlNewNode(NULL, BAD_CAST tmp2);
		    free(tmp2);
		  }
		}
	      }
	      else{
		xmlAddChild(nc6,xmlNewText(BAD_CAST tmp1->value));
	      }
	      if(vid==0 || oI>=3){
		if(vid==0 || oI!=4){
		  if(datatype==2 && vid==1)
		    xmlAddChild(nc3,nc6);
		  else
		    xmlAddChild(nc5,nc6);
		}
		else{
		  if(oI!=4)
		    xmlFreeNode(nc6);
		  else{
		    nc56=xmlCopyNode(nc55,1);
		    xmlReplaceNode(xmlGetLastChild(nc56),nc6);
		    xmlAddChild(nc3,nc56);
		  }
		}
	      }
	      else{
		xmlFreeNode(nc6);
	      }
	    }
	    tmp1=tmp1->next;
	  }
	if(hasSupported<=0){
	  if(datatype==0){
	    if(vid==0){
	      xmlAddChild(nc4,nc5);
	      xmlAddChild(nc3,nc4);
	    }
	    else{
	      xmlAddChild(nc3,nc5);
	    }

	  }else{
	    if(datatype==2 && vid==0)
	      xmlAddChild(nc3,nc5);
	  }
	  hasSupported=1;
	}
	else
	  if(datatype==0){
	    if(vid==0){
	      xmlAddChild(nc4,nc5);
	      xmlAddChild(nc3,nc4);
	    }
	    else{
	      xmlAddChild(nc3,nc5);
	    }
	  }
	  else
	    if(datatype==0){
	      xmlFreeNode(nc4);
	      xmlAddChild(nc3,nc5);
	    }

	_tmp=_tmp->next;
      }

      if(hasSupported==0){
	if(datatype==0 && vid!=0){
	  xmlFreeNode(nc4);
	}
	xmlFreeNode(nc5);
      }

      _tmp=e->defaults;
      if(datatype==1 && hasUOM1==true){
	if(vid==0){
	  xmlAddChild(nc4,nc5);
	  xmlAddChild(nc3,nc4);
	}
	else{
	  xmlAddChild(nc3,nc5);
	}
      }

      if(vid==0 && _tmp!=NULL && (tmp1=getMap(_tmp->content,"value"))!=NULL){
	nc7 = xmlNewNode(NULL, BAD_CAST "DefaultValue");
	xmlAddChild(nc7,xmlNewText(BAD_CAST tmp1->value));
	xmlAddChild(nc3,nc7);
      }
      xmlAddChild(nc2,nc3);
    }else{
      if(e->child!=NULL && vid!=0){
	printFullDescription(doc,in,e->child,type,ns,ns_ows,nc2,vid,fromDb,NULL);
      }
    }
    if(e->child!=NULL && vid==0){
      elements* children=dupElements(e->child);
      elements* cursor=children;
      while(cursor!=NULL){
	elements* ccursor=cursor;
	char* tmp=zStrdup(ccursor->name);
	free(ccursor->name);
	ccursor->name=(char*)malloc((strlen(tmp)+strlen(e->name)+2)*sizeof(char));
	sprintf(ccursor->name,"%s.%s",e->name,tmp);
	cursor=cursor->next;
      }
      printFullDescription(doc,in,children,type,ns,ns_ows,nc1,vid,fromDb,serviceType);
      freeElements(&children);
      free(children);
    }else{
      if(nc2!=NULL){
	xmlAddChild(nc1,nc2);
      }
    }
    e=e->next;
  }
}

/**
 * Generate a wps:Execute XML document.
 * 
 * @param m the conf maps containing the main.cfg settings
 * @param request the map representing the HTTP request
 * @param pid the process identifier linked to a service
 * @param serv the serv structure created from the zcfg file
 * @param service the service name
 * @param status the status returned by the service
 * @param inputs the inputs provided
 * @param outputs the outputs generated by the service
 */
void printProcessResponse(maps* m,map* request, int pid,service* serv,const char* service,int status,maps* inputs,maps* outputs){	
  xmlNsPtr ns,ns_ows,ns_xlink;
  xmlNodePtr nr,n,nc,nc1=NULL,nc3;
  xmlDocPtr doc;
  time_t time1;  
  time(&time1);
  nr=NULL;

  doc = xmlNewDoc(BAD_CAST "1.0");
  map* version=getMapFromMaps(m,"main","rversion");
  int vid=getVersionId(version->value);
  n = printWPSHeader(doc,m,"Execute",root_nodes[vid][2],(version!=NULL?version->value:"1.0.0"),2);
  int wpsId=zooXmlAddNs(NULL,schemas[vid][2],"wps");
  ns=usedNs[wpsId];
  int owsId=zooXmlAddNs(NULL,schemas[vid][1],"ows");
  ns_ows=usedNs[owsId];
  int xlinkId=zooXmlAddNs(NULL,"http://www.w3.org/1999/xlink","xlink");
  ns_xlink=usedNs[xlinkId];
  bool hasStoredExecuteResponse=false;
  char stored_path[1024];
  memset(stored_path,0,1024);
  
  if(vid==0){
    char tmp[256];
    char url[1024];
    memset(tmp,0,256);
    memset(url,0,1024);
    maps* tmp_maps=getMaps(m,"main");
    if(tmp_maps!=NULL && tmp_maps->content!=NULL){
      map* tmpm1=getMap(tmp_maps->content,"serverAddress");
      /**
       * Check if the ZOO Service GetStatus is available in the local directory.
       * If yes, then it uses a reference to an URL which the client can access
       * to get information on the status of a running Service (using the 
       * percentCompleted attribute). 
       * Else fallback to the initial method using the xml file to write in ...
       */
      map* cwdMap=getMapFromMaps(m,"main","servicePath");
      struct stat myFileInfo;
      int statRes;
      char file_path[1024];
      if(cwdMap!=NULL){
	sprintf(file_path,"%s/GetStatus.zcfg",cwdMap->value);
      }else{
	char ntmp[1024];
#ifndef WIN32
	getcwd(ntmp,1024);
#else
	_getcwd(ntmp,1024);
#endif
	sprintf(file_path,"%s/GetStatus.zcfg",ntmp);
      }
      statRes=stat(file_path,&myFileInfo);
      if(statRes==0){
	char currentSid[128];
	map* tmpm=getMap(tmp_maps->content,"rewriteUrl");
	map *tmp_lenv=NULL;
	tmp_lenv=getMapFromMaps(m,"lenv","usid");
	if(tmp_lenv==NULL)
	  sprintf(currentSid,"%i",pid);
	else
	  sprintf(currentSid,"%s",tmp_lenv->value);
	if(tmpm==NULL || strcasecmp(tmpm->value,"false")==0){
	  sprintf(url,"%s?request=Execute&service=WPS&version=1.0.0&Identifier=GetStatus&DataInputs=sid=%s&RawDataOutput=Result",tmpm1->value,currentSid);
	}else{
	  if(strlen(tmpm->value)>0)
	    if(strcasecmp(tmpm->value,"true")!=0)
	      sprintf(url,"%s/%s/GetStatus/%s",tmpm1->value,tmpm->value,currentSid);
	    else
	      sprintf(url,"%s/GetStatus/%s",tmpm1->value,currentSid);
	  else
	    sprintf(url,"%s/?request=Execute&service=WPS&version=1.0.0&Identifier=GetStatus&DataInputs=sid=%s&RawDataOutput=Result",tmpm1->value,currentSid);
	}
      }else{
	int lpid;
	map* tmpm2=getMapFromMaps(m,"lenv","usid");
	map* tmpm3=getMap(tmp_maps->content,"tmpUrl");
	if(tmpm1!=NULL && tmpm3!=NULL){
	  if( strncasecmp( tmpm3->value, "http://", 7) == 0 ||
	      strncasecmp( tmpm3->value, "https://", 8 ) == 0 ){
	    sprintf(url,"%s/%s_%s.xml",tmpm3->value,service,tmpm2->value);
	  }else
	    sprintf(url,"%s/%s_%s.xml",tmpm1->value,service,tmpm2->value);
	}
      }
      if(tmpm1!=NULL){
	sprintf(tmp,"%s",tmpm1->value);
      }
      int lpid;
      map* tmpm2=getMapFromMaps(m,"lenv","usid");
      tmpm1=getMapFromMaps(m,"main","TmpPath");
      sprintf(stored_path,"%s/%s_%s.xml",tmpm1->value,service,tmpm2->value);
    }

    xmlNewProp(n,BAD_CAST "serviceInstance",BAD_CAST tmp);
    map* test=getMap(request,"storeExecuteResponse");
    if(test!=NULL && strcasecmp(test->value,"true")==0){
      xmlNewProp(n,BAD_CAST "statusLocation",BAD_CAST url);
      hasStoredExecuteResponse=true;
    }

    nc = xmlNewNode(ns, BAD_CAST "Process");
    map* tmp2=getMap(serv->content,"processVersion");
    if(tmp2!=NULL)
      xmlNewNsProp(nc,ns,BAD_CAST "processVersion",BAD_CAST tmp2->value);
    else
      xmlNewNsProp(nc,ns,BAD_CAST "processVersion",BAD_CAST "1");
  
    map* tmpI=getMapFromMaps(m,"lenv","oIdentifier");
    printDescription(nc,ns_ows,tmpI->value,serv->content,0);

    xmlAddChild(n,nc);

    nc = xmlNewNode(ns, BAD_CAST "Status");
    const struct tm *tm;
    size_t len;
    time_t now;
    char *tmp1;
    map *tmpStatus;
  
    now = time ( NULL );
    tm = localtime ( &now );

    tmp1 = (char*)malloc((TIME_SIZE+1)*sizeof(char));

    len = strftime ( tmp1, TIME_SIZE, "%Y-%m-%dT%I:%M:%SZ", tm );

    xmlNewProp(nc,BAD_CAST "creationTime",BAD_CAST tmp1);

    char sMsg[2048];
    switch(status){
    case SERVICE_SUCCEEDED:
      nc1 = xmlNewNode(ns, BAD_CAST "ProcessSucceeded");
      sprintf(sMsg,_("The service \"%s\" ran successfully."),serv->name);
      nc3=xmlNewText(BAD_CAST sMsg);
      xmlAddChild(nc1,nc3);
      break;
    case SERVICE_STARTED:
      nc1 = xmlNewNode(ns, BAD_CAST "ProcessStarted");
      tmpStatus=getMapFromMaps(m,"lenv","status");
      xmlNewProp(nc1,BAD_CAST "percentCompleted",BAD_CAST tmpStatus->value);
      sprintf(sMsg,_("The ZOO service \"%s\" is currently running. Please reload this document to get the up-to-date status of the service."),serv->name);
      nc3=xmlNewText(BAD_CAST sMsg);
      xmlAddChild(nc1,nc3);
      break;
    case SERVICE_ACCEPTED:
      nc1 = xmlNewNode(ns, BAD_CAST "ProcessAccepted");
      sprintf(sMsg,_("The service \"%s\" was accepted by the ZOO-Kernel and is running as a background task. Please access the URL in the statusLocation attribute provided in this document to get the up-to-date status and results."),serv->name);
      nc3=xmlNewText(BAD_CAST sMsg);
      xmlAddChild(nc1,nc3);
      break;
    case SERVICE_FAILED:
      nc1 = xmlNewNode(ns, BAD_CAST "ProcessFailed");
      map *errorMap;
      map *te;
      te=getMapFromMaps(m,"lenv","code");
      if(te!=NULL)
	errorMap=createMap("code",te->value);
      else
	errorMap=createMap("code","NoApplicableCode");
      te=getMapFromMaps(m,"lenv","message");
      if(te!=NULL)
	addToMap(errorMap,"text",_ss(te->value));
      else
	addToMap(errorMap,"text",_("No more information available"));
      nc3=createExceptionReportNode(m,errorMap,0);
      freeMap(&errorMap);
      free(errorMap);
      xmlAddChild(nc1,nc3);
      break;
    default :
      printf(_("error code not know : %i\n"),status);
      //exit(1);
      break;
    }
    xmlAddChild(nc,nc1);
    xmlAddChild(n,nc);
    free(tmp1);

#ifdef DEBUG
    fprintf(stderr,"printProcessResponse %d\n",__LINE__);
#endif

    map* lineage=getMap(request,"lineage");
    if(lineage!=NULL && strcasecmp(lineage->value,"true")==0){
      nc = xmlNewNode(ns, BAD_CAST "DataInputs");
      maps* mcursor=inputs;
      elements* scursor=NULL;
      while(mcursor!=NULL /*&& scursor!=NULL*/){
	scursor=getElements(serv->inputs,mcursor->name);
	printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Input",vid);
	mcursor=mcursor->next;
      }
      xmlAddChild(n,nc);

      nc = xmlNewNode(ns, BAD_CAST "OutputDefinitions");
      mcursor=outputs;
      scursor=NULL;
      while(mcursor!=NULL){
	scursor=getElements(serv->outputs,mcursor->name);
	printOutputDefinitions(doc,nc,ns,ns_ows,scursor,mcursor,"Output");
	mcursor=mcursor->next;
      }
      xmlAddChild(n,nc);
    }
  }
  
  /**
   * Display the process output only when requested !
   */
  if(status==SERVICE_SUCCEEDED){
    if(vid==0){
      nc = xmlNewNode(ns, BAD_CAST "ProcessOutputs");
    }
    maps* mcursor=outputs;
    elements* scursor=serv->outputs;
    map* testResponse=getMap(request,"RawDataOutput");	
    if(testResponse==NULL)
      testResponse=getMap(request,"ResponseDocument");
    while(mcursor!=NULL){		
      map* tmp0=getMap(mcursor->content,"inRequest");
      scursor=getElements(serv->outputs,mcursor->name);	  
      if(scursor!=NULL){		  
	if(testResponse==NULL || tmp0==NULL){		
	  if(vid==0)
	    printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Output",vid);
	  else
	    printIOType(doc,n,ns,ns_ows,ns_xlink,scursor,mcursor,"Output",vid);
	}
	else
	  if(tmp0!=NULL && strncmp(tmp0->value,"true",4)==0){		  
	    if(vid==0)
	      printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Output",vid);
	    else
	      printIOType(doc,n,ns,ns_ows,ns_xlink,scursor,mcursor,"Output",vid);
	  }	
      }else
	/**
	 * In case there was no definition found in the ZCFG file but 
	 * present in the service code
	 */
	if(vid==0)
	  printIOType(doc,nc,ns,ns_ows,ns_xlink,scursor,mcursor,"Output",vid);
	else
	  printIOType(doc,n,ns,ns_ows,ns_xlink,scursor,mcursor,"Output",vid);
      mcursor=mcursor->next;
    }	
    if(vid==0)
      xmlAddChild(n,nc);
  }  
  if(vid==0 && 
     hasStoredExecuteResponse==true 
     && status!=SERVICE_STARTED 
#ifndef WIN32
     && status!=SERVICE_ACCEPTED
#endif
     ){
#ifndef RELY_ON_DB
    semid lid=acquireLock(m);//,1);
    if(lid<0){
      /* If the lock failed */
      errorException(m,_("Lock failed."),"InternalError",NULL);
      xmlFreeDoc(doc);
      xmlCleanupParser();
      zooXmlCleanupNs();
      return;
    }
    else{
#endif
      /* We need to write the ExecuteResponse Document somewhere */
      FILE* output=fopen(stored_path,"w");
      if(output==NULL){
	/* If the file cannot be created return an ExceptionReport */
	char tmpMsg[1024];
	sprintf(tmpMsg,_("Unable to create the file \"%s\" for storing the ExecuteResponse."),stored_path);

	errorException(m,tmpMsg,"InternalError",NULL);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	zooXmlCleanupNs();
#ifndef RELY_ON_DB
	unlockShm(lid);
#endif
	return;
      }
      xmlChar *xmlbuff;
      int buffersize;
      xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, "UTF-8", 1);
      fwrite(xmlbuff,1,xmlStrlen(xmlbuff)*sizeof(char),output);
      xmlFree(xmlbuff);
      fclose(output);
#ifndef RELY_ON_DB
#ifdef DEBUG
      fprintf(stderr,"UNLOCK %s %d !\n",__FILE__,__LINE__);
#endif
      unlockShm(lid);
      map* v=getMapFromMaps(m,"lenv","sid");
      // Remove the lock when running as a normal task
      if(getpid()==atoi(v->value)){
	removeShmLock (m, 1);
      }
    }
#endif
  }

  printDocument(m,doc,pid);

  xmlCleanupParser();
  zooXmlCleanupNs();
}

/**
 * Print a XML document.
 * 
 * @param m the conf maps containing the main.cfg settings
 * @param doc the XML document
 * @param pid the process identifier linked to a service
 */
void printDocument(maps* m, xmlDocPtr doc,int pid){
  char *encoding=getEncoding(m);
  if(pid==getpid()){
    printHeaders(m);
    printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
  }
  xmlChar *xmlbuff;
  int buffersize;
  /*
   * Dump the document to a buffer and print it on stdout
   * for demonstration purposes.
   */
  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  printf("%s",xmlbuff);
  fflush(stdout);
  /*
   * Free associated memory.
   */
  xmlFree(xmlbuff);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  zooXmlCleanupNs();
}

/**
 * Print a XML document.
 * 
 * @param doc the XML document (unused)
 * @param nc the XML node to add the output definition
 * @param ns_wps the wps XML namespace
 * @param ns_ows the ows XML namespace
 * @param e the output elements 
 * @param m the conf maps containing the main.cfg settings
 * @param type the type (unused)
 */
void printOutputDefinitions(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,elements* e,maps* m,const char* type){
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
    if(strncasecmp(tmp->name,"MIMETYPE",strlen(tmp->name))==0
       || strncasecmp(tmp->name,"ENCODING",strlen(tmp->name))==0
       || strncasecmp(tmp->name,"SCHEMA",strlen(tmp->name))==0
       || strncasecmp(tmp->name,"UOM",strlen(tmp->name))==0)
      xmlNewProp(nc1,BAD_CAST tmp->name,BAD_CAST tmp->value);
    tmp=tmp->next;
  }
  tmp=getMap(e->defaults->content,"asReference");
  if(tmp==NULL)
    xmlNewProp(nc1,BAD_CAST "asReference",BAD_CAST "false");

  tmp=e->content;

  printDescription(nc1,ns_ows,m->name,e->content,0);

  xmlAddChild(nc,nc1);

}

/**
 * Generate XML nodes describing inputs or outputs metadata.
 * 
 * @param doc the XML document 
 * @param nc the XML node to add the definition
 * @param ns_wps the wps namespace
 * @param ns_ows the ows namespace
 * @param ns_xlink the xlink namespace
 * @param e the output elements 
 * @param m the conf maps containing the main.cfg settings
 * @param type the type
 */
void printIOType(xmlDocPtr doc,xmlNodePtr nc,xmlNsPtr ns_wps,xmlNsPtr ns_ows,xmlNsPtr ns_xlink,elements* e,maps* m,const char* type,int vid){	
  xmlNodePtr nc1,nc2,nc3;
  nc1=xmlNewNode(ns_wps, BAD_CAST type);
  map *tmp=NULL;
  if(e!=NULL)
    tmp=e->content;
  else
    tmp=m->content;  
  if(vid==0){
    nc2=xmlNewNode(ns_ows, BAD_CAST "Identifier");
    if(e!=NULL)
      nc3=xmlNewText(BAD_CAST e->name);
    else
      nc3=xmlNewText(BAD_CAST m->name);
    
    xmlAddChild(nc2,nc3);
    xmlAddChild(nc1,nc2);
  
    xmlAddChild(nc,nc1);

    if(e!=NULL)
      tmp=getMap(e->content,"Title");
    else
      tmp=getMap(m->content,"Title");
    
    if(tmp!=NULL){
      nc2=xmlNewNode(ns_ows, BAD_CAST tmp->name);
      nc3=xmlNewText(BAD_CAST _ss(tmp->value));
      xmlAddChild(nc2,nc3);  
      xmlAddChild(nc1,nc2);
    }

    if(e!=NULL)
      tmp=getMap(e->content,"Abstract");
    else
      tmp=getMap(m->content,"Abstract");

    if(tmp!=NULL){
      nc2=xmlNewNode(ns_ows, BAD_CAST tmp->name);
      nc3=xmlNewText(BAD_CAST _ss(tmp->value));
      xmlAddChild(nc2,nc3);  
      xmlAddChild(nc1,nc2);
      xmlAddChild(nc,nc1);
    }
  }else{
    xmlNewProp(nc1,BAD_CAST "id",BAD_CAST (e!=NULL?e->name:m->name));
  }
  
  // IO type nested outputs
  if(m->child!=NULL){	  
    maps* curs=m->child;
    elements* ecurs=getElements(e,(e!=NULL?e->name:m->name));
    ecurs=ecurs->child;
    while(curs!=NULL/* && ecurs!=NULL*/){
      ecurs=getElements(ecurs,(curs->name));
      map* inRequest=getMap(curs->content,"inRequest");
      if(inRequest!=NULL && strncasecmp(inRequest->value,"true",4)==0)
	printIOType(doc,nc1,ns_wps,ns_ows,ns_xlink,ecurs,curs,type,vid);
      curs=curs->next;
      ecurs=getElements(e,(e!=NULL?e->name:m->name));
      ecurs=ecurs->child;
    }
  }
  else{	  
    map *tmpMap=getMap(m->content,"Reference");
    if(tmpMap==NULL){		
      nc2=xmlNewNode(ns_wps, BAD_CAST "Data");
      if(e!=NULL && e->format!=NULL){		  
		  if (strncasecmp(e->format, "LiteralOutput", strlen(e->format)) == 0)		  			  
			  nc3 = xmlNewNode(ns_wps, BAD_CAST "LiteralData");		  
	else
	  if(strncasecmp(e->format,"ComplexOutput",strlen(e->format))==0)
	    nc3=xmlNewNode(ns_wps, BAD_CAST "ComplexData");
	  else if(strncasecmp(e->format,"BoundingBoxOutput",strlen(e->format))==0)
	    nc3=xmlNewNode(ns_wps, BAD_CAST "BoundingBoxData");
	  else
	    nc3=xmlNewNode(ns_wps, BAD_CAST e->format);		  
      }
      else {		  
	map* tmpV=getMapFromMaps(m,"format","value");	
	if(tmpV!=NULL)
	  nc3=xmlNewNode(ns_wps, BAD_CAST tmpV->value);
	else
	  nc3=xmlNewNode(ns_wps, BAD_CAST "LiteralData");
      } 
      tmp=m->content;	  

      while(tmp!=NULL){
	if(strcasecmp(tmp->name,"mimeType")==0 ||
	   strcasecmp(tmp->name,"encoding")==0 ||
	   strcasecmp(tmp->name,"schema")==0 ||
	   strcasecmp(tmp->name,"datatype")==0 ||
	   strcasecmp(tmp->name,"uom")==0) {	
	  if(vid==0)
	    xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
	  else{
	    if(strcasecmp(tmp->name,"datatype")==0)
	      xmlNewProp(nc2,BAD_CAST "mimeType",BAD_CAST "text/plain");
	    else
	      if(strcasecmp(tmp->name,"uom")!=0)
		xmlNewProp(nc2,BAD_CAST tmp->name,BAD_CAST tmp->value);
	  }
	}

	if(vid==0)
	  xmlAddChild(nc2,nc3);
	tmp=tmp->next;	
      }
      if(e!=NULL && e->format!=NULL && strcasecmp(e->format,"BoundingBoxData")==0) {
	map* bb=getMap(m->content,"value");
	if(bb!=NULL) {
	  map* tmpRes=parseBoundingBox(bb->value);
	  printBoundingBox(ns_ows,nc3,tmpRes);
	  freeMap(&tmpRes);
	  free(tmpRes);
	}
      }
      else {		  
		  //if (e != NULL) {
		  if (e != NULL && e->defaults != NULL) { // knut: add extra NULL pointer check in case user omits <Default> block in config file			  
			  tmp = getMap(e->defaults->content, "mimeType");
		  }
	else
	  tmp=NULL;	
	map* tmp1=getMap(m->content,"encoding");
	map* tmp2=getMap(m->content,"mimeType");
	map* tmp3=getMap(m->content,"value");
	int hasValue=1;
	if(tmp3==NULL){
	  tmp3=createMap("value","");
	  hasValue=-1;
	}
	
	if( ( tmp1 != NULL && strncmp(tmp1->value,"base64",6) == 0 )     // if encoding is base64
	    || 	                                                       // or if 
	    ( tmp2 != NULL && ( strstr(tmp2->value,"text") == NULL       //  mime type is not text 
				&&                                       //  nor 
				strstr(tmp2->value,"xml") == NULL        //  xml
				&&                                       // nor
				strstr(tmp2->value,"javascript") == NULL // javascript
				&&
				strstr(tmp2->value,"json") == NULL
				&&
				strstr(tmp2->value,"ecmascript") == NULL
				&&
				// include for backwards compatibility,
				// although correct mime type is ...kml+xml:
				strstr(tmp2->value,"google-earth.kml") == NULL							)
	      )
	    ) { 	                                                 // then	
	  map* rs=getMap(m->content,"size");                       // obtain size
	  bool isSized=true;
	  if(rs==NULL){
	    char tmp1[1024];
	    sprintf(tmp1,"%ld",strlen(tmp3->value));
	    rs=createMap("size",tmp1);
	    isSized=false;
	  }
	  
	  xmlAddChild((vid==0?nc3:nc2),xmlNewText(BAD_CAST base64(tmp3->value, atoi(rs->value))));  // base 64 encode in XML
	  
	  if(tmp1==NULL || (tmp1!=NULL && strncmp(tmp1->value,"base64",6)!=0)) {
	    xmlAttrPtr ap = xmlHasProp((vid==0?nc3:nc2), BAD_CAST "encoding");
	    if (ap != NULL) {
	      xmlRemoveProp(ap);
	    }			
	    xmlNewProp((vid==0?nc3:nc2),BAD_CAST "encoding",BAD_CAST "base64");
	  }
	  
	  if(!isSized){
	    freeMap(&rs);
	    free(rs);
	  }
	}
	else if (tmp2!=NULL) {                                 // else (text-based format) 
	  if(strstr(tmp2->value, "javascript") != NULL ||      //    if javascript put code in CDATA block 
	     strstr(tmp2->value, "json") != NULL ||            //    (will not be parsed by XML reader) 
	     strstr(tmp2->value, "ecmascript") != NULL
	     ) {
	    xmlAddChild((vid==0?nc3:nc2),xmlNewCDataBlock(doc,BAD_CAST tmp3->value,strlen(tmp3->value)));
	  }   
	  else {                                                     // else
	    if (strstr(tmp2->value, "xml") != NULL ||                 // if XML-based format
		// include for backwards compatibility,
		// although correct mime type is ...kml+xml:		   
		strstr(tmp2->value, "google-earth.kml") != NULL
		) { 
	      
	      int li=zooXmlAddDoc(tmp3->value);
	      xmlDocPtr doc = iDocs[li];
	      xmlNodePtr ir = xmlDocGetRootElement(doc);
	      xmlAddChild((vid==0?nc3:nc2),ir);
	    }
	    else                                                     // else	
	      xmlAddChild((vid==0?nc3:nc2),xmlNewText(BAD_CAST tmp3->value));    //   add text node
	  }
	  xmlAddChild(nc2,nc3);
	}
	else {
	  xmlAddChild((vid==0?nc3:nc2),xmlNewText(BAD_CAST tmp3->value));
	}
	
	if(hasValue<0) {
	  freeMap(&tmp3);
	  free(tmp3);
	}
      }
    }
    else { // Reference
      tmpMap=getMap(m->content,"Reference");
      nc3=nc2=xmlNewNode(ns_wps, BAD_CAST "Reference");
      /* Special case to avoid failing to validate against the WPS 2.0 schema */
      if(strcasecmp(type,"Output")==0 && vid==0)
	xmlNewProp(nc3,BAD_CAST "href",BAD_CAST tmpMap->value);
      else
	xmlNewNsProp(nc3,ns_xlink,BAD_CAST "href",BAD_CAST tmpMap->value);
      
      tmp=m->content;
      while(tmp!=NULL) {
	if(strcasecmp(tmp->name,"mimeType")==0 ||
	   strcasecmp(tmp->name,"encoding")==0 ||
	   strcasecmp(tmp->name,"schema")==0 ||
	   strcasecmp(tmp->name,"datatype")==0 ||
	   strcasecmp(tmp->name,"uom")==0){
	  
	  if(strcasecmp(tmp->name,"datatype")==0)
	    xmlNewProp(nc3,BAD_CAST "mimeType",BAD_CAST "text/plain");
	  else
	    xmlNewProp(nc3,BAD_CAST tmp->name,BAD_CAST tmp->value);
	}
	tmp=tmp->next;
	xmlAddChild(nc2,nc3);
      }
    }
    xmlAddChild(nc1,nc2);
  }
  xmlAddChild(nc,nc1);
}

/**
 * Create XML node with basic ows metadata information (Identifier,Title,Abstract)
 *
 * @param root the root XML node to add the description
 * @param ns_ows the ows XML namespace
 * @param identifier the identifier to use
 * @param amap the map containing the ows metadata information 
 */
void printDescription(xmlNodePtr root,xmlNsPtr ns_ows,const char* identifier,map* amap,int vid=0){
  xmlNodePtr nc2;
  if(vid==0){
    nc2 = xmlNewNode(ns_ows, BAD_CAST "Identifier");
    xmlAddChild(nc2,xmlNewText(BAD_CAST identifier));
    xmlAddChild(root,nc2);
  }
  map* tmp=amap;
  const char *tmp2[2];
  tmp2[0]="Title";
  tmp2[1]="Abstract";
  int j=0;
  for(j=0;j<2;j++){
    map* tmp1=getMap(tmp,tmp2[j]);
    if(tmp1!=NULL){
      nc2 = xmlNewNode(ns_ows, BAD_CAST tmp2[j]);
      xmlAddChild(nc2,xmlNewText(BAD_CAST _ss(tmp1->value)));
      xmlAddChild(root,nc2);
    }
  }
  if(vid==1){
    nc2 = xmlNewNode(ns_ows, BAD_CAST "Identifier");
    xmlAddChild(nc2,xmlNewText(BAD_CAST identifier));
    xmlAddChild(root,nc2);
  }
}

/**
 * Print an OWS ExceptionReport Document and HTTP headers (when required) 
 * depending on the code.
 * Set hasPrinted value to true in the [lenv] section.
 * 
 * @param m the maps containing the settings of the main.cfg file
 * @param s the map containing the text,code,locator keys (or a map array of the same keys)
 */
void printExceptionReportResponse(maps* m,map* s){
  if(getMapFromMaps(m,"lenv","hasPrinted")!=NULL)
    return;
  int buffersize;
  xmlDocPtr doc;
  xmlChar *xmlbuff;
  xmlNodePtr n;

  zooXmlCleanupNs();
  doc = xmlNewDoc(BAD_CAST "1.0");
  maps* tmpMap=getMaps(m,"main");
  char *encoding=getEncoding(tmpMap);
  const char *exceptionCode;
  
  map* tmp=getMap(s,"code");
  if(tmp!=NULL){
    if(strcmp(tmp->value,"OperationNotSupported")==0 ||
       strcmp(tmp->value,"NoApplicableCode")==0)
      exceptionCode="501 Not Implemented";
    else
      if(strcmp(tmp->value,"MissingParameterValue")==0 ||
	 strcmp(tmp->value,"InvalidUpdateSequence")==0 ||
	 strcmp(tmp->value,"OptionNotSupported")==0 ||
	 strcmp(tmp->value,"VersionNegotiationFailed")==0 ||
	 strcmp(tmp->value,"InvalidParameterValue")==0)
	exceptionCode="400 Bad request";
      else
	exceptionCode="501 Internal Server Error";
  }
  else
    exceptionCode="501 Internal Server Error";
  tmp=getMapFromMaps(m,"lenv","status_code");
  if(tmp!=NULL)
    exceptionCode=tmp->value;
  if(m!=NULL){
    map *tmpSid=getMapFromMaps(m,"lenv","sid");
    if(tmpSid!=NULL){
      if( getpid()==atoi(tmpSid->value) ){
	printHeaders(m);
	printf("Content-Type: text/xml; charset=%s\r\nStatus: %s\r\n\r\n",encoding,exceptionCode);
      }
    }
    else{
      printHeaders(m);
      printf("Content-Type: text/xml; charset=%s\r\nStatus: %s\r\n\r\n",encoding,exceptionCode);
    }
  }else{
    printf("Content-Type: text/xml; charset=%s\r\nStatus: %s\r\n\r\n",encoding,exceptionCode);
  }
  n=createExceptionReportNode(m,s,1);
  xmlDocSetRootElement(doc, n);
  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  printf("%s",xmlbuff);
  fflush(stdout);
  xmlFreeDoc(doc);
  xmlFree(xmlbuff);
  xmlCleanupParser();
  zooXmlCleanupNs();
  if(m!=NULL)
    setMapInMaps(m,"lenv","hasPrinted","true");
}

/**
 * Create an OWS ExceptionReport Node.
 * 
 * @param m the conf maps
 * @param s the map containing the text,code,locator keys
 * @param use_ns (0/1) choose if you want to generate an ExceptionReport or 
 *  ows:ExceptionReport node respectively
 * @return the ExceptionReport/ows:ExceptionReport node
 */
xmlNodePtr createExceptionReportNode(maps* m,map* s,int use_ns){
  
  xmlNsPtr ns,ns_xsi;
  xmlNodePtr n,nc,nc1;

  int nsid=zooXmlAddNs(NULL,"http://www.opengis.net/ows","ows");
  ns=usedNs[nsid];
  if(use_ns==0){
    ns=NULL;
  }
  n = xmlNewNode(ns, BAD_CAST "ExceptionReport");
  map* version=getMapFromMaps(m,"main","rversion");
  int vid=-1;
  if(version!=NULL)
    vid=getVersionId(version->value);
  if(vid<0)
    vid=0;
  if(use_ns==1){
    xmlNewNs(n,BAD_CAST schemas[vid][1],BAD_CAST"ows");
    int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
    ns_xsi=usedNs[xsiId];
    char tmp[1024];
    sprintf(tmp,"%s %s",schemas[vid][1],schemas[vid][5]);
    xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST tmp);
  }


  addLangAttr(n,m);
  xmlNewProp(n,BAD_CAST "version",BAD_CAST schemas[vid][6]);
  
  int length=1;
  int cnt=0;
  map* len=getMap(s,"length");
  if(len!=NULL)
    length=atoi(len->value);
  for(cnt=0;cnt<length;cnt++){
    nc = xmlNewNode(ns, BAD_CAST "Exception");
    
    map* tmp=getMapArray(s,"code",cnt);
    if(tmp==NULL)
      tmp=getMap(s,"code");
    if(tmp!=NULL)
      xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST tmp->value);
    else
      xmlNewProp(nc,BAD_CAST "exceptionCode",BAD_CAST "NoApplicableCode");
    
    tmp=getMapArray(s,"locator",cnt);
    if(tmp==NULL)
      tmp=getMap(s,"locator");
    if(tmp!=NULL && strcasecmp(tmp->value,"NULL")!=0)
      xmlNewProp(nc,BAD_CAST "locator",BAD_CAST tmp->value);

    tmp=getMapArray(s,"text",cnt);
    if(tmp==NULL)
      tmp=getMapArray(s,"message",cnt);
    nc1 = xmlNewNode(ns, BAD_CAST "ExceptionText");
    if(tmp!=NULL){
      xmlNodePtr txt=xmlNewText(BAD_CAST tmp->value);
      xmlAddChild(nc1,txt);
      if(cnt==0)
	setMapInMaps(m,"lenv","message",tmp->value);
    }
    else{
      xmlNodeSetContent(nc1, BAD_CAST _("No debug message available"));
    }
    xmlAddChild(nc,nc1);
    xmlAddChild(n,nc);
  }
  return n;
}

/**
 * Print an OWS ExceptionReport.
 * 
 * @param m the conf maps
 * @param message the error message 
 * @param errorcode the error code
 * @param locator the potential locator
 */
int errorException(maps *m, const char *message, const char *errorcode, const char *locator) 
{
  map* errormap = createMap("text", message);
  addToMap(errormap,"code", errorcode);
  if(locator!=NULL)
    addToMap(errormap,"locator", locator);
  else
    addToMap(errormap,"locator", "NULL");
  printExceptionReportResponse(m,errormap);
  freeMap(&errormap);
  free(errormap);
  return -1;
}

/**
 * Generate the output response (RawDataOutput or ResponseDocument)
 *
 * @param s the service structure containing the metadata information
 * @param request_inputs the inputs provided to the service for execution
 * @param request_outputs the outputs updated by the service execution
 * @param request_inputs1 the map containing the HTTP request
 * @param cpid the process identifier attached to a service execution
 * @param m the conf maps containing the main.cfg settings
 * @param res the value returned by the service execution
 */
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
  map* version=getMapFromMaps(m,"main","rversion");
  int vid=getVersionId(version->value);
  maps* tmpSess=getMaps(m,"senv");
  if(tmpSess!=NULL){
    map *_tmp=getMapFromMaps(m,"lenv","cookie");
    maps *tmps=getMaps(m,"senv");
    char* sessId=NULL;
    if(_tmp!=NULL){
      printf("Set-Cookie: %s; HttpOnly\r\n",_tmp->value);
      map *_tmp1=getMapFromMaps(m,"senv","ecookie_length");
      if(_tmp1!=NULL){
	int len=atoi(_tmp1->value);
	int cnt=0;
	for(cnt=0;cnt<len;cnt++){
	  map* _tmp2=getMapArray(tmps->content,"ecookie",cnt);
	  if(_tmp2!=NULL)
	    printf("Set-Cookie: %s; HttpOnly\r\n",_tmp2->value);
	}
      }
      printf("P3P: CP=\"IDC DSP COR ADM DEVi TAIi PSA PSD IVAi IVDi CONi HIS OUR IND CNT\"\r\n");
      char session_file_path[100];
      char *tmp1=strtok(_tmp->value,";");
      if(tmp1!=NULL)
	sprintf(session_file_path,"%s",strstr(tmp1,"=")+1);
      else
	sprintf(session_file_path,"%s",strstr(_tmp->value,"=")+1);
      sessId=zStrdup(session_file_path);
    }else{
      maps* t=getMaps(m,"senv");
      map*p=t->content;
      while(p!=NULL){
	if(strstr(p->name,"ID")!=NULL){
	  sessId=zStrdup(p->value);
	  break;
	}
	p=p->next;
      }
    }
    char session_file_path[1024];
    map *tmpPath=getMapFromMaps(m,"main","sessPath");
    if(tmpPath==NULL)
      tmpPath=getMapFromMaps(m,"main","tmpPath");
    sprintf(session_file_path,"%s/sess_%s.cfg",tmpPath->value,sessId);
    FILE* teste=fopen(session_file_path,"w");
    if(teste==NULL){
      char tmpMsg[1024];
      sprintf(tmpMsg,_("Unable to create the file \"%s\" for storing the session maps."),session_file_path);
      errorException(m,tmpMsg,"InternalError",NULL);
      return;
    }
    else{
      fclose(teste);
      dumpMapsToFile(tmpSess,session_file_path,1);
    }
  }
  if(res==SERVICE_FAILED){
    map *lenv;
    lenv=getMapFromMaps(m,"lenv","message");
    char *tmp0;
    if(lenv!=NULL){
      tmp0=(char*)malloc((strlen(lenv->value)+strlen(_("Unable to run the Service. The message returned back by the Service was the following: "))+1)*sizeof(char));
      sprintf(tmp0,_("Unable to run the Service. The message returned back by the Service was the following: %s"),lenv->value);
    }
    else{
      tmp0=(char*)malloc((strlen(_("Unable to run the Service. No more information was returned back by the Service."))+1)*sizeof(char));
      sprintf(tmp0,"%s",_("Unable to run the Service. No more information was returned back by the Service."));
    }
    errorException(m,tmp0,"InternalError",NULL);
    free(tmp0);
    return;
  }

  if(res==SERVICE_ACCEPTED && vid==1){
    map* statusInfo=createMap("Status","Accepted");
    map *usid=getMapFromMaps(m,"lenv","usid");
    addToMap(statusInfo,"JobID",usid->value);
    printStatusInfo(m,statusInfo,(char*)"Execute");
    freeMap(&statusInfo);
    free(statusInfo);
    return;
  }

  if(res!=SERVICE_SUCCEEDED){	  
    printProcessResponse(m,request_inputs1,cpid,
                         s, s->name,res,  // replace serviceProvider with serviceName in stored response file name
                         request_inputs,
                         request_outputs);
    return;
  }
      
  map *tmp1=getMapFromMaps(m,"main","tmpPath");
  if(asRaw==0){
#ifdef DEBUG
    fprintf(stderr,"REQUEST_OUTPUTS FINAL\n");
    dumpMaps(request_outputs);
#endif
    maps* tmpI=request_outputs;
    maps* stmpI=NULL;
    map* usid=getMapFromMaps(m,"lenv","usid");
    int itn=0;
    int error=-1;
  NESTED0:
    while(tmpI!=NULL){
      if(tmpI->child!=NULL){
	stmpI=tmpI;
	tmpI=tmpI->child;
      }
#ifdef USE_MS
      map* testMap=getMap(tmpI->content,"useMapserver");	
#endif
      map *gfile=getMap(tmpI->content,"generated_file");
      char *file_name=NULL;	  
      if(gfile!=NULL){
	gfile=getMap(tmpI->content,"expected_generated_file");
	if(gfile==NULL){
	  gfile=getMap(tmpI->content,"generated_file");
	}
	readGeneratedFile(m,tmpI->content,gfile->value);
	file_name=zStrdup((gfile->value)+strlen(tmp1->value));
      }	  
      toto=getMap(tmpI->content,"asReference");
#ifdef USE_MS
      map* geodatatype=getMap(tmpI->content,"geodatatype");
      if(toto!=NULL && strcasecmp(toto->value,"true")==0 &&
	 (testMap==NULL ||
	  strncasecmp(testMap->value,"true",4)!=0 ||
	  (geodatatype!=NULL && strcasecmp(geodatatype->value,"other")==0) ) ) 
#else
	if(toto!=NULL && strcasecmp(toto->value,"true")==0)
#endif
	  {		
	    elements* in=getElements(s->outputs,tmpI->name);
	    if(in==NULL && s->outputs->child!=NULL){
	      in=getElements(s->outputs->child,tmpI->name);
	    }
	    char *format=NULL;
	    if(in!=NULL && in->format!=NULL){
	      format=in->format;
	    }else
	      format=(char*)"LiteralData";
	    if(format!=NULL && strcasecmp(format,"BoundingBoxData")==0){
	      addToMap(tmpI->content,"extension","xml");
	      addToMap(tmpI->content,"mimeType","text/xml");
	      addToMap(tmpI->content,"encoding","UTF-8");
	      addToMap(tmpI->content,"schema","http://schemas.opengis.net/ows/1.1.0/owsCommon.xsd");
	    }		
	    char *file_path=NULL;
	    if(gfile==NULL) {
	      map *ext=getMap(tmpI->content,"extension");
	      char file_ext[32];
	    
	      if( ext != NULL && ext->value != NULL) {
		strncpy(file_ext, ext->value, 32);
	      }
	      else {
		// Obtain default file extension (see mimetypes.h).	      
		// If the MIME type is not recognized, txt is used as the default extension
		map* mtype=getMap(tmpI->content,"mimeType");
		getFileExtension(mtype != NULL ? mtype->value : NULL, file_ext, 32);
	      }
	      if(file_name!=NULL)
		free(file_name);
	      file_name=(char*)malloc((strlen(s->name)+strlen(usid->value)+strlen(file_ext)+strlen(tmpI->name)+45)*sizeof(char));
	      sprintf(file_name,"ZOO_DATA_%s_%s_%s_%d.%s",s->name,tmpI->name,usid->value,itn,file_ext);
	      itn++;
	      file_path=(char*)malloc((strlen(tmp1->value)+strlen(file_name)+2)*sizeof(char));
	      sprintf(file_path,"%s/%s",tmp1->value,file_name);

	      FILE *ofile=fopen(file_path,"wb");
	      if(ofile==NULL){
		char tmpMsg[1024];
		sprintf(tmpMsg,_("Unable to create the file \"%s\" for storing the %s final result."),file_name,tmpI->name);
		errorException(m,tmpMsg,"InternalError",NULL);
		free(file_name);
		free(file_path);
		return;
	      }

	      toto=getMap(tmpI->content,"value");
	      if(toto==NULL){
		char tmpMsg[1024];
		sprintf(tmpMsg,_("No value found for the requested output %s."),tmpI->name);
		errorException(m,tmpMsg,"InternalError",NULL);
		fclose(ofile);
		free(file_name);
		free(file_path);
		return;
	      }
	      if(strcasecmp(format,"BoundingBoxData")!=0){
		map* size=getMap(tmpI->content,"size");
		if(size!=NULL && toto!=NULL)
		  fwrite(toto->value,1,(atoi(size->value))*sizeof(char),ofile);
		else
		  if(toto!=NULL && toto->value!=NULL)
		    fwrite(toto->value,1,strlen(toto->value)*sizeof(char),ofile);
	      }else{
		printBoundingBoxDocument(m,tmpI,ofile);
	      }
	      fclose(ofile);
	    }

	    map *tmp2=getMapFromMaps(m,"main","tmpUrl");
	    map *tmp3=getMapFromMaps(m,"main","serverAddress");
	    char *file_url=NULL;
	    if(strncasecmp(tmp2->value,"http://",7)==0 ||
	       strncasecmp(tmp2->value,"https://",8)==0){
	      file_url=(char*)malloc((strlen(tmp2->value)+strlen(file_name)+2)*sizeof(char));
	      sprintf(file_url,"%s/%s",tmp2->value,file_name);
	    }else{
	      file_url=(char*)malloc((strlen(tmp3->value)+strlen(tmp2->value)+strlen(file_name)+3)*sizeof(char));
	      sprintf(file_url,"%s/%s/%s",tmp3->value,tmp2->value,file_name);
	    }
	    addToMap(tmpI->content,"Reference",file_url);
	    if(file_name!=NULL)
	      free(file_name);
	    if(file_url!=NULL)
	      free(file_url);
	    file_name=NULL;
	  }
#ifdef USE_MS
	else{
	  if(testMap!=NULL){
	    map* nbFeatures;
	    setMapInMaps(m,"lenv","state","out");
	    setReferenceUrl(m,tmpI);
	    nbFeatures=getMap(tmpI->content,"nb_features");
	    geodatatype=getMap(tmpI->content,"geodatatype");
	    if((nbFeatures!=NULL && atoi(nbFeatures->value)==0) ||
	       (geodatatype!=NULL && strcasecmp(geodatatype->value,"other")==0)){
	      error=1;
	      res=SERVICE_FAILED;
	    }
	  }
	}
#endif	
      if(file_name!=NULL){
	free(file_name);
	file_name=NULL;
      }
      tmpI=tmpI->next;
    }
    if(stmpI!=NULL){
      tmpI=stmpI->next;
      stmpI=NULL;
      if(tmpI!=NULL)
	goto NESTED0;
    }    
#ifdef DEBUG
    fprintf(stderr,"SERVICE : %s\n",s->name);
    dumpMaps(m);
#endif	
    if(error<0)
      printProcessResponse(m,request_inputs1,cpid,
			   s, s->name,res,  // replace serviceProvider with serviceName in stored response file name
			   request_inputs,
			   request_outputs);
    else{
      maps* tmpMaps=getMaps(m,"lenv");
#ifdef USE_CALLBACK
      invokeCallback(m,NULL,NULL,7,0);
#endif
      printExceptionReportResponse(m,tmpMaps->content);
    }
  }
  else{
    /**
     * We get the requested output or fallback to the first one if the 
     * requested one is not present in the resulting outputs maps.
     */
    maps* tmpI=NULL;
    map* tmpIV=getMap(request_inputs1,"RawDataOutput");
    if(tmpIV!=NULL){
      tmpI=getMaps(request_outputs,tmpIV->value);	  
    }
    if(tmpI==NULL)
      tmpI=request_outputs;

    elements* e=getElements(s->outputs,tmpI->name);
    if(e!=NULL && strcasecmp(e->format,"BoundingBoxData")==0){
      printBoundingBoxDocument(m,tmpI,NULL);
    }else{
      map *gfile=getMap(tmpI->content,"generated_file");
      if(gfile!=NULL){
	gfile=getMap(tmpI->content,"expected_generated_file");	
	if(gfile==NULL){
	  gfile=getMap(tmpI->content,"generated_file");
	}
	readGeneratedFile(m,tmpI->content,gfile->value);
      }
      toto=getMap(tmpI->content,"value");	 
      if(toto==NULL){
	char tmpMsg[1024];
	sprintf(tmpMsg,_("Wrong RawDataOutput parameter: unable to fetch any result for the given parameter name: \"%s\"."),tmpI->name);
	errorException(m,tmpMsg,"InvalidParameterValue","RawDataOutput");
	return;
      }	  
      map* fname=getMapFromMaps(tmpI,tmpI->name,"filename");	  	  
      if(fname!=NULL)
	printf("Content-Disposition: attachment; filename=\"%s\"\r\n",fname->value);
      map* rs=getMapFromMaps(tmpI,tmpI->name,"size");
      if(rs!=NULL)
	printf("Content-Length: %s\r\n",rs->value);
      printHeaders(m);
      char mime[1024];
      map* mi=getMap(tmpI->content,"mimeType");
#ifdef DEBUG
      fprintf(stderr,"SERVICE OUTPUTS\n");
      dumpMaps(request_outputs);
      fprintf(stderr,"SERVICE OUTPUTS\n");
#endif
      map* en=getMap(tmpI->content,"encoding");
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
      printf("%s",mime);
      if(rs!=NULL)
	fwrite(toto->value,1,atoi(rs->value),stdout);
      else
	fwrite(toto->value,1,strlen(toto->value),stdout);
#ifdef DEBUG
      dumpMap(toto);
#endif
    }
  }
}

/**
 * Create required XML nodes for boundingbox and update the current XML node
 *
 * @param ns_ows the ows XML namespace
 * @param n the XML node to update
 * @param boundingbox the map containing the boundingbox definition
 */
void printBoundingBox(xmlNsPtr ns_ows,xmlNodePtr n,map* boundingbox){

  xmlNodePtr lw=NULL,uc=NULL;

  map* tmp=getMap(boundingbox,"value");

  tmp=getMap(boundingbox,"lowerCorner");
  if(tmp!=NULL){
    lw=xmlNewNode(ns_ows,BAD_CAST "LowerCorner");
    xmlAddChild(lw,xmlNewText(BAD_CAST tmp->value));
  }

  tmp=getMap(boundingbox,"upperCorner");
  if(tmp!=NULL){
    uc=xmlNewNode(ns_ows,BAD_CAST "UpperCorner");
    xmlAddChild(uc,xmlNewText(BAD_CAST tmp->value));
  }

  tmp=getMap(boundingbox,"crs");
  if(tmp!=NULL)
    xmlNewProp(n,BAD_CAST "crs",BAD_CAST tmp->value);

  tmp=getMap(boundingbox,"dimensions");
  if(tmp!=NULL)
    xmlNewProp(n,BAD_CAST "dimensions",BAD_CAST tmp->value);

  xmlAddChild(n,lw);
  xmlAddChild(n,uc);

}

/**
 * Parse a BoundingBox string
 *
 * [OGC 06-121r3](http://portal.opengeospatial.org/files/?artifact_id=20040):
 *  10.2 Bounding box
 * 
 *
 * Value is provided as : lowerCorner,upperCorner,crs,dimension
 * Exemple : 189000,834000,285000,962000,urn:ogc:def:crs:OGC:1.3:CRS84
 *
 * A map to store boundingbox information should contain:
 *  - lowerCorner : double,double (minimum within this bounding box)
 *  - upperCorner : double,double (maximum within this bounding box)
 *  - crs : URI (Reference to definition of the CRS)
 *  - dimensions : int 
 * 
 * Note : support only 2D bounding box.
 *
 * @param value the char* containing the KVP bouding box
 * @return a map containing all the bounding box keys
 */
map* parseBoundingBox(const char* value){
  map *res=NULL;
  if(value!=NULL){
    char *cv,*cvp;
    cv=strtok_r((char*) value,",",&cvp);
    int cnt=0;
    int icnt=0;
    char *currentValue=NULL;
    while(cv){
      if(cnt<2)
	if(currentValue!=NULL){
	  char *finalValue=(char*)malloc((strlen(currentValue)+strlen(cv)+1)*sizeof(char));
	  sprintf(finalValue,"%s%s",currentValue,cv);
	  switch(cnt){
	  case 0:
	    res=createMap("lowerCorner",finalValue);
	    break;
	  case 1:
	    addToMap(res,"upperCorner",finalValue);
	    icnt=-1;
	    break;
	  }
	  cnt++;
	  free(currentValue);
	  currentValue=NULL;
	  free(finalValue);
	}
	else{
	  currentValue=(char*)malloc((strlen(cv)+2)*sizeof(char));
	  sprintf(currentValue,"%s ",cv);
	}
      else
	if(cnt==2){
	  addToMap(res,"crs",cv);
	  cnt++;
	}
	else
	  addToMap(res,"dimensions",cv);
      icnt++;
      cv=strtok_r(NULL,",",&cvp);
    }
  }
  return res;
}

/**
 * Print an ows:BoundingBox XML document
 *
 * @param m the maps containing the settings of the main.cfg file
 * @param boundingbox the maps containing the boundingbox definition
 * @param file the file to print the BoundingBox (if NULL then print on stdout)
 * @see parseBoundingBox, printBoundingBox
 */
void printBoundingBoxDocument(maps* m,maps* boundingbox,FILE* file){
  if(file==NULL)
    rewind(stdout);
  xmlNodePtr n;
  xmlDocPtr doc;
  xmlNsPtr ns_ows,ns_xsi;
  xmlChar *xmlbuff;
  int buffersize;
  char *encoding=getEncoding(m);
  map *tmp;
  if(file==NULL){
    int pid=0;
    tmp=getMapFromMaps(m,"lenv","sid");
    if(tmp!=NULL)
      pid=atoi(tmp->value);
    if(pid==getpid()){
      printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
    }
    fflush(stdout);
  }

  doc = xmlNewDoc(BAD_CAST "1.0");
  int owsId=zooXmlAddNs(NULL,"http://www.opengis.net/ows/1.1","ows");
  ns_ows=usedNs[owsId];
  n = xmlNewNode(ns_ows, BAD_CAST "BoundingBox");
  xmlNewNs(n,BAD_CAST "http://www.opengis.net/ows/1.1",BAD_CAST "ows");
  int xsiId=zooXmlAddNs(n,"http://www.w3.org/2001/XMLSchema-instance","xsi");
  ns_xsi=usedNs[xsiId];
  xmlNewNsProp(n,ns_xsi,BAD_CAST "schemaLocation",BAD_CAST "http://www.opengis.net/ows/1.1 http://schemas.opengis.net/ows/1.1.0/owsCommon.xsd");
  map *tmp1=getMap(boundingbox->content,"value");
  tmp=parseBoundingBox(tmp1->value);
  printBoundingBox(ns_ows,n,tmp);
  xmlDocSetRootElement(doc, n);

  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  if(file==NULL)
    printf("%s",xmlbuff);
  else{
    fprintf(file,"%s",xmlbuff);
  }

  if(tmp!=NULL){
    freeMap(&tmp);
    free(tmp);
  }
  xmlFree(xmlbuff);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  zooXmlCleanupNs();
  
}

/**
 * Print a StatusInfo XML document.
 * a statusInfo map should contain the following keys:
 *  * JobID corresponding to usid key from the lenv section
 *  * Status the current state (Succeeded,Failed,Accepted,Running)
 *  * PercentCompleted (optional) the percent completed
 *  * Message (optional) any messages the service may wish to share
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param statusInfo the map containing the statusInfo definition
 * @param req the WPS requests (GetResult, GetStatus or Dismiss)
 */
void printStatusInfo(maps* conf,map* statusInfo,char* req){
  rewind(stdout);
  xmlNodePtr n,n1;
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlChar *xmlbuff;
  int buffersize;
  char *encoding=getEncoding(conf);
  map *tmp;
  int pid=0;
  printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);

  map* version=getMapFromMaps(conf,"main","rversion");
  int vid=getVersionId(version->value);

  doc = xmlNewDoc(BAD_CAST "1.0");
  n1=printWPSHeader(doc,conf,req,"StatusInfo",version->value,1);

  map* val=getMap(statusInfo,"JobID");
  int wpsId=zooXmlAddNs(NULL,schemas[vid][2],"wps");
  ns=usedNs[wpsId];
  n = xmlNewNode(ns, BAD_CAST "JobID");
  xmlAddChild(n,xmlNewText(BAD_CAST val->value));

  xmlAddChild(n1,n);

  val=getMap(statusInfo,"Status");
  n = xmlNewNode(ns, BAD_CAST "Status");
  xmlAddChild(n,xmlNewText(BAD_CAST val->value));

  xmlAddChild(n1,n);

  if(strncasecmp(val->value,"Failed",6)!=0 &&
     strncasecmp(val->value,"Succeeded",9)!=0){
    val=getMap(statusInfo,"PercentCompleted");
    if(val!=NULL){
      n = xmlNewNode(ns, BAD_CAST "PercentCompleted");
      xmlAddChild(n,xmlNewText(BAD_CAST val->value));
      xmlAddChild(n1,n);
    }

    val=getMap(statusInfo,"Message");
    if(val!=NULL){    
      xmlAddChild(n1,xmlNewComment(BAD_CAST val->value));
    }
  }
  xmlDocSetRootElement(doc, n1);

  xmlDocDumpFormatMemoryEnc(doc, &xmlbuff, &buffersize, encoding, 1);
  printf("%s",xmlbuff);

  xmlFree(xmlbuff);
  xmlFreeDoc(doc);
  xmlCleanupParser();
  zooXmlCleanupNs();
  
}

