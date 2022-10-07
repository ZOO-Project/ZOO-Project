/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2014 GeoLabs SARL
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
#define	PyString_Size PyUnicode_GetSize
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
  tmp = PyInt_FromLong(3);
  PyDict_SetItemString(d, "SERVICE_SUCCEEDED", tmp);
  Py_DECREF(tmp);

  tmp = PyInt_FromLong(4);
  PyDict_SetItemString(d, "SERVICE_FAILED", tmp);
  Py_DECREF(tmp);

  tmp = PyString_FromString(ZOO_VERSION);
  PyDict_SetItemString(d, "VERSION", tmp);
  Py_DECREF(tmp);

  ZooError = PyErr_NewException((char*)"zoo.error", NULL, NULL);
  Py_INCREF(ZooError);
  PyModule_AddObject(module, "error", ZooError);
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
	  libPath = "";
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
    //pythonpath=(char*)malloc((4+strlen(python_path)+strlen(ntmp)+strlen(tmp->value))*sizeof(char));
	  pythonpath = (char*)malloc((5 + strlen(python_path) + strlen(ntmp) + strlen(tmp->value) + strlen(libPath)) * sizeof(char));
#ifdef WIN32
  //sprintf(pythonpath,"%s/%s/;%s",ntmp,tmp->value,python_path);
	sprintf(pythonpath, "%s/%s/;%s;%s", ntmp, tmp->value, python_path, libPath);
#else
  //sprintf(pythonpath,"%s/%s/:%s",ntmp,tmp->value,python_path);
  sprintf(pythonpath, "%s/%s/:%s:%s", ntmp, tmp->value, python_path, libPath);  
#endif
  }
  else{
    //pythonpath=(char*)malloc((2+strlen(python_path)+strlen(ntmp))*sizeof(char));
	  pythonpath = (char*)malloc((3 + strlen(python_path) + strlen(ntmp) + strlen(libPath)) * sizeof(char));
#ifdef WIN32
    //sprintf(pythonpath,"%s;%s",ntmp,python_path);
	sprintf(pythonpath, "%s;%s;%s", ntmp, python_path, libPath);
#else
    //sprintf(pythonpath,"%s:%s",ntmp,python_path);
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
  //= getMapFromMaps(*main_conf, "env", "PYTHONHOME"); 
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
  PyEval_InitThreads();
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
    errorException (m, "Unable to parse serviceProvider please check your zcfg file.", "NoApplicableCode", NULL);
    exit(-1);
  } 

  pModule = PyImport_Import(pName);
  Py_DECREF(pName);
  int res=SERVICE_FAILED;
  if (pModule != NULL) {
    pFunc=PyObject_GetAttrString(pModule,s->name);
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
	freeMaps(real_outputs);
	free(*real_outputs);
	freeMaps(main_conf);
	free(*main_conf);
	*main_conf=mapsFromPyDict(arg1);
	*real_outputs=mapsFromPyDict(arg3);	
#ifdef DEBUG
	fprintf(stderr,"Result of call: %i\n", PyInt_AsLong(pValue));
	dumpMaps(inputs);
	dumpMaps(*real_outputs);
#endif
      }else{
	PythonZooReport(m,tmp->value,0);
	res=-1;
      }
    }
    else{
      char tmpS[1024];
      sprintf(tmpS, "Cannot find the %s function in the %s file.\n", s->name, tmp->value);
      errorException(m,tmpS,"NoApplicableCode",NULL);
      res=-1;
    }
  } else{
    PythonZooReport(m,tmp->value,1);
    res=-1;
  } 
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
 * @param m the conf maps containing the main.cfg settings
 * @param module the service name
 * @param load 1 if the Python module was not loaded yet
 */
