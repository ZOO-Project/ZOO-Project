/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2014 GeoLabs SARL
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

#include "service_internal_ruby.h"

#if RUBY_API_VERSION_MAJOR >= 2 || RUBY_API_VERSION_MINOR == 9 
int argc=0;
char **argv=NULL;
#endif

/**
 * Load a Ruby file then run the function corresponding to the service by
 * passing the conf, inputs and outputs parameters by refernce as Ruby Hash.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 * @return SERVICE_SUCCEEDED or SERVICE_FAILED if the service run, -1 
 *  if the service failed to load or throw error at runtime.
 */
int zoo_ruby_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
#if RUBY_API_VERSION_MAJOR >= 2 || RUBY_API_VERSION_MINOR == 9
  ruby_sysinit(&argc,&argv);
  RUBY_INIT_STACK;
#endif
  ruby_init();
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  map* tmp0=getMapFromMaps(*main_conf,"lenv","cwd");
  char *ntmp=tmp0->value;
  map* tmp=NULL;
  ruby_init_loadpath();
  ruby_script("ZOO_EMBEDDED_ENV");
  
  VALUE klass=rb_define_module("Zoo");
  rb_define_const(klass,"SERVICE_SUCCEEDED",INT2FIX(3));
  rb_define_const(klass,"SERVICE_FAILED",INT2FIX(4));
  typedef VALUE (*HOOK)(...);
  rb_define_module_function(klass,"Translate",reinterpret_cast<HOOK>(RubyTranslate),-1);
  rb_define_module_function(klass,"UpdateStatus",reinterpret_cast<HOOK>(RubyUpdateStatus),-1);

  int error = 0;
		
  ID rFunc=Qnil;
  tmp=getMap(s->content,"serviceProvider");
  if(tmp!=NULL){
#if RUBY_VERSION_MINOR == 8
    const char* script = ruby_sourcefile = rb_source_filename(tmp->value);
    rb_protect(LoadWrap, reinterpret_cast<VALUE>(script), &error);
#else
    rb_load_protect(rb_str_new2(tmp->value), 0, &error);
#endif
    if(error) {
      ruby_trace_error(m);
      return -1;
    }
#if RUBY_VERSION_MINOR == 8
    ruby_exec();
#else
    ruby_exec_node(NULL);
#endif
  }
  else{
    map* err=createMap("text","Unable to parse serviceProvider please check your zcfg file.");
    addToMap(err,"code","NoApplicableCode");
    printExceptionReportResponse(m,err);
    return -1;
  }
  int res=SERVICE_FAILED;
  rFunc=rb_intern(s->name);
  if(rFunc!=Qnil){
    VALUE arg1=RubyHash_FromMaps(m);
    VALUE arg2=RubyHash_FromMaps(inputs);
    VALUE arg3=RubyHash_FromMaps(outputs);
    VALUE rArgs[3]={arg1,arg2,arg3};
    if (!rArgs)
      return -1;
    struct my_callback data;
    data.obj=Qnil;
    data.method_id=rFunc;
    data.nargs=3;
    data.args[0]=rArgs[0];
    data.args[1]=rArgs[1];
    data.args[2]=rArgs[2];
    typedef VALUE (*HOOK)(VALUE);
    VALUE tres=rb_protect(reinterpret_cast<HOOK>(FunCallWrap),(VALUE)(&data),&error);
    if (TYPE(tres) == T_FIXNUM) {
      res=FIX2INT(tres);
      freeMaps(real_outputs);
      free(*real_outputs);
      freeMaps(main_conf);
      free(*main_conf);
      *main_conf=mapsFromRubyHash(arg1);
      *real_outputs=mapsFromRubyHash(arg3);
#ifdef DEBUG
      dumpMaps(*main_conf);
      dumpMaps(*real_outputs);
#endif
    }else{
      ruby_trace_error(m);
      res=-1;
    }
  }
  else{
    char tmpS[1024];
    sprintf(tmpS, "Cannot find the %s function in the %s file.\n", s->name, tmp->value);
    map* tmps=createMap("text",tmpS);
    printExceptionReportResponse(m,tmps);
    res=-1;
  }
  ruby_finalize();
  return res;
}

