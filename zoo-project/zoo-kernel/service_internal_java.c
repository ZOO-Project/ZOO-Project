/**
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2011 GeoLabs SARL
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

#include "service_internal_java.h"

int zoo_java_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  char ntmp[1024];
  getcwd(ntmp,1024);
  map* tmp=getMap(request,"metapath");
  char classpath[2048];
  char oclasspath[2068];
  int res=SERVICE_FAILED;
  char *cclasspath=getenv("CLASSPATH");
  if(tmp!=NULL){
    if(cclasspath!=NULL)
      sprintf(classpath,"%s/%s/:%s",ntmp,tmp->value,cclasspath);
    else
      sprintf(classpath,"%s/%s/",ntmp,tmp->value);
    sprintf(oclasspath,"-Djava.class.path=%s",classpath);
  }
  else{
    if(cclasspath!=NULL)
      sprintf(classpath,"%s:%s",ntmp,cclasspath);
    else
      sprintf(classpath,"%s/%s/",ntmp,tmp->value);
    sprintf(oclasspath,"-Djava.class.path=%s",classpath);
  }
#ifdef DEBUG
  fprintf(stderr,"CLASSPATH=%s\n",classpath);
#endif
  setenv("CLASSPATH",classpath,1);

  JavaVMOption options[1];
  JavaVMInitArgs vm_args;
  JavaVM *jvm;
  JNIEnv *env;
  long result;
  jmethodID pmid;
  jfieldID fid;
  jobject jobj;
  jclass cls,cls_gr;
  int i;

  options[0].optionString = strdup(oclasspath);

  JNI_GetDefaultJavaVMInitArgs(&vm_args);
  vm_args.version = JNI_VERSION_1_6;
  vm_args.options = options;
  vm_args.nOptions = 1;
  vm_args.ignoreUnrecognized = JNI_FALSE;

  result = JNI_CreateJavaVM(&jvm,(void **)&env, &vm_args);
  if(result == JNI_ERR ) {
    fprintf(stderr,"Error invoking the JVM");
    return -1;
  }
#ifdef DEBUG
  else  
    fprintf(stderr,"JAVA VM Started\n");
#endif

  tmp=getMap(s->content,"serviceProvider");
  cls = (*env)->FindClass(env,tmp->value);
  cls_gr = (*env)->NewGlobalRef(env, cls);
  if( cls == NULL ) {
    char pbt[10240];
    sprintf(pbt,"can't find class %s\n",tmp->value);
    map* err=createMap("text",pbt);
    addToMap(err,"code","NoApplicableCode");
    printExceptionReportResponse(m,err);
    freeMap(&err);
    free(err);
    (*jvm)->DestroyJavaVM(jvm);
    return -1;
  }
#ifdef DEBUG
  else{
    fprintf(stderr,"%s loaded\n",tmp->value);
  }
#endif

  if (cls != NULL) {
    (*env)->ExceptionClear(env);
    pmid=(*env)->GetStaticMethodID(env,cls_gr, s->name, "(Ljava/util/HashMap;Ljava/util/HashMap;Ljava/util/HashMap;)I");
    if (pmid!=0){
#ifdef DEBUG
      fprintf(stderr,"Function successfully loaded\n");
#endif
      jclass scHashMapClass,scHashMap_class;
      jmethodID scHashMap_constructor;
      scHashMapClass = (*env)->FindClass(env, "java/util/HashMap");
      scHashMap_class = (*env)->NewGlobalRef(env, scHashMapClass);
      scHashMap_constructor = (*env)->GetMethodID(env, scHashMap_class, "<init>", "()V");
      /**
       * The 3 standard parameter for each services
       */
      jobject arg1=HashMap_FromMaps(env,m,scHashMapClass,scHashMap_class,scHashMap_constructor);
      jobject arg2=HashMap_FromMaps(env,inputs,scHashMapClass,scHashMap_class,scHashMap_constructor);
      jobject arg3=HashMap_FromMaps(env,outputs,scHashMapClass,scHashMap_class,scHashMap_constructor);
      jint pValue=0;

      pValue=(*env)->CallStaticIntMethod(env,cls,pmid,arg1,arg2,arg3);
      if (pValue != (jint)NULL){
	res=pValue;
	m=mapsFromHashMap(env,arg1,scHashMapClass);
	*main_conf=m;
	outputs=mapsFromHashMap(env,arg3,scHashMapClass);
	*real_outputs=outputs;

#ifdef DEBUG
	fprintf(stderr,"Result of call: %i\n", pValue);
	dumpMaps(inputs);
	dumpMaps(outputs);
#endif
      }else{	  
	/**
	 * Error handling displayig stack trace in ExceptionReport
	 */
	map *tmpm=getMapFromMaps(*main_conf,"main","tmpPath");
	char tmps[1024];
	sprintf(tmps,"%s/%d.ztmp",tmpm->value,getpid());
	int old_stdout=dup(fileno(stdout));
	FILE* new_stdout=fopen(tmps,"w+");
	dup2(fileno(new_stdout),fileno(stdout));
	(*env)->ExceptionDescribe(env);
	fflush(stdout);
	dup2(old_stdout,fileno(stdout));
	fseek(new_stdout, 0, SEEK_END);
	long flen=ftell(new_stdout);
	rewind(new_stdout);
	char tmps1[flen];
	fread(tmps1,flen,1,new_stdout);
	fclose(new_stdout);
	char pbt[100+flen];
	sprintf(pbt,"Unable to run your java process properly. Server returns : %s",tmps1);
	map* err=createMap("text",pbt);
	addToMap(err,"code","NoApplicableCode");
	printExceptionReportResponse(m,err);
	freeMap(&err);
	free(err);
	(*jvm)->DestroyJavaVM(jvm);
	return -1;
      }
    }
    else{
      char tmpS[1024];
      sprintf(tmpS, "Cannot find function %s \n", s->name);
      map* err=createMap("text",tmpS);
      printExceptionReportResponse(m,err);
      freeMap(&err);
      free(err);
      (*jvm)->DestroyJavaVM(jvm);
      return -1;
    }
  }else{
    char tmpS[1024];
    sprintf(tmpS, "Cannot find function %s \n", tmp->value);
    map* err=createMap("text",tmpS);
    printExceptionReportResponse(m,err);
    freeMap(&err);
    free(err);
    (*jvm)->DestroyJavaVM(jvm);
    return -1;
  }
  (*jvm)->DestroyJavaVM(jvm);
  return res;
}

