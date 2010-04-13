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

#include "service_internal_js.h"

static char dbg[1024];

int zoo_js_support(maps** main_conf,map* request,service* s,
		   maps **inputs,maps **outputs)
{
  /* The class of the global object. */
  JSClass global_class = {
    "global", JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
  };

  /* JS variables. */
  JSRuntime *rt;
  JSContext *cx;
  JSObject  *global;

  /* Create a JS runtime. */
  rt = JS_NewRuntime(8L * 1024L * 1024L);
  if (rt == NULL)
    return 1;

  /* Create a context. */
  cx = JS_NewContext(rt, 8192);
  if (cx == NULL){
    return 1;
  }
  JS_SetOptions(cx, JSOPTION_VAROBJFIX);
  JS_SetVersion(cx, JSVERSION_LATEST);
  JS_SetErrorReporter(cx, reportError);

  /* Create the global object. */
  global = JS_NewObject(cx, &global_class, NULL, NULL);
  if (global == NULL){
    return 1;
  }

  /* Populate the global object with the standard globals,
     like Object and Array. */
  if (!JS_InitStandardClasses(cx, global)){
    return 1;
  }
  if (!JS_DefineFunction(cx, global, "ZOORequest", JSRequest, 4, 0))
    return 1;

  /* Your application code here. This may include JSAPI calls
     to create your own custom JS objects and run scripts. */
  maps* out=*outputs;
  int res=SERVICE_FAILED;
  maps* mc=*main_conf;
  map* tmpm1=getMap(request,"metapath");
  map* tmpm2=getMap(s->content,"serviceProvider");
  char *filename[strlen(tmpm1->value)+strlen(tmpm2->value)+6];
  char ntmp[1024];
  getcwd(ntmp,1024);
  sprintf(filename,"%s/%s%s",ntmp,tmpm1->value,tmpm2->value);
  struct stat file_status;
  stat(filename, &file_status);
  char source[file_status.st_size];
  uint16 lineno;
  jsval rval;
  FILE *jsfile=fopen(filename,"rb");
  JSBool ok ;
  JSScript *script = JS_CompileFileHandle(cx, global, filename, jsfile);
  if(script){
    (void)JS_ExecuteScript(cx, global, script, &rval);
  }
  else{
    char tmp1[1024];
    sprintf(tmp1,"Unable to load JavaScript file %s",filename);
    map* err=createMap("text",tmp1);
    addMapToMap(&err,createMap("code","NoApplicableCode"));
    printExceptionReportResponse(mc,err);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    exit(-1);
  }
  /* Call a function in obj's scope. */
  jsval argv[3];
  argv[0] = OBJECT_TO_JSVAL(JSObject_FromMaps(cx,*main_conf));
  argv[1] = OBJECT_TO_JSVAL(JSObject_FromMaps(cx,*inputs));
  argv[2] = OBJECT_TO_JSVAL(JSObject_FromMaps(cx,*outputs));
  jsval rval1;
#ifdef JS_DEBUG
  fprintf(stderr, "object %p\n", (void *) argv[2]);
#endif

  ok = JS_CallFunctionName(cx, global, s->name, 3, argv, &rval1);

#ifdef JS_DEBUG
  fprintf(stderr, "object %p\n", (void *) argv[2]);
#endif

  JSObject *d;
  if (ok==JS_TRUE) {
#ifdef JS_DEBUG
    fprintf(stderr,"Function run sucessfully !\n");
#endif
    /* Should get a number back from the service function call. */
    ok = JS_ValueToObject(cx, rval1, &d);
    if (ok==JS_TRUE) {
#ifdef JS_DEBUG
      fprintf(stderr,"Function run sucessfully !\n");
#endif
      /*char tmpres[10];
      sprintf(tmpres,"%p",(void *)d);
      res=atoi(tmpres);*/
    }
  }else{
    /* Unable to run JS function */
    char tmp1[1024];
    //sprintf(tmp1,"Unable to run %s from the JavScript file %s",s->name,filename);
    sprintf(tmp1,"Unable to run %s from the JavScript file %s : \n %s",s->name,filename,dbg);
    fprintf(stderr,"%s",tmp1);
    map* err=createMap("text",tmp1);
    addMapToMap(err,createMap("code","NoApplicableCode"));
    printExceptionReportResponse(*main_conf,err);
    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);
    JS_ShutDown();
    exit(-1);
  }

  jsval t=OBJECT_TO_JSVAL(d);
  //JSVAL_TO_OBJECT(t);
  if(JS_IsArrayObject(cx,t)){
#ifdef JS_DEBUG
    fprintf(stderr,"An array was returned !\n");
#endif
    jsdouble len;
    if((JS_GetArrayLength(cx, t, &len)==JS_FALSE)){
#ifdef JS_DEBUG
      fprintf(stderr,"outputs array is empty\n");
#endif
    }
    jsval tmp1;
    JSBool hasResult=JS_GetElement(cx,t,0,&tmp1);
    res=JSVAL_TO_INT(tmp1);
#ifdef JS_DEBUG
    fprintf(stderr," * %d * \n",res);
#endif
    jsval tmp2;
    JSBool hasElement=JS_GetElement(cx,t,1,&tmp2);
    out=mapsFromJSObject(cx,tmp2);
    *outputs=out;
  }
  else{
    //#ifdef JS_DEBUG
    fprintf(stderr,"The service didn't return an array !\n");
    //#endif
    jsval tmp1;
    JSBool hasResult=JS_GetProperty(cx,t,"result",&tmp1);
    res=JSVAL_TO_INT(tmp1);

    //#ifdef JS_DEBUG
    fprintf(stderr," * %d * \n",res);
    //#endif
    jsval tmp2;
    JSBool hasElement=JS_GetProperty(cx,t,"outputs",&tmp2);
    if(!hasElement)
      fprintf(stderr,"No outputs property returned\n");
    if(JS_IsArrayObject(cx,tmp2))
      fprintf(stderr,"outputs is array an as expected\n");
    else
      fprintf(stderr,"outputs is not an array as expected\n");
    out=mapsFromJSObject(cx,tmp2);
#ifdef JS_DEBUG
    dumpMaps(out);
#endif
    *outputs=out;
  }

  *inputs=mapsFromJSObject(cx,argv[1]);
  *main_conf=mapsFromJSObject(cx,argv[0]);

  
  //}
  //else
  // ERROR 
  //;

  /* Cleanup. */
  JS_MaybeGC(cx);
  JS_DestroyScript(cx, script);
  JS_DestroyContext(cx);
  JS_DestroyRuntime(rt);
  JS_ShutDown();
  fflush(stderr);
