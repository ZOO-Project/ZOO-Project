/*
 * Author : Gérald FENOY
 * 
 * ref. http://www.mono-project.com/docs/advanced/embedding/
 *      http://www.mono-project.com/docs/advanced/pinvoke/
 *      http://www.giorgosdimtsas.net/embedding-mono-in-your-c-application/
 *
 * Copyright (c) 2009-2013 GeoLabs SARL
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

#include "service_internal_mono.h"
#include "response_print.h"

MonoClass *iclasses[4];
MonoMethod *imethods[10];

/**
 * Load a Mono dll then run the static public method corresponding to the 
 * service by passing the conf, inputs and outputs parameters.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 */
int zoo_mono_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  char *ntmp=NULL;
  map* tmp=getMap(request,"metapath");
  char *classpath;
  char *oclasspath;
  int res=SERVICE_FAILED;
  MonoAssembly* assembly;
  MonoImage* monoImage;

  char* libPath="/usr/lib";
  char* etcPath="/etc";
  map* libPathMap=getMapFromMaps(*main_conf,"mono","lib");
  map* etcPathMap=getMapFromMaps(*main_conf,"mono","etc");
  if(libPathMap!=NULL)
    libPath=libPathMap->value;
  if(etcPathMap!=NULL)
    etcPath=etcPathMap->value;
  
  mono_set_dirs(libPath,etcPath);

  map* configMap=getMapFromMaps(*main_conf,"mono","config");
  if(configMap!=NULL)
    mono_config_parse(configMap->value);
  else
    mono_config_parse(NULL);

  MonoDomain* monoDomain;
  map* mono;
  if (hasvalue(*main_conf, "mono", "version", &mono)) {	  
	  monoDomain = mono_jit_init_version("ZOO_Embedded_Domain", mono->value);
  }
  else {	  
	  monoDomain = mono_jit_init("ZOO_Embedded_Domain"); // use default framework version
  }

  //MonoDomain* monoDomain = mono_jit_init_version("ZOO_Embedded_Domain", "v4.0.30319"); 
  //MonoDomain* monoDomain = mono_jit_init("ZOO_Embedded_Domain");  

  char* ZMapsLib = "ZMaps.dll";
  char *zooAssembly = NULL;
  
  map* config = NULL, *basedir = NULL;
  if (hasvalue(*main_conf, "mono", "configdir", &basedir) && // knut: rename to e.g. "base_dir"?
	  hasvalue(*main_conf, "mono", "configfile", &config)) {	  
	  mono_domain_set_config(monoDomain, basedir->value, config->value); 
	  zooAssembly = file_exists(basedir->value, ZMapsLib);
  }
  else {	  
	  mono_domain_set_config(monoDomain, "./", "");
  }    

  map* cwdMap;
  if (! hasvalue(*main_conf, "main", "servicePath", &cwdMap) &&
	  ! hasvalue(*main_conf, "main", "libPath", &cwdMap) ) {
	  cwdMap = getMapFromMaps(*main_conf, "lenv", "cwd");
  }  

  if (zooAssembly != NULL || (zooAssembly = file_exists(cwdMap->value, ZMapsLib)) != NULL) {
	  assembly = mono_domain_assembly_open(monoDomain, zooAssembly);
  }
  else { 
	  // try to load from default locations (GAC?)
	  assembly = mono_domain_assembly_open(monoDomain, ZMapsLib);
  }  
  free(zooAssembly);

  if (assembly == NULL) {
	  setMapInMaps(*main_conf, "lenv", "message", _("The ZMaps.dll assembly cannot be found!"));
	  return 4;
  }

  mono_add_internal_call ("ZOO_API::Translate", MonoTranslate);    
  mono_add_internal_call("ZOO_API::UpdateStatus", MonoUpdateStatus);
  
  monoImage = mono_assembly_get_image(assembly);
  MonoClass *KeysList = mono_class_from_name(monoImage, "ZooGenerics", "KeysList");
  MonoClass *ZMapsClass = mono_class_from_name(monoImage, "ZooGenerics", "ZMaps");
  MonoMethod *ZMapsAdd = mono_class_get_method_from_name(ZMapsClass, "addToMaps", 2);  
  MonoMethod *ZMapsGetMaps = mono_class_get_method_from_name(ZMapsClass, "getMaps", 1);
  MonoClass *_ZMapsClass = mono_class_from_name(monoImage, "ZooGenerics", "_ZMaps");
  MonoMethod *_ZMapsSetContent = mono_class_get_method_from_name(_ZMapsClass, "setContent", 1);
  MonoMethod *_ZMapsSetChild = mono_class_get_method_from_name(_ZMapsClass, "setChild", 1);
  MonoMethod *_ZMapsGetContent = mono_class_get_method_from_name(_ZMapsClass, "getContent", 0);
  MonoMethod *_ZMapsGetChild = mono_class_get_method_from_name(_ZMapsClass, "getChild", 0);
  MonoClass *ZMapClass = mono_class_from_name(monoImage, "ZooGenerics", "ZMap");
  MonoMethod *ZMapGetMap = mono_class_get_method_from_name(ZMapClass, "getMap", 1);
  MonoMethod *ZMapAdd = mono_class_get_method_from_name(ZMapClass, "addToMap", 2);
  MonoMethod *ZMapDisplay = mono_class_get_method_from_name(ZMapClass, "display", 1);
  MonoObject *exc=NULL;

  MonoClass *classes[3]={_ZMapsClass,ZMapsClass,ZMapClass};
  MonoMethod *methods[5]={ZMapsAdd,_ZMapsSetContent,_ZMapsSetChild,ZMapAdd,ZMapDisplay};
  
  MonoMethod *ZMapsGetKeys = mono_class_get_method_from_name(ZMapsClass, "getKeys", 0);
  
  MonoMethod *ZMapsGetKeysCount = mono_class_get_method_from_name(ZMapsClass, "getKeysCount", 0);
  MonoMethod *ZMapsGetKey = mono_class_get_method_from_name(ZMapsClass, "getKey", 1);
  MonoMethod *ZMapGetKeys = mono_class_get_method_from_name(ZMapClass, "getKeys", 0);
  MonoClassField *ZMapKeys = mono_class_get_field_from_name(ZMapClass, "Keys");
  MonoMethod *ZMapGetKeysCount = mono_class_get_method_from_name(ZMapClass, "getKeysCount", 0);
  MonoMethod *ZMapGetKey = mono_class_get_method_from_name(ZMapClass, "getKey", 1);
  MonoMethod *ZMapGetSize = mono_class_get_method_from_name(ZMapClass, "getSize", 0);
  MonoMethod *ZMapGetMapAsBytes = mono_class_get_method_from_name(ZMapClass, "getMapAsBytes", 0);
  MonoClass *oclasses[4]={_ZMapsClass,ZMapsClass,ZMapClass,KeysList};
  MonoMethod *omethods[10]={ZMapsGetMaps,ZMapsGetKeysCount,ZMapsGetKey,ZMapGetMap,ZMapGetKeysCount,ZMapGetKey,_ZMapsGetContent,_ZMapsGetChild,ZMapGetSize,ZMapGetMapAsBytes};
  
  for(int i=0;i<4;i++){
    iclasses[i]=oclasses[i];
    imethods[i]=omethods[i];
  }
  for(int i=4;i<10;i++)
    imethods[i]=omethods[i];

  void *args[3];

  MonoObject * main=ZMapsFromMaps(monoDomain,classes,methods,&exc,m);

  MonoObject * in=ZMapsFromMaps(monoDomain,classes,methods,&exc,inputs);

  MonoObject * out=ZMapsFromMaps(monoDomain,classes,methods,&exc,outputs);
  
  args [0] = main;
  args [1] = in;
  args [2] = out;

  char* sName = NULL;
  MonoAssembly* serviceAssembly = NULL;
  
  tmp = getMap(s->content, "serviceProvider");  
  char* test = file_exists(NULL, "log.txt");
  if ( (sName = file_exists(cwdMap->value, tmp->value)) != NULL ||
	   (sName = file_exists(".", tmp->value)) != NULL ||  // in case servicePath is defined but serviceProvider is in working directory
	   (basedir != NULL && (sName = file_exists(basedir->value, tmp->value)) != NULL) ) {

	   serviceAssembly = mono_domain_assembly_open(monoDomain, sName);
  }
  else {
	  // try to open from default locations	  
	  serviceAssembly = mono_domain_assembly_open(monoDomain, tmp->value);
  }
  free(sName);

  //char *sName=(char*)malloc((strlen(tmp->value)+strlen(cwdMap->value)+2)*sizeof(char));
  //sprintf(sName,"%s/%s",cwdMap->value,tmp->value);
  //MonoAssembly* serviceAssembly = mono_domain_assembly_open(monoDomain,sName);
  
  if(serviceAssembly==NULL){
    char *msg=(char*)malloc((strlen(_("Your service assembly: %s cannot be found!"))+strlen(tmp->value)+1)*sizeof(char));
    sprintf(msg,_("Your service assembly %s cannot be found!"),tmp->value);
    setMapInMaps(*main_conf,"lenv","message",msg);
    free(msg);
    return 4;
  }
  
  MonoImage* serviceImage = mono_assembly_get_image(serviceAssembly);

  map* tmp1=getMap(s->content,"serviceNameSpace");
  
  tmp=getMap(s->content,"serviceClass");
  MonoClass *serviceClass = mono_class_from_name(serviceImage, tmp1->value, tmp->value);
  if(serviceClass==NULL){
    char *msg=(char*)malloc((strlen(_("Your service class method %s::%s cannot be found!"))+strlen(tmp1->value)+strlen(tmp->value)+1)*sizeof(char));
    sprintf(msg,_("Your service class method %s::%s cannot be found!"),tmp1->value,tmp->value);
    setMapInMaps(*main_conf,"lenv","message",msg);
    free(msg);
    return 4;
  }
  
  MonoMethod *serviceFunction = mono_class_get_method_from_name(serviceClass, s->name, 3);
  if(serviceFunction==NULL){
    char *msg=(char*)malloc((strlen(_("Your service static method %s cannot be found!"))+strlen(s->name)+1)*sizeof(char));
    sprintf(msg,_("Your service static method %s cannot be found!"),s->name);
    setMapInMaps(*main_conf,"lenv","message",msg);
    free(msg);
    return 4;
  }
  
  MonoObject *exception=NULL;
  MonoObject *result = mono_runtime_invoke(serviceFunction,NULL,args,&exception);
  
  if (exception != NULL) { // knut: add exception handling
	  MonoString* pMsg = mono_object_to_string(exception, NULL);	  
	  setErrorMessage(*main_conf, s->name, NoApplicableCode, mono_string_to_utf8(pMsg));
	  return 4;
  }

  res=*(int*)mono_object_unbox (result);
  
  if(res==3){
    freeMaps(&outputs);
    free(outputs);
    outputs=mapsFromZMaps(monoDomain,oclasses,omethods,(MonoObject*)args[2]);
    *real_outputs=outputs;
  }
  
  freeMaps(&m);
  free(m);
  m=mapsFromZMaps(monoDomain,oclasses,omethods,(MonoObject*)args[0]);
  *main_conf=m;
  mono_jit_cleanup (monoDomain);  
  return res;
}

