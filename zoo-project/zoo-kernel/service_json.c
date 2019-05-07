/*
 * Author : Gérald FENOY
 *
 *  Copyright 2017 GeoLabs SARL. All rights reserved.
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

#ifdef __cplusplus
extern "C" {
#endif

  json_object* mapToJson(map* myMap){
    json_object *res=json_object_new_object();
    map* cursor=myMap;
    map* sizeMap=getMap(myMap,"size");
    map* length=getMap(myMap,"length");
    while(cursor!=NULL){
      json_object *val=NULL;
      if(length==NULL && sizeMap==NULL)
	val=json_object_new_string(cursor->value);
      else{
	if(length==NULL && sizeMap!=NULL){
	  if(strncasecmp(cursor->name,"value",5)==0)
	    val=json_object_new_string_len(cursor->value,atoi(sizeMap->value));
	  else
	    val=json_object_new_string(cursor->value);
	}
	else{
	  // In other case we should consider array with potential sizes
	  int limit=atoi(length->value);
	  int i=0;
	  val=json_object_new_array();
	  for(i=0;i<limit;i++){
	    map* lsizeMap=getMapArray(myMap,"size",i);
	    if(lsizeMap!=NULL && strncasecmp(cursor->name,"value",5)==0){
	      json_object_array_add(val,json_object_new_string_len(cursor->value,atoi(sizeMap->value)));
	    }else{
	      json_object_array_add(val,json_object_new_string(cursor->value));
	    }
	  }
	}
      }
      if(val!=NULL)
	json_object_object_add(res,cursor->name,val);
      cursor=cursor->next;
    }
    return res;
  }

  json_object* mapsToJson(maps* myMap){
    json_object *res=json_object_new_object();
    maps* cursor=myMap;
    while(cursor!=NULL){
      json_object *obj=NULL;
      if(cursor->content!=NULL){
	//json_object *content=NULL;
	obj=mapToJson(cursor->content);
	//json_object_object_add(obj,"content",content);
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
	fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
	fflush(stderr);
      }
      
      dumpElements(cur->child);
      json_object_object_add(cres,"child",elementsToJson(cur->child));
      fprintf(stderr,"%s %d\n",__FILE__,__LINE__);
      fflush(stderr);

      json_object_object_add(res,cur->name,cres);
      cur=cur->next;
    }
    return res;
  }
  
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
  
  
#ifdef __cplusplus
}
#endif
