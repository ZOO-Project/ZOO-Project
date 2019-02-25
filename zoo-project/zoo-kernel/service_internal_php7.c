/*
 * Author : Gérald FENOY, Knut LANDMARK
 *
 * Copyright (c) 2009-2016 GeoLabs SARL
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
  #define ZEND_WIN32_KEEP_INLINE
#endif

#ifndef ZEND_DEBUG
  #define ZEND_DEBUG 0
#endif  

#include <sapi/embed/php_embed.h>
#include <zend_stream.h>

#include "service_internal_php.h"
#include "response_print.h"

map* php_hashtable_to_map(HashTable* tab);
maps* php_array_to_maps(HashTable* arr);
void php_map_to_array(map* src, zval* arr);
void php_maps_to_array(maps* src, zval* arr);

#define zval_make_ref_array(zv,len) for (int elem = 0; elem < len; elem++) ZVAL_MAKE_REF(&zv[elem]);
#define zval_unref_array(zv,len)    for (int elem = 0; elem < len; elem++) ZVAL_UNREF(&zv[elem]);	

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
  size_t value_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len) == FAILURE) {
    RETURN_NULL();
  }
  //RETURN_STRING(_ss(value), 1);
  RETURN_STRINGL(_ss(value), 1);
}

PHP_FUNCTION(zoo_UpdateStatus)
{
  zval *arr;
  char *message;
  size_t message_len;
  zend_long pourcent;
  char *status=(char*)malloc(4*sizeof(char));
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "asl", &arr, &message, &message_len,&pourcent) == FAILURE) {
    RETURN_NULL();
  }
  HashTable* t=HASH_OF(arr);
  maps* conf = php_array_to_maps(t);
    
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
int zoo_php_support(maps** main_conf, map* request, service* s, maps **real_inputs, maps **real_outputs) {	
  maps* m = *main_conf;
  maps* inputs = *real_inputs;
  maps* outputs = *real_outputs;
    
  map* libp = getMapFromMaps(m, "main", "libPath");  
  int res=SERVICE_FAILED;

  map* tmp = getMap(s->content, "serviceProvider");
  if (tmp == NULL || tmp->value == NULL) {
    return errorException(m, "Missing serviceProvider (library file)", "NoApplicableCode", NULL);
  }  
  
  #ifdef IGNORE_METAPATH
  map* mp = createMap("metapath", "");
  #else  
  map* mp = getMap(request, "metapath");
  #endif
  
  map* cwd = getMapFromMaps(m,"lenv","cwd");
  char *scriptName;
  
  if (libp != NULL && libp->value != NULL) {
    scriptName = (char*) malloc((strlen(libp->value) + strlen(tmp->value) + 2)*sizeof(char));
    sprintf (scriptName, "%s/%s", libp->value, tmp->value);	
  }
  else {	
    if(mp != NULL && strlen(mp->value) > 0) {
      scriptName = (char*) malloc((strlen(cwd->value)+strlen(mp->value)+strlen(tmp->value)+3)*sizeof(char));
      sprintf(scriptName, "%s/%s/%s", cwd->value, mp->value, tmp->value);
    } 
	  else {
      scriptName = (char*) malloc((strlen(cwd->value)+strlen(tmp->value)+2)*sizeof(char));
      sprintf(scriptName, "%s/%s", cwd->value, tmp->value);
    }
  }
  
  zend_file_handle iscript;
  iscript.type = ZEND_HANDLE_FD;
  iscript.opened_path = NULL;
  iscript.filename = tmp->value;
  iscript.free_filename = 0;
  
  FILE* temp = fopen(scriptName, "rb");
  if(temp == NULL) {
    char tmp1[1024];
    sprintf(tmp1, _("Unable to load the PHP file %s"), tmp->value);
    free(scriptName);
    return errorException(m, tmp1, "NoApplicableCode", NULL);
  }
  iscript.handle.fd = fileno(temp);

  php_embed_init(0, NULL);

  zend_try {
    zend_startup_module(&zoo_module_entry);
    php_execute_script(&iscript TSRMLS_CC);

	  zval iargs[3];
    zval iret, ifunc;	
		      
	  ZVAL_STRING(&ifunc, s->name);
	
	  php_maps_to_array(*main_conf, iargs);    
    php_maps_to_array(*real_inputs, iargs+1);    
    php_maps_to_array(*real_outputs, iargs+2);
    zval_make_ref_array(iargs,3);
    
    if((res = call_user_function(EG(function_table), NULL, &ifunc, &iret, 3, iargs)) == SUCCESS) {	
		
	    zval_unref_array(iargs,3);	  	  
	    zend_string_release(Z_STR_P(&ifunc));
      
      HashTable* t = HASH_OF(iargs+2);
      HashTable* t1 = HASH_OF(iargs);
      freeMaps(real_outputs);
      free(*real_outputs);
      freeMaps(main_conf);
      free(*main_conf);
      
      *real_outputs = php_array_to_maps(t);	  
      *main_conf = php_array_to_maps(t1);

      res = Z_LVAL(iret);
    }
    else {
      free(scriptName);
      fclose(temp);
      return errorException(m, "Unable to process.", "NoApplicableCode", NULL);;
    }
  } 
  zend_catch { 
    free(scriptName);
    fclose(temp);
    return errorException(m, "Unable to process.", "NoApplicableCode", NULL);;
  } 
  zend_end_try();
  
  free(scriptName);
  fclose(temp);
  php_embed_shutdown(TSRMLS_C);

  return res;
}

/**
 * Convert a Zoo struct maps to a php array
 *
 * @param src the struct maps* to convert (source data structure)
 * @param arr pointer to zval that is to hold the php array
 */
