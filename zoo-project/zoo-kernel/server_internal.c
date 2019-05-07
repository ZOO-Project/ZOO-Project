/*
 * Author : Gérald Fenoy
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

#include "server_internal.h"
#include "service_internal.h"
#include "response_print.h"
//#include "service_callback.h"
#include "mimetypes.h"
#ifndef WIN32
#include <dlfcn.h>
#include <uuid/uuid.h>
#else
#include <rpc.h>
#define ERROR_MSG_MAX_LENGTH 1024
#endif
#include <signal.h>

// #include <stdlib.h>
/*
 * Compare two file path strings to see if they refer to the same file.
 *
 * @param path1 the first file path
 * @param path2 the second file path
 * 
 * @return 0 if the files are identical
 */
#define PATHBUFSIZE 4096
int zoo_path_compare(char* path1, char* path2) {

  if (path1 == NULL || path2 == NULL) {
    return -1;
  }

  char realpath1[PATHBUFSIZE];
  char realpath2[PATHBUFSIZE];

#ifdef WIN32 
  int res1 = GetFullPathName(path1, PATHBUFSIZE, realpath1, NULL);
  int res2 = GetFullPathName(path2, PATHBUFSIZE, realpath2, NULL);

  if (res1 == 0 || res2 == 0) {
    return -1;
  }
  else {
    return strncasecmp(realpath1, realpath2, PATHBUFSIZE);
  }
#else
  char* ptr1 = realpath(path1, realpath1);
  char* ptr2 = realpath(path2, realpath2);

  if (ptr1 == NULL || ptr2 == NULL) {
    return -1;
  }
  else {
    return strncmp(realpath1, realpath2, PATHBUFSIZE);
  }
#endif
}

/**
 * Detect WPS version used (1.0.0 or 2.0.0).
 *
 * @param version number as char* (1.0.0 or 2.0.0)
 * @return 0 in case of version 1.0.0, 1 for 2.0.0, -1 in other case
 */
int getVersionId(const char* version){
  int schemaId=0;
  for(;schemaId<2;schemaId++){
    if(strncasecmp(version,schemas[schemaId][0],5)==0)
      return schemaId;
  }
  return -1;
}

/**
 * Generate a UUID.
 * ref: https://www.ietf.org/rfc/rfc4122.txt / 4.2 
 *
 * @return a new char* containing the UUID, make sure to free the returned 
 *  resource once used.
 */
char *get_uuid(){
  char *res=(char*)malloc(37*sizeof(char));
#ifdef WIN32
  UUID uuid;
  UuidCreate(&uuid);
  RPC_CSTR rest = NULL;
  UuidToString(&uuid,&rest);
#else
  uuid_t uuid;
  uuid_generate_time(uuid);
  char rest[128];
  uuid_unparse(uuid,rest);
#endif
  sprintf(res,"%s",rest);
#ifdef WIN32
  RpcStringFree(&rest);
#endif
  return res;
}

/**
 * Extract the service identifier from the full service identifier
 * ie: 
 *  - Full service name: OTB.BandMath
 *  - Service name: BandMath
 *
 * @param conf the maps containing the settings of the main.cfg file
 * @param conf_dir the full path to the ZOO-Kernel directory
 * @param identifier the full service name (potentialy including a prefix, ie:
 *  Prefix.MyService)
 * @param buffer the resulting service identifier (without any prefix)
 */
void parseIdentifier(maps* conf,char* conf_dir,char *identifier,char* buffer){
  setMapInMaps(conf,"lenv","oIdentifier",identifier);
  char *lid=zStrdup(identifier);
  char *saveptr1;
  char *tmps1=strtok_r(lid,".",&saveptr1);
  int level=0;
  char key[25];
  char levels[18];
  while(tmps1!=NULL){
    char *test=zStrdup(tmps1);
    char* tmps2=(char*)malloc((strlen(test)+2)*sizeof(char));
    sprintf(key,"sprefix_%d",level);
    sprintf(tmps2,"%s.",test);
    sprintf(levels,"%d",level);
    setMapInMaps(conf,"lenv","level",levels);
    setMapInMaps(conf,"lenv",key,tmps2);
    free(tmps2);
    free(test);
    level++;
    tmps1=strtok_r(NULL,".",&saveptr1);
  }
  int i=0;
  sprintf(buffer,"%s",conf_dir);
  for(i=0;i<level;i++){
    char *tmp0=zStrdup(buffer);
    sprintf(key,"sprefix_%d",i);
    map* tmp00=getMapFromMaps(conf,"lenv",key);
    if(tmp00!=NULL)
      sprintf(buffer,"%s/%s",tmp0,tmp00->value);
    free(tmp0);
    buffer[strlen(buffer)-1]=0;
    if(i+1<level){ 
      #ifdef IGNORE_METAPATH
        map* tmpMap = createMap("metapath", "");
      #else  
        map* tmpMap=getMapFromMaps(conf,"lenv","metapath");
      #endif	  
      if(tmpMap==NULL || strlen(tmpMap->value)==0){
	char *tmp01=zStrdup(tmp00->value);
	tmp01[strlen(tmp01)-1]=0;
	setMapInMaps(conf,"lenv","metapath",tmp01);
	free(tmp01);
	tmp01=NULL;
      }
      else{
	if(tmp00!=NULL && tmpMap!=NULL){
	  char *tmp00s=zStrdup(tmp00->value);
	  tmp00s[strlen(tmp00s)-1]=0;
	  char *value=(char*)malloc((strlen(tmp00s)+strlen(tmpMap->value)+2)*sizeof(char));
	  sprintf(value,"%s/%s",tmpMap->value,tmp00s);
	  setMapInMaps(conf,"lenv","metapath",value);
	  free(value);
	  free(tmp00s);
	  value=NULL;
	}
      }
    }else{
      char *tmp01=zStrdup(tmp00->value);
      tmp01[strlen(tmp01)-1]=0;
      setMapInMaps(conf,"lenv","Identifier",tmp01);
      free(tmp01);
    }
  }
  char *tmp0=zStrdup(buffer);
  sprintf(buffer,"%s.zcfg",tmp0);
  free(tmp0);
  free(lid);
}

