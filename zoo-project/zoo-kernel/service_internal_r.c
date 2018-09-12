/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2018 GeoLabs SARL
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

#include "service_internal_r.h"

void jump_to_toplevel(void){
  fprintf(stderr,"Error occured\n");
  fprintf(stderr,"%s %d \n",__FILE__,__LINE__);
  fflush(stderr);
  //resetStack(1);
  fprintf(stderr,"%s %d \n",__FILE__,__LINE__);
  fflush(stderr);
}

void init_zoo(SEXP conf,SEXP outputs){
  const char* names[]={
    "SERVICE_SUCCEEDEED",
    "SERVICE_FAILED",
    "conf",
    "outputs",
    ""
  };
  SEXP res = PROTECT(mkNamed(VECSXP, names));
  SET_VECTOR_ELT(res, 0, Rf_ScalarInteger(3));
  SET_VECTOR_ELT(res, 1, Rf_ScalarInteger(4));
  SET_VECTOR_ELT(res, 2, conf);
  SET_VECTOR_ELT(res, 3, outputs);
  defineVar(install("zoo"),res, R_GlobalEnv);

  static const R_CallMethodDef callMethods[]  = {
    {"ZOOTranslate", (DL_FUNC) &RTranslate, 1},
    {"ZOOUpdateStatus", (DL_FUNC) &RUpdateStatus, 2},
    {NULL, NULL, 0}
  };
  
  static R_NativePrimitiveArgType RTranslate_t[] = {
    STRSXP
  };
  static R_NativePrimitiveArgType RUpdateStatus_t[] = {
    VECSXP,STRSXP
  };
  
  static const R_CMethodDef cMethods[] = {
   {"ZOOTranslate", (DL_FUNC) &RTranslate, 1, RTranslate_t},
   {"ZOOUpdateStatus", (DL_FUNC) &RUpdateStatus, 2, RUpdateStatus_t},
   {NULL, NULL, 0, NULL}
  };
  
  DllInfo *info = R_getEmbeddingDllInfo();
  R_registerRoutines(info, cMethods, callMethods, NULL, NULL);
}

/**
 * Load a R script then run the function corresponding to the service
 * by passing the conf, inputs and outputs parameters by reference. 
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param real_inputs the maps containing the inputs
 * @param real_outputs the maps containing the outputs
 */