jobject HashMap_FromMaps(JNIEnv *env,maps* t,jclass scHashMapClass,jclass scHashMap_class,jmethodID scHashMap_constructor){
  jobject scObject,scObject1;
  if(scHashMap_constructor!=NULL){
    scObject = (*env)->NewObject(env, scHashMap_class, scHashMap_constructor);
    jmethodID put_mid = 0;

    put_mid = (*env)->GetMethodID(env,scHashMapClass, "put",
				  "(Ljava/lang/Object;Ljava/lang/Object;)"
				  "Ljava/lang/Object;");
    maps* tmp=t;
    while(tmp!=NULL){
      map* tmp1=tmp->content;
      scObject1 = (*env)->NewObject(env, scHashMap_class, scHashMap_constructor);
      map* sizeV=getMap(tmp1,"size");
      while(tmp1!=NULL){
	if(sizeV!=NULL && strcmp(tmp1->name,"value")==0){
	  jbyteArray tmpData=(*env)->NewByteArray(env,atoi(sizeV->value));
	  (*env)->SetByteArrayRegion(env,tmpData,0,atoi(sizeV->value),tmp1->value);
	  (*env)->CallObjectMethod(env,scObject1, put_mid, (*env)->NewStringUTF(env,tmp1->name), tmpData);
	}else
	  (*env)->CallObjectMethod(env,scObject1, put_mid, (*env)->NewStringUTF(env,tmp1->name), (*env)->NewStringUTF(env,tmp1->value));
	tmp1=tmp1->next;
      }
      (*env)->CallObjectMethod(env,scObject, put_mid, (*env)->NewStringUTF(env,tmp->name), scObject1);
      tmp=tmp->next;
    }
    return scObject;
  }
  else
    return NULL;
}

