/*
 * Author : Gérald FENOY
 *
 *  Copyright 2017-2023 GeoLabs SARL. All rights reserved.
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

#include "service_json.h"
#ifdef RELY_ON_DB
#include "sqlapi.h"
#endif
#include "json.h"
#include "json_c_version.h"
#include <errno.h>
#include "json_tokener.h"
#include "json_object.h"
#include "json.h"
#include "stdlib.h"
#include "mimetypes.h"
#include "server_internal.h"
#include "service_internal.h"
#ifdef USE_MS
#include "service_internal_ms.h"
#endif
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Convert a json object to maps
   *
   * @param pjoObj the json object to convert
   * @return the allocated maps
   */
  maps* jsonToMaps(json_object* pjoObj){
    maps* pmsRes=NULL;
    //enum json_type type;
    json_object_object_foreach(pjoObj, pcKey, pjoVal) { /*Passing through every object element*/
      json_object* pjoInput=NULL;
      if(json_object_object_get_ex(pjoObj, pcKey, &pjoInput)!=FALSE){
        map* pmTmp=jsonToMap(pjoInput);
        if(pmsRes==NULL){
          pmsRes=createMaps(pcKey);
          addMapToMap(&(pmsRes->content),pmTmp);
        }else{
          maps *pmsTmpRes=createMaps(pcKey);
          addMapToMap(&(pmsTmpRes->content),pmTmp);
          addMapsToMaps(&pmsRes,pmsTmpRes);
          freeMaps(&pmsTmpRes);
          free(pmsTmpRes);
        }
      }
    }
    return pmsRes;
  }

  /**
   * Convert a json object to map
   *
   * @param pjoObj the json object to convert
   * @return the allocated map
   */
  map* jsonToMap(json_object* pjoObj){
    map* pmRes=NULL;
    json_object_object_foreach(pjoObj, pcKey, pjoVal) {
      if(pjoVal!=NULL && json_object_is_type (pjoVal,json_type_string)){
        const char* pcVal=json_object_get_string(pjoVal);
        if(strlen(pcVal)>0){
          if(pmRes==NULL)
            pmRes=createMap(pcKey,pcVal);
          else
            addToMap(pmRes,pcKey,pcVal);
        }
      }
    }
    return pmRes;
  }

  /**
   * Trivial verification, check if a json_bool is equal to FALSE
   *
   * @param jbValue json_bool value to verify if null
   * @return bool result of the test (jbValue==FALSE)
   */
  bool jsonIsFalse(json_bool value){
    return (value==FALSE);
  }

  /**
   * Convert a map to a json object
   * @param pmMap the map to be converted into json object
   * @return a json_object pointer to the created json_object
   */
  json_object* mapToJson(map* pmMap){
    json_object *pjoRes=json_object_new_object();
    map* pmCursor=pmMap;
    map* pmSize=getMap(pmMap,"size");
    map* pmLength=getMap(pmMap,"length");
    map* pmToLoad=getMap(pmMap,"to_load");
    while(pmCursor!=NULL){
      json_object *pjoVal=NULL;
      if(pmLength==NULL && pmSize==NULL){
        if(strstr(pmCursor->name,"title")!=NULL)
          pjoVal=json_object_new_string(_(pmCursor->value));
        else
          pjoVal=json_object_new_string(pmCursor->value);
      }
      else{
        if(pmLength==NULL && pmSize!=NULL){
          if(strncasecmp(pmCursor->name,"value",5)==0){
            if(strlen(pmCursor->value)!=0 && pmToLoad!=NULL && strncasecmp(pmToLoad->value,"true",4)==0)
              pjoVal=json_object_new_string_len(pmCursor->value,atoi(pmSize->value));
          }
          else{
            if(strstr(pmCursor->name,"title")!=NULL)
              pjoVal=json_object_new_string(_(pmCursor->value));
            else
              pjoVal=json_object_new_string(pmCursor->value);
          }
        }
        else{
          pjoVal=json_object_new_string(pmCursor->value);
        }
      }
      if(pjoVal!=NULL)
        json_object_object_add(pjoRes,pmCursor->name,pjoVal);
      pmCursor=pmCursor->next;
    }
    return pjoRes;
  }

  /**
   * Convert a maps to a json object
   * @param pmMap the maps to be converted into json object
   * @return a json_object pointer to the created json_object
   */
  json_object* mapsToJson(maps* pmMap){
    json_object *pjoRes=json_object_new_object();
    maps* pmCursor=pmMap;
    while(pmCursor!=NULL){
      json_object *pjoObj=NULL;
      if(pmCursor->content!=NULL){
        pjoObj=mapToJson(pmCursor->content);
      }else
        pjoObj=json_object_new_object();
      if(pmCursor->child!=NULL){
        json_object *child=NULL;
        child=mapsToJson(pmCursor->child);
        json_object_object_add(pjoObj,"child",child);
      }
      json_object_object_add(pjoRes,pmCursor->name,pjoObj);
      pmCursor=pmCursor->next;
    }
    return pjoRes;
  }

  /**
   * Convert an elements to a json object
   * @param peElements the elements pointer to be converted into a json object
   * @return a json_object pointer to the created json_object
   */
  json_object* elementsToJson(elements* peElements){
    json_object *pjoRes=json_object_new_object();
    elements* peCursor=peElements;
    while(peCursor!=NULL){
      json_object *pjoCres=json_object_new_object();
      json_object_object_add(pjoCres,"content",mapToJson(peCursor->content));
      json_object_object_add(pjoCres,"metadata",mapToJson(peCursor->metadata));
      json_object_object_add(pjoCres,"additional_parameters",mapToJson(peCursor->additional_parameters));
      if(peCursor->format!=NULL){
        json_object_object_add(pjoCres,"format",json_object_new_string(peCursor->format));
      }
      if(peCursor->child==NULL){
        if(peCursor->defaults!=NULL)
          json_object_object_add(pjoCres,"defaults",mapToJson(peCursor->defaults->content));
        else
          json_object_object_add(pjoCres,"defaults",mapToJson(NULL));
        iotype* piotCur=peCursor->supported;
        json_object *pjoResi=json_object_new_array();
        while(piotCur!=NULL){
          json_object_array_add(pjoResi,mapToJson(piotCur->content));
          piotCur=piotCur->next;
        }
        json_object_object_add(pjoCres,"supported",pjoResi);
      }
      
      json_object_object_add(pjoCres,"child",elementsToJson(peCursor->child));

      json_object_object_add(pjoRes,peCursor->name,pjoCres);
      peCursor=peCursor->next;
    }
    return pjoRes;
  }
  
  /**
   * Convert an service to a json object
   *
   * @param psService the service pointer to be converted into a json object
   * @return a json_object pointer to the created json_object
   */
  json_object* serviceToJson(service* psService){
    json_object *pjoRes=json_object_new_object();
    json_object_object_add(pjoRes,"name",json_object_new_string(psService->name));
    json_object_object_add(pjoRes,"content",mapToJson(psService->content));
    json_object_object_add(pjoRes,"metadata",mapToJson(psService->metadata));
    json_object_object_add(pjoRes,"additional_parameters",mapToJson(psService->additional_parameters));
    json_object_object_add(pjoRes,"inputs",elementsToJson(psService->inputs));
    json_object_object_add(pjoRes,"outputs",elementsToJson(psService->outputs));
    return pjoRes;
  }

  /**
   * Add a description to one or two json objects using the abstract_file or
   * abstract value. In case an abstract_file is found, then the ZOO-Kernel will
   * try to read the provided file path and use its content as the description
   * value.
   *
   * @param pmElement the current map to search for abstract_file/abstract field
   * @param iIndex the index to use to fetch from the map array
   * @param pjoOutput the first json object to add the description
   * @param pjoOutput1 the second json object to add the description (optional)
   */
  void addDescription(map* pmElement,int iIndex,json_object* pjoOutput,json_object* pjoOutput1){
    if(pmElement==NULL)
      return ;
    map* pmCurrent=getMapArray(pmElement,"abstract_file",iIndex);
    if(pmCurrent!=NULL){
      FILE* pfData=fopen(pmCurrent->value,"rb");
      if(pfData!=NULL){
        zStatStruct f_status;
        int iStat=zStat(pmCurrent->value, &f_status);
        if(f_status.st_size>0){
          char* pcaTmp=(char*)malloc((f_status.st_size+1)*sizeof(char));
          size_t sLength = fread(pcaTmp,1,f_status.st_size,pfData);
          pcaTmp[f_status.st_size]=0;
          fclose(pfData);
          json_object_object_add(pjoOutput,"description",json_object_new_string(_(pcaTmp)));
          if(pjoOutput1!=NULL)
            json_object_object_add(pjoOutput1,"description",json_object_new_string(_(pcaTmp)));
          free(pcaTmp);
        }
      }else{
        json_object_object_add(pjoOutput,"description",json_object_new_string(_("Unable to load your file")));
        if(pjoOutput1!=NULL)
          json_object_object_add(pjoOutput1,"description",json_object_new_string(_("Unable to load your file")));
      }
    }else{
      pmCurrent=getMapArray(pmElement,"abstract",iIndex);
      if(pmCurrent!=NULL){
        json_object_object_add(pjoOutput,"description",json_object_new_string(_(pmCurrent->value)));
        if(pjoOutput1!=NULL)
          json_object_object_add(pjoOutput1,"description",json_object_new_string(_(pmCurrent->value)));
      }
    }
  }

  /**
   * Add Allowed Range properties to a json_object
   *
   * @param pmsConf the main configuration maps pointer
   * @param piotType the iotype pointer
   * @param pjoProp the json_object pointer to add the allowed range properties
   * @return a json_object pointer to the created json_object
   */
  void printAllowedRangesJ(maps* pmsConf,iotype* piotType,json_object* pjoProp){
    map* pmTmp;
    map* pmType=getMap(piotType->content,"DataType");
    int iHasMin=0;
    int iHasMax=0;
    for(int iCounter=0;iCounter<4;iCounter++){
      pmTmp=getMap(piotType->content,rangeCorrespondances[iCounter][0]);
      if(pmTmp!=NULL){
        if(iCounter>=1 && ( (iHasMin==0 && iCounter==1) || (iHasMax==0 && iCounter==2)) ){
          map* pmaTmp=createMap("value",pmTmp->value);
          printLiteralValueJ(pmsConf,pmType,pmaTmp,pjoProp,rangeCorrespondances[iCounter][1]);
          freeMap(&pmaTmp);
          free(pmaTmp);
        }
        else{
          char* currentValue=NULL;
          map* pmTmp1=getMap(piotType->content,rangeCorrespondances[1][0]);
          if(pmTmp->value[0]=='o' && pmTmp1!=NULL){
            iHasMin=1;
            map* pmaTmp=createMap("value",pmTmp1->value);
            printLiteralValueJ(pmsConf,pmType,pmaTmp,pjoProp,"exclusiveMinimum");
            freeMap(&pmaTmp);
            free(pmaTmp);
            if(strlen(pmTmp->value)==1){
              pmTmp1=getMap(piotType->content,rangeCorrespondances[2][0]);
              if(pmTmp1!=NULL){
                iHasMax=1;
                map* pmaTmp1=createMap("value",pmTmp1->value);
                printLiteralValueJ(pmsConf,pmType,pmaTmp1,pjoProp,"exclusiveMaximum");
                freeMap(&pmaTmp1);
                free(pmaTmp1);
              }
            }
          }
          if(strlen(pmTmp->value)>1 && pmTmp->value[1]=='o'){
            pmTmp1=getMap(piotType->content,rangeCorrespondances[2][0]);
            if(pmTmp1!=NULL){
              iHasMax=1;
              map* pmaTmp=createMap("value",pmTmp1->value);
              printLiteralValueJ(pmsConf,pmType,pmaTmp,pjoProp,"exclusiveMaximum");
              freeMap(&pmaTmp);
              free(pmaTmp);
            }
          }
        }
      }
    }
  }

  /**
   * Add literal property depending on the dataType
   *
   * @param pmConf the main configuration maps pointer
   * @param pmType the dataType map pointer
   * @param pmContent the iotype content map pointer
   * @param pjoSchema the json_object pointer to add the value
   * @param pccFieldName the field name string
   */
  void printLiteralValueJ(maps* pmConf,map* pmType,map* pmContent,json_object* pjoSchema,const char* pccFieldName){
    map* pmTmp=getMap(pmContent,"value");
    map* pmLength=getMap(pmContent,"length");
    if(pmLength!=NULL){
      int iLen=atoi(pmLength->value);
      json_object* defaultProp=json_object_new_array();
      for(int iCounter=0;iCounter<iLen;iCounter++){
        map* pmTmp0=getMapArray(pmContent,"value",iCounter);
        if(pmTmp0!=NULL){
          if(pmType!=NULL && strncasecmp(pmType->value,"integer",7)==0)
            json_object_array_add(defaultProp,json_object_new_int(atoi(pmTmp0->value)));
          else{
            if(pmType!=NULL && (strncasecmp(pmType->value,"float",5)==0 || strncasecmp(pmType->value,"double",6)==0) ){
              json_object_array_add(defaultProp,json_object_new_double(atof(pmTmp0->value)));
              if(strncasecmp(pmType->value,"float",5)==0)
                json_object_object_add(pjoSchema,"format",json_object_new_string("float"));
              else
                json_object_object_add(pjoSchema,"format",json_object_new_string("double"));
            }
            else{
              if(pmType!=NULL && strncasecmp(pmType->value,"bool",4)==0){
                if(strncasecmp(pmTmp0->value,"true",4)==0)
                  json_object_array_add(defaultProp,json_object_new_boolean(true));
                else
                  json_object_array_add(defaultProp,json_object_new_boolean(false));
              }
              else
                json_object_array_add(defaultProp,json_object_new_string(pmTmp0->value));
            }
          }
        }else{
          if(pmType!=NULL && (strncasecmp(pmType->value,"float",5)==0 || strncasecmp(pmType->value,"double",6)==0) ){
            if(strncasecmp(pmType->value,"float",5)==0)
              json_object_object_add(pjoSchema,"format",json_object_new_string("float"));
            else
              json_object_object_add(pjoSchema,"format",json_object_new_string("double"));
          }
        }
      }
      json_object_object_add(pjoSchema,pccFieldName,defaultProp);
    }
    else if(pmTmp!=NULL){
      if((strstr(pmTmp->value,"[")!=NULL && strstr(pmTmp->value,"]")!=NULL) ||
        (strstr(pmTmp->value,"{")!=NULL && strstr(pmTmp->value,"}")!=NULL)){
        json_object* defaultProp=parseJson(pmConf,pmTmp->value);
        json_object_object_add(pjoSchema,pccFieldName,defaultProp);
      }
      else
        if(pmType!=NULL && strncasecmp(pmType->value,"integer",7)==0)
          json_object_object_add(pjoSchema,pccFieldName,json_object_new_int(atoi(pmTmp->value)));
        else{
          if(pmType!=NULL && (strncasecmp(pmType->value,"float",5)==0 || strncasecmp(pmType->value,"double",6)==0) ){
            json_object_object_add(pjoSchema,pccFieldName,json_object_new_double(atof(pmTmp->value)));
            if(strncasecmp(pmType->value,"float",5)==0)
              json_object_object_add(pjoSchema,"format",json_object_new_string("float"));
            else
              json_object_object_add(pjoSchema,"format",json_object_new_string("double"));
          }
          else{
            if(pmType!=NULL && strncasecmp(pmType->value,"bool",4)==0){
              if(strncasecmp(pmTmp->value,"true",4)==0)
                json_object_object_add(pjoSchema,pccFieldName,json_object_new_boolean(true));
              else
                json_object_object_add(pjoSchema,pccFieldName,json_object_new_boolean(false));
            }
            else{
              json_object_object_add(pjoSchema,pccFieldName,json_object_new_string(pmTmp->value));
            }
          }
        }
    }else{
      if(pmType!=NULL && (strncasecmp(pmType->value,"float",5)==0 || strncasecmp(pmType->value,"double",6)==0) ){
        if(strncasecmp(pmType->value,"float",5)==0)
          json_object_object_add(pjoSchema,"format",json_object_new_string("float"));
        else
          json_object_object_add(pjoSchema,"format",json_object_new_string("double"));
      }
    }
  }
  
  /**
   * Add literalDataDomains property to a json_object
   *
   * @param pmsConf the main configuration maps pointer
   * @param peInput the elements pointer
   * @param pjoInput the json_object pointer to add the literalDataDomains property
   * @return a json_object pointer to the created json_object
   */
  void printLiteralDataJ(maps* pmsConf,elements* peInput,json_object* pjoInput){
    json_object* pjoSchema=json_object_new_object();
    map* pmMin=getMap(peInput->content,"minOccurs");
    map* pmMax=getMap(peInput->content,"maxOccurs");
    if(peInput->defaults!=NULL){
      map* pmTmp=getMap(peInput->defaults->content,"DataType");
      if(pmTmp!=NULL){
        if(strncasecmp(pmTmp->value,"float",5)==0 || strncasecmp(pmTmp->value,"double",6)==0)
          json_object_object_add(pjoSchema,"type",json_object_new_string("number"));
        else
          if(strncasecmp(pmTmp->value,"bool",5)==0)
            json_object_object_add(pjoSchema,"type",json_object_new_string("boolean"));
          else
            if(strstr(pmTmp->value,"date")!=NULL){
              json_object_object_add(pjoSchema,"type",json_object_new_string("string"));
              json_object_object_add(pjoSchema,"format",json_object_new_string(pmTmp->value));
            }
            else
              json_object_object_add(pjoSchema,"type",json_object_new_string(pmTmp->value));
      }
      printLiteralValueJ(pmsConf,pmTmp,peInput->defaults->content,pjoSchema,"default");
      pmTmp=getMap(peInput->defaults->content,"rangeMin");
      if(pmTmp!=NULL){
        printAllowedRangesJ(pmsConf,peInput->defaults,pjoSchema);
        if(peInput->supported!=NULL){
          iotype* piotType=peInput->supported;
          while(piotType!=NULL){
            printAllowedRangesJ(pmsConf,piotType,pjoSchema);
            piotType=piotType->next;
          }
        }
      }
      else{
        pmTmp=getMap(peInput->defaults->content,"range");
        if(pmTmp!=NULL){
          // TODO: parse range = [rangeMin,rangeMax] (require generic function)
        }else{
          // AllowedValues
          pmTmp=getMap(peInput->defaults->content,"AllowedValues");
          if(pmTmp!=NULL){
            char* pcSavePtr;
            json_object* prop5=json_object_new_array();
            char *pcTmp = strtok_r (pmTmp->value, ",", &pcSavePtr);
            while(pcTmp!=NULL){
              json_object_array_add(prop5,json_object_new_string(pcTmp));
              pcTmp = strtok_r (NULL, ",", &pcSavePtr);
            }
            json_object_object_add(pjoSchema,"enum",prop5);
          }
        }
      }
    }
    if(pmMin!=NULL && atoi(pmMin->value)==0)
      json_object_object_add(pjoSchema,"nullable",json_object_new_boolean(true));
#if JSON_C_MINOR_VERSION >= 13
    // Add experimental support for array of literals using extended-schema
    if(pmMax!=NULL && atoi(pmMax->value)>1 && pmMin!=NULL){
      json_object* pjoSchema1=json_object_new_object();
      json_object_object_add(pjoSchema1,"type",json_object_new_string("array"));
      json_object* pjoSchema2=NULL;
      json_object_deep_copy(pjoSchema,&pjoSchema2,NULL);
      json_object_object_add(pjoSchema1,"items",pjoSchema2);
      json_object_object_add(pjoSchema1,"minItems",json_object_new_int(atoi(pmMin->value)));
      json_object_object_add(pjoSchema1,"maxItems",json_object_new_int(atoi(pmMax->value)));
      json_object_object_add(pjoInput,"extended-schema",pjoSchema1);
    }
#endif
    json_object_object_add(pjoInput,"schema",pjoSchema);
  }

  /**
   * Add support in the extended-schema for links object
   * 
   * @param pmsConf the main configuration maps pointer
   * @param peElement the elements pointer
   * @param pjoRes the json_object pointer on the array
   * @param isDefault boolean, try if it is the default value
   * @param pmMaxSize map pointer (not used)
   */
  void printComplexHref(maps* pmsConf,elements* peElement,json_object* pjoRes,bool isDefault,map* pmMaxSize){
    if(peElement!=NULL && peElement->defaults!=NULL){
      json_object* prop=json_object_new_object();
      json_object* prop1=json_object_new_array();
      json_object* prop2=json_object_new_object();
      map* pmTmp0=getMapFromMaps(pmsConf,"openapi","link_href");
      if(pmTmp0!=NULL)
        json_object_object_add(prop2,"$ref",json_object_new_string(pmTmp0->value));
      json_object_array_add(prop1,prop2);
      json_object* prop3=json_object_new_object();
      json_object_object_add(prop3,"type",json_object_new_string("object"));
      json_object* prop4=json_object_new_object();
      json_object* prop5=json_object_new_array();
      map* pmTmp2=getMap(peElement->defaults->content,"mimeType");
      if(pmTmp2!=NULL)
        json_object_array_add(prop5,json_object_new_string(pmTmp2->value));
      iotype* piotSupported=peElement->supported;
      while(piotSupported!=NULL){
        pmTmp2=getMap(piotSupported->content,"mimeType");
        if(pmTmp2!=NULL)
          json_object_array_add(prop5,json_object_new_string(pmTmp2->value));
        piotSupported=piotSupported->next;
      }
      json_object_object_add(prop4,"enum",prop5);
      json_object* prop6=json_object_new_object();
      json_object_object_add(prop6,"type",prop4);
      json_object_object_add(prop3,"properties",prop6);
      json_object_array_add(prop1,prop3);
      json_object_object_add(prop,"allOf",prop1);
      json_object_array_add(pjoRes,prop);
    }
  }

  /**
   * Add support for type array in extended-schema
   *
   *
   * @param m the main configuration maps pointer
   * @param peElement the elements pointer
   */
  json_object* addArray(maps* m,elements* peElement){
    json_object* pjoRes=json_object_new_object();
    json_object_object_add(pjoRes,"type",json_object_new_string("array"));
    map* pmTmp=getMap(peElement->content,"minOccurs");
    if(pmTmp!=NULL /*&& strcmp(pmTmp->value,"1")!=0*/){
      json_object_object_add(pjoRes,"minItems",json_object_new_int(atoi(pmTmp->value)));
    }else{
      json_object_put(pjoRes);
      pjoRes=NULL;
      return pjoRes;
    }
    pmTmp=getMap(peElement->content,"maxOccurs");
    if(pmTmp!=NULL){
      if(atoi(pmTmp->value)>1){
        json_object_object_add(pjoRes,"maxItems",json_object_new_int(atoi(pmTmp->value)));
        return pjoRes;
      }
      else{
        json_object_put(pjoRes);
        pjoRes=NULL;
        return pjoRes;
      }
    }else
      return pjoRes;
  }

  /**
   * Get the data format definition from the schemas section, return 1 if found.
   *
   * @param pmsConf the main configuration maps pointer
   * @param pccName the name of the data format to search for
   * @return 1 if found, 0 otherwise
   */
  int getDefinitionFromSchemas(maps* pmsConf,const char* pccName,json_object* pjoInput){
    maps* pmsTmp=getMaps(pmsConf,"schemas");
    if(pmsTmp==NULL)
      return 0;
    map* pmLength=getMap(pmsTmp->content,"length");
    int iLen=1;
    if(pmLength!=NULL)
      iLen=atoi(pmLength->value);
    for(int iCnt=0;iCnt<iLen;iCnt++){
      map* pmTmp=getMapArray(pmsTmp->content,"name",iCnt);
      if(pmTmp!=NULL && strncasecmp(pmTmp->value,pccName,strlen(pmTmp->value))==0){
        json_object* pjoFinal=json_object_new_object();
        json_object* pjoAllOf=json_object_new_array();
        json_object* pjoFormat=json_object_new_object();
        json_object_object_add(pjoFormat,"format",json_object_new_string(pccName));
        json_object* pjoRef=json_object_new_object();
        pmTmp=getMapArray(pmsTmp->content,"value",iCnt);
        if(pmTmp!=NULL)
          json_object_object_add(pjoRef,"$ref",json_object_new_string(pmTmp->value));
        json_object_array_add(pjoAllOf,pjoFormat);
        json_object_array_add(pjoAllOf,pjoRef);
        json_object_object_add(pjoFinal,"allOf",pjoAllOf);
        json_object_object_add(pjoInput,"schema",pjoFinal);
        return 1;
      }
    }
    return 0;
  }

  /**
   * Add basic schema definition for the BoundingBox type
   * 
   * @param pmsConf the main configuration maps pointer
   * @param peInput the elements pointing to an input/output
   * @param pjoInput the json_object pointer used to store the schema definition
   */
  void printBoundingBoxJ(maps* pmsConf,elements* peInput,json_object* pjoInput){
    map* pmMin=getMap(peInput->content,"minOccurs");
    if(getDefinitionFromSchemas(pmsConf,"ogc-bbox",pjoInput)>0)
      return;

    json_object* pjoFinal=json_object_new_object();
    json_object_object_add(pjoFinal,"type",json_object_new_string("object"));

    json_object* pjoReqiredProperties=json_object_new_array();
    json_object_array_add(pjoReqiredProperties,json_object_new_string("bbox"));
    json_object_array_add(pjoReqiredProperties,json_object_new_string("crs"));
    json_object_object_add(pjoFinal,"required",pjoReqiredProperties);

    json_object* pjoProperties=json_object_new_object();

    json_object* pjoBBoxProperty=json_object_new_object();
    json_object_object_add(pjoBBoxProperty,"type",json_object_new_string("array"));

    json_object* pjoMinMax4=json_object_new_object();
    json_object_object_add(pjoMinMax4,"minItems",json_object_new_int(4));
    json_object_object_add(pjoMinMax4,"maxItems",json_object_new_int(4));

    json_object* pjoMinMax6=json_object_new_object();
    json_object_object_add(pjoMinMax6,"minItems",json_object_new_int(6));
    json_object_object_add(pjoMinMax6,"maxItems",json_object_new_int(6));

    json_object* pjoOneOf=json_object_new_array();
    json_object_array_add(pjoOneOf,pjoMinMax4);
    json_object_array_add(pjoOneOf,pjoMinMax6);

    json_object_object_add(pjoBBoxProperty,"oneOf",pjoOneOf);

    json_object* pjoItems=json_object_new_object();
    json_object_object_add(pjoItems,"type",json_object_new_string("number"));
    json_object_object_add(pjoItems,"format",json_object_new_string("double"));
    json_object_object_add(pjoBBoxProperty,"items",pjoItems);

    json_object_object_add(pjoProperties,"bbox",pjoBBoxProperty);

    json_object* pjoCrs=json_object_new_object();
    json_object_object_add(pjoCrs,"type",json_object_new_string("string"));
    json_object_object_add(pjoCrs,"format",json_object_new_string("uri"));

    if(peInput->defaults!=NULL){
      map* pmTmp=getMap(peInput->defaults->content,"crs");
      if(pmTmp!=NULL){
        json_object* pjoSupportedCRS=json_object_new_array();
        json_object_array_add(pjoSupportedCRS,json_object_new_string(pmTmp->value));
        json_object_object_add(pjoCrs,"default",json_object_new_string(pmTmp->value));
        iotype* piotSupported=peInput->supported;
        while(piotSupported!=NULL){
          pmTmp=getMap(piotSupported->content,"crs");
          if(pmTmp!=NULL)
            json_object_array_add(pjoSupportedCRS,json_object_new_string(pmTmp->value));
          piotSupported=piotSupported->next;
        }
        json_object_object_add(pjoCrs,"enum",pjoSupportedCRS);
      }
    }

    json_object_object_add(pjoProperties,"crs",pjoCrs);

    json_object_object_add(pjoFinal,"properties",pjoProperties);
    if(pmMin!=NULL && atoi(pmMin->value)==0)
      json_object_object_add(pjoFinal,"nullable",json_object_new_boolean(true));

    json_object_object_add(pjoFinal,"format",json_object_new_string("ogc-bbox"));

    json_object_object_add(pjoInput,"schema",pjoFinal);
  }

  /**
   * Add schema property to a json_object (ComplexData)
   * @param m the main configuration maps pointer
   * @param piotType the current iotype pointer
   * @param pjoRes the json_object pointer to add the properties to
   * @param isDefault boolean specifying if the currrent iotype is default
   * @param pmMaxSize a map pointer to the maximumMegabytes param defined in the zcfg file for this input/output
   */
  void printFormatJ1(maps* m,iotype* piotType,json_object* pjoRes,bool isDefault,map* pmMaxSize){
    if(piotType!=NULL){
      int i=0;
      json_object* pjoMetadata=json_object_new_object();
      map* pmTmp=getMap(piotType->content,"encoding");
      map* pmTmp2=getMap(piotType->content,"mimeType");
      map* pmTmp3=getMap(piotType->content,"schema");
      if(pmTmp2!=NULL && (strncasecmp(pmTmp2->value,"application/json",16)==0 || strstr(pmTmp2->value,"json")!=NULL)){
        json_object_object_add(pjoMetadata,"type",json_object_new_string("object"));
        if(pmTmp3!=NULL)
          json_object_object_add(pjoMetadata,"$ref",json_object_new_string(pmTmp3->value));
        if((pmTmp2=getMap(piotType->content,"dataFormat"))!=NULL){
          json_object_object_add(pjoMetadata,"format",json_object_new_string(pmTmp2->value));
        }
        json_object_array_add(pjoRes,pjoMetadata);
        return ;
      }
      //json_object* prop2=json_object_new_object();
      json_object_object_add(pjoMetadata,"type",json_object_new_string("string"));
      if(pmTmp!=NULL)
        json_object_object_add(pjoMetadata,"contentEncoding",json_object_new_string(pmTmp->value));
      else{
        json_object_object_add(pjoMetadata,"contentEncoding",json_object_new_string("base64"));
        i=1;
      }
      if(pmTmp2!=NULL){
        json_object_object_add(pjoMetadata,"contentMediaType",json_object_new_string(pmTmp2->value));
      }
      // TODO: specific handling of schema?!
      if(pmTmp3!=NULL)
        json_object_object_add(pjoMetadata,"contentSchema",json_object_new_string(pmTmp3->value));
      if(pmMaxSize!=NULL)
        json_object_object_add(pjoMetadata,"contentMaximumMegabytes",json_object_new_int64(atoll(pmMaxSize->value)));
      json_object_array_add(pjoRes,pjoMetadata);
    }
  }
  
  /**
   * Add format properties to a json_object
   * @param m the main configuration maps pointer
   * @param piotType the current iotype pointer
   * @param pjoRes the json_object pointer to add the properties to
   * @param isDefault boolean specifying if the currrent iotype is default
   * @param pmMaxSize a map pointer to the maximumMegabytes param defined in the zcfg file for this input/output
   */
  void printFormatJ(maps* m,iotype* piotType,json_object* pjoRes,bool isDefault,map* pmMaxSize){
    if(piotType!=NULL){
      map* pmTmp=getMap(piotType->content,"mimeType");
      json_object* prop1=json_object_new_object();
      json_object_object_add(prop1,"default",json_object_new_boolean(isDefault));
      json_object_object_add(prop1,"mediaType",json_object_new_string(pmTmp->value));
      pmTmp=getMap(piotType->content,"encoding");
      if(pmTmp!=NULL)
        json_object_object_add(prop1,"characterEncoding",json_object_new_string(pmTmp->value));
      pmTmp=getMap(piotType->content,"schema");
      if(pmTmp!=NULL)
        json_object_object_add(prop1,"schema",json_object_new_string(pmTmp->value));
      if(pmMaxSize!=NULL)
        json_object_object_add(prop1,"maximumMegabytes",json_object_new_int64(atoll(pmMaxSize->value)));
      json_object_array_add(pjoRes,prop1);
    }
  }

    /**
   * Add additionalParameters property to a json_object
   * @param pmsConf the main configuration maps pointer
   * @param pmMetaadta a map pointer to the current metadata informations
   * @param doc the json_object pointer to add the property to
   */
  void printJAdditionalParameters(maps* pmsConf,map* pmMetaadta,json_object* doc){
    map* cmeta=pmMetaadta;
    json_object* carr=json_object_new_array();
    int hasElement=-1;
    json_object* jcaps=json_object_new_object();
    while(cmeta!=NULL){
      json_object* jcmeta=json_object_new_object();
      if(strcasecmp(cmeta->name,"role")!=0 &&
          strcasecmp(cmeta->name,"href")!=0 &&
          strcasecmp(cmeta->name,"title")!=0 &&
          strcasecmp(cmeta->name,"length")!=0 ){
        json_object_object_add(jcmeta,"name",json_object_new_string(cmeta->name));
        json_object* pjoValueArray=json_object_new_array();
        if(strcasecmp(cmeta->name,"title")!=0)
          json_object_array_add(pjoValueArray,json_object_new_string(_(cmeta->value)));
        else
          json_object_array_add(pjoValueArray,json_object_new_string(cmeta->value));
        json_object_object_add(jcmeta,"value",pjoValueArray);
        json_object_array_add(carr,jcmeta);
        hasElement++;
      }else{
        if(strcasecmp(cmeta->name,"length")!=0)
          json_object_object_add(jcaps,cmeta->name,json_object_new_string(cmeta->value));
      }
      cmeta=cmeta->next;
    }
    if(hasElement>=0){
      json_object_object_add(jcaps,"parameters",carr);
      json_object_object_add(doc,"additionalParameters",jcaps);
    }else{
      json_object_put(carr);
      json_object_put(jcaps);
    }
  }

  /**
   * Add metadata property to a json_object
   * @param pmsConf the main configuration maps pointer
   * @param meta a map pointer to the current metadata informations
   * @param doc the json_object pointer to add the property to
   */
  void printJMetadata(maps* pmsConf,map* meta,json_object* doc){
    map* cmeta=meta;
    json_object* carr=json_object_new_array();
    int hasElement=-1;
    int hasKey=-1;
    int iCnt=0;
    int iLen=1;
    if(cmeta==NULL)
      return;
    map* pmLength=getMap(cmeta,"length");
    if(pmLength!=NULL)
      iLen=atoi(pmLength->value);
    char* apcFields[]={
      (char*)"role",
      (char*)"href",
      (char*)"title"
    };
    for(;iCnt<iLen;iCnt++){
      json_object* pjoMeta=json_object_new_object();
      int iCnt1=0;
      hasKey=-1;
      for(;iCnt1<3;iCnt1++){
        if(pjoMeta==NULL){
          pjoMeta=json_object_new_object();
          hasKey=-1;
        }
        map* pmValue=getMapArray(meta,apcFields[iCnt1],iCnt);
        if(pmValue!=NULL){
          if(strcasecmp(pmValue->name,"title")==0)
            json_object_object_add(pjoMeta,apcFields[iCnt1],json_object_new_string(_(pmValue->value)));
          else
            if(iCnt1==1)
              json_object_object_add(pjoMeta,"value",json_object_new_string(pmValue->value));
            else
              json_object_object_add(pjoMeta,apcFields[iCnt1],json_object_new_string(pmValue->value));
          hasElement++;
          hasKey++;
        }else{
          if(iCnt1==1){
            char* pcaPrefix=(char*)malloc(17*sizeof(char));
            sprintf(pcaPrefix,"role_%d",iCnt);
            char* pcaELength=(char*)malloc(25*sizeof(char));
            sprintf(pcaELength,"%s_elength",pcaPrefix);
            map* pmELength=getMap(cmeta,pcaELength);
            int iELength=1;
            if(pmELength!=NULL){
              iELength=atoi(pmELength->value);
            }
            int iCnt4=0;
            for(;iCnt4<iELength;iCnt4++){
              if(pjoMeta==NULL){
                pjoMeta=json_object_new_object();
                size_t sLen=json_object_array_length(carr);
                json_object* pjoPrevious=json_object_array_get_idx(carr,sLen-1);
                if(pjoPrevious!=NULL){
                  json_object* pjoRole=NULL;
                  if(json_object_object_get_ex(pjoPrevious,"role",&pjoRole)!=FALSE){
                    json_object_object_add(pjoMeta,"role",json_object_new_string(json_object_get_string(pjoRole)));
                  }
                }
              }
              if(iCnt4>0){
                char* pcaTmp=zStrdup(pcaPrefix);
                if(iCnt4==1){
                  pcaPrefix=(char*)realloc(pcaPrefix,(12+strlen(pcaTmp))*sizeof(char));
                  sprintf(pcaPrefix,"%s_%d",pcaTmp,iCnt4);
                }else
                  sprintf(pcaPrefix,"%s_%d",pcaTmp,iCnt4);
                free(pcaTmp);
              }
              char* pcaLength=(char*)malloc((9+strlen(pcaPrefix))*sizeof(char));
              sprintf(pcaLength,"%s_length",pcaPrefix);
              map* pmLength=getMap(cmeta,pcaLength);
              free(pcaLength);
              if(pmLength!=NULL){
                json_object* innerElement=json_object_new_object();
                int iLength1=atoi(pmLength->value);
                int iCnt2=0;
                for(;iCnt2<iLength1;iCnt2++){
                  int iCnt3=0;
                  char* pcaNameValue[2];
                  for(;iCnt3<2;iCnt3++){
                    char* pcaName=(char*)malloc(20*sizeof(char));
                    sprintf(pcaName,"%s_%s",pcaPrefix,apcFields[iCnt3]);
                    map* pmRole=getMapArray(meta,pcaName,iCnt2);
                    if(pmRole!=NULL){
                      pcaNameValue[iCnt3]=zStrdup(pmRole->value);
                      free(pcaName);
                    }
                  }
                  if(pcaNameValue[0]!=NULL && pcaNameValue[1]!=NULL){
                    if(pcaNameValue[1][0]=='{' || pcaNameValue[1][0]=='['){
                      json_object* pjoTmp=parseJson(pmsConf,pcaNameValue[1]);
                      if(pjoTmp!=NULL)
                        json_object_object_add(innerElement,pcaNameValue[0],pjoTmp);
                    }
                    else
                      json_object_object_add(innerElement,pcaNameValue[0],json_object_new_string(pcaNameValue[1]));
                    hasElement++;
                  }
                  if(pcaNameValue[0]!=NULL)
                    free(pcaNameValue[0]);
                  if(pcaNameValue[1]!=NULL)
                    free(pcaNameValue[1]);
                  char* pcaName=(char*)malloc(17*sizeof(char));
                  sprintf(pcaName,"%s_title",pcaPrefix);
                  map* pmRole=getMapArray(meta,pcaName,iCnt2);
                  if(pmRole!=NULL){
                    json_object_object_add(innerElement,apcFields[2],json_object_new_string(pmRole->value));
                  }
                  free(pcaName);
                }
                json_object_object_add(pjoMeta,"value",innerElement);
#if JSON_C_MINOR_VERSION >= 13
                json_object* pjoMetaTmp=NULL;
                json_object_deep_copy(pjoMeta,&pjoMetaTmp,NULL);
                json_object_array_add(carr,pjoMetaTmp);
                json_object_put(pjoMeta);
#endif
                pjoMeta=NULL;
                hasKey++;
              }
            }
            free(pcaPrefix);
          }
        }
      }
      if(pjoMeta!=NULL && hasKey>=0)
        json_object_array_add(carr,pjoMeta);
    }
    if(hasElement>=0){
      json_object_object_add(doc,"metadata",carr);
    }
    else
      json_object_put(carr);
  }

  /**
   * Add metadata properties to a json_object
   *
   * @param pmsConf the main configuration maps pointer
   * @param pccIo a string "input" or "output" 
   * @param peInput an elements pointer to the current input/output
   * @param pjoInputs the json_object pointer to add the property to
   * @param psServ the service pointer to extract the metadata from
   */
  void printIOTypeJ(maps* pmsConf, const char *pccIo, elements* peInput,json_object* pjoInputs,service* psServ){
    while(peInput!=NULL){
      json_object* pjoInput=json_object_new_object();
      map* pmTitle=getMap(peInput->content,"title");
      map* pmMin=NULL;
      map* pmMax=NULL;
      if(strcmp(pccIo,"input")==0){
        pmMin=getMap(peInput->content,"minOccurs");
        pmMax=getMap(peInput->content,"maxOccurs");
      }
      if(pmTitle!=NULL)
        json_object_object_add(pjoInput,"title",json_object_new_string(_ss(pmTitle->value)));
      addDescription(peInput->content,0,pjoInput,NULL);
      if(strcmp(pccIo,"input")==0){
        if(pmMin!=NULL && strcmp(pmMin->value,"1")!=0)
          json_object_object_add(pjoInput,"minOccurs",json_object_new_int(atoi(pmMin->value)));
        if(pmMax!=NULL){
          if(strncasecmp(pmMax->value,"unbounded",9)==0)
            json_object_object_add(pjoInput,"maxOccurs",json_object_new_string(pmMax->value));
          else
            if(strcmp(pmMax->value,"1")!=0)
              json_object_object_add(pjoInput,"maxOccurs",json_object_new_int(atoi(pmMax->value)));
        }
      }
      if(peInput->format!=NULL){
        //json_object* input1=json_object_new_object();
        json_object* pjoProp0=json_object_new_array();
        json_object* pjoProp1=json_object_new_array();
        json_object* pjoSchema=json_object_new_array();
        if(strcasecmp(peInput->format,"LiteralData")==0 ||
            strcasecmp(peInput->format,"LiteralOutput")==0){
          printLiteralDataJ(pmsConf,peInput,pjoInput);
          json_object_put(pjoProp0);
          json_object_put(pjoProp1);
          json_object_put(pjoSchema);
        }else{
          if(strcasecmp(peInput->format,"ComplexData")==0 ||
              strcasecmp(peInput->format,"ComplexOutput")==0) {
            map* pmSize=getMap(peInput->content,"maximumMegabytes");
            printComplexHref(pmsConf,peInput,pjoProp1,false,pmSize);
            printFormatJ(pmsConf,peInput->defaults,pjoProp0,true,pmSize);
            json_object* pjoPredefinedSchema=json_object_new_object();
            map* pmTmp=getMap(peInput->defaults->content,"dataFormat");
            if(pmTmp!=NULL && getDefinitionFromSchemas(pmsConf,pmTmp->value,pjoInput)>0){
              json_object_put(pjoSchema);
            }else{
              json_object* pjoSchemaPart=json_object_new_array();
              printFormatJ1(pmsConf,peInput->defaults,pjoSchemaPart,true,pmSize);

              printFormatJ1(pmsConf,peInput->defaults,pjoSchema,true,pmSize);
              iotype* piotSupported=peInput->supported;
              while(piotSupported!=NULL){
                printFormatJ(pmsConf,piotSupported,pjoProp0,false,pmSize);
                printFormatJ1(pmsConf,piotSupported,pjoSchemaPart,false,pmSize);
                printFormatJ1(pmsConf,piotSupported,pjoSchema,true,pmSize);
                piotSupported=piotSupported->next;
              }

              json_object* pjoQualifiedValue=json_object_new_object();
              json_object_object_add(pjoQualifiedValue,"type",json_object_new_string("object"));

              json_object* pjoRequiredArray=json_object_new_array();
              json_object_array_add(pjoRequiredArray,json_object_new_string("value"));
              json_object_object_add(pjoQualifiedValue,"required",pjoRequiredArray);

              json_object* pjoProperties=json_object_new_object();
              json_object* pjoValueField=json_object_new_object();
              json_object_object_add(pjoValueField,"oneOf",pjoSchemaPart);
              json_object_object_add(pjoProperties,"value",pjoValueField);
              json_object_object_add(pjoQualifiedValue,"properties",pjoProperties);
              json_object_array_add(pjoProp1,pjoQualifiedValue);

              json_object* prop2=json_object_new_object();
              json_object_object_add(prop2,"oneOf",pjoProp1);
              json_object* prop3=addArray(pmsConf,peInput);
              if(prop3!=NULL){
                json_object_object_add(prop3,"items",prop2);
                if(pmMin!=NULL && atoi(pmMin->value)==0)
                  json_object_object_add(prop3,"nullable",json_object_new_boolean(true));
                json_object_object_add(pjoInput,"extended-schema",prop3);
              }
              else{
                if(pmMin!=NULL && atoi(pmMin->value)==0)
                  json_object_object_add(prop2,"nullable",json_object_new_boolean(true));
                json_object_object_add(pjoInput,"extended-schema",prop2);
              }

              json_object* prop4=json_object_new_object();
              json_object_object_add(prop4,"oneOf",pjoSchema);
              json_object_object_add(pjoInput,"schema",prop4);
              json_object_object_add(pjoInput,"raw_schema1",json_object_new_string("true"));
              json_object_put(pjoProp0);
            }

          }
          else{
            printBoundingBoxJ(pmsConf,peInput,pjoInput);
            json_object_put(pjoProp0);
            json_object_put(pjoProp1);
            json_object_put(pjoSchema);
          }
        }
      }
#if JSON_C_MINOR_VERSION >= 13
      map* pmUseRawSchema=getMapFromMaps(pmsConf,"openapi","cwl2ogc_schema");
      map* pmRawSchema=getMap(peInput->content,"raw_schema");
      if((pmUseRawSchema==NULL || strcasecmp(pmUseRawSchema->value,"false")==0) && pmRawSchema!=NULL){
        if(pmRawSchema->value[0]=='{' || pmRawSchema->value[0]=='['){
          json_object* pjoRawSchema=parseJson(pmsConf,pmRawSchema->value);
          // If there is a schema definition already, rename it to original-schema
          json_object* pjoSchema=NULL;
          if(json_object_object_get_ex(pjoInput,"schema",&pjoSchema)!=FALSE){
            json_object* pjoSchema2=NULL;
            json_object_deep_copy(pjoSchema,&pjoSchema2,NULL);
            json_object_object_add(pjoInput,"original-schema",pjoSchema2);
            json_object_object_del(pjoInput,"schema");
          }
          json_object_object_add(pjoInput,"schema",pjoRawSchema);
        }
      }
#endif
      printJMetadata(pmsConf,peInput->metadata,pjoInput);
      printJAdditionalParameters(pmsConf,peInput->additional_parameters,pjoInput);
      json_object_object_add(pjoInputs,peInput->name,pjoInput);
      peInput=peInput->next;
    }

  }

  /**
   * Add all the capabilities properties to a json_object
   *
   * @param reg the registry pointer
   * @param pmsConf the main configuration maps pointer
   * @param doc0 the void (json_object) pointer to add the property to
   * @param nc0 the void (json_object) pointer to add the property to
   * @param serv the service pointer to extract the metadata from
   */
  void printGetCapabilitiesForProcessJ(registry *reg, maps* pmsConf,void* doc0,void* nc0,service* serv){
    json_object* doc=(json_object*) doc0;
    json_object* nc=(json_object*) nc0;
    json_object *pjoRes;
    if(doc!=NULL)
      pjoRes=json_object_new_object();
    else
      pjoRes=(json_object*) nc0;
#ifdef DRU_ENABLED
    if(serviceIsDRU(pmsConf,serv->name))
      return;
#endif
    if(serviceIsFilter(pmsConf,serv->name))
      return;
    map* pmTmp0=getMapFromMaps(pmsConf,"lenv","level");
    char* rUrl=serv->name;
    if(pmTmp0!=NULL && atoi(pmTmp0->value)>0){
      int i=0;
      maps* pmsLenv=getMaps(pmsConf,"lenv");
      char* pcaTmpName=NULL;
      for(i=0;i<atoi(pmTmp0->value);i++){
        char* pcaKey=(char*)malloc(21*sizeof(char));
        sprintf(pcaKey,"sprefix_%d",i);
        map* pmTmp=getMap(pmsLenv->content,pcaKey);
        free(pcaKey);
        if(pmTmp!=NULL){
          if(i+1==atoi(pmTmp0->value))
            if(pcaTmpName==NULL){
              pcaTmpName=(char*) malloc((strlen(serv->name)+strlen(pmTmp->value)+1)*sizeof(char));
              sprintf(pcaTmpName,"%s%s",pmTmp->value,serv->name);
            }else{
              char* pcTmp=zStrdup(pcaTmpName);
              pcaTmpName=(char*) realloc(pcaTmpName,(strlen(pcTmp)+strlen(pmTmp->value)+strlen(serv->name)+1)*sizeof(char));
              sprintf(pcaTmpName,"%s%s%s",pcTmp,pmTmp->value,serv->name);
              free(pcTmp);
            }
          else
            if(pcaTmpName==NULL){
              pcaTmpName=(char*) malloc((strlen(pmTmp->value)+1)*sizeof(char));
              sprintf(pcaTmpName,"%s",pmTmp->value);
            }else{
              char* pcTmp=zStrdup(pcaTmpName);
              pcaTmpName=(char*) realloc(pcaTmpName,(strlen(pcTmp)+strlen(pmTmp->value)+1)*sizeof(char));
              sprintf(pcaTmpName,"%s%s",pcTmp,pmTmp->value);
              free(pcTmp);
          }
        }
      }
      if(pcaTmpName!=NULL){
        json_object_object_add(pjoRes,"id",json_object_new_string(pcaTmpName));
        rUrl=zStrdup(pcaTmpName);
        free(pcaTmpName);
      }
    }
    else{
      json_object_object_add(pjoRes,"id",json_object_new_string(serv->name));
      rUrl=serv->name;
    }
    if(serv->content!=NULL){
      map* tmpMap=getMap(serv->content,"title");
      if(tmpMap!=NULL){
        json_object_object_add(pjoRes,"title",json_object_new_string(_ss(tmpMap->value)));
      }
      addDescription(serv->content,0,pjoRes,NULL);
      map* pmMutable=getMap(serv->content,"mutable");
      int limit=4;
      int iCnt=0;
      if(pmMutable==NULL)
        json_object_object_add(pjoRes,"mutable",json_object_new_boolean(FALSE));
      else{
        if(strcmp(pmMutable->value,"false")==0)
          json_object_object_add(pjoRes,"mutable",json_object_new_boolean(FALSE));
        else {
          iCnt=2;
          limit=6;
          json_object_object_add(pjoRes,"mutable",json_object_new_boolean(TRUE));
        }
      }
      tmpMap=getMap(serv->content,"processVersion");
      if(tmpMap!=NULL){
        if(strlen(tmpMap->value)<5){
          char *val=(char*)malloc((strlen(tmpMap->value)+5)*sizeof(char));
          sprintf(val,"%s.0.0",tmpMap->value);
          json_object_object_add(pjoRes,"version",json_object_new_string(val));
          free(val);
        }
        else
          json_object_object_add(pjoRes,"version",json_object_new_string(tmpMap->value));
      }else
        json_object_object_add(pjoRes,"version",json_object_new_string("1.0.0"));
      tmpMap=getMap(serv->content,"keywords");
      if(tmpMap!=NULL){
        char* pcaListSections=zStrdup(tmpMap->value);
	      char *pcPtr;
	      char *pcCurrent = strtok_r (pcaListSections, ",", &pcPtr);
        json_object *pjoRes2=json_object_new_array();
        while(pcCurrent!=NULL){
          json_object_array_add(pjoRes2,json_object_new_string(pcCurrent));
          pcCurrent = strtok_r (NULL, ",", &pcPtr);
        }
        free(pcaListSections);
        json_object_object_add(pjoRes,"keywords",pjoRes2);
      }
      if(serv->metadata!=NULL){
        printJMetadata(pmsConf,serv->metadata,pjoRes);
      }
      if(serv->additional_parameters!=NULL){
        printJAdditionalParameters(pmsConf,serv->additional_parameters,pjoRes);
      }
      map* sType=getMap(serv->content,"serviceType");
      for(;iCnt<limit;iCnt+=2){
        json_object *pjoRes2=json_object_new_array();
        char *pcSavePtr;
        char* dupStr=strdup(jcapabilities[iCnt+1]);
        char *tmps = strtok_r (dupStr, " ", &pcSavePtr);
        while(tmps!=NULL){
          json_object_array_add(pjoRes2,json_object_new_string(tmps));
          tmps = strtok_r (NULL, " ", &pcSavePtr);
        }
        free(dupStr);
        json_object_object_add(pjoRes,jcapabilities[iCnt],pjoRes2);
      }
      json_object *pjoRes1=json_object_new_array();
      json_object *pjoRes2=json_object_new_object();
      json_object *pjoRes3=json_object_new_object();
      map* pmTmp=getMapFromMaps(pmsConf,"lenv","requestType");
      if(pmTmp!=NULL && strncasecmp(pmTmp->value,"desc",4)==0){
        //Previous value: "process-desc"
        map* pmTmp=getMapFromMaps(pmsConf,"processes/{processID}","rel");
        if(pmTmp!=NULL)
          json_object_object_add(pjoRes2,"rel",json_object_new_string(pmTmp->value));
        else
          json_object_object_add(pjoRes2,"rel",json_object_new_string("self"));
      }
      else{
        //Previous value: "execute"
        map* pmTmp=getMapFromMaps(pmsConf,"processes/{processID}/execution","rel");
        if(pmTmp!=NULL)
          json_object_object_add(pjoRes2,"rel",json_object_new_string(pmTmp->value));
      }
      json_object_object_add(pjoRes3,"rel",json_object_new_string("alternate"));
      json_object_object_add(pjoRes3,"type",json_object_new_string("text/html"));
      json_object_object_add(pjoRes2,"type",json_object_new_string("application/json"));
      map* tmpUrl=getMapFromMaps(pmsConf,"openapi","rootUrl");
      char* pcTmp=(char*) malloc((strlen(tmpUrl->value)+strlen(rUrl)+13)*sizeof(char));
      sprintf(pcTmp,"%s/processes/%s",tmpUrl->value,rUrl);
      if(pmTmp!=NULL && strncasecmp(pmTmp->value,"desc",4)!=0){
        json_object_object_add(pjoRes2,"title",json_object_new_string("Execute End Point"));
        json_object_object_add(pjoRes3,"title",json_object_new_string("Execute End Point"));
        char* pcTmp1=zStrdup(pcTmp);
        pcTmp=(char*) realloc(pcTmp,(strlen(pcTmp1)+11)*sizeof(char));
        sprintf(pcTmp,"%s/execution",pcTmp1);
        free(pcTmp1);
      }else{
        json_object_object_add(pjoRes2,"title",json_object_new_string("Process Description"));
        json_object_object_add(pjoRes3,"title",json_object_new_string("Process Description"));
      }
      char* pcTmp3=(char*) malloc((strlen(pcTmp)+6)*sizeof(char));
      sprintf(pcTmp3,"%s.html",pcTmp);
      json_object_object_add(pjoRes3,"href",json_object_new_string(pcTmp3));
      free(pcTmp3);
      json_object_object_add(pjoRes2,"href",json_object_new_string(pcTmp));
      free(pcTmp);
      json_object_array_add(pjoRes1,pjoRes2);
      tmpUrl=getMapFromMaps(pmsConf,"openapi","partial_html_support");
      if(tmpUrl!=NULL && strncasecmp(tmpUrl->value,"true",4)==0)
        json_object_array_add(pjoRes1,pjoRes3);
      else
        json_object_put(pjoRes3);
      json_object_object_add(pjoRes,"links",pjoRes1);
    }
    if(doc==NULL){
      elements* in=serv->inputs;
      json_object* inputs=json_object_new_object();
      printIOTypeJ(pmsConf,"input",in,inputs,serv);
      json_object_object_add(pjoRes,"inputs",inputs);

      in=serv->outputs;
      json_object* outputs=json_object_new_object();
      printIOTypeJ(pmsConf,"output",in,outputs,serv);
      json_object_object_add(pjoRes,"outputs",outputs);

    }
    if(strcmp(rUrl,serv->name)!=0)
      free(rUrl);
    if(doc!=NULL)
      json_object_array_add(doc,pjoRes);
  }

  /**
   * Produce an exception JSON object
   * 
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param s the map containing the text,code,locator keys (or a map array of the same keys)
   * @return the create JSON object (make sure to free memory)
   */
  json_object *createExceptionJ(maps* pmsConf,map* s){
    json_object *pjoRes=json_object_new_object();
    map* pmTmp=getMap(s,"code");
    if(pmTmp==NULL)
      pmTmp=getMapFromMaps(pmsConf,"lenv","code");
    if(pmTmp!=NULL){
      json_object_object_add(pjoRes,"title",json_object_new_string(_(pmTmp->value)));
      int i=0;
      int hasType=-1;
      for(i=0;i<8;i++){
        if(strcasecmp(pmTmp->value,WPSExceptionCode[OAPIPCorrespondances[i][0]])==0){
          map* pmExceptionUrl=getMapFromMaps(pmsConf,"openapi","exceptionsUrl");
          // Use OGC API - Processes - Part 2: Deploy, Replace, Undeploy
          // exceptions
          // cf. /req/deploy-replace-undeploy/deploy-response-duplicate
          // cf. /req/deploy-replace-undeploy/deploy-response-immutable
          // cf. /req/deploy-replace-undeploy/unsupported-content-type
          for(int iSCnt=0;iSCnt<1;iSCnt++){
            if(i>=OAPIPExceptionLimits[iSCnt][0]){
              maps* pmsOpenApi=getMaps(pmsConf,"openapi");
              if(pmsOpenApi!=NULL){
                pmExceptionUrl=getMapArray(pmsOpenApi->content,"exceptionsUrl",OAPIPExceptionLimits[iSCnt][1]);
              }
            }
          }
          char* pcaTmp=(char*)malloc((strlen(pmExceptionUrl->value)+strlen(OAPIPExceptionCode[OAPIPCorrespondances[i][1]])+2)*sizeof(char));
          sprintf(pcaTmp,"%s/%s",pmExceptionUrl->value,OAPIPExceptionCode[OAPIPCorrespondances[i][1]]);
          json_object_object_add(pjoRes,"type",json_object_new_string(pcaTmp));
          free(pcaTmp);
          hasType=0;
          break;
        }
      }
      if(hasType<0){
        json_object_object_add(pjoRes,"type",json_object_new_string(pmTmp->value));
      }
    }
    else{
      json_object_object_add(pjoRes,"title",json_object_new_string("NoApplicableCode"));
      json_object_object_add(pjoRes,"type",json_object_new_string("NoApplicableCode"));
    }
    pmTmp=getMap(s,"text");
    if(pmTmp==NULL)
      pmTmp=getMap(s,"message");
    if(pmTmp==NULL)
      pmTmp=getMapFromMaps(pmsConf,"lenv","message");
    if(pmTmp!=NULL)
      json_object_object_add(pjoRes,"detail",json_object_new_string(pmTmp->value));
    if(getMapFromMaps(pmsConf,"openapi","use_problem_exception"))
      setMapInMaps(pmsConf,"headers","Content-Type","application/problem+json;charset=UTF-8");
    return pjoRes;
  }
  
  /**
   * Print an OWS ExceptionReport Document and HTTP headers (when required) 
   * depending on the code.
   * Set hasPrinted value to true in the [lenv] section. 
   * @see createExceptionJ
   * 
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param s the map containing the text,code,locator keys (or a map array of the same keys)
   */
  void printExceptionReportResponseJ(maps** pmsConf,map* s){
    map* pmHasprinted=getMapFromMaps(*pmsConf,"lenv","hasExceptionPrinted");
    if(pmHasprinted!=NULL && strncasecmp(pmHasprinted->value,"true",4)==0){
      return;
    }
    pmHasprinted=getMapFromMaps(*pmsConf,"lenv","hasPrinted");
    int buffersize;
    //json_object *res=json_object_new_object();
    json_object *pjoRes=createExceptionJ(*pmsConf,s);
    maps* tmpMap=getMaps(*pmsConf,"main");
    const char *exceptionCode;
    map* pmTmp=getMap(s,"code");
    if(pmTmp==NULL)
      pmTmp=getMapFromMaps(*pmsConf,"lenv","code");
    exceptionCode=produceStatusString(*pmsConf,pmTmp);
    map* pmNoHeaders=getMapFromMaps(*pmsConf,"lenv","no-headers");
    if(pmNoHeaders==NULL || strncasecmp(pmNoHeaders->value,"false",5)==0){
      if(getMapFromMaps(*pmsConf,"headers","Content-Type")==NULL){
        map* pmExceptionContentType=getMapFromMaps(*pmsConf,"openapi","exception_mime_tye");
        if(pmExceptionContentType!=NULL)
          setMapInMaps(*pmsConf,"headers","Content-Type",pmExceptionContentType->value);
        else
          setMapInMaps(*pmsConf,"headers","Content-Type","application/json;encoding=utf-8");
      }
      printHeaders(pmsConf);
      if(getMapFromMaps(*pmsConf,"headers","status")==NULL){
        pmTmp=getMapFromMaps(*pmsConf,"lenv","status_code");
        if(pmTmp!=NULL)
          exceptionCode=pmTmp->value;
        if(exceptionCode==NULL)
          exceptionCode=aapccStatusCodes[3][0];
        if(pmNoHeaders==NULL || strncasecmp(pmNoHeaders->value,"false",5)==0){
          if(pmsConf!=NULL){
            map *tmpSid=getMapFromMaps(*pmsConf,"lenv","sid");
            if(tmpSid!=NULL){
              if( getpid()==atoi(tmpSid->value) ){
                printf("Status: %s\r\n\r\n",exceptionCode);
              }
            }
            else{
              printf("Status: %s\r\n\r\n",exceptionCode);
            }
          }else{
            printf("Status: %s\r\n\r\n",exceptionCode);
          }
        }
      }
    }
    const char* jsonStr=json_object_to_json_string_ext(pjoRes,JSON_C_TO_STRING_NOSLASHESCAPE);
    if(getMapFromMaps(*pmsConf,"lenv","jsonStr")==NULL)
      setMapInMaps(*pmsConf,"lenv","jsonStr",jsonStr);
    if(pmHasprinted==NULL || strncasecmp(pmHasprinted->value,"false",5)==0){
      printf("%s",jsonStr);
      if(*pmsConf!=NULL){
        setMapInMaps(*pmsConf,"lenv","hasPrinted","true");
        setMapInMaps(*pmsConf,"lenv","hasExceptionPrinted","true");
      }
    }
    json_object_put(pjoRes);
  }

  /**
   * Parse LiteralData value
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param element
   * @param output the maps to set current json structure
   */
  void parseJLiteral(maps* pmsConf,json_object* req,elements* element,maps* output){
    json_object* json_cinput=NULL;
    if(json_object_object_get_ex(req,"value",&json_cinput)!=FALSE){
      output->content=createMap("value",json_object_get_string(json_cinput));
    }
    const char* apccTmp[2]={
      "dataType",
      "uom"
    };
    for(int i=0;i<2;i++)
      if(json_object_object_get_ex(req,apccTmp[i],&json_cinput)!=FALSE){
        json_object* json_cinput1;
        if(json_object_object_get_ex(json_cinput,"name",&json_cinput1)!=FALSE){
          if(output->content==NULL)
            output->content=createMap(apccTmp[i],json_object_get_string(json_cinput1));
          else
            addToMap(output->content,apccTmp[i],json_object_get_string(json_cinput1));
        }
        if(json_object_object_get_ex(json_cinput,"reference",&json_cinput1)!=FALSE){
          if(output->content==NULL)
            output->content=createMap(apccTmp[i],json_object_get_string(json_cinput1));
          else
            addToMap(output->content,apccTmp[i],json_object_get_string(json_cinput1));
        }
      }
  }

  /**
   * Search field names used in the OGC API - Processes specification and 
   * replace them by old names from WPS.
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param req json_object to fetch attributes from
   * @param output the maps to store metadata informations as a maps
   */ 
  void checkCorrespondingJFields(maps* pmsConf,json_object* req,maps* output){
    json_object* json_cinput=NULL;
    for(int i=0;i<9;i++){
      if(json_object_object_get_ex(req,pccFields[i],&json_cinput)!=FALSE){
        if(output->content==NULL)
          output->content=createMap(pccFields[i],json_object_get_string(json_cinput));
        else
          addToMap(output->content,pccFields[i],json_object_get_string(json_cinput));

        if(i==0 || i==3 || i==6){
          if(output->content==NULL)
            output->content=createMap(pccRFields[0],json_object_get_string(json_cinput));
          else
            addToMap(output->content,pccRFields[0],json_object_get_string(json_cinput));
        }
        if(i==1 || i==4 || i==7){
          if(output->content==NULL)
            output->content=createMap(pccRFields[1],json_object_get_string(json_cinput));
          else
            addToMap(output->content,pccRFields[1],json_object_get_string(json_cinput));
        }
        if(i==2 || i==5 || i==8){
          if(output->content==NULL)
            output->content=createMap(pccRFields[2],json_object_get_string(json_cinput));
          else
            addToMap(output->content,pccRFields[2],json_object_get_string(json_cinput));
        }
      }
    }
  }

  /**
   * Add a http request to the queue
   *
   * @param ppmsConf the maps containing the settings of the main.cfg file
   * @param pccValue the URL to fetch
   * @param pccName the name of the input
   */
  void addToJRequests(maps** ppmsConf,const char* pccValue,const char* pccName){
    int len=0;
    int createdStr=0;
    char *pcaTmp=(char*)"url";
    char *pcaTmp1=(char*)"input";
    maps* pmsConf=*ppmsConf;
    if(getMaps(pmsConf,"http_requests")==NULL){
      maps* pmsaHttpRequests=createMaps("http_requests");
      pmsaHttpRequests->content=createMap("length","1");
      addMapsToMaps(ppmsConf,pmsaHttpRequests);
      freeMaps(&pmsaHttpRequests);
      free(pmsaHttpRequests);
    }else{
      map* tmpMap=getMapFromMaps(pmsConf,"http_requests","length");
      int len=atoi(tmpMap->value);
      createdStr=1;
      pcaTmp=(char*) malloc((16)*sizeof(char));
      sprintf(pcaTmp,"%d",len+1);
      setMapInMaps(pmsConf,"http_requests","length",pcaTmp);
      sprintf(pcaTmp,"url_%d",len);
      pcaTmp1=(char*) malloc((19)*sizeof(char));
      sprintf(pcaTmp1,"input_%d",len);
    }
    setMapInMaps(pmsConf,"http_requests",pcaTmp,pccValue);
    setMapInMaps(pmsConf,"http_requests",pcaTmp1,pccName);
    if(createdStr>0){
      free(pcaTmp);
      free(pcaTmp1);
    }
  }

  /**
   * Parse all keys in a json object except value, href and format
   * 
   * @param req json_object pointing to the input/output
   * @param output the maps to set current json structure
   */
  void parseAllKeys(json_object* req,maps* output){
    json_object_object_foreach(req, key, val) {
      if(strcmp(key,"value")!=0 && strcmp(key,"href")!=0 && strcmp(key,"format")!=0){
        enum json_type type = json_object_get_type(val);
        if(type!=json_type_object && type!=json_type_array){
          if(output->content==NULL)
            output->content=createMap(key,json_object_get_string(val));
          else
            addToMap(output->content,key,json_object_get_string(val));
        }
      }
    }
  }
  /**
   * Parse a nested process json object
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param element the metadata for this input
   * @param output the maps to set current json structure
   * @param pccName the name of the request from http_requests
   */
  void parseNestedProcess(maps* pmsConf,json_object* req,elements* element,maps* output,const char* pccName){
    // Parse and execute nested processes handling
    json_object* json_value=NULL;
    if(json_object_object_get_ex(req,"process",&json_value)!=FALSE){
      // Need to store the request body and the proper url to fetch
      ZOO_DEBUG("Nested process execution invocation");
      const char* pccTmp=json_object_get_string(json_value);
      // Build URL
      char *pcaTmp=(char*)malloc((strlen(pccTmp)+11)*sizeof(char));
      sprintf(pcaTmp,"%s/execution",pccTmp);
      output->content=createMap("xlink:href",pcaTmp);
      // Add body and content-type header
      const char* pccBody=json_object_to_json_string_ext(req,JSON_C_TO_STRING_NOSLASHESCAPE);
      map* pmExecuteMode=getMapFromMaps(pmsConf,"lenv","execute_mode");
      addToMap(output->content,"Body",pccBody);
      addToMap(output->content,"Headers","Content-Type: application/json");
      if(pmExecuteMode!=NULL && strncasecmp(pmExecuteMode->value,"async",5)==0){
        map* pmExtraSupportedCodes=getMapFromMaps(pmsConf,"main","extra_supported_codes");
        if(pmExtraSupportedCodes!=NULL){
          char* pcaTmp=(char*)malloc((strlen(pmExtraSupportedCodes->value)+5)*sizeof(char));
          sprintf(pcaTmp,"%s,201",pmExtraSupportedCodes->value);
          setMapInMaps(pmsConf,"main","extra_supported_codes",pcaTmp);
          free(pcaTmp);
        }
        else
          setMapInMaps(pmsConf,"main","extra_supported_codes","201");
        addToMap(output->content,"Headers_extra","Prefer: respond-async;return=representation");
      }
      addToJRequests(&pmsConf,pcaTmp,pccName);
      map* pmTmp=getMapFromMaps(pmsConf,"http_requests","length");
      if(pmTmp!=NULL){
        ZOO_DEBUG("Nested process execution invocation");
        maps* pmsRequests=getMaps(pmsConf,"http_requests");
        setMapArray(pmsRequests->content,"body",atoi(pmTmp->value)-1, pccBody);
        setMapArray(pmsRequests->content,"Headers",atoi(pmTmp->value)-1, "Content-Type: application/json");
        if(pmExecuteMode!=NULL && strncasecmp(pmExecuteMode->value,"async",5)==0){
          setMapArray(pmsRequests->content,"Headers_extra",atoi(pmTmp->value)-1, "Preference: respond-async;return=representation");
        }
        // Add jobs_handlers section in the main configuration maps
        addToMap(output->content,"require_job_handler", "true");
        setMapArray(pmsRequests->content,"require_job_handler",atoi(pmTmp->value)-1, "true");
        maps* pmsJobHandlers=getMaps(pmsConf,"jobs_handlers");
        char* pcaTmp=(char*)malloc((12)*sizeof(char));
        sprintf(pcaTmp,"%d",atoi(pmTmp->value)-1);
        if(pmsJobHandlers==NULL){
          maps* pmsaJobsHandlers=createMaps("jobs_handlers");
          pmsaJobsHandlers->content=createMap("value",pcaTmp);
          addToMap(pmsaJobsHandlers->content,"length","1");
          addMapsToMaps(&pmsConf,pmsaJobsHandlers);
          freeMaps(&pmsaJobsHandlers);
          free(pmsaJobsHandlers);
        }else{
          map* pmsNbJobHandlers=getMapFromMaps(pmsConf,"jobs_handlers","length");
          int iNbJobHandlers=atoi(pmsNbJobHandlers->value);
          setMapArray(pmsJobHandlers->content,"value",atoi(pmsNbJobHandlers->value),pcaTmp);
        }
      }
      free(pcaTmp);
    }
  }

  /**
   * Parse ComplexData value
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param element the metadata
   * @param output the maps to set current json structure
   * @param pccName the name of the request from http_requests
   */
  void parseJComplex(maps* pmsConf,json_object* req,elements* element,maps* output,const char* pccName){
    json_object* json_cinput=NULL;
    if(json_object_object_get_ex(req,"value",&json_cinput)!=FALSE){
      output->content=createMap("value",json_object_get_string(json_cinput));
    }
    else{
      json_object* json_value=NULL;
      if(json_object_object_get_ex(req,"href",&json_value)!=FALSE){
        output->content=createMap("xlink:href",json_object_get_string(json_value));
        addToJRequests(&pmsConf,json_object_get_string(json_value),pccName);
      }
    }
    if(json_object_object_get_ex(req,"format",&json_cinput)!=FALSE){
      json_object_object_foreach(json_cinput, pcKey, pjoVal) {
        if(output->content==NULL)
          output->content=createMap(pcKey,json_object_get_string(pjoVal));
        else
          addToMap(output->content,pcKey,json_object_get_string(pjoVal));
      }
      checkCorrespondingJFields(pmsConf,json_cinput,output);
    }
    checkCorrespondingJFields(pmsConf,req,output);
    json_object_object_foreach(req, key, val) {
      if(strcmp(key,"value")!=0 && strcmp(key,"href")!=0 && strcmp(key,"format")!=0){
        enum json_type type = json_object_get_type(val);
        if(type!=json_type_object && type!=json_type_array){
          if(output->content==NULL)
            output->content=createMap(key,json_object_get_string(val));
          else
            addToMap(output->content,key,json_object_get_string(val));
        }
      }
    }
  }

  /**
   * Parse BoundingBox value
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoInput json_object pointing to the input/output
   * @param element the metadata
   * @param output the maps to set current json structure
   */
  void parseJBoundingBox(maps* pmsConf,json_object* pjoInput,elements* element,maps* output){
    json_object* json_cinput=NULL;
    if(json_object_object_get_ex(pjoInput,"bbox",&json_cinput)!=FALSE){
      output->content=createMap("value",json_object_get_string(json_cinput));
    }
    if(json_object_object_get_ex(pjoInput,"crs",&json_cinput)!=FALSE){
      if(output->content==NULL)
        output->content=createMap("crs",json_object_get_string(json_cinput));
      else
        addToMap(output->content,"crs","http://www.opengis.net/def/crs/OGC/1.3/CRS84");
    }
    char* apcTmp[2]={
      (char*)"lowerCorner",
      (char*)"upperCorner"
    };
    for(int i=0;i<2;i++)
      if(json_object_object_get_ex(pjoInput,apcTmp[i],&json_cinput)!=FALSE){
        if(output->content==NULL)
          output->content=createMap(apcTmp[i],json_object_get_string(json_cinput));
        else
          addToMap(output->content,apcTmp[i],json_object_get_string(json_cinput));
      }
  }

  /**
   * Return an exception response with Status: "400 Bad Request" if inputs are
   * not in the proper format.
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param  peInput the input causing the issue
   */
  void returnBadRequestException(maps* pmsConf,elements* peInput){
    map* pmError=createMap("code","BadRequest");
    char tmpS1[1024];
    sprintf(tmpS1,_("Wrong type used for input %s"),peInput->name);
    addToMap(pmError,"message",tmpS1);
    setMapInMaps(pmsConf,"lenv","status_code","400 Bad Request");
    printExceptionReportResponseJ(&pmsConf,pmError);
    freeMap(&pmError);
    free(pmError);
    return;
  }

  /**
   * Parse a single input / output entity
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   * @param pccKey char* the input/output name
   * @param val json_object pointing to the input/output
   * @param cMaps the outputed maps containing input (or output) metedata 
   */
  void _parseJIOSingle1(maps* pmsConf,elements* cio, maps** ioMaps,const char* ioType,const char* pccKey,json_object* val,maps* cMaps){
    json_object* json_cinput;
    if(strncmp(ioType,"inputs",6)==0) {
      if(json_object_is_type(val,json_type_object)) {
        // ComplexData
        elements* peCurrentInput=getElements(cio,cMaps->name);
        // TODO: review if this is required as now a process can be the value
        // for everything (including LiteralData)
        if(getMapFromMaps(pmsConf,"openapi","ensure_type_validation")!=NULL)
          if(peCurrentInput!=NULL){
            if(peCurrentInput->format!=NULL
              && strcasecmp(peCurrentInput->format,"LiteralData")!=0
              && strcasecmp(peCurrentInput->format,"ComplexData")!=0
              && strcasecmp(peCurrentInput->format,"BoundingBoxData")!=0){
              return returnBadRequestException(pmsConf,peCurrentInput);
            }
          }
        if(json_object_object_get_ex(val,"process",&json_cinput) != FALSE){
          parseNestedProcess(pmsConf,val,cio,cMaps,pccKey);
        } else if(json_object_object_get_ex(val,"value",&json_cinput)!= FALSE ||
            json_object_object_get_ex(val,"href",&json_cinput) != FALSE ){
          parseJComplex(pmsConf,val,cio,cMaps,pccKey);
        } else if( json_object_object_get_ex(val,"bbox",&json_cinput)!= FALSE ){
          parseJBoundingBox(pmsConf,val,cio,cMaps);
        } else if(getMapFromMaps(pmsConf,"openapi","ensure_type_validation")!=NULL){
          return returnBadRequestException(pmsConf,peCurrentInput);
        }
      } else if(json_object_is_type(val,json_type_array)){
        // Need to run detection on each item within the array
      }else{
        // Basic types
        if(getMapFromMaps(pmsConf,"openapi","ensure_type_validation")!=NULL
          &&
          getMapFromMaps(pmsConf,"openapi","ensure_type_validation_but_ets")==NULL){
          elements* peCurrentInput=getElements(cio,cMaps->name);
          if(peCurrentInput!=NULL){
            if(peCurrentInput->format!=NULL && strcasecmp(peCurrentInput->format,"LiteralData")!=0){
              map* error=createMap("code","BadRequest");
              char tmpS1[1024];
              sprintf(tmpS1,_("Wrong type used for input %s"),peCurrentInput->name);
              addToMap(error,"message",tmpS1);
              setMapInMaps(pmsConf,"lenv","status_code","400 Bad Request");
              printExceptionReportResponseJ(&pmsConf,error);
              return;
            }
          }
        }
        cMaps->content=createMap("value",json_object_get_string(val));
      }
    }
    else{
      if(json_object_object_get_ex(val,"dataType",&json_cinput)!=FALSE){
        parseJLiteral(pmsConf,val,cio,cMaps);
      } else if(json_object_object_get_ex(val,"format",&json_cinput)!=FALSE){
        parseJComplex(pmsConf,val,cio,cMaps,pccKey);
      } else if(json_object_object_get_ex(val,"bbox",&json_cinput)!=FALSE){
        parseJBoundingBox(pmsConf,val,cio,cMaps);
      }// else error!
      else{
        if(json_object_object_get_ex(val,"value",&json_cinput)!=FALSE){
          map* error=createMap("code","BadRequest");
          char tmpS[1024];
          sprintf(tmpS,_("Missing input for %s"),cio->name);
          addToMap(error,"message",tmpS);
          setMapInMaps(pmsConf,"lenv","status_code","400 Bad Request");
          printExceptionReportResponseJ(&pmsConf,error);
          return;
        }else{
          if(json_object_get_type(val)==json_type_string){
            parseJLiteral(pmsConf,val,cio,cMaps);
          }else if(json_object_get_type(val)==json_type_object){
            json_object* json_ccinput=NULL;
            if(json_object_object_get_ex(val,"bbox",&json_ccinput)!=FALSE){
              parseJBoundingBox(pmsConf,val,cio,cMaps);
            }
            else{
              parseJComplex(pmsConf,val,cio,cMaps,pccKey);
            }
          }else{
            if(strcmp(ioType,"input")==0){
              map* error=createMap("code","BadRequest");
              char tmpS1[1024];
              sprintf(tmpS1,_("Issue with input %s"),cio->name);
              addToMap(error,"message",tmpS1);
              setMapInMaps(pmsConf,"lenv","status_code","400 Bad Request");
              printExceptionReportResponseJ(&pmsConf,error);
              return;
            }
          }
        }
      }
    }
  }
  
  /**
   * Parse a single input / output entity
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   * @param pccKey char* the input/output name
   * @param val json_object pointing to the input/output
   * @param cMaps the outputed maps containing input (or output) metedata 
   */
  void _parseJIOSingle(maps* pmsConf,elements* cio, maps** ioMaps,const char* ioType,const char* pccKey,json_object* val,maps* cMaps){
    json_object* json_cinput;
    if(json_object_object_get_ex(val,"dataType",&json_cinput)!=FALSE){
      parseJLiteral(pmsConf,val,cio,cMaps);
    } else if(json_object_object_get_ex(val,"format",&json_cinput)!=FALSE){
      parseJComplex(pmsConf,val,cio,cMaps,pccKey);
    } else if(json_object_object_get_ex(val,"bbox",&json_cinput)!=FALSE){
      parseJBoundingBox(pmsConf,val,cio,cMaps);
    }// else error!
    else{
      if(json_object_object_get_ex(val,"value",&json_cinput)!=FALSE){
        map* error=createMap("code","BadRequest");
        char tmpS[1024];
        sprintf(tmpS,_("Missing input for %s"),cio->name);
        addToMap(error,"message",tmpS);
        setMapInMaps(pmsConf,"lenv","status_code","400 Bad Request");
        printExceptionReportResponseJ(&pmsConf,error);
        return;
      }else{
        if(json_object_get_type(json_cinput)==json_type_string){
          parseJLiteral(pmsConf,val,cio,cMaps);
        }else if(json_object_get_type(json_cinput)==json_type_object){
          json_object* json_ccinput=NULL;
          if(json_object_object_get_ex(json_cinput,"bbox",&json_ccinput)!=FALSE){
            parseJComplex(pmsConf,val,cio,cMaps,pccKey);
          }
          else{
            parseJBoundingBox(pmsConf,val,cio,cMaps);
          }
        }else{
          if(strcmp(ioType,"input")==0){
            map* error=createMap("code","BadRequest");
            char tmpS1[1024];
            sprintf(tmpS1,_("Issue with input %s"),cio->name);
            addToMap(error,"message",tmpS1);
            setMapInMaps(pmsConf,"lenv","status_code","400 Bad Request");
            printExceptionReportResponseJ(&pmsConf,error);
            return;
          }
        }
      }
    }    
  }
  
  /**
   * Parse a single input / output entity that can be an array 
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   * @param pccKey char* the input/output name
   * @param val json_object pointing to the input/output
   */
  void parseJIOSingle(maps* pmsConf,elements* ioElements, maps** ioMaps,const char* ioType,const char* pccKey,json_object* val){
    maps *cMaps=NULL;
    cMaps=createMaps(pccKey);
    elements* cio=getElements(ioElements,pccKey);    
    if(json_object_is_type(val,json_type_array)){
      int i=0;
      size_t len=json_object_array_length(val);
      for(i=0;i<len;i++){
        json_object* json_current_io=json_object_array_get_idx(val,i);
        maps* pmsExtra=createMaps(pccKey);
        _parseJIOSingle1(pmsConf,cio,ioMaps,ioType,pccKey,json_current_io,pmsExtra);
        map* pmCtx=pmsExtra->content;
        while(pmCtx!=NULL){
          if(cMaps->content==NULL)
            cMaps->content=createMap(pmCtx->name,pmCtx->value);
          else
            setMapArray(cMaps->content,pmCtx->name,i,pmCtx->value);
          pmCtx=pmCtx->next;
        }
        freeMaps(&pmsExtra);
        free(pmsExtra);
      }
    }else{
      _parseJIOSingle1(pmsConf,cio,ioMaps,ioType,pccKey,val,cMaps);
    }

    
    if(strcmp(ioType,"outputs")==0){
      // Outputs
      json_object* json_cinput;
      if(json_object_object_get_ex(val,"transmissionMode",&json_cinput)!=FALSE){
        if(cMaps->content==NULL)
          cMaps->content=createMap("transmissionMode",json_object_get_string(json_cinput));
        else
          addToMap(cMaps->content,"transmissionMode",json_object_get_string(json_cinput));
      }else{
        // Default to transmissionMode value
        if(cMaps->content==NULL)
          cMaps->content=createMap("transmissionMode","value");
        else
          addToMap(cMaps->content,"transmissionMode","value");
      }
      if(json_object_object_get_ex(val,"format",&json_cinput)!=FALSE){
        json_object_object_foreach(json_cinput, pcKey1, pjoVal1) {
          if(cMaps->content==NULL)
            cMaps->content=createMap(pcKey1,json_object_get_string(pjoVal1));
          else
            addToMap(cMaps->content,pcKey1,json_object_get_string(pjoVal1));
        }
      }
    }

    addToMap(cMaps->content,"inRequest","true");
    if (ioMaps == NULL)
      *ioMaps = dupMaps(&cMaps);
    else
      addMapsToMaps (ioMaps, cMaps);

    freeMaps(&cMaps);
    free(cMaps);    
  }
  
  /**
   * Parse all the inputs / outputs entities
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoInput json_object pointing to the input/output
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   */
  void parseJIO(maps* pmsConf, json_object* pjoInput, elements* ioElements, maps** ioMaps,const char* ioType){
    json_object* json_io=NULL;
    if(json_object_object_get_ex(pjoInput,ioType,&json_io)!=FALSE){
      json_object* json_current_io=NULL;
      if(json_object_is_type(json_io,json_type_array)){
        size_t len=json_object_array_length(json_io);
        for(int i=0;i<len;i++){
          maps *cMaps=NULL;
          json_current_io=json_object_array_get_idx(json_io,i);
          json_object* cname=NULL;
          if(json_object_object_get_ex(json_current_io,"id",&cname)!=FALSE) {
            json_object* pjoInput=NULL;
            if(json_object_object_get_ex(json_current_io,"input",&pjoInput)!=FALSE) {
              parseJIOSingle(pmsConf,ioElements,ioMaps,ioType,json_object_get_string(cname),pjoInput);
            }else
              parseJIOSingle(pmsConf,ioElements,ioMaps,ioType,json_object_get_string(cname),json_current_io);
          }
        }
      }else{
        json_object_object_foreach(json_io, pcKey, pjoVal) {
          parseJIOSingle(pmsConf,ioElements,ioMaps,ioType,pcKey,pjoVal);
        }
      }
    }else{
      // Requirement 27: when no output is specified, the server should consider returning all outputs.
      // default transmissionMode set to "value"
      if(strcmp(ioType,"outputs")==0){
        elements* peTmp=ioElements;
        while(peTmp!=NULL){
          maps* pmsTmp=createMaps(peTmp->name);
          pmsTmp->content=createMap("transmissionMode","value");
          if(*ioMaps==NULL)
            *ioMaps=dupMaps(&pmsTmp);
          else
            addMapsToMaps(ioMaps,pmsTmp);
          freeMaps(&pmsTmp);
          free(pmsTmp);
          peTmp=peTmp->next;
        }
      }
    }
  }

  /**
   * Parse Json Request
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param s the current service metadata
   * @param pjoRequestBody the JSON object of the request body
   * @param pmRequestInputs the produced maps
   * @param inputs the produced maps
   * @param outputs the produced maps
   */
  void parseJRequest(maps* pmsConf, service* s,json_object* pjoRequestBody, map* pmRequestInputs, maps** inputs,maps** outputs){
    elements* io=s->inputs;
    json_object* json_io=NULL;
    int parsed=0;
    char* tmpS=(char*)"input";
    maps* in=*inputs;
    parseJIO(pmsConf,pjoRequestBody,s->inputs,inputs,"inputs");
    parseJIO(pmsConf,pjoRequestBody,s->outputs,outputs,"outputs");

    if(parsed==0){
      json_io=NULL;
      if(json_object_object_get_ex(pjoRequestBody,"mode",&json_io)!=FALSE){
        addToMap(pmRequestInputs,"mode",json_object_get_string(json_io));
        setMapInMaps(pmsConf,"request","mode",json_object_get_string(json_io));
      }
      json_io=NULL;
      map* preference=getMapFromMaps(pmsConf,"renv","HTTP_PREFER");
      if(preference!=NULL && strstr(preference->value,"return=minimal")!=NULL){
        addToMap(pmRequestInputs,"response","raw");
        setMapInMaps(pmsConf,"request","response","raw");
      }
      else{
        if(preference!=NULL && strstr(preference->value,"return=representation")!=NULL){
          addToMap(pmRequestInputs,"response","document");
          setMapInMaps(pmsConf,"request","response","document");
        }
      }
      if(json_object_object_get_ex(pjoRequestBody,"response",&json_io)!=FALSE){
        if(getMap(pmRequestInputs,"response")==NULL)
          addToMap(pmRequestInputs,"response",json_object_get_string(json_io));
        else{
          map* pmTmp=getMap(pmRequestInputs,"response");
          free(pmTmp->value);
          pmTmp->value=zStrdup(json_object_get_string(json_io));
        }
        setMapInMaps(pmsConf,"request","response",json_object_get_string(json_io));
      }else{
        if(getMap(pmRequestInputs,"response")==NULL){
          // OGC Test Suite requirement (the execute.yaml schema said that it should be raw)
          if(getMapFromMaps(pmsConf,"openapi","default_result_as_document")!=NULL){
            addToMap(pmRequestInputs,"response","document");
            setMapInMaps(pmsConf,"request","response","document");
          }else{
            addToMap(pmRequestInputs,"response","raw");
            setMapInMaps(pmsConf,"request","response","raw");
          }
        }
      }
      json_io=NULL;
      if(json_object_object_get_ex(pjoRequestBody,"subscriber",&json_io)!=FALSE){
        maps* subscribers=createMaps("subscriber");
        json_object* json_subscriber=NULL;
        if(json_object_object_get_ex(json_io,"successUri",&json_subscriber)!=FALSE){
          subscribers->content=createMap("successUri",json_object_get_string(json_subscriber));
        }
        if(json_object_object_get_ex(json_io,"inProgressUri",&json_subscriber)!=FALSE){
          if(subscribers->content==NULL)
            subscribers->content=createMap("inProgressUri",json_object_get_string(json_subscriber));
          else
            addToMap(subscribers->content,"inProgressUri",json_object_get_string(json_subscriber));
        }
        if(json_object_object_get_ex(json_io,"failedUri",&json_subscriber)!=FALSE){
          if(subscribers->content==NULL)
            subscribers->content=createMap("failedUri",json_object_get_string(json_subscriber));
          else
            addToMap(subscribers->content,"failedUri",json_object_get_string(json_subscriber));
        }
        addMapsToMaps(&pmsConf,subscribers);
        freeMaps(&subscribers);
        free(subscribers);
      }
    }
  }

  /**
   * Print the jobs status info
   * cf. 
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pcJobId the char pointer to the jobid
   * @return the JSON object pointer to the statusInfo
   */
  json_object* printJobStatus(maps** ppmsConf,char* pcJobId){
    json_object* pjoRes=NULL;
    maps* pmsConf=*ppmsConf;
    runGetStatus(ppmsConf,pcJobId,(char*)"GetStatus");
    map* pmError=getMapFromMaps(pmsConf,"lenv","error");
    if(pmError!=NULL && strncasecmp(pmError->value,"true",4)==0){
      printExceptionReportResponseJ(ppmsConf,getMapFromMaps(pmsConf,"lenv","code"));
      return NULL;
    }else{
      map* pmStatus=getMapFromMaps(pmsConf,"lenv","status");
      setMapInMaps(pmsConf,"lenv","gs_location","false");
      setMapInMaps(pmsConf,"lenv","gs_usid",pcJobId);
      if(pmStatus!=NULL && strncasecmp(pmStatus->value,"Failed",6)==0)
        pjoRes=createStatus(pmsConf,SERVICE_FAILED);
      else
        if(pmStatus!=NULL  && strncasecmp(pmStatus->value,"Succeeded",9)==0)
          pjoRes=createStatus(pmsConf,SERVICE_SUCCEEDED);
        else
          if(pmStatus!=NULL  && strncasecmp(pmStatus->value,"Running",7)==0){
            /*map* tmpMap=getMapFromMaps(pmsConf,"lenv","Message");
            if(tmpMap!=NULL)
              setMapInMaps(pmsConf,"lenv","gs_message",tmpMap->value);*/
            pjoRes=createStatus(pmsConf,SERVICE_STARTED);
          }
          else
            pjoRes=createStatus(pmsConf,SERVICE_FAILED);
    }
    return pjoRes;
  }

  /**
   * Verify if the process identified by pccPid is part of the processID filters,
   * if any.
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pccPid the const char pointer to the processID
   * @return true in case the filter is empty or in case the processId pccPid
   * is one of the filtered processID, false in other case.
   */
  bool isFilteredPid(maps* pmsConf,const char* pccPid){
#ifndef RELY_ON_DB
    maps* pmsLenv=getMaps(pmsConf,"lenv");
    map* pmCurrent=getMap(pmsLenv->content,"servicePidFilter");
    if(pmCurrent==NULL)
      return true;
    map* pmLength=getMapFromMaps(pmsConf,"lenv","length");
    int iLimit=1;
    if(pmLength!=NULL)
      iLimit=atoi(pmLength->value);
    for(int iCnt=0;iCnt<iLimit;iCnt++){
      map* pmCurrent=getMapArray(pmsLenv->content,"servicePidFilter",iCnt);
      if(pmCurrent!=NULL && strcmp(pccPid,pmCurrent->value)==0){
        return true;
      }
    }
    return false;
#else
    return true;
#endif
  }

  /**
   * Verify if the process identified by pccPid is part of the processID filters,
   * if any.
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pccPid the const char pointer to the processID
   * @return true in case the filter is empty or in case the processId pccPid
   * is one of the filtered processID, false in other case.
   */
  bool isFilteredDMM(maps* pmsConf,const char* pccPid){
#ifndef RELY_ON_DB
    maps* pmsRequest=getMaps(pmsConf,"request");
    if(pmsRequest!=NULL){
      map* pmTmpPath=getMapFromMaps(pmsConf,"main","tmpPath");
      map* pmMinDate=getMap(pmsRequest->content,"minDuration");
      map* pmMaxDate=getMap(pmsRequest->content,"maxDuration");
      map* pmTimeInterval=getMap(pmsRequest->content,"datetime");
      if(pmMinDate==NULL && pmMaxDate==NULL && pmTimeInterval==NULL)
        return true;
      else{
        // Find datetime for files <pccPid>.sid
        zStatStruct zssSid_status;
        char* pcaSidPath=(char*)malloc((strlen(pccPid)+strlen(pmTmpPath->value)+6)*sizeof(char));
        sprintf(pcaSidPath,"%s/%s.sid",pmTmpPath->value,pccPid);
        int iS1=zStat(pcaSidPath, &zssSid_status);
        if(pmMinDate!=NULL || pmMaxDate!=NULL){
          // Find datetime for files <pccPid>_lenv.cfg
          int iMin=0,iMax=86400;
          zStatStruct zssLenv_status;
          char* pcaLenvPath=(char*)malloc((strlen(pccPid)+strlen(pmTmpPath->value)+7)*sizeof(char));
          sprintf(pcaLenvPath,"%s/%s.json",pmTmpPath->value,pccPid);
          int iStat=zStat(pcaLenvPath, &zssLenv_status);
          if(iStat!=0)
            return false;
          free(pcaLenvPath);
          // Use difftime to get elapsed seconds
          double ulElapsedSeconds=0;
          if(zssLenv_status.st_mtime!=zssSid_status.st_mtime){
            ulElapsedSeconds=(zssLenv_status.st_mtime-zssSid_status.st_mtime);
          }
          if(pmMinDate!=NULL)
            iMin=atoi(pmMinDate->value);
          if(pmMaxDate!=NULL)
            iMax=atoi(pmMaxDate->value);
          if(iMax<ulElapsedSeconds || iMin>ulElapsedSeconds){
            return false;
          }
        }
        if(pmTimeInterval!=NULL){
          // Find datetime or interval
          struct tm tmaLimits[2];
          bool baLimits[2]={false,false};
          char* token, *pcSavePtr,*pcaDecodedValue,*pcaDupValue;
          int iCnt=0;
          pcaDupValue=zStrdup(pmTimeInterval->value);
          pcaDecodedValue=url_decode(pcaDupValue);
          free(pcaDupValue);
          token=strtok_r(pcaDecodedValue,"/",&pcSavePtr);
          while(token!=NULL){
            if(iCnt<2){
              if(strncmp(token,"..",2)!=0 && strstr(token,",")==NULL){
                char* pcaDup=zStrdup(token);
                strptime(pcaDup,zDateFormat,&tmaLimits[iCnt]);
                baLimits[iCnt]=true;
                free(pcaDup);
              }
            }
            if(strstr(token,",")==NULL)
              iCnt++;
            token=strtok_r(NULL,"/",&pcSavePtr);
          }
          free(pcaDecodedValue);
          if((baLimits[0] && zssSid_status.st_mtime<mktime(&tmaLimits[0])) ||
            (baLimits[1] && zssSid_status.st_mtime>mktime(&tmaLimits[1]))){
            return false;
          }
        }
        return true;
      }
    }else
      return true;
    return true;
#else
    return true;
#endif
  }

  /**
   * Print the jobs list
   *
   * @param ppmsConf the maps containing the settings of the main.cfg file
   * @return the JSON object pointer to the jobs list
   */
  json_object* printJobList(maps** ppmsConf){
    json_object* pjoRes=json_object_new_array();
    maps* pmsConf=*ppmsConf;
    map* pmJobs=getMapFromMaps(pmsConf,"lenv","selectedJob");
    map* pmTmpPath=getMapFromMaps(pmsConf,"main","tmpPath");
    struct dirent *dp;
    int cnt=0;
    int skip=0;
    int limit=10000;
    map* pmLimit=getMapFromMaps(pmsConf,"lenv","serviceCntLimit");
    map* pmSkip=getMapFromMaps(pmsConf,"lenv","serviceCntSkip");
    if(pmLimit!=NULL){
      limit=atoi(pmLimit->value);
    }
    if(pmSkip!=NULL){
      skip=atoi(pmSkip->value);
    }
    if(pmJobs!=NULL){
      if(strcmp(pmJobs->value,"-1")==0){
        // Exception invalid-query-paramter-value
        map* pmaTmp=createMap("code","InvalidQueryParameterValue");
        addToMap(pmaTmp,"message","The server was unable to parse one of the query pareter provided");
        json_object_put(pjoRes);
        printExceptionReportResponseJ(ppmsConf,pmaTmp);
        freeMap(&pmaTmp);
        free(pmaTmp);
        return NULL;
      }
      char *pcSavePtr;
      char *tmps = strtok_r(pmJobs->value, ",", &pcSavePtr);
      while(tmps!=NULL){
        if(cnt>=skip && cnt<limit+skip && strlen(tmps)>2){
          char* pcaTmp=zStrdup(tmps+1);
          pcaTmp[strlen(pcaTmp)-1]=0;
          json_object* cjob=printJobStatus(ppmsConf,pcaTmp);
          json_object_array_add(pjoRes,cjob);
          free(pcaTmp);
        }
        if(cnt==limit+skip)
          setMapInMaps(pmsConf,"lenv","serviceCntNext","true");
        cnt++;
        tmps = strtok_r (NULL, ",", &pcSavePtr);
      }
    }else{
      DIR *dirp = opendir (pmTmpPath->value);
      if(dirp!=NULL){
        while ((dp = readdir (dirp)) != NULL){
          char* extn = strstr(dp->d_name, ".json");
          if(extn!=NULL && strlen(dp->d_name)==41){
            char* pcaTmp=zStrdup(dp->d_name);
            pcaTmp[strlen(dp->d_name)-5]=0;
#ifndef RELY_ON_DB
            //map* pmTmpPath=getMapFromMaps(pmsConf,"main","tmpPath");
            char* pcaLenvPath=(char*)malloc((strlen(pcaTmp)+strlen(pmTmpPath->value)+11)*sizeof(char));
            sprintf(pcaLenvPath,"%s/%s_lenv.cfg",pmTmpPath->value,pcaTmp);
            maps *pmsaLenv = (maps *) malloc (MAPS_SIZE);
            pmsaLenv->content = NULL;
            pmsaLenv->child = NULL;
            pmsaLenv->next = NULL;
            map *pmaPid=NULL, *pmaUsid=NULL;
            if(conf_read(pcaLenvPath,pmsaLenv) !=2){
              map *pmTmp=getMapFromMaps(pmsaLenv,"lenv","Identifier");
              if(pmTmp!=NULL){
                pmaPid=createMap("value",pmTmp->value);
                addToMap(pmaPid,"toRemove","true");
              }
              pmTmp=getMapFromMaps(pmsaLenv,"lenv","usid");
              if(pmTmp!=NULL){
                pmaUsid=createMap("value",pmTmp->value);
                addToMap(pmaPid,"toRemove","true");
              }
              freeMaps(&pmsaLenv);
              free(pmsaLenv);
            }else{
              pmaPid=createMap("toRemove","true");
              pmaUsid=createMap("toRemove","true");
            }
            free(pcaLenvPath);
#else
            map *pmaPid=NULL, *pmaUsid=NULL;
#endif
            if(isFilteredPid(pmsConf,pmaPid->value) &&
                isFilteredDMM(pmsConf,pmaUsid->value)){
              if(cnt>=skip && cnt<limit+skip){
                json_object* cjob=printJobStatus(ppmsConf,pcaTmp);
                json_object_array_add(pjoRes,cjob);
              }
              if(cnt==limit+skip)
                setMapInMaps(pmsConf,"lenv","serviceCntNext","true");
              cnt++;
            }
            if(pmaPid!=NULL && getMap(pmaPid,"toRemove")!=NULL){
              freeMap(&pmaPid);
              free(pmaPid);
            }
            if(pmaUsid!=NULL && getMap(pmaUsid,"toRemove")!=NULL){
              freeMap(&pmaUsid);
              free(pmaUsid);
            }
            free(pcaTmp);
          }
        }
        closedir (dirp);
      }
    }
    json_object* pjoResFinal=json_object_new_object();
    json_object_object_add(pjoResFinal,"jobs",pjoRes);
    setMapInMaps(pmsConf,"lenv","path","jobs");
    char pcCnt[12];
    sprintf(pcCnt,"%d",cnt);
    setMapInMaps(pmsConf,"lenv","serviceCnt",pcCnt);
    createNextLinks(pmsConf,pjoResFinal);
    return pjoResFinal;
  }

