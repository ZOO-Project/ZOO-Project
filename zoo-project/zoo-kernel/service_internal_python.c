/**
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

static PyObject* ZooError;

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

  ZooError = PyErr_NewException("zoo.error", NULL, NULL);
  Py_INCREF(ZooError);
  PyModule_AddObject(module, "error", ZooError);
#if PY_MAJOR_VERSION >= 3
  return module;
#endif
}

int zoo_python_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  char *pythonpath;
  char *python_path;
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  map* tmp0=getMapFromMaps(*main_conf,"lenv","cwd");
  char *ntmp=tmp0->value;
  map* tmp=NULL;
  int hasToClean=0;
  tmp=getMapFromMaps(*main_conf,"env","PYTHONPATH");
#ifdef DEBUG
  fprintf(stderr,"PYTHON SUPPORT \n");
#endif
  if(tmp!=NULL){
#ifdef DEBUG
    fprintf(stderr,"PYTHON SUPPORT (%i)\n",strlen(tmp->value));
#endif
    python_path=(char*)malloc((strlen(tmp->value))*sizeof(char));
    sprintf(python_path,"%s",tmp->value);
    hasToClean=1;
  }
  else{
    python_path=".";
  }
  tmp=NULL;
  tmp=getMap(request,"metapath");
  if(tmp!=NULL && strcmp(tmp->value,"")!=0){
    pythonpath=(char*)malloc((4+strlen(python_path)+strlen(ntmp)+strlen(tmp->value))*sizeof(char));
#ifdef WIN32
  sprintf(pythonpath,"%s/%s/;%s",ntmp,tmp->value,python_path);
#else
  sprintf(pythonpath,"%s/%s/:%s",ntmp,tmp->value,python_path);
#endif
  }
  else{
    pythonpath=(char*)malloc((2+strlen(python_path)+strlen(ntmp))*sizeof(char));
#ifdef WIN32
    sprintf(pythonpath,"%s;%s",ntmp,python_path);
#else
    sprintf(pythonpath,"%s:%s",ntmp,python_path);
#endif
  }
#ifdef DEBUG
    fprintf(stderr,"PYTHONPATH=%s\n",pythonpath);
#endif
#ifndef WIN32
  setenv("PYTHONPATH",pythonpath,1);
#else
  SetEnvironmentVariable("PYTHONPATH",pythonpath);
  char* toto=(char*)malloc((strlen(pythonpath)+12)*sizeof(char));
  sprintf(toto,"PYTHONPATH=%s",pythonpath);
  putenv(toto);
  free(toto);
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
  mainstate = PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();
  PyObject *pName, *pModule, *pFunc;
  tmp=getMap(s->content,"serviceProvider");
  if(tmp!=NULL)
    pName = PyString_FromString(tmp->value);
  else{
    map* err=createMap("text","Unable to parse serviceProvider please check your zcfg file.");
    addToMap(err,"code","NoApplicableCode");
    printExceptionReportResponse(m,err);
    exit(-1);
  }
  pModule = PyImport_Import(pName);
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
	PyObject *ptype,*pvalue, *ptraceback;
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	PyObject *trace=PyObject_Str(pvalue);
	char pbt[10240];
	if(PyString_Check(trace))
	  sprintf(pbt,"TRACE : %s",PyString_AsString(trace));
	else
	  fprintf(stderr,"EMPTY TRACE ?");
	trace=NULL;
	trace=PyObject_Str(ptype);
	if(PyString_Check(trace)){
	  char *tpbt=zStrdup(pbt);
	  sprintf(pbt,"%s\n%s\0",tpbt,PyString_AsString(trace));
	  free(tpbt);
	}
	else
	  fprintf(stderr,"EMPTY TRACE ?");
	
	char *tpbt=zStrdup(pbt);
	pName = PyString_FromString("traceback");
	pModule = PyImport_Import(pName);
	pArgs = PyTuple_New(1);
	PyTuple_SetItem(pArgs, 0, ptraceback);
	pFunc = PyObject_GetAttrString(pModule,"format_tb");
	pValue = PyObject_CallObject(pFunc, pArgs);
	trace=NULL;
	trace=PyObject_Str(pValue);
	if(PyString_Check(trace))
	  sprintf(pbt,"%s\nUnable to run your python process properly. Please check the following messages : %s",tpbt,PyString_AsString(trace));
	else
	  sprintf(pbt,"%s \n Unable to run your python process properly. Unable to provide any futher informations. %s",tpbt);
	free(tpbt);
	map* err=createMap("text",pbt);
	addToMap(err,"code","NoApplicableCode");
	printExceptionReportResponse(m,err);
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
  } else{
    char tmpS[1024];
    sprintf(tmpS, "Python module %s cannot be loaded.\n", tmp->value);
    map* tmps=createMap("text",tmpS);
    printExceptionReportResponse(m,tmps);
    if (PyErr_Occurred())
      PyErr_Print();
    PyErr_Clear();
    res=-1;
    //exit(-1);
  } 
#if PY_MAJOR_VERSION < 3
  PyGILState_Release(gstate);
  PyEval_ReleaseLock();
#endif
  PyThreadState_Swap(mainstate);
  Py_Finalize();
  return res;
}

PyDictObject* PyDict_FromMaps(maps* t){
  PyObject* res=PyDict_New( );
  maps* tmp=t;
  while(tmp!=NULL){
    PyObject* value=(PyObject*)PyDict_FromMap(tmp->content);
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

PyDictObject* PyDict_FromMap(map* t){
  PyObject* res=PyDict_New( );
  map* tmp=t;
  int hasSize=0;
  map* isArray=getMap(tmp,"isArray");
  map* size=getMap(tmp,"size");
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

	for(int i=0;i<cnt;i++){
	  
	  map* vMap=getMapArray(tmp,"value",i);	    
	  map* sMap=getMapArray(tmp,"size",i);

	  if(vMap!=NULL){
	    
	    PyObject* lvalue;
	    PyObject* lsvalue;
	    if(sMap==NULL){
	      lvalue=PyString_FromString(vMap->value);
	      lsvalue=Py_None;
	    }
	    else{    
	      lvalue=PyString_FromStringAndSize(vMap->value,atoi(sMap->value));
	      lsvalue=PyString_FromString(sMap->value);
	      hasSize=1;
	    }

	    if(PyList_SetItem(value,i,lvalue)<0){
	      fprintf(stderr,"Unable to set key value pair...");
	      return NULL;
	    } 
	    if(PyList_SetItem(svalue,i,lsvalue)<0){
	      fprintf(stderr,"Unable to set key value pair...");
	      return NULL;
	    } 
	  }
	  
	  map* mMap=getMapArray(tmp,tmap->name,i);
	  PyObject* lmvalue;
	  if(mMap!=NULL){
	    lmvalue=PyString_FromString(mMap->value);
	  }else
	    lmvalue=Py_None;
	  
	  if(PyList_SetItem(mvalue,i,lmvalue)<0){
	      fprintf(stderr,"Unable to set key value pair...");
	      return NULL;
	  } 
	  
	}

	if(PyDict_SetItem(res,name,value)<0){
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	if(PyDict_SetItem(res,PyString_FromString(tmap->name),mvalue)<0){
	  fprintf(stderr,"Unable to set key value pair...");
	  return NULL;
	}
	if(hasSize>0)
	  if(PyDict_SetItem(res,PyString_FromString("size"),svalue)<0){
	    fprintf(stderr,"Unable to set key value pair...");
	    return NULL;
	  }
      }
      else if(size!=NULL){
	PyObject* value=PyString_FromStringAndSize(tmp->value,atoi(size->value));
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

maps* mapsFromPyDict(PyDictObject* t){
  maps* res=NULL;
  maps* cursor=res;
  PyObject* list=PyDict_Keys((PyObject*)t);
  int nb=PyList_Size(list);
  int i;
  for(i=0;i<nb;i++){
#ifdef DEBUG
    fprintf(stderr,">> parsing maps %d\n",i);
#endif
    PyObject* key=PyList_GetItem(list,i);
    PyObject* value=PyDict_GetItem((PyObject*)t,key);
#ifdef DEBUG
    fprintf(stderr,">> DEBUG VALUES : %s => %s\n",
	    PyString_AsString(key),PyString_AsString(value));
#endif
    cursor=(maps*)malloc(MAPS_SIZE);
    cursor->name=PyString_AsString(key);
    cursor->content=mapFromPyDict((PyDictObject*)value);
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
    Py_DECREF(key);
#ifdef DEBUG
    dumpMaps(res);
    fprintf(stderr,">> parsed maps %d\n",i);
#endif
  }
  return res;
}

map* mapFromPyDict(PyDictObject* t){
  map* res=NULL;
  PyObject* list=PyDict_Keys((PyObject*)t);
  int nb=PyList_Size(list);
  int i;
  for(i=0;i<nb;i++){
    PyObject* key=PyList_GetItem(list,i);
    PyObject* value=PyDict_GetItem((PyObject*)t,key);
#ifdef DEBUG
    fprintf(stderr,">> DEBUG VALUES : %s => %s\n",
	    PyString_AsString(key),PyString_AsString(value));
#endif
    
    if(strcmp(PyString_AsString(key),"value")==0){
      char *buffer=NULL;
      Py_ssize_t size;
#if PY_MAJOR_VERSION >= 3
      buffer=_PyUnicode_AsStringAndSize(value,&size);
#else
      PyString_AsStringAndSize(value,&buffer,&size);
#endif
      if(res!=NULL){
	addToMap(res,PyString_AsString(key),"");
      }else{
	res=createMap(PyString_AsString(key),"");
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
      char* lkey=PyString_AsString(key);
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
  return res;
}

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

PyObject*
PythonUpdateStatus(PyObject* self, PyObject* args)
{
  maps* conf;
  PyObject* confdict;
  int istatus;
  char* status;
  if (!PyArg_ParseTuple(args, "O!i", &PyDict_Type, &confdict, &istatus)){
#ifdef DEBUG
    fprintf(stderr,"Incorrect arguments to update status function");
#endif
    return NULL;
  }
  if (istatus < 0 || istatus > 100){
     PyErr_SetString(ZooError, "Status must be a percentage.");
     return NULL;
  }else{
     char tmpStatus[4];
     snprintf(tmpStatus, 4, "%i", istatus);
     status = zStrdup(tmpStatus);
  }
  /* now update the map */
  {
    PyObject* lenv = PyMapping_GetItemString(confdict, "lenv");
    if (lenv && PyMapping_Check(lenv)){
      PyObject* valobj = PyString_FromString(status);
      PyMapping_SetItemString(lenv, "status", valobj);
      Py_DECREF(valobj);
    }
    Py_DECREF(lenv);
  }
  conf = mapsFromPyDict((PyDictObject*)confdict);
  if (getMapFromMaps(conf,"lenv","status") != NULL){
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
  Py_RETURN_NONE;
}