maps* mapsFromHashMap(JNIEnv *env,jobject t,jclass scHashMapClass){
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap start\n");
#endif
  /**
   * What need to be done (in java).
   * Set set = hm.entrySet();
   * Iterator i = set.iterator();
   * while(i.hasNext()){
   *   Map.Entry me = (Map.Entry)i.next();
   *   System.out.println(me.getKey() + " : " + me.getValue() );
   * }
   */
  jclass scHashMap_class,scSetClass,scIteratorClass,scMapEntryClass,scSet_class,scMapClass;
  jmethodID entrySet_mid,containsKey_mid,get_mid,iterator_mid,hasNext_mid,next_mid,getKey_mid,getValue_mid;
  jobject scObject,scObject1;
  if(scHashMapClass==NULL){
#ifdef DEBUG
    fprintf(stderr,"Unable to load java.util.HashMap\n");
#endif
    return NULL;
  }
  entrySet_mid = (*env)->GetMethodID(env, scHashMapClass, "entrySet", "()Ljava/util/Set;"); 
  containsKey_mid = (*env)->GetMethodID(env, scHashMapClass, "containsKey", "(Ljava/lang/Object;)Z");
  get_mid = (*env)->GetMethodID(env, scHashMapClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;"); 

  if(containsKey_mid==0){
#ifdef DEBUG
    fprintf(stderr,"unable to load containsKey from HashMap object (%d) \n",entrySet_mid);
#endif
    return NULL;
  }
  if(get_mid==0){
#ifdef DEBUG
    fprintf(stderr,"unable to load get from HashMap object (%d) \n",entrySet_mid);
#endif
    return NULL;
  }
  if(entrySet_mid==0){
#ifdef DEBUG
    fprintf(stderr,"unable to load entrySet from HashMap object (%d) \n",entrySet_mid);
#endif
    return NULL;
  }
#ifdef DEBUG
  else
    fprintf(stderr,"entrySet loaded from HashMap object (%d) \n",entrySet_mid);
#endif

  scSetClass = (*env)->FindClass(env, "java/util/Set");
  iterator_mid = (*env)->GetMethodID(env, scSetClass, "iterator", "()Ljava/util/Iterator;"); 
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 1 (%d) \n",iterator_mid);
#endif

  scIteratorClass = (*env)->FindClass(env, "java/util/Iterator");
  hasNext_mid = (*env)->GetMethodID(env, scIteratorClass, "hasNext", "()Z");
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 2 (%d)\n",hasNext_mid);
#endif
  next_mid = (*env)->GetMethodID(env, scIteratorClass, "next", "()Ljava/lang/Object;");
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 3 (%d)\n",next_mid);
#endif

  scMapEntryClass = (*env)->FindClass(env, "java/util/Map$Entry");
  getKey_mid = (*env)->GetMethodID(env, scMapEntryClass, "getKey", "()Ljava/lang/Object;");
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 4 (%d)\n",getKey_mid);
#endif
  getValue_mid = (*env)->GetMethodID(env, scMapEntryClass, "getValue", "()Ljava/lang/Object;");
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 5 (%d)\n",getValue_mid);
#endif

  jobject final_set=(*env)->CallObjectMethod(env,t,entrySet_mid);
  jobject final_iterator=(*env)->CallObjectMethod(env,final_set,iterator_mid);


  maps* final_res=NULL;
  map* res=NULL;
  while((*env)->CallBooleanMethod(env,final_iterator,hasNext_mid)){
    jobject tmp=(*env)->CallObjectMethod(env,final_iterator,next_mid);

    jobject imap=(*env)->CallObjectMethod(env,tmp,getValue_mid);
    jobject set=(*env)->CallObjectMethod(env,imap,entrySet_mid);
    jobject iterator=(*env)->CallObjectMethod(env,set,iterator_mid);

    int size=-1;
    if((*env)->CallBooleanMethod(env,imap,containsKey_mid,(*env)->NewStringUTF(env,"size"))){
      jobject sizeV=(*env)->CallObjectMethod(env, imap, get_mid,(*env)->NewStringUTF(env,"size"));
      const char* sizeVS=(*env)->GetStringUTFChars(env, sizeV, NULL);
      size=atoi(sizeVS);
      fprintf(stderr,"SIZE : %s\n",sizeVS);
      (*env)->ReleaseStringUTFChars(env, sizeV, sizeVS);
    }
    
    while((*env)->CallBooleanMethod(env,iterator,hasNext_mid)){
      jobject tmp1=(*env)->CallObjectMethod(env,iterator,next_mid);
      jobject jk=(*env)->CallObjectMethod(env,tmp1,getKey_mid);
      jobject jv=(*env)->CallObjectMethod(env,tmp1,getValue_mid);

      const char* jkd=(*env)->GetStringUTFChars(env, jk, NULL);
      if(size>=0 && strcmp(jkd,"value")==0){
	jobject value=(*env)->GetByteArrayElements(env, jv, NULL);
	if(res==NULL){
	  res=createMap(jkd,"");
	}else{
	  addToMap(res,jkd,"");
	}
	map* tmpR=getMap(res,"value");
	free(tmpR->value);
	tmpR->value=(char*)malloc((size+1)*sizeof(char));
	memmove(tmpR->value,value,size*sizeof(char));
	tmpR->value[size]=0;
	char tmp[128];
	sprintf(tmp,"%d",size);
	addToMap(res,"size",tmp);
      }
      else{
	const char* jvd=(*env)->GetStringUTFChars(env, jv, NULL);
	if(res==NULL){
	  res=createMap(jkd,jvd);
	}else{
	  addToMap(res,jkd,jvd);
	}
	(*env)->ReleaseStringUTFChars(env, jv, jvd);
      }

      (*env)->ReleaseStringUTFChars(env, jk, jkd);

    }
    jobject jk=(*env)->CallObjectMethod(env,tmp,getKey_mid);
    maps* cmap=(maps*)malloc(sizeof(maps));
    cmap->name=(*env)->GetStringUTFChars(env, jk, NULL);
#ifdef DEBUG
    fprintf(stderr," / %s \n",cmap->name);
#endif
    cmap->content=res;
    cmap->next=NULL;
    if(final_res==NULL)
      final_res=dupMaps(&cmap);
    else
      addMapsToMaps(&final_res,cmap);
    freeMaps(&cmap);
    free(cmap);
    cmap=NULL;
    res=NULL;
  }
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap end\n");
#endif

  return final_res;
}