void PythonZooReport(maps* m,const char* module,int load){
  PyObject *pName, *pModule, *pFunc;
  PyObject *ptype, *pvalue, *ptraceback,*pValue,*pArgs;
  PyErr_Fetch(&ptype, &pvalue, &ptraceback);
#if PY_MAJOR_VERSION >= 3  
  const
#endif
    char *pStrErrorMessage = PyString_AsString(pvalue);
  const char *tmp0=_("Python module %s cannot be loaded. Message: %s\n");
  
  PyObject *trace=PyObject_Str(pvalue);
  char *pbt=NULL;
  if(trace!=NULL && PyString_Check(trace)){
    pbt=(char*)malloc((8+strlen(PyString_AsString(trace)))*sizeof(char));
    sprintf(pbt,"TRACE: %s",PyString_AsString(trace));
  }
  else
    fprintf(stderr,"EMPTY TRACE ?");

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
    fprintf(stderr,"EMPTY TRACE ?");
  
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
  if(load>0 && pbt!=NULL){
    char *tmpS=(char*)malloc((strlen(tmp0)+strlen(module)+strlen(pbt)+1)*sizeof(char));
    sprintf(tmpS,tmp0,module,pbt);
    errorException(m,tmpS,"NoApplicableCode",NULL);
    free(tmpS);
  }else{
    if(pbt!=NULL)
      errorException(m,pbt,"NoApplicableCode",NULL);
    else
      errorException(m,_("Something went wrong but, no Python traceback can be fecthed."),"NoApplicableCode",NULL);
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
  map* tmp=t;
  int hasSize=0;
  map* isArray=getMap(tmp,"isArray");
  map* size=getMap(tmp,"size");
  map* useFile=getMap(tmp,"use_file");
  map* cacheFile=getMap(tmp,"cache_file");
  map* tmap=getMapType(tmp);
  while(tmp!=NULL){
    PyObject* name=PyString_FromString(tmp->name);
    if(strcasecmp(tmp->name,"value")==0) {
      if(isArray!=NULL){
	map* len=getMap(tmp,"length");
	int cnt=atoi(len->value);
	PyObject* value=PyList_New(cnt);
	PyObject* mvalue=PyList_New(cnt);
	PyObject* svalue=PyList_New(cnt);
	PyObject* cvalue=PyList_New(cnt);

	for(int i=0;i<cnt;i++){
	  
	  map* vMap=getMapArray(t,"value",i);
	  map* uMap=getMapArray(t,"use_file",i);
	  map* sMap=getMapArray(t,"size",i);
	  map* cMap=getMapArray(t,"cache_file",i);

	  if(vMap!=NULL){
	    
	    PyObject* lvalue;
	    PyObject* lsvalue;
	    PyObject* lcvalue;
	    if(sMap==NULL || uMap!=NULL){
	      lvalue=PyString_FromString(vMap->value);
	    }
	    else{
	      if(strlen(vMap->value)>0)
		lvalue=PyString_FromStringAndSize(vMap->value,atoi(sMap->value));
	      else
		lvalue=Py_None;
	    }
	    if(sMap!=NULL){
	      lsvalue=PyString_FromString(sMap->value);
	      hasSize=1;
	    }
	    else
	      lsvalue=Py_None;
	    if(uMap!=NULL){
	      lcvalue=PyString_FromString(cMap->value);;
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
	  
	  if(tmap!=NULL){
	    PyObject* lmvalue;
	    map* mMap=getMapArray(tmp,tmap->name,i);
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

	if(tmap!=NULL && PyDict_SetItem(res,PyString_FromString(tmap->name),mvalue)<0){
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
      else if(size!=NULL && useFile==NULL){
	PyObject* value;
	if(strlen(tmp->value)>0)
	  value=PyString_FromStringAndSize(tmp->value,atoi(size->value));
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
	PyObject* value=PyString_FromString(tmp->value);
	if(PyDict_SetItem(res,name,value)<0){
	  Py_DECREF(value);
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	Py_DECREF(value);
      }
    }
    else{
      if(PyDict_GetItem(res,name)==NULL){
	PyObject* value=PyString_FromString(tmp->value);
	if(PyDict_SetItem(res,name,value)<0){
	  Py_DECREF(value);
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	Py_DECREF(value);
      }
    }
    Py_DECREF(name);
    tmp=tmp->next;
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
    fprintf(stderr,">> DEBUG VALUES : %s => %s\n",
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
    cursor->next=NULL;
    if(res==NULL)
      res=dupMaps(&cursor);
    else
      addMapsToMaps(&res,cursor);
    freeMap(&cursor->content);
    free(cursor->content);
    free(cursor);
#ifdef DEBUG
    dumpMaps(res);
    fprintf(stderr,">> parsed maps %d\n",i);
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
	  if(PyString_Size(value)>0)
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
     snprintf(tmpStatus, 4, "%i", istatus);
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