/**
 * Converts a hex character to its integer value 
 *
 * @param ch the char to convert
 * @return the converted char 
 */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/**
 * Converts an integer value to its hec character 
 *
 * @param code the char to convert
 * @return the converted char 
 */
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/**
 * URLEncode an url
 *
 * @param str the url to encode
 * @return a url-encoded version of str
 * @warning be sure to free() the returned string after use
 */
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

/**
 * Decode an URLEncoded url
 *
 * @param str the URLEncoded url to decode
 * @return a url-decoded version of str
 * @warning be sure to free() the returned string after use
 */
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

/**
 * Verify if a given language is listed in the lang list defined in the [main] 
 * section of the main.cfg file.
 * 
 * @param conf the map containing the settings from the main.cfg file
 * @param str the specific language
 * @return 1 if the specific language is listed, -1 in other case.
 */
int isValidLang(maps* conf,const char *str){
  map *tmpMap=getMapFromMaps(conf,"main","language");
  char *tmp0=NULL,*tmp=NULL,*tmp1=NULL;
  if(tmpMap!=NULL)
    tmp0=zStrdup(tmpMap->value);
  tmpMap=getMapFromMaps(conf,"main","lang");
  if(tmpMap!=NULL)
    tmp=zStrdup(tmpMap->value);
  if(tmp0!=NULL && tmp!=NULL){
    tmp1=(char*)malloc((strlen(tmp0)+strlen(tmp)+2)*sizeof(char));
    sprintf(tmp1,"%s,%s",tmp0,tmp);
    free(tmp0);
    free(tmp);
  }else{
    if(tmp!=NULL){
      tmp1=zStrdup(tmp);
      free(tmp);
    }else{
      if(tmp0!=NULL){
	tmp1=zStrdup(tmp0);
	free(tmp0);
      }
    }
  }
  char *pToken,*saveptr;
  pToken=strtok_r(tmp1,",",&saveptr);
  int res=-1;
  while(pToken!=NULL){
    if(strcasecmp(str,pToken)==0){
      res=1;
      break;
    }
    pToken=strtok_r(NULL,",",&saveptr);
  }
  if(tmp1!=NULL)
    free(tmp1);
  return res;
}


/**
 * Access the value of the encoding key in a maps
 *
 * @param m the maps to search for the encoding key
 * @return the value of the encoding key in a maps if encoding key exists,
 *  "UTF-8" in other case.
 */
char* getEncoding(maps* m){
  if(m!=NULL){
    map* tmp=getMap(m->content,"encoding");
    if(tmp!=NULL){
      return tmp->value;
    }
    else
      return (char*)"UTF-8";
  }
  else
    return (char*)"UTF-8";  
}

/**
 * Access the value of the version key in a maps
 *
 * @param m the maps to search for the version key
 * @return the value of the version key in a maps if encoding key exists,
 *  "1.0.0" in other case.
 */
char* getVersion(maps* m){
  if(m!=NULL){
    map* tmp=getMap(m->content,"version");
    if(tmp!=NULL){
      return tmp->value;
    }
    else
      return (char*)"1.0.0";
  }
  else
    return (char*)"1.0.0";
}

/**
 * Read a file generated by a service.
 * 
 * @param m the conf maps
 * @param content the output item
 * @param filename the file to read
 */
void readGeneratedFile(maps* m,map* content,char* filename){
  char rsize[1024];
  FILE * file=fopen(filename,"rb");
  if(file==NULL){
    setMapInMaps(m,"lenv","message","Unable to read produced file. Please try again later");
    return ;
  }
  fseek(file, 0, SEEK_END);
  zStatStruct f_status;
  int s=zStat(filename, &f_status);
  sprintf(rsize,"%lld",f_status.st_size);
  rewind(file);
  if(getMap(content,"storage")==NULL){
    map* memUse=getMapFromMaps(m,"main","memory");
    if(memUse==NULL || strncmp(memUse->value,"load",4)==0){
      map* tmpMap1=getMap(content,"value");
      if(tmpMap1==NULL){
	addToMap(content,"value","");
	tmpMap1=getMap(content,"value");
      }
      free(tmpMap1->value);
      tmpMap1->value=(char*) malloc((f_status.st_size+1)*sizeof(char));
      if(tmpMap1->value==NULL){
	setMapInMaps(m,"lenv","message","Unable to allocate the memory required to read the produced file.");
      }
      fread(&tmpMap1->value,1,f_status.st_size,file);
      tmpMap1->value[f_status.st_size]=0;
    }
  }
  fclose(file);  
  addToMap(content,"size",rsize);
}


/**
 * Write a file from value and length
 *
 * @param fname the file name
 * @param val the value
 * @param length the value length
 */