#ifdef JS_DEBUG
  fprintf(stderr,"Returned value %d\n",res);
#endif
  return res;
}

JSObject* JSObject_FromMaps(JSContext *cx,maps* t){
  JSObject *res = JS_NewArrayObject(cx, 0, NULL);
  maps* tmp=t;
  while(tmp!=NULL){
    jsuint len;
    JSObject* res1=JS_NewObject(cx, NULL, NULL, NULL);
    JSObject *pval=JSObject_FromMap(cx,tmp->content);
    jsval pvalj=OBJECT_TO_JSVAL(pval);
    JS_SetProperty(cx, res1, tmp->name, &pvalj);
    JS_GetArrayLength(cx, res, &len);
    jsval res1j = OBJECT_TO_JSVAL(res1);
    JS_SetElement(cx,res,len,&res1j);
#ifdef JS_DEBUG
    fprintf(stderr,"Length of the Array %d, element : %s added \n",len,tmp->name);
#endif
    tmp=tmp->next;
  }  
  return res;
}

JSObject* JSObject_FromMap(JSContext *cx,map* t){
  JSObject* res=JS_NewObject(cx, NULL, NULL, NULL);
  jsval resf =  OBJECT_TO_JSVAL(res);
  map* tmpm=t;
  while(tmpm!=NULL){
    jsval jsstr = STRING_TO_JSVAL(JS_NewString(cx,tmpm->value,strlen(tmpm->value)));
    JS_SetProperty(cx, resf, tmpm->name,&jsstr);
#ifdef JS_DEBUG
    fprintf(stderr,"%s => %s\n",tmpm->name,tmpm->value);
#endif
    tmpm=tmpm->next;
  }
  return res;
}