#ifndef RELY_ON_DB
  // TODO: implement this function in service_internal.c
  // Implement user filtering when not relying on DB
  void filterJobByUser(maps* pmsConf,char** pcaClauseFinal,char* pcaClauseDate){
  }
#endif

  /**
   * Print the filtered jobs list
   *
   * @param ppmsConf the maps containing the settings of the main.cfg file
   * @return the JSON object pointer to the jobs list
   * @see printJobList
   */
  json_object* printFilteredJobList(maps** ppmsConf, map* pmRequestInputs){
    map* pmTmp=getMap(pmRequestInputs,"limit");
    maps* pmsConf=*ppmsConf;
    if(pmTmp!=NULL)
      setMapInMaps(pmsConf,"lenv","serviceCntLimit",pmTmp->value);
    else{
      pmTmp=getMapFromMaps(pmsConf,"limitParam","schema_default");
      if(pmTmp!=NULL)
        setMapInMaps(pmsConf,"lenv","serviceCntLimit",pmTmp->value);
    }
    pmTmp=getMap(pmRequestInputs,"skip");
    if(pmTmp!=NULL)
      setMapInMaps(pmsConf,"lenv","serviceCntSkip",pmTmp->value);
    // Build the SQL Clause
    char* pcaClause=NULL;
    char* pcaClauseFinal=NULL;
    for(int k=0;k<2;k++){
      pmTmp=getMap(pmRequestInputs,statusSearchFields[k]);
      if(pmTmp!=NULL){
        char *pcSavePtr;
        char *tmps = strtok_r(pmTmp->value, ",", &pcSavePtr);
        int iCnt=0;
        while (tmps != NULL){
          for(int l=0;l<5;l++){
            if(strcmp(tmps,oapipStatus[l])==0){
              tmps=wpsStatus[l];
              break;
            }
          }
          maps* pmsLenv=getMaps(pmsConf,"lenv");
          if(pcaClause==NULL){
            pcaClause=(char*)malloc((strlen(statusSearchFieldsReal[k])+strlen(tmps)+10)*sizeof(char));
            sprintf(pcaClause," (%s=$q$%s$q$",statusSearchFieldsReal[k],tmps);
            setMapArray(pmsLenv->content,"servicePidFilter",iCnt,tmps);
          }else{
            char* pcaTmp=zStrdup(pcaClause);
            pcaClause=(char*)realloc(pcaClause,strlen(statusSearchFieldsReal[k])+strlen(tmps)+strlen(pcaTmp)+12);
            sprintf(pcaClause,"%s OR %s=$q$%s$q$",pcaTmp,statusSearchFieldsReal[k],tmps);
            setMapArray(pmsLenv->content,"servicePidFilter",iCnt,tmps);
            free(pcaTmp);
          }
          tmps = strtok_r (NULL, ",", &pcSavePtr);
          iCnt++;
        }
        char* pcaTmp=zStrdup(pcaClause);
        pcaClause=(char*)realloc(pcaClause,strlen(pcaTmp)+3);
        sprintf(pcaClause,"%s) ",pcaTmp);
        free(pcaTmp);
      }
      if(pcaClause!=NULL){
        if(pcaClauseFinal==NULL){
          pcaClauseFinal=(char*)malloc((strlen(pcaClause)+1)*sizeof(char));
          sprintf(pcaClauseFinal,"%s",pcaClause);
        }else{
          char* pcaTmp=zStrdup(pcaClauseFinal);
          pcaClauseFinal=(char*)realloc(pcaClauseFinal,strlen(pcaTmp)+(strlen(pcaClause)+6)*sizeof(char));
          sprintf(pcaClauseFinal,"%s AND %s",pcaClause,pcaTmp);
          free(pcaTmp);
        }
        free(pcaClause);
        pcaClause=NULL;
      }
    }
    // (min/max)Duration should be set '%s second'::interval
    char* pcaClauseMin=NULL;
    pmTmp=getMap(pmRequestInputs,"minDuration");
    if(pmTmp!=NULL){
      setMapInMaps(pmsConf,"lenv","serviceMinDuration",pmTmp->value);
      char *pcSavePtr;
      char *tmps = strtok_r(pmTmp->value, ",", &pcSavePtr);
      while (tmps != NULL){
        if(pcaClauseMin==NULL){
          pcaClauseMin=(char*)malloc((strlen(tmps)+52)*sizeof(char));
          sprintf(pcaClauseMin," ('%s second'::interval <= (end_time - creation_time)",tmps);
        }else{
          char* pcaTmp=zStrdup(pcaClauseMin);
          pcaClauseMin=(char*)realloc(pcaClauseMin,strlen(tmps)+strlen(pcaTmp)+54);
          sprintf(pcaClauseMin,"%s OR '%s second'::interval <= (end_time - creation_time)",pcaTmp,tmps);
          free(pcaTmp);
        }
        tmps = strtok_r (NULL, ",", &pcSavePtr);
      }
      char* pcaTmp=zStrdup(pcaClauseMin);
      pcaClauseMin=(char*)realloc(pcaClauseMin,strlen(pcaTmp)+3);
      sprintf(pcaClauseMin,"%s) ",pcaTmp);
      free(pcaTmp);
      if(pcaClauseFinal!=NULL){
        char* pcaTmp=zStrdup(pcaClauseFinal);
        pcaClauseFinal=(char*)realloc(pcaClauseFinal,strlen(pcaTmp)+(strlen(pcaClauseMin)+6)*sizeof(char));
        sprintf(pcaClauseFinal,"%s AND %s",pcaClauseMin,pcaTmp);
        free(pcaTmp);
      }else{
        pcaClauseFinal=(char*)malloc((strlen(pcaClauseMin)+1)*sizeof(char));
        sprintf(pcaClauseFinal,"%s",pcaClauseMin);
      }
      free(pcaClauseMin);
    }
    char* pcaClauseMax=NULL;
    pmTmp=getMap(pmRequestInputs,"maxDuration");
    if(pmTmp!=NULL){
      setMapInMaps(pmsConf,"lenv","serviceMaxDuration",pmTmp->value);
      char *pcSavePtr;
      char *tmps = strtok_r(pmTmp->value, ",", &pcSavePtr);
      while (tmps != NULL){
        if(pcaClauseMax==NULL){
          pcaClauseMax=(char*)malloc((strlen(tmps)+52)*sizeof(char));
          sprintf(pcaClauseMax," ('%s second'::interval >= (end_time - creation_time)",tmps);
        }else{
          char* pcaTmp=zStrdup(pcaClauseMax);
          pcaClauseMax=(char*)realloc(pcaClauseMax,strlen(tmps)+strlen(pcaTmp)+54);
          sprintf(pcaClauseMax,"%s OR '%s second'::interval >= (end_time - creation_time)",pcaTmp,tmps);
          free(pcaTmp);
        }
        tmps = strtok_r (NULL, ",", &pcSavePtr);
      }
      char* pcaTmp=zStrdup(pcaClauseMax);
      pcaClauseMax=(char*)realloc(pcaClauseMax,strlen(pcaTmp)+3);
      sprintf(pcaClauseMax,"%s) ",pcaTmp);
      free(pcaTmp);
      if(pcaClauseFinal!=NULL){
        char* pcaTmp=zStrdup(pcaClauseFinal);
        pcaClauseFinal=(char*)realloc(pcaClauseFinal,strlen(pcaTmp)+(strlen(pcaClauseMax)+6)*sizeof(char));
        sprintf(pcaClauseFinal,"%s AND %s",pcaClauseMax,pcaTmp);
        free(pcaTmp);
      }else{
        pcaClauseFinal=(char*)malloc((strlen(pcaClauseMax)+1)*sizeof(char));
        sprintf(pcaClauseFinal,"%s",pcaClauseMax);
      }
      free(pcaClauseMax);
    }
    char* pcaClauseType=NULL;
    pmTmp=getMap(pmRequestInputs,"type");
    if(pmTmp!=NULL){
      setMapInMaps(pmsConf,"lenv","serviceType",pmTmp->value);
      char *pcSavePtr;
      char *tmps = strtok_r(pmTmp->value, ",", &pcSavePtr);
      while (tmps != NULL){
        if(strcmp(tmps,"process")==0)
          pcaClauseType=zStrdup(tmps);
        tmps = strtok_r (NULL, ",", &pcSavePtr);
      }
      if(pcaClauseType!=NULL){
        if(pcaClauseFinal!=NULL){
          char* pcaTmp=zStrdup(pcaClauseFinal);
          pcaClauseFinal=(char*)realloc(pcaClauseFinal,
                                        (strlen(pcaTmp)+18)*sizeof(char));
          sprintf(pcaClauseFinal,"%s AND itype='json'",
                  pcaTmp);
          free(pcaTmp);
        }else{
          pcaClauseFinal=(char*)malloc(13*sizeof(char));
          sprintf(pcaClauseFinal,"itype='json'");
        }
        free(pcaClauseType);
      }
    }
    // datetime parameter
    char* pcaClauseDate=NULL;
    pmTmp=getMap(pmRequestInputs,"datetime");
    if(pmTmp!=NULL){
      if(strstr(pmTmp->value,"/")!=NULL){
        char *pcSavePtr0;
        char *tmps0 = strtok_r(pmTmp->value, ",", &pcSavePtr0);
        int iCnt0=0;
        while(tmps0 != NULL){
          if(iCnt0>0){
            char *pcSavePtr;
            char *tmps = strtok_r(tmps0, "/", &pcSavePtr);
            int iCnt=0;
            while (tmps != NULL){
              if(iCnt==0 && tmps0[0]!='/'){
                if(strlen(tmps)>2){
                  pcaClauseDate=(char*)malloc((strlen(tmps)+30+1)*sizeof(char));
                  sprintf(pcaClauseDate,"creation_time>='%s'::timestamptz",tmps);
                }
              }else{
                if(strlen(tmps)>2){
                  if(pcaClauseDate==NULL){
                    pcaClauseDate=(char*)malloc((strlen(tmps)+30+1)*sizeof(char));
                    sprintf(pcaClauseDate,"creation_time<='%s'::timestamptz",tmps);
                  }else{
                    char* pcaTmp=zStrdup(pcaClauseDate);
                    pcaClauseDate=(char*)realloc(pcaClauseDate,(strlen(pcaTmp)+strlen(tmps)+35+1)*sizeof(char));
                    sprintf(pcaClauseDate,"%s AND creation_time<='%s'::timestamptz",pcaTmp,tmps);
                    free(pcaTmp);
                  }
                }
              }
              iCnt++;
              tmps = strtok_r (NULL, "/", &pcSavePtr);
            }
          }
          iCnt0++;
          tmps0 = strtok_r (NULL, ",", &pcSavePtr0);
        }
      }else{
        char *pcSavePtr0;
        char *tmps0 = strtok_r(pmTmp->value, ",", &pcSavePtr0);
        int iCnt0=0;
        while(tmps0 != NULL){
          if(iCnt0>0){
            pcaClauseDate=(char*)malloc((strlen(tmps0)+29+1)*sizeof(char));
            sprintf(pcaClauseDate,"creation_time='%s'::timestamptz",tmps0);
          }
          iCnt0++;
          tmps0 = strtok_r (NULL, ",", &pcSavePtr0);
        }
      }
      if(pcaClauseFinal==NULL)
        pcaClauseFinal=zStrdup(pcaClauseDate);
      else{
        char* pcaTmp=zStrdup(pcaClauseFinal);
        pcaClauseFinal=(char*)realloc(pcaClauseFinal,
                                      (strlen(pcaTmp)+strlen(pcaClauseDate)+6)*sizeof(char));
        sprintf(pcaClauseFinal,"%s AND %s",
                pcaTmp,pcaClauseDate);
        free(pcaTmp);
      }
      free(pcaClauseDate);
    }
    if(pcaClauseFinal==NULL){
      pcaClauseFinal=zStrdup("true");
    }else{
      fflush(stderr);
    }
    // Filter jobs list based on the potential user_id
    filterJobByUser(pmsConf,&pcaClauseFinal,pcaClauseDate);
    map *schema=getMapFromMaps(pmsConf,"database","schema");
    if(pcaClauseFinal!=NULL && schema!=NULL){
      char* pcaTmp=(char*) malloc((strlen(pcaClauseFinal)+
                                    strlen(schema->value)+
                                    98+1)
                                  *sizeof(char));
      sprintf(pcaTmp,
              "select replace(replace(array(select ''''||uuid||'''' "
              "from  %s.services where %s)::text,'{',''),'}','')",
              schema->value,pcaClauseFinal);
      free(pcaClauseFinal);
      char* tmp1=
#ifdef RELY_ON_DB
        runSqlQuery(pmsConf,pcaTmp)
#else
        NULL
#endif
        ;
      free(pcaTmp);
      if(tmp1!=NULL){
        setMapInMaps(pmsConf,"lenv","selectedJob",tmp1);
        free(tmp1);
      }
#ifdef RELY_ON_DB
      else{
        setMapInMaps(pmsConf,"lenv","selectedJob","-1");
      }
#endif
    }else{
      free(pcaClauseFinal);
    }
    return printJobList(ppmsConf);
  }
  
  /**
   * Print the result of an execution
   *
   * @param ppmsConf the maps containing the settings of the main.cfg file
   * @param s service pointer to metadata
   * @param result outputs of the service
   * @param iRes the status of execution SERVICE_FAILED/SERVICE_SUCCEEDED
   * @return the JSON object pointer to the result
   */
  json_object* printJResult(maps** ppmsConf,service* s,maps* result,int iRes){
    maps* conf=*ppmsConf;
    if(iRes==SERVICE_FAILED){
      char* pcaTmp=produceErrorMessage(conf);
      map* pamTmp=createMap("message",pcaTmp);
      free(pcaTmp);
      map* pmTmp=getMapFromMaps(conf,"lenv","code");
      if(pmTmp!=NULL)
        addToMap(pamTmp,"code",pmTmp->value);
      printExceptionReportResponseJ(&conf,pamTmp);
      freeMap(&pamTmp);
      free(pamTmp);
      return NULL;
    }
    map* pmMode=getMapFromMaps(conf,"request","response");
    if(pmMode!=NULL && strncasecmp(pmMode->value,"raw",3)==0){
      // raw response
      maps* resu=result;

      // when the response is SERVICE DEPLOYED
      // we want to return a 201 status code
      if (iRes == SERVICE_DEPLOYED){
          setMapInMaps(conf,"headers","Status","201 Service Deployed");
      }
      printRawdataOutputs(ppmsConf,s,resu);
      map* pmError=getMapFromMaps(conf,"lenv","error");
      if(pmError!=NULL && strncasecmp(pmError->value,"true",4)==0){
        printExceptionReportResponseJ(&conf,pmError);
      }
      return NULL;
    }else{
      // not raw response
      json_object* eres1=json_object_new_object();
      map* pmIOAsArray=getMapFromMaps(conf,"openapi","io_as_array");
      json_object* eres=NULL;
      if(pmIOAsArray!=NULL && strncasecmp(pmIOAsArray->value,"true",4)==0)
        eres=json_object_new_array();
      else
        eres=json_object_new_object();
      maps* resu=result;
      int itn=0;
      while(resu!=NULL) {
        json_object* res1=json_object_new_object();
        if(pmIOAsArray!=NULL && strncasecmp(pmIOAsArray->value,"true",4)==0)
          json_object_object_add(res1,"id",json_object_new_string(resu->name));
        map* tmpMap=getMap(resu->content,"mimeType");
        json_object* pjoRes3=json_object_new_object();
        if(tmpMap!=NULL)
          json_object_object_add(pjoRes3,"mediaType",json_object_new_string(tmpMap->value));
        else{
          json_object_object_add(pjoRes3,"mediaType",json_object_new_string("text/plain"));
        }
        map* pmTransmissionMode=NULL;
        map* pmPresence=getMap(resu->content,"inRequest");
        if(((tmpMap=getMap(resu->content,"value"))!=NULL ||
          (getMap(resu->content,"generated_file"))!=NULL) &&
          (pmPresence!=NULL && strncmp(pmPresence->value,"true",4)==0)){
          map* pmTmp=NULL;
          if((pmTmp=getMap(resu->content,"transmissionMode"))!=NULL) {
            pmTransmissionMode=getMap(resu->content,"transmissionMode");
            if(strcmp(pmTmp->value,"value")==0) {
              map* pmTmp2=getMap(resu->content,"mimeType");
              if(pmTmp2!=NULL && strstr(pmTmp2->value,"json")!=NULL){
                json_object *jobj = NULL;
                map *gfile=getMap(resu->content,"generated_file");
                if(gfile!=NULL){
                  gfile=getMap(resu->content,"expected_generated_file");
                  if(gfile==NULL){
                    gfile=getMap(resu->content,"generated_file");
                  }
                  jobj=json_readFile(conf,gfile->value);
                }
                else{
                  int slen = 0;
                  enum json_tokener_error jerr;
                  struct json_tokener* tok=json_tokener_new();
                  do {
                    slen = strlen(tmpMap->value);
                    jobj = json_tokener_parse_ex(tok, tmpMap->value, slen);
                  } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
                  if (jerr != json_tokener_success) {
                    fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
                    json_tokener_free(tok);
                    return eres1;
                  }
                  if (tok->char_offset < slen){
                    json_tokener_free(tok);
                    return eres1;
                  }
                  json_tokener_free(tok);
                }
                json_object_object_add(pjoRes3,"encoding",json_object_new_string("utf-8"));
                json_object_object_add(res1,"value",jobj);
              }else{
                map* tmp1=getMapFromMaps(conf,"main","tmpPath");
                map *gfile=getMap(resu->content,"generated_file");
                if(gfile!=NULL){
                  gfile=getMap(resu->content,"expected_generated_file");
                  if(gfile==NULL){
                    gfile=getMap(resu->content,"generated_file");
                  }
                  FILE* pfData=fopen(gfile->value,"rb");
                  if(pfData!=NULL){
                    zStatStruct f_status;
                    int iStat=zStat(gfile->value, &f_status);
                    if(f_status.st_size>0){
                      char* pcaTmp=(char*)malloc((f_status.st_size+1)*sizeof(char));
                      size_t sLength = fread(pcaTmp,1,f_status.st_size,pfData);
                      pcaTmp[f_status.st_size]=0;
                      fclose(pfData);
                      outputSingleJsonComplexRes(conf,resu,res1,pjoRes3,pcaTmp,f_status.st_size);
                      free(pcaTmp);
                    }
                  }
                }else{
                  json_object_object_add(pjoRes3,"encoding",json_object_new_string("utf-8"));
                  map* pmTmp0=getMap(resu->content,"crs");
                  if(pmTmp0!=NULL){
                    map* pmTmp2=getMap(resu->content,"value");
                    json_object *jobj = NULL;
                    const char *mystring = NULL;
                    int slen = 0;
                    enum json_tokener_error jerr;
                    struct json_tokener* tok=json_tokener_new();
                    do {
                      mystring = pmTmp2->value;  // get JSON string, e.g. read from file, etc...
                      slen = strlen(mystring);
                      jobj = json_tokener_parse_ex(tok, mystring, slen);
                    } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
                    if (jerr != json_tokener_success) {
                      map* pamError=createMap("code","InvalidParameterValue");
                      const char* pcTmpErr=json_tokener_error_desc(jerr);
                      const char* pccErr=_("ZOO-Kernel cannot parse your POST data: %s");
                      char* pcaMessage=(char*)malloc((strlen(pcTmpErr)+strlen(pccErr)+1)*sizeof(char));
                      sprintf(pcaMessage,pccErr,pcTmpErr);
                      addToMap(pamError,"message",pcaMessage);
                      printExceptionReportResponseJ(&conf,pamError);
                      fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
                      json_tokener_free(tok);
                      return NULL;
                    }
                    if (tok->char_offset < slen){
                      map* pamError=createMap("code","InvalidParameterValue");
                      const char* pcTmpErr="None";
                      const char* pccErr=_("ZOO-Kernel cannot parse your POST data: %s");
                      char* pcaMessage=(char*)malloc((strlen(pcTmpErr)+strlen(pccErr)+1)*sizeof(char));
                      sprintf(pcaMessage,pccErr,pcTmpErr);
                      addToMap(pamError,"message",pcaMessage);
                      printExceptionReportResponseJ(&conf,pamError);
                      fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
                      json_tokener_free(tok);
                      return NULL;
                    }
                    json_tokener_free(tok);
                    json_object_object_add(res1,"bbox",jobj);
                    json_object_object_add(res1,"crs",json_object_new_string(pmTmp0->value));
                  }
                  else{
                    //tmpMap=getMap(resu->content,"value");
                    if(getMap(resu->content,"mimeType")!=NULL){
                      map* pmSize=getMap(resu->content,"size");
                      long len=0;
                      if(pmSize!=NULL)
                        len=atol(pmSize->value);
                      else
                        len=strlen(tmpMap->value);
                      outputSingleJsonComplexRes(conf,resu,res1,pjoRes3,tmpMap->value,len);
                    }
                    else{
                      outputSingleJsonLiteralData(conf,resu,res1,"value",tmpMap->value);
                    }
                  }
                }
                if(getMapFromMaps(conf,"oas","noformat")!=NULL){
                  // This is an option to not use the format key and allocate
                  // a the format object keys to the result itself
                  map* pmTmp0=getMap(resu->content,"crs");
                  if(pmTmp0==NULL){
                    json_object_object_foreach(pjoRes3, pcKey, pjoVal) {
                      if(strncasecmp(pcKey,pccFields[0],4)==0 ||
                          strncasecmp(pcKey,pccFields[3],8)==0 ||
                          strncasecmp(pcKey,pccFields[6],15)==0)
                        json_object_object_add(res1,pccFields[3],json_object_new_string(json_object_get_string(pjoVal)));
                      if(strncasecmp(pcKey,pccFields[1],4)==0 ||
                          strncasecmp(pcKey,pccFields[4],8)==0 ||
                          strncasecmp(pcKey,pccFields[7],15)==0)
                        json_object_object_add(res1,pccFields[4],json_object_new_string(json_object_get_string(pjoVal)));
                      if(strncasecmp(pcKey,pccFields[2],4)==0 ||
                          strncasecmp(pcKey,pccFields[5],8)==0 ||
                          strncasecmp(pcKey,pccFields[8],15)==0)
                        json_object_object_add(res1,pccFields[5],json_object_new_string(json_object_get_string(pjoVal)));
                    }
                  }
                }
              }
            }
            else{
#ifdef USE_MS
              map* pmTest=getMap(resu->content,"useMapserver");
              if(pmTest!=NULL){
                map* geodatatype=getMap(resu->content,"geodatatype");
                map* nbFeatures;
                setMapInMaps(conf,"lenv","state","out");
                setReferenceUrl(ppmsConf,resu);
                nbFeatures=getMap(resu->content,"nb_features");
                geodatatype=getMap(resu->content,"geodatatype");
                if((nbFeatures!=NULL && atoi(nbFeatures->value)==0) ||
                  (geodatatype!=NULL && strcasecmp(geodatatype->value,"other")==0)){
                  //error=1;
                  //res=SERVICE_FAILED;
                }else{
                  map* pmUrl=getMap(resu->content,"Reference");
                  json_object_object_add(res1,"href",
                      json_object_new_string(pmUrl->value));
                  map* pmMimeType=getMap(resu->content,"requestedMimeType");
                  if(pmMimeType!=NULL)
                    json_object_object_add(res1,pccFields[0],json_object_new_string(pmMimeType->value));
                }
              }else{
#endif
                char *pcaFileUrl=produceFileUrl(s,conf,resu,NULL,itn);
                itn++;
                if(pcaFileUrl!=NULL){
                  json_object_object_add(res1,"href",
                      json_object_new_string(pcaFileUrl));
                  free(pcaFileUrl);
                }
                if(getMapFromMaps(conf,"oas","noformat")!=NULL){
                  json_object_object_foreach(pjoRes3, pcKey, pjoVal) {
                    if(strncasecmp(pcKey,pccFields[0],4)==0 ||
                      strncasecmp(pcKey,pccFields[3],8)==0 ||
                      strncasecmp(pcKey,pccFields[6],15)==0)
                      json_object_object_add(res1,pccFields[0],json_object_new_string(json_object_get_string(pjoVal)));
                  }
                }
#ifdef USE_MS
              }
#endif
            }
          }
          // Add format to res1 for complex output except json based ones
          map* pmTmp1=getMap(resu->content,"mimeType");
          if(pmTmp1!=NULL && getMapFromMaps(conf,"oas","noformat")==NULL){
            if(pmTmp1!=NULL && strstr(pmTmp1->value,"json")==NULL)
              json_object_object_add(res1,"format",pjoRes3);
          }
          else
            json_object_put(pjoRes3);
          if(pmIOAsArray!=NULL && strncasecmp(pmIOAsArray->value,"true",4)==0){
            json_object_array_add(eres,res1);
          }else{
            map* pmDataType=getMap(resu->content,"dataType");
            if(pmDataType==NULL || (pmTransmissionMode!=NULL && strncasecmp(pmTransmissionMode->value,"reference",9)==0) )
              json_object_object_add(eres,resu->name,res1);
            else{
              if(((tmpMap=getMap(resu->content,"value"))!=NULL)){
                if(eres==NULL)
                  eres=json_object_new_object();
                outputSingleJsonLiteralData(conf,resu,eres,resu->name,tmpMap->value);
              }
            }
          }
        }
        resu=resu->next;
      }
      const char* jsonStr =
        json_object_to_json_string_ext(eres,JSON_C_TO_STRING_NOSLASHESCAPE);
      map *tmpPath = getMapFromMaps (conf, "main", "tmpPath");
      map *cIdentifier = getMapFromMaps (conf, "lenv", "oIdentifier");
      map *pmUsid = getMapFromMaps (conf, "lenv", "usid");
      if(tmpPath!=NULL){
        char *pcaTmp=(char*)malloc((strlen(tmpPath->value)+strlen(cIdentifier->value)+strlen(pmUsid->value)+8)*sizeof(char));
        sprintf(pcaTmp,"%s/%s_%s.json",
          tmpPath->value,cIdentifier->value,pmUsid->value);
        zStatStruct zsFStatus;
        int iStat=zStat(pcaTmp, &zsFStatus);
        if(iStat==0 && zsFStatus.st_size>0){
          map* tmpPath1 = getMapFromMaps (conf, "main", "tmpUrl");
          char* pcaTmpUrl=(char*)malloc((strlen(tmpPath1->value)+strlen(cIdentifier->value)+strlen(pmUsid->value)+8)*sizeof(char));;
          sprintf(pcaTmpUrl,"%s/%s_%s.json",tmpPath1->value,
            cIdentifier->value,pmUsid->value);
          if(getMapFromMaps(conf,"lenv","gs_location")==NULL)
            setMapInMaps(conf,"headers","Location",pcaTmpUrl);
          free(pcaTmpUrl);
        }
        free(pcaTmp);
      }
      if(iRes==3){
        map* mode=getMapFromMaps(conf,"request","mode");
        if(mode!=NULL && strncasecmp(mode->value,"async",5)==0) {
            setMapInMaps(conf, "headers", "Status", "201 Created");
        }
        else {
            setMapInMaps(conf, "headers", "Status", "200 Ok");
        }
      } else{
        setMapInMaps(conf,"headers","Status","500 Issue running your service");
      }
      return eres;
    }
  }

  /**
   * Append required literal data output
   * 
   * @param pmsConf maps pointer to the main configuration maps
   * @param pmsResult maps pointer to the output
   * @param pjoRes json_object pointer to which the field should be added
   * @param pccFieldName the field name to be added
   * @param pcValue char pointer to the value
   */
  void outputSingleJsonLiteralData(maps* pmsConf,maps* pmsResult,json_object* pjoRes,const char* pccFieldName,char* pcValue){
    map* pmType=getMap(pmsResult->content,"dataType");
    if(pmType!=NULL){
      if(strncasecmp(pmType->value,"integer",7)==0){
        json_object_object_add(pjoRes,pccFieldName,json_object_new_int(atoi(pcValue)));
      }else if(strncasecmp(pmType->value,"float",5)==0 ||
          strncasecmp(pmType->value,"double",6)==0 ||
          strncasecmp(pmType->value,"decimal",7)==0){
        json_object_object_add(pjoRes,pccFieldName,json_object_new_double(atof(pcValue)));
      }else if(strncasecmp(pmType->value,"boolean",7)==0){
        if(strncasecmp(pcValue,"true",4)==0 ||
            strcmp(pcValue,"1")==0)
          json_object_object_add(pjoRes,pccFieldName,json_object_new_boolean(1));
        else
          json_object_object_add(pjoRes,pccFieldName,json_object_new_boolean(0));
      }else
        json_object_object_add(pjoRes,pccFieldName,json_object_new_string(pcValue));
    }
    else
      json_object_object_add(pjoRes,pccFieldName,json_object_new_string(pcValue));
  }

  /**
   * Append required field to Json objects for a complex output
   * 
   * @param conf maps pointer to the main configuration maps
   * @param resu maps pointer to the output
   * @param pjoRes1 json_object pointer to which the value field should be added
   * @param pjoRes3 json_object pointer to the format object associated with pjoRes1
   * @param pcValue char pointer to the value to be allocated
   * @param lLen length of pcValue
   */
  void outputSingleJsonComplexRes(maps* conf,maps* resu,json_object* pjoRes1,json_object* pjoRes3,char* pcValue, long lLen){
    map* pmMediaType=getMap(resu->content,"mediaType");
    if(pmMediaType==NULL)
      pmMediaType=getMap(resu->content,"mimeType");
    if(pmMediaType!=NULL &&
       (strstr(pmMediaType->value,"json")!=NULL || strstr(pmMediaType->value,"text")!=NULL || strstr(pmMediaType->value,"kml")!=NULL)){
      if(strstr(pmMediaType->value,"json")!=NULL){
        json_object* pjoaTmp=parseJson(conf,pcValue);
        json_object_object_add(pjoRes1,"value",pjoaTmp);
      }
      else
        json_object_object_add(pjoRes1,"value",json_object_new_string(pcValue));
      json_object_object_add(pjoRes3,"mediaType",json_object_new_string(pmMediaType->value));
    }
    else{
      char* pcaB64=base64(pcValue,lLen);
      json_object_object_add(pjoRes1,"value",json_object_new_string(pcaB64));
      json_object_object_add(pjoRes3,"encoding",json_object_new_string("base64"));
      if(pmMediaType!=NULL)
        json_object_object_add(pjoRes3,"mediaType",json_object_new_string(pmMediaType->value));
      free(pcaB64);
    }
  }

  /**
   * Create a link object with ref, type and href.
   *
   * @param conf maps pointer to the main configuration maps
   * @param rel char pointer defining the rel attribute
   * @param type char pointer defining the type attribute
   * @param href char pointer defining the href attribute
   * @return json_object pointer to the link object
   */
  json_object* createALink(maps* conf,const char* rel,const char* type,const char* href){
    json_object* val=json_object_new_object();
    json_object_object_add(val,"rel",
                            json_object_new_string(rel));
    json_object_object_add(val,"type",
                            json_object_new_string(type));
    json_object_object_add(val,"href",json_object_new_string(href));
    return val;
  }

  /**
   * Create the next links
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param result an integer (>0 for adding the /result link)
   * @param obj the JSON object pointer to add the links to
   * @return 0
   */
  int createNextLinks(maps* pmsConf,json_object* obj){
    json_object* pjoRes=json_object_new_array();

    map *tmpPath = getMapFromMaps (pmsConf, "openapi", "rootUrl");
    map *pmLenvPath = getMapFromMaps (pmsConf, "lenv", "path");

    char *Url0=(char*) malloc((strlen(tmpPath->value)+
                              strlen(pmLenvPath->value)+2)*sizeof(char));
    sprintf(Url0,"%s/%s",
              tmpPath->value,
              pmLenvPath->value);
    json_object_array_add(pjoRes,createALink(pmsConf,"self","application/json",Url0));

    map* pmHtml=getMapFromMaps(pmsConf,"openapi","partial_html_support");
    if(pmHtml!=NULL && strcmp(pmHtml->value,"true")==0){
      char *Url1=(char*) malloc((strlen(tmpPath->value)+
                                strlen(pmLenvPath->value)+7)*sizeof(char));
      sprintf(Url1,"%s/%s.html",
	      tmpPath->value,
	      pmLenvPath->value);
      json_object_array_add(pjoRes,createALink(pmsConf,"alternate","text/html",Url1));
      free(Url1);
    }

    map* pmTmp=getMapFromMaps(pmsConf,"lenv","serviceCntSkip");
    map* pmLimit=getMapFromMaps(pmsConf,"lenv","serviceCntLimit");
    map* pmCnt=getMapFromMaps(pmsConf,"lenv","serviceCnt");
    if(pmTmp!=NULL && atoi(pmTmp->value)>0){
      char* pcaTmp=(char*) malloc((strlen(Url0)+strlen(pmLimit->value)+25)*sizeof(char));
      int cnt=atoi(pmTmp->value)-atoi(pmLimit->value);
      if(cnt>=0)
        sprintf(pcaTmp,"%s?limit=%s&skip=%d",Url0,pmLimit->value,cnt);
      else
        sprintf(pcaTmp,"%s?limit=%s&skip=%d",Url0,pmLimit->value,0);
      json_object_array_add(pjoRes,createALink(pmsConf,"prev","application/json",pcaTmp));
      free(pcaTmp);
    }
    if(getMapFromMaps(pmsConf,"lenv","serviceCntNext")!=NULL){
      char* pcaTmp=(char*) malloc((strlen(Url0)+strlen(pmLimit->value)+strlen(pmCnt->value)+15)*sizeof(char));
      int cnt=(pmTmp!=NULL?atoi(pmTmp->value):0)+atoi(pmLimit->value);
      sprintf(pcaTmp,"%s?limit=%s&skip=%d",Url0,pmLimit->value,cnt);
      json_object_array_add(pjoRes,createALink(pmsConf,"next","application/json",pcaTmp));
      free(pcaTmp);
    }
    json_object_object_add(obj,"links",pjoRes);
    free(Url0);
    map* pmNb=getMapFromMaps(pmsConf,"lenv","serviceCnt");
    if(pmNb!=NULL)
      json_object_object_add(obj,"numberTotal",json_object_new_int(atoi(pmNb->value)));

    return 0;
  }

  /**
   * Fetch the current jobid (can be usid or gs_usid in the lenv section).
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   */
  map* fetchUsid(maps* pmsConf){
    map* pmUsid=getMapFromMaps(pmsConf,"lenv","usid");
    if(pmUsid!=NULL){
      pmUsid = getMapFromMaps (pmsConf, "lenv", "gs_usid");
      if(pmUsid==NULL)
        pmUsid = getMapFromMaps (pmsConf, "lenv", "usid");
    }else
      pmUsid = getMapFromMaps (pmsConf, "lenv", "gs_usid");
    return pmUsid;
  }

  /**
   * Detect if there is a _logs.cfg file associated with the execution.
   * If yes, add the links defined in the section to the standard links associated
   * with a job statusInfo.
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoObj  the JSON object pointer to add the links to
   */
  void createStatusLinksFromLogs(maps* pmsConf,json_object* pjoObj){
    map* pmTmpPath=getMapFromMaps(pmsConf,"main","tmpPath");
    map* pmUsid=fetchUsid(pmsConf);
    char* pcaLogsPath=(char*)malloc((strlen(pmUsid->value)+strlen(pmTmpPath->value)+11)*sizeof(char));
    sprintf(pcaLogsPath,"%s/%s_logs.cfg",pmTmpPath->value,pmUsid->value);
    maps *pmsLogs = (maps *) malloc (MAPS_SIZE);
    pmsLogs->content = NULL;
    pmsLogs->child = NULL;
    pmsLogs->next = NULL;
    if (conf_read (pcaLogsPath,pmsLogs) != 2){
      map* pmLength=getMap(pmsLogs->content,"length");
      int iLen=1;
      const char* ppccKeys[]={
        "url",
        "title",
        "rel"
      };
      if(pmLength!=NULL)
        iLen=atoi(pmLength->value);
      for(int iI=0;iI<iLen;iI++){
        json_object* pjoCurrentLink=json_object_new_object();
        for(int iJ=0;iJ<3;iJ++){
          map* pmValue=getMapArray(pmsLogs->content,ppccKeys[iJ],iI);
          if(pmValue!=NULL){
            // Add the key to the current link
            if(strncmp(ppccKeys[iJ],"url",3)!=0)
              json_object_object_add(pjoCurrentLink,ppccKeys[iJ],
                  json_object_new_string(pmValue->value));
            else
              json_object_object_add(pjoCurrentLink,"href",
                  json_object_new_string(pmValue->value));
          }
        }
        json_object_object_add(pjoCurrentLink,"type",
                  json_object_new_string("text/plain"));
        json_object_array_add(pjoObj,pjoCurrentLink);
      }
      freeMaps(&pmsLogs);
      free(pmsLogs);
      pmsLogs=NULL;
    }
    free(pcaLogsPath);
  }

  /**
   * Create the status links
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param result an integer (>0 for adding the /result link)
   * @param obj the JSON object pointer to add the links to
   * @return 0
   */
  int createStatusLinks(maps* pmsConf,int result,json_object* obj){
    json_object* pjoRes=json_object_new_array();
    map *tmpPath = getMapFromMaps (pmsConf, "openapi", "rootUrl");
    map *pmUsid = fetchUsid(pmsConf);
    char *Url0=(char*) malloc((strlen(tmpPath->value)+
			       strlen(pmUsid->value)+18)*sizeof(char));
    int needResult=-1;
    char *message, *status;
    sprintf(Url0,"%s/jobs/%s",
	    tmpPath->value,
	    pmUsid->value);
    if(getMapFromMaps(pmsConf,"lenv","gs_location")==NULL)
      setMapInMaps(pmsConf,"headers","Location",Url0);
      json_object* val=json_object_new_object();
      json_object_object_add(val,"title",
			   json_object_new_string(_("Status location")));
      // The relation type should be set to monitor
      json_object_object_add(val,"rel",
			   json_object_new_string("monitor"));
      json_object_object_add(val,"type",
			   json_object_new_string("application/json"));
      json_object_object_add(val,"href",json_object_new_string(Url0));
      json_object_array_add(pjoRes,val);
      if(result>0){
        free(Url0);
      Url0=(char*) malloc((strlen(tmpPath->value)+
			  strlen(pmUsid->value)+
			  25)*sizeof(char));
      sprintf(Url0,"%s/jobs/%s/results",
	    tmpPath->value,
	    pmUsid->value);
      json_object* val1=json_object_new_object();
      json_object_object_add(val1,"title",
			    json_object_new_string(_("Result location")));
      map* pmTmp=getMapFromMaps(pmsConf,"/jobs/{jobID}/results","rel");
      if(pmTmp!=NULL)
	      json_object_object_add(val1,"rel",
			      json_object_new_string(pmTmp->value));
      else
	      json_object_object_add(val1,"rel",
			      json_object_new_string("results"));
      json_object_object_add(val1,"type",
			   json_object_new_string("application/json"));
      json_object_object_add(val1,"href",json_object_new_string(Url0));
      json_object_array_add(pjoRes,val1);
    }
    createStatusLinksFromLogs(pmsConf,pjoRes);
    json_object_object_add(obj,"links",pjoRes);
    free(Url0);
    return 0;
  }

  /**
   * Get the status file path
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @return a char* containing the full status file path
   */
  char* json_getStatusFilePath(maps* pmsConf){
    map *tmpPath = getMapFromMaps (pmsConf, "main", "tmpPath");
    map *pmUsid = fetchUsid(pmsConf);
    char *tmp1=(char*) malloc((strlen(tmpPath->value)+
			       strlen(pmUsid->value)+14)*sizeof(char));
    sprintf(tmp1,"%s/%s_status.json",
	    tmpPath->value,
	    pmUsid->value);
    return tmp1;
  }

  /**
   * Get the result path
   *
   * @param pmsConf the conf maps containing the main.cfg settings
   * @param jobId the job identifier
   * @return the full path to the result file
   */
  char* getResultPath(maps* pmsConf,char* jobId){
    map *pmTmpPath = getMapFromMaps (pmsConf, "main", "tmpPath");
    char *pcaUrl=(char*) malloc((strlen(pmTmpPath->value)+
				 strlen(jobId)+8)*sizeof(char));
    sprintf(pcaUrl,"%s/%s.json",pmTmpPath->value,jobId);
    return pcaUrl;
  }

  /**
   * Parse a json string
   *
   * @param pmsConf the conf maps containing the main.cfg settings
   * @param myString the string containing the json content
   * @return a pointer to the created json_object
   */
  json_object* parseJson(maps* pmsConf,char* myString){
    json_object *pjoaObj = NULL;
    enum json_tokener_error jerr;
    struct json_tokener* tok=json_tokener_new();
    int slen = 0;
    do {
      slen = strlen(myString);
      pjoaObj = json_tokener_parse_ex(tok, myString, slen);
    } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
    if (jerr != json_tokener_success) {
      setMapInMaps(pmsConf,"lenv","message",json_tokener_error_desc(jerr));
      fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
      ZOO_DEBUG("FAILED");
      json_tokener_free(tok);
      return NULL;
    }
    if (tok->char_offset < slen){
      fprintf(stderr, "Error parsing json\n");
      ZOO_DEBUG("FAILED");
      json_tokener_free(tok);
      return NULL;
    }
    json_tokener_free(tok);
    return pjoaObj;
  }

  /**
   * Read a json file
   *
   * @param pmsConf the conf maps containing the main.cfg settings
   * @praam filePath the file path to read
   * @return a pointer to the created json_object
   */
  json_object* json_readFile(maps* pmsConf,char* filePath){
    json_object *pjoaObj = NULL;
    zStatStruct zsFStatus;
    int iStat=zStat(filePath, &zsFStatus);
    if(iStat==0 && zsFStatus.st_size>0){
      FILE* cdat=fopen(filePath,"rb");
      if(cdat!=NULL){
        char* pcaMyString=(char*)malloc((zsFStatus.st_size+1)*sizeof(char));
        size_t sLength = fread(pcaMyString,1,zsFStatus.st_size,cdat);
        pcaMyString[zsFStatus.st_size]=0;
        fclose(cdat);
        pjoaObj=parseJson(pmsConf,pcaMyString);
        free(pcaMyString);
      }
      else
        return NULL;
    }else
      return NULL;
    return pjoaObj;
  }

  /**
   * Fetch the statusFields attributes
   *
   * @param pmsConf the conf maps containing the main.cfg settings
   * @param pmSessId the map pointer to the session id
   * @param pjoRes the object to store metadata informations
   * @param iStatus integer of the current execution status
   * @see statusFields,
   */
  void json_getStatusAttributes(maps* pmsConf, map* pmSessId, json_object* pjoRes,int iStatus){
#ifdef RELY_ON_DB
    for(int iCounter=0;iCounter<6;iCounter++){
      char* pcaTmp=_getStatusField(pmsConf,pmSessId->value,statusFieldsC[iCounter]);
      if(pcaTmp!=NULL && strcmp(pcaTmp,"-1")!=0){
        json_object_object_add(pjoRes,statusFields[iCounter],json_object_new_string(pcaTmp));
      }
      if(pcaTmp!=NULL && strncmp(pcaTmp,"-1",2)==0)
        free(pcaTmp);
    }
#else
    json_object_object_add(pjoRes,statusFields[0],json_object_new_string("process"));
    map* pmTmpPath=getMapFromMaps(pmsConf,"main","tmpPath");
    char* pcaLenvPath=(char*)malloc((strlen(pmSessId->value)+strlen(pmTmpPath->value)+11)*sizeof(char));
    sprintf(pcaLenvPath,"%s/%s_lenv.cfg",pmTmpPath->value,pmSessId->value);
    maps *pmsaLenv = (maps *) malloc (MAPS_SIZE);
    pmsaLenv->content = NULL;
    pmsaLenv->child = NULL;
    pmsaLenv->next = NULL;
    if (conf_read (pcaLenvPath,pmsaLenv) != 2){
      zStatStruct zssLenv_status,zssSid_status;
      int iStat=zStat(pcaLenvPath, &zssLenv_status);
      char* pcaSidPath=(char*)malloc((strlen(pmSessId->value)+strlen(pmTmpPath->value)+6)*sizeof(char));
      sprintf(pcaSidPath,"%s/%s.sid",pmTmpPath->value,pmSessId->value);
      int iStat1=zStat(pcaSidPath, &zssSid_status);
      map* pmPid=getMapFromMaps(pmsaLenv,"lenv","Identifier");
      json_object_object_add(pjoRes,statusFields[1],json_object_new_string(pmPid->value));
      char *pcaTime=(char*)malloc((TIME_SIZE+1)*sizeof(char));
      size_t st_Time=strftime(pcaTime,TIME_SIZE, zDateFormat, localtime(&zssSid_status.st_mtime) );
      json_object_object_add(pjoRes,statusFields[2],json_object_new_string(pcaTime));
      json_object_object_add(pjoRes,statusFields[3],json_object_new_string(pcaTime));
      if(iStatus==SERVICE_SUCCEEDED || iStatus==SERVICE_FAILED){
        char* pcaStatusPath=(char*)malloc((strlen(pmSessId->value)+strlen(pmTmpPath->value)+6)*sizeof(char));
        sprintf(pcaStatusPath,"%s/%s.json",pmTmpPath->value,pmSessId->value);
        zStatStruct zssStatus_status;
        int iS2=zStat(pcaStatusPath, &zssStatus_status);
        st_Time=strftime(pcaTime,TIME_SIZE, zDateFormat, localtime(&zssStatus_status.st_mtime) );
        json_object_object_add(pjoRes,statusFields[4],json_object_new_string(pcaTime));
        json_object_object_add(pjoRes,statusFields[5],json_object_new_string(pcaTime));
      }else{
        st_Time=strftime(pcaTime,TIME_SIZE, zDateFormat, localtime(&zssLenv_status.st_mtime) );
        json_object_object_add(pjoRes,statusFields[5],json_object_new_string(pcaTime));
      }
      freeMaps(&pmsaLenv);
      free(pmsaLenv);
      pmsaLenv=NULL;
    }
    free(pcaLenvPath);
#endif
  }

  /**
   * Create the json object for job status
   *
   * @param pmsConf the conf maps containing the main.cfg settings
   * @param status integer
   * @return the created json_object
   */
  json_object* createStatus(maps* pmsConf,int status){
    int hasMessage=0;
    int needResult=0;
    int isDismissed=0;
    const char *rstatus;
    char *message;
    // Create statusInfo JSON object
    // cf. https://github.com/opengeospatial/wps-rest-binding/blob/master/core/
    //     openapi/schemas/statusInfo.yaml
    json_object* pjoRes=json_object_new_object();
    switch(status){
    case SERVICE_ACCEPTED:
      {
        message=_("ZOO-Kernel accepted to run your service!");
        rstatus="accepted";
        break;
      }
    case SERVICE_STARTED:
      {
        message=_("ZOO-Kernel is currently running your service!");
        map* pmStatus=getMapFromMaps(pmsConf,"lenv","message");
        if(pmStatus!=NULL){
          setMapInMaps(pmsConf,"lenv","gs_message",pmStatus->value);
          message=zStrdup(pmStatus->value);
          hasMessage=1;
        }
        map* mMap=NULL;
        if((mMap=getMapFromMaps(pmsConf,"lenv","status"))!=NULL){
          map* pmMap1=getMapFromMaps(pmsConf,"lenv","PercentCompleted");
          if(pmMap1!=NULL && atoi(pmMap1->value)>atoi(mMap->value))
            json_object_object_add(pjoRes,"progress",json_object_new_int(atoi(pmMap1->value)));
          else
            json_object_object_add(pjoRes,"progress",json_object_new_int(atoi(mMap->value)));
        }
        else if((mMap=getMapFromMaps(pmsConf,"lenv","PercentCompleted"))!=NULL)
          json_object_object_add(pjoRes,"progress",json_object_new_int(atoi(mMap->value)));
        rstatus="running";
        break;
      }
    case SERVICE_PAUSED:
      {
        message=_("ZOO-Kernel pause your service!");
        rstatus="paused";
        break;
      }
    case SERVICE_SUCCEEDED:
      {
        message=_("ZOO-Kernel successfully run your service!");
        rstatus="successful";
        setMapInMaps(pmsConf,"lenv","PercentCompleted","100");
        needResult=1;
        break;
      }
    case SERVICE_DISMISSED:
      {
        message=_("ZOO-Kernel successfully dismissed your service!");
        rstatus="dismissed";
        needResult=1;
        isDismissed=1;
        break;
      }
    default:
      {
        map* pmTmp=getMapFromMaps(pmsConf,"lenv","force");
        if(pmTmp==NULL || strncasecmp(pmTmp->value,"false",5)==0){
          char* pcaTmp=json_getStatusFilePath(pmsConf);
          json_object* pjoStatus=json_readFile(pmsConf,pcaTmp);
          free(pcaTmp);
          if(pjoStatus!=NULL){
            json_object* pjoMessage=NULL;
            if(json_object_object_get_ex(pjoStatus,"message",&pjoMessage)!=FALSE){
              message=(char*)json_object_get_string(pjoMessage);
            }
          }
          // TODO: Error
        }else{
          map* mMap=getMapFromMaps(pmsConf,"lenv","gs_message");
          if(mMap!=NULL)
            setMapInMaps(pmsConf,"lenv","message",mMap->value);
          message=produceErrorMessage(pmsConf);
          hasMessage=1;
          needResult=-1;
        }
        rstatus="failed";
        break;
      }
    }
    setMapInMaps(pmsConf,"lenv","message",message);

    map *pmUsid = getMapFromMaps (pmsConf, "lenv", "usid");
    if(pmUsid!=NULL){
      pmUsid = getMapFromMaps (pmsConf, "lenv", "gs_usid");
      if(pmUsid==NULL)
        pmUsid = getMapFromMaps (pmsConf, "lenv", "usid");
    }else
      pmUsid = getMapFromMaps (pmsConf, "lenv", "gs_usid");
    if(pmUsid!=NULL){
      json_object_object_add(pjoRes,"id",json_object_new_string(pmUsid->value));
      json_object_object_add(pjoRes,"jobID",json_object_new_string(pmUsid->value));
      if(isDismissed==0)
        json_getStatusAttributes(pmsConf,pmUsid,pjoRes,status);
    }
    json_object_object_add(pjoRes,"status",json_object_new_string(rstatus));
    json_object_object_add(pjoRes,"message",json_object_new_string(message));
    if(status!=SERVICE_DISMISSED)
      createStatusLinks(pmsConf,needResult,pjoRes);
    else{
      json_object* pjoRes1=json_object_new_array();
      map *pmRootUrl = getMapFromMaps (pmsConf, "openapi", "rootUrl");
      char *Url0=(char*) malloc((strlen(pmRootUrl->value)+17)*sizeof(char));
      sprintf(Url0,"%s/jobs",pmRootUrl->value);
      json_object* val=json_object_new_object();
      json_object_object_add(val,"title",
			     json_object_new_string(_("The job list for the current process")));
      json_object_object_add(val,"rel",
			     json_object_new_string("parent"));
      json_object_object_add(val,"type",
			     json_object_new_string("application/json"));
      json_object_object_add(val,"href",json_object_new_string(Url0));
      json_object_array_add(pjoRes1,val);
      free(Url0);
      json_object_object_add(pjoRes,"links",pjoRes1);
    }
    if(hasMessage>0)
      free(message);
    return pjoRes;
  }
  
  /**
   * Create the status file 
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param status an integer (SERVICE_ACCEPTED,SERVICE_STARTED...)
   * @return an integer (0 in case of success, 1 in case of failure)
   */
  int createStatusFile(maps* pmsConf,int status){
    json_object* pjoRes=createStatus(pmsConf,status);
    char* tmp1=json_getStatusFilePath(pmsConf);
    FILE* foutput1=fopen(tmp1,"w+");
    if(foutput1!=NULL){
      const char* jsonStr1=json_object_to_json_string_ext(pjoRes,JSON_C_TO_STRING_NOSLASHESCAPE);
      fprintf(foutput1,"%s",jsonStr1);
      fclose(foutput1);
    }else{
      // Failure
      setMapInMaps(pmsConf,"lenv","message",_("Unable to store the statusInfo!"));
      free(tmp1);
      return 1;
    }
    free(tmp1);
    return 0;
  }

  /**
   * Create the status file 
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @return an integer (0 in case of success, 1 in case of failure)
   */
  int json_getStatusFile(maps* pmsConf){
    /*char* tmp1=json_getStatusFilePath(pmsConf);
    FILE* statusFile=fopen(tmp1,"rb");*/

    map* tmpMap=getMapFromMaps(pmsConf,"lenv","gs_usid");
    if(tmpMap!=NULL){
      char* pcaTmp=_getStatus(pmsConf,tmpMap->value);
      if(pcaTmp!=NULL && strncmp(pcaTmp,"-1",2)!=0){
        char *pcaTmp1=zStrdup(pcaTmp);
        char *pcaTmp0=zStrdup(strstr(pcaTmp,"|")+1);
        free(pcaTmp);
        pcaTmp1[strlen(pcaTmp1)-strlen(pcaTmp0)-1]='\0';
        setMapInMaps(pmsConf,"lenv","PercentCompleted",pcaTmp1);
        setMapInMaps(pmsConf,"lenv","gs_message",pcaTmp0);
        free(pcaTmp0);
        free(pcaTmp1);
        return 0;
      }else{
        return 1;
      }
    }
    return 1;
  }

  /**
   * Produce the JSON object for api info object
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoRes the JSON object for the api info
   */
  void produceApiInfo(maps* pmsConf,json_object* pjoRes,json_object* pjoRes5){
    json_object *pjoRes1=json_object_new_object();
    map* tmpMap=getMapFromMaps(pmsConf,"provider","providerName");
    if(tmpMap!=NULL){
      json_object *pjoRes2=json_object_new_object();
      json_object_object_add(pjoRes2,"name",json_object_new_string(tmpMap->value));
      tmpMap=getMapFromMaps(pmsConf,"provider","addressElectronicMailAddress");
      if(tmpMap!=NULL)
        json_object_object_add(pjoRes2,"email",json_object_new_string(tmpMap->value));
      tmpMap=getMapFromMaps(pmsConf,"provider","providerSite");
      if(tmpMap!=NULL)
        json_object_object_add(pjoRes2,"url",json_object_new_string(tmpMap->value));
      json_object_object_add(pjoRes1,"contact",pjoRes2);
    }
    maps* pmsTmp=getMaps(pmsConf,"identification");
    addDescription(pmsTmp->content,0,pjoRes1,pjoRes5);
    tmpMap=getMapFromMaps(pmsConf,"openapi","rootUrl");
    if(tmpMap!=NULL)
      json_object_object_add(pjoRes5,"url",json_object_new_string(tmpMap->value));
    tmpMap=getMapFromMaps(pmsConf,"identification","title");
    if(tmpMap!=NULL){
      json_object_object_add(pjoRes1,"title",json_object_new_string(_(tmpMap->value)));
      json_object_object_add(pjoRes5,"description",json_object_new_string(_(tmpMap->value)));
    }
    tmpMap=getMapFromMaps(pmsConf,"identification","logo");
    if(tmpMap!=NULL){
      json_object* pjoLogo=json_object_new_object();
      json_object_object_add(pjoLogo,"url",json_object_new_string(_(tmpMap->value)));;
      tmpMap=getMapFromMaps(pmsConf,"identification","logoBgColor");
      if(tmpMap!=NULL)
        json_object_object_add(pjoLogo,"backgroundColor",json_object_new_string(_(tmpMap->value)));;
      tmpMap=getMapFromMaps(pmsConf,"identification","logoAltText");
      if(tmpMap!=NULL)
      json_object_object_add(pjoLogo,"altText",json_object_new_string(_(tmpMap->value)));;
      json_object_object_add(pjoRes1,"x-logo",pjoLogo);
    }
    json_object_object_add(pjoRes1,"version",json_object_new_string(ZOO_VERSION));
    tmpMap=getMapFromMaps(pmsConf,"identification","keywords");
    if(tmpMap!=NULL){
      char *pcSavePtr;
      char *tmps = strtok_r (tmpMap->value, ",", &pcSavePtr);
      json_object *pjoRes3=json_object_new_array();
      while (tmps != NULL){
        json_object_array_add(pjoRes3,json_object_new_string(tmps));
        tmps = strtok_r (NULL, ",", &pcSavePtr);
      }
      json_object_object_add(pjoRes1,"x-keywords",pjoRes3);
    }
    maps* pmsProvider=getMaps(pmsConf,"provider");
    if(pmsProvider!=NULL){
      json_object *pjoRes4=json_object_new_object();
      map* cItem=pmsProvider->content;
      while(cItem!=NULL){
        json_object_object_add(pjoRes4,cItem->name,json_object_new_string(cItem->value));
        cItem=cItem->next;
      }
      json_object_object_add(pjoRes1,"x-ows-servicecontact",pjoRes4);
    }

    json_object *pjoRes4=json_object_new_object();
    tmpMap=getMapFromMaps(pmsConf,"openapi","license_name");
    if(tmpMap!=NULL){
      json_object_object_add(pjoRes4,"name",json_object_new_string(tmpMap->value));
      tmpMap=getMapFromMaps(pmsConf,"openapi","license_url");
      if(tmpMap!=NULL){
        json_object_object_add(pjoRes4,"url",json_object_new_string(tmpMap->value));
      }
    }
    json_object_object_add(pjoRes1,"license",pjoRes4);

    json_object_object_add(pjoRes,"info",pjoRes1);
  }

  // addResponse(pmUseContent,cc3,vMap,tMap,"200","successful operation");
  void addResponse(const map* useContent,json_object* pjoRes,const map* pmSchema,const map* pmType,const char* code,const char* msg){
    json_object *pjoaSchema=json_object_new_object();
    if(pmSchema!=NULL)
      json_object_object_add(pjoaSchema,"$ref",json_object_new_string(pmSchema->value));
    if(useContent!=NULL && strncasecmp(useContent->value,"true",4)!=0){
      json_object_object_add(pjoRes,code,pjoaSchema);
    }else{
      json_object *cc0=json_object_new_object();
      if(pmSchema!=NULL)
        json_object_object_add(cc0,"schema",pjoaSchema);
      json_object *pjoaContent=json_object_new_object();
      if(pmType!=NULL)
        json_object_object_add(pjoaContent,pmType->value,cc0);
      else
        json_object_object_add(pjoaContent,"application/json",cc0);
      json_object *cc2=json_object_new_object();
      if(strcmp(code,"204")!=0)
        json_object_object_add(cc2,"content",pjoaContent);
      else
        json_object_put(pjoaContent);
      json_object_object_add(cc2,"description",json_object_new_string(msg));
      json_object_object_add(pjoRes,code,cc2);
    }
  }

  void addParameter(maps* pmsConf,const char* oName,const char* fName,const char* in,json_object* pjoRes){
    maps* pmsTmp=getMaps(pmsConf,oName);
    json_object *pjoRes8=json_object_new_object();
    int hasSchema=0;
    if(pmsTmp!=NULL){
      map* tmpMap=getMap(pmsTmp->content,"title");
      if(tmpMap!=NULL)
        json_object_object_add(pjoRes8,"x-internal-summary",json_object_new_string(_(tmpMap->value)));
      tmpMap=getMap(pmsTmp->content,"schema");
      if(tmpMap!=NULL){
        json_object_object_add(pjoRes8,"$ref",json_object_new_string(tmpMap->value));
        hasSchema=1;
      }
      addDescription(pmsTmp->content,0,pjoRes8,NULL);
      tmpMap=getMap(pmsTmp->content,"example");
      if(tmpMap!=NULL)
        json_object_object_add(pjoRes8,"example",json_object_new_string(tmpMap->value));
      tmpMap=getMap(pmsTmp->content,"required");
      if(tmpMap!=NULL){
        if(strcmp(tmpMap->value,"true")==0)
          json_object_object_add(pjoRes8,"required",json_object_new_boolean(TRUE));
        else
          json_object_object_add(pjoRes8,"required",json_object_new_boolean(FALSE));
      }
      else
        if(hasSchema==0)
          json_object_object_add(pjoRes8,"required",json_object_new_boolean(FALSE));
      map* pmTmp=getMap(pmsTmp->content,"in");
      if(pmTmp!=NULL)
        json_object_object_add(pjoRes8,"in",json_object_new_string(pmTmp->value));
      else
        if(hasSchema==0)
          json_object_object_add(pjoRes8,"in",json_object_new_string(in));
      tmpMap=getMap(pmsTmp->content,"name");
      if(tmpMap!=NULL)
        json_object_object_add(pjoRes8,"name",json_object_new_string(tmpMap->value));
      else
        if(hasSchema==0)
          json_object_object_add(pjoRes8,"name",json_object_new_string(fName));
      tmpMap=getMap(pmsTmp->content,"schema");
      if(tmpMap==NULL){
        json_object *pjoRes6=json_object_new_object();
        tmpMap=getMap(pmsTmp->content,"type");
        char* pcaType=NULL;
        if(tmpMap!=NULL){
          json_object_object_add(pjoRes6,"type",json_object_new_string(tmpMap->value));
          pcaType=zStrdup(tmpMap->value);
        }
        else
          json_object_object_add(pjoRes6,"type",json_object_new_string("string"));

        tmpMap=getMap(pmsTmp->content,"enum");
        if(tmpMap!=NULL){
          char *pcSavePtr12;
          char *tmps12 = strtok_r (tmpMap->value, ",", &pcSavePtr12);
          json_object *pjoEnum=json_object_new_array();
          while(tmps12!=NULL){
            json_object_array_add(pjoEnum,json_object_new_string(tmps12));
            tmps12 = strtok_r (NULL, ",", &pcSavePtr12);
          }
          json_object_object_add(pjoRes6,"enum",pjoEnum);
        }

        pmTmp=pmsTmp->content;
        while(pmTmp!=NULL){
          if(strstr(pmTmp->name,"schema_")!=NULL){
            if(pcaType!=NULL && strncmp(pcaType,"integer",7)==0){
              json_object_object_add(pjoRes6,strstr(pmTmp->name,"schema_")+7,json_object_new_int(atoi(pmTmp->value)));
            }else
              json_object_object_add(pjoRes6,strstr(pmTmp->name,"schema_")+7,json_object_new_string(pmTmp->value));
          }
          pmTmp=pmTmp->next;
        }
        if(pcaType!=NULL)
          free(pcaType);
        json_object_object_add(pjoRes8,"schema",pjoRes6);
      }
    }
    json_object_object_add(pjoRes,fName,pjoRes8);
  }
  
  /**
   * Produce the JSON object for api parameter
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoRes the JSON object to populate with the parameters
   */
  void produceApiParameters(maps* pmsConf,json_object* pjoRes){
    json_object *pjoRes9=json_object_new_object();
    maps* pmsTmp=getMaps(pmsConf,"{id}");
    map* pmTmp2=getMapFromMaps(pmsConf,"openapi","parameters");
    char *pcSavePtr12;
    char *tmps12 = strtok_r (pmTmp2->value, ",", &pcSavePtr12);
    while(tmps12!=NULL){
      char* pcaId=(char*) malloc((strlen(tmps12)+3)*sizeof(char));
      sprintf(pcaId,"{%s}",tmps12);
      addParameter(pmsConf,pcaId,tmps12,"path",pjoRes9);
      free(pcaId);
      tmps12 = strtok_r (NULL, ",", &pcSavePtr12);
    }    
    pmTmp2=getMapFromMaps(pmsConf,"openapi","header_parameters");
    if(pmTmp2!=NULL){
      char *pcSavePtr13;
      char *tmps13 = strtok_r (pmTmp2->value, ",", &pcSavePtr13);
      while(tmps13!=NULL){
        char* pcaId=zStrdup(tmps13);
        addParameter(pmsConf,pcaId,pcaId,"header",pjoRes9);
        free(pcaId);
        tmps13 = strtok_r (NULL, ",", &pcSavePtr13);
      }
    }
    json_object_object_add(pjoRes,"parameters",pjoRes9);
    maps* pmResponses=getMaps(pmsConf,"responses");
    if(pmResponses!=NULL){
      json_object *cc=json_object_new_object();
      map* pmLen=getMap(pmResponses->content,"length");
      int iLen=atoi(pmLen->value);
      map* pmUseContent=getMapFromMaps(pmsConf,"openapi","use_content");
      for(int i=0;i<iLen;i++){
        map* cMap=getMapArray(pmResponses->content,"code",i);
        map* vMap=getMapArray(pmResponses->content,"schema",i);
        map* tMap=getMapArray(pmResponses->content,"type",i);
        map* tMap0=getMapArray(pmResponses->content,"title",i);
        if(vMap!=NULL)
          addResponse(pmUseContent,cc,vMap,tMap,cMap->value,(tMap0==NULL)?"successful operation":tMap0->value);
      }
      json_object_object_add(pjoRes,"responses",cc);
    }
  }

  /**
   * Add a securityScheme to the OpenAPI components/securitySchemes object
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param fName the key to add to the res json object
   * @param pjoRes the json object to store the content
   */
  void addSecurityScheme(maps* pmsConf,const char* fName,json_object* pjoRes){
    json_object *poJsonObject=json_object_new_object();
    maps* pmsTmp=getMaps(pmsConf,"osecurity");
    // Get type
    map* pmTmp=getMap(pmsTmp->content,"type");
    if(pmTmp!=NULL){
      json_object_object_add(poJsonObject,"type",json_object_new_string(pmTmp->value));
      if(strcasecmp(pmTmp->value,"openIdConnect")==0){
        pmTmp=getMap(pmsTmp->content,"openIdConnectUrl");
        if(pmTmp!=NULL)
          json_object_object_add(poJsonObject,"openIdConnectUrl",json_object_new_string(pmTmp->value));
            }else{
        if(strcasecmp(pmTmp->value,"oauth2")==0){
          pmTmp=getMap(pmsTmp->content,"authorizationUrl");
          // TODO: continue integration of oauth2 security scheme
        }
      }
    }
    // Get scheme if any
    pmTmp=getMap(pmsTmp->content,"scheme");
    if(pmTmp!=NULL)
      json_object_object_add(poJsonObject,"scheme",json_object_new_string(pmTmp->value));
    // Get format if any
    pmTmp=getMap(pmsTmp->content,"format");
    if(pmTmp!=NULL)
      json_object_object_add(poJsonObject,"bearerFormat",json_object_new_string(pmTmp->value));
    // Get the in key if any (for type=apiKey)
    pmTmp=getMap(pmsTmp->content,"in");
    if(pmTmp!=NULL)
      json_object_object_add(poJsonObject,"in",json_object_new_string(pmTmp->value));
    // Get the name of the apiKey
    pmTmp=getMap(pmsTmp->content,"oname");
    if(pmTmp!=NULL)
      json_object_object_add(poJsonObject,"name",json_object_new_string(pmTmp->value));
    json_object_object_add(pjoRes,fName,poJsonObject);
  }

  /**
   * Produce the JSON object for api securitySchemes
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoRes the JSON object to populate with the parameters
   */
  void produceApiSecuritySchemes(maps* pmsConf,json_object* pjoRes){
    map* pmTmp=getMapFromMaps(pmsConf,"osecurity","name");
    if(pmTmp!=NULL){
      json_object *pjoRes9=json_object_new_object();
      char *pcSavePtr12;
      char *tmps12 = strtok_r (pmTmp->value, ",", &pcSavePtr12);
      while(tmps12!=NULL){
        addSecurityScheme(pmsConf,tmps12,pjoRes9);
        tmps12 = strtok_r (NULL, ",", &pcSavePtr12);
      }
      json_object_object_add(pjoRes,"securitySchemes",pjoRes9);
    }
  }

  /**
   * Produce the JSON object for api schemas
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoRes the JSON object to populate with the parameters
   */
  void produceApiSchemas(maps* pmsConf,json_object* pjoRes){
    map* pmTmp=getMapFromMaps(pmsConf,"schemas","length");
    if(pmTmp!=NULL){
      json_object *pjoRes9=json_object_new_object();
      maps* pmsTmp=getMaps(pmsConf,"schemas");
      int iLimit=atoi(pmTmp->value);
      for(int iCnt=0;iCnt<iLimit;iCnt++){
        map* pmName=getMapArray(pmsTmp->content,"name",iCnt);
        map* pmValue=getMapArray(pmsTmp->content,"value",iCnt);
        if(pmName!=NULL && pmValue!=NULL){
          json_object *pjoRes10=json_object_new_object();
          json_object_object_add(pjoRes10,"$ref",json_object_new_string(pmValue->value));
          json_object_object_add(pjoRes9,pmName->value,pjoRes10);
        }
      }
      json_object_object_add(pjoRes,"schemas",pjoRes9);
    }
  }

  void produceApiComponents(maps*pmsConf,json_object* pjoRes){
    json_object* pjoRes1=json_object_new_object();
    produceApiParameters(pmsConf,pjoRes1);
    produceApiSchemas(pmsConf,pjoRes1);
    produceApiSecuritySchemes(pmsConf,pjoRes1);
    json_object_object_add(pjoRes,"components",pjoRes1);
  }

  /**
   * Generate an unique operationId attribute for a given endpoints
   *
   * @param pmsConf maps pointing to the main confif maps
   * @param pmsContent the maps containing endpoint's metadata information
   * @param iIndex the index for map array, 0 otherwise
   * @param pjoRes the json object to store the operationId attribute
   */
  void produceOperationId(maps* pmsConf,maps* pmsContent,int iIndex,json_object* pjoRes){
    map* pmMethod=getMapArray(pmsContent->content,"method",iIndex);
    if(pmMethod!=NULL){
      char *pcaOperationId = (char*) malloc((strlen(pmsContent->name)+
			strlen(pmMethod->value)+
			2)*sizeof(char));
      char* pcaTmp=zStrdup(pmsContent->name);
      _translateChar(pcaTmp,'/','_');
      _translateChar(pcaTmp,'{','_');
      _translateChar(pcaTmp,'}','_');
      sprintf(pcaOperationId,"%s_%s",pmMethod->value,pcaTmp);
      json_object_object_add(pjoRes,"operationId",json_object_new_string(pcaOperationId));
      free(pcaTmp);
      free(pcaOperationId);
    }
  }
  
  /**
   * Produce the JSON object for /api
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoRes the JSON object to populate
   */
  void produceApi(maps* pmsConf,json_object* pjoRes){
    // TODO: add 401 Gone for dismiss request
    json_object *pjoRes9=json_object_new_object();
    json_object *pjoRes10=json_object_new_object();
    maps* pmsTmp1=getMaps(pmsConf,"{id}");
    produceApiComponents(pmsConf,pjoRes);
    json_object *pjoRes4=json_object_new_object();
    map* pmTmp=getMapFromMaps(pmsConf,"/api","type");
    if(pmTmp!=NULL){
      char* pcaTmp=(char*)malloc((15+strlen(pmTmp->value))*sizeof(char));
      sprintf(pcaTmp,"%s;charset=UTF-8",pmTmp->value);
      setMapInMaps(pmsConf,"headers","Content-Type",pcaTmp);
      free(pcaTmp);
    }
    else
      setMapInMaps(pmsConf,"headers","Content-Type","application/vnd.oai.openapi+json;version=3.0;charset=UTF-8");

    produceApiInfo(pmsConf,pjoRes,pjoRes4);
    map* tmpMap=getMapFromMaps(pmsConf,"provider","providerName");

    tmpMap=getMapFromMaps(pmsConf,"openapi","version");
    if(tmpMap!=NULL)
      json_object_object_add(pjoRes,"openapi",json_object_new_string(tmpMap->value));
    else
      json_object_object_add(pjoRes,"openapi",json_object_new_string("3.0.2"));

    tmpMap=getMapFromMaps(pmsConf,"openapi","tags");
    maps* pmsTmp=getMaps(pmsConf,"tags");
    if(tmpMap!=NULL){
      json_object *pjoRes5=json_object_new_array();
      char *pcSavePtr;
      char *tmps = strtok_r (tmpMap->value, ",", &pcSavePtr);
      int iCnt=0;
      while(tmps!=NULL){
        json_object *pjoRes6=json_object_new_object();
        json_object_object_add(pjoRes6,"name",json_object_new_string(tmps));
        if(pmsTmp!=NULL){
          map* pmDesc=getMapArray(pmsTmp->content,"value",iCnt);
          if(pmDesc!=NULL)
            json_object_object_add(pjoRes6,"description",json_object_new_string(pmDesc->value));
          else
          json_object_object_add(pjoRes6,"description",json_object_new_string(tmps));
        }else
          json_object_object_add(pjoRes6,"description",json_object_new_string(tmps));
        json_object_array_add(pjoRes5,pjoRes6);
        tmps = strtok_r (NULL, ",", &pcSavePtr);
        iCnt++;
      }
      json_object_object_add(pjoRes,"tags",pjoRes5);
    }

    tmpMap=getMapFromMaps(pmsConf,"openapi","paths");
    if(tmpMap!=NULL){
      json_object *pjoRes5=json_object_new_object();
      char *pcSavePtr;
      char *tmps = strtok_r (tmpMap->value, ",", &pcSavePtr);
      int cnt=0;
      maps* pmsTmp;
      json_object *paths=json_object_new_object();
      while (tmps != NULL){
        pmsTmp=getMaps(pmsConf,tmps+1);
        json_object *method=json_object_new_object();
        if(pmsTmp!=NULL){
          if(getMap(pmsTmp->content,"length")==NULL)
            addToMap(pmsTmp->content,"length","1");
          map* len=getMap(pmsTmp->content,"length");
          for(int i=0;i<atoi(len->value);i++){
            json_object *methodc=json_object_new_object();
            map* cMap=getMapArray(pmsTmp->content,"method",i);
            map* vMap=getMapArray(pmsTmp->content,"title",i);
            map* pmName=getMapArray(pmsTmp->content,"pname",i);
            if(vMap!=NULL)
              json_object_object_add(methodc,"summary",json_object_new_string(_(vMap->value)));
            vMap=getMapArray(pmsTmp->content,"secured",i);
            if(vMap!=NULL){
              json_object *poSecurity=json_object_new_array();
              json_object *poScopes=json_object_new_array();
              map* pmSMap=getMapArray(pmsTmp->content,"scopes",i);
              char *pcSaveptr;
              char *pcTmps = strtok_r (tmpMap->value, ",", &pcSaveptr);
              while(pcTmps!=NULL){
                json_object_array_add(poScopes,json_object_new_string(pcTmps));
                pcTmps = strtok_r (NULL, ",", &pcSaveptr);
              }
              json_object *poSecurityItem=json_object_new_object();
              json_object_object_add(poSecurityItem,vMap->value,poScopes);
              json_object_array_add(poSecurity,poSecurityItem);
              json_object_object_add(methodc,"security",poSecurity);
            }
            addDescription(pmsTmp->content,i,methodc,NULL);
            vMap=getMapArray(pmsTmp->content,"tags",i);
            if(vMap!=NULL){
              json_object *cc=json_object_new_array();
              json_object_array_add(cc,json_object_new_string(vMap->value));
              json_object_object_add(methodc,"tags",cc);
              if(pmName==NULL)
                pmName=getMapArray(pmsTmp->content,"operationId",i);
              if(pmName!=NULL){
                char* pcaOperationId=(char*)malloc((strlen(vMap->value)+strlen(pmName->value)+1)*sizeof(char));
                sprintf(pcaOperationId,"%s%s",vMap->value,pmName->value);
                json_object_object_add(methodc,"operationId",json_object_new_string(pcaOperationId));
                free(pcaOperationId);
              }
              else{
                produceOperationId(pmsConf,pmsTmp,i,methodc);
              }
            }
            json_object *responses=json_object_new_object();
            json_object *pjoaResponse=json_object_new_object();
            map* pmUseContent=getMapFromMaps(pmsConf,"openapi","use_content");
            map* pmCode=getMapArray(pmsTmp->content,"code",i);
            vMap=getMapArray(pmsTmp->content,"schema",i);
            if(vMap!=NULL){
              map* tMap=getMapArray(pmsTmp->content,"type",i);
              if(pmCode!=NULL)
                addResponse(pmUseContent,pjoaResponse,vMap,tMap,pmCode->value,"successful operation");
              else
                addResponse(pmUseContent,pjoaResponse,vMap,tMap,"200","successful operation");
              vMap=getMapArray(pmsTmp->content,"eschema",i);
              if(pmCode==NULL && vMap!=NULL && cMap!=NULL && strncasecmp(cMap->value,"post",4)==0)
                addResponse(pmUseContent,pjoaResponse,vMap,tMap,"201","successful operation");
            }else{
              map* tMap=getMapFromMaps(pmsConf,tmps,"type");
              map* pMap=createMap("ok","true");
              if(pmCode!=NULL)
                addResponse(pMap,pjoaResponse,vMap,tMap,pmCode->value,"successful operation");
              else
                addResponse(pMap,pjoaResponse,vMap,tMap,"200","successful operation");
              if(pmCode==NULL && cMap!=NULL && strncasecmp(cMap->value,"post",4)==0)
                addResponse(pmUseContent,pjoaResponse,vMap,tMap,"201","successful operation");
              freeMap(&pMap);
              free(pMap);
            }
            vMap=getMapArray(pmsTmp->content,"ecode",i);
            if(vMap!=NULL){
              char *pcSavePtr0;
              char *tmps1 = strtok_r (vMap->value, ",", &pcSavePtr0);
              while(tmps1!=NULL){
                char* pcaTmp=(char*)malloc((strlen(tmps1)+24)*sizeof(char));
                sprintf(pcaTmp,"#/components/responses/%s",tmps1);
                vMap=createMap("ok",pcaTmp);
                map* pMap=createMap("ok","false");
                //TODO: fix successful operation with correct value
                addResponse(pMap,pjoaResponse,vMap,NULL,tmps1,"successful operation");
                tmps1 = strtok_r (NULL, ",", &pcSavePtr0);
              }
            }else{
              if(strstr(tmps,"{id}")!=NULL){
                vMap=getMapFromMaps(pmsConf,"exception","schema");
                map* tMap=getMapFromMaps(pmsConf,"exception","type");
                if(vMap!=NULL)
                  addResponse(pmUseContent,pjoaResponse,vMap,tMap,"404",
                    (strstr(tmps,"{jobID}")==NULL)?"The process with id {id} does not exist.":"The process with id {id} or job with id {jobID} does not exist.");
              }
            }
            json_object_object_add(methodc,"responses",pjoaResponse);
            vMap=getMapArray(pmsTmp->content,"parameters",i);
            if(vMap!=NULL){
              char *pcSavePtr0;
              char *tmps1 = strtok_r (vMap->value, ",", &pcSavePtr0);
              json_object *cc2=json_object_new_array();
              int cnt=0;
              while(tmps1!=NULL){
                char* pcaTmp=(char*)malloc((strlen(tmps1)+2)*sizeof(char));
                sprintf(pcaTmp,"#%s",tmps1);
                json_object *cc1=json_object_new_object();
                json_object_object_add(cc1,"$ref",json_object_new_string(pcaTmp));
                json_object_array_add(cc2,cc1);
                free(pcaTmp);
                cnt++;
                tmps1 = strtok_r (NULL, ",", &pcSavePtr0);
              }
              json_object_object_add(methodc,"parameters",cc2);
            }
            if(/*i==1 && */cMap!=NULL &&
              (strncasecmp(cMap->value,"post",4)==0
              ||
              strncasecmp(cMap->value,"put",3)==0)){
                int iLen=1;
                map* pmRequestBodyLength=getMapArray(pmsTmp->content,"requestBody_length",i);
                if(pmRequestBodyLength!=NULL)
                  iLen=atoi(pmRequestBodyLength->value);
                  char* pcaKey0=getMapArrayKey(pmsTmp->content,"requestBody",i);
                  int iCnt=0;
                  int iCounter=0;
                  json_object *pjoaContent=NULL;
                  json_object *cc2=NULL;
                  for(;iCounter<iLen;iCounter++){
                    map* pmRequestBodyName=getMapArray(pmsTmp->content,pcaKey0,iCounter);
                    maps* pmsTmp1=getMaps(pmsConf,(pmRequestBodyName==NULL?"requestBody":pmRequestBodyName->value));
                    if(pmsTmp1!=NULL){
                      vMap=getMap(pmsTmp1->content,"schema");
                      if(vMap!=NULL){
                        int iIsRef=-1;
                        json_object *cc=json_object_new_object();
                        if(strcasecmp(vMap->value,"string")!=0)
                          json_object_object_add(cc,"$ref",json_object_new_string(vMap->value));
                        else
                          json_object_object_add(cc,"type",json_object_new_string(vMap->value));
                        json_object *cc0=json_object_new_object();
                        json_object_object_add(cc0,"schema",cc);
                        // Add examples from here
                        char* pcaKey1=getMapArrayKey(pmsTmp->content,"examples",i);
                        char actmp0[12];
                        sprintf(actmp0,"%d",iCounter);
                        map* pmExample=getMapArray(pmsTmp->content,pcaKey1,iCounter);
                        if(pmExample==NULL){
                          free(pcaKey1);
                          pcaKey1=getMapArrayKey(pmsTmp->content,"examples_ref",i);
                          pmExample=getMapArray(pmsTmp->content,pcaKey1,iCounter);
                          iIsRef=1;
                        }
                        if(pmExample!=NULL) {
                          char* pcSavePtr;
                          map* pmExamplesPath=getMapFromMaps(pmsConf,"openapi","examplesPath");
                          json_object* prop5=json_object_new_object();
                          char *tmps = strtok_r (pmExample->value, ",", &pcSavePtr);
                          while(tmps!=NULL && strlen(tmps)>0){
                            json_object* prop6=json_object_new_object();
                            json_object* pjoExempleItem=json_object_new_object();
                            char* pcaExempleFile=NULL;
                            if(iIsRef<0)
                              if(pmName!=NULL){
                                pcaExempleFile=(char*)malloc((strlen(pmExamplesPath->value)+strlen(pmName->value)+strlen(tmps)+3)*sizeof(char));
                                sprintf(pcaExempleFile,"%s/%s/%s",pmExamplesPath->value,pmName->value,tmps);
                              }else{
                                pcaExempleFile=(char*)malloc((strlen(pmExamplesPath->value)+strlen(tmps)+2)*sizeof(char));
                                sprintf(pcaExempleFile,"%s/%s",pmExamplesPath->value,tmps);
                              }
                            char actmp[12];
                            sprintf(actmp,"%d",iCnt);
                            char* pcaTmp;
                            char* pcaKey2=getMapArrayKey(pmsTmp->content,"examples_summary",i);
                            map* pmSummary=getMapArray(pmsTmp->content,pcaKey2,iCnt);
                            if(pmSummary!=NULL){
                              pcaTmp=(char*)malloc((strlen(actmp)+strlen(pmSummary->value)+strlen(_("Example "))+3)*sizeof(char));
                              sprintf(pcaTmp,"%s%s: %s",_("Example "),actmp,pmSummary->value);
                            }
                            else{
                              pcaTmp=(char*)malloc((strlen(actmp)+strlen(_("Example "))+3)*sizeof(char));
                              sprintf(pcaTmp,"%s%s",_("Example "),actmp);
                            }
                            free(pcaKey2);
                            json_object_object_add(pjoExempleItem,"summary",json_object_new_string(pcaTmp));
                            if(iIsRef<0){
                              // Apply filter by Content-Type
                              if(strstr(pcaExempleFile,"cwl")==NULL){
                                json_object* pjoValue=json_readFile(pmsConf,pcaExempleFile);
                                json_object_object_add(pjoExempleItem,"value",pjoValue);
                              }else{
                                // Read the cwl and insert it as example item
                                zStatStruct zssStatus;
                                int iStat=zStat(pcaExempleFile, &zssStatus);
                                if(iStat==0){
                                  char* pcaFcontent = NULL;
                                  long long llFsize=0;
                                  FILE* f=fopen(pcaExempleFile,"rb");
                                  if(f!=NULL){
                                    pcaFcontent=(char*)malloc(sizeof(char)*(zssStatus.st_size+1));
                                    size_t sLength = fread(pcaFcontent,zssStatus.st_size,1,f);
                                    pcaFcontent[zssStatus.st_size]=0;
                                    fclose(f);
                                    char* pcaCurrentValue=(char*)malloc((zssStatus.st_size+4)*sizeof(char));
                                    sprintf(pcaCurrentValue,"|-\n%s",pcaFcontent);
                                    json_object_object_add(pjoExempleItem,"value",json_object_new_string(pcaFcontent));
                                    free(pcaFcontent);
                                    free(pcaCurrentValue);
                                  }
                                }
                              }
                            }else{
                              json_object_object_add(pjoExempleItem,"externalValue",json_object_new_string(tmps));
                            }
                            json_object_object_add(prop5,actmp,pjoExempleItem);
                            tmps = strtok_r (NULL, ",", &pcSavePtr);
                            iCnt++;
                          }
                          json_object_object_add(cc0,"examples",prop5);
                        }
                        free(pcaKey1);
                        if(iCounter==0)
                          pjoaContent=json_object_new_object();
                        map* pmTmp3=getMap(pmsTmp1->content,"type");
                        if(pmTmp3!=NULL)
                          json_object_object_add(pjoaContent,pmTmp3->value,cc0);
                        else
                          json_object_object_add(pjoaContent,"application/json",cc0);
                        if(iCounter==0)
                          cc2=json_object_new_object();
                        if(iCounter+1==iLen){
                          json_object_object_add(cc2,"content",pjoaContent);
                          addDescription(pmsTmp1->content,0,cc2,NULL);
                          json_object_object_add(cc2,"required",json_object_new_boolean(true));
                          json_object_object_add(methodc,"requestBody",cc2);
                        }
                      }
                    }
                  }
                  free(pcaKey0);
                  map* pmCallbacksReference=getMapArray(pmsTmp->content,"callbacksReference",i);
                  if(pmCallbacksReference!=NULL){
                    pmsTmp1=getMaps(pmsConf,pmCallbacksReference->value);
                    if(pmsTmp1!=NULL){
                      map* pmTmp2=getMap(pmsTmp1->content,"length");
                      int iLen=atoi(pmTmp2->value);
                      json_object *pjoaRes=json_object_new_object();
                      for(int iCallbackCnt=0;iCallbackCnt<iLen;iCallbackCnt++){
                        map* pmState=getMapArray(pmsTmp1->content,"state",iCallbackCnt);
                        map* pmUri=getMapArray(pmsTmp1->content,"uri",iCallbackCnt);
                        map* pmSchema=getMapArray(pmsTmp1->content,"schema",iCallbackCnt);
                        map* pmType=getMapArray(pmsTmp1->content,"type",iCallbackCnt);
                        map* pmTitle=getMapArray(pmsTmp1->content,"title",iCallbackCnt);
                        json_object *pjoaSchema=json_object_new_object();
                        if(pmSchema!=NULL)
                          json_object_object_add(pjoaSchema,"$ref",json_object_new_string(pmSchema->value));
                        json_object *pjoaType=json_object_new_object();
                        json_object_object_add(pjoaType,"schema",pjoaSchema);
                        json_object *pjoaContent=json_object_new_object();
                        if(pmType!=NULL)
                          json_object_object_add(pjoaContent,pmType->value,pjoaType);
                        else
                          json_object_object_add(pjoaContent,"application/json",pjoaType);
                        json_object *pjoaRBody=json_object_new_object();
                        json_object_object_add(pjoaRBody,"content",pjoaContent);

                        json_object *pjoaDescription=json_object_new_object();
                        json_object *pjoaPost=json_object_new_object();
                        if(pmTitle!=NULL){
                          json_object_object_add(pjoaDescription,"description",json_object_new_string(_(pmTitle->value)));
                          json_object_object_add(pjoaPost,"summary",json_object_new_string(_(pmTitle->value)));
                        }
                        json_object *pjoaResponse=json_object_new_object();
                        json_object_object_add(pjoaResponse,"200",pjoaDescription);

                        json_object_object_add(pjoaPost,"requestBody",pjoaRBody);
                        json_object_object_add(pjoaPost,"responses",pjoaResponse);
                        if(pmName==NULL)
                          pmName=getMapArray(pmsTmp->content,"operationId",iCallbackCnt);
                        if(pmName!=NULL){
                          char* pcaOperationId=(char*)malloc((strlen(pmState->value)+strlen(pmName->value)+1)*sizeof(char));
                          sprintf(pcaOperationId,"%s%s",pmState->value,pmName->value);
                          json_object_object_add(pjoaPost,"operationId",json_object_new_string(pcaOperationId));
                          free(pcaOperationId);
                        }
                        else{
                          produceOperationId(pmsConf,pmsTmp1,iCallbackCnt,pjoaPost);
                        }

                        json_object *pjoaMethod=json_object_new_object();
                        json_object_object_add(pjoaMethod,"post",pjoaPost);

                        char* pcaUri=(char*) malloc((strlen(pmUri->value)+29)*sizeof(char));
                        sprintf(pcaUri,"{$request.body#/subscriber/%s}",pmUri->value);

                        json_object *pjoaFinal=json_object_new_object();
                        json_object_object_add(pjoaFinal,pcaUri,pjoaMethod);
                        json_object_object_add(pjoaRes,pmState->value,pjoaFinal);

                      }
                      json_object_object_add(methodc,"callbacks",pjoaRes);
                    }
                  }
              }
              map* mMap=getMapArray(pmsTmp->content,"method",i);
              if(mMap!=NULL)
                json_object_object_add(method,mMap->value,methodc);
              else
                json_object_object_add(method,"get",methodc);
          }

          tmpMap=getMapFromMaps(pmsConf,"openapi","version");
          if(tmpMap!=NULL)
            json_object_object_add(pjoRes,"openapi",json_object_new_string(tmpMap->value));
          else
            json_object_object_add(pjoRes,"openapi",json_object_new_string("3.0.2"));
          if(strstr(tmps,"/root")!=NULL)
            json_object_object_add(paths,"/",method);
          else
            json_object_object_add(paths,tmps,method);
        }

        tmps = strtok_r (NULL, ",", &pcSavePtr);
        cnt++;
      }
      json_object_object_add(pjoRes,"paths",paths);
    }
      
    tmpMap=getMapFromMaps(pmsConf,"openapi","links");
    if(tmpMap!=NULL){
	
    }
    
    json_object *pjoRes3=json_object_new_array();
    json_object_array_add(pjoRes3,pjoRes4);
    json_object_object_add(pjoRes,"servers",pjoRes3);
  }

  /**
   * Produce the JSON response for /
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoRes the JSON object to populate
   */
  void produceRoot(maps* pmsConf,json_object *pjoRes){
    json_object *pjoArray=json_object_new_array();
    map* pmVal=getMapFromMaps(pmsConf,"openapi","links");
    char *orig=zStrdup(pmVal->value);
    char *pcSavePtr;
    char *tmps = strtok_r (orig, ",", &pcSavePtr);
    map* tmpUrl=getMapFromMaps(pmsConf,"openapi","rootUrl");
    while(tmps!=NULL){
      maps* pmsTmp=getMaps(pmsConf,tmps);
      if(pmsTmp!=NULL){
        json_object *pjoContent=mapToJson(pmsTmp->content);
        char* pcaTmp=(char*) malloc((strlen(tmpUrl->value)+strlen(tmps)+2)*sizeof(char));
        sprintf(pcaTmp,"%s%s",tmpUrl->value,tmps);
        json_object_object_add(pjoContent,"href",json_object_new_string(pcaTmp));
        free(pcaTmp);
        json_object_array_add(pjoArray,pjoContent);

        map* pmTmp=getMap(pmsTmp->content,"title");
        char *pcaTitle=NULL;
        if(pmTmp!=NULL)
          pcaTitle=zStrdup(pmTmp->value);
        char* pacTmp=(char*) malloc((strlen(tmps)+6)*sizeof(char));
        sprintf(pacTmp,"%s.html",tmps);
        pmsTmp=getMaps(pmsConf,pacTmp);
        if(pmsTmp==NULL && strncasecmp(pacTmp,"/.html",6)==0)
          pmsTmp=getMaps(pmsConf,"/index.html");
        if(pmsTmp!=NULL){
          json_object *pjoExtra=mapToJson(pmsTmp->content);
          if(getMap(pmsTmp->content,"title")==NULL && pcaTitle!=NULL){
            json_object_object_add(pjoExtra,"title",json_object_new_string(_(pcaTitle)));
          }
          char* pcaTmp=NULL;
          if(strncasecmp(pacTmp,"/.html",6)==0){
            pcaTmp=(char*) malloc((strlen(tmpUrl->value)+12)*sizeof(char));
            sprintf(pcaTmp,"%s/index.html",tmpUrl->value);
          }else{map* tmpUrl=getMapFromMaps(pmsConf,"openapi","rootUrl");
            pcaTmp=(char*) malloc((strlen(tmpUrl->value)+strlen(tmps)+6)*sizeof(char));
            sprintf(pcaTmp,"%s%s.html",tmpUrl->value,tmps);
          }
          json_object_object_add(pjoExtra,"href",json_object_new_string(pcaTmp));
          free(pcaTmp);
          json_object_array_add(pjoArray,pjoExtra);
        }
        free(pacTmp);
        if(pcaTitle!=NULL)
          free(pcaTitle);
      }
      tmps = strtok_r (NULL, ",", &pcSavePtr);
    }
    free(orig);

    map* pmTmp=getMapFromMaps(pmsConf,"identification","title");
    if(pmTmp!=NULL)
      json_object_object_add(pjoRes,"title",json_object_new_string(_(pmTmp->value)));
    pmTmp=getMapFromMaps(pmsConf,"identification","abstract");
    if(pmTmp!=NULL)
      json_object_object_add(pjoRes,"description",json_object_new_string(_(pmTmp->value)));
    json_object_object_add(pjoRes,"links",pjoArray);
  }

  /**
   * Produce the JSON response for /conformance
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pjoRes the JSON object to populate
   */
  void produceConformances(maps* pmsConf,json_object* pjoRes){
    maps* pmsConformsTo=getMaps(pmsConf,"conformsTo");
    map* pmExtentionsLength=getMapFromMaps(pmsConf,"conformsTo","extentionsLength");
    int len=0;
    map* pmExtentionUrl=NULL;
    if(pmExtentionsLength!=NULL){
      len=atoi(pmExtentionsLength->value);
    }
    map* rootUrl=getMapFromMaps(pmsConf,"conformsTo","rootUrl");
    json_object *pjoArray=json_object_new_array();
    map* length=getMapFromMaps(pmsConf,"conformsTo","length");
    for(int kk=0;kk<atoi(length->value);kk++){
      map* pmTmp=getMapArray(pmsConformsTo->content,"link",kk);
      json_object *res2;
      if(pmTmp!=NULL){
        char* pcaTmp=NULL;
        if(getMapArray(pmsConformsTo->content,"extention",kk)!=NULL){
          map* pmCid=getMapArray(pmsConformsTo->content,"extid",kk);
          if(pmCid!=NULL){
            pmExtentionUrl=getMapArray(pmsConformsTo->content,"extentionUrl",atoi(pmCid->value));
            if(pmExtentionUrl!=NULL){
              pcaTmp=(char*) malloc((strlen(pmExtentionUrl->value)+strlen(pmTmp->value)+1)*sizeof(char));
              sprintf(pcaTmp,"%s%s",pmExtentionUrl->value,pmTmp->value);
            }
          }
        }
        else{
          pcaTmp=(char*) malloc((strlen(rootUrl->value)+strlen(pmTmp->value)+1)*sizeof(char));
          sprintf(pcaTmp,"%s%s",rootUrl->value,pmTmp->value);
        }
        if(pcaTmp!=NULL){
          json_object_array_add(pjoArray,json_object_new_string(pcaTmp));
          free(pcaTmp);
        }
      }
    }
    json_object_object_add(pjoRes,"conformsTo",pjoArray);
  }