int writeFile(char* fname,char* val,int length){
  FILE* of=fopen(fname,"wb");
  if(of==NULL){
    return -1;
  }
  size_t ret=fwrite(val,sizeof(char),length,of);
  if(ret<length){
    fprintf(stderr,"Write error occurred!\n");
    fclose(of);
    return -1;
  }
  fclose(of);
  return 1;
}

/**
 * Dump all values in a maps as files
 *
 * @param main_conf the maps containing the settings of the main.cfg file
 * @param in the maps containing values to dump as files
 */
void dumpMapsValuesToFiles(maps** main_conf,maps** in){
  map* tmpPath=getMapFromMaps(*main_conf,"main","tmpPath");
  map* tmpUrl=getMapFromMaps(*main_conf,"main","tmpUrl");
  map* tmpSid=getMapFromMaps(*main_conf,"lenv","usid");
  maps* inputs=*in;
  int length=0;
  while(inputs!=NULL){
    if(getMap(inputs->content,"mimeType")!=NULL &&
       getMap(inputs->content,"cache_file")==NULL){
      map* cMap=inputs->content;
      if(getMap(cMap,"length")!=NULL){
	map* tmpLength=getMap(cMap,"length");
	int len=atoi(tmpLength->value);
	int k=0;
	for(k=0;k<len;k++){
	  map* cMimeType=getMapArray(cMap,"mimeType",k);
	  map* cValue=getMapArray(cMap,"value",k);
	  map* cSize=getMapArray(cMap,"size",k);
	  char file_ext[32];
	  getFileExtension(cMimeType != NULL ? cMimeType->value : NULL, file_ext, 32);
	  char* val=(char*)malloc((strlen(tmpPath->value)+strlen(inputs->name)+strlen(tmpSid->value)+strlen(file_ext)+16)*sizeof(char));
	  sprintf(val,"%s/Input_%s_%s_%d.%s",tmpPath->value,inputs->name,tmpSid->value,k,file_ext);
	  length=0;
	  if(cSize!=NULL){
	    length=atoi(cSize->value);
	  }else
	    length=strlen(cValue->value);
	  writeFile(val,cValue->value,length);
	  setMapArray(cMap,"cache_file",k,val);
	  free(val);
	  val=(char*)malloc((strlen(tmpUrl->value)+strlen(inputs->name)+strlen(tmpSid->value)+strlen(file_ext)+16)*sizeof(char));
	  sprintf(val,"%s/Input_%s_%s_%d.%s",tmpUrl->value,inputs->name,tmpSid->value,k,file_ext);
	  setMapArray(cMap,"cache_url",k,val);
	  setMapArray(cMap,"byValue",k,"true");
	  free(val);
	}
      }else{
	int length=0;
	map* cMimeType=getMap(cMap,"mimeType");
	map* cValue=getMap(cMap,"value");
	map* cSize=getMap(cMap,"size");
	char file_ext[32];
	getFileExtension(cMimeType != NULL ? cMimeType->value : NULL, file_ext, 32);
	char *val=(char*)malloc((strlen(tmpPath->value)+strlen(inputs->name)+strlen(tmpSid->value)+strlen(file_ext)+16)*sizeof(char));
	sprintf(val,"%s/Input_%s_%s_%d.%s",tmpPath->value,inputs->name,tmpSid->value,0,file_ext);
	if(cSize!=NULL){
	  length=atoi(cSize->value);
	}else
	  length=strlen(cValue->value);
	writeFile(val,cValue->value,length);
	addToMap(cMap,"cache_file",val);
	free(val);
	val=(char*)malloc((strlen(tmpUrl->value)+strlen(inputs->name)+strlen(tmpSid->value)+strlen(file_ext)+16)*sizeof(char));
	sprintf(val,"%s/Input_%s_%s_%d.%s",tmpUrl->value,inputs->name,tmpSid->value,0,file_ext);
	addToMap(cMap,"cache_url",val);
	addToMap(cMap,"byValue",val);
	free(val);
      }
    }
    inputs=inputs->next;
  }
}


/**
 * Base64 encoding of a char*
 *
 * @param input the value to encode
 * @param length the value length
 * @return the buffer containing the base64 value
 * @warning make sure to free the returned value
 */
char *base64(const char *input, int length)
{
  BIO *bmem, *b64;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_write(b64, input, length);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  char *buff = (char *)malloc((bptr->length+1)*sizeof(char));
  memcpy(buff, bptr->data, bptr->length);
  buff[bptr->length] = 0;

  BIO_free_all(b64);

  return buff;
}

/**
 * Base64 decoding of a char*
 *
 * @param input the value to decode
 * @param length the value length
 * @param red the value length
 * @return the buffer containing the base64 value 
 * @warning make sure to free the returned value
 */
char *base64d(const char *input, int length,int* red)
{
  BIO *b64, *bmem;

  char *buffer = (char *)malloc(length);
  if(buffer){
    memset(buffer, 0, length);
    b64 = BIO_new(BIO_f_base64());
    if(b64){
      bmem = BIO_new_mem_buf((unsigned char*)input,length);
      bmem = BIO_push(b64, bmem);
      *red=BIO_read(bmem, buffer, length);
      buffer[length-1]=0;
      BIO_free_all(bmem);
    }
  }
  return buffer;
}

/**
 * Read Base64 value and split it value by lines of 64 char.
 *
 * @param in the map containing the value to split
 */
