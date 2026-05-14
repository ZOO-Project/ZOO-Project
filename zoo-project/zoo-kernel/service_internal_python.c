/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2023 GeoLabs SARL
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

#include "service_internal_python.h"

/**
 * The state for the zoo Python module
 */
struct module_state {
   PyObject *error;
};

#if PY_MAJOR_VERSION >= 3
#define GETSTATE(m) ((struct module_state*)PyModule_GetState(m))
#define PyInt_FromLong PyLong_FromLong
#define PyInt_AsLong PyLong_AsLong
#define PyString_FromString PyUnicode_FromString
#define PyString_FromStringAndSize PyUnicode_FromStringAndSize
#define PyString_Check PyUnicode_Check
#define	PyString_AsString _PyUnicode_AsString
#define	PyString_Size PyUnicode_GET_LENGTH
#else
#define GETSTATE(m) (&_state)
static struct module_state _state;
#endif

/**
 * The exception for the zoo Python module
 */
static PyObject* ZooError;

/**
 * Function definitions for the zoo Python Module
 *
 * Define the following functions available from a service loaded and running
 * from the ZOO-Kernel Python environment:
 *  - "_" corresponding to the PythonTranslate function
 *  - "update_status" corresponding to the PythonUpdateStatus function
 * @see PythonTranslate, PythonUpdateStatus
 */
PyMethodDef zooMethods[] = {
  {"_", PythonTranslate, METH_VARARGS, "Translate a string using the zoo-services textdomain."},
  {"update_status", PythonUpdateStatus, METH_VARARGS, "Update status percentage of a running process."},
  {"trace", PythonTraceMessage, METH_VARARGS, "Equivalent to logger.trace()."},
  {"debug", PythonDebugMessage, METH_VARARGS, "Equivalent to logger.debug()."},
  {"info", PythonInfoMessage, METH_VARARGS, "Equivalent to logger.info()."},
  {"success", PythonSuccessMessage, METH_VARARGS, "Equivalent to logger.success()."},
  {"warning", PythonWarningMessage, METH_VARARGS, "Equivalent to logger.warning()."},
  {"error", PythonErrorMessage, METH_VARARGS|METH_COEXIST, "Equivalent to logger.error()."},
  {"critical", PythonCriticalMessage, METH_VARARGS, "Equivalent to logger.critical()."},
  {NULL, NULL, 0, NULL} /* tempt not the blade, all fear the sentinel */
};

#if PY_MAJOR_VERSION >= 3

static int myextension_traverse(PyObject *m, visitproc visit, void *arg) {
  Py_VISIT(GETSTATE(m)->error);
  return 0;
}

static int myextension_clear(PyObject *m) {
  Py_CLEAR(GETSTATE(m)->error);
  return 0;
}

static struct PyModuleDef moduledef = {
  PyModuleDef_HEAD_INIT,
  "zoo",
  NULL,
  sizeof(struct module_state),
  zooMethods,
  NULL,
  myextension_traverse,
  myextension_clear,
  NULL
};
#endif

/**
 * Function to create and initialize the zoo Python module
 *
 * @return the Python module (for Python versions < 3, nothing for version >=3)
 */
PyMODINIT_FUNC init_zoo(){
  PyObject *tmp,*d;
  PyObject *module = 
#if PY_MAJOR_VERSION >= 3
    PyModule_Create(&moduledef);
#else
    Py_InitModule("zoo", zooMethods);
#endif
  if (module == NULL){
#if PY_MAJOR_VERSION >= 3
    return NULL;
#else
    return;
#endif
  }

  struct module_state *st = GETSTATE(module);

  d = PyModule_GetDict(module);
  int iCnt=0;
  // Define the ZOO-Kernel status values
  for(;ZOO_STATUS[iCnt]!=NULL;iCnt++){
    tmp = PyInt_FromLong(iCnt);
    PyDict_SetItemString(d, ZOO_STATUS[iCnt], tmp);
    Py_DECREF(tmp);
  }

  tmp = PyString_FromString(ZOO_VERSION);
  PyDict_SetItemString(d, "VERSION", tmp);
  Py_DECREF(tmp);

  ZooError = PyErr_NewException((char*)"zoo.exception", NULL, NULL);
  Py_INCREF(ZooError);
  PyModule_AddObject(module, "exception", ZooError);
#if PY_MAJOR_VERSION >= 3
  return module;
#endif
}