#ifdef DRU_ENABLED
  /**
   * Print exception report in case Deploy or Undeploy failed to execute
   *
   * @param ppmsConf the main configuration maps pointer
   */
  void handleDRUError(maps** ppmsConf){
    maps* pmsConf=*ppmsConf;
    map* pmError=getMapFromMaps(pmsConf,"lenv","jsonStr");
    setMapInMaps(pmsConf,"lenv","hasPrinted","false");
    setMapInMaps(pmsConf,"lenv","no-headers","false");
    setMapInMaps(pmsConf,"headers","Status","500 Internal Server Error");
    setMapInMaps(pmsConf,"lenv","status_code","500 Internal Server Error");
    if(pmError!=NULL){
      printHeaders(ppmsConf);
      printf("\r\n");
      printf("%s",pmError->value);
      printf("\n");
    }else{
      pmError=createMap("code","InternalError");
      map* pmMessage=getMapFromMaps(pmsConf,"lenv","message");
      map* pmORequestMethod=getMapFromMaps(pmsConf,"lenv","orequest_method");
      if(pmORequestMethod!=NULL && strncasecmp(pmORequestMethod->value,"put",3)==0){
        if(pmMessage!=NULL){
          char* pcaMessage=(char*)malloc((strlen(pmMessage->value)+strlen(_("Failed to update the process!"))+1)*sizeof(char));
          sprintf(pcaMessage,"%s %s",_("Failed to update the process!"),pmMessage->value);
          addToMap(pmError,"message",pcaMessage);
          free(pcaMessage);
        }else
          addToMap(pmError,"message",_("Failed to update the process!"));
      }
      else{
        if(pmMessage!=NULL){
          char* pcaMessage=(char*)malloc((strlen(pmMessage->value)+strlen(_("Failed to deploy process!"))+1)*sizeof(char));
          sprintf(pcaMessage,"%s %s",_("Failed to deploy process!"),pmMessage->value);
          addToMap(pmError,"message",pcaMessage);
          free(pcaMessage);
        }else
          addToMap(pmError,"message",_("Failed to deploy process!"));
      }
      printExceptionReportResponseJ(ppmsConf,pmError);
      freeMap(&pmError);
      free(pmError);
    }
    setMapInMaps(pmsConf,"lenv","no-headers","true");
    setMapInMaps(pmsConf,"lenv","hasPrinted","true");
  }

  /**
   * Convert a CWL to OGC Application Package format
   *
   * @param pmsConf the main configuration maps pointer
   * @param pmRequestInputs map containing the request inputs
   * @return a json_object pointer containing the OGC Application Package
   */
  json_object* convertCwlToOGCAppPkg(maps* pmsConf,map* pmRequestInputs){
    json_object* pjoRes=NULL;
    map* pmJRequest=getMap(pmRequestInputs,"jrequest");
    map* pmCgiContentType=getMapFromMaps(pmsConf,"request","Content-Type");
    if(pmJRequest!=NULL && pmCgiContentType!=NULL && strstr(pmCgiContentType->value,"cwl")!=NULL){
      pjoRes=json_object_new_object();
      json_object *pjoExecutionUnit=json_object_new_object();
      json_object *pjoExecutionUnitFormat=json_object_new_object();
      json_object_object_add(pjoExecutionUnitFormat,"mediaType",json_object_new_string("application/cwl"));
      json_object_object_add(pjoExecutionUnit,"value",json_object_new_string(pmJRequest->value));
      json_object_object_add(pjoExecutionUnit,"format",pjoExecutionUnitFormat);
      json_object* pjoExecutionUnits=json_object_new_array();
      json_object_array_add(pjoExecutionUnits,pjoExecutionUnit);
      json_object_object_add(pjoRes,"executionUnit",pjoExecutionUnits);
    }
    return pjoRes;
  }

  /**
   * Convert an OGC Application Package into a standard execute payload
   *
   * @param pmsConf the main configuration maps pointer
   * @param pmRequestInputs map containing the request inputs
   * @param ppjoRequest the json_object corresponding to the initial payload
   * @return 0 in case of success.
   */
  int convertOGCAppPkgToExecute(maps* pmsConf,map* pmRequestInputs,json_object** ppjoRequest){
    json_object* pjoRes=json_object_new_object();
    json_object* pjoProcessDescription=NULL;
    json_object* pjoExecutionUnit=NULL;
    if(json_object_object_get_ex(*ppjoRequest, "processDescription", &pjoProcessDescription)==FALSE){
      if(json_object_object_get_ex(*ppjoRequest, "executionUnit", &pjoExecutionUnit)!=FALSE){
        json_object* pjoInputs=json_object_new_object();
        json_object_object_add(pjoInputs,"applicationPackage",json_object_get(pjoExecutionUnit));
        json_object_object_add(pjoRes,"inputs",pjoInputs);
        json_object_put(*ppjoRequest);
        *ppjoRequest=json_object_get(pjoRes);
        const char* jsonStr=json_object_to_json_string_ext(*ppjoRequest,JSON_C_TO_STRING_NOSLASHESCAPE);
        setMapInMaps(pmsConf,"lenv","jrequest",jsonStr);
      }
      json_object_put(pjoRes);
    }else{
      json_object* pjoInputs=json_object_new_object();
      json_object_object_add(pjoInputs,"applicationPackage",json_object_get(*ppjoRequest));
      json_object_object_add(pjoRes,"inputs",pjoInputs);
      json_object_put(*ppjoRequest);
      *ppjoRequest=json_object_get(pjoRes);
      const char* jsonStr=json_object_to_json_string_ext(*ppjoRequest,JSON_C_TO_STRING_NOSLASHESCAPE);
      setMapInMaps(pmsConf,"lenv","jrequest",jsonStr);
    }
    return 0;
  }

  /**
   * Verify that a service name correspond to the Deploy or Undeploy service
   * name
   *
   * @param pmsConf the main configuration maps pointer
   * @param pcService the service name
   * @return true if the service name correspond to deploy or undeploy, 
   * false otherwise
   */
  bool serviceIsDRU(maps* pmsConf,char* pcService){
    map* pmDeployService=getMapFromMaps(pmsConf,"servicesNamespace","deploy_service_provider");
    if(pmDeployService!=NULL && strstr(pmDeployService->value,pcService)!=NULL)
      return true;
    else{
      pmDeployService=getMapFromMaps(pmsConf,"servicesNamespace","undeploy_service_provider");
      if(pmDeployService!=NULL && strstr(pmDeployService->value,pcService)!=NULL)
        return true;
    }
    return false;
  }

  /**
   * Parse process description (do nothing currently)
   *
   * @param pmsConf the main configuration maps pointer
   * @return 0
   */
  int parseProcessDescription(maps* pmsConf){
    fprintf(stderr,"Should parse process description %s %d \n",__FILE__,__LINE__);
    fflush(stderr);
    return 0;
  }

  /**
   * Search for an application package
   *
   * @param pmsConf the main configuration maps pointer
   * @param pcFilename the char pointer to the file name
   * @param pcConfDir the location of the main.cfg file
   */
  char* searchForFile(maps* pmsConf,char* pcFilename,char* pcConfDir){
    map* pmHasSearchPath=getMapFromMaps(pmsConf,"main","search_path");
    zStatStruct zssStatus;
    char* pcaFilePath=(char*)malloc((strlen(pcConfDir)+strlen(pcFilename)+2)*sizeof(char));
    sprintf(pcaFilePath,"%s/%s",pcConfDir,pcFilename);
    int iStat=zStat(pcaFilePath, &zssStatus);
    if(iStat==0){
      return pcaFilePath;
    }else{
      free(pcaFilePath);
      if(pmHasSearchPath!=NULL && strncasecmp(pmHasSearchPath->value,"true",4)==0){
        char* pcaConfDir=(char*) malloc(1024*sizeof(char));
        getServicesNamespacePath(pmsConf,pcConfDir,pcaConfDir,1024);
        if(strncmp(pcaConfDir,pcConfDir,strlen(pcConfDir))!=0){
          pcaFilePath=(char*)malloc((strlen(pcaConfDir)+strlen(pcFilename)+2)*sizeof(char));
          sprintf(pcaFilePath,"%s/%s",pcaConfDir,pcFilename);
          iStat=zStat(pcaFilePath, &zssStatus);
          free(pcaConfDir);
          if(iStat==0){
            return pcaFilePath;
          }else{
            return NULL;
          }
        }else{
          free(pcaConfDir);
          return NULL;
        }
      }else
        return NULL;
    }
  }

  extern char* isInCache(maps*,char*);

  /**
   * Print the Application Package in various encoding
   *
   * @param pmsConf the main configuration maps pointer
   * @param pcAccess the Accept header associated with the input HTTP request
   * @param pcProcessId the process identifier passed as path parameter
   * @param pcConfDir the location of the main.cfg file
   * @return 0 in case of success, other value in case of error
   */
  int handlePackage(maps** ppmsConf,char* pcAccept,char* pcProcessId,char* pcConfDir){
    maps* pmsConf=*ppmsConf;
    if(strstr(pcAccept,"application/ogcapppkg+json")!=NULL){
      char* pcaFileName=(char*)malloc((strlen(pcProcessId)+6)*sizeof(char));
      sprintf(pcaFileName,"%s.json",pcProcessId);
      char* pcaFilePath=searchForFile(pmsConf,pcaFileName,pcConfDir);
      if(pcaFilePath==NULL){
        // Search for the CWL version of the application package
        free(pcaFilePath);
        free(pcaFileName);
        pcaFileName=(char*)malloc((strlen(pcProcessId)+5)*sizeof(char));
        sprintf(pcaFileName,"%s.cwl",pcProcessId);
        pcaFilePath=searchForFile(pmsConf,pcaFileName,pcConfDir);
        if(pcaFilePath==NULL){
          setMapInMaps(pmsConf,"lenv","status_code","406 Not Acceptable");
          map* error=createMap("code","NotAcceptable");
          addToMap(error,"message",_("The resource is not available in the requested encoding"));
          localPrintException(ppmsConf,error);
          free(pcaFileName);
          return 1;
        }else{
          zStatStruct zssStatus;
          int iStat=zStat(pcaFilePath, &zssStatus);
          char* pcaFcontent=(char*)malloc(sizeof(char)*(zssStatus.st_size+1));
          FILE *pfRequest = fopen (pcaFilePath, "rb");
          if(pfRequest!=NULL){
            size_t sLength = fread(pcaFcontent,zssStatus.st_size,sizeof(char),pfRequest);
            pcaFcontent[zssStatus.st_size]=0;
            setMapInMaps(pmsConf,"lenv","json_response_object",pcaFcontent);
            setMapInMaps(pmsConf,"lenv","goto_json_print_out","true");
            setMapInMaps(pmsConf,"lenv","require_conversion_to_json","true");
            setMapInMaps(pmsConf,"lenv","require_conversion_to_ogcapppkg","true");
            free(pcaFcontent);
            return 0;
          }else{
            setMapInMaps(pmsConf,"lenv","status_code","406 Not Acceptable");
            map* error=createMap("code","NotAcceptable");
            addToMap(error,"message",_("The resource is not available in the requested encoding"));
            localPrintException(ppmsConf,error);
            free(pcaFileName);
            return 1;
          }
        }
      }else{
        zStatStruct zssStatus;
        int iStat=zStat(pcaFilePath, &zssStatus);
        setMapInMaps(pmsConf,"headers","Content-Type",pcAccept);
        printAFile(ppmsConf,pcaFilePath,zssStatus,localPrintException);
        free(pcaFileName);
        free(pcaFilePath);
        return 0;
      }
    }else{
      if(strstr(pcAccept,"application/cwl")!=NULL){
        char* pcaFileName=(char*)malloc((strlen(pcConfDir)+strlen(pcProcessId)+5)*sizeof(char));
        sprintf(pcaFileName,"%s.cwl",pcProcessId);
        char* pcaFilePath=searchForFile(pmsConf,pcaFileName,pcConfDir);
        if(pcaFilePath==NULL){
          // Should try to read the json file supposed to be in OGC Application
          // Package format and try to fetch the cached file for the referenced
          // CWL. If this failed, we can return an exception NotAcceptable
          // mentioning that the content negotiation failed.
          free(pcaFileName);
          pcaFileName=(char*)malloc((strlen(pcProcessId)+6)*sizeof(char));
          sprintf(pcaFileName,"%s.json",pcProcessId);
          pcaFilePath=searchForFile(pmsConf,pcaFileName,pcConfDir);
          if(pcaFilePath==NULL){
            setMapInMaps(pmsConf,"lenv","status_code","406 Not Acceptable");
            map* error=createMap("code","NotAcceptable");
            addToMap(error,"message",_("The resource is not available in the requested encoding"));
            localPrintException(ppmsConf,error);
            return 1;
          }else{
            // Should parse the JSON OGC Application Package and try to fetch
            // the corresponding CWL file stored in the cache.
            zStatStruct zssStatus;
            int iStat=zStat(pcaFilePath, &zssStatus);
            char* pcaFcontent=(char*)malloc(sizeof(char)*(zssStatus.st_size+1));
            FILE *pfRequest = fopen (pcaFilePath, "rb");
            if(pfRequest!=NULL){
              size_t sLength = fread(pcaFcontent,zssStatus.st_size,sizeof(char),pfRequest);
              pcaFcontent[zssStatus.st_size]=0;
              json_object *pjoRes=parseJson(pmsConf,pcaFcontent);
              if(pjoRes==NULL)
                ZOO_DEBUG("pjoRes==NULL!");
              json_object* pjoExecutionUnit;
              if(json_object_object_get_ex(pjoRes, "executionUnit", &pjoExecutionUnit)!=FALSE){
                if(json_object_is_type(pjoExecutionUnit,json_type_array)){
                  // TODO: handle multiple execution units?
                }else{
                  json_object* pjoHref;
                  if(json_object_object_get_ex(pjoExecutionUnit, "href", &pjoHref)!=FALSE){
                    if(json_object_is_type(pjoHref,json_type_string)){
                      const char* pccHref=json_object_get_string(pjoHref);
                      char* pcaCachedFile=isInCache(pmsConf,(char*)pccHref);
                      if(pcaCachedFile!=NULL){
                        map* pmConvertionRequired=getMapFromMaps(pmsConf,"lenv","require_conversion_to_json");
                        zStatStruct zssStatus1;
                        int iStat=zStat(pcaCachedFile, &zssStatus1);
                        if(pmConvertionRequired==NULL || strncmp(pmConvertionRequired->value,"true",4)!=0){
                          setMapInMaps(pmsConf,"headers","Content-Type",pcAccept);
                          printAFile(ppmsConf,pcaCachedFile,zssStatus1,localPrintException);
                        }else{
                          if(iStat==0){
                            FILE *pfRequest1 = fopen (pcaCachedFile, "rb");
                            if(pfRequest1==NULL){
                              setMapInMaps(pmsConf,"lenv","status_code","406 Not Acceptable");
                              map* error=createMap("code","NotAcceptable");
                              addToMap(error,"message",_("The resource is not available in the requested encoding"));
                              localPrintException(ppmsConf,error);
                              return 1;
                            }
                            else{
                              char* pcaFcontent1=(char*)malloc(sizeof(char)*(zssStatus1.st_size+1));
                              size_t sLength = fread(pcaFcontent1,zssStatus1.st_size,sizeof(char),pfRequest1);
                              pcaFcontent[zssStatus1.st_size]=0;
                              setMapInMaps(pmsConf,"lenv","json_response_object",pcaFcontent1);
                              setMapInMaps(pmsConf,"lenv","goto_json_print_out","true");
                              setMapInMaps(pmsConf,"lenv","require_conversion_to_json","true");
                              free(pcaFcontent1);
                            }
                          }
                        }
                        free(pcaCachedFile);
                      }
                    }
                  }
                }
              }else{
                setMapInMaps(pmsConf,"lenv","json_response_object",pcaFcontent);
                setMapInMaps(pmsConf,"lenv","goto_json_print_out","true");
              }
              free(pcaFcontent);
              return 0;
            }else{
              setMapInMaps(pmsConf,"lenv","status_code","406 Not Acceptable");
              map* error=createMap("code","NotAcceptable");
              addToMap(error,"message",_("The resource is not available in the requested encoding"));
              localPrintException(ppmsConf,error);
              free(pcaFileName);
              return 1;
            }
          }
        }else{
          // Return the CWL content
          zStatStruct zssStatus;
          int iStat=zStat(pcaFilePath, &zssStatus);
          map* pmConvertionRequired=getMapFromMaps(pmsConf,"lenv","require_conversion_to_json");
          if(pmConvertionRequired==NULL || strncmp(pmConvertionRequired->value,"true",4)!=0){
            setMapInMaps(pmsConf,"headers","Content-Type",pcAccept);
            printAFile(ppmsConf,pcaFilePath,zssStatus,localPrintException);
          }else{
            if(iStat==0){
              FILE *pfRequest = fopen (pcaFilePath, "rb");
              if(pfRequest==NULL){
                setMapInMaps(pmsConf,"lenv","status_code","406 Not Acceptable");
                map* error=createMap("code","NotAcceptable");
                addToMap(error,"message",_("The resource is not available in the requested encoding"));
                localPrintException(ppmsConf,error);
                return 1;
              }
              else{
                char* pcaFcontent=(char*)malloc(sizeof(char)*(zssStatus.st_size+1));
                size_t sLength = fread(pcaFcontent,zssStatus.st_size,sizeof(char),pfRequest);
                pcaFcontent[zssStatus.st_size]=0;
                setMapInMaps(pmsConf,"lenv","json_response_object",pcaFcontent);
                setMapInMaps(pmsConf,"lenv","goto_json_print_out","true");
                free(pcaFcontent);
              }
            }
          }
          free(pcaFilePath);
          return 0;
        }
      }else{
        setMapInMaps(pmsConf,"lenv","status_code","406 Not Acceptable");
        map* error=createMap("code","NotAcceptable");
        addToMap(error,"message",_("The resource is not available in the requested encoding"));
        localPrintException(ppmsConf,error);
        return 1;
      }
    }
    return 1;
  }