/**
 * Load a ruby file
 *
 * @arg the file to load
 * @return Qnil
 */
VALUE LoadWrap(VALUE arg) {
  const char *filename = reinterpret_cast<const char*>(arg);
  rb_load_file(filename);
  return Qnil;
}

/**
 * Call a ruby function with parameters
 *
 * @arg the callback structure
 * @return the value returned the called ruby function
 */
VALUE FunCallWrap(VALUE rdata) {
  struct my_callback* data = (struct my_callback*) rdata;
  return rb_funcall2(data->obj,data->method_id,data->nargs,data->args);
}

/**
 * Print the Ruby Stack Trace in an ows:ExceptionReport XML Document
 *
 * @param m the conf maps containing the main.cfg settings
 * @see printExceptionReportResponse
 */
void ruby_trace_error(maps* m){
#if RUBY_VERSION_MINOR == 8
  VALUE lasterr = rb_gv_get("$!");
#else
  VALUE lasterr = rb_errinfo();
  VALUE ruby_errinfo = lasterr;
#endif
  VALUE message = rb_obj_as_string(lasterr);
  VALUE lklass = rb_class_path(CLASS_OF(lasterr));
#if RUBY_VERSION_MINOR == 8
  char *trace=(char*)malloc((strlen(RSTRING(lklass)->ptr)+strlen(RSTRING(message)->ptr)+3)*sizeof(char));
  sprintf(trace,"%s: %s",RSTRING_PTR(lklass),RSTRING_PTR(message));
#else
  char *trace=(char*)malloc((strlen(RSTRING_PTR(lklass))+strlen(RSTRING_PTR(message))+3)*sizeof(char));
  sprintf(trace,"%s: %s",RSTRING_PTR(lklass),RSTRING_PTR(message));
#endif
  if(!NIL_P(ruby_errinfo))
    {
      VALUE ary = rb_funcall(ruby_errinfo, rb_intern("backtrace"), 0);
      int c;
      for (c=0; c<RARRAY_LEN(ary); c++) {
	int len=strlen(trace);
	char *tmp0=zStrdup(trace);
#if RUBY_VERSION_MINOR == 8
	trace=(char *) realloc(trace,(len+strlen(RSTRING(RARRAY(ary)->ptr[c])->ptr)+2)*sizeof(char));
	sprintf(trace,"%s\n%s",tmp0,RSTRING(RARRAY(ary)->ptr[c])->ptr);
#else
	trace=(char *) realloc(trace,(len+strlen(RSTRING_PTR(RARRAY_PTR(ary)[c]))+2)*sizeof(char));
	sprintf(trace,"%s\n%s",tmp0,RSTRING_PTR(RARRAY_PTR(ary)[c]));
#endif
	free(tmp0);
      }
    }
  map* err=createMap("text",trace);
  addToMap(err,"code","NoApplicableCode");
  printExceptionReportResponse(m,err);
}

/**
 * Convert a maps to a Ruby Hash
 *
 * @param t the maps to convert
 * @return a new Ruby Hash
 */
VALUE RubyHash_FromMaps(maps* t){
  VALUE res=rb_hash_new();
  maps* tmp=t;
  while(tmp!=NULL){
    VALUE value=RubyHash_FromMap(tmp->content);
    VALUE name=rb_str_new2(tmp->name);
    rb_hash_aset(res,name,value);
    tmp=tmp->next;
  }  
  return res;
}

/**
 * Push the key on the array
 *
 * @param key the key to push
 * @param value not used
 * @param ary the Array
 * @return ST_CONTINUE
 */
int
keys_i(VALUE key,VALUE value,VALUE ary)
{
  if (key == Qundef) return ST_CONTINUE;
  rb_ary_push(ary, key);
  return ST_CONTINUE;
}

/**
 * Return the size of a Ruby Hash
 * 
 * @param hash the input Hash
 */
VALUE
rb_hash_size(VALUE hash)
{
    return INT2FIX(RHASH_TBL(hash)->num_entries);
}

