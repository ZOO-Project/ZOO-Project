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

#ifndef ZEND_DEBUG
  #define ZEND_DEBUG 0
#endif  

#include <sapi/embed/php_embed.h>
#include <zend_stream.h>

#include "service_internal_php.h"
#include "response_print.h"

zval *php_Array_from_maps(maps* t);
zval*  php_Array_from_map(map*);
maps* php_maps_from_Array(HashTable* t);
map* php_map_from_HasTable(HashTable* t);

#ifdef ZTS
void ***tsrm_ls;
#endif

ZEND_BEGIN_MODULE_GLOBALS(zoo)
long _SERVICE_SUCCEEDED;
long _SERVICE_FAILED;
ZEND_END_MODULE_GLOBALS(zoo)

#ifdef ZTS
#define ZOO_G(v) TSRMG(zoo_globals_id, zend_zoo_globals *, v)
#else
#define ZOO_G(v) (zoo_globals.v)
#endif

#define PHP_ZOO_VERSION "1.0"
#define PHP_ZOO_EXTNAME "ZOO"

PHP_MINIT_FUNCTION(zoo);
PHP_MSHUTDOWN_FUNCTION(zoo);
PHP_RINIT_FUNCTION(zoo);

PHP_FUNCTION(zoo_Translate);
PHP_FUNCTION(zoo_UpdateStatus);
PHP_FUNCTION(zoo_SERVICE_SUCCEEDED);
PHP_FUNCTION(zoo_SERVICE_FAILED);

extern zend_module_entry zoo_module_entry;
#define phpext_zoo_ptr &zoo_entry

ZEND_DECLARE_MODULE_GLOBALS(zoo)

static zend_function_entry zoo_functions[] = {
  PHP_FE(zoo_Translate, NULL)
  PHP_FE(zoo_UpdateStatus, NULL)
  PHP_FE(zoo_SERVICE_SUCCEEDED, NULL)
  PHP_FE(zoo_SERVICE_FAILED, NULL)
  {NULL, NULL, NULL}
};

zend_module_entry zoo_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_ZOO_EXTNAME,
    zoo_functions,
    PHP_MINIT(zoo),
    PHP_MSHUTDOWN(zoo),
    PHP_RINIT(zoo),
    NULL,
    NULL,
#if ZEND_MODULE_API_NO >= 20010901
    PHP_ZOO_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(zoo)

PHP_INI_BEGIN()
PHP_INI_END()

static void
php_zoo_init_globals(zend_zoo_globals *zoo_globals)
{
  zoo_globals->_SERVICE_SUCCEEDED=3;
  zoo_globals->_SERVICE_FAILED=4;
}

PHP_RINIT_FUNCTION(zoo)
{
  return SUCCESS;
}

PHP_MINIT_FUNCTION(zoo)
{
  ZEND_INIT_MODULE_GLOBALS(zoo, php_zoo_init_globals,NULL);
  REGISTER_INI_ENTRIES();
  return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(zoo)
{
  UNREGISTER_INI_ENTRIES();
  return SUCCESS;
}

PHP_FUNCTION(zoo_Translate)
{
  char *value;
  int value_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len) == FAILURE) {
    RETURN_NULL();
  }
  RETURN_STRING(_ss(value), 1);
}

PHP_FUNCTION(zoo_UpdateStatus)
{
  zval *arr;
  char *message;
  int message_len;
  long pourcent;
  char *status=(char*)malloc(4*sizeof(char));
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "asl", &arr, &message, &message_len,&pourcent) == FAILURE) {
    RETURN_NULL();
  }
  HashTable* t=HASH_OF(arr);
  maps* conf=php_maps_from_Array(t);
  setMapInMaps(conf,"lenv","message",message);
  sprintf(status,"%d",pourcent);
  setMapInMaps(conf,"lenv","status",status);
  _updateStatus(conf);
  freeMaps(&conf);
  free(conf);
  free(status);
  RETURN_NULL();
}

