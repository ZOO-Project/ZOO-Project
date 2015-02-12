/*
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

#ifdef WIN32
  #define NO_FCGI_DEFINES
#endif
 
#include "service_internal_php.h"

#include <sapi/embed/php_embed.h>
#include <zend_stream.h>

#ifdef ZTS
void ***tsrm_ls;
#endif

zval *php_Array_from_maps(maps* t);
zval*  php_Array_from_map(map*);
maps* php_maps_from_Array(HashTable* t);
map* php_map_from_HasTable(HashTable* t);

/**
 * Load a PHP script then run the function corresponding to the service by
 * passing the conf, inputs and outputs parameters by reference.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 */
int zoo_php_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){	
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  char ntmp[1024];
  getcwd(ntmp,1024);
  map* tmp=getMap(request,"metapath");

  int res=SERVICE_FAILED;

  tmp=getMap(s->content,"serviceProvider");
  zend_file_handle iscript;
  iscript.type=ZEND_HANDLE_FP;
  iscript.filename=tmp->value;
  iscript.opened_path=NULL;
  //iscript.free_filname=0;
  if(!(iscript.handle.fp=fopen(iscript.filename,"rb"))){
    char tmp1[1024];
    sprintf(tmp1,"Unable to load PHP file %s",tmp->value);
    map* err=createMap("text",tmp1);
    addToMap(err,"code","NoApplicableCode");
    printExceptionReportResponse(m,err);
    exit(-1);
  }

  php_embed_init(0,NULL,&tsrm_ls);
   
  zend_try {

    php_execute_script(&iscript TSRMLS_CC);

    zval *iargs[3];
    zval iret, ifunc,ifile;
      
    ZVAL_STRING(&ifunc, s->name, 0);
    iargs[0] = php_Array_from_maps(*main_conf);
    iargs[1] = php_Array_from_maps(*real_inputs);
    iargs[2] = php_Array_from_maps(*real_outputs);
    
    call_user_function(EG(function_table), NULL, &ifunc, &iret, 3, iargs TSRMLS_CC);

    HashTable* t=HASH_OF(iargs[2]);
    *real_outputs=php_maps_from_Array(t);
    
    char tmp1[1024];

    res=SERVICE_SUCCEEDED;

  } zend_catch { 
    map* err=createMap("text","Unable to process.");
    addToMap(err,"code","NoApplicableCode");
    printExceptionReportResponse(m,err);
    exit(-1);
  } zend_end_try();

  php_embed_shutdown(TSRMLS_C);

  return res;
}

/**
 * Convert a maps to a php Array
 *
 * @param t the maps to convert
 * @return the php Array
 */
zval *php_Array_from_maps(maps* t){
  zval *mapArray;
  zval *mapArrayTmp;
  maps* tmp=t;
  int tres=0;
  MAKE_STD_ZVAL(mapArray);
  tres=array_init(mapArray);
  while(tmp!=NULL){
    add_assoc_zval(mapArray,tmp->name,php_Array_from_map(tmp->content));
    tmp=tmp->next;
  }
  return mapArray;
}

/**
 * Convert a map to a php Array
 *
 * @param t the map to convert
 * @return the php Array
 */
zval *php_Array_from_map(map* t){
  zval *mapArray;
  zval *mapArrayTmp;
  map* tmp=t;
  int tres=0;
  MAKE_STD_ZVAL(mapArray);
  tres=array_init(mapArray);
  while(tmp!=NULL){
    map* sMap=getMapArray(tmp,"size",0);    
	if(strncmp(tmp->name,"value",5)==0 && sMap!=NULL && tmp->value != NULL){
      tres=add_assoc_stringl(mapArray,tmp->name,tmp->value,atoi(sMap->value),1);
	} 
	else if (tmp->value != NULL) {
      tres=add_assoc_string(mapArray,tmp->name,tmp->value,1);
	}
    tmp=tmp->next;
  }
  return mapArray;
}

/**
 * Convert a php Array to a maps
 *
 * @param t the php Array to convert
 * @return the created maps
 */
