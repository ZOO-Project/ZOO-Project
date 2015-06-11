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
#include "response_print.h"
#include "mimetypes.h"
#ifndef WIN32
#include <dlfcn.h>
#endif
#include <uuid/uuid.h>

int getVersionId(const char* version){
  int schemaId=0;
  for(;schemaId<2;schemaId++){
    if(strncasecmp(version,schemas[schemaId][0],5)==0)
      return schemaId;
  }
  return 0;
}

/**
 * Generate a UUID.
 * ref: https://www.ietf.org/rfc/rfc4122.txt / 4.2 
 *
 * @return a new char* containing the UUID, make sure to free the returned 
 *  ressource once used.
 */
char *get_uuid(){
  char *res=(char*)malloc(37*sizeof(char));
  uuid_t uuid;
  uuid_generate_time(uuid);
  char * rest;
  uuid_unparse(uuid,rest);
  sprintf(res,"%s", rest);
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
  map *tmpMap=getMapFromMaps(conf,"main","lang");
  char *tmp=zStrdup(tmpMap->value);
  char *pToken,*saveptr;
  pToken=strtok_r(tmp,",",&saveptr);
  int res=-1;
  char *pToken1,*saveptr1;
  pToken1=strtok_r(tmp,",",&saveptr1);
  while(pToken1!=NULL){
    while(pToken!=NULL){
      if(strcasecmp(pToken1,pToken)==0){
	res=1;
	break;
      }
      pToken=strtok_r(NULL,",",&saveptr);
    }
    pToken1=strtok_r(NULL,",",&saveptr1);
  }
  free(tmp);
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
  FILE * file=fopen(filename,"rb");
  if(file==NULL){
    fprintf(stderr,"Failed to open file %s for reading purpose.\n",filename);
    setMapInMaps(m,"lenv","message","Unable to read produced file. Please try again later");
    return ;
  }
  fseek(file, 0, SEEK_END);
  long count = ftell(file);
  rewind(file);
  struct stat file_status; 
  stat(filename, &file_status);
  map* tmpMap1=getMap(content,"value");
  if(tmpMap1==NULL){
    addToMap(content,"value","");
    tmpMap1=getMap(content,"value");
  }
  free(tmpMap1->value);
  tmpMap1->value=(char*) malloc((count+1)*sizeof(char));  
  fread(tmpMap1->value,1,count,file);
  tmpMap1->value[count]=0;
  fclose(file);
  char rsize[1000];
  sprintf(rsize,"%ld",count);
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
    fprintf(stderr,"Write error occured!\n");
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
	  }
	  writeFile(val,cValue->value,length);
	  setMapArray(cMap,"cache_file",k,val);
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
	}
	writeFile(val,cValue->value,length);
	addToMap(cMap,"cache_file",val);
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
  maps* out1=*out;
  char *result=NULL;
  int nb=0;
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
      out1=out1->next;
    }
    if(res!=NULL){
      *err=res;
      return result;
    }
    out1=*out;
  }
  while(tmpInputs!=NULL){
    maps *tmpMaps=getMaps(out1,tmpInputs->name);
    if(tmpMaps==NULL){
      maps* tmpMaps2=(maps*)malloc(MAPS_SIZE);
      tmpMaps2->name=strdup(tmpInputs->name);
      tmpMaps2->content=NULL;
      tmpMaps2->next=NULL;
      
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
	    else
	      addToMap(tmpMaps2->content,tmpm->name,tmpm->value);
	    tmpm=tmpm->next;
	  }
	}
	addToMap(tmpMaps2->content,"inRequest","false");
	if(type==0){
	  map *tmpMap=getMap(tmpMaps2->content,"value");
	  if(tmpMap==NULL)
	    addToMap(tmpMaps2->content,"value","NULL");
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
    else{
      iotype* tmpIoType=getIoTypeFromElement(tmpInputs,tmpInputs->name,
					     tmpMaps->content);
      if(type==0) {
	/**
	 * In case of an Input maps, then add the minOccurs and maxOccurs to the
	 * content map.
	 */
	map* tmpMap1=getMap(tmpInputs->content,"minOccurs");
	if(tmpMap1!=NULL){
	  if(tmpMaps->content==NULL)
	    tmpMaps->content=createMap("minOccurs",tmpMap1->value);
	  else
	    addToMap(tmpMaps->content,"minOccurs",tmpMap1->value);
	}
	map* tmpMaxO=getMap(tmpInputs->content,"maxOccurs");
	if(tmpMaxO!=NULL){
	  if(tmpMaps->content==NULL)
	    tmpMaps->content=createMap("maxOccurs",tmpMaxO->value);
	  else
	    addToMap(tmpMaps->content,"maxOccurs",tmpMaxO->value);
	}
	map* tmpMaxMB=getMap(tmpInputs->content,"maximumMegabytes");
	if(tmpMaxMB!=NULL){
	  if(tmpMaps->content==NULL)
	    tmpMaps->content=createMap("maximumMegabytes",tmpMaxMB->value);
	  else
	    addToMap(tmpMaps->content,"maximumMegabytes",tmpMaxMB->value);
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
	      char *tmpVS=strdup(tmpV->value);
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
	      char *tcn=strdup(tmpContent->name);
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
	map* tmpCheck=getMap(tmpIoType->content,"useMapServer");
	if(tmpCheck!=NULL){
	  // Get the default value
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
#endif
      }
      if(tmpMaps->content==NULL)
	tmpMaps->content=createMap("inRequest","true");
      else
	addToMap(tmpMaps->content,"inRequest","true");

    }
    tmpInputs=tmpInputs->next;
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