PHP_FUNCTION(zoo_SERVICE_SUCCEEDED)
{
  RETURN_LONG(ZOO_G(_SERVICE_SUCCEEDED));
}

PHP_FUNCTION(zoo_SERVICE_FAILED)
{
  RETURN_LONG(ZOO_G(_SERVICE_FAILED));
}

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
    
  map* libp = getMapFromMaps(m, "main", "libPath");  
  int res=SERVICE_FAILED;

  map* tmp=getMap(s->content,"serviceProvider");
  if (tmp == NULL || tmp->value == NULL) {
	  return errorException(m, "Missing serviceProvider (library file)", "NoApplicableCode", NULL);
  }
  
  map* cwd=getMapFromMaps(m,"lenv","cwd");
#ifdef IGNORE_METAPATH
  map* mp = createMap("metapath", "");
#else  
  map* mp = getMap(request, "metapath");
#endif
  char *scriptName;
  
  if (libp != NULL && libp->value != NULL) {
	scriptName = (char*) malloc((strlen(libp->value) + strlen(tmp->value) + 2)*sizeof(char));
    sprintf (scriptName, "%s/%s", libp->value, tmp->value);	
  }
  else {	
    if(mp!=NULL && strlen(mp->value)>0){
      scriptName=(char*)malloc((strlen(cwd->value)+strlen(mp->value)+strlen(tmp->value)+3)*sizeof(char));
      sprintf(scriptName,"%s/%s/%s",cwd->value,mp->value,tmp->value);
    }else{
      scriptName=(char*)malloc((strlen(cwd->value)+strlen(tmp->value)+2)*sizeof(char));
      sprintf(scriptName,"%s/%s",cwd->value,tmp->value);
    }
  } 
  zend_file_handle iscript;
  iscript.type=ZEND_HANDLE_FD;
  iscript.opened_path=NULL;
  iscript.filename=tmp->value;
  iscript.free_filename=0;
  FILE* temp=fopen(scriptName,"rb");
  if(temp==NULL){
    char tmp1[1024];
    sprintf(tmp1,_("Unable to load the PHP file %s"),tmp->value);
    free(scriptName);
    return errorException(m,tmp1,"NoApplicableCode",NULL);
  }
  iscript.handle.fd=fileno(temp);

  php_embed_init(0,NULL PTSRMLS_CC);
   
  zend_try {
    zend_startup_module(&zoo_module_entry);
    php_execute_script(&iscript TSRMLS_CC);
    zval *iargs[3];
    zval iret, ifunc,ifile;
      
    ZVAL_STRING(&ifunc, s->name, 0);
    iargs[0] = php_Array_from_maps(*main_conf);
    iargs[1] = php_Array_from_maps(*real_inputs);
    iargs[2] = php_Array_from_maps(*real_outputs);
      
    if((res=call_user_function(EG(function_table), NULL, &ifunc, &iret, 3, iargs TSRMLS_CC))==SUCCESS){
      
      HashTable* t=HASH_OF(iargs[2]);
      HashTable* t1=HASH_OF(iargs[0]);
      freeMaps(real_outputs);
      free(*real_outputs);
      freeMaps(main_conf);
      free(*main_conf);
      *real_outputs=php_maps_from_Array(t);
      *main_conf=php_maps_from_Array(t1);

      res=Z_LVAL(iret);
    }else{
      free(scriptName);
      fclose(temp);
      return errorException(m,"Unable to process.","NoApplicableCode",NULL);;
    }
  } zend_catch { 
    free(scriptName);
    fclose(temp);
    return errorException(m,"Unable to process.","NoApplicableCode",NULL);;
  } zend_end_try();
  free(scriptName);
  fclose(temp);
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
    else{
      addMapsToMaps(&final_res,cursor);
      freeMaps(&cursor);
      free(cursor);
    }
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
	  final_res = addToMapWithSize(final_res,key,Z_STRVAL_P(&tmpcopy),len);
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