maps* mapsFromJSObject(JSContext *cx,jsval t){
  maps *res=NULL;
  maps *tres=NULL;
  JSVAL_TO_OBJECT(t);
#ifdef JS_DEBUG
  fprintf(stderr,"Is finally an array ?\n");
#endif
  if(JS_IsArrayObject(cx,t)){
#ifdef JS_DEBUG
    fprintf(stderr,"Is finally an array !\n");
#endif
  }
#ifdef JS_DEBUG
  else
    fprintf(stderr,"Is not an array !\n");
#endif
  jsdouble len;
  if((JS_GetArrayLength(cx, t, &len)==JS_FALSE)){
#ifdef JS_DEBUG
    fprintf(stderr,"outputs array is empty\n");
#endif
  }
  int i=0;
  char tmp[10];
  sprintf(tmp,"%d",len);
  int clen=atoi(tmp);
#ifdef JS_DEBUG
  fprintf(stderr,"outputs array length : %d\n",len);
#endif
  for(i=0;i<len;i++){
#ifdef JS_DEBUG
    fprintf(stderr,"outputs array length : %d step %d\n",len,i);
#endif
    jsval tmp1;
    JSBool hasElement=JS_GetElement(cx,t,i,&tmp1);
    JSVAL_TO_OBJECT(tmp1);
    JSIdArray *idp=JS_Enumerate(cx,tmp1);
    if(idp!=NULL) {
      int index;
      jsdouble argNum;
#ifdef JS_DEBUG
      fprintf(stderr,"Properties length :  %d \n",idp->length);
#endif
      for (index=0,argNum=idp->length;index<argNum;index++) { 
	jsid* id = idp->vector[index];
	jsval vp;
	JSString* str; 
	JS_IdToValue(cx,id,&vp);
	char *c, *tmp;
	JSString *jsmsg;
	size_t len1;
	jsmsg = JS_ValueToString(cx,vp);
	len1 = JS_GetStringLength(jsmsg);
#ifdef JS_DEBUG
	fprintf(stderr,"Enumerate id : %d => %s\n",i,JS_GetStringBytes(jsmsg));
#endif
	jsval nvp;
	if((JS_GetProperty(cx, tmp1, JS_GetStringBytes(jsmsg), &nvp)==JS_FALSE))
#ifdef JS_DEBUG
	fprintf(stderr,"Enumerate id : %d => %s => No more value\n",i,JS_GetStringBytes(jsmsg));
#endif
	if(JSVAL_IS_OBJECT(nvp)){
#ifdef JS_DEBUG
	  fprintf(stderr,"JSVAL NVP IS OBJECT\n");
#endif
	}
#ifdef JS_DEBUG
	else
	  fprintf(stderr,"JSVAL NVP IS NOT OBJECT !!\n");
#endif
	JSObject *nvp1;
	JS_ValueToObject(cx,nvp,&nvp1);
	jsval nvp1j=OBJECT_TO_JSVAL(nvp1);
	if(JSVAL_IS_OBJECT(nvp1j)){
#ifdef JS_DEBUG
	  fprintf(stderr,"JSVAL NVP1J IS OBJECT\n");
#endif
	  tres=(maps*)malloc(sizeof(maps*));
	  tres->name=strdup(JS_GetStringBytes(jsmsg));
	  tres->content=mapFromJSObject(cx,nvp1j);
	  tres->next=NULL;
#ifdef JS_DEBUG
	  dumpMaps(res);
#endif
	  if(res==NULL)
	    res=tres;
	  else
	    addMapsToMaps(&res,tres);
#ifdef JS_DEBUG
	  dumpMaps(res);
#endif
	}
#ifdef JS_DEBUG
	else
	  fprintf(stderr,"JSVAL NVP1J IS NOT OBJECT !!\n");
#endif
      }
    }
  }
#ifdef JS_DEBUG
  dumpMaps(res);
#endif
  return res;
}

map* mapFromJSObject(JSContext *cx,jsval t){
  map *res=NULL;
  JSIdArray *idp=JS_Enumerate(cx,t);
#ifdef JS_DEBUG
  fprintf(stderr,"Properties %p\n",(void*)t);
#endif
  if(idp!=NULL) {
    int index;
    jsdouble argNum;
#ifdef JS_DEBUG
    fprintf(stderr,"Properties length :  %d \n",idp->length);
#endif
    for (index=0,argNum=idp->length;index<argNum;index++) { 
      jsid* id = idp->vector[index];
      jsval vp;
      JSString* str; 
      JS_IdToValue(cx,id,&vp);
      char *c, *tmp;
      JSString *jsmsg,*jsmsg1;
      size_t len,len1;
      jsmsg = JS_ValueToString(cx,vp);
      len = JS_GetStringLength(jsmsg);
      jsval nvp;
      JS_GetProperty(cx, t, JS_GetStringBytes(jsmsg), &nvp);
      jsmsg1 = JS_ValueToString(cx,nvp);
      len1 = JS_GetStringLength(jsmsg1);
#ifdef JS_DEBUG
      fprintf(stderr,"Enumerate id : %d [ %s => %s ]\n",index,JS_GetStringBytes(jsmsg),JS_GetStringBytes(jsmsg1));
#endif
      if(res!=NULL)
	addMapToMap(&res,createMap(JS_GetStringBytes(jsmsg),JS_GetStringBytes(jsmsg1)));
      else
	res=createMap(JS_GetStringBytes(jsmsg),JS_GetStringBytes(jsmsg1));
    }
  }
#ifdef JS_DEBUG
  dumpMap(res);
#endif
  return res;
}


/* The error reporter callback. */
void reportError(JSContext *cx, const char *message, JSErrorReport *report)
{
  sprintf(dbg,"%s:%u:%s\n",
	  report->filename ? report->filename : "<no filename>",
	  (unsigned int) report->lineno,
	  message);
#ifdef JS_DEBUG
  fprintf(stderr,"%s",dbg);
#endif
  fflush(stderr);
}