void php_maps_to_array(maps* src, zval* arr) {
	
  int tres = array_init(arr);
  maps* pm = src;
		
  while (pm != NULL) {
    zval zv; // allocate on heap??
    php_map_to_array(pm->content, &zv);
    add_assoc_zval(arr, pm->name, &zv);	
    pm = pm->next;
	}
}

/**
 * Convert a Zoo map to a php array
 *
 * @param src the struct map* to convert (source data structure)
 * @param arr pointer to zval that is to hold the php array
 */
void php_map_to_array(map* src, zval* arr) {
	
  int tres = array_init(arr);
  map* pm = src;
	
  while(pm != NULL) {
    map* sMap = getMapArray(pm, "size", 0);
		
    if (pm->value != NULL && strncmp(pm->name, "value", 5) == 0 && sMap != NULL) {			 
      tres = add_assoc_stringl(arr, pm->name, pm->value, atoi(sMap->value));
		}
    else {
      tres = add_assoc_string(arr, pm->name, pm->value);			
		}						
    pm = pm->next;			
  }	
}

/**
 * Convert a php hashtable (array) to a Zoo maps
 *
 * @param arr the php hashtable to convert
 * @return the created struct maps*
 */
maps* php_array_to_maps(HashTable* arr) {

  maps* res = NULL;
  zval* data;
  zend_string* key = NULL;
  ulong num_key;	
	
  ZEND_HASH_FOREACH_KEY_VAL(arr, num_key, key, data)
    if (!key) { // HASH_KEY_IS_LONG			
      key = strpprintf(64, "%lu", num_key);
	  }	
    zval copy;
    ZVAL_DUP(&copy, data);
    HashTable* tab = HASH_OF(&copy);
		
    maps* node = createMaps(ZSTR_VAL(key));					
    node->content = php_hashtable_to_map(tab);
		
    if(res == NULL) {
      res = node;
    }
    else {
      addMapsToMaps(&res, node);
      freeMaps(&node);
      free(node);
    }
    zval_dtor(&copy);		
  ZEND_HASH_FOREACH_END();
	
  return res;
}

/**
 * Convert a php hashtable (array) to a Zoo map
 *
 * @param tab the php hashtable to convert
 * @return the created struct map*
 */
map* php_hashtable_to_map(HashTable* tab) {

  map* res = NULL;
  zval* data;
  zend_string* key = NULL;
  ulong num_key;
	
  ZEND_HASH_FOREACH_KEY_VAL(tab, num_key, key, data)
    if (!key) { // HASH_KEY_IS_LONG			
      key = strpprintf(64, "%lu", num_key);
    }
    zval copy;
    ZVAL_DUP(&copy, data);
    convert_to_string(&copy);
		
    if(strncmp(ZSTR_VAL(key), "value", 5) == 0) {			
      size_t len = Z_STRLEN(copy);      			
      res = addToMapWithSize(res, ZSTR_VAL(key), Z_STRVAL(copy), len);
    }
    else {
      if (res == NULL) {				
        res = createMap(ZSTR_VAL(key), Z_STRVAL(copy));
			}
      else {				
        addToMap(res, ZSTR_VAL(key), Z_STRVAL(copy));
      }
    }				
    zval_dtor(&copy); 			
  ZEND_HASH_FOREACH_END();
	
  return res;	
}	