/**
 * Convert a map to a Ruby Hash
 *
 * @param t the map to convert
 * @return a new Ruby Hash
 */
VALUE RubyHash_FromMap(map* t){
  VALUE res=rb_hash_new( );
  map* tmp=t;
  int hasSize=0;
  map* isArray=getMap(tmp,"isArray");
  map* useFile=getMap(tmp,"use_file");
  map* size=getMap(tmp,"size");
  map* tmap=getMapType(tmp);
  while(tmp!=NULL){
    VALUE name= rb_str_new2(tmp->name);
    if(strcasecmp(tmp->name,"value")==0) {
      if(isArray!=NULL){
	map* len=getMap(tmp,"length");
	int cnt=atoi(len->value);
	VALUE value=rb_ary_new2(cnt);
	VALUE mvalue=rb_ary_new2(cnt);
	VALUE svalue=rb_ary_new2(cnt);
	VALUE cvalue=rb_ary_new2(cnt);

	for(int i=0;i<cnt;i++){
	  
	  map* vMap=getMapArray(tmp,"value",i);	    
	  map* sMap=getMapArray(tmp,"size",i);
	  map* uMap=getMapArray(tmp,"use_file",i);
	  map* cMap=getMapArray(tmp,"cache_file",i);

	  if(vMap!=NULL){
	    
	    VALUE lvalue;
	    VALUE lsvalue;
	    VALUE lcvalue;
	    if(sMap==NULL || uMap==NULL){
	      lvalue=rb_str_new2(vMap->value);
	      if(sMap==NULL)
		lsvalue=Qnil;
	      else
		lsvalue=rb_str_new2(sMap->value);
	    }
	    else{
	      lvalue=rb_str_new(vMap->value,atoi(sMap->value));
	      lsvalue=rb_str_new2(sMap->value);
	      hasSize=1;
	    }
	    if(uMap!=NULL)
	      lcvalue=rb_str_new2(cMap->value);
	    else
	      lcvalue=Qnil;

	    rb_ary_push(value,lvalue);
	    rb_ary_push(svalue,lsvalue);
	    rb_ary_push(cvalue,lcvalue);
	  }
	  
	  map* mMap=getMapArray(tmp,tmap->name,i);

	  VALUE lmvalue;
	  if(mMap!=NULL){
	    lmvalue=rb_str_new2(mMap->value);
	  }else
	    lmvalue=Qnil;
	  rb_ary_push(mvalue,lmvalue);
	  
	}

	rb_hash_aset(res, name, mvalue);
	rb_hash_aset(res, rb_str_new2(tmap->name), mvalue);
	VALUE lname0=rb_str_new2("cache_size");
	rb_hash_aset(res, lname0, value);
      
	if(hasSize>0){
	  VALUE lname=rb_str_new2("size");
	  rb_hash_aset(res, lname, value);
	}
      }
      else if(size!=NULL){
	VALUE value=rb_str_new(tmp->value,atoi(size->value));
	rb_hash_aset(res, name, value);
      }
      else{
	VALUE value=rb_str_new2(tmp->value);
	rb_hash_aset(res, name, value);
      }
    }
    else{
      VALUE value=rb_str_new2(tmp->value);
      rb_hash_aset(res, name, value);
    }
    tmp=tmp->next;
  }
  return res;
}

/**
 * Convert a Ruby Hash to a maps
 *
 * @param t the Ruby Hash to convert
 * @return a new maps
 */
maps* mapsFromRubyHash(VALUE t){
  maps* res=NULL;
  maps* cursor=res;
  VALUE list;
  list = rb_ary_new();
  typedef int (*HOOK)(...);
  rb_hash_foreach(t, reinterpret_cast<HOOK>(keys_i), list);
  int nb=rb_hash_size(t);
  int i;
  for(i=0;i<FIX2INT(nb);i++){
    VALUE key=rb_ary_pop(list);
    VALUE value=rb_hash_aref(t,key);
#ifdef DEBUG
    fprintf(stderr,">> DEBUG VALUES : %s => %s\n",
	    StringValueCStr(key),StringValueCStr(value));
#endif
    cursor=createMaps(StringValueCStr(key));
    cursor->content=mapFromRubyHash(value);
    if(res==NULL)
      res=dupMaps(&cursor);
    else
      addMapsToMaps(&res,cursor);
    freeMaps(&cursor);
    free(cursor);
  }
  return res;
}