void readBase64(map **in){
  char *res = NULL;
  char *curs = (*in)->value;
  int i = 0;
  for (i = 0; i <= strlen ((*in)->value) / 64;
       i++)
    {
      if (res == NULL)
	res =
	  (char *) malloc (65 * sizeof (char));
      else
	res =
	  (char *) realloc (res,
			    (((i + 1) * 65) +
			     i) * sizeof (char));
      int csize = i * 65;
      strncpy (res + csize, curs, 64);
      if (i == strlen ((*in)->value) / 64)
	strcat (res, "\n\0");
      else
	{
	  strncpy (res + (((i + 1) * 64) + i),
		   "\n\0", 2);
	  curs += 64;
	}
    }
  free ((*in)->value);
  (*in)->value = zStrdup (res);
  free (res);
}


/**
 * Add the default values defined in the zcfg to a maps.
 *
 * @param out the maps containing the inputs or outputs given in the initial
 *  HTTP request
 * @param in the description of all inputs or outputs available for a service
 * @param m the maps containing the settings of the main.cfg file
 * @param type 0 for inputs and 1 for outputs
 * @param err the map to store potential missing mandatory input parameters or
 *  wrong output names depending on the type.
 * @return "" if no error was detected, the name of last input or output causing
 *  an error.
 */