/**
 * Create ZMaps from maps
 * @param domain the Mono Domain
 * @param classes the Mono classes required to create (_ZMaps,ZMaps and ZMap)
 * @param methods the Mono methods required (ZMapsAdd, _ZMapsSetContent, _ZMapsSetChild, ZMapAdd and ZMapDisplay)
 * @param m the maps to use as source to create the corresponding ZMaps
 * @return MonoObject* to a new ZMap
 */
MonoObject* ZMapsFromMaps(MonoDomain* domain,MonoClass **classes,MonoMethod **methods,MonoObject **exc,maps* m){
  MonoObject *res=mono_object_new(domain,classes[1]);
  MonoMethod* constructorMethod = mono_class_get_method_from_name(classes[1],
								  ".ctor",
								  0);
  mono_runtime_invoke(constructorMethod, res, NULL, exc);
  MonoMethod* constructorMethod1 = mono_class_get_method_from_name(classes[0],
								   ".ctor",
								   0);
  
  while(m!=NULL){
    MonoObject *tmpRes=mono_object_new(domain,classes[0]);
    mono_runtime_invoke(constructorMethod1, tmpRes, NULL, exc);
    void *values0 [1];
    MonoObject* content = ZMapFromMap (domain,classes[2],methods,exc,m->content);
    values0 [0] = content;
    MonoObject *exception=NULL;
    mono_runtime_invoke(methods[1],tmpRes,values0,&exception);
    if(exception!=NULL)
      {
	fprintf(stderr,"ERROR : %s\n", mono_string_to_utf8(mono_object_to_string(exception, NULL)));
	fflush(stderr);
	return res;
      }
    if(m->child!=NULL){
      void *values [1];
      values [0] = ZMapsFromMaps (domain,classes,methods,exc,m->child);
      mono_runtime_invoke(methods[2],tmpRes,values,exc);
    }
    void *values [2];
    values [0] = mono_string_new (domain, m->name);
    values [1] = tmpRes;
    mono_runtime_invoke(methods[0],res,values,exc);
    m=m->next;
  }
  return res;
}