/**
 * Load a Python module then run the function corresponding to the service
 * by passing the conf, inputs and outputs parameters by reference. 
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 */
int zoo_python_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){

  char *pythonpath;
  char *python_path;

#ifdef WIN32
  const char* os_pathsep = ";";
#else
  const char* os_pathsep = ":";
#endif

  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  map* tmp0=getMapFromMaps(*main_conf,"lenv","cwd");
  char *ntmp=tmp0->value;  
  map* tmp=NULL;
  int hasToClean=0;
  tmp=getMapFromMaps(*main_conf,"env","PYTHONPATH"); 

  map* kvp = NULL;
  char* libPath = NULL;
  if (hasvalue(*main_conf, "main", "libPath", &kvp)) {
	  libPath = kvp->value;
  }
  else {
    libPath = (char*) "";
  }

#ifdef DEBUG
  fprintf(stderr,"PYTHON SUPPORT \n");
#endif
  if(tmp!=NULL){
#ifdef DEBUG
    fprintf(stderr,"PYTHON SUPPORT (%i)\n",strlen(tmp->value));
#endif
    python_path=(char*)malloc((strlen(tmp->value)+1)*sizeof(char));
    sprintf(python_path,"%s",tmp->value);
    hasToClean=1;
  }
  else{
    map* cwdMap=getMapFromMaps(*main_conf,"main","servicePath");
    if(cwdMap!=NULL)
      python_path=cwdMap->value;
    else
      python_path=(char*)".";	
  }
  tmp=NULL;
  tmp=getMap(request,"metapath");
  if(tmp!=NULL && strcmp(tmp->value,"")!=0){
	  pythonpath = (char*)malloc((5 + strlen(python_path) + strlen(ntmp) + strlen(tmp->value) + strlen(libPath)) * sizeof(char));
#ifdef WIN32
	sprintf(pythonpath, "%s/%s/;%s;%s", ntmp, tmp->value, python_path, libPath);
#else
  sprintf(pythonpath, "%s/%s/:%s:%s", ntmp, tmp->value, python_path, libPath);  
#endif
  }
  else{
	  pythonpath = (char*)malloc((3 + strlen(python_path) + strlen(ntmp) + strlen(libPath)) * sizeof(char));
#ifdef WIN32
	sprintf(pythonpath, "%s;%s;%s", ntmp, python_path, libPath);
#else
	sprintf(pythonpath, "%s:%s:%s", ntmp, python_path, libPath);
#endif	
  }
#ifdef DEBUG
    fprintf(stderr,"PYTHONPATH=%s\n",pythonpath);
#endif
	map* home = NULL;
// knut: also set PYTHONHOME environment variable so that Python can load standard modules
#ifndef WIN32
  setenv("PYTHONPATH",pythonpath,1);  
  if (hasvalue(*main_conf, "env", "PYTHONHOME", &home)) {
	  setenv("PYTHONHOME", home->value, 1); // overwrite
  }
#else
  SetEnvironmentVariable("PYTHONPATH",pythonpath);
  char* toto=(char*)malloc((strlen(pythonpath)+12)*sizeof(char));
  sprintf(toto,"PYTHONPATH=%s",pythonpath);
  _putenv(toto);
  free(toto);
  if (hasvalue(*main_conf, "env", "PYTHONHOME", &home)) { 
	  SetEnvironmentVariable("PYTHONHOME", home->value);
  }
  char buffer[128];
#endif
  if(hasToClean>0)
    free(python_path);
  free(pythonpath);

  PyThreadState *mainstate;
#if PY_MAJOR_VERSION >= 3  
  PyImport_AppendInittab("zoo", init_zoo);
#else
  PyEval_InitThreads();
#endif
  Py_Initialize();
#if PY_MAJOR_VERSION >= 3
#if PY_MINOR_VERSION < 9
  PyEval_InitThreads();
#endif
  PyImport_ImportModule("zoo");
#else
  init_zoo();
#endif
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION < 10
  mainstate = PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
#else
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 10
  mainstate = PyEval_SaveThread();
#else
  mainstate = PyThreadState_Swap(NULL);
#endif
#endif
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  PyObject *pName, *pModule, *pFunc;
  tmp=getMap(s->content,"serviceProvider");
  map* mp=getMap(request,"metapath");
  if(tmp!=NULL){
    if(mp!=NULL && strlen(mp->value)>0){
      char *mps=zStrdup(mp->value);
      int i,len=strlen(mps);
      int j=0;
      for(i=0;i<len;i++){
        if(mps[i]=='/'){
          mps[i]='.';
        }
      }
      char *mn=(char*)malloc((strlen(mps)+strlen(tmp->value)+2)*sizeof(char));
      sprintf(mn,"%s.%s",mps,tmp->value);
      pName = PyString_FromString(mn);
      free(mn);
      free(mps);
    }
    else{
      pName = PyString_FromString(tmp->value);
    }
  }
  else{
    errorException (main_conf, "Unable to parse serviceProvider please check your zcfg file.", "NoApplicableCode", NULL);
    return -1;
  } 

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);
  int res=SERVICE_FAILED;
  char* pcaServiceName=zStrdup(s->name);
  _translateChar(pcaServiceName,'-','_');
  if (pModule != NULL) {
    pFunc=PyObject_GetAttrString(pModule,pcaServiceName);
    if (pFunc && PyCallable_Check(pFunc)){
      PyObject *pValue;
      PyDictObject* arg1=PyDict_FromMaps(m);
      PyDictObject* arg2=PyDict_FromMaps(inputs);
      PyDictObject* arg3=PyDict_FromMaps(outputs);
      PyObject *pArgs=PyTuple_New(3);
      if (!pArgs)
        return -1;
      PyTuple_SetItem(pArgs, 0, (PyObject *)arg1);
      PyTuple_SetItem(pArgs, 1, (PyObject *)arg2);
      PyTuple_SetItem(pArgs, 2, (PyObject *)arg3);
      pValue = PyObject_CallObject(pFunc, pArgs);
      if (pValue != NULL) {
        res=PyInt_AsLong(pValue);
        bool isNotNull=false;
        freeMaps(&m);
        free(m);
        *main_conf=mapsFromPyDict(arg1);
        if(*real_outputs!=NULL && arg3!=(PyDictObject*) Py_None){
          freeMaps(real_outputs);
          free(*real_outputs);
          isNotNull=true;
          *real_outputs=mapsFromPyDict(arg3);
        }
#ifdef DEBUG
        fprintf(stderr,"Result of call: %i\n", PyInt_AsLong(pValue));
        dumpMaps(inputs);
        dumpMaps(*real_outputs);
#endif
      }else{
        PythonZooReport(main_conf,tmp->value,0);
        res=-1;
      }
    }
    else{
      char tmpS[1024];
      sprintf(tmpS, "Cannot find the %s function in the %s file.\n", s->name, tmp->value);
      errorException(main_conf,tmpS,"NoApplicableCode",NULL);
      res=-1;
    }
  } else{
    PythonZooReport(main_conf,tmp->value,1);
    res=-1;
  }
  free(pcaServiceName);