char* addDefaultValues(maps** out,elements* in,maps* m,int type,map** err){
  map *res=*err;
  elements* tmpInputs=in;
  elements* tmpInputss=NULL;
  maps* out1=*out;
  maps* out1s=NULL;
  char *result=NULL;
  int nb=0;
  int inb=0;
 loopOnInputs:
  if(type==1){
    while(out1!=NULL){
      if(getElements(in,out1->name)==NULL){
	if(res==NULL){
	  res=createMap("value",out1->name);
	}else{
	  setMapArray(res,"value",nb,out1->name);
	}
	nb++;
	result=out1->name;
      }
      inb++;
      out1=out1->next;
    }
    if(res!=NULL){
      fflush(stderr);
      *err=res;
      return result;
    }
    if(out1==NULL && inb>=1)
      out1=*out;
  }
  while(tmpInputs!=NULL){
    maps *tmpMaps=getMaps(out1,tmpInputs->name);
    if(tmpMaps==NULL){
      maps* tmpMaps2=createMaps(tmpInputs->name);
      if(type==0){
	map* tmpMapMinO=getMap(tmpInputs->content,"minOccurs");
	if(tmpMapMinO!=NULL){
	  if(atoi(tmpMapMinO->value)>=1){
	    freeMaps(&tmpMaps2);
	    free(tmpMaps2);
	    if(res==NULL){
	      res=createMap("value",tmpInputs->name);
	    }else{
	      setMapArray(res,"value",nb,tmpInputs->name);
	    }
	    nb++;
	    result=tmpInputs->name;
	  }
	  else{
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("minOccurs",tmpMapMinO->value);
	    else
	      addToMap(tmpMaps2->content,"minOccurs",tmpMapMinO->value);
	  }
	}
	if(res==NULL){
	  map* tmpMaxO0=getMap(tmpInputs->content,"useMapserver");
	  if(tmpMaxO0!=NULL){
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("useMapserver",tmpMaxO0->value);
	    else
	      addToMap(tmpMaps2->content,"useMapserver",tmpMaxO0->value);
	  }
	  map* tmpMaxO=getMap(tmpInputs->content,"maxOccurs");
	  if(tmpMaxO!=NULL){
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("maxOccurs",tmpMaxO->value);
	    else
	      addToMap(tmpMaps2->content,"maxOccurs",tmpMaxO->value);
	  }
	  map* tmpMaxMB=getMap(tmpInputs->content,"maximumMegabytes");
	  if(tmpMaxMB!=NULL){
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap("maximumMegabytes",tmpMaxMB->value);
	    else
	      addToMap(tmpMaps2->content,"maximumMegabytes",tmpMaxMB->value);
	  }
	}
      }
      
      if(res==NULL){
	iotype* tmpIoType=tmpInputs->defaults;
	if(tmpIoType!=NULL){
	  map* tmpm=tmpIoType->content;
	  while(tmpm!=NULL){
	    if(tmpMaps2->content==NULL)
	      tmpMaps2->content=createMap(tmpm->name,tmpm->value);
	    else{
	      addToMap(tmpMaps2->content,tmpm->name,tmpm->value);
	    }
	    tmpm=tmpm->next;
	  }
	}
	if(tmpMaps2->content==NULL){
	  tmpMaps2->content=createMap("inRequest","false");
	  dumpMaps(tmpMaps2);
	}
	else
	  addToMap(tmpMaps2->content,"inRequest","false");
	if(type==0){
	  map *tmpMap=getMap(tmpMaps2->content,"value");
	  if(tmpMap==NULL)
	    addToMap(tmpMaps2->content,"value","NULL");
	}
	elements* tmpElements=getElements(in,tmpMaps2->name);
	if(tmpElements!=NULL && tmpElements->child!=NULL){
	  char *res=addDefaultValues(&tmpMaps2->child,tmpElements->child,m,type,err);
	  if(strlen(res)>0){
	    return res;
	  }
	}

	if(out1==NULL){
	  *out=dupMaps(&tmpMaps2);
	  out1=*out;
	}
	else
	  addMapsToMaps(&out1,tmpMaps2);
	freeMap(&tmpMaps2->content);
	free(tmpMaps2->content);
	tmpMaps2->content=NULL;
	freeMaps(&tmpMaps2);
	free(tmpMaps2);
	tmpMaps2=NULL;
      }
    }
    else /*toto*/{ 
      iotype* tmpIoType=NULL;
      if(tmpMaps->content!=NULL){
	tmpIoType=getIoTypeFromElement(tmpInputs,tmpInputs->name,
				       tmpMaps->content);
	if(type==0) {
	  /**
	   * In case of an Input maps, then add the minOccurs and maxOccurs to the
	   * content map.
	   */
	  char* keys[4]={
	    "minOccurs",
	    "maxOccurs",
	    "maximumMegabytes",
	    "useMapserver"
	  };
	  int i=0;
	  for(i=0;i<4;i++){
	    map* tmpMap1=getMap(tmpInputs->content,keys[i]);
	    if(tmpMap1!=NULL){
	      addToMap(tmpMaps->content,keys[i],tmpMap1->value);
	    }
	  }
	  /**
	   * Parsing BoundingBoxData, fill the following map and then add it to
	   * the content map of the Input maps: 
	   * lowerCorner, upperCorner, srs and dimensions
	   * cf. parseBoundingBox
	   */
	  if(tmpInputs->format!=NULL && strcasecmp(tmpInputs->format,"BoundingBoxData")==0){
	    maps* tmpI=getMaps(*out,tmpInputs->name);
	    if(tmpI!=NULL){
	      map* tmpV=getMap(tmpI->content,"value");
	      if(tmpV!=NULL){
		char *tmpVS=zStrdup(tmpV->value);
		map* tmp=parseBoundingBox(tmpVS);
		free(tmpVS);
		map* tmpC=tmp;
		while(tmpC!=NULL){
		  addToMap(tmpMaps->content,tmpC->name,tmpC->value);
		  tmpC=tmpC->next;
		}
		freeMap(&tmp);
		free(tmp);
	      }
	    }
	  }
	}
      }else{
	if(tmpInputs!=NULL){
	  tmpIoType=tmpInputs->defaults;
	}
      }

      if(tmpIoType!=NULL){
	map* tmpContent=tmpIoType->content;
	map* cval=NULL;
	int hasPassed=-1;
	while(tmpContent!=NULL){
	  if((cval=getMap(tmpMaps->content,tmpContent->name))==NULL){
#ifdef DEBUG
	    fprintf(stderr,"addDefaultValues %s => %s\n",tmpContent->name,tmpContent->value);
#endif
	    if(tmpMaps->content==NULL)
	      tmpMaps->content=createMap(tmpContent->name,tmpContent->value);
	    else
	      addToMap(tmpMaps->content,tmpContent->name,tmpContent->value);
	    
	    if(hasPassed<0 && type==0 && getMap(tmpMaps->content,"isArray")!=NULL){
	      map* length=getMap(tmpMaps->content,"length");
	      int i;
	      char *tcn=zStrdup(tmpContent->name);
	      for(i=1;i<atoi(length->value);i++){
#ifdef DEBUG
		dumpMap(tmpMaps->content);
		fprintf(stderr,"addDefaultValues %s_%d => %s\n",tcn,i,tmpContent->value);
#endif
		int len=strlen((char*) tcn);
		char *tmp1=(char *)malloc((len+10)*sizeof(char));
		sprintf(tmp1,"%s_%d",tcn,i);
#ifdef DEBUG
		fprintf(stderr,"addDefaultValues %s => %s\n",tmp1,tmpContent->value);
#endif
		addToMap(tmpMaps->content,tmp1,tmpContent->value);
		free(tmp1);
		hasPassed=1;
	      }
	      free(tcn);
	    }
	  }
	  tmpContent=tmpContent->next;
	}
#ifdef USE_MS
	/**
	 * check for useMapServer presence
	 */
	if(tmpIoType!=NULL){
	  map* tmpCheck=getMap(tmpIoType->content,"useMapserver");
	  if(tmpCheck!=NULL && strncasecmp(tmpCheck->value,"true",4)==0){
	    // Get the default value
	    addToMap(tmpMaps->content,"useMapserver","true");
	    tmpIoType=getIoTypeFromElement(tmpInputs,tmpInputs->name,NULL);
	    tmpCheck=getMap(tmpMaps->content,"mimeType");
	    addToMap(tmpMaps->content,"requestedMimeType",tmpCheck->value);
	    map* cursor=tmpIoType->content;
	    while(cursor!=NULL){
	      addToMap(tmpMaps->content,cursor->name,cursor->value);
	      cursor=cursor->next;
	    }
	  
	    cursor=tmpInputs->content;
	    while(cursor!=NULL){
	      if(strcasecmp(cursor->name,"Title")==0 ||
		 strcasecmp(cursor->name,"Abstract")==0)
		addToMap(tmpMaps->content,cursor->name,cursor->value);
	      cursor=cursor->next;
	    }
	  }
	}
#endif
      }
      if(tmpMaps->content==NULL)
	tmpMaps->content=createMap("inRequest","true");
      else
	addToMap(tmpMaps->content,"inRequest","true");
      elements* tmpElements=getElements(in,tmpMaps->name);
      if(/*tmpMaps->child!=NULL && */tmpElements!=NULL && tmpElements->child!=NULL){
	char *res=addDefaultValues(&tmpMaps->child,tmpElements->child,m,type,err);
	if(strlen(res)>0){
	  return res;
	}
      }
    }
    if(tmpInputs->child!=NULL){
      tmpInputss=tmpInputs->next;
      tmpInputs=tmpInputs->child;
      if(tmpMaps!=NULL){
	out1=tmpMaps->child;
	out1s=tmpMaps;
      }
    }
    tmpInputs=tmpInputs->next;
  }
  if(tmpInputss!=NULL){
    out1=out1s;
    tmpInputs=tmpInputss;
    tmpInputss=NULL;
    out1s=NULL;
    goto loopOnInputs;
  }
  if(res!=NULL){
    *err=res;
    return result;
  }
  return "";
}