int zoo_r_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  SEXP pName;
  int result=0;
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  map* tmp0=getMapFromMaps(*main_conf,"lenv","cwd");
  char *ntmp=tmp0->value;
  map* tmp=NULL;
  int hasToClean=0;
  char *r_path, *rpath;
  map* cwdMap=getMapFromMaps(*main_conf,"main","servicePath");
  int r_argc = 3;
  char *r_argv[] = { "R", "--no-save", "--silent" };
  Rf_initEmbeddedR(r_argc, r_argv);
  if(cwdMap!=NULL)
    r_path=cwdMap->value;
  else{
    if(tmp0!=NULL)
      r_path=tmp0->value;
    else
      r_path=(char*)".";
  }

  tmp=getMap(s->content,"serviceProvider");
  map* mp=getMap(request,"metapath");
  if(tmp!=NULL){
    if(mp!=NULL && strlen(mp->value)>0){
      char *mn=(char*)malloc((strlen(mp->value)+strlen(tmp->value)+2)*sizeof(char));
      sprintf(mn,"%s/%s",mp->value,tmp->value);
      pName = mkString(mn);
      free(mn);
    }
    else{
      char *tmpStr=(char*)malloc((strlen(r_path)+strlen(tmp->value)+2)*sizeof(char));
      sprintf(tmpStr,"%s/%s",r_path,tmp->value);
      pName = mkString(tmpStr);
      free(tmpStr);
    }
  }
  else{
    errorException (m, "Unable to parse serviceProvider please check your zcfg file.", "NoApplicableCode", NULL);
    return -1;
  }
  SEXP e;
  int errorOccurred;
  PROTECT(e = lang2(install("source"), pName));
  R_tryEval(e, R_GlobalEnv, &errorOccurred);
  UNPROTECT(1);
  if (errorOccurred){
    setMapInMaps(*main_conf,"lenv","message",_("Unable to load your R file"));
    return SERVICE_FAILED;
  }else{
    result=SERVICE_FAILED;
    {
      SEXP pValue;
      SEXP  arg1=RList_FromMaps(m);
      SEXP  arg2=RList_FromMaps(*real_inputs);
      SEXP  arg3=RList_FromMaps(*real_outputs);
      SEXP r_call;
      init_zoo(arg1,arg3);
      PROTECT(r_call = lang4(install(s->name), arg1,arg2,arg3));
      int errorOccurred;
      
      SEXP ret = R_tryEval(r_call, R_GlobalEnv, &errorOccurred);
      if (!errorOccurred) {
	int *val = INTEGER(ret);
        for (int i = 0; i < LENGTH(ret); i++)
	  if(i==0){
	      result=val[i];
	      SEXP zooEnv = findVar(install("zoo"), R_GlobalEnv);
	      if(zooEnv!=NULL){
		SEXP names = Rf_getAttrib(zooEnv, R_NamesSymbol);
		int nbKeys=nrows(names);
		int i;
		for(i=0;i<nbKeys;i++){
		  if(i==2){
		    freeMaps(main_conf);
		    free(*main_conf);
		    SEXP confList=VECTOR_ELT(zooEnv,i);
		    *main_conf=mapsFromRList(confList);
		  }
		  if(i==3){
		    freeMaps(real_outputs);
		    free(*real_outputs);
		    SEXP outList=VECTOR_ELT(zooEnv,i);
		    *real_outputs=mapsFromRList(outList);
		  }
		}
	      }
	      return result;
	    }
      }else{
	const char* tmpStr=R_curErrorBuf();
	setMapInMaps(*main_conf,"lenv","message",tmpStr);
	char* finalStr=(char*)malloc((strlen(tmpStr)+strlen(_("Unable to run your R service: "))+2)*sizeof(char));
	sprintf(finalStr,"%s %s",_("Unable to run your R service: "),tmpStr);
	errorException(*main_conf,finalStr,"NoApplicableCode",NULL);
	free(finalStr);
	result=-1;
      }
    }
  }
  Rf_endEmbeddedR(0);
  return result;
}

char** listMapsKeys(maps* m){
  char** res=NULL;
  maps* tmp=m;
  int i=0;
  while(tmp!=NULL){
    if(i==0)
      res=(char**)malloc(2*sizeof(char*));
    else
      res=(char**)realloc(res,(i+2)*sizeof(char*));
    res[i]=zStrdup(tmp->name);
    res[i+1]="";
    i++;
    tmp=tmp->next;
  }
  return res;
}

char** listMapKeys(map* m){
  char** res=NULL;
  map* tmp=m;
  int i=0;
  while(tmp!=NULL){
    if(i==0)
      res=(char**)malloc(2*sizeof(char*));
    else
      res=(char**)realloc(res,(i+2)*sizeof(char*));
    res[i]=zStrdup(tmp->name);
    res[i+1]="";
    i++;
    tmp=tmp->next;
  }
  return res;
}

/**
 * Convert a maps to a R List
 *
 * @param t the maps to convert
 * @return a new SEXP containing the converted maps
 * @see RList_FromMap
 * @warning make sure to free resources returned by this function
 */
SEXP RList_FromMaps(maps* t){
  maps* tmp=t;
  char** keys=listMapsKeys(t);
  SEXP res = PROTECT(mkNamed(VECSXP,(const char**) keys));
  free(keys);
  int cnt=0;
  while(tmp!=NULL){
    SEXP input = RList_FromMap(tmp->content);
    SET_VECTOR_ELT(res,cnt,input);
    cnt++;
    tmp=tmp->next;
  } 
  UNPROTECT(1);
  return res;
}

/**
 * Convert a map to a R List
 *
 * @param t the map to convert
 * @return a new SEXP containing the converted maps
 * @warning make sure to free resources returned by this function
 */