#endif // DRU_ENABLED

  /**
   * Verify that a service name correspond to the a given process name
   * 
   * @param pmsConf the main configuration maps pointer
   * @param pcService the service name to verify
   * @param pmsFilter the maps pointer containing the process name
   * @return true if the process name corresponds to tje pmsFilter
   */
  bool serviceIsFiltered(maps* pmsConf,char* pcService,maps* pmsFilter){
    if(pmsFilter!=NULL){
      int len=1;
      map* pmLength=getMap(pmsFilter->content,"length");
      if(pmLength!=NULL)
        len=atoi(pmLength->value);
      for(int i=0;i<len;i++){
        map* pmFilterService=getMapArray(pmsFilter->content,"service",i);
        if(pmFilterService!=NULL && strstr(pmFilterService->value,pcService)!=NULL)
          return true;
      }
    }
    return false;
  }

  /**
   * Verify that a service name correspond to the Deploy or Undeploy service
   * name
   *
   * @param pmsConf the main configuration maps pointer
   * @param pcService the service name
   * @return true if the process name corresponds to a filter_in or filter_out;
   * false otherwise
   */
  bool serviceIsFilter(maps* pmsConf,char* pcService){
    maps* pmsFilterIn=getMaps(pmsConf,"filter_in");
    if(pcService==NULL)
      return true;
    if(serviceIsFiltered(pmsConf,pcService,pmsFilterIn)==true)
      return true;
    else{
      if(getMaps(pmsConf,"hidden_processes")==NULL){
        maps* pmsFilterOut=getMaps(pmsConf,"filter_out");
        return serviceIsFiltered(pmsConf,pcService,pmsFilterOut);
      }else{
        maps* pmsFilterOut=getMaps(pmsConf,"filter_out");
        if(serviceIsFiltered(pmsConf,pcService,pmsFilterOut)==true)
          return true;
        else{
          maps* pmsHiddenProcesses=getMaps(pmsConf,"hidden_processes");
          return serviceIsFiltered(pmsConf,pcService,pmsHiddenProcesses);
        }
      }
    }
    return false;
  }

#ifdef __cplusplus
}
#endif