/**
 * Access the last error message returned by the OS when trying to dynamically
 * load a shared library.
 *
 * @return the last error message
 * @warning The character string returned from getLastErrorMessage resides
 * in a static buffer. The application should not write to this
 * buffer or attempt to free() it.
 */ 
char* getLastErrorMessage() {                                              
#ifdef WIN32
  LPVOID lpMsgBuf;
  DWORD errCode = GetLastError();
  static char msg[ERROR_MSG_MAX_LENGTH];
  size_t i;
  
  DWORD length = FormatMessage(
			       FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			       FORMAT_MESSAGE_FROM_SYSTEM |
			       FORMAT_MESSAGE_IGNORE_INSERTS,
			       NULL,
			       errCode,
			       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			       (LPTSTR) &lpMsgBuf,
			       0, NULL );	
  
#ifdef UNICODE		
  wcstombs_s( &i, msg, ERROR_MSG_MAX_LENGTH,
	      (wchar_t*) lpMsgBuf, _TRUNCATE );
#else
  strcpy_s( msg, ERROR_MSG_MAX_LENGTH,
	    (char *) lpMsgBuf );		
#endif	
  LocalFree(lpMsgBuf);

  return msg;
#else
  return dlerror();
#endif
}

#include <dirent.h>
#ifndef RELY_ON_DB
/**
 * Read the Result file (.res).
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 */
void readFinalRes(maps* conf,char* pid,map* statusInfo){
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  char* fbkpid =
    (char *)
    malloc ((strlen (r_inputs->value) + strlen (pid) + 7) * sizeof (char));
  sprintf (fbkpid, "%s/%s.res", r_inputs->value, pid);
  struct stat file_status;
  int istat = stat (fbkpid, &file_status);
  if (istat == 0 && file_status.st_size > 0)
    {
      maps *res = (maps *) malloc (MAPS_SIZE);
      conf_read (fbkpid, res);
      res->child=NULL;
      map* status=getMapFromMaps(res,"status","status");
      addToMap(statusInfo,"Status",status->value);
      freeMaps(&res);
      free(res);
    }
  else
    addToMap(statusInfo,"Status","Failed");  
  free(fbkpid);
}

/**
 * Check if a service is running.
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the unique service identifier (usid from the lenv section)
 * @return 1 in case the service is still running, 0 otherwise
 */
int isRunning(maps* conf,char* pid){
  int res=0;
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  char* fbkpid =
    (char *)
    malloc ((strlen (r_inputs->value) + strlen (pid) + 7) * sizeof (char));
  sprintf (fbkpid, "%s/%s.pid", r_inputs->value, pid);  
  FILE* f0 = fopen (fbkpid, "r");
  if(f0!=NULL){
    fclose(f0);
    res=1;
  }
  free(fbkpid);
  return res;
}
#else
#include "sqlapi.h"
#endif

/**
 * Run GetStatus requests.
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @param req the request (GetStatus / GetResult)
 */
void runGetStatus(maps* conf,char* pid,char* req){
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  char *sid=getStatusId(conf,pid);
  if(sid==NULL){
    errorException (conf, _("The JobID from the request does not match any of the Jobs running on this server"),
		    "NoSuchJob", pid);
  }else{
    map* statusInfo=createMap("JobID",pid);
    if(isRunning(conf,pid)>0){		
      if(strncasecmp(req,"GetResult",strlen(req))==0){
	errorException (conf, _("The result for the requested JobID has not yet been generated. The service is currently running."),
			"ResultNotReady", pid);
	return;
      }
      else
	if(strncasecmp(req,"GetStatus",strlen(req))==0){
	  addToMap(statusInfo,"Status","Running");
	  char* tmpStr=_getStatus(conf,pid);
	  if(tmpStr!=NULL && strncmp(tmpStr,"-1",2)!=0){
	    char *tmpStr1=zStrdup(tmpStr);
	    char *tmpStr0=zStrdup(strstr(tmpStr,"|")+1);
	    free(tmpStr);
	    tmpStr1[strlen(tmpStr1)-strlen(tmpStr0)-1]='\0';
	    addToMap(statusInfo,"PercentCompleted",tmpStr1);
	    addToMap(statusInfo,"Message",tmpStr0);
	    free(tmpStr0);
	    free(tmpStr1);
	  }
	}
    }
    else{
      if(strncasecmp(req,"GetResult",strlen(req))==0){
	char* result=_getStatusFile(conf,pid);
	if(result!=NULL){
	  char *encoding=getEncoding(conf);
	  printf("Content-Type: text/xml; charset=%s\r\nStatus: 200 OK\r\n\r\n",encoding);
	  printf("%s",result);
	  fflush(stdout);
	  free(sid);
	  freeMap(&statusInfo);
	  free(statusInfo);
	  free(result);
	  return;
	}else{
	  errorException (conf, _("The result for the requested JobID has not yet been generated. The service ends but it still needs to produce the outputs."),
			  "ResultNotReady", pid);
	  freeMap(&statusInfo);
	  free(statusInfo);
	  return;
	}
      }else
	if(strncasecmp(req,"GetStatus",strlen(req))==0){
	  readFinalRes(conf,pid,statusInfo);
	  char* tmpStr=_getStatus(conf,pid);
	  if(tmpStr!=NULL && strncmp(tmpStr,"-1",2)!=0){
	    char *tmpStr1=zStrdup(tmpStr);
	    char *tmpStr0=zStrdup(strstr(tmpStr,"|")+1);
	    free(tmpStr);
	    tmpStr1[strlen(tmpStr1)-strlen(tmpStr0)-1]='\0';
	    addToMap(statusInfo,"PercentCompleted",tmpStr1);
	    addToMap(statusInfo,"Message",tmpStr0);
	    free(tmpStr0);
	    free(tmpStr1);
	  }
	}
    }
    free(sid);
    printStatusInfo(conf,statusInfo,req);
    freeMap(&statusInfo);
    free(statusInfo);
  }
  return;
}