maps* php_maps_from_Array(HashTable *t){
  maps* final_res=NULL;
  maps* cursor=final_res;
  char key[1024];
  for(zend_hash_internal_pointer_reset(t); 
      zend_hash_has_more_elements(t) == SUCCESS; 
      zend_hash_move_forward(t)) { 
    char *key; 
    uint keylen; 
    ulong idx; 
    int type; 
    zval **ppzval, tmpcopy; 
    type = zend_hash_get_current_key_ex(t, &key, &keylen, &idx, 0, NULL); 
    if (zend_hash_get_current_data(t, (void**)&ppzval) == FAILURE) { 
      /**
       * Should never actually fail since the key is known to exist.
       */
      continue; 
    }
    /**
     * Duplicate the zval so that * the orignal’s contents are not destroyed
     */
    tmpcopy = **ppzval;
#ifdef DEBUG
    fprintf(stderr,"key : %s\n",key);
#endif
    zval_copy_ctor(&tmpcopy); 
#ifdef DEBUG
    fprintf(stderr,"key : %s\n",key);
#endif
    /**
     * Reset refcount & Convert
     */
    INIT_PZVAL(&tmpcopy); 
    //convert_to_string(&tmpcopy); 
    if (type == HASH_KEY_IS_STRING) { 
      /**
       * String Key / Associative
       */
      cursor=(maps*)malloc(MAPS_SIZE);
      cursor->name=strdup(key);
    }
#ifdef DEBUG	
    fprintf(stderr,"key : %s\n",key);
#endif	
    HashTable* t=HASH_OF(*ppzval);
#ifdef DEBUG
    fprintf(stderr,"key : %s\n",key);
#endif
    cursor->content=php_map_from_HasTable(t);
    cursor->next=NULL;
    if(final_res==NULL)
      final_res=cursor;
    else
      addMapsToMaps(&final_res,cursor);
#ifdef DEBUG
    fprintf(stderr,"key : %s\n",key);
#endif
    /**
     * Toss out old copy
     */
    zval_dtor(&tmpcopy);
  }
  return final_res;
}

/**
 * Convert a php Array to a map
 *
 * @param t the php Array to convert
 * @return the created map
 */
map* php_map_from_HasTable(HashTable* t){
#ifdef DEBUG
  fprintf(stderr,"mapsFromPHPArray start\n");
#endif
  map* final_res=(map*)malloc(MAP_SIZE);
  final_res=NULL;
  char key[1024];
  for(zend_hash_internal_pointer_reset(t);
      zend_hash_has_more_elements(t) == SUCCESS;
      zend_hash_move_forward(t)) {
    char *key;
    uint keylen;
    ulong idx;
    int type;
    int len;
    zval **ppzval, tmpcopy;
    type = zend_hash_get_current_key_ex(t, &key, &keylen, &idx, 0, NULL); 
    if (zend_hash_get_current_data(t, (void**)&ppzval) == FAILURE) { 
      /* Should never actually fail * since the key is known to exist. */ 
      continue; 
    }
    /**
     * Duplicate the zval so that * the orignal’s contents are not destroyed 
     */ 
    tmpcopy = **ppzval; 
    zval_copy_ctor(&tmpcopy); 
    /**
     * Reset refcount & Convert 
     */ 
    INIT_PZVAL(&tmpcopy); 
    convert_to_string(&tmpcopy);
    if(strncmp(key,"value",5)==0){
      len=Z_STRLEN_P(&tmpcopy);
      addToMapWithSize(final_res,key,Z_STRVAL_P(&tmpcopy),len);
    }
    else{
      if(final_res==NULL){
#ifdef DEBUG
	fprintf(stderr,"%s => %s\n",key,Z_STRVAL(tmpcopy));
#endif
	final_res=createMap(key,Z_STRVAL(tmpcopy));
      }
      else{
#ifdef DEBUG
	fprintf(stderr,"%s => %s\n",key,Z_STRVAL(tmpcopy));
#endif
	addToMap(final_res,key,Z_STRVAL(tmpcopy));
      }
    }
    /* Toss out old copy */ 
    zval_dtor(&tmpcopy); 
  }
  return final_res;
}