/**
 * Create ZMap from map
 * @param domain the Mono Domain
 * @param classes the Mono classes required to create (_ZMaps,ZMaps and ZMap)
 * @param methods the Mono methods required (ZMapsAdd, _ZMapsSetContent, _ZMapsSetChild, ZMapAdd and ZMapDisplay)
 * @param exc the Mono exception object
 * @param m the map to use as source to create a new ZMap
 * @return MonoObject* to a new ZMap
 */
MonoObject* ZMapFromMap(MonoDomain* domain,MonoClass *ZMapClass,MonoMethod **methods,MonoObject **exc,map* m){
  MonoObject *res=mono_object_new(domain,ZMapClass);
  MonoMethod* constructorMethod = mono_class_get_method_from_name(ZMapClass,
								  ".ctor",
								  0);
  mono_runtime_invoke(constructorMethod, res, NULL, exc);
  map* sizeMap=getMap(m,"size");
  while(m!=NULL){
    void *values [2];
    MonoString* name = mono_string_new(domain, m->name);
    MonoString* value;
    if(sizeMap!=NULL && strcasecmp(m->name,"value")==0)
      value = mono_string_new_len(domain, m->value,atoi(sizeMap->value));
    else
      value = mono_string_new(domain, m->value);
    values [0] = name;
    values [1] = value;
    mono_runtime_invoke(methods[3],res,values,exc);
    m=m->next;
  }
  return res;
}