/**
 * Run Dismiss requests.
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 */
void runDismiss(maps* conf,char* pid){
  map* r_inputs = getMapFromMaps (conf, "main", "tmpPath");
  char *sid=getStatusId(conf,pid);
  if(sid==NULL){
    errorException (conf, _("The JobID from the request does not match any of the Jobs running on this server"),
		    "NoSuchJob", pid);
  }else{
    // We should send the Dismiss request to the target host if it differs
    char* fbkpid =
      (char *)
      malloc ((strlen (r_inputs->value) + strlen (pid) + 7) * sizeof (char));
    sprintf (fbkpid, "%s/%s.pid", r_inputs->value, pid);
    FILE* f0 = fopen (fbkpid, "r");
    if(f0!=NULL){
      long flen;
      char *fcontent;
      fseek (f0, 0, SEEK_END);
      flen = ftell (f0);
      fseek (f0, 0, SEEK_SET);
      fcontent = (char *) malloc ((flen + 1) * sizeof (char));
      fread(fcontent,flen,1,f0);
      fcontent[flen]=0;
      fclose(f0);
#ifndef WIN32
      kill(atoi(fcontent),SIGKILL);
#else
      HANDLE myZooProcess=OpenProcess(PROCESS_ALL_ACCESS,false,atoi(fcontent));
      TerminateProcess(myZooProcess,1);
      CloseHandle(myZooProcess);
#endif
      free(fcontent);
    }
    free(fbkpid);
    struct dirent *dp;
    DIR *dirp = opendir(r_inputs->value);
    char fileName[1024];
    int hasFile=-1;
    if(dirp!=NULL){
      while ((dp = readdir(dirp)) != NULL){
#ifdef DEBUG
	fprintf(stderr,"File : %s searched : %s\n",dp->d_name,tmp);
#endif
	if(strstr(dp->d_name,pid)!=0){
	  sprintf(fileName,"%s/%s",r_inputs->value,dp->d_name);
	  if(zUnlink(fileName)!=0){
	    errorException (conf, 
			    _("The job cannot be removed, a file cannot be removed"),
			    "NoApplicableCode", NULL);
	    return;
	  }
	}
      }
    }
#ifdef RELY_ON_DB
    removeService(conf,pid);
#endif
    /* No need to call 7_1 when an execution is dismissed.
      fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
      invokeCallback(conf,NULL,NULL,7,1);
      fprintf(stderr,"************************* %s %d \n\n",__FILE__,__LINE__);
    */
    map* statusInfo=createMap("JobID",pid);
    addToMap(statusInfo,"Status","Dismissed");
    printStatusInfo(conf,statusInfo,"Dismiss");
    free(statusInfo);
  }
  return;
}

extern int getServiceFromFile (maps *, const char *, service **);

/**
 * Parse the service file using getServiceFromFile or use getServiceFromYAML
 * if YAML support was activated.
 *
 * @param conf the conf maps containing the main.cfg settings
 * @param file the file name to parse
 * @param service the service to update witht the file content
 * @param name the service name
 * @return true if the file can be parsed or false
 * @see getServiceFromFile, getServiceFromYAML
 */
int readServiceFile (maps * conf, char *file, service ** service, char *name){
  int t = getServiceFromFile (conf, file, service);
#ifdef YAML
  if (t < 0){
    t = getServiceFromYAML (conf, file, service, name);
  }
#endif
  return t;
}

/**
 * Create the profile registry.
 *
 * The profile registry is optional (created only if the registry key is
 * available in the [main] section of the main.cfg file) and can be used to
 * store the profiles hierarchy. The registry is a directory which should
 * contain the following sub-directories: 
 *  * concept: direcotry containing .html files describing concept
 *  * generic: directory containing .zcfg files for wps:GenericProcess
 *  * implementation: directory containing .zcfg files for wps:Process
 *
 * @param m the conf maps containing the main.cfg settings
 * @param r the registry to update
 * @param reg_dir the resgitry 
 * @return 0 if the resgitry is null or was correctly updated, -1 on failure
 */
