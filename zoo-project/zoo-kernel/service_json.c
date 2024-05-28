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
#include "json.h"
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
   * @param jopObj the json object to convert
   * @return the allocated maps
   */
  maps* jsonToMaps(json_object* jopObj){
    maps* res=NULL;
    //enum json_type type;
    json_object_object_foreach(jopObj, key, val) { /*Passing through every object element*/
      json_object* json_input=NULL;
      if(json_object_object_get_ex(jopObj, key, &json_input)!=FALSE){
	map* tmpMap=jsonToMap(json_input);
	if(res==NULL){
	  res=createMaps(key);
	  addMapToMap(&(res->content),tmpMap);
	}else{
	  maps *tres=createMaps(key);
	  addMapToMap(&(tres->content),tmpMap);
	  addMapsToMaps(&res,tres);
	  freeMaps(&tres);
	  free(tres);
	}
      }
    }
    return res;
  }

  /**
   * Convert a json object to map
   *
   * @param jopObj the json object to convert
   * @return the allocated map
   */
  map* jsonToMap(json_object* jopObj){
    map* res=NULL;
    json_object_object_foreach(jopObj, key, val) {
      if(val!=NULL && json_object_is_type (val,json_type_string)){
	const char* pcVal=json_object_get_string(val);
	if(strlen(pcVal)>0){
	  if(res==NULL)
	    res=createMap(key,pcVal);
	  else
	    addToMap(res,key,pcVal);
	}
      }
    }
    return res;
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
   * @param myMap the map to be converted into json object
   * @return a json_object pointer to the created json_object
   */
  json_object* mapToJson(map* myMap){
    json_object *res=json_object_new_object();
    map* cursor=myMap;
    map* sizeMap=getMap(myMap,"size");
    map* length=getMap(myMap,"length");
    map* toLoad=getMap(myMap,"to_load");
    while(cursor!=NULL){
      json_object *val=NULL;
      if(length==NULL && sizeMap==NULL){
	if(strstr(cursor->name,"title")!=NULL)
	  val=json_object_new_string(_(cursor->value));
	else
	  val=json_object_new_string(cursor->value);
      }
      else{
	if(length==NULL && sizeMap!=NULL){
	  if(strncasecmp(cursor->name,"value",5)==0){
	    if(strlen(cursor->value)!=0 && toLoad!=NULL && strncasecmp(toLoad->value,"true",4)==0)
	      val=json_object_new_string_len(cursor->value,atoi(sizeMap->value));
	  }
	  else{
	    if(strstr(cursor->name,"title")!=NULL)
	      val=json_object_new_string(_(cursor->value));
	    else
	      val=json_object_new_string(cursor->value);
	  }
	}
	else{
	  val=json_object_new_string(cursor->value);
	}
      }
      if(val!=NULL)
	json_object_object_add(res,cursor->name,val);
      cursor=cursor->next;
    }
    return res;
  }

  /**
   * Convert a maps to a json object
   * @param myMap the maps to be converted into json object
   * @return a json_object pointer to the created json_object
   */
  json_object* mapsToJson(maps* myMap){
    json_object *res=json_object_new_object();
    maps* cursor=myMap;
    while(cursor!=NULL){
      json_object *obj=NULL;
      if(cursor->content!=NULL){
	obj=mapToJson(cursor->content);
      }else
	obj=json_object_new_object();
      if(cursor->child!=NULL){
	json_object *child=NULL;
	child=mapsToJson(cursor->child);
	json_object_object_add(obj,"child",child);
      }
      json_object_object_add(res,cursor->name,obj);
      cursor=cursor->next;
    }
    return res;
  }

  /**
   * Convert an elements to a json object
   * @param myElements the elements pointer to be converted into a json object
   * @return a json_object pointer to the created json_object
   */
  json_object* elementsToJson(elements* myElements){
    json_object *res=json_object_new_object();
    elements* cur=myElements;
    while(cur!=NULL){
      json_object *cres=json_object_new_object();
      json_object_object_add(cres,"content",mapToJson(cur->content));
      json_object_object_add(cres,"metadata",mapToJson(cur->metadata));
      json_object_object_add(cres,"additional_parameters",mapToJson(cur->additional_parameters));
      if(cur->format!=NULL){
	json_object_object_add(cres,"format",json_object_new_string(cur->format));
      }
      if(cur->child==NULL){
	if(cur->defaults!=NULL)
	  json_object_object_add(cres,"defaults",mapToJson(cur->defaults->content));
	else
	  json_object_object_add(cres,"defaults",mapToJson(NULL));
	iotype* scur=cur->supported;
	json_object *resi=json_object_new_array();
	while(scur!=NULL){
	  json_object_array_add(resi,mapToJson(scur->content));
	  scur=scur->next;
	}
	json_object_object_add(cres,"supported",resi);
      }
      
      json_object_object_add(cres,"child",elementsToJson(cur->child));

      json_object_object_add(res,cur->name,cres);
      cur=cur->next;
    }
    return res;
  }
  
  /**
   * Convert an service to a json object
   *
   * @param myService the service pointer to be converted into a json object
   * @return a json_object pointer to the created json_object
   */
  json_object* serviceToJson(service* myService){
    json_object *res=json_object_new_object();
    json_object_object_add(res,"name",json_object_new_string(myService->name));
    json_object_object_add(res,"content",mapToJson(myService->content));
    json_object_object_add(res,"metadata",mapToJson(myService->metadata));
    json_object_object_add(res,"additional_parameters",mapToJson(myService->additional_parameters));
    json_object_object_add(res,"inputs",elementsToJson(myService->inputs));
    json_object_object_add(res,"outputs",elementsToJson(myService->outputs));
    return res;
  }

  /**
   * Add a description to one or two json objects using the abstract_file or
   * abstract value. In case an abstract_file is found, then the ZOO-Kernel will
   * try to read the provided file path and use its content as the description
   * value.
   *
   * @param pmElement the current map to search for abstract_file/abstract field
   * @param iIndex the index to use to fetch from the map array
   * @param output the first json object to add the description
   * @param output1 the second json object to add the description (optional)
   */
  void addDescription(map* pmElement,int iIndex,json_object* output,json_object* output1){
    if(pmElement==NULL)
      return ;
    map* pmCurrent=getMapArray(pmElement,"abstract_file",iIndex);
    if(pmCurrent!=NULL){
      FILE* pfData=fopen(pmCurrent->value,"rb");
      if(pfData!=NULL){
	zStatStruct f_status;
	int s=zStat(pmCurrent->value, &f_status);
	if(f_status.st_size>0){
	  char* pcaTmp=(char*)malloc((f_status.st_size+1)*sizeof(char));
	  fread(pcaTmp,1,f_status.st_size,pfData);
	  pcaTmp[f_status.st_size]=0;
	  fclose(pfData);
	  json_object_object_add(output,"description",json_object_new_string(_(pcaTmp)));
	  if(output1!=NULL)
	    json_object_object_add(output1,"description",json_object_new_string(_(pcaTmp)));
	  free(pcaTmp);
	}
      }else{
	  json_object_object_add(output,"description",json_object_new_string(_("Unable to load your file")));
	  if(output1!=NULL)
	    json_object_object_add(output1,"description",json_object_new_string(_("Unable to load your file")));
      }
    }else{
      pmCurrent=getMapArray(pmElement,"abstract",iIndex);
      if(pmCurrent!=NULL){
	json_object_object_add(output,"description",json_object_new_string(_(pmCurrent->value)));
	if(output1!=NULL)
	  json_object_object_add(output1,"description",json_object_new_string(_(pmCurrent->value)));
      }
    }
  }

  /**
   * Add Allowed Range properties to a json_object
   *
   * @param m the main configuration maps pointer
   * @param iot the iotype pointer
   * @param prop the json_object pointer to add the allowed range properties
   * @return a json_object pointer to the created json_object
   */
  void printAllowedRangesJ(maps* m,iotype* iot,json_object* prop){
    map* tmpMap1;
    map* pmType=getMap(iot->content,"DataType");
    json_object* prop4=json_object_new_object();
    int hasMin=0;
    int hasMax=0;
    for(int i=0;i<4;i++){
      tmpMap1=getMap(iot->content,rangeCorrespondances[i][0]);
      if(tmpMap1!=NULL){
	if(i>=1 && ( (hasMin==0 && i==1) || (hasMax==0 && i==2)) ){
	  map* pmaTmp=createMap("value",tmpMap1->value);
	  printLiteralValueJ(m,pmType,pmaTmp,prop,rangeCorrespondances[i][1]);
	  freeMap(&pmaTmp);
	  free(pmaTmp);
	}
	else{
	  char* currentValue=NULL;	  
	  map* pmTmp=getMap(iot->content,rangeCorrespondances[1][0]);
	  if(tmpMap1->value[0]=='o' && pmTmp!=NULL){
	    hasMin=1;
	    map* pmaTmp=createMap("value",pmTmp->value);
	    printLiteralValueJ(m,pmType,pmaTmp,prop,"exclusiveMinimum");
	    freeMap(&pmaTmp);
	    free(pmaTmp);
	    if(strlen(tmpMap1->value)==1){
	      pmTmp=getMap(iot->content,rangeCorrespondances[2][0]);
	      if(pmTmp!=NULL){
		hasMax=1;
		map* pmaTmp1=createMap("value",pmTmp->value);
		printLiteralValueJ(m,pmType,pmaTmp1,prop,"exclusiveMaximum");
		freeMap(&pmaTmp1);
		free(pmaTmp1);
	      }
	    }
	  }
	  if(strlen(tmpMap1->value)>1 && tmpMap1->value[1]=='o'){
	    pmTmp=getMap(iot->content,rangeCorrespondances[2][0]);
	    if(pmTmp!=NULL){
	      hasMax=1;
	      map* pmaTmp=createMap("value",pmTmp->value);
	      printLiteralValueJ(m,pmType,pmaTmp,prop,"exclusiveMaximum");
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
   * @param schema the json_object pointer to add the value
   * @param field the field name string
   */
  void printLiteralValueJ(maps* pmConf,map* pmType,map* pmContent,json_object* schema,const char* field){
    map* pmTmp=getMap(pmContent,"value");
    map* pmLength=getMap(pmContent,"length");
    if(pmLength!=NULL){
      int len=atoi(pmLength->value);
      json_object* defaultProp=json_object_new_array();
      for(int i=0;i<len;i++){
	map* pmTmp0=getMapArray(pmContent,"value",i);
	if(pmTmp0!=NULL){
	  if(pmType!=NULL && strncasecmp(pmType->value,"integer",7)==0)
	    json_object_array_add(defaultProp,json_object_new_int(atoi(pmTmp0->value)));
	  else{
	    if(pmType!=NULL && strncasecmp(pmType->value,"float",5)==0){
	      json_object_array_add(defaultProp,json_object_new_double(atof(pmTmp0->value)));
	      json_object_object_add(schema,"format",json_object_new_string("double"));
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
	}
      }
      json_object_object_add(schema,field,defaultProp);
    }
    else if(pmTmp!=NULL)
      if(pmType!=NULL && strncasecmp(pmType->value,"integer",7)==0)
	json_object_object_add(schema,field,json_object_new_int(atoi(pmTmp->value)));
      else{
	if(pmType!=NULL && strncasecmp(pmType->value,"float",5)==0){
	  json_object_object_add(schema,field,json_object_new_double(atof(pmTmp->value)));
	  json_object_object_add(schema,"format",json_object_new_string("double"));
	}
	else{
	  if(pmType!=NULL && strncasecmp(pmType->value,"bool",4)==0){
	    if(strncasecmp(pmType->value,"true",4)==0)
	      json_object_object_add(schema,field,json_object_new_boolean(true));
	    else
	      json_object_object_add(schema,field,json_object_new_boolean(false));
	  }
	  else
	    json_object_object_add(schema,field,json_object_new_string(pmTmp->value));
	}
      }
  }
  
  /**
   * Add literalDataDomains property to a json_object
   *
   * @param m the main configuration maps pointer
   * @param in the elements pointer
   * @param input the json_object pointer to add the literalDataDomains property
   * @return a json_object pointer to the created json_object
   */
  void printLiteralDataJ(maps* m,elements* in,json_object* input){
    json_object* schema=json_object_new_object();
    map* pmMin=getMap(in->content,"minOccurs");
    if(in->defaults!=NULL){
      map* tmpMap1=getMap(in->defaults->content,"DataType");
      if(tmpMap1!=NULL){
	if(strncasecmp(tmpMap1->value,"float",5)==0)
	  json_object_object_add(schema,"type",json_object_new_string("number"));
	else
	  if(strncasecmp(tmpMap1->value,"bool",5)==0)
	    json_object_object_add(schema,"type",json_object_new_string("boolean"));
	  else
	    if(strstr(tmpMap1->value,"date")!=NULL){
	      json_object_object_add(schema,"type",json_object_new_string("string"));
	      json_object_object_add(schema,"format",json_object_new_string(tmpMap1->value));
	    }
	    else
	      json_object_object_add(schema,"type",json_object_new_string(tmpMap1->value));
      }
      printLiteralValueJ(m,tmpMap1,in->defaults->content,schema,"default");
      tmpMap1=getMap(in->defaults->content,"rangeMin");
      if(tmpMap1!=NULL){
	printAllowedRangesJ(m,in->defaults,schema);
	if(in->supported!=NULL){
	  iotype* iot=in->supported;
	  while(iot!=NULL){
	    printAllowedRangesJ(m,iot,schema);
	    iot=iot->next;
	  }
	}
      }
      else{
	tmpMap1=getMap(in->defaults->content,"range");
	if(tmpMap1!=NULL){
	  // TODO: parse range = [rangeMin,rangeMax] (require generic function)
	}else{ 
	  // AllowedValues
	  tmpMap1=getMap(in->defaults->content,"AllowedValues");
	  if(tmpMap1!=NULL){
	    char* saveptr;
	    json_object* prop5=json_object_new_array();
	    char *tmps = strtok_r (tmpMap1->value, ",", &saveptr);
	    while(tmps!=NULL){
	      json_object_array_add(prop5,json_object_new_string(tmps));
	      tmps = strtok_r (NULL, ",", &saveptr);
	    }
	    json_object_object_add(schema,"enum",prop5);
	  }
	}
      }
    }
    if(pmMin!=NULL && atoi(pmMin->value)==0)
	json_object_object_add(schema,"nullable",json_object_new_boolean(true));
    json_object_object_add(input,"schema",schema);
  }

  /**
   * Add support in the extended-schema for links object
   * 
   * @param m the main configuration maps pointer
   * @param el the elements pointer
   * @param res the json_object pointer on the array
   * @param isDefault boolean, try if it is the default value
   * @param maxSize map pointer (not used)
   */
  void printComplexHref(maps* m,elements* el,json_object* res,bool isDefault,map* maxSize){
    if(el!=NULL && el->defaults!=NULL){
      json_object* prop=json_object_new_object();
      json_object* prop1=json_object_new_array();
      json_object* prop2=json_object_new_object();
      map* tmpMap0=getMapFromMaps(m,"openapi","link_href");
      if(tmpMap0!=NULL)
	json_object_object_add(prop2,"$ref",json_object_new_string(tmpMap0->value));
      json_object_array_add(prop1,prop2);
      json_object* prop3=json_object_new_object();
      json_object_object_add(prop3,"type",json_object_new_string("object"));
      json_object* prop4=json_object_new_object();
      json_object* prop5=json_object_new_array();
      map* tmpMap2=getMap(el->defaults->content,"mimeType");
      if(tmpMap2!=NULL)
	json_object_array_add(prop5,json_object_new_string(tmpMap2->value));
      iotype* sup=el->supported;
      while(sup!=NULL){
	tmpMap2=getMap(sup->content,"mimeType");
	if(tmpMap2!=NULL)
	  json_object_array_add(prop5,json_object_new_string(tmpMap2->value));
	sup=sup->next;
      }
      json_object_object_add(prop4,"enum",prop5);
      json_object* prop6=json_object_new_object();
      json_object_object_add(prop6,"type",prop4);
      json_object_object_add(prop3,"properties",prop6);
      json_object_array_add(prop1,prop3);
      json_object_object_add(prop,"allOf",prop1);
      json_object_array_add(res,prop);
    }
  }

  /**
   * Add support for type array in extended-schema
   *
   *
   * @param m the main configuration maps pointer
   * @param el the elements pointer
   */
  json_object* addArray(maps* m,elements* el){
    json_object* res=json_object_new_object();
    json_object_object_add(res,"type",json_object_new_string("array"));
    map* tmpMap=getMap(el->content,"minOccurs");
    if(tmpMap!=NULL /*&& strcmp(tmpMap->value,"1")!=0*/){
      json_object_object_add(res,"minItems",json_object_new_int(atoi(tmpMap->value)));
    }else{
      json_object_put(res);
      res=NULL;
      return res;
    }
    tmpMap=getMap(el->content,"maxOccurs");
    if(tmpMap!=NULL){
      if(atoi(tmpMap->value)>1){
	json_object_object_add(res,"maxItems",json_object_new_int(atoi(tmpMap->value)));
	return res;
      }
      else{
	json_object_put(res);
	res=NULL;
	return res;
      }
    }else
      return res;
  }

  /**
   * Add basic schema definition for the BoundingBox type
   * 
   * @param m the main configuration maps pointer
   * @param in the main configuration maps pointer
   * @param input the json_object pointer used to store the schema definition
   */
  void printBoundingBoxJ(maps* m,elements* in,json_object* input){
    map* pmMin=getMap(in->content,"minOccurs");
    json_object* prop20=json_object_new_object();
    json_object_object_add(prop20,"type",json_object_new_string("object"));
    json_object* prop21=json_object_new_array();
    json_object_array_add(prop21,json_object_new_string("bbox"));
    json_object_array_add(prop21,json_object_new_string("crs"));
    json_object_object_add(prop20,"required",prop21);

    json_object* prop22=json_object_new_object();

    json_object* prop23=json_object_new_object();
    json_object_object_add(prop23,"type",json_object_new_string("array"));
    json_object* prop27=json_object_new_object();
    json_object* prop28=json_object_new_object();
    json_object* prop29=json_object_new_array();
    json_object_object_add(prop27,"minItems",json_object_new_int(4));
    json_object_object_add(prop27,"maxItems",json_object_new_int(4));
    json_object_array_add(prop29,prop27);
    json_object_object_add(prop28,"minItems",json_object_new_int(6));
    json_object_object_add(prop28,"maxItems",json_object_new_int(6));
    json_object_array_add(prop29,prop28);
    json_object_object_add(prop23,"oneOf",prop29);
    json_object* prop24=json_object_new_object();
    json_object_object_add(prop24,"type",json_object_new_string("number"));
    json_object_object_add(prop24,"format",json_object_new_string("double"));
    json_object_object_add(prop23,"items",prop24);
    json_object_object_add(prop22,"bbox",prop23);

    json_object* prop25=json_object_new_object();
    json_object_object_add(prop25,"type",json_object_new_string("string"));
    json_object_object_add(prop25,"format",json_object_new_string("uri"));

    json_object* prop26=json_object_new_array();

    map* tmpMap1=getMap(in->defaults->content,"crs");
    if(tmpMap1==NULL)
      return;
    json_object_array_add(prop26,json_object_new_string(tmpMap1->value));
    json_object_object_add(prop25,"default",json_object_new_string(tmpMap1->value));
    iotype* sup=in->supported;
    while(sup!=NULL){
      tmpMap1=getMap(sup->content,"crs");
      if(tmpMap1!=NULL)
	json_object_array_add(prop26,json_object_new_string(tmpMap1->value));
      sup=sup->next;
    }
    json_object_object_add(prop25,"enum",prop26);

    json_object_object_add(prop22,"crs",prop25);

    json_object_object_add(prop20,"properties",prop22);
    if(pmMin!=NULL && atoi(pmMin->value)==0)
      json_object_object_add(prop20,"nullable",json_object_new_boolean(true));
    json_object_object_add(input,"schema",prop20);
  }

  /**
   * Add schema property to a json_object (ComplexData)
   * @param m the main configuration maps pointer
   * @param iot the current iotype pointer
   * @param res the json_object pointer to add the properties to
   * @param isDefault boolean specifying if the currrent iotype is default
   * @param maxSize a map pointer to the maximumMegabytes param defined in the zcfg file for this input/output
   */
  void printFormatJ1(maps* m,iotype* iot,json_object* res,bool isDefault,map* maxSize){
    if(iot!=NULL){
      int i=0;
      json_object* prop1=json_object_new_object();
      map* tmpMap1=getMap(iot->content,"encoding");
      map* tmpMap2=getMap(iot->content,"mimeType");
      map* tmpMap3=getMap(iot->content,"schema");
      if(tmpMap2!=NULL && (strncasecmp(tmpMap2->value,"application/json",16)==0 || strstr(tmpMap2->value,"json")!=NULL)){
	json_object_object_add(prop1,"type",json_object_new_string("object"));
	if(tmpMap3!=NULL)
	  json_object_object_add(prop1,"$ref",json_object_new_string(tmpMap3->value));
	json_object_array_add(res,prop1);
	return ;
      }
      //json_object* prop2=json_object_new_object();
      json_object_object_add(prop1,"type",json_object_new_string("string"));
      if(tmpMap1!=NULL)
	json_object_object_add(prop1,"contentEncoding",json_object_new_string(tmpMap1->value));
      else{
	json_object_object_add(prop1,"contentEncoding",json_object_new_string("base64"));
	i=1;
      }
      if(tmpMap2!=NULL){
	json_object_object_add(prop1,"contentMediaType",json_object_new_string(tmpMap2->value));
      }
      // TODO: specific handling of schema?!
      if(tmpMap3!=NULL)
	json_object_object_add(prop1,"contentSchema",json_object_new_string(tmpMap3->value));
      if(maxSize!=NULL)
	json_object_object_add(prop1,"contentMaximumMegabytes",json_object_new_int64(atoll(maxSize->value)));
      json_object_array_add(res,prop1);
    }
  }
  
  /**
   * Add format properties to a json_object
   * @param m the main configuration maps pointer
   * @param iot the current iotype pointer
   * @param res the json_object pointer to add the properties to
   * @param isDefault boolean specifying if the currrent iotype is default
   * @param maxSize a map pointer to the maximumMegabytes param defined in the zcfg file for this input/output
   */
  void printFormatJ(maps* m,iotype* iot,json_object* res,bool isDefault,map* maxSize){
    if(iot!=NULL){
      map* tmpMap1=getMap(iot->content,"mimeType");
      json_object* prop1=json_object_new_object();
      json_object_object_add(prop1,"default",json_object_new_boolean(isDefault));
      json_object_object_add(prop1,"mediaType",json_object_new_string(tmpMap1->value));
      tmpMap1=getMap(iot->content,"encoding");
      if(tmpMap1!=NULL)
	json_object_object_add(prop1,"characterEncoding",json_object_new_string(tmpMap1->value));
      tmpMap1=getMap(iot->content,"schema");
      if(tmpMap1!=NULL)
	json_object_object_add(prop1,"schema",json_object_new_string(tmpMap1->value));
      if(maxSize!=NULL)
	json_object_object_add(prop1,"maximumMegabytes",json_object_new_int64(atoll(maxSize->value)));
      json_object_array_add(res,prop1);
    }
  }

  /**
   * Add additionalParameters property to a json_object
   * @param conf the main configuration maps pointer
   * @param meta a map pointer to the current metadata informations
   * @param doc the json_object pointer to add the property to
   */
  void printJAdditionalParameters(maps* conf,map* meta,json_object* doc){
    map* cmeta=meta;
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
   * @param conf the main configuration maps pointer
   * @param meta a map pointer to the current metadata informations
   * @param doc the json_object pointer to add the property to
   */
  void printJMetadata(maps* conf,map* meta,json_object* doc){
    map* cmeta=meta;
    json_object* carr=json_object_new_array();
    int hasElement=-1;
    int iCnt=0;
    int iLen=1;
    if(cmeta==NULL)
      return;
    map* pmLength=getMap(cmeta,"length");
    if(pmLength!=NULL)
      iLen=atoi(pmLength->value);
    char* pcFields[]={
      (char*)"role",
      (char*)"href",
      (char*)"title"
    };
    for(;iCnt<iLen;iCnt++){
      json_object* jcmeta=json_object_new_object();
      int iCnt1=0;
      for(;iCnt1<3;iCnt1++){
        map* pmValue=getMapArray(meta,pcFields[iCnt1],iCnt);
        if(pmValue!=NULL){
          if(strcasecmp(pmValue->name,"title")==0)
            json_object_object_add(jcmeta,pcFields[iCnt1],json_object_new_string(_(pmValue->value)));
          else
            if(iCnt1==1)
              json_object_object_add(jcmeta,"value",json_object_new_string(pmValue->value));
            else
              json_object_object_add(jcmeta,pcFields[iCnt1],json_object_new_string(pmValue->value));
          hasElement++;
        }else{
          if(iCnt1==1){
            char* pcaPrefix=(char*)malloc(10*sizeof(char));
            sprintf(pcaPrefix,"role_%d",iCnt);
            char* pcaLength=(char*)malloc(18*sizeof(char));
            sprintf(pcaLength,"%s_length",pcaPrefix);
            map* pmLength=getMap(cmeta,pcaLength);
            if(pmLength!=NULL){
              json_object* innerElement=json_object_new_object();
              int iLength1=atoi(pmLength->value);
              int iCnt2=0;
              for(;iCnt2<iLength1;iCnt2++){
                int iCnt3=0;
                char* pcaNameValue[2];
                for(;iCnt3<2;iCnt3++){
                  char* pcaName=(char*)malloc(20*sizeof(char));
                  sprintf(pcaName,"%s_%s",pcaPrefix,pcFields[iCnt3]);
                  map* pmRole=getMapArray(meta,pcaName,iCnt2);
                  if(pmRole!=NULL){
                    pcaNameValue[iCnt3]=zStrdup(pmRole->value);
                    free(pcaName);
                  }
                }
                if(pcaNameValue[0]!=NULL && pcaNameValue[1]!=NULL){
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
                if(pmRole!=NULL)
                  json_object_object_add(innerElement,pcFields[2],json_object_new_string(pmRole->value));
                free(pcaName);
              }
              json_object_object_add(jcmeta,"value",innerElement);
            }
            free(pcaPrefix);
            free(pcaLength);
          }
        }
      }
      json_object_array_add(carr,jcmeta);
    }
    if(hasElement>=0)
      json_object_object_add(doc,"metadata",carr);
    else
      json_object_put(carr);
  }

  /**
   * Add metadata properties to a json_object
   * @param m the main configuration maps pointer
   * @param io a string 
   * @param in an elements pointer to the current input/output
   * @param inputs the json_object pointer to add the property to
   * @param serv the service pointer to extract the metadata from
   */
  void printIOTypeJ(maps* m, const char *io, elements* in,json_object* inputs,service* serv){
    while(in!=NULL){
      json_object* input=json_object_new_object();
      map* tmpMap=getMap(in->content,"title");
      map* pmMin=NULL;
      map* pmMax=NULL;
      if(strcmp(io,"input")==0){
	pmMin=getMap(in->content,"minOccurs");
	pmMax=getMap(in->content,"maxOccurs");
      }
      if(tmpMap!=NULL)
	json_object_object_add(input,"title",json_object_new_string(_ss(tmpMap->value)));
      addDescription(in->content,0,input,NULL);
      if(strcmp(io,"input")==0){
	if(pmMin!=NULL && strcmp(pmMin->value,"1")!=0 && strcmp(pmMin->value,"0")!=0)
	  json_object_object_add(input,"minOccurs",json_object_new_int(atoi(pmMin->value)));
	if(pmMax!=NULL){
	  if(strncasecmp(pmMax->value,"unbounded",9)==0)
	    json_object_object_add(input,"maxOccurs",json_object_new_string(pmMax->value));
	  else
	    if(strcmp(pmMax->value,"1")!=0)
	      json_object_object_add(input,"maxOccurs",json_object_new_int(atoi(pmMax->value)));
	}
      }
      if(in->format!=NULL){
        //json_object* input1=json_object_new_object();
        json_object* prop0=json_object_new_array();
        json_object* prop1=json_object_new_array();
        json_object* pjoSchema=json_object_new_array();
        if(strcasecmp(in->format,"LiteralData")==0 ||
            strcasecmp(in->format,"LiteralOutput")==0){
          printLiteralDataJ(m,in,input);
          json_object_put(prop0);
          json_object_put(prop1);
          json_object_put(pjoSchema);
        }else{
          if(strcasecmp(in->format,"ComplexData")==0 ||
              strcasecmp(in->format,"ComplexOutput")==0) {
            map* sizeMap=getMap(in->content,"maximumMegabytes");
            printComplexHref(m,in,prop1,false,sizeMap);
            printFormatJ(m,in->defaults,prop0,true,sizeMap);
            json_object* pjoSchemaPart=json_object_new_array();
            printFormatJ1(m,in->defaults,pjoSchemaPart,true,sizeMap);


            printFormatJ1(m,in->defaults,pjoSchema,true,sizeMap);
            iotype* sup=in->supported;
            while(sup!=NULL){
              printFormatJ(m,sup,prop0,false,sizeMap);
              printFormatJ1(m,sup,pjoSchemaPart,false,sizeMap);
              printFormatJ1(m,sup,pjoSchema,true,sizeMap);
              sup=sup->next;
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
            json_object_array_add(prop1,pjoQualifiedValue);

            json_object* prop2=json_object_new_object();
            json_object_object_add(prop2,"oneOf",prop1);
            json_object* prop3=addArray(m,in);
            if(prop3!=NULL){
              json_object_object_add(prop3,"items",prop2);
              if(pmMin!=NULL && atoi(pmMin->value)==0)
                json_object_object_add(prop3,"nullable",json_object_new_boolean(true));
              json_object_object_add(input,"extended-schema",prop3);
            }
            else{
              if(pmMin!=NULL && atoi(pmMin->value)==0)
                json_object_object_add(prop2,"nullable",json_object_new_boolean(true));
              json_object_object_add(input,"extended-schema",prop2);
            }

            json_object* prop4=json_object_new_object();
            json_object_object_add(prop4,"oneOf",pjoSchema);
            json_object_object_add(input,"schema",prop4);
            json_object_put(prop0);
          }
          else{
            printBoundingBoxJ(m,in,input);
            json_object_put(prop0);
            json_object_put(prop1);
            json_object_put(pjoSchema);
          }
        }
      }
      printJMetadata(m,in->metadata,input);
      printJAdditionalParameters(m,in->additional_parameters,input);
      json_object_object_add(inputs,in->name,input);
      in=in->next;
    }
    
  }

  /**
   * Add all the capabilities properties to a json_object
   *
   * @param ref the registry pointer
   * @param m the main configuration maps pointer
   * @param doc0 the void (json_object) pointer to add the property to
   * @param nc0 the void (json_object) pointer to add the property to
   * @param serv the service pointer to extract the metadata from
   */
  void printGetCapabilitiesForProcessJ(registry *reg, maps* m,void* doc0,void* nc0,service* serv){
    json_object* doc=(json_object*) doc0;
    json_object* nc=(json_object*) nc0;
    json_object *res;
    if(doc!=NULL)
      res=json_object_new_object();
    else
      res=(json_object*) nc0;
#ifdef DRU_ENABLED
    if(serviceIsDRU(m,serv->name))
      return;
#endif
    if(serviceIsFilter(m,serv->name))
      return;
    map* tmpMap0=getMapFromMaps(m,"lenv","level");
    char* rUrl=serv->name;
    if(tmpMap0!=NULL && atoi(tmpMap0->value)>0){
      int i=0;
      maps* tmpMaps=getMaps(m,"lenv");
      char* tmpName=NULL;
      for(i=0;i<atoi(tmpMap0->value);i++){
        char* key=(char*)malloc(15*sizeof(char));
        sprintf(key,"sprefix_%d",i);
        map* tmpMap1=getMap(tmpMaps->content,key);
        free(key);
        if(tmpMap1!=NULL){
          if(i+1==atoi(tmpMap0->value))
            if(tmpName==NULL){
              tmpName=(char*) malloc((strlen(serv->name)+strlen(tmpMap1->value)+1)*sizeof(char));
              sprintf(tmpName,"%s%s",tmpMap1->value,serv->name);
            }else{
              char* tmpStr=zStrdup(tmpName);
              tmpName=(char*) realloc(tmpName,(strlen(tmpStr)+strlen(tmpMap1->value)+strlen(serv->name)+1)*sizeof(char));
              sprintf(tmpName,"%s%s%s",tmpStr,tmpMap1->value,serv->name);
              free(tmpStr);
            }
          else
            if(tmpName==NULL){
              tmpName=(char*) malloc((strlen(tmpMap1->value)+1)*sizeof(char));
              sprintf(tmpName,"%s",tmpMap1->value);
            }else{
              char* tmpStr=zStrdup(tmpName);
              tmpName=(char*) realloc(tmpName,(strlen(tmpStr)+strlen(tmpMap1->value)+1)*sizeof(char));
              sprintf(tmpName,"%s%s",tmpStr,tmpMap1->value);
              free(tmpStr);
          }
        }
      }
      if(tmpName!=NULL){
        json_object_object_add(res,"id",json_object_new_string(tmpName));
        rUrl=zStrdup(tmpName);
        free(tmpName);
      }
    }
    else{
      json_object_object_add(res,"id",json_object_new_string(serv->name));
      rUrl=serv->name;
    }
    if(serv->content!=NULL){
      map* tmpMap=getMap(serv->content,"title");
      if(tmpMap!=NULL){
        json_object_object_add(res,"title",json_object_new_string(_ss(tmpMap->value)));
      }
      addDescription(serv->content,0,res,NULL);
      map* pmMutable=getMap(serv->content,"mutable");
      int limit=4;
      int iCnt=0;
      if(pmMutable==NULL)
        json_object_object_add(res,"mutable",json_object_new_boolean(FALSE));
      else{
        if(strcmp(pmMutable->value,"false")==0)
          json_object_object_add(res,"mutable",json_object_new_boolean(FALSE));
        else {
          iCnt=2;
          limit=6;
          json_object_object_add(res,"mutable",json_object_new_boolean(TRUE));
        }
      }
      tmpMap=getMap(serv->content,"processVersion");
      if(tmpMap!=NULL){
        if(strlen(tmpMap->value)<5){
          char *val=(char*)malloc((strlen(tmpMap->value)+5)*sizeof(char));
          sprintf(val,"%s.0.0",tmpMap->value);
          json_object_object_add(res,"version",json_object_new_string(val));
          free(val);
        }
        else
          json_object_object_add(res,"version",json_object_new_string(tmpMap->value));
      }else
        json_object_object_add(res,"version",json_object_new_string("1.0.0"));
      tmpMap=getMap(serv->content,"keywords");
      if(tmpMap!=NULL){
        char* pcaListSections=zStrdup(tmpMap->value);
	      char *pcPtr;
	      char *pcCurrent = strtok_r (pcaListSections, ",", &pcPtr);
        json_object *res2=json_object_new_array();
        while(pcCurrent!=NULL){
          json_object_array_add(res2,json_object_new_string(pcCurrent));
          pcCurrent = strtok_r (NULL, ",", &pcPtr);
        }
        free(pcaListSections);
        json_object_object_add(res,"keywords",res2);
      }
      if(serv->metadata!=NULL){
        printJMetadata(m,serv->metadata,res);
      }
      if(serv->additional_parameters!=NULL){
        printJAdditionalParameters(m,serv->additional_parameters,res);
      }
      map* sType=getMap(serv->content,"serviceType");
      for(;iCnt<limit;iCnt+=2){
        json_object *res2=json_object_new_array();
        char *saveptr;
        char* dupStr=strdup(jcapabilities[iCnt+1]);
        char *tmps = strtok_r (dupStr, " ", &saveptr);
        while(tmps!=NULL){
          json_object_array_add(res2,json_object_new_string(tmps));
          tmps = strtok_r (NULL, " ", &saveptr);
        }
        free(dupStr);
        json_object_object_add(res,jcapabilities[iCnt],res2);
      }
      json_object *res1=json_object_new_array();
      json_object *res2=json_object_new_object();
      json_object *res3=json_object_new_object();
      map* pmTmp=getMapFromMaps(m,"lenv","requestType");
      if(pmTmp!=NULL && strncasecmp(pmTmp->value,"desc",4)==0){
        //Previous value: "process-desc"
        map* pmTmp=getMapFromMaps(m,"processes/{processID}","rel");
        if(pmTmp!=NULL)
          json_object_object_add(res2,"rel",json_object_new_string(pmTmp->value));
        else
          json_object_object_add(res2,"rel",json_object_new_string("self"));
      }
      else{
        //Previous value: "execute"
        map* pmTmp=getMapFromMaps(m,"processes/{processID}/execution","rel");
        if(pmTmp!=NULL)
          json_object_object_add(res2,"rel",json_object_new_string(pmTmp->value));
      }
      json_object_object_add(res3,"rel",json_object_new_string("alternate"));
      json_object_object_add(res3,"type",json_object_new_string("text/html"));
      json_object_object_add(res2,"type",json_object_new_string("application/json"));
      map* tmpUrl=getMapFromMaps(m,"openapi","rootUrl");
      char* tmpStr=(char*) malloc((strlen(tmpUrl->value)+strlen(rUrl)+13)*sizeof(char));
      sprintf(tmpStr,"%s/processes/%s",tmpUrl->value,rUrl);
      if(pmTmp!=NULL && strncasecmp(pmTmp->value,"desc",4)!=0){
        json_object_object_add(res2,"title",json_object_new_string("Execute End Point"));
        json_object_object_add(res3,"title",json_object_new_string("Execute End Point"));
        char* tmpStr1=zStrdup(tmpStr);
        tmpStr=(char*) realloc(tmpStr,(strlen(tmpStr1)+11)*sizeof(char));
        sprintf(tmpStr,"%s/execution",tmpStr1);
        free(tmpStr1);
      }else{
        json_object_object_add(res2,"title",json_object_new_string("Process Description"));
        json_object_object_add(res3,"title",json_object_new_string("Process Description"));
      }
      char* tmpStr3=(char*) malloc((strlen(tmpStr)+6)*sizeof(char));
      sprintf(tmpStr3,"%s.html",tmpStr);
      json_object_object_add(res3,"href",json_object_new_string(tmpStr3));
      free(tmpStr3);
      json_object_object_add(res2,"href",json_object_new_string(tmpStr));
      free(tmpStr);
      json_object_array_add(res1,res2);
      tmpUrl=getMapFromMaps(m,"openapi","partial_html_support");
      if(tmpUrl!=NULL && strncasecmp(tmpUrl->value,"true",4)==0)
        json_object_array_add(res1,res3);
      else
        json_object_put(res3);
      json_object_object_add(res,"links",res1);
    }
    if(doc==NULL){
      elements* in=serv->inputs;
      json_object* inputs=json_object_new_object();
      printIOTypeJ(m,"input",in,inputs,serv);
      json_object_object_add(res,"inputs",inputs);

      in=serv->outputs;
      json_object* outputs=json_object_new_object();
      printIOTypeJ(m,"output",in,outputs,serv);
      json_object_object_add(res,"outputs",outputs);

    }
    if(strcmp(rUrl,serv->name)!=0)
      free(rUrl);
    if(doc!=NULL)
      json_object_array_add(doc,res);
  }

  /**
   * Produce an exception JSON object
   * 
   * @param m the maps containing the settings of the main.cfg file
   * @param s the map containing the text,code,locator keys (or a map array of the same keys)
   * @return the create JSON object (make sure to free memory)
   */
  json_object *createExceptionJ(maps* m,map* s){
    json_object *res=json_object_new_object();
    map* pmTmp=getMap(s,"code");
    if(pmTmp==NULL)
      pmTmp=getMapFromMaps(m,"lenv","code");
    if(pmTmp!=NULL){
      json_object_object_add(res,"title",json_object_new_string(_(pmTmp->value)));
      int i=0;
      int hasType=-1;
      for(i=0;i<7;i++){
        if(strcasecmp(pmTmp->value,WPSExceptionCode[OAPIPCorrespondances[i][0]])==0){
          map* pmExceptionUrl=getMapFromMaps(m,"openapi","exceptionsUrl");
          // Use OGC API - Processes - Part 2: Deploy, Replace, Undeploy
          // exceptions
          // cf. /req/deploy-replace-undeploy/deploy/response-duplicate
          // cf. /req/deploy-replace-undeploy/deploy/response-immutable
          // cf. /req/deploy-replace-undeploy/unsupported-content-type
          if(i>=4)
            pmExceptionUrl=getMapFromMaps(m,"openapi","exceptionsUrl_1");
          char* pcaTmp=(char*)malloc((strlen(pmExceptionUrl->value)+strlen(OAPIPExceptionCode[OAPIPCorrespondances[i][1]])+2)*sizeof(char));
          sprintf(pcaTmp,"%s/%s",pmExceptionUrl->value,OAPIPExceptionCode[OAPIPCorrespondances[i][1]]);
          json_object_object_add(res,"type",json_object_new_string(pcaTmp));
          free(pcaTmp);
          hasType=0;
          break;
        }
      }
      if(hasType<0){
        json_object_object_add(res,"type",json_object_new_string(pmTmp->value));
      }
    }
    else{
      json_object_object_add(res,"title",json_object_new_string("NoApplicableCode"));
      json_object_object_add(res,"type",json_object_new_string("NoApplicableCode"));
    }
    pmTmp=getMap(s,"text");
    if(pmTmp==NULL)
      pmTmp=getMap(s,"message");
    if(pmTmp==NULL)
      pmTmp=getMapFromMaps(m,"lenv","message");
    if(pmTmp!=NULL)
      json_object_object_add(res,"detail",json_object_new_string(pmTmp->value));
    if(getMapFromMaps(m,"openapi","use_problem_exception"))
      setMapInMaps(m,"headers","Content-Type","application/problem+json;charset=UTF-8");
    return res;
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
    json_object *res=createExceptionJ(*pmsConf,s);
    maps* tmpMap=getMaps(*pmsConf,"main");
    const char *exceptionCode;
    map* pmTmp=getMap(s,"code");
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
      printHeaders(*pmsConf);
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
    const char* jsonStr=json_object_to_json_string_ext(res,JSON_C_TO_STRING_NOSLASHESCAPE);
    if(getMapFromMaps(*pmsConf,"lenv","jsonStr")==NULL)
      setMapInMaps(*pmsConf,"lenv","jsonStr",jsonStr);
    if(pmHasprinted==NULL || strncasecmp(pmHasprinted->value,"false",5)==0){
      printf(jsonStr);
      if(*pmsConf!=NULL){
	setMapInMaps(*pmsConf,"lenv","hasPrinted","true");
	setMapInMaps(*pmsConf,"lenv","hasExceptionPrinted","true");
      }
    }
    json_object_put(res);
  }

  /**
   * Parse LiteralData value
   * @param conf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param element
   * @param output the maps to set current json structure
   */
  void parseJLiteral(maps* conf,json_object* req,elements* element,maps* output){
    json_object* json_cinput=NULL;
    if(json_object_object_get_ex(req,"value",&json_cinput)!=FALSE){
      output->content=createMap("value",json_object_get_string(json_cinput));
    }
    const char* tmpStrs[2]={
      "dataType",
      "uom"
    };
    for(int i=0;i<2;i++)
      if(json_object_object_get_ex(req,tmpStrs[i],&json_cinput)!=FALSE){
	json_object* json_cinput1;
	if(json_object_object_get_ex(json_cinput,"name",&json_cinput1)!=FALSE){
	  if(output->content==NULL)
	    output->content=createMap(tmpStrs[i],json_object_get_string(json_cinput1));
	  else
	    addToMap(output->content,tmpStrs[i],json_object_get_string(json_cinput1));
	}
	if(json_object_object_get_ex(json_cinput,"reference",&json_cinput1)!=FALSE){
	  if(output->content==NULL)
	    output->content=createMap(tmpStrs[i],json_object_get_string(json_cinput1));
	  else
	    addToMap(output->content,tmpStrs[i],json_object_get_string(json_cinput1));
	}
      }
  }

  /**
   * Search field names used in the OGC API - Processes specification and 
   * replace them by old names from WPS.
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param req json_object to fetch attributes from
   * @param output the maps to store metadata informations as a maps
   */ 
  void checkCorrespondingJFields(maps* conf,json_object* req,maps* output){
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
   * Add a http reques to the queue
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param pccValue the URL to fetch
   * @param name the name of the input
   */
  void addToJRequests(maps** pmsConf,const char* pccValue,const char* name){
    int len=0;
    int createdStr=0;
    char *tmpStr=(char*)"url";
    char *tmpStr1=(char*)"input";
    maps* conf=*pmsConf;
    if(getMaps(conf,"http_requests")==NULL){
      maps* tmpMaps=createMaps("http_requests");
      tmpMaps->content=createMap("length","1");
      addMapsToMaps(&conf,tmpMaps);
      freeMaps(&tmpMaps);
      free(tmpMaps);
    }else{
      map* tmpMap=getMapFromMaps(conf,"http_requests","length");
      int len=atoi(tmpMap->value);
      createdStr=1;
      tmpStr=(char*) malloc((12)*sizeof(char));
      sprintf(tmpStr,"%d",len+1);
      setMapInMaps(conf,"http_requests","length",tmpStr);
      sprintf(tmpStr,"url_%d",len);
      tmpStr1=(char*) malloc((14)*sizeof(char));
      sprintf(tmpStr1,"input_%d",len);
    }
    setMapInMaps(conf,"http_requests",tmpStr,pccValue);
    setMapInMaps(conf,"http_requests",tmpStr1,name);
    if(createdStr>0){
      free(tmpStr);
      free(tmpStr1);
    }
  }

  /**
   * Parse a nested process json object
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param element the metadata for this input
   * @param output the maps to set current json structure
   * @param name the name of the request from http_requests
   */
  void parseNestedProcess(maps* conf,json_object* req,elements* element,maps* output,const char* name){
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
      map* pmExecuteMode=getMapFromMaps(conf,"lenv","execute_mode");
      addToMap(output->content,"Body",pccBody);
      addToMap(output->content,"Headers","Content-Type: application/json");
      if(pmExecuteMode!=NULL && strncasecmp(pmExecuteMode->value,"async",5)==0){
        map* pmExtraSupportedCodes=getMapFromMaps(conf,"main","extra_supported_codes");
        if(pmExtraSupportedCodes!=NULL){
          char* pcaTmp=(char*)malloc((strlen(pmExtraSupportedCodes->value)+5)*sizeof(char));
          sprintf(pcaTmp,"%s,201",pmExtraSupportedCodes->value);
          setMapInMaps(conf,"main","extra_supported_codes",pcaTmp);
          free(pcaTmp);
        }
        else
          setMapInMaps(conf,"main","extra_supported_codes","201");
        addToMap(output->content,"Headers_extra","Prefer: respond-async;return=representation");
      }
      addToJRequests(&conf,pcaTmp,name);
      map* pmTmp=getMapFromMaps(conf,"http_requests","length");
      if(pmTmp!=NULL){
        ZOO_DEBUG("Nested process execution invocation");
        maps* pmsRequests=getMaps(conf,"http_requests");
        setMapArray(pmsRequests->content,"body",atoi(pmTmp->value)-1, pccBody);
        setMapArray(pmsRequests->content,"Headers",atoi(pmTmp->value)-1, "Content-Type: application/json");
        if(pmExecuteMode!=NULL && strncasecmp(pmExecuteMode->value,"async",5)==0){
          setMapArray(pmsRequests->content,"Headers_extra",atoi(pmTmp->value)-1, "Preference: respond-async;return=representation");
        }
        // Add jobs_handlers section in the main configuration maps
        addToMap(output->content,"require_job_handler", "true");
        setMapArray(pmsRequests->content,"require_job_handler",atoi(pmTmp->value)-1, "true");
        maps* pmsJobHandlers=getMaps(conf,"jobs_handlers");
        char* pcaTmp=(char*)malloc((11)*sizeof(char));
        sprintf(pcaTmp,"%d",atoi(pmTmp->value)-1);
        if(pmsJobHandlers==NULL){
          maps* tmpMaps=createMaps("jobs_handlers");
          tmpMaps->content=createMap("value",pcaTmp);
          addToMap(tmpMaps->content,"length","1");
          addMapsToMaps(&conf,tmpMaps);
          freeMaps(&tmpMaps);
          free(tmpMaps);
        }else{
          map* pmsNbJobHandlers=getMapFromMaps(conf,"jobs_handlers","length");
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param element the metadata
   * @param output the maps to set current json structure
   * @param name the name of the request from http_requests
   */
  void parseJComplex(maps* conf,json_object* req,elements* element,maps* output,const char* name){
    json_object* json_cinput=NULL;
    if(json_object_object_get_ex(req,"value",&json_cinput)!=FALSE){
      output->content=createMap("value",json_object_get_string(json_cinput));
    }
    else{
      json_object* json_value=NULL;
      if(json_object_object_get_ex(req,"href",&json_value)!=FALSE){
        output->content=createMap("xlink:href",json_object_get_string(json_value));
        addToJRequests(&conf,json_object_get_string(json_value),name);
      }
    }
    if(json_object_object_get_ex(req,"format",&json_cinput)!=FALSE){
      json_object_object_foreach(json_cinput, key, val) {
        if(output->content==NULL)
          output->content=createMap(key,json_object_get_string(val));
        else
          addToMap(output->content,key,json_object_get_string(val));
      }
      checkCorrespondingJFields(conf,json_cinput,output); 
    }
    checkCorrespondingJFields(conf,req,output); 
  }

  /**
   * Parse BoundingBox value
   * @param conf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param element the metadata
   * @param output the maps to set current json structure
   */
  void parseJBoundingBox(maps* conf,json_object* req,elements* element,maps* output){
    json_object* json_cinput=NULL;
    if(json_object_object_get_ex(req,"bbox",&json_cinput)!=FALSE){
      output->content=createMap("value",json_object_get_string(json_cinput));
    }
    if(json_object_object_get_ex(req,"crs",&json_cinput)!=FALSE){
      if(output->content==NULL)
	output->content=createMap("crs",json_object_get_string(json_cinput));
      else
	addToMap(output->content,"crs","http://www.opengis.net/def/crs/OGC/1.3/CRS84");
    }
    char* tmpStrs[2]={
      (char*)"lowerCorner",
      (char*)"upperCorner"
    };
    for(int i=0;i<2;i++)
      if(json_object_object_get_ex(req,tmpStrs[i],&json_cinput)!=FALSE){
	if(output->content==NULL)
	  output->content=createMap(tmpStrs[i],json_object_get_string(json_cinput));
	else
	  addToMap(output->content,tmpStrs[i],json_object_get_string(json_cinput));
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   * @param key char* the input/output name
   * @param val json_object pointing to the input/output
   * @param cMaps the outputed maps containing input (or output) metedata 
   */
  void _parseJIOSingle1(maps* conf,elements* cio, maps** ioMaps,const char* ioType,const char* key,json_object* val,maps* cMaps){
    json_object* json_cinput;
    if(strncmp(ioType,"inputs",6)==0) {
      if(json_object_is_type(val,json_type_object)) {
        // ComplexData
        elements* peCurrentInput=getElements(cio,cMaps->name);
        // TODO: review if this is required as now a process can be the value
        // for everything (including LiteralData)
        if(getMapFromMaps(conf,"openapi","ensure_type_validation")!=NULL)
          if(peCurrentInput!=NULL){
            if(peCurrentInput->format!=NULL
              && strcasecmp(peCurrentInput->format,"LiteralData")!=0
              && strcasecmp(peCurrentInput->format,"ComplexData")!=0
              && strcasecmp(peCurrentInput->format,"BoundingBoxData")!=0){
              return returnBadRequestException(conf,peCurrentInput);
            }
          }
        if(json_object_object_get_ex(val,"process",&json_cinput) != FALSE){
          parseNestedProcess(conf,val,cio,cMaps,key);
        } else if(json_object_object_get_ex(val,"value",&json_cinput)!= FALSE ||
            json_object_object_get_ex(val,"href",&json_cinput) != FALSE ){
          parseJComplex(conf,val,cio,cMaps,key);
        } else if( json_object_object_get_ex(val,"bbox",&json_cinput)!= FALSE ){
          parseJBoundingBox(conf,val,cio,cMaps);
        } else if(getMapFromMaps(conf,"openapi","ensure_type_validation")!=NULL){
          return returnBadRequestException(conf,peCurrentInput);
        }
      } else if(json_object_is_type(val,json_type_array)){
        // Need to run detection on each item within the array
      }else{
        // Basic types
        if(getMapFromMaps(conf,"openapi","ensure_type_validation")!=NULL
          &&
          getMapFromMaps(conf,"openapi","ensure_type_validation_but_ets")==NULL){
          elements* peCurrentInput=getElements(cio,cMaps->name);
          if(peCurrentInput!=NULL){
            if(peCurrentInput->format!=NULL && strcasecmp(peCurrentInput->format,"LiteralData")!=0){
              map* error=createMap("code","BadRequest");
              char tmpS1[1024];
              sprintf(tmpS1,_("Wrong type used for input %s"),peCurrentInput->name);
              addToMap(error,"message",tmpS1);
              setMapInMaps(conf,"lenv","status_code","400 Bad Request");
              printExceptionReportResponseJ(&conf,error);
              return;
            }
          }
        }
        cMaps->content=createMap("value",json_object_get_string(val));
      }
    }
    else{
      if(json_object_object_get_ex(val,"dataType",&json_cinput)!=FALSE){
        parseJLiteral(conf,val,cio,cMaps);
      } else if(json_object_object_get_ex(val,"format",&json_cinput)!=FALSE){
        parseJComplex(conf,val,cio,cMaps,key);
      } else if(json_object_object_get_ex(val,"bbox",&json_cinput)!=FALSE){
        parseJBoundingBox(conf,val,cio,cMaps);
      }// else error!
      else{
        if(json_object_object_get_ex(val,"value",&json_cinput)!=FALSE){
          map* error=createMap("code","BadRequest");
          char tmpS[1024];
          sprintf(tmpS,_("Missing input for %s"),cio->name);
          addToMap(error,"message",tmpS);
          setMapInMaps(conf,"lenv","status_code","400 Bad Request");
          printExceptionReportResponseJ(&conf,error);
          return;
        }else{
          if(json_object_get_type(val)==json_type_string){
            parseJLiteral(conf,val,cio,cMaps);
          }else if(json_object_get_type(val)==json_type_object){
            json_object* json_ccinput=NULL;
            if(json_object_object_get_ex(val,"bbox",&json_ccinput)!=FALSE){
              parseJBoundingBox(conf,val,cio,cMaps);
            }
            else{
              parseJComplex(conf,val,cio,cMaps,key);
            }
          }else{
            if(strcmp(ioType,"input")==0){
              map* error=createMap("code","BadRequest");
              char tmpS1[1024];
              sprintf(tmpS1,_("Issue with input %s"),cio->name);
              addToMap(error,"message",tmpS1);
              setMapInMaps(conf,"lenv","status_code","400 Bad Request");
              printExceptionReportResponseJ(&conf,error);
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   * @param key char* the input/output name
   * @param val json_object pointing to the input/output
   * @param cMaps the outputed maps containing input (or output) metedata 
   */
  void _parseJIOSingle(maps* conf,elements* cio, maps** ioMaps,const char* ioType,const char* key,json_object* val,maps* cMaps){
    json_object* json_cinput;
    if(json_object_object_get_ex(val,"dataType",&json_cinput)!=FALSE){
      parseJLiteral(conf,val,cio,cMaps);
    } else if(json_object_object_get_ex(val,"format",&json_cinput)!=FALSE){
      parseJComplex(conf,val,cio,cMaps,key);
    } else if(json_object_object_get_ex(val,"bbox",&json_cinput)!=FALSE){
      parseJBoundingBox(conf,val,cio,cMaps);
    }// else error!
    else{
      if(json_object_object_get_ex(val,"value",&json_cinput)!=FALSE){
        map* error=createMap("code","BadRequest");
        char tmpS[1024];
        sprintf(tmpS,_("Missing input for %s"),cio->name);
        addToMap(error,"message",tmpS);
        setMapInMaps(conf,"lenv","status_code","400 Bad Request");
        printExceptionReportResponseJ(&conf,error);
        return;
      }else{
        if(json_object_get_type(json_cinput)==json_type_string){
          parseJLiteral(conf,val,cio,cMaps);
        }else if(json_object_get_type(json_cinput)==json_type_object){
          json_object* json_ccinput=NULL;
          if(json_object_object_get_ex(json_cinput,"bbox",&json_ccinput)!=FALSE){
            parseJComplex(conf,val,cio,cMaps,key);
          }
          else{
            parseJBoundingBox(conf,val,cio,cMaps);
          }
        }else{
          if(strcmp(ioType,"input")==0){
            map* error=createMap("code","BadRequest");
            char tmpS1[1024];
            sprintf(tmpS1,_("Issue with input %s"),cio->name);
            addToMap(error,"message",tmpS1);
            setMapInMaps(conf,"lenv","status_code","400 Bad Request");
            printExceptionReportResponseJ(&conf,error);
            return;
          }
        }
      }
    }    
  }
  
  /**
   * Parse a single input / output entity that can be an array 
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   * @param key char* the input/output name
   * @param val json_object pointing to the input/output
   */
  void parseJIOSingle(maps* conf,elements* ioElements, maps** ioMaps,const char* ioType,const char* key,json_object* val){
    maps *cMaps=NULL;
    cMaps=createMaps(key);
    elements* cio=getElements(ioElements,key);    
    if(json_object_is_type(val,json_type_array)){
      int i=0;
      size_t len=json_object_array_length(val);
      for(i=0;i<len;i++){
        json_object* json_current_io=json_object_array_get_idx(val,i);
        maps* pmsExtra=createMaps(key);
        _parseJIOSingle1(conf,cio,ioMaps,ioType,key,json_current_io,pmsExtra);
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
      _parseJIOSingle1(conf,cio,ioMaps,ioType,key,val,cMaps);
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
        json_object_object_foreach(json_cinput, key1, val1) {
          if(cMaps->content==NULL)
            cMaps->content=createMap(key1,json_object_get_string(val1));
          else
            addToMap(cMaps->content,key1,json_object_get_string(val1));
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param req json_object pointing to the input/output
   * @param ioElements the elements extracted from the zcfg
   * @param ioMaps the produced maps containing inputs (or outputs) 
   * @param ioType the char* set to inputs or outputs
   */
  void parseJIO(maps* conf, json_object* req, elements* ioElements, maps** ioMaps,const char* ioType){
    json_object* json_io=NULL;
    if(json_object_object_get_ex(req,ioType,&json_io)!=FALSE){
      json_object* json_current_io=NULL;
      if(json_object_is_type(json_io,json_type_array)){
        size_t len=json_object_array_length(json_io);
        for(int i=0;i<len;i++){
          maps *cMaps=NULL;
          json_current_io=json_object_array_get_idx(json_io,i);
          json_object* cname=NULL;
          if(json_object_object_get_ex(json_current_io,"id",&cname)!=FALSE) {
            json_object* json_input=NULL;
            if(json_object_object_get_ex(json_current_io,"input",&json_input)!=FALSE) {
              parseJIOSingle(conf,ioElements,ioMaps,ioType,json_object_get_string(cname),json_input);
            }else
              parseJIOSingle(conf,ioElements,ioMaps,ioType,json_object_get_string(cname),json_current_io);
          }
        }
      }else{
        json_object_object_foreach(json_io, key, val) {
          parseJIOSingle(conf,ioElements,ioMaps,ioType,key,val);
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param s the current service metadata
   * @param req the JSON object of the request body
   * @param inputs the produced maps
   * @param outputs the produced maps
   */
  void parseJRequest(maps* conf, service* s,json_object* req, map* request_inputs, maps** inputs,maps** outputs){
    elements* io=s->inputs;
    json_object* json_io=NULL;
    int parsed=0;
    char* tmpS=(char*)"input";
    maps* in=*inputs;
    parseJIO(conf,req,s->inputs,inputs,"inputs");
    parseJIO(conf,req,s->outputs,outputs,"outputs");

    if(parsed==0){
      json_io=NULL;
      if(json_object_object_get_ex(req,"mode",&json_io)!=FALSE){
        addToMap(request_inputs,"mode",json_object_get_string(json_io));
        setMapInMaps(conf,"request","mode",json_object_get_string(json_io));
      }
      json_io=NULL;
      map* preference=getMapFromMaps(conf,"renv","HTTP_PREFER");
      if(preference!=NULL && strstr(preference->value,"return=minimal")!=NULL){
        addToMap(request_inputs,"response","raw");
        setMapInMaps(conf,"request","response","raw");
      }
      else{
        if(preference!=NULL && strstr(preference->value,"return=representation")!=NULL){
          addToMap(request_inputs,"response","document");
          setMapInMaps(conf,"request","response","document");
        }
      }
      if(json_object_object_get_ex(req,"response",&json_io)!=FALSE){
        if(getMap(request_inputs,"response")==NULL)
          addToMap(request_inputs,"response",json_object_get_string(json_io));
        else{
          map* pmTmp=getMap(request_inputs,"response");
          free(pmTmp->value);
          pmTmp->value=zStrdup(json_object_get_string(json_io));
        }
        setMapInMaps(conf,"request","response",json_object_get_string(json_io));
      }else{
        if(getMap(request_inputs,"response")==NULL){
          // OGC Test Suite requirement (the execute.yaml schema said that it should be raw)
          if(getMapFromMaps(conf,"openapi","default_result_as_document")!=NULL){
            addToMap(request_inputs,"response","document");
            setMapInMaps(conf,"request","response","document");
          }else{
            addToMap(request_inputs,"response","raw");
            setMapInMaps(conf,"request","response","raw");
          }
        }
      }
      json_io=NULL;
      if(json_object_object_get_ex(req,"subscriber",&json_io)!=FALSE){
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
        addMapsToMaps(&conf,subscribers);
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
  json_object* printJobStatus(maps* pmsConf,char* pcJobId){
    json_object* pjoRes=NULL;
    runGetStatus(pmsConf,pcJobId,(char*)"GetStatus");
    map* pmError=getMapFromMaps(pmsConf,"lenv","error");
    if(pmError!=NULL && strncasecmp(pmError->value,"true",4)==0){
      printExceptionReportResponseJ(&pmsConf,getMapFromMaps(pmsConf,"lenv","code"));
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
          int iS=zStat(pcaLenvPath, &zssLenv_status);
          if(iS!=0)
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
          char* token, *saveptr,*pcaDecodedValue,*pcaDupValue;
          int iCnt=0;
          pcaDupValue=zStrdup(pmTimeInterval->value);
          pcaDecodedValue=url_decode(pcaDupValue);
          free(pcaDupValue);
          token=strtok_r(pcaDecodedValue,"/",&saveptr);
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
            token=strtok_r(NULL,"/",&saveptr);
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
   * @param conf the maps containing the settings of the main.cfg file
   * @return the JSON object pointer to the jobs list
   */
  json_object* printJobList(maps* conf){
    json_object* res=json_object_new_array();
    map* pmJobs=getMapFromMaps(conf,"lenv","selectedJob");
    map* tmpPath=getMapFromMaps(conf,"main","tmpPath");
    struct dirent *dp;
    int cnt=0;
    int skip=0;
    int limit=10000;
    map* pmLimit=getMapFromMaps(conf,"lenv","serviceCntLimit");
    map* pmSkip=getMapFromMaps(conf,"lenv","serviceCntSkip");
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
	json_object_put(res);
	printExceptionReportResponseJ(&conf,pmaTmp);
	freeMap(&pmaTmp);
	free(pmaTmp);
	return NULL;
      }
      char *saveptr;
      char *tmps = strtok_r(pmJobs->value, ",", &saveptr);
      while(tmps!=NULL){
	if(cnt>=skip && cnt<limit+skip && strlen(tmps)>2){
	  char* tmpStr=zStrdup(tmps+1);
	  tmpStr[strlen(tmpStr)-1]=0;
	  json_object* cjob=printJobStatus(conf,tmpStr);
	  json_object_array_add(res,cjob);
	  free(tmpStr);
	}
	if(cnt==limit+skip)
	  setMapInMaps(conf,"lenv","serviceCntNext","true");
	cnt++;
	tmps = strtok_r (NULL, ",", &saveptr);
      }
    }else{
      DIR *dirp = opendir (tmpPath->value);
      if(dirp!=NULL){
	while ((dp = readdir (dirp)) != NULL){
	  char* extn = strstr(dp->d_name, ".json");
	  if(extn!=NULL && strlen(dp->d_name)==41){
	    char* tmpStr=zStrdup(dp->d_name);
	    tmpStr[strlen(dp->d_name)-5]=0;
#ifndef RELY_ON_DB
	    map* pmTmpPath=getMapFromMaps(conf,"main","tmpPath");
	    char* pcaLenvPath=(char*)malloc((strlen(tmpStr)+strlen(pmTmpPath->value)+11)*sizeof(char));
	    sprintf(pcaLenvPath,"%s/%s_lenv.cfg",pmTmpPath->value,tmpStr);
	    maps *pmsLenv = (maps *) malloc (MAPS_SIZE);
	    pmsLenv->content = NULL;
	    pmsLenv->child = NULL;
	    pmsLenv->next = NULL;
	    map *pmaPid=NULL, *pmaUsid=NULL;
	    if(conf_read(pcaLenvPath,pmsLenv) !=2){
	      map *pmTmp=getMapFromMaps(pmsLenv,"lenv","Identifier");
	      if(pmTmp!=NULL){
		pmaPid=createMap("value",pmTmp->value);
		addToMap(pmaPid,"toRemove","true");
	      }
	      pmTmp=getMapFromMaps(pmsLenv,"lenv","usid");
	      if(pmTmp!=NULL){
		pmaUsid=createMap("value",pmTmp->value);
		addToMap(pmaPid,"toRemove","true");
	      }
	      freeMaps(&pmsLenv);
	      free(pmsLenv);
	    }else{
	      pmaPid=createMap("toRemove","true");
	      pmaUsid=createMap("toRemove","true");
	    }
	    free(pcaLenvPath);
#else
	    map *pmaPid=NULL, *pmaUsid=NULL;
#endif
	    if(isFilteredPid(conf,pmaPid->value) &&
	       isFilteredDMM(conf,pmaUsid->value)){
	      if(cnt>=skip && cnt<limit+skip){
		json_object* cjob=printJobStatus(conf,tmpStr);
		json_object_array_add(res,cjob);
	      }
	      if(cnt==limit+skip)
		setMapInMaps(conf,"lenv","serviceCntNext","true");
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
	    free(tmpStr);
	  }
	}
	closedir (dirp);
      }
    }
    json_object* resFinal=json_object_new_object();
    json_object_object_add(resFinal,"jobs",res);
    setMapInMaps(conf,"lenv","path","jobs");
    char pcCnt[10];
    sprintf(pcCnt,"%d",cnt);
    setMapInMaps(conf,"lenv","serviceCnt",pcCnt);
    createNextLinks(conf,resFinal);
    return resFinal;
  }
  
  /**
   * Print the result of an execution
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param s service pointer to metadata
   * @param result outputs of the service
   * @param res the status of execution SERVICE_FAILED/SERVICE_SUCCEEDED
   * @return the JSON object pointer to the result
   */
  json_object* printJResult(maps* conf,service* s,maps* result,int res){

    if(res==SERVICE_FAILED){
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
      if (res == SERVICE_DEPLOYED){
          setMapInMaps(conf,"headers","Status","201 Service Deployed");
      }
      printRawdataOutputs(conf,s,resu);
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
      while(resu!=NULL){
	json_object* res1=json_object_new_object();
	if(pmIOAsArray!=NULL && strncasecmp(pmIOAsArray->value,"true",4)==0)
	  json_object_object_add(res1,"id",json_object_new_string(resu->name));
	map* tmpMap=getMap(resu->content,"mimeType");
	json_object* res3=json_object_new_object();
	if(tmpMap!=NULL)
	  json_object_object_add(res3,"mediaType",json_object_new_string(tmpMap->value));
	else{
	  json_object_object_add(res3,"mediaType",json_object_new_string("text/plain"));
	}
	map* pmTransmissionMode=NULL;
	map* pmPresence=getMap(resu->content,"inRequest");
	if(((tmpMap=getMap(resu->content,"value"))!=NULL ||
	   (getMap(resu->content,"generated_file"))!=NULL) &&
	   (pmPresence!=NULL && strncmp(pmPresence->value,"true",4)==0)){
	  map* tmpMap1=NULL;
	  if((tmpMap1=getMap(resu->content,"transmissionMode"))!=NULL) {
	    pmTransmissionMode=getMap(resu->content,"transmissionMode");
	    if(strcmp(tmpMap1->value,"value")==0) {
	      map* tmpMap2=getMap(resu->content,"mimeType");
	      if(tmpMap2!=NULL && strstr(tmpMap2->value,"json")!=NULL){
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
		json_object_object_add(res3,"encoding",json_object_new_string("utf-8"));
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
		    int s=zStat(gfile->value, &f_status);
		    if(f_status.st_size>0){
		      char* pcaTmp=(char*)malloc((f_status.st_size+1)*sizeof(char));
		      fread(pcaTmp,1,f_status.st_size,pfData);
		      pcaTmp[f_status.st_size]=0;
		      fclose(pfData);
		      outputSingleJsonComplexRes(conf,resu,res1,res3,pcaTmp,f_status.st_size);
		      free(pcaTmp);
		    }
		  }
		}else{
		  json_object_object_add(res3,"encoding",json_object_new_string("utf-8"));
		  map* tmpMap0=getMap(resu->content,"crs");
		  if(tmpMap0!=NULL){
		    map* tmpMap2=getMap(resu->content,"value");
		    json_object *jobj = NULL;
		    const char *mystring = NULL;
		    int slen = 0;
		    enum json_tokener_error jerr;
		    struct json_tokener* tok=json_tokener_new();
		    do {
		      mystring = tmpMap2->value;  // get JSON string, e.g. read from file, etc...
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
		    json_object_object_add(res1,"crs",json_object_new_string(tmpMap0->value));
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
		      outputSingleJsonComplexRes(conf,resu,res1,res3,tmpMap->value,len);
		    }
		    else{
		      if(tmpMap!=NULL)
			json_object_object_add(res1,"value",json_object_new_string(tmpMap->value));
		    }
		  }
		}
		if(getMapFromMaps(conf,"oas","noformat")!=NULL){
		  // This is an option to not use the format key and allocate
		  // a the format object keys to the result itself
		  map* tmpMap0=getMap(resu->content,"crs");
		  if(tmpMap0==NULL){
		    json_object_object_foreach(res3, key, val) {
		      if(strncasecmp(key,pccFields[0],4)==0 ||
			 strncasecmp(key,pccFields[3],8)==0 ||
			 strncasecmp(key,pccFields[6],15)==0)
			json_object_object_add(res1,pccFields[3],json_object_new_string(json_object_get_string(val)));
		      if(strncasecmp(key,pccFields[1],4)==0 ||
			 strncasecmp(key,pccFields[4],8)==0 ||
			 strncasecmp(key,pccFields[7],15)==0)
			json_object_object_add(res1,pccFields[4],json_object_new_string(json_object_get_string(val)));
		      if(strncasecmp(key,pccFields[2],4)==0 ||
			 strncasecmp(key,pccFields[5],8)==0 ||
			 strncasecmp(key,pccFields[8],15)==0)
			json_object_object_add(res1,pccFields[5],json_object_new_string(json_object_get_string(val)));
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
		setReferenceUrl(conf,resu);
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
		  json_object_object_foreach(res3, key, val) {
		    if(strncasecmp(key,pccFields[0],4)==0 ||
		       strncasecmp(key,pccFields[3],8)==0 ||
		       strncasecmp(key,pccFields[6],15)==0)
		      json_object_object_add(res1,pccFields[0],json_object_new_string(json_object_get_string(val)));
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
	      json_object_object_add(res1,"format",res3);
	  }
	  else
	    json_object_put(res3);
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
		json_object_object_add(eres,resu->name,json_object_new_string(tmpMap->value));
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
      map *sessId = getMapFromMaps (conf, "lenv", "usid");
      if(tmpPath!=NULL){
	char *pcaTmp=(char*)malloc((strlen(tmpPath->value)+strlen(cIdentifier->value)+strlen(sessId->value)+8)*sizeof(char));
	sprintf(pcaTmp,"%s/%s_%s.json",
		tmpPath->value,cIdentifier->value,sessId->value);
	zStatStruct zsFStatus;
	int iS=zStat(pcaTmp, &zsFStatus);
	if(iS==0 && zsFStatus.st_size>0){
	  map* tmpPath1 = getMapFromMaps (conf, "main", "tmpUrl");
	  char* pcaTmpUrl=(char*)malloc((strlen(tmpPath1->value)+strlen(cIdentifier->value)+strlen(sessId->value)+8)*sizeof(char));;
	  sprintf(pcaTmpUrl,"%s/%s_%s.json",tmpPath1->value,
		  cIdentifier->value,sessId->value);
	  if(getMapFromMaps(conf,"lenv","gs_location")==NULL)
	    setMapInMaps(conf,"headers","Location",pcaTmpUrl);
	  free(pcaTmpUrl);
	}
	free(pcaTmp);
      }
      if(res==3){
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
   * Append required field to Json objects for a complex output
   * 
   * @param conf maps pointer to the main configuration maps
   * @param resu maps pointer to the output
   * @param res1 json_object pointer to which the value field should be added
   * @param res3 json_object pointer to the format object associated with res1
   * @param accValue char pointer to the value to be allocated
   * @param len length of apcValue
   */
  void outputSingleJsonComplexRes(maps* conf,maps* resu,json_object* res1,json_object* res3,char* apcValue, long len){
    map* pmMediaType=getMap(resu->content,"mediaType");
    if(pmMediaType==NULL)
      pmMediaType=getMap(resu->content,"mimeType");
    if(pmMediaType!=NULL &&
       (strstr(pmMediaType->value,"json")!=NULL || strstr(pmMediaType->value,"text")!=NULL || strstr(pmMediaType->value,"kml")!=NULL)){
      if(strstr(pmMediaType->value,"json")!=NULL){
	json_object* pjoaTmp=parseJson(conf,apcValue);
	json_object_object_add(res1,"value",pjoaTmp);
      }
      else
	json_object_object_add(res1,"value",json_object_new_string(apcValue));
      json_object_object_add(res3,"mediaType",json_object_new_string(pmMediaType->value));
    }
    else{
      char* pcaB64=base64(apcValue,len);
      json_object_object_add(res1,"value",json_object_new_string(pcaB64));
      json_object_object_add(res3,"encoding",json_object_new_string("base64"));
      if(pmMediaType!=NULL)
	json_object_object_add(res3,"mediaType",json_object_new_string(pmMediaType->value));
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param result an integer (>0 for adding the /result link)
   * @param obj the JSON object pointer to add the links to
   * @return 0
   */
  int createNextLinks(maps* conf,json_object* obj){
    json_object* res=json_object_new_array();

    map *tmpPath = getMapFromMaps (conf, "openapi", "rootUrl");
    map *sessId = getMapFromMaps (conf, "lenv", "path");

    char *Url0=(char*) malloc((strlen(tmpPath->value)+
			       strlen(sessId->value)+2)*sizeof(char));
    sprintf(Url0,"%s/%s",
	    tmpPath->value,
	    sessId->value);
    json_object_array_add(res,createALink(conf,"self","application/json",Url0));

    map* pmHtml=getMapFromMaps(conf,"openapi","partial_html_support");
    if(pmHtml!=NULL && strcmp(pmHtml->value,"true")==0){
      char *Url1=(char*) malloc((strlen(tmpPath->value)+
				 strlen(sessId->value)+7)*sizeof(char));
      sprintf(Url1,"%s/%s.html",
	      tmpPath->value,
	      sessId->value);
      json_object_array_add(res,createALink(conf,"alternate","text/html",Url1));
      free(Url1);
    }

    map* pmTmp=getMapFromMaps(conf,"lenv","serviceCntSkip");
    map* pmLimit=getMapFromMaps(conf,"lenv","serviceCntLimit");
    map* pmCnt=getMapFromMaps(conf,"lenv","serviceCnt");
    if(pmTmp!=NULL && atoi(pmTmp->value)>0){
      char* pcaTmp=(char*) malloc((strlen(Url0)+strlen(pmLimit->value)+25)*sizeof(char));
      int cnt=atoi(pmTmp->value)-atoi(pmLimit->value);
      if(cnt>=0)
	sprintf(pcaTmp,"%s?limit=%s&skip=%d",Url0,pmLimit->value,cnt);
      else
	sprintf(pcaTmp,"%s?limit=%s&skip=%d",Url0,pmLimit->value,0);
      json_object_array_add(res,createALink(conf,"prev","application/json",pcaTmp));
      free(pcaTmp);
    }
    if(getMapFromMaps(conf,"lenv","serviceCntNext")!=NULL){
      char* pcaTmp=(char*) malloc((strlen(Url0)+strlen(pmLimit->value)+strlen(pmCnt->value)+15)*sizeof(char));
      int cnt=(pmTmp!=NULL?atoi(pmTmp->value):0)+atoi(pmLimit->value);
      sprintf(pcaTmp,"%s?limit=%s&skip=%d",Url0,pmLimit->value,cnt);
      json_object_array_add(res,createALink(conf,"next","application/json",pcaTmp));
      free(pcaTmp);
    }
    json_object_object_add(obj,"links",res);
    free(Url0);
    map* pmNb=getMapFromMaps(conf,"lenv","serviceCnt");
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param pjoObj  the JSON object pointer to add the links to
   */
  void createStatusLinksFromLogs(maps* conf,json_object* pjoObj){
    map* pmTmpPath=getMapFromMaps(conf,"main","tmpPath");
    map* pmUsid=fetchUsid(conf);
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param result an integer (>0 for adding the /result link)
   * @param obj the JSON object pointer to add the links to
   * @return 0
   */
  int createStatusLinks(maps* conf,int result,json_object* obj){
    json_object* res=json_object_new_array();
    map *tmpPath = getMapFromMaps (conf, "openapi", "rootUrl");
    map *sessId = fetchUsid(conf);
    char *Url0=(char*) malloc((strlen(tmpPath->value)+
			       strlen(sessId->value)+18)*sizeof(char));
    int needResult=-1;
    char *message, *status;
    sprintf(Url0,"%s/jobs/%s",
	    tmpPath->value,
	    sessId->value);
    if(getMapFromMaps(conf,"lenv","gs_location")==NULL)
      setMapInMaps(conf,"headers","Location",Url0);
      json_object* val=json_object_new_object();
      json_object_object_add(val,"title",
			   json_object_new_string(_("Status location")));
      // The relation type should be set to monitor
      json_object_object_add(val,"rel",
			   json_object_new_string("monitor"));
      json_object_object_add(val,"type",
			   json_object_new_string("application/json"));
      json_object_object_add(val,"href",json_object_new_string(Url0));
      json_object_array_add(res,val);
      if(result>0){
        free(Url0);
      Url0=(char*) malloc((strlen(tmpPath->value)+
			  strlen(sessId->value)+
			  25)*sizeof(char));
      sprintf(Url0,"%s/jobs/%s/results",
	    tmpPath->value,
	    sessId->value);
      json_object* val1=json_object_new_object();
      json_object_object_add(val1,"title",
			    json_object_new_string(_("Result location")));
      map* pmTmp=getMapFromMaps(conf,"/jobs/{jobID}/results","rel");
      if(pmTmp!=NULL)
	      json_object_object_add(val1,"rel",
			      json_object_new_string(pmTmp->value));
      else
	      json_object_object_add(val1,"rel",
			      json_object_new_string("results"));
      json_object_object_add(val1,"type",
			   json_object_new_string("application/json"));
      json_object_object_add(val1,"href",json_object_new_string(Url0));
      json_object_array_add(res,val1);
    }
    createStatusLinksFromLogs(conf,res);
    json_object_object_add(obj,"links",res);
    free(Url0);
    return 0;
  }

  /**
   * Get the status file path
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @return a char* containing the full status file path
   */
  char* json_getStatusFilePath(maps* conf){
    map *tmpPath = getMapFromMaps (conf, "main", "tmpPath");
    map *sessId = fetchUsid(conf);
    char *tmp1=(char*) malloc((strlen(tmpPath->value)+
			       strlen(sessId->value)+14)*sizeof(char));
    sprintf(tmp1,"%s/%s_status.json",
	    tmpPath->value,
	    sessId->value);
    return tmp1;
  }

  /**
   * Get the result path
   *
   * @param conf the conf maps containing the main.cfg settings
   * @param jobId the job identifier
   * @return the full path to the result file
   */
  char* getResultPath(maps* conf,char* jobId){
    map *tmpPath = getMapFromMaps (conf, "main", "tmpPath");
    char *pcaUrl=(char*) malloc((strlen(tmpPath->value)+
				 strlen(jobId)+8)*sizeof(char));
    sprintf(pcaUrl,"%s/%s.json",tmpPath->value,jobId);
    return pcaUrl;
  }

  /**
   * Parse a json string
   *
   * @param conf the conf maps containing the main.cfg settings
   * @param myString the string containing the json content
   * @return a pointer to the created json_object
   */
  json_object* parseJson(maps* conf,char* myString){
    json_object *pajObj = NULL;
    enum json_tokener_error jerr;
    struct json_tokener* tok=json_tokener_new();
    int slen = 0;
    do {
      slen = strlen(myString);
      pajObj = json_tokener_parse_ex(tok, myString, slen);
    } while ((jerr = json_tokener_get_error(tok)) == json_tokener_continue);
    if (jerr != json_tokener_success) {
      setMapInMaps(conf,"lenv","message",json_tokener_error_desc(jerr));
      fprintf(stderr, "Error: %s\n", json_tokener_error_desc(jerr));
      json_tokener_free(tok);
      return NULL;
    }
    if (tok->char_offset < slen){
      fprintf(stderr, "Error parsing json\n");
      json_tokener_free(tok);
      return NULL;
    }
    json_tokener_free(tok);
    return pajObj;
  }

  /**
   * Read a json file
   *
   * @param conf the conf maps containing the main.cfg settings
   * @praam filePath the file path to read
   * @return a pointer to the created json_object
   */
  json_object* json_readFile(maps* conf,char* filePath){
    json_object *pajObj = NULL;
    zStatStruct zsFStatus;
    int iS=zStat(filePath, &zsFStatus);
    if(iS==0 && zsFStatus.st_size>0){
      FILE* cdat=fopen(filePath,"rb");
      if(cdat!=NULL){
	char* pcaMyString=(char*)malloc((zsFStatus.st_size+1)*sizeof(char));
	fread(pcaMyString,1,zsFStatus.st_size,cdat);
	pcaMyString[zsFStatus.st_size]=0;
	fclose(cdat);
	pajObj=parseJson(conf,pcaMyString);
	free(pcaMyString);
      }
      else
	return NULL;
    }else
      return NULL;
    return pajObj;
  }

  /**
   * Create the json object for job status
   *
   * @param conf the conf maps containing the main.cfg settings
   * @param status integer
   * @return the created json_object
   */
  json_object* createStatus(maps* conf,int status){
    int hasMessage=0;
    int needResult=0;
    int isDismissed=0;
    const char *rstatus;
    char *message;
    // Create statusInfo JSON object
    // cf. https://github.com/opengeospatial/wps-rest-binding/blob/master/core/
    //     openapi/schemas/statusInfo.yaml
    json_object* res=json_object_new_object();
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
	map* pmStatus=getMapFromMaps(conf,"lenv","message");
	if(pmStatus!=NULL){
	  setMapInMaps(conf,"lenv","gs_message",pmStatus->value);
	  message=zStrdup(pmStatus->value);
	  hasMessage=1;
	}
	map* mMap=NULL;
	if((mMap=getMapFromMaps(conf,"lenv","PercentCompleted"))!=NULL)
	  json_object_object_add(res,"progress",json_object_new_int(atoi(mMap->value)));
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
	setMapInMaps(conf,"lenv","PercentCompleted","100");
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
	map* pmTmp=getMapFromMaps(conf,"lenv","force");
	if(pmTmp==NULL || strncasecmp(pmTmp->value,"false",5)==0){
	  char* pcaTmp=json_getStatusFilePath(conf);
	  json_object* pjoStatus=json_readFile(conf,pcaTmp);
	  free(pcaTmp);
	  if(pjoStatus!=NULL){
	    json_object* pjoMessage=NULL;
	    if(json_object_object_get_ex(pjoStatus,"message",&pjoMessage)!=FALSE){
	      message=(char*)json_object_get_string(pjoMessage);
	    }
	  }
	  // TODO: Error
	}else{
	  map* mMap=getMapFromMaps(conf,"lenv","gs_message");
	  if(mMap!=NULL)
	    setMapInMaps(conf,"lenv","message",mMap->value);
	  message=produceErrorMessage(conf);
	  hasMessage=1;
	  needResult=-1;
	}
	rstatus="failed";
	break;
      }
    }
    setMapInMaps(conf,"lenv","message",message);

    map *sessId = getMapFromMaps (conf, "lenv", "usid");
    if(sessId!=NULL){
      sessId = getMapFromMaps (conf, "lenv", "gs_usid");
      if(sessId==NULL)
	sessId = getMapFromMaps (conf, "lenv", "usid");
    }else
      sessId = getMapFromMaps (conf, "lenv", "gs_usid");
    if(sessId!=NULL && isDismissed==0){
      json_object_object_add(res,"jobID",json_object_new_string(sessId->value));
#ifdef RELY_ON_DB
      for(int i=0;i<6;i++){
	char* pcaTmp=_getStatusField(conf,sessId->value,statusFieldsC[i]);
	if(pcaTmp!=NULL && strcmp(pcaTmp,"-1")!=0){
	  json_object_object_add(res,statusFields[i],json_object_new_string(pcaTmp));
	}
	if(pcaTmp!=NULL && strncmp(pcaTmp,"-1",2)==0)
	  free(pcaTmp);
      }
#else
      json_object_object_add(res,statusFields[0],json_object_new_string("process"));
      map* pmTmpPath=getMapFromMaps(conf,"main","tmpPath");
      char* pcaLenvPath=(char*)malloc((strlen(sessId->value)+strlen(pmTmpPath->value)+11)*sizeof(char));
      sprintf(pcaLenvPath,"%s/%s_lenv.cfg",pmTmpPath->value,sessId->value);
      maps *pmsLenv = (maps *) malloc (MAPS_SIZE);
      pmsLenv->content = NULL;
      pmsLenv->child = NULL;
      pmsLenv->next = NULL;
      if (conf_read (pcaLenvPath,pmsLenv) != 2){
	zStatStruct zssLenv_status,zssSid_status;
	int iS=zStat(pcaLenvPath, &zssLenv_status);
	char* pcaSidPath=(char*)malloc((strlen(sessId->value)+strlen(pmTmpPath->value)+6)*sizeof(char));
	sprintf(pcaSidPath,"%s/%s.sid",pmTmpPath->value,sessId->value);
	int iS1=zStat(pcaSidPath, &zssSid_status);
	map* pmPid=getMapFromMaps(pmsLenv,"lenv","Identifier");
	json_object_object_add(res,statusFields[1],json_object_new_string(pmPid->value));
	char *pcaTime=(char*)malloc((TIME_SIZE+1)*sizeof(char));
	size_t st_Time=strftime(pcaTime,TIME_SIZE, zDateFormat, localtime(&zssSid_status.st_mtime) );
	json_object_object_add(res,statusFields[2],json_object_new_string(pcaTime));
	json_object_object_add(res,statusFields[3],json_object_new_string(pcaTime));
	if(status==SERVICE_SUCCEEDED || status==SERVICE_FAILED){
	  char* pcaStatusPath=(char*)malloc((strlen(sessId->value)+strlen(pmTmpPath->value)+6)*sizeof(char));
	  sprintf(pcaStatusPath,"%s/%s.json",pmTmpPath->value,sessId->value);
	  zStatStruct zssStatus_status;
	  int iS2=zStat(pcaStatusPath, &zssStatus_status);
	  st_Time=strftime(pcaTime,TIME_SIZE, zDateFormat, localtime(&zssStatus_status.st_mtime) );
	  json_object_object_add(res,statusFields[4],json_object_new_string(pcaTime));
	  json_object_object_add(res,statusFields[5],json_object_new_string(pcaTime));
	}else{
	  st_Time=strftime(pcaTime,TIME_SIZE, zDateFormat, localtime(&zssLenv_status.st_mtime) );
	  json_object_object_add(res,statusFields[5],json_object_new_string(pcaTime));
	}
	freeMaps(&pmsLenv);
	free(pmsLenv);
	pmsLenv=NULL;
      }
      free(pcaLenvPath);
#endif
    }
    json_object_object_add(res,"status",json_object_new_string(rstatus));
    json_object_object_add(res,"message",json_object_new_string(message));
    if(status!=SERVICE_DISMISSED)
      createStatusLinks(conf,needResult,res);
    else{
      json_object* res1=json_object_new_array();
      map *tmpPath = getMapFromMaps (conf, "openapi", "rootUrl");
      char *Url0=(char*) malloc((strlen(tmpPath->value)+17)*sizeof(char));
      sprintf(Url0,"%s/jobs",tmpPath->value);
      json_object* val=json_object_new_object();
      json_object_object_add(val,"title",
			     json_object_new_string(_("The job list for the current process")));
      json_object_object_add(val,"rel",
			     json_object_new_string("parent"));
      json_object_object_add(val,"type",
			     json_object_new_string("application/json"));
      json_object_object_add(val,"href",json_object_new_string(Url0));
      json_object_array_add(res1,val);
      free(Url0);
      json_object_object_add(res,"links",res1);
    }
    if(hasMessage>0)
      free(message);
    return res;
  }
  
  /**
   * Create the status file 
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param status an integer (SERVICE_ACCEPTED,SERVICE_STARTED...)
   * @return an integer (0 in case of success, 1 in case of failure)
   */
  int createStatusFile(maps* conf,int status){
    json_object* res=createStatus(conf,status);
    char* tmp1=json_getStatusFilePath(conf);
    FILE* foutput1=fopen(tmp1,"w+");
    if(foutput1!=NULL){
      const char* jsonStr1=json_object_to_json_string_ext(res,JSON_C_TO_STRING_NOSLASHESCAPE);
      fprintf(foutput1,"%s",jsonStr1);
      fclose(foutput1);
    }else{
      // Failure
      setMapInMaps(conf,"lenv","message",_("Unable to store the statusInfo!"));
      free(tmp1);
      return 1;
    }
    free(tmp1);
    return 0;
  }

  /**
   * Create the status file 
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @return an integer (0 in case of success, 1 in case of failure)
   */
  int json_getStatusFile(maps* conf){
    char* tmp1=json_getStatusFilePath(conf);
    FILE* statusFile=fopen(tmp1,"rb");

    map* tmpMap=getMapFromMaps(conf,"lenv","gs_usid");
    if(tmpMap!=NULL){
      char* tmpStr=_getStatus(conf,tmpMap->value);
      if(tmpStr!=NULL && strncmp(tmpStr,"-1",2)!=0){
	char *tmpStr1=zStrdup(tmpStr);
	char *tmpStr0=zStrdup(strstr(tmpStr,"|")+1);
	free(tmpStr);
	tmpStr1[strlen(tmpStr1)-strlen(tmpStr0)-1]='\0';
	setMapInMaps(conf,"lenv","PercentCompleted",tmpStr1);
	setMapInMaps(conf,"lenv","gs_message",tmpStr0);
	free(tmpStr0);
	free(tmpStr1);
	return 0;
      }else{
	  
      }
    }
  }

  /**
   * Produce the JSON object for api info object
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param res the JSON object for the api info
   */
  void produceApiInfo(maps* conf,json_object* res,json_object* res5){
    json_object *res1=json_object_new_object();
    map* tmpMap=getMapFromMaps(conf,"provider","providerName");
    if(tmpMap!=NULL){
      json_object *res2=json_object_new_object();
      json_object_object_add(res2,"name",json_object_new_string(tmpMap->value));
      tmpMap=getMapFromMaps(conf,"provider","addressElectronicMailAddress");
      if(tmpMap!=NULL)
	json_object_object_add(res2,"email",json_object_new_string(tmpMap->value));
      tmpMap=getMapFromMaps(conf,"provider","providerSite");
      if(tmpMap!=NULL)
	json_object_object_add(res2,"url",json_object_new_string(tmpMap->value));
      json_object_object_add(res1,"contact",res2);
    }
    maps* pmsTmp=getMaps(conf,"identification");
    addDescription(pmsTmp->content,0,res1,res5);
    tmpMap=getMapFromMaps(conf,"openapi","rootUrl");
    if(tmpMap!=NULL)
      json_object_object_add(res5,"url",json_object_new_string(tmpMap->value));
    tmpMap=getMapFromMaps(conf,"identification","title");
    if(tmpMap!=NULL){
      json_object_object_add(res1,"title",json_object_new_string(_(tmpMap->value)));
      json_object_object_add(res5,"description",json_object_new_string(_(tmpMap->value)));
    }
    json_object_object_add(res1,"version",json_object_new_string(ZOO_VERSION));
    tmpMap=getMapFromMaps(conf,"identification","keywords");
    if(tmpMap!=NULL){
      char *saveptr;
      char *tmps = strtok_r (tmpMap->value, ",", &saveptr);
      json_object *res3=json_object_new_array();
      while (tmps != NULL){
	json_object_array_add(res3,json_object_new_string(tmps));
	tmps = strtok_r (NULL, ",", &saveptr);
      }
      json_object_object_add(res1,"x-keywords",res3);
    }
    maps* tmpMaps=getMaps(conf,"provider");
    if(tmpMaps!=NULL){
      json_object *res4=json_object_new_object();
      map* cItem=tmpMaps->content;
      while(cItem!=NULL){
	json_object_object_add(res4,cItem->name,json_object_new_string(cItem->value));
	cItem=cItem->next;
      }
      json_object_object_add(res1,"x-ows-servicecontact",res4);
    }

    json_object *res4=json_object_new_object();
    tmpMap=getMapFromMaps(conf,"openapi","license_name");
    if(tmpMap!=NULL){
      json_object_object_add(res4,"name",json_object_new_string(tmpMap->value));
      tmpMap=getMapFromMaps(conf,"openapi","license_url");
      if(tmpMap!=NULL){
	json_object_object_add(res4,"url",json_object_new_string(tmpMap->value));      
      }
    }
    json_object_object_add(res1,"license",res4);

    json_object_object_add(res,"info",res1);    
  }

  // addResponse(pmUseContent,cc3,vMap,tMap,"200","successful operation");
  void addResponse(const map* useContent,json_object* res,const map* pmSchema,const map* pmType,const char* code,const char* msg){
    json_object *cc=json_object_new_object();
    if(pmSchema!=NULL)
      json_object_object_add(cc,"$ref",json_object_new_string(pmSchema->value));
    if(useContent!=NULL && strncasecmp(useContent->value,"true",4)!=0){
      json_object_object_add(res,code,cc);
    }else{
      json_object *cc0=json_object_new_object();
      if(pmSchema!=NULL)
	json_object_object_add(cc0,"schema",cc);
      json_object *cc1=json_object_new_object();
      if(pmType!=NULL)
	json_object_object_add(cc1,pmType->value,cc0);
      else
	json_object_object_add(cc1,"application/json",cc0);
      json_object *cc2=json_object_new_object();
      if(strcmp(code,"204")!=0)
	json_object_object_add(cc2,"content",cc1);
      else
	json_object_put(cc1);
      json_object_object_add(cc2,"description",json_object_new_string(msg));
      json_object_object_add(res,code,cc2);
    }
  }

  void addParameter(maps* conf,const char* oName,const char* fName,const char* in,json_object* res){
    maps* tmpMaps1=getMaps(conf,oName);
    json_object *res8=json_object_new_object();
    int hasSchema=0;
    if(tmpMaps1!=NULL){
      map* tmpMap=getMap(tmpMaps1->content,"title");
      if(tmpMap!=NULL)
	json_object_object_add(res8,"x-internal-summary",json_object_new_string(_(tmpMap->value)));
      tmpMap=getMap(tmpMaps1->content,"schema");
      if(tmpMap!=NULL){
	json_object_object_add(res8,"$ref",json_object_new_string(tmpMap->value));
	hasSchema=1;
      }
      addDescription(tmpMaps1->content,0,res8,NULL);
      tmpMap=getMap(tmpMaps1->content,"example");
      if(tmpMap!=NULL)
	json_object_object_add(res8,"example",json_object_new_string(tmpMap->value));
      tmpMap=getMap(tmpMaps1->content,"required");
      if(tmpMap!=NULL){
	if(strcmp(tmpMap->value,"true")==0)
	  json_object_object_add(res8,"required",json_object_new_boolean(TRUE));
	else
	  json_object_object_add(res8,"required",json_object_new_boolean(FALSE));
      }
      else
	if(hasSchema==0)
	  json_object_object_add(res8,"required",json_object_new_boolean(FALSE));
      map* pmTmp=getMap(tmpMaps1->content,"in");
      if(pmTmp!=NULL)
	json_object_object_add(res8,"in",json_object_new_string(pmTmp->value));
      else
	if(hasSchema==0)
	  json_object_object_add(res8,"in",json_object_new_string(in));
      tmpMap=getMap(tmpMaps1->content,"name");
      if(tmpMap!=NULL)
	json_object_object_add(res8,"name",json_object_new_string(tmpMap->value));
      else
	if(hasSchema==0)
	  json_object_object_add(res8,"name",json_object_new_string(fName));
      tmpMap=getMap(tmpMaps1->content,"schema");
      if(tmpMap==NULL){
	json_object *res6=json_object_new_object();
	tmpMap=getMap(tmpMaps1->content,"type");
	char* pcaType=NULL;
	if(tmpMap!=NULL){
	  json_object_object_add(res6,"type",json_object_new_string(tmpMap->value));
	  pcaType=zStrdup(tmpMap->value);
	}
	else
	  json_object_object_add(res6,"type",json_object_new_string("string"));

	tmpMap=getMap(tmpMaps1->content,"enum");
	if(tmpMap!=NULL){
	  char *saveptr12;
	  char *tmps12 = strtok_r (tmpMap->value, ",", &saveptr12);
	  json_object *pjoEnum=json_object_new_array();
	  while(tmps12!=NULL){
	    json_object_array_add(pjoEnum,json_object_new_string(tmps12));
	    tmps12 = strtok_r (NULL, ",", &saveptr12);
	  }
	  json_object_object_add(res6,"enum",pjoEnum);
	}

	pmTmp=tmpMaps1->content;
	while(pmTmp!=NULL){
	  if(strstr(pmTmp->name,"schema_")!=NULL){
	    if(pcaType!=NULL && strncmp(pcaType,"integer",7)==0){
	      json_object_object_add(res6,strstr(pmTmp->name,"schema_")+7,json_object_new_int(atoi(pmTmp->value)));
	    }else
	      json_object_object_add(res6,strstr(pmTmp->name,"schema_")+7,json_object_new_string(pmTmp->value));
	  }
	  pmTmp=pmTmp->next;
	}
	if(pcaType!=NULL)
	  free(pcaType);
	json_object_object_add(res8,"schema",res6);
      }
    }
    json_object_object_add(res,fName,res8);    
  }
  
  /**
   * Produce the JSON object for api parameter
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param res the JSON object to populate with the parameters
   */
  void produceApiParameters(maps* conf,json_object* res){
    json_object *res9=json_object_new_object();
    maps* tmpMaps1=getMaps(conf,"{id}");
    map* tmpMap2=getMapFromMaps(conf,"openapi","parameters");
    char *saveptr12;
    char *tmps12 = strtok_r (tmpMap2->value, ",", &saveptr12);
    while(tmps12!=NULL){
      char* pcaId=(char*) malloc((strlen(tmps12)+3)*sizeof(char));
      sprintf(pcaId,"{%s}",tmps12);
      addParameter(conf,pcaId,tmps12,"path",res9);
      free(pcaId);
      tmps12 = strtok_r (NULL, ",", &saveptr12);
    }    
    tmpMap2=getMapFromMaps(conf,"openapi","header_parameters");
    if(tmpMap2!=NULL){
      char *saveptr13;
      char *tmps13 = strtok_r (tmpMap2->value, ",", &saveptr13);
      while(tmps13!=NULL){
	char* pcaId=zStrdup(tmps13);
	addParameter(conf,pcaId,pcaId,"header",res9);
	free(pcaId);
	tmps13 = strtok_r (NULL, ",", &saveptr13);
      }
    }
    json_object_object_add(res,"parameters",res9);
    maps* pmResponses=getMaps(conf,"responses");
    if(pmResponses!=NULL){
      json_object *cc=json_object_new_object();
      map* pmLen=getMap(pmResponses->content,"length");
      int iLen=atoi(pmLen->value);
      map* pmUseContent=getMapFromMaps(conf,"openapi","use_content");
      for(int i=0;i<iLen;i++){
	map* cMap=getMapArray(pmResponses->content,"code",i);
	map* vMap=getMapArray(pmResponses->content,"schema",i);
	map* tMap=getMapArray(pmResponses->content,"type",i);
	map* tMap0=getMapArray(pmResponses->content,"title",i);
	if(vMap!=NULL)
	  addResponse(pmUseContent,cc,vMap,tMap,cMap->value,(tMap0==NULL)?"successful operation":tMap0->value);
      }
      json_object_object_add(res,"responses",cc);
    }
  }

  /**
   * Add a securityScheme to the OpenAPI components/securitySchemes object
   *
   * @param pmsConf the maps containing the settings of the main.cfg file
   * @param fName the key to add to the res json object
   * @param res the json object to store the content
   */
  void addSecurityScheme(maps* pmsConf,const char* fName,json_object* res){
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
    json_object_object_add(res,fName,poJsonObject);
  }

  /**
   * Produce the JSON object for api securitySchemes
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param res the JSON object to populate with the parameters
   */
  void produceApiSecuritySchemes(maps* conf,json_object* res){
    map* pmTmp=getMapFromMaps(conf,"osecurity","name");
    if(pmTmp!=NULL){
      json_object *res9=json_object_new_object();
      char *saveptr12;
      char *tmps12 = strtok_r (pmTmp->value, ",", &saveptr12);
      while(tmps12!=NULL){
	addSecurityScheme(conf,tmps12,res9);
	tmps12 = strtok_r (NULL, ",", &saveptr12);
      }
      json_object_object_add(res,"securitySchemes",res9);
    }
  }

  /**
   * Produce the JSON object for api schemas
   *
   * @param conf the maps containing the settings of the main.cfg file
   * @param res the JSON object to populate with the parameters
   */
  void produceApiSchemas(maps* conf,json_object* res){
    map* pmTmp=getMapFromMaps(conf,"schemas","length");
    if(pmTmp!=NULL){
      json_object *res9=json_object_new_object();
      maps* pmsTmp=getMaps(conf,"schemas");
      int iLimit=atoi(pmTmp->value);
      for(int iCnt=0;iCnt<iLimit;iCnt++){
        map* pmName=getMapArray(pmsTmp->content,"name",iCnt);
        map* pmValue=getMapArray(pmsTmp->content,"value",iCnt);
        if(pmName!=NULL && pmValue!=NULL){
          json_object *res10=json_object_new_object();
          json_object_object_add(res10,"$ref",json_object_new_string(pmValue->value));
          json_object_object_add(res9,pmName->value,res10);
        }
      }
      json_object_object_add(res,"schemas",res9);
    }
  }

  void produceApiComponents(maps*conf,json_object* res){
    json_object* res1=json_object_new_object();
    produceApiParameters(conf,res1);
    produceApiSchemas(conf,res1);
    produceApiSecuritySchemes(conf,res1);
    json_object_object_add(res,"components",res1);
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
      char *pcaOperationId =
	(char*) malloc((strlen(pmsContent->name)+
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
   * @param conf the maps containing the settings of the main.cfg file
   * @param res the JSON object to populate
   */
  void produceApi(maps* conf,json_object* res){
    // TODO: add 401 Gone for dismiss request
    json_object *res9=json_object_new_object();
    json_object *res10=json_object_new_object();
    maps* tmpMaps1=getMaps(conf,"{id}");
    produceApiComponents(conf,res);
    json_object *res4=json_object_new_object();
    map* pmTmp=getMapFromMaps(conf,"/api","type");
    if(pmTmp!=NULL){
      char* pcaTmp=(char*)malloc((15+strlen(pmTmp->value))*sizeof(char));
      sprintf(pcaTmp,"%s;charset=UTF-8",pmTmp->value);
      setMapInMaps(conf,"headers","Content-Type",pcaTmp);
      free(pcaTmp);
    }
    else
      setMapInMaps(conf,"headers","Content-Type","application/vnd.oai.openapi+json;version=3.0;charset=UTF-8");

    produceApiInfo(conf,res,res4);
    map* tmpMap=getMapFromMaps(conf,"provider","providerName");

    tmpMap=getMapFromMaps(conf,"openapi","version");
    if(tmpMap!=NULL)
      json_object_object_add(res,"openapi",json_object_new_string(tmpMap->value));
    else
      json_object_object_add(res,"openapi",json_object_new_string("3.0.2"));

    tmpMap=getMapFromMaps(conf,"openapi","tags");
    maps* pmsTmp=getMaps(conf,"tags");
    if(tmpMap!=NULL){
      json_object *res5=json_object_new_array();
      char *saveptr;
      char *tmps = strtok_r (tmpMap->value, ",", &saveptr);
      int iCnt=0;
      while(tmps!=NULL){
        json_object *res6=json_object_new_object();
        json_object_object_add(res6,"name",json_object_new_string(tmps));
        if(pmsTmp!=NULL){
          map* pmDesc=getMapArray(pmsTmp->content,"value",iCnt);
          if(pmDesc!=NULL)
            json_object_object_add(res6,"description",json_object_new_string(pmDesc->value));
          else
          json_object_object_add(res6,"description",json_object_new_string(tmps));
        }else
          json_object_object_add(res6,"description",json_object_new_string(tmps));
        json_object_array_add(res5,res6);
        tmps = strtok_r (NULL, ",", &saveptr);
        iCnt++;
      }
      json_object_object_add(res,"tags",res5);
    }

    tmpMap=getMapFromMaps(conf,"openapi","paths");
    if(tmpMap!=NULL){
      json_object *res5=json_object_new_object();
      char *saveptr;
      char *tmps = strtok_r (tmpMap->value, ",", &saveptr);
      int cnt=0;
      maps* tmpMaps;
      json_object *paths=json_object_new_object();
      while (tmps != NULL){
        tmpMaps=getMaps(conf,tmps+1);
        json_object *method=json_object_new_object();
        map* pmName=getMap(tmpMaps->content,"pname");
        if(tmpMaps!=NULL){
          if(getMap(tmpMaps->content,"length")==NULL)
            addToMap(tmpMaps->content,"length","1");
          map* len=getMap(tmpMaps->content,"length");
          for(int i=0;i<atoi(len->value);i++){
            json_object *methodc=json_object_new_object();
            map* cMap=getMapArray(tmpMaps->content,"method",i);
            map* vMap=getMapArray(tmpMaps->content,"title",i);
            if(vMap!=NULL)
              json_object_object_add(methodc,"summary",json_object_new_string(_(vMap->value)));
            vMap=getMapArray(tmpMaps->content,"secured",i);
            if(vMap!=NULL){
              json_object *poSecurity=json_object_new_array();
              json_object *poScopes=json_object_new_array();
              map* pmSMap=getMapArray(tmpMaps->content,"scopes",i);
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
            addDescription(tmpMaps->content,i,methodc,NULL);
            vMap=getMapArray(tmpMaps->content,"tags",i);
            if(vMap!=NULL){
              json_object *cc=json_object_new_array();
              json_object_array_add(cc,json_object_new_string(vMap->value));
              json_object_object_add(methodc,"tags",cc);
              if(pmName==NULL)
                pmName=getMapArray(tmpMaps->content,"operationId",i);
              if(pmName!=NULL){
                char* pcaOperationId=(char*)malloc((strlen(vMap->value)+strlen(pmName->value)+1)*sizeof(char));
                sprintf(pcaOperationId,"%s%s",vMap->value,pmName->value);
                json_object_object_add(methodc,"operationId",json_object_new_string(pcaOperationId));
                free(pcaOperationId);
              }
              else
                produceOperationId(conf,tmpMaps,i,methodc);
            }
            json_object *responses=json_object_new_object();
            json_object *cc3=json_object_new_object();
            map* pmUseContent=getMapFromMaps(conf,"openapi","use_content");
            map* pmCode=getMapArray(tmpMaps->content,"code",i);
            vMap=getMapArray(tmpMaps->content,"schema",i);
            if(vMap!=NULL){
              map* tMap=getMapArray(tmpMaps->content,"type",i);
              if(pmCode!=NULL)
                addResponse(pmUseContent,cc3,vMap,tMap,pmCode->value,"successful operation");
              else
                addResponse(pmUseContent,cc3,vMap,tMap,"200","successful operation");
              vMap=getMapArray(tmpMaps->content,"eschema",i);
              if(pmCode==NULL && vMap!=NULL && cMap!=NULL && strncasecmp(cMap->value,"post",4)==0)
                addResponse(pmUseContent,cc3,vMap,tMap,"201","successful operation");
            }else{
              map* tMap=getMapFromMaps(conf,tmps,"type");
              map* pMap=createMap("ok","true");
              if(pmCode!=NULL)
                addResponse(pMap,cc3,vMap,tMap,pmCode->value,"successful operation");
              else
                addResponse(pMap,cc3,vMap,tMap,"200","successful operation");
              if(pmCode==NULL && cMap!=NULL && strncasecmp(cMap->value,"post",4)==0)
                addResponse(pmUseContent,cc3,vMap,tMap,"201","successful operation");
              freeMap(&pMap);
              free(pMap);
            }
            vMap=getMapArray(tmpMaps->content,"ecode",i);
            if(vMap!=NULL){
              char *saveptr0;
              char *tmps1 = strtok_r (vMap->value, ",", &saveptr0);
              while(tmps1!=NULL){
                char* tmpStr=(char*)malloc((strlen(tmps1)+24)*sizeof(char));
                sprintf(tmpStr,"#/components/responses/%s",tmps1);
                vMap=createMap("ok",tmpStr);
                map* pMap=createMap("ok","false");
                //TODO: fix successful operation with correct value
                addResponse(pMap,cc3,vMap,NULL,tmps1,"successful operation");
                tmps1 = strtok_r (NULL, ",", &saveptr0);
              }
            }else{
              if(strstr(tmps,"{id}")!=NULL){
                vMap=getMapFromMaps(conf,"exception","schema");
                map* tMap=getMapFromMaps(conf,"exception","type");
                if(vMap!=NULL)
                  addResponse(pmUseContent,cc3,vMap,tMap,"404",
                    (strstr(tmps,"{jobID}")==NULL)?"The process with id {id} does not exist.":"The process with id {id} or job with id {jobID} does not exist.");
              }
            }
            json_object_object_add(methodc,"responses",cc3);
            vMap=getMapArray(tmpMaps->content,"parameters",i);
            if(vMap!=NULL){
              char *saveptr0;
              char *tmps1 = strtok_r (vMap->value, ",", &saveptr0);
              json_object *cc2=json_object_new_array();
              int cnt=0;
              while(tmps1!=NULL){
                char* tmpStr=(char*)malloc((strlen(tmps1)+2)*sizeof(char));
                sprintf(tmpStr,"#%s",tmps1);
                json_object *cc1=json_object_new_object();
                json_object_object_add(cc1,"$ref",json_object_new_string(tmpStr));
                json_object_array_add(cc2,cc1);
                free(tmpStr);
                cnt++;
                tmps1 = strtok_r (NULL, ",", &saveptr0);
              }
              json_object_object_add(methodc,"parameters",cc2);
            }
            if(/*i==1 && */cMap!=NULL &&
              (strncasecmp(cMap->value,"post",4)==0
              ||
              strncasecmp(cMap->value,"put",3)==0)){
                int iLen=1;
                map* pmRequestBodyLength=getMapArray(tmpMaps->content,"requestBody_length",i);
                if(pmRequestBodyLength!=NULL)
                  iLen=atoi(pmRequestBodyLength->value);
                  char* pcaKey0=getMapArrayKey(tmpMaps->content,"requestBody",i);
                  int iCnt=0;
                  int iCounter=0;
                  json_object *cc1=NULL;
                  json_object *cc2=NULL;
                  for(;iCounter<iLen;iCounter++){
                    map* pmRequestBodyName=getMapArray(tmpMaps->content,pcaKey0,iCounter);
                    maps* tmpMaps1=getMaps(conf,(pmRequestBodyName==NULL?"requestBody":pmRequestBodyName->value));
                    if(tmpMaps1!=NULL){
                      vMap=getMap(tmpMaps1->content,"schema");
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
                        char* pcaKey1=getMapArrayKey(tmpMaps->content,"examples",i);
                        char actmp0[10];
                        sprintf(actmp0,"%d",iCounter);
                        map* pmExample=getMapArray(tmpMaps->content,pcaKey1,iCounter);
                        if(pmExample==NULL){
                          free(pcaKey1);
                          pcaKey1=getMapArrayKey(tmpMaps->content,"examples_ref",i);
                          pmExample=getMapArray(tmpMaps->content,pcaKey1,iCounter);
                          iIsRef=1;
                        }
                        if(pmExample!=NULL) {
                          char* saveptr;
                          map* pmExamplesPath=getMapFromMaps(conf,"openapi","examplesPath");
                          json_object* prop5=json_object_new_object();
                          char *tmps = strtok_r (pmExample->value, ",", &saveptr);
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
                            char actmp[10];
                            sprintf(actmp,"%d",iCnt);
                            char* pcaTmp;
                            char* pcaKey2=getMapArrayKey(tmpMaps->content,"examples_summary",i);
                            map* pmSummary=getMapArray(tmpMaps->content,pcaKey2,iCnt);
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
                                json_object* pjoValue=json_readFile(conf,pcaExempleFile);
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
                                    fread(pcaFcontent,zssStatus.st_size,1,f);
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
                            tmps = strtok_r (NULL, ",", &saveptr);
                            iCnt++;
                          }
                          json_object_object_add(cc0,"examples",prop5);
                        }
                        free(pcaKey1);
                        if(iCounter==0)
                          cc1=json_object_new_object();
                        map* tmpMap3=getMap(tmpMaps1->content,"type");
                        if(tmpMap3!=NULL)
                          json_object_object_add(cc1,tmpMap3->value,cc0);
                        else
                          json_object_object_add(cc1,"application/json",cc0);
                        if(iCounter==0)
                          cc2=json_object_new_object();
                        if(iCounter+1==iLen){
                          json_object_object_add(cc2,"content",cc1);
                          addDescription(tmpMaps1->content,0,cc2,NULL);
                          json_object_object_add(cc2,"required",json_object_new_boolean(true));
                          json_object_object_add(methodc,"requestBody",cc2);
                        }
                      }
                    }
                  }
                  free(pcaKey0);
                  map* pmCallbacksReference=getMapArray(tmpMaps->content,"callbacksReference",i);
                  if(pmCallbacksReference!=NULL){
                    tmpMaps1=getMaps(conf,pmCallbacksReference->value);
                    if(tmpMaps1!=NULL){
                      map* pmTmp2=getMap(tmpMaps1->content,"length");
                      int iLen=atoi(pmTmp2->value);
                      json_object *pajRes=json_object_new_object();
                      for(int i/*iCallbackCnt*/=0;i<iLen;i/*iCallbackCnt*/++){
                        map* pmState=getMapArray(tmpMaps1->content,"state",i/*iCallbackCnt*/);
                        map* pmUri=getMapArray(tmpMaps1->content,"uri",i/*iCallbackCnt*/);
                        map* pmSchema=getMapArray(tmpMaps1->content,"schema",i/*iCallbackCnt*/);
                        map* pmType=getMapArray(tmpMaps1->content,"type",i/*iCallbackCnt*/);
                        map* pmTitle=getMapArray(tmpMaps1->content,"title",i/*iCallbackCnt*/);
                        json_object *pajSchema=json_object_new_object();
                        if(pmSchema!=NULL)
                          json_object_object_add(pajSchema,"$ref",json_object_new_string(pmSchema->value));
                        json_object *pajType=json_object_new_object();
                        json_object_object_add(pajType,"schema",pajSchema);
                        json_object *pajContent=json_object_new_object();
                        if(pmType!=NULL)
                          json_object_object_add(pajContent,pmType->value,pajType);
                        else
                          json_object_object_add(pajContent,"application/json",pajType);
                        json_object *pajRBody=json_object_new_object();
                        json_object_object_add(pajRBody,"content",pajContent);

                        json_object *pajDescription=json_object_new_object();
                        json_object *pajPost=json_object_new_object();
                        if(pmTitle!=NULL){
                          json_object_object_add(pajDescription,"description",json_object_new_string(_(pmTitle->value)));
                          json_object_object_add(pajPost,"summary",json_object_new_string(_(pmTitle->value)));
                        }
                        json_object *pajResponse=json_object_new_object();
                        json_object_object_add(pajResponse,"200",pajDescription);

                        json_object_object_add(pajPost,"requestBody",pajRBody);
                        json_object_object_add(pajPost,"responses",pajResponse);
                        if(pmName==NULL)
                          pmName=getMapArray(tmpMaps->content,"operationId",i/*iCallbackCnt*/);
                        if(pmName!=NULL){
                          char* pcaOperationId=(char*)malloc((strlen(pmState->value)+strlen(pmName->value)+1)*sizeof(char));
                          sprintf(pcaOperationId,"%s%s",pmState->value,pmName->value);
                          json_object_object_add(pajPost,"operationId",json_object_new_string(pcaOperationId));
                          free(pcaOperationId);
                        }
                        else
                          produceOperationId(conf,tmpMaps1,i/*iCallbackCnt*/,pajPost);

                        json_object *pajMethod=json_object_new_object();
                        json_object_object_add(pajMethod,"post",pajPost);

                        char* pcaUri=(char*) malloc((strlen(pmUri->value)+29)*sizeof(char));
                        sprintf(pcaUri,"{$request.body#/subscriber/%s}",pmUri->value);

                        json_object *pajFinal=json_object_new_object();
                        json_object_object_add(pajFinal,pcaUri,pajMethod);
                        json_object_object_add(pajRes,pmState->value,pajFinal);

                      }
                      json_object_object_add(methodc,"callbacks",pajRes);
                    }
                  }
              }
              map* mMap=getMapArray(tmpMaps->content,"method",i);
              if(mMap!=NULL)
                json_object_object_add(method,mMap->value,methodc);
              else
                json_object_object_add(method,"get",methodc);
          }

          tmpMap=getMapFromMaps(conf,"openapi","version");
          if(tmpMap!=NULL)
            json_object_object_add(res,"openapi",json_object_new_string(tmpMap->value));
          else
            json_object_object_add(res,"openapi",json_object_new_string("3.0.2"));
          if(strstr(tmps,"/root")!=NULL)
            json_object_object_add(paths,"/",method);
          else
            json_object_object_add(paths,tmps,method);
        }

        tmps = strtok_r (NULL, ",", &saveptr);
        cnt++;
      }
      json_object_object_add(res,"paths",paths);
    }
      
    tmpMap=getMapFromMaps(conf,"openapi","links");
    if(tmpMap!=NULL){
	
    }
    
    json_object *res3=json_object_new_array();
    json_object_array_add(res3,res4);
    json_object_object_add(res,"servers",res3);
  }

#ifdef DRU_ENABLED
  /**
   * Print exception report in case Deploy or Undeploy failed to execute
   *
   * @param conf the main configuration maps pointer
   */
  void handleDRUError(maps* conf){
    map* pmError=getMapFromMaps(conf,"lenv","jsonStr");
    setMapInMaps(conf,"lenv","hasPrinted","false");
    setMapInMaps(conf,"lenv","no-headers","false");
    setMapInMaps(conf,"headers","Status","500 Internal Server Error");
    setMapInMaps(conf,"lenv","status_code","500 Internal Server Error");
    if(pmError!=NULL){
      printHeaders(conf);
      printf("\r\n");
      printf(pmError->value);
      printf("\n");
    }else{
      pmError=createMap("code","InternalError");
      map* pmMessage=getMapFromMaps(conf,"lenv","message");
      map* pmORequestMethod=getMapFromMaps(conf,"lenv","orequest_method");
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
      printExceptionReportResponseJ(&conf,pmError);
      freeMap(&pmError);
      free(pmError);
    }
    setMapInMaps(conf,"lenv","no-headers","true");
    setMapInMaps(conf,"lenv","hasPrinted","true");
  }

  /**
   * Convert a CWL to OGC Application Package format
   *
   * @param conf the main configuration maps pointer
   * @param request_inputs map containing the request inputs
   * @param pjRequest the json_object corresponding to the initial payload
   */
  json_object* convertCwlToOGCAppPkg(maps* conf,map* request_inputs){
    json_object* pjRes=NULL;
    map* pmJRequest=getMap(request_inputs,"jrequest");
    map* pmCgiContentType=getMapFromMaps(conf,"request","Content-Type");
    if(pmJRequest!=NULL && pmCgiContentType!=NULL && strstr(pmCgiContentType->value,"cwl")!=NULL){
      pjRes=json_object_new_object();
      json_object *pjExecutionUnit=json_object_new_object();
      json_object *pjExecutionUnitFormat=json_object_new_object();
      json_object_object_add(pjExecutionUnitFormat,"mediaType",json_object_new_string("application/cwl"));
      json_object_object_add(pjExecutionUnit,"value",json_object_new_string(pmJRequest->value));
      json_object_object_add(pjExecutionUnit,"format",pjExecutionUnitFormat);
      json_object* pjExecutionUnits=json_object_new_array();
      json_object_array_add(pjExecutionUnits,pjExecutionUnit);
      json_object_object_add(pjRes,"executionUnit",pjExecutionUnits);
    }
    return pjRes;
  }

  /**
   * Convert an OGC Application Package into a standard execute payload
   *
   * @param conf the main configuration maps pointer
   * @param request_inputs map containing the request inputs
   * @param pjRequest the json_object corresponding to the initial payload
   * @return 0 in case of success.
   */
  int convertOGCAppPkgToExecute(maps* conf,map* request_inputs,json_object** pjRequest){
    json_object* pjRes=json_object_new_object();
    json_object* pjProcessDescription=NULL;
    json_object* pjExecutionUnit=NULL;
    if(json_object_object_get_ex(*pjRequest, "processDescription", &pjProcessDescription)==FALSE){
      if(json_object_object_get_ex(*pjRequest, "executionUnit", &pjExecutionUnit)!=FALSE){
	json_object* pjInputs=json_object_new_object();
	json_object_object_add(pjInputs,"applicationPackage",json_object_get(pjExecutionUnit));
	json_object_object_add(pjRes,"inputs",pjInputs);
	json_object_put(*pjRequest);
	*pjRequest=json_object_get(pjRes);
	const char* jsonStr=json_object_to_json_string_ext(*pjRequest,JSON_C_TO_STRING_NOSLASHESCAPE);
	setMapInMaps(conf,"lenv","jrequest",jsonStr);
      }
      json_object_put(pjRes);
    }else{
      json_object* pjInputs=json_object_new_object();
      json_object_object_add(pjInputs,"applicationPackage",json_object_get(*pjRequest));
      json_object_object_add(pjRes,"inputs",pjInputs);
      json_object_put(*pjRequest);
      *pjRequest=json_object_get(pjRes);
      const char* jsonStr=json_object_to_json_string_ext(*pjRequest,JSON_C_TO_STRING_NOSLASHESCAPE);
      setMapInMaps(conf,"lenv","jrequest",jsonStr);
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
   * Print exception report in case Deploy or Undeploy failed to execute
   *
   * @param conf the main configuration maps pointer
   */
  int parseProcessDescription(maps* conf){
    fprintf(stderr,"Should parse process description %s %d \n",__FILE__,__LINE__);
    fflush(stderr);
    return 0;
  }
#endif // DRU_ENABLED

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
    if(pmsFilterIn!=NULL){
      int len=1;
      map* pmLength=getMap(pmsFilterIn->content,"length");
      if(pmLength!=NULL)
	len=atoi(pmLength->value);
      for(int i=0;i<len;i++){
	map* pmFilterService=getMapArray(pmsFilterIn->content,"service",i);
	if(pmFilterService!=NULL && strstr(pmFilterService->value,pcService)!=NULL)
	  return true;
      }
    }
    maps* pmsFilterOut=getMaps(pmsConf,"filter_out");
    if(pmsFilterOut!=NULL){
      int len=1;
      map* pmLength=getMap(pmsFilterOut->content,"length");
      if(pmLength!=NULL)
	len=atoi(pmLength->value);
      for(int i=0;i<len;i++){
	map* pmFilterService=getMapArray(pmsFilterOut->content,"service",i);
	if(pmFilterService!=NULL && strstr(pmFilterService->value,pcService)!=NULL)
	  return true;
      }
    }
    return false;
  }

#ifdef __cplusplus
}
#endif