#if PY_MAJOR_VERSION < 3
  PyGILState_Release(gstate);
  PyEval_AcquireLock();
#endif
  PyThreadState_Swap(mainstate);
  Py_Finalize();
  return res;
}

/**
 * Report Python error which may occur on loading the Python module or at 
 * runtime.
 * 
 * @param main_conf the conf maps containing the main.cfg settings
 * @param module the service name
 * @param load 1 if the Python module was not loaded yet
 */
void PythonZooReport(maps** main_conf,const char* module,int load){
  maps* m=*main_conf;
  PyObject *pName, *pModule, *pFunc;
  PyObject *ptype, *pvalue, *ptraceback,*pValue,*pArgs;
  PyErr_Fetch(&ptype, &pvalue, &ptraceback);
#if PY_MAJOR_VERSION >= 3  
  const
#endif
    char *pStrErrorMessage = PyString_AsString(pvalue);
  const char *tmp0=_("Python module %s cannot be loaded. Message: %s\n");
  
  iZooLogLevel=ZOO_DEBUG_LEVEL_ERROR;
  PyObject *trace=PyObject_Str(pvalue);
  char *pbt=NULL;
  if(trace!=NULL && PyString_Check(trace)){
    pbt=(char*)malloc((8+strlen(PyString_AsString(trace)))*sizeof(char));
    sprintf(pbt,"TRACE: %s",PyString_AsString(trace));
  }
  else
    ZOO_DEBUG("EMPTY TRACE ?");

  trace=NULL;
  trace=PyObject_Str(ptype);
  if(trace!=NULL && PyString_Check(trace) && pbt!=NULL){
    char *tpbt=zStrdup(pbt);
    if(pbt!=NULL)
      free(pbt);
    pbt=(char*)malloc((1+strlen(tpbt)+strlen(PyString_AsString(trace))+1)*sizeof(char));
    sprintf(pbt,"%s\n%s",tpbt,PyString_AsString(trace));
    free(tpbt);
  }
  else
    ZOO_DEBUG("EMPTY TRACE ?");
  
  if(ptraceback!=NULL && pbt!=NULL){
    char *tpbt=zStrdup(pbt);
    pName = PyString_FromString("traceback");
    pModule = PyImport_Import(pName);
    pArgs = PyTuple_New(1);
    PyTuple_SetItem(pArgs, 0, ptraceback);
    pFunc = PyObject_GetAttrString(pModule,"format_tb");
    pValue = PyObject_CallObject(pFunc, pArgs);
    trace=NULL;
    trace=PyObject_Str(pValue);
    if(PyString_Check(trace)){
      if(pbt!=NULL)
        free(pbt);
      const char* format=_("%s\nUnable to run your python process properly. Please check the following messages : %s");
      pbt=(char*)malloc((strlen(format)+strlen(tpbt)+strlen(PyString_AsString(trace))+1)*sizeof(char));
      sprintf(pbt,format,tpbt,PyString_AsString(trace));
    }
    else{
      if(pbt!=NULL)
        free(pbt);
      const char* format=_("%s \n Unable to run your python process properly. Unable to provide any further information.");
      pbt=(char*)malloc((strlen(format)+strlen(tpbt)+strlen(PyString_AsString(trace))+1)*sizeof(char));
      sprintf(pbt,format,tpbt);
    }
    free(tpbt);
  }
  // Set conf["lenv"]["trace"]="true"
  setMapInMaps(m,"lenv","trace","true");
  if(load>0 && pbt!=NULL){
    char *tmpS=(char*)malloc((strlen(tmp0)+strlen(module)+strlen(pbt)+1)*sizeof(char));
    sprintf(tmpS,tmp0,module,pbt);
    errorException(main_conf,tmpS,"NoApplicableCode",NULL);
    free(tmpS);
  }else{
    if(pbt!=NULL)
      errorException(main_conf,pbt,"NoApplicableCode",NULL);
    else
      errorException(main_conf,_("Something went wrong but, no Python traceback can be fecthed."),"NoApplicableCode",NULL);
  }
  if(pbt!=NULL)
    free(pbt);
}

