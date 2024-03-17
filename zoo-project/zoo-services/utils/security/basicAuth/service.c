/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2022 GeoLabs SARL
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
#include "apr-1.0/apr_md5.h"
#include "stdio.h"

extern "C" {

  static char* supportedType[]={
    "Basic",
    NULL
  };

  int parseBasic(maps*);

  static int (*functionType[])(maps*)={
    parseBasic,
    NULL
  };

  size_t bas64d_length(const char* pccInput){
    size_t iLen = strlen(pccInput),
      iPadding = 0;
    if (pccInput[iLen-1] == '=' && pccInput[iLen-2] == '=') //last two chars are =
      iPadding = 2;
    else if (pccInput[iLen-1] == '=') //last char is =
      iPadding = 1;
    return (((iLen+iPadding)/3)*4);
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
  char *base64d_nonl(const char *input, int length,int* red)
  {
    BIO *b64, *bmem;
    int iLen = bas64d_length(input)+1;
    char *buffer = (char *)malloc(iLen+1);
    if(buffer){
      memset(buffer, 0, iLen);
      b64 = BIO_new(BIO_f_base64());
      if(b64){
	bmem = BIO_new_mem_buf((unsigned char*)input,-1);
	bmem = BIO_push(b64, bmem);
	BIO_set_flags(bmem, BIO_FLAGS_BASE64_NO_NL);
	*red=BIO_read(bmem, buffer, length);
	buffer[length-1]=0;
	BIO_free_all(bmem);
      }
    }
    return buffer;
  }
  
  /**
   * Always failed
   *
   * @param pmsConf the main configuration maps
   * @param pmsInputs empty maps
   * @param pmsOutputs empty maps
   * @return SERVICE_SUCCEEDED in case of success, SERVICE_FAILED in case of 
   * error
   */ 
#ifdef WIN32
  __declspec(dllexport)
#endif
  int securityInFailed(maps*& pmsConf,maps*& pmsInputs,maps*& pmsOutputs){
    setMapInMaps(pmsConf,"headers","status","403 Forbidden");
    setMapInMaps(pmsConf,"lenv","code","NotAllowed");
    setMapInMaps(pmsConf,"lenv","message","Unable to ensure that you are allowed to access the resource.");
    return SERVICE_FAILED;
  }
  
  /**
   * Parse any security header using the defined osecurity scheme
   *
   * @param pmsConf the main configuration maps
   * @param pmsInputs empty maps
   * @param pmsOutputs empty maps
   * @return SERVICE_SUCCEEDED in case of success, SERVICE_FAILED in case of 
   * error
   */ 
#ifdef WIN32
  __declspec(dllexport)
#endif
  int securityIn(maps*& pmsConf,maps*& pmsInputs,maps*& pmsOutputs){
    map* pmAuthorization=NULL;
    maps* pmsSecurity=getMaps(pmsConf,"osecurity");
    map* pmSecuredUrl=getMapFromMaps(pmsConf,"lenv","secured_url");
    setMapInMaps(pmsConf,"headers","X-Also-Powered-By","Filter Security in");
    if(pmsSecurity==NULL)
      return SERVICE_SUCCEEDED;
    pmAuthorization=getMapFromMaps(pmsConf,"renv","REDIRECT_HTTP_AUTHORIZATION");
    if(pmAuthorization==NULL)
      pmAuthorization=getMapFromMaps(pmsConf,"renv","HTTP_AUTHORIZATION");
    if(pmAuthorization!=NULL){
      char* pcaAuthType=zStrdup(pmAuthorization->value);
      char* pcValue=strstr(pmAuthorization->value," ")+1;
      if(pcValue!=NULL){
	int iLen=strlen(pmAuthorization->value)-strlen(pcValue);
	pcaAuthType[iLen-1]=0;
	maps* pmsaTmp=createMaps("auth_env");
	pmsaTmp->content=createMap("type",pcaAuthType);
	addToMap(pmsaTmp->content,"value",pcValue);
	addMapsToMaps(&pmsConf,pmsaTmp);
	freeMaps(&pmsaTmp);
	free(pmsaTmp);      
	int iCnt=0;
	while(supportedType[iCnt]!=NULL){
	  if(strcasecmp(supportedType[iCnt],pcaAuthType)==0){
	    if(functionType[iCnt](pmsConf)!=0)
	      return SERVICE_FAILED;
	    break;
	  }
	  iCnt++;
	}
	free(pcaAuthType);
      }else
	return SERVICE_FAILED;
    } else{
      if(pmSecuredUrl!=NULL && strcasecmp(pmSecuredUrl->value,"true")==0){
	setMapInMaps(pmsConf,"headers","Status","401 Unauthorized");
	map* pmRealm=getMapFromMaps(pmsConf,"osecurity","realm");
	map* pmCharset=getMapFromMaps(pmsConf,"osecurity","charset");
	if(pmRealm!=NULL){
	  char* pcaHeader=NULL;
	  if(pmCharset!=NULL){
	    pcaHeader=(char*)malloc((strlen(pmCharset->value)+strlen(pmRealm->value)+27)*sizeof(char));
	    sprintf(pcaHeader,"Basic realm=\"%s\", charset=\"%s\"",pmRealm->value,pmCharset->value);
	  }else{
	    pcaHeader=(char*)malloc((strlen(pmRealm->value)+15)*sizeof(char));
	    sprintf(pcaHeader,"Basic realm=\"%s\"",pmRealm->value);
	  }
	  setMapInMaps(pmsConf,"headers","WWW-Authenticate",pcaHeader);
	  free(pcaHeader);
	}
	else 
	  setMapInMaps(pmsConf,"headers","WWW-Authenticate","Basic");
	setMapInMaps(pmsConf,"lenv","message","You are not authorized to access the resource");
	return SERVICE_FAILED;
      }
    }
    maps* pmsRenv=getMaps(pmsConf,"renv");
    return SERVICE_SUCCEEDED;
  }

  /**
   * Parse Basic authentication user / password and try to authenticate the user
   * 
   * The following fields are added in the SecurityState section in pmsConf:
   *
   * * user: the user's name
   * * password: the users' password
   * * scope: "public" in case authentication failed, "valid-user" otherwise
   *
   * @param pmsConf the main configuration maps
   * @return 0 in case of success, 1 in case of error
   */
  int parseBasic(maps* pmsConf){
    int iRed=0;
    map* pmFile=getMapFromMaps(pmsConf,"osecurity","passwd");
    map* pmValue=getMapFromMaps(pmsConf,"auth_env","value");
    char* pcaDecoded=base64d_nonl(pmValue->value,strlen(pmValue->value),&iRed);
    char* pcSaveptr;
    char *pcTmp = strtok_r (pcaDecoded, ":", &pcSaveptr);
    int iCnt=0;
    while(pcTmp!=NULL){
      if(iCnt==0)
	setMapInMaps(pmsConf,"auth_env","user",pcTmp);
      else
	setMapInMaps(pmsConf,"auth_env","password",pcTmp);
      iCnt++;
      pcTmp = strtok_r (NULL, ":", &pcSaveptr);
    }
    free(pcaDecoded);
    setMapInMaps(pmsConf,"auth_env","scope","public");
    if(pmFile!=NULL){
      FILE* pfPasswd=fopen(pmFile->value,"rb");
      if(pfPasswd!=NULL){
	char *pcLine = NULL;
	size_t iLen = 0, iSize;
	pmValue=getMapFromMaps(pmsConf,"auth_env","user");
	while ((iSize = getline(&pcLine, &iLen, pfPasswd)) != -1){
	  if(strstr(pcLine,pmValue->value)!=NULL){
	    char* pcaHash=zStrdup(strstr(pcLine,":")+1);
	    pcaHash[strlen(pcaHash)-1]=0;
	    map* pmPasswordValue=getMapFromMaps(pmsConf,"auth_env","password");
	    if(APR_SUCCESS==apr_password_validate(pmPasswordValue->value,pcaHash)){
	      setMapInMaps(pmsConf,"auth_env","scope","valid-user");
	    }else{
	      setMapInMaps(pmsConf,"headers","Status","403 Forbidden");
	      setMapInMaps(pmsConf,"lenv","code","NotAllowed");
	      setMapInMaps(pmsConf,"lenv","message","Unable to ensure that you are allowed to access the resource.");
	      return 1;
	    }
	    free(pcaHash);
	    break;
	  }
	}
	free(pcLine);
	pcLine=NULL;
	fclose(pfPasswd);
      }else{
	setMapInMaps(pmsConf,"headers","Status","403 Forbidden");
	setMapInMaps(pmsConf,"lenv","code","NotAllowed");
	setMapInMaps(pmsConf,"lenv","message","The htpasswd file cannot be read.");
	return 1;
      }
    }
    return 0;
  }

  /**
   * Parse any security header using the defined osecurity scheme
   *
   * @param pmsConf the main configuration maps
   * @param pmsInputs empty maps
   * @param pmsOutputs empty maps
   * @return SERVICE_SUCCEEDED in case of success, SERVICE_FAILED in case of 
   * error
   */ 
#ifdef WIN32
  __declspec(dllexport)
#endif
  int securityOut(maps*& pmsConf,maps*& pmsInputs,maps*& pmsOutputs){
    setMapInMaps(pmsConf,"headers","X-Also-Powered-By-1","Filter Security out");
    return SERVICE_SUCCEEDED;
  }

}
