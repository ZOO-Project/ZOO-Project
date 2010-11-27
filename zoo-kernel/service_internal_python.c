/**
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

#include "service_internal_python.h"

int zoo_python_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  char ntmp[1024];
  getcwd(ntmp,1024);
  map* tmp=NULL;
  tmp=getMapFromMaps(*main_conf,"env","PYTHONPATH");
  char *python_path;
  fprintf(stderr,"PYTHON SUPPORT \n");
  fflush(stderr);
  if(tmp!=NULL){
    fprintf(stderr,"PYTHON SUPPORT (%i)\n",strlen(tmp->value));
    python_path=(char*)malloc((strlen(tmp->value))*sizeof(char));
    sprintf(python_path,"%s",tmp->value);
  }
  else{
    python_path=strdup(".");
  }
  tmp=NULL;
  tmp=getMap(request,"metapath");
  char *pythonpath=(char*)malloc((1+strlen(python_path)+2048)*sizeof(char));
  if(tmp!=NULL && strcmp(tmp->value,"")!=0)
#ifdef WIN32
    sprintf(pythonpath,"%s/%s/;%s",ntmp,tmp->value,python_path);
#else
  sprintf(pythonpath,"%s/%s/:%s",ntmp,tmp->value,python_path);
#endif
  else
#ifdef WIN32
    sprintf(pythonpath,"%s;%s",ntmp,python_path);
#else
  sprintf(pythonpath,"%s:%s",ntmp,python_path);
#endif
  //#ifdef DEBUG
    fprintf(stderr,"PYTHONPATH=%s\n",pythonpath);
  //#endif
#ifndef WIN32
  setenv("PYTHONPATH",pythonpath,1);
#else
  SetEnvironmentVariable("PYTHONPATH",pythonpath);
#endif
  free(python_path);
  free(pythonpath);

  Py_Initialize();
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
  int i;
  int res=SERVICE_FAILED;
  int cpid=getpid();
  if (pModule != NULL) {
    pFunc=PyObject_GetAttrString(pModule,s->name);
    if (pFunc && PyCallable_Check(pFunc)){
      PyDictObject* arg1=PyDict_FromMaps(m);
      PyDictObject* arg2=PyDict_FromMaps(inputs);
      PyDictObject* arg3=PyDict_FromMaps(outputs);
      PyObject *pArgs=PyTuple_New(3);
      PyObject *pValue;
      PyTuple_SetItem(pArgs, 0, (PyObject *)arg1);
      PyTuple_SetItem(pArgs, 1, (PyObject *)arg2);
      PyTuple_SetItem(pArgs, 2, (PyObject *)arg3);
      tmp=getMap(request,"storeExecuteResponse");
#ifdef DEBUG
      fprintf(stderr,"RUN IN NORMAL MODE \n");
      fflush(stderr);
#endif
      pValue = PyObject_CallObject(pFunc, pArgs);
      if (pValue != NULL) {
	res=PyInt_AsLong(pValue);
	freeMaps(real_outputs);
	free(*real_outputs);
	//*real_inputs=mapsFromPyDict(arg2);
	//createMapsFromPyDict(real_outputs,arg3);
	*real_outputs=mapsFromPyDict(arg3);
#ifdef DEBUG
	fprintf(stderr,"Result of call: %i\n", PyInt_AsLong(pValue));
	dumpMaps(inputs);
	dumpMaps(outputs);
#endif
	Py_DECREF(arg1);
	Py_DECREF(arg2);
	Py_DECREF(arg3);
      	Py_DECREF(pArgs);
	Py_DECREF(pValue);
	Py_XDECREF(pFunc);
	Py_DECREF(pModule);
      }else{	  
	PyObject *ptype,*pvalue, *ptraceback;
	PyErr_Fetch(&ptype, &pvalue, &ptraceback);
	PyObject *trace=PyObject_Str(pvalue);
	char tb[1024];
	char pbt[10240];
	if(PyString_Check(trace))
	  sprintf(pbt,"TRACE : %s",PyString_AsString(trace));
	else
	  fprintf(stderr,"EMPTY TRACE ?");
	trace=NULL;
	trace=PyObject_Str(ptype);
	if(PyString_Check(trace))
	  sprintf(pbt,"%s\nTRACE : %s",strdup(pbt),PyString_AsString(trace));
	else
	  fprintf(stderr,"EMPTY TRACE ?");
	PyObject *t;
	pName = PyString_FromString("traceback");
	pModule = PyImport_Import(pName);
	pArgs = PyTuple_New(1);
	PyTuple_SetItem(pArgs, 0, ptraceback);
	pFunc = PyObject_GetAttrString(pModule,"format_tb");
	pValue = PyObject_CallObject(pFunc, pArgs);
	trace=NULL;
	trace=PyObject_Str(pValue);
	if(PyString_Check(trace))
	  sprintf(pbt,"%s\nUnable to run your python process properly. Please check the following messages : %s",strdup(pbt),PyString_AsString(trace));
	else
	  sprintf(pbt,"%s \n Unable to run your python process properly. Unable to provide any futher informations.",strdup(pbt));
	map* err=createMap("text",pbt);
	addToMap(err,"code","NoApplicableCode");
	printExceptionReportResponse(m,err);
	Py_XDECREF(pFunc);
      	Py_DECREF(pArgs);
	Py_DECREF(pModule);
	exit(-1);
      }
    }
    else{
      char tmpS[1024];
      sprintf(tmpS, "Cannot find the %s function int the %s file.\n", s->name, tmp->value);
      map* tmps=createMap("text",tmpS);
      printExceptionReportResponse(m,tmps);
      Py_XDECREF(pFunc);
      Py_DECREF(pModule);
      exit(-1);
    }
  } else{
    char tmpS[1024];
    sprintf(tmpS, "Python module %s cannot be loaded.\n", tmp->value);
    map* tmps=createMap("text",tmpS);
    printExceptionReportResponse(m,tmps);
    if (PyErr_Occurred())
      PyErr_Print();
    exit(-1);
  } 
#ifndef DEBUG
  // Failed when DEBUG is defined
  Py_Finalize();
#endif
  return res;
}

PyDictObject* PyDict_FromMaps(maps* t){
  PyObject* res=PyDict_New( );
  maps* tmp=t;
  while(tmp!=NULL){
    if(PyDict_SetItem(res,PyString_FromString(tmp->name),(PyObject*)PyDict_FromMap(tmp->content))<0){
      fprintf(stderr,"Unable to parse params...");
      exit(1);
    }
    tmp=tmp->next;
  }  
  return (PyDictObject*) res;
}

PyDictObject* PyDict_FromMap(map* t){
  PyObject* res=PyDict_New( );
  map* tmp=t;
  while(tmp!=NULL){
    if(strcasecmp(tmp->name,"value")==0){
      map* ttmp=getMap(t,"size");
      if(ttmp!=NULL)
	if(PyDict_SetItem(res,PyString_FromString(tmp->name),PyString_FromStringAndSize(tmp->value,(Py_ssize_t) atoi(ttmp->value)))<0){
	  fprintf(stderr,"Unable to parse params...");
	  exit(1);
	}
      else
	if(PyDict_SetItem(res,PyString_FromString(tmp->name),PyString_FromString(tmp->value))<0){
	  fprintf(stderr,"Unable to parse params...");
	  exit(1);
	}
    } 
    else
      if(PyDict_SetItem(res,PyString_FromString(tmp->name),PyString_FromString(tmp->value))<0){
	fprintf(stderr,"Unable to parse params...");
	exit(1);
      }
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
    while(cursor!=NULL){
      cursor=cursor->next;
    }
    cursor=(maps*)malloc(MAPS_SIZE);
    cursor->name=PyString_AsString(key);
#ifdef DEBUG
    dumpMap(mapFromPyDict((PyDictObject*)value));
#endif
    cursor->content=mapFromPyDict((PyDictObject*)value);
    cursor->next=NULL;
    if(res==NULL)
      res=cursor;
    else
      addMapsToMaps(&res,cursor);
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
    if(res!=NULL)
      addToMap(res,PyString_AsString(key),PyString_AsString(value));
    else
      res=createMap(PyString_AsString(key),PyString_AsString(value));
  }
  return res;
}