/**
 * Convert a maps to a Python dictionary
 *
 * @param t the maps to convert
 * @return a new PyDictObject containing the converted maps
 * @see PyDict_FromMap
 * @warning make sure to free resources returned by this function
 */
PyDictObject* PyDict_FromMaps(maps* t){
  if(t==NULL)
    return Py_INCREF(Py_None),(PyDictObject*) Py_None;
  PyObject* res=PyDict_New( );
  maps* tmp=t;
  while(tmp!=NULL){
    PyObject* value=(PyObject*)PyDict_FromMap(tmp->content);
    if(tmp->child!=NULL){
      PyObject* cname=PyString_FromString("child");
      PyObject* childs=(PyObject*)PyDict_FromMaps(tmp->child);
      if(PyDict_SetItem(value,cname,childs)<0){
        fprintf(stderr,"Unable to set map value ...");
        return NULL;
      } 
      Py_DECREF(cname);
    }
    PyObject* name=PyString_FromString(tmp->name);
    if(PyDict_SetItem(res,name,value)<0){
      fprintf(stderr,"Unable to set map value ...");
      return NULL;
    }
    Py_DECREF(name);
    tmp=tmp->next;
  }
  return (PyDictObject*) res;
}

/**
 * Convert a map to a Python dictionary
 *
 * @param t the map to convert
 * @return a new PyDictObject containing the converted maps
 * @warning make sure to free resources returned by this function
 */