/**
 * Create maps from ZMaps
 * @param domain the Mono Domain
 * @param classes the Mono classes required to create (_ZMapsClass,ZMapsClass,ZMapClass,KeysList)
 * @param methods the Mono methods required (ZMapsGetMaps,ZMapsGetKeysCount,ZMapsGetKey,ZMapGetMap,ZMapGetKeysCount,ZMapGetKey,_ZMapsGetContent,_ZMapsGetChild)
 * @param m the ZMaps to use as source to create a new maps
 * @return MonoObject* to a new ZMaps
 */
maps* mapsFromZMaps(MonoDomain* domain,MonoClass **classes,MonoMethod **methods,MonoObject* m){
  maps* res=NULL;
  if(m!=NULL){
    MonoObject *exc=NULL;
    MonoObject *okeys = mono_runtime_invoke(methods[1],m,NULL,&exc);
    if(exc!=NULL)
      {
	fprintf(stderr,"ERROR : %s\n", mono_string_to_utf8(mono_object_to_string(exc, NULL)));
	fflush(stderr);
	return res;
      }

    int keyCount=*(int*)mono_object_unbox (okeys);
    for(int i=0;i<keyCount;i++){
      void* values[1];
      values[0]=&i;
      exc=NULL;
      MonoString *okey = (MonoString*) mono_runtime_invoke(methods[2],m,values,&exc);
      char* key = mono_string_to_utf8(okey);
      maps* tmp=createMaps(key);
      values[0]=mono_string_new (domain, key);
      MonoString *content_ZMaps = (MonoString*) mono_runtime_invoke(methods[0],m,values,&exc);
      MonoObject *content = mono_runtime_invoke(methods[6],(MonoObject*)content_ZMaps,NULL,&exc);
      tmp->content=mapFromZMap(domain,classes,methods,content);
      MonoObject *childs = mono_runtime_invoke(methods[7],content_ZMaps,NULL,&exc);
      tmp->child=mapsFromZMaps(domain,classes,methods,childs);
      if(res==NULL){
	res=dupMaps(&tmp);
      }else
	addMapsToMaps(&res,tmp);
      freeMaps(&tmp);
      free(tmp);
      mono_free(key);
    }
  }
  return res;
}