SEXP RList_FromMap(map* t){
  map* tmp=t;
  int hasSize=0;
  char** keys=listMapKeys(t);
  SEXP res = PROTECT(mkNamed(VECSXP, (const char**)keys));
  free(keys);
  int cnt=0;
  while(tmp!=NULL){
    SEXP value=mkString(tmp->value);
    SET_VECTOR_ELT(res,cnt,value);
    cnt++;
    tmp=tmp->next;
  }
  UNPROTECT(1);
  return res;
}

/**
 * Convert a R List to a maps 
 *
 * @param t the PyDictObject to convert
 * @return a new maps containing the converted PyDictObject
 * @warning make sure to free resources returned by this function
 */
maps* mapsFromRList(SEXP t){
  maps* res=NULL;
  maps* cursor=NULL;
  SEXP names = Rf_getAttrib(t, R_NamesSymbol);
  int nbKeys=nrows(names);
  int i;
  for(i=0;i<nbKeys;i++){
    SEXP key=STRING_ELT(names,i);
    SEXP value=VECTOR_ELT(t,i);
    cursor=createMaps(R_CHAR(key));
    cursor->content=mapFromRList(value);
    cursor->next=NULL;
    if(res==NULL)
      res=dupMaps(&cursor);
    else
      addMapsToMaps(&res,cursor);
    freeMap(&cursor->content);
    free(cursor->content);
    free(cursor);
  }
  return res;
}

/**
 * Convert a R List to a map 
 *
 * @param t the PyDictObject to convert
 * @return a new map containing the converted PyDictObject
 * @warning make sure to free resources returned by this function
 */
map* mapFromRList(SEXP t){
  map* res=NULL;
  SEXP names = Rf_getAttrib(t, R_NamesSymbol);
  int nbKeys=nrows(names);
  int i;
  for(i=0;i<nbKeys;i++){
    SEXP key=STRING_ELT(names,i);
    SEXP value=VECTOR_ELT(t,i);
    if(strncmp(R_CHAR(key),"child",5)!=0){
      {
	const char* lkey=R_CHAR(key);
	const char* lvalue=CHAR(STRING_ELT(value,0));
	if(res!=NULL){
	  addToMap(res,lkey,lvalue);
	}
	else{
	  res=createMap(lkey,lvalue);
	}
      }
    }
  }
  return res;
}

/**
 * Use the ZOO-Services messages translation function from the R
 * environment
 *
 * @param str the R string passed from the R environment
 * @return a new R string containing the translated value
 * @see _ss
 */
SEXP
RTranslate(SEXP str)
{
  if (!isString(str) || !TYPEOF(STRING_ELT( str, 0 )) == CHARSXP){
#ifdef DEBUG
    fprintf(stderr,"Incorrect arguments to update status function");
#endif
    return R_NilValue;
  }
  const char* tmpStr=CHAR(STRING_ELT(str,0));
  return mkString(_ss(tmpStr));
}

/**
 * Update the ongoing status of a running service from the R environment
 *
 * @param confdict the R arguments passed from the R environment
 * @param status the R arguments passed from the R environment
 * @return Nil to the Python environment
 * @see _updateStatus
 */
SEXP
RUpdateStatus(SEXP confdict,SEXP status)
{
  maps* conf;
  int istatus;
  char* cstatus=NULL;
  if (!isInteger(status) && !isReal(status)){
#ifdef DEBUG
    fprintf(stderr,"Incorrect arguments to update status function");
#endif
    return R_NilValue;
  }
  if(isInteger(status))
    istatus=asInteger(status);
  else
    istatus=asReal(status);
  if (istatus < 0 || istatus > 100){
    return R_NilValue;
  }
  // create a local copy and update the lenv map
  conf = mapsFromRList(confdict);
  if(status!=NULL){
    maps* tmpMaps=getMaps(conf,"lenv");
    addIntToMap(tmpMaps->content,"status",istatus);
  }
  else
    setMapInMaps(conf,"lenv","status","15");
  _updateStatus(conf);
  freeMaps(&conf);
  free(conf);
  return R_NilValue;
}