/**
 * Convert a Ruby Hash to a map
 *
 * @param t the Ruby Hash to convert
 * @return a new map
 */
map* mapFromRubyHash(VALUE t){
  map* res=NULL;
  VALUE list;
  list = rb_ary_new();
  typedef int (*HOOK)(...);
  rb_hash_foreach(t,reinterpret_cast<HOOK>(keys_i), list);
  int nb=RHASH_TBL(t)->num_entries;
  int i;
  for(i=0;i<nb;i++){
    VALUE key=rb_ary_pop(list);
    VALUE value=rb_hash_aref(t,key);
#ifdef DEBUG
    fprintf(stderr,">> DEBUG VALUES : %s => %s\n",
	    StringValueCStr(key),StringValueCStr(value));
#endif
    if(strcmp(StringValueCStr(key),"value")==0){
      char *buffer=NULL;
      int size=RSTRING_LEN(value);
      buffer=StringValueCStr(value);
      if(res!=NULL){
	addToMap(res,StringValueCStr(key),"");
      }else{
	res=createMap(StringValueCStr(key),"");
      }
      map* tmpR=getMap(res,"value");
      free(tmpR->value);
      tmpR->value=(char*)malloc((size+1)*sizeof(char));
      memmove(tmpR->value,buffer,size*sizeof(char));
      tmpR->value[size]=0;
      char sin[1024];
      sprintf(sin,"%d",size);
      addToMap(res,"size",sin);
    }else{
      if(res!=NULL){
	addToMap(res,StringValueCStr(key),StringValueCStr(value));
      }
      else{
	res=
	  createMap(StringValueCStr(key),StringValueCStr(value));
      }
    }
  }
  return res;
}

/**
 * Use the ZOO-Services messages translation function from the Ruby
 * environment (ZOO-API)
 *
 * @param argc the number of parameters
 * @param argv the parameter values given from the Ruby environment
 * @param obj the Ruby object on which we run the method
 * @return a new Ruby string containing the translated value
 * @see _ss
 */
VALUE
RubyTranslate(int argc, VALUE *argv, VALUE obj)
{
  return rb_str_new2(_ss(StringValueCStr(argv[0])));
}

/**
 * Update the ongoing status of a running service from the Ruby environment
 * (ZOO-API)
 *
 * @param argc the number of parameters
 * @param argv the parameter values given from the Ruby environment
 * @param obj the Ruby object on which we run the method
 * @return a new Ruby string containing the translated value
 * @see _updateStatus
 */
VALUE
RubyUpdateStatus(int argc, VALUE *argv, VALUE obj)
{
  maps* conf;
  VALUE confdict=argv[0];
  int istatus=argv[1];
  char* status;
  if (istatus < 0 || istatus > 100){
    fprintf(stderr,"Status must be a percentage.");
    return Qnil;
  }else{
     char tmpStatus[4];
     snprintf(tmpStatus, 4, "%i", istatus);
     status = zStrdup(tmpStatus);
  }
  /* now update the map */
  {
    VALUE lenv = rb_hash_aref(confdict,rb_str_new2("lenv"));
    if(TYPE(lenv)!=T_NIL){
      VALUE valobj=rb_str_new2(status);
      rb_hash_aset(lenv,rb_str_new2("status"),valobj);
    }
  }
  conf = mapsFromRubyHash(confdict);
  if (getMapFromMaps(conf,"lenv","status") != NULL){
    fprintf(stderr,"STATUS RETURNED : %s\n",status);
    if(status!=NULL){
      setMapInMaps(conf,"lenv","status",status);
      free(status);
    }
    else
      setMapInMaps(conf,"lenv","status","15");
    _updateStatus(conf);
  }
  freeMaps(&conf);
  free(conf);
  return Qnil;
}