/**
 * Create map from ZMap
 * @param domain the Mono Domain
 * @param classes the Mono classes required (_ZMapsClass,ZMapsClass,ZMapClass,KeysList)
 * @param methods the Mono methods required (ZMapsGetMaps,ZMapsGetKeysCount,ZMapsGetKey,ZMapGetMap,ZMapGetKeysCount,ZMapGetKey,_ZMapsGetContent,_ZMapsGetChild,ZMapGetSize)
 * @param m the ZMap to use as source to create a new map
 * @return MonoObject* to a new ZMap
 */
map* mapFromZMap(MonoDomain* domain,MonoClass **classes,MonoMethod **methods,MonoObject* m){
  map* res=NULL;
  if(m!=NULL){
    MonoObject *exc = NULL;
    MonoObject *okeys = mono_runtime_invoke(methods[4],m,NULL,&exc);
    int keyCount=*(int*)mono_object_unbox (okeys);
    MonoObject *osize = mono_runtime_invoke(methods[8],m,NULL,&exc);
    int size=*(int*)mono_object_unbox (osize);
    for(int i=0;i<keyCount;i++){
      void* values[1];
      values[0]=&i;
      exc=NULL;
      MonoString *okey = (MonoString*) mono_runtime_invoke(methods[5],m,values,&exc);
      char* key = mono_string_to_utf8(okey);
      values[0]=mono_string_new (domain, key);
      exc=NULL;
      MonoString *ovalue = (MonoString*) mono_runtime_invoke(methods[3],m,values,&exc);
      char* value = mono_string_to_utf8(ovalue);
      if(res==NULL)
	res=createMap(key,value);
      else
	addToMap(res,key,value);
    }
  }
  return res;
}

/**
 * The translate function to search in zoo-services gettext domain for translating a string.
 * @param str the string to be translated
 * @return the translated string
 */
static MonoString* MonoTranslate(MonoString *str){
  return mono_string_new (mono_domain_get(), _(mono_string_to_utf8(str)));
}

/**
 * The updateStatus function to update the ongoing status of a ZOO-Service.
 * @param m the main.cfg map
 * @param str the message for the current step 
 * @param p the percentage of the 
 * @return the translated string
 */
static void MonoUpdateStatus(MonoObject* obj, MonoString *str, int p) {	
	MonoDomain* domain = mono_object_get_domain(obj);	
	maps *confMaps = mapsFromZMaps(domain, iclasses, imethods, obj);	
	setMapInMaps(confMaps, "lenv", "message", mono_string_to_utf8(str));	
	int tmp = p > 100 || p < 0 ? -1 : p;	
	char tmpStatus[5];
	snprintf(tmpStatus, 4, "%d", tmp);	
	setMapInMaps(confMaps, "lenv", "status", (char*)tmpStatus);	
	_updateStatus(confMaps);	
	freeMaps(&confMaps);	
	free(confMaps);
	return;
}