PyDictObject* PyDict_FromMap(map* t){
  PyObject* res=PyDict_New( );
  map* pmTmp=t;
  int hasSize=0;
  map* pmIsArray=getMap(pmTmp,"isArray");
  map* pmSize=getMap(pmTmp,"size");
  map* pmUseFile=getMap(pmTmp,"use_file");
  map* pmCacheFile=getMap(pmTmp,"cache_file");
  map* pmType=getMapType(pmTmp);
  while(pmTmp!=NULL){
    PyObject* name=PyString_FromString(pmTmp->name);
    if(strcasecmp(pmTmp->name,"value")==0) {
      if(pmIsArray!=NULL){
	map* len=getMap(pmTmp,"length");
	int cnt=1;
	if(len!=NULL)
	  cnt=atoi(len->value);
	PyObject* value=PyList_New(cnt);
	PyObject* mvalue=PyList_New(cnt);
	PyObject* svalue=PyList_New(cnt);
	PyObject* cvalue=PyList_New(cnt);

	for(int i=0;i<cnt;i++){

	  map* pmValue=getMapArray(t,"value",i);
	  pmUseFile=getMapArray(t,"use_file",i);
	  pmSize=getMapArray(t,"size",i);
	  pmCacheFile=getMapArray(t,"cache_file",i);

	  if(pmValue!=NULL){
	    
	    PyObject* lvalue;
	    PyObject* lsvalue;
	    PyObject* lcvalue;
	    if(pmSize==NULL || pmUseFile!=NULL){
	      lvalue=PyString_FromString(pmValue->value);
	    }
	    else{
	      if(pmSize!=NULL && strlen(pmValue->value)>0)
		lvalue=PyString_FromStringAndSize(pmValue->value,atoi(pmSize->value));
	      else
		lvalue=Py_None;
	    }
	    if(pmSize!=NULL){
	      lsvalue=PyString_FromString(pmSize->value);
	      hasSize=1;
	    }
	    else
	      lsvalue=Py_None;
	    if(pmCacheFile!=NULL){
	      lcvalue=PyString_FromString(pmCacheFile->value);;
	    }else
	      lcvalue=Py_None;
	    if(PyList_SetItem(value,i,lvalue)<0){
	      fprintf(stderr,"Unable to set key value pair...");
	      return NULL;
	    } 
	    if(PyList_SetItem(svalue,i,lsvalue)<0){
	      fprintf(stderr,"Unable to set key value pair...");
	      return NULL;
	    } 
	    if(PyList_SetItem(cvalue,i,lcvalue)<0){
	      fprintf(stderr,"Unable to set key value pair...");
	      return NULL;
	    }
	  }
	  
	  if(pmType!=NULL){
	    PyObject* lmvalue;
	    map* mMap=getMapArray(pmTmp,pmType->name,i);
	    if(mMap!=NULL){
	      lmvalue=PyString_FromString(mMap->value);
	    }else
	      lmvalue=Py_None;
	  
	    if(PyList_SetItem(mvalue,i,lmvalue)<0){
	      fprintf(stderr,"Unable to set key value pair...");
	      return NULL;
	    }
	  }
	}

	if(PyDict_SetItem(res,name,value)<0){
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}

	if(pmType!=NULL && PyDict_SetItem(res,PyString_FromString(pmType->name),mvalue)<0){
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	if(PyDict_SetItem(res,PyString_FromString("cache_file"),cvalue)<0){
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	if(hasSize>0)
	  if(PyDict_SetItem(res,PyString_FromString("size"),svalue)<0){
	    fprintf(stderr,"Unable to set key value pair...");
	    return NULL;
	  }
      }
      else if(pmSize!=NULL && pmUseFile==NULL){
	PyObject* value;
	if(strlen(pmTmp->value)>0)
	  value=PyString_FromStringAndSize(pmTmp->value,atoi(pmSize->value));
	else
	  value=Py_None;
	if(PyDict_SetItem(res,name,value)<0){
	  Py_DECREF(value);
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	Py_DECREF(value);
      }
      else{
	PyObject* value=PyString_FromString(pmTmp->value);
	if(PyDict_SetItem(res,name,value)<0){
	  Py_DECREF(value);
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	Py_DECREF(value);
      }
    }
    else{
      if(PyDict_GetItem(res,name)==NULL && pmTmp!=NULL && pmTmp->value!=NULL){
	PyObject* value=PyString_FromString(pmTmp->value);
	if(PyDict_SetItem(res,name,value)<0){
	  Py_DECREF(value);
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	Py_DECREF(value);
      }
    }
    Py_DECREF(name);
    pmTmp=pmTmp->next;
  }
  return (PyDictObject*) res;
}

/**
 * Convert a Python dictionary to a maps 
 *
 * @param t the PyDictObject to convert
 * @return a new maps containing the converted PyDictObject
 * @warning make sure to free resources returned by this function
 */
maps* mapsFromPyDict(PyDictObject* t){
  maps* res=NULL;
  maps* cursor=NULL;
  if(t==(PyDictObject*) Py_None)
    return NULL;
  PyObject* list=PyDict_Keys((PyObject*)t);
  int nb=PyList_Size(list);
  int i;
  PyObject* key;
  PyObject* value;
  for(i=0;i<nb;i++){
#ifdef DEBUG
    fprintf(stderr,">> parsing maps %d\n",i);
#endif
    key=PyList_GetItem(list,i);
    value=PyDict_GetItem((PyObject*)t,key);
#ifdef DEBUG
    fprintf(stderr,">> %s %d DEBUG VALUES : %s => %s\n",
	    __FILE__,__LINE__,
	    PyString_AsString(key),PyString_AsString(value));
#endif
    cursor=createMaps(PyString_AsString(key));
    cursor->content=mapFromPyDict((PyDictObject*)value);
    PyObject* cname=PyString_FromString("child");
    PyObject* childs=PyDict_GetItem((PyObject*)value,cname);
    if(childs!=NULL)
      cursor->child=mapsFromPyDict((PyDictObject*)childs);
    Py_DECREF(cname);
#ifdef DEBUG
    dumpMap(cursor->content);
#endif
    if(res==NULL)
      res=dupMaps(&cursor);
    else
      addMapsToMaps(&res,cursor);
    freeMaps(&cursor);
    free(cursor);
#ifdef DEBUG
    fprintf(stderr,">> parsed maps %d\n",i);
    dumpMaps(res);
    fprintf(stderr,">> %s %d parsed maps %d\n",__FILE__,__LINE__,i);
#endif
  }
  Py_DECREF(list);
  return res;
}

/**
 * Convert a Python dictionary to a maps 
 *
 * @param t the PyDictObject to convert
 * @return a new maps containing the converted PyDictObject
 * @warning make sure to free resources returned by this function
 */
maps* _mapsFromPyDict(PyDictObject* t){
	
	PyObject* list = PyDict_Keys((PyObject*)t); // new ref	
	int nb = PyList_Size(list);	
	
	if (nb < 1) {
		Py_DECREF(list);
		return NULL;
	}

	maps* ptr = (maps*) malloc(MAPS_SIZE);
	ptr->content = NULL;
	ptr->child = NULL;
	ptr->next = NULL;

	maps* res = ptr;	
	
	PyObject* key;
	PyObject* value;
	
	for(int i = 0; i < nb; i++) {
		
		key = PyList_GetItem(list,i); // borrowed ref
		value = PyDict_GetItem((PyObject*) t, key); // borrowed ref
		
		ptr->name = zStrdup(PyString_AsString(key));
		ptr->content = mapFromPyDict((PyDictObject*) value);
		
		ptr->next = i < nb - 1 ? (maps*) malloc(MAPS_SIZE) : NULL;
		ptr = ptr->next;
	}
	Py_DECREF(list);

	return res;
} // mapsFromPyDict

/**
 * Convert a Python dictionary to a map 
 *
 * @param t the PyDictObject to convert
 * @return a new map containing the converted PyDictObject
 * @warning make sure to free resources returned by this function
 */
map* mapFromPyDict(PyDictObject* t){
  map* res=NULL;
  PyObject* list=PyDict_Keys((PyObject*)t);
  if(list==(PyObject*) Py_None)
    return NULL;
  int nb=PyList_Size(list);
  int i;
  PyObject* key;
  PyObject* value;
  for(i=0;i<nb;i++){
    key=PyList_GetItem(list,i);
    value=PyDict_GetItem((PyObject*)t,key);
#ifdef DEBUG
    fprintf(stderr,">> DEBUG VALUES : %s => %s\n",
	    PyString_AsString(key),PyString_AsString(value));
#endif
    if(strncmp(PyString_AsString(key),"child",5)!=0){
      if(strncmp(PyString_AsString(key),"value",5)==0){
#if PY_MAJOR_VERSION >= 3  
	const
#endif
	  char *buffer=NULL;
	Py_ssize_t size;
#if PY_MAJOR_VERSION >= 3
	if(PyBytes_Check(value)){
	  size=PyBytes_Size(value);
	  buffer=PyBytes_AsString(value);
	}
	else
	  if(PyUnicode_Check(value) && PyUnicode_READY(value) == 0){
	    buffer=PyUnicode_AsUTF8AndSize(value,&size);
	  }
	  else{
#ifdef DEBUG
	    fprintf(stderr,"Unsupported return value.");
#endif
	    return NULL;
	  }
#else
	PyString_AsStringAndSize(value,&buffer,&size);
#endif      
	res = addToMapWithSize(res,PyString_AsString(key),buffer,size);
      }else{
#if PY_MAJOR_VERSION >= 3  
	const
#endif
	  char* lkey=PyString_AsString(key);
#if PY_MAJOR_VERSION >= 3  
	const
#endif
	  char* lvalue=PyString_AsString(value);
	if(res!=NULL){
	  if(PyUnicode_GetLength(value)>0)
	    addToMap(res,lkey,lvalue);
	}
	else{
	  if(PyString_Size(value)>0)
	    res=createMap(lkey,lvalue);
	}
      }
    }
  }
  Py_DECREF(list);
  return res;
}

/**
 * Convert a Python dictionary to a map 
 *
 * @param t the PyDictObject to convert
 * @return a new map containing the converted PyDictObject
 * @warning make sure to free resources returned by this function
 */
map* _mapFromPyDict(PyDictObject* t) {
	
  PyObject* list = PyDict_Keys((PyObject*) t); // new ref
  int nb = PyList_Size(list);
	
  if (nb < 1) {
    Py_DECREF(list);
    return NULL;
  }	
	
  map* ptr = (map*) malloc(MAP_SIZE);
  map* res = ptr;
	
  PyObject* key;
  PyObject* value;
#if PY_MAJOR_VERSION >= 3  
  const
#endif
    char *buffer = NULL;
  Py_ssize_t size;
  for(int i = 0; i < nb; i++) {
		
    key = PyList_GetItem(list, i); // borrowed ref
    value = PyDict_GetItem((PyObject*) t, key); // borrowed ref		
				
    ptr->name = zStrdup(PyString_AsString(key));		
    map* msize = NULL;
		
#if PY_MAJOR_VERSION >= 3
    if (PyBytes_Check(value)) {
      // value is byte array
      size = PyBytes_Size(value);				
      buffer = PyBytes_AsString(value); // pointer to internal buffer
      char sz[32];
      sprintf(sz, "%d", (int) size);	
      msize = createMap("size", sz);			
    }
    else if (PyUnicode_Check(value) && PyUnicode_READY(value) == 0) {
      // value is string object		
      buffer = PyUnicode_AsUTF8AndSize(value, &size);
      size++;
    }
    else {
      printf("Type not recognized\n");
      // error handling
      // ...
    }
#else	
    PyString_AsStringAndSize(value, &buffer, &size);
    size++;
    // to do: handle byte arrays
#endif
		
    ptr->value = (char*) malloc(size); // check for NULL pointer
    memmove(ptr->value, buffer, size);
			
    if (msize != NULL) {
      ptr->next = msize;
      ptr = ptr->next;
    }						
		
    ptr->next = i < nb - 1 ? (map*) malloc(MAP_SIZE) : NULL;
    ptr = ptr->next;
  }	
  Py_DECREF(list);
			
  return res;
} // mapFromPyDict

/**
 * Use the ZOO-Services messages translation function from the Python
 * environment
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonTranslate(PyObject* self, PyObject* args)
{
  char *str;
  if (!PyArg_ParseTuple(args, "s", &str)){
#ifdef DEBUG
    fprintf(stderr,"Incorrect arguments to update status function");
#endif
    return NULL;
  }
  return PyString_FromString(_ss(str));
}

/**
 * Update the ongoing status of a running service from the Python environment
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return None to the Python environment
 * @see _updateStatus
 */
PyObject*
PythonUpdateStatus(PyObject* self, PyObject* args)
{
  maps* conf;
  PyObject* confdict;
  int istatus;
  char* status=NULL;
  if (!PyArg_ParseTuple(args, "O!i", &PyDict_Type, &confdict, &istatus)){
#ifdef DEBUG
    fprintf(stderr,"Incorrect arguments to update status function");
#endif
    Py_RETURN_NONE;
  }
  if (istatus < 0 || istatus > 100){
     PyErr_SetString(ZooError, "Status must be a percentage.");
     Py_RETURN_NONE;
  }else{
     char tmpStatus[4];
     snprintf(tmpStatus, 4, "%d", istatus);
     status = zStrdup(tmpStatus);
  }
  // create a local copy and update the lenv map
  conf = mapsFromPyDict((PyDictObject*)confdict);
  if(status!=NULL){
    setMapInMaps(conf,"lenv","status",status);
    free(status);
  }
  else
    setMapInMaps(conf,"lenv","status","15");
  _updateStatus(conf);
  freeMaps(&conf);
  free(conf);
  Py_RETURN_NONE;
}

/**
 * ZOO-API debug message handling in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
#include <Python.h>
#if PY_VERSION_HEX < 0x030C0000  // For Python < 3.12
#include "frameobject.h"
#endif

// This function prints a debug message along with the module, function, and line number
// where it was called from. Only string arguments are supported.
PyObject*
PythonPrintDebugMessage(PyObject* self, PyObject* args)
{
    char *str;

    // Expecting a single string argument
    if (!PyArg_ParseTuple(args, "s", &str)) {
        fprintf(stderr, "Incorrect arguments to debug, only strings are supported\n");
        Py_RETURN_NONE;
    }

    // Get the current Python frame using the public API
#if PY_VERSION_HEX < 0x030C0000  // For Python < 3.12
    PyFrameObject* poFrame = PyEval_GetFrame();
    char const* pccModule = _PyUnicode_AsString(poFrame->f_code->co_filename);
    char const* pccFunction = _PyUnicode_AsString(poFrame->f_code->co_name);
    int iLine = PyFrame_GetLineNumber(poFrame);
#else  // For Python >= 3.12
    const char* pccModule = "unknown";
    const char* pccFunction = "unknown";
    int iLine = -1;
    PyThreadState* tstate = PyThreadState_Get();
    PyFrameObject* poFrame = tstate ? PyThreadState_GetFrame(tstate) : NULL;

    if (poFrame) {
        // Extract code object safely (don't access f_code directly)
        PyCodeObject* code = PyFrame_GetCode(poFrame);
        if (code) {
            // Make sure these are Unicode objects before converting to UTF-8
            if (code->co_filename && PyUnicode_Check(code->co_filename)) {
                pccModule = PyUnicode_AsUTF8(code->co_filename);
            }
            if (code->co_name && PyUnicode_Check(code->co_name)) {
                pccFunction = PyUnicode_AsUTF8(code->co_name);
            }
            Py_DECREF(code);  // Avoid memory leaks
        }

        // Get the current line number in the frame
        iLine = PyFrame_GetLineNumber(poFrame);
    }
#endif

    // Custom debug macro
    _ZOO_DEBUG(str, pccModule, pccFunction, iLine);

    Py_RETURN_NONE;
}

/**
 * ZOO-API equivalent to logger.debug() in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonTraceMessage(PyObject* self, PyObject* args)
{
  iZooLogLevel=ZOO_DEBUG_LEVEL_TRACE;
  return PythonPrintDebugMessage(self,args);
}
/**
 * ZOO-API equivalent to logger.debug() in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonDebugMessage(PyObject* self, PyObject* args)
{
  iZooLogLevel=ZOO_DEBUG_LEVEL_DEBUG;
  return PythonPrintDebugMessage(self,args);
}
/**
 * ZOO-API equivalent to logger.debug() in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonInfoMessage(PyObject* self, PyObject* args)
{
  iZooLogLevel=ZOO_DEBUG_LEVEL_INFO;
  return PythonPrintDebugMessage(self,args);
}
/**
 * ZOO-API equivalent to logger.debug() in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonSuccessMessage(PyObject* self, PyObject* args)
{
  iZooLogLevel=ZOO_DEBUG_LEVEL_SUCCESS;
  return PythonPrintDebugMessage(self,args);
}
/**
 * ZOO-API equivalent to logger.debug() in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonWarningMessage(PyObject* self, PyObject* args)
{
  iZooLogLevel=ZOO_DEBUG_LEVEL_WARNING;
  return PythonPrintDebugMessage(self,args);
}
/**
 * ZOO-API equivalent to logger.debug() in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonErrorMessage(PyObject* self, PyObject* args)
{
  iZooLogLevel=ZOO_DEBUG_LEVEL_ERROR;
  return PythonPrintDebugMessage(self,args);
}
/**
 * ZOO-API equivalent to logger.debug() in Python
 *
 * @param self the Python object on which we can run the method
 * @param args the Python arguments given from the Python environment
 * @return a new Python string containing the translated value
 * @see _ss
 */
PyObject*
PythonCriticalMessage(PyObject* self, PyObject* args)
{
  iZooLogLevel=ZOO_DEBUG_LEVEL_FATAL;
  return PythonPrintDebugMessage(self,args);
}