int createRegistry (maps* m,registry ** r, char *reg_dir)
{
  char registryKeys[3][15]={
    "concept",
    "generic",
    "implementation"
  };
  int scount = 0,i=0;
  if (reg_dir == NULL)
    return 0;
  for(i=0;i<3;i++){
    char * tmpName =
      (char *) malloc ((strlen (reg_dir) + strlen (registryKeys[i]) + 2) *
		       sizeof (char));
    sprintf (tmpName, "%s/%s", reg_dir, registryKeys[i]);
    
    DIR *dirp1 = opendir (tmpName);
    if(dirp1==NULL){
      setMapInMaps(m,"lenv","message",_("Unable to open the registry directory."));
      setMapInMaps(m,"lenv","type","InternalError");
      return -1;
    }
    struct dirent *dp1;
    while ((dp1 = readdir (dirp1)) != NULL){
      char* extn = strstr(dp1->d_name, ".zcfg");
      if(dp1->d_name[0] != '.' && extn != NULL && strlen(extn) == 5)
	{
	  int t;
	  char *tmps1=
	    (char *) malloc ((strlen (tmpName) + strlen (dp1->d_name) + 2) *
			     sizeof (char));
	  sprintf (tmps1, "%s/%s", tmpName, dp1->d_name);
	  char *tmpsn = zStrdup (dp1->d_name);
	  tmpsn[strlen (tmpsn) - 5] = 0;
	  service* s1 = (service *) malloc (SERVICE_SIZE);
	  if (s1 == NULL)
	    {
	      setMapInMaps(m,"lenv","message",_("Unable to allocate memory."));
	      setMapInMaps(m,"lenv","type","InternalError");
	      return -2;
	    }
	  t = readServiceFile (m, tmps1, &s1, tmpsn);
	  free (tmpsn);
	  if (t < 0)
	    {
	      map *tmp00 = getMapFromMaps (m, "lenv", "message");
	      char tmp01[1024];
	      if (tmp00 != NULL)
		sprintf (tmp01, _("Unable to parse the ZCFG file: %s (%s)"),
			 dp1->d_name, tmp00->value);
	      else
		sprintf (tmp01, _("Unable to parse the ZCFG file: %s."),
			 dp1->d_name);
	      setMapInMaps(m,"lenv","message",tmp01);
	      setMapInMaps(m,"lenv","type","InternalError");
	      return -1;
	    }
	  if(strncasecmp(registryKeys[i],"implementation",14)==0){
	    inheritance(*r,&s1);
	  }
	  addServiceToRegistry(r,registryKeys[i],s1);
	  freeService (&s1);
	  free (s1);
	  scount++;
	}
    }
    (void) closedir (dirp1);
  }
  return 0;
}

#ifdef WIN32
/**
 * Create a KVP request for executing background task.
 * TODO: use the XML request in case of input POST request.
 *
 * @param m the maps containing the parameters from the main.cfg file
 * @param length the total length of the KVP parameters
 * @param type
 */
char* getMapsAsKVP(maps* m,int length,int type){
  char *dataInputsKVP=(char*) malloc(length*sizeof(char));
  char *dataInputsKVPi=NULL;
  maps* curs=m;
  int i=0;
  while(curs!=NULL){
    map *inRequest=getMap(curs->content,"inRequest");
    map *hasLength=getMap(curs->content,"length");
    if((inRequest!=NULL && strncasecmp(inRequest->value,"true",4)==0) ||
       inRequest==NULL){
      if(i==0)
	if(type==0){
	  sprintf(dataInputsKVP,"%s=",curs->name);
	  if(hasLength!=NULL){
	    dataInputsKVPi=(char*)malloc((strlen(curs->name)+2)*sizeof(char));
	    sprintf(dataInputsKVPi,"%s=",curs->name);
	  }
	}
	else
	  sprintf(dataInputsKVP,"%s",curs->name);
      else{
	char *temp=zStrdup(dataInputsKVP);
	if(type==0)
	  sprintf(dataInputsKVP,"%s;%s=",temp,curs->name);
	else
	  sprintf(dataInputsKVP,"%s;%s",temp,curs->name);
      }
      map* icurs=curs->content;
      if(type==0){
	char *temp=zStrdup(dataInputsKVP);
	if(getMap(curs->content,"xlink:href")!=NULL)
	  sprintf(dataInputsKVP,"%sReference",temp);
	else{
	  if(hasLength!=NULL){
	    int j;
	    for(j=0;j<atoi(hasLength->value);j++){
	      map* tmp0=getMapArray(curs->content,"value",j);
	      if(j==0)
		free(temp);
	      temp=zStrdup(dataInputsKVP);
	      if(j==0)
		sprintf(dataInputsKVP,"%s%s",temp,tmp0->value);
	      else
		sprintf(dataInputsKVP,"%s;%s%s",temp,dataInputsKVPi,tmp0->value);
	    }
	  }
	  else
	    sprintf(dataInputsKVP,"%s%s",temp,icurs->value);
	}
	free(temp);
      }
      while(icurs!=NULL){
	if(strncasecmp(icurs->name,"value",5)!=0 &&
	   strncasecmp(icurs->name,"mimeType_",9)!=0 &&
	   strncasecmp(icurs->name,"dataType_",9)!=0 &&
	   strncasecmp(icurs->name,"size",4)!=0 &&
	   strncasecmp(icurs->name,"length",4)!=0 &&
	   strncasecmp(icurs->name,"isArray",7)!=0 &&
	   strcasecmp(icurs->name,"Reference")!=0 &&
	   strcasecmp(icurs->name,"minOccurs")!=0 &&
	   strcasecmp(icurs->name,"maxOccurs")!=0 &&
	   strncasecmp(icurs->name,"fmimeType",9)!=0 &&
	   strcasecmp(icurs->name,"inRequest")!=0){
	  char *itemp=zStrdup(dataInputsKVP);
	  if(strcasecmp(icurs->name,"xlink:href")!=0)
	    sprintf(dataInputsKVP,"%s@%s=%s",itemp,icurs->name,icurs->value);
	  else
	    sprintf(dataInputsKVP,"%s@%s=%s",itemp,icurs->name,url_encode(icurs->value));
	  free(itemp);
	}
	icurs=icurs->next;
      }
    }
    curs=curs->next;
    i++;
  }
  return dataInputsKVP;
}
#endif
