/**
 * Author : Gérald FENOY
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

#include "service_internal_java.h"

int zoo_java_support(maps** main_conf,map* request,service* s,maps **real_inputs,maps **real_outputs){
  maps* m=*main_conf;
  maps* inputs=*real_inputs;
  maps* outputs=*real_outputs;
  char ntmp[1024];
  getcwd(ntmp,1024);
  map* tmp=getMap(request,"metapath");
  char *classpath;
  char *oclasspath;
  int res=SERVICE_FAILED;
  char *cclasspath=getenv("CLASSPATH");
  if(tmp!=NULL){
    if(cclasspath!=NULL){
      classpath=(char*) malloc((strlen(ntmp)+strlen(tmp->value)+strlen(cclasspath)+4)*sizeof(char));
      oclasspath=(char*) malloc((strlen(ntmp)+strlen(tmp->value)+strlen(cclasspath)+22)*sizeof(char));
#ifndef WIN32
      sprintf(classpath,"%s/%s/:%s",ntmp,tmp->value,cclasspath);
#else
      sprintf(classpath,"%s/%s/;%s",ntmp,tmp->value,cclasspath);
#endif
    }
    else{
      classpath=(char*) malloc((strlen(ntmp)+strlen(tmp->value)+3)*sizeof(char));
      oclasspath=(char*) malloc((strlen(ntmp)+strlen(tmp->value)+21)*sizeof(char));
      sprintf(classpath,"%s/%s/",ntmp,tmp->value);
    }
    sprintf(oclasspath,"-Djava.class.path=%s",classpath);
  }else{
    if(cclasspath!=NULL){
      classpath=(char*) malloc((strlen(ntmp)+strlen(cclasspath)+3)*sizeof(char));
      oclasspath=(char*) malloc((strlen(ntmp)+strlen(cclasspath)+21)*sizeof(char));
#ifndef WIN32
      sprintf(classpath,"%s/:%s",ntmp,cclasspath);
#else
      sprintf(classpath,"%s/;%s",ntmp,cclasspath);
#endif
    }
    else{
      classpath=(char*) malloc((strlen(ntmp)+2)*sizeof(char));
      oclasspath=(char*) malloc((strlen(ntmp)+20)*sizeof(char));
      sprintf(classpath,"%s/",ntmp);
    }
    sprintf(oclasspath,"-Djava.class.path=%s",classpath);

  }
  sprintf(oclasspath,"-Djava.class.path=%s",classpath);

#ifdef DEBUG
  fprintf(stderr,"CLASSPATH=%s\n",classpath);
  fprintf(stderr,"(%s)\n",oclasspath);
#endif

#ifdef WIN32
  JavaVMOption options[2];
#else
  JavaVMOption options[1];
#endif
  JavaVMInitArgs vm_args;
  JavaVM *jvm;
  JNIEnv *env;
  long result;
  jmethodID pmid;
  jfieldID fid;
  jobject jobj;
  jclass cls;
#ifdef JAVA7
  jobject cls_gr;
#else
  jclass cls_gr;
#endif
  int i;

  options[0].optionString = oclasspath;
#ifdef WIN32
  options[1].optionString = "-Xmx512m";
  /*options[2].optionString = "-Xms128m";
  options[3].optionString = "-XX:MaxPermSize=256m";
  options[4].optionString = "-XX:MaxHeapFreeRatio=70";*/
#endif

  JNI_GetDefaultJavaVMInitArgs(&vm_args);
  vm_args.version = JNI_VERSION_1_6;
  vm_args.options = options;
#ifdef WIN32
  vm_args.nOptions = 2;
#else
  vm_args.nOptions = 1;
#endif
  vm_args.ignoreUnrecognized = JNI_TRUE;

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
#ifdef JAVA7
  cls = env->FindClass(tmp->value);
  cls_gr = env->NewGlobalRef(cls);
#else
  cls = (*env)->FindClass(env,tmp->value);
  cls_gr = (*env)->NewGlobalRef(env, cls);
#endif
  if( cls == NULL ) {
    displayStack(env,*main_conf);
#ifdef JAVA7
    (*jvm).DestroyJavaVM();
#else
    (*jvm)->DestroyJavaVM(jvm);
#endif
    return -1;
  }
#ifdef DEBUG
  else{
    fprintf(stderr,"%s loaded\n",tmp->value);
  }
#endif

  if (cls != NULL) {
#ifdef JAVA7
    (*env).ExceptionClear();
    pmid=(*env).GetStaticMethodID(cls, s->name, "(Ljava/util/HashMap;Ljava/util/HashMap;Ljava/util/HashMap;)I");
#else
    (*env)->ExceptionClear(env);
    pmid=(*env)->GetStaticMethodID(env,cls_gr, s->name, "(Ljava/util/HashMap;Ljava/util/HashMap;Ljava/util/HashMap;)I");
#endif
    if (pmid!=0){
#ifdef DEBUG
      fprintf(stderr,"Function successfully loaded\n");
#endif
      jclass scHashMapClass,scHashMap_class;
      jmethodID scHashMap_constructor;
#ifdef JAVA7
      scHashMapClass = (*env).FindClass("java/util/HashMap");
      scHashMap_class = (jclass)(*env).NewGlobalRef(scHashMapClass);
      scHashMap_constructor = (*env).GetMethodID(scHashMap_class, "<init>", "()V");
#else
      scHashMapClass = (*env)->FindClass(env, "java/util/HashMap");
      scHashMap_class = (*env)->NewGlobalRef(env, scHashMapClass);
      scHashMap_constructor = (*env)->GetMethodID(env, scHashMap_class, "<init>", "()V");
#endif
      /**
       * The 3 standard parameter for each services
       */
      jobject arg1=HashMap_FromMaps(env,m,scHashMapClass,scHashMap_class,scHashMap_constructor);
      jobject arg2=HashMap_FromMaps(env,inputs,scHashMapClass,scHashMap_class,scHashMap_constructor);
      jobject arg3=HashMap_FromMaps(env,outputs,scHashMapClass,scHashMap_class,scHashMap_constructor);
      jint pValue=0;

#ifdef JAVA7
      pValue=(*env).CallStaticIntMethod(cls,pmid,arg1,arg2,arg3);
#else
      pValue=(*env)->CallStaticIntMethod(env,cls,pmid,arg1,arg2,arg3);
#endif
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
	displayStack(env,*main_conf);
#ifdef JAVA7
	(*jvm).DestroyJavaVM();
#else
	(*jvm)->DestroyJavaVM(jvm);
#endif
	return -1;
      }
    }
    else{
      displayStack(env,*main_conf);
#ifdef JAVA7
      (*jvm).DestroyJavaVM();
#else
      (*jvm)->DestroyJavaVM(jvm);
#endif
      return -1;
    }
  }
#ifdef JAVA7
      (*jvm).DestroyJavaVM();
#else
  (*jvm)->DestroyJavaVM(jvm);
#endif
  return res;
}

/**
 * Error handling: display stack trace in an ExceptionReport Document
 */
void displayStack(JNIEnv *env,maps* main_conf){
  map *tmpm=getMapFromMaps(main_conf,"main","tmpPath");
  char tmps[1024];
  sprintf(tmps,"%s/%d.ztmp",tmpm->value,getpid());
  FILE* new_stdout=fopen(tmps,"wb+");
  fflush(stderr);
  dup2(fileno(new_stdout),fileno(stderr));
  fprintf(stderr,"Unable to run your java process properly: ");
  fflush(stderr);
#ifdef JAVA7
  (*env).ExceptionDescribe();
#else
  (*env)->ExceptionDescribe(env);
#endif
  fflush(new_stdout);
  fseek(new_stdout, 0, SEEK_END);
  long flen=ftell(new_stdout);
  fseek(new_stdout, 0, SEEK_SET);
  char *tmps1=(char*)malloc((flen+1)*sizeof(char));
  fread(tmps1,flen,1,new_stdout);
  fclose(new_stdout);
  tmps1[flen]=0;
  map* err=createMap("text",tmps1);
  addToMap(err,"code","InternalError");
  printExceptionReportResponse(main_conf,err);
  freeMap(&err);
  free(err);
}

jobject HashMap_FromMaps(JNIEnv *env,maps* t,jclass scHashMapClass,jclass scHashMap_class,jmethodID scHashMap_constructor){
  jobject scObject,scObject1;
  if(scHashMap_constructor!=NULL){
#ifdef JAVA7
    scObject = (*env).NewObject(scHashMap_class, scHashMap_constructor);
#else
    scObject = (*env)->NewObject(env, scHashMap_class, scHashMap_constructor);
#endif
    jmethodID put_mid = 0;

#ifdef JAVA7
    put_mid = (*env).GetMethodID(scHashMapClass, "put",
				  "(Ljava/lang/Object;Ljava/lang/Object;)"
				  "Ljava/lang/Object;");
#else
    put_mid = (*env)->GetMethodID(env,scHashMapClass, "put",
				  "(Ljava/lang/Object;Ljava/lang/Object;)"
				  "Ljava/lang/Object;");
#endif
    maps* tmp=t;
    while(tmp!=NULL){
      map* tmap=getMapType(tmp->content);
      map* tmp1=tmp->content;
#ifdef JAVA7
      scObject1 = (*env).NewObject(scHashMap_class, scHashMap_constructor);
#else
      scObject1 = (*env)->NewObject(env, scHashMap_class, scHashMap_constructor);
#endif
      map* sizeV=getMap(tmp1,"size");
      map* isArray=getMap(tmp1,"isArray");
      map* alen=getMap(tmp1,"length");
      while(tmp1!=NULL){
	if(strcmp(tmp1->name,"value")==0){
	  if(isArray==NULL){
	    if(sizeV!=NULL && strcmp(tmp1->name,"value")==0){
#ifdef JAVA7
	      jbyteArray tmpData=(*env).NewByteArray(atoi(sizeV->value));
	      (*env).SetByteArrayRegion(tmpData,0,atoi(sizeV->value),(const jbyte *)tmp1->value);
	      (*env).CallObjectMethod(scObject1, put_mid, (*env).NewStringUTF(tmp1->name), tmpData);
#else
	      jbyteArray tmpData=(*env)->NewByteArray(env,atoi(sizeV->value));
	      (*env)->SetByteArrayRegion(env,tmpData,0,atoi(sizeV->value),tmp1->value);
	      (*env)->CallObjectMethod(env,scObject1, put_mid, (*env)->NewStringUTF(env,tmp1->name), tmpData);
#endif
	    }else
#ifdef JAVA7
	      (*env).CallObjectMethod(scObject1, put_mid, (*env).NewStringUTF(tmp1->name), (*env).NewStringUTF(tmp1->value));
#else
	      (*env)->CallObjectMethod(env,scObject1, put_mid, (*env)->NewStringUTF(env,tmp1->name), (*env)->NewStringUTF(env,tmp1->value));
#endif
	  }
	  else{
	    int alen1=atoi(alen->value);
	    fprintf(stderr,"LENGTH %d \n",alen1);
	    
	    jclass scArrayListClass,scArrayList_class;
	    jmethodID scArrayList_constructor;
	    jobject scObject2;
#ifdef JAVA7
	    scArrayListClass = (*env).FindClass("java/util/ArrayList");
	    scArrayList_class = (jclass)(*env).NewGlobalRef(scArrayListClass);
	    scArrayList_constructor = (*env).GetMethodID(scArrayList_class, "<init>", "()V");
	    jmethodID add_mid = 0;
	    scObject2 = (*env).NewObject(scArrayList_class, scArrayList_constructor);

	    add_mid = (*env).GetMethodID(scArrayListClass,
					  "add","(Ljava/lang/Object;)Z");
#else
	    scArrayListClass = (*env)->FindClass(env, "java/util/ArrayList");
	    scArrayList_class = (*env)->NewGlobalRef(env, scArrayListClass);
	    scArrayList_constructor = (*env)->GetMethodID(env, scArrayList_class, "<init>", "()V");
	    jmethodID add_mid = 0;
	    scObject2 = (*env)->NewObject(env, scArrayList_class, scArrayList_constructor);

	    add_mid = (*env)->GetMethodID(env,scArrayListClass,
					  "add","(Ljava/lang/Object;)Z");
#endif	    
	    int i;
	    
	    for(i=0;i<alen1;i++){
	      map* vMap=getMapArray(tmp->content,"value",i);	    
	      map* sMap=getMapArray(tmp->content,"size",i);
	      map* mMap=getMapArray(tmp->content,tmap->value,i);
	      
	      if(sMap!=NULL && vMap!=NULL && strncmp(vMap->name,"value",5)==0){
#ifdef JAVA7
		jbyteArray tmpData=(*env).NewByteArray(atoi(sMap->value));
		(*env).SetByteArrayRegion(tmpData,0,atoi(sMap->value),(const jbyte *)vMap->value);
		(*env).CallObjectMethod(scObject2, add_mid, tmpData);
#else
		jbyteArray tmpData=(*env)->NewByteArray(env,atoi(sMap->value));
		(*env)->SetByteArrayRegion(env,tmpData,0,atoi(sMap->value),vMap->value);
		(*env)->CallObjectMethod(env,scObject2, add_mid, tmpData);
#endif
	      }else{
#ifdef JAVA7
		jobject tmpData=(*env).NewStringUTF(vMap->value);
		(*env).CallObjectMethod(scObject2, add_mid, tmpData);
#else
		jobject tmpData=(*env)->NewStringUTF(env,vMap->value);
		(*env)->CallObjectMethod(env,scObject2, add_mid, tmpData);
#endif
	      }
	      
	    }

#ifdef JAVA7
	    (*env).CallObjectMethod(scObject1, put_mid, (*env).NewStringUTF(tmp1->name), scObject2);
#else	    
	    (*env)->CallObjectMethod(env,scObject1, put_mid, (*env)->NewStringUTF(env,tmp1->name), scObject2);
#endif

	  }
	}
	else
#ifdef JAVA7
	  (*env).CallObjectMethod(scObject1, put_mid, (*env).NewStringUTF(tmp1->name), (*env).NewStringUTF(tmp1->value));
#else
	  (*env)->CallObjectMethod(env,scObject1, put_mid, (*env)->NewStringUTF(env,tmp1->name), (*env)->NewStringUTF(env,tmp1->value));
#endif
	tmp1=tmp1->next;
      }
#ifdef JAVA7
      (*env).CallObjectMethod(scObject, put_mid, (*env).NewStringUTF(tmp->name), scObject1);
#else
      (*env)->CallObjectMethod(env,scObject, put_mid, (*env)->NewStringUTF(env,tmp->name), scObject1);
#endif
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
#ifdef JAVA7
  entrySet_mid = (*env).GetMethodID(scHashMapClass, "entrySet", "()Ljava/util/Set;"); 
  containsKey_mid = (*env).GetMethodID(scHashMapClass, "containsKey", "(Ljava/lang/Object;)Z");
  get_mid = (*env).GetMethodID(scHashMapClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;"); 
#else
  entrySet_mid = (*env)->GetMethodID(env, scHashMapClass, "entrySet", "()Ljava/util/Set;"); 
  containsKey_mid = (*env)->GetMethodID(env, scHashMapClass, "containsKey", "(Ljava/lang/Object;)Z");
  get_mid = (*env)->GetMethodID(env, scHashMapClass, "get", "(Ljava/lang/Object;)Ljava/lang/Object;"); 
#endif

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

#ifdef JAVA7
  scSetClass = (*env).FindClass("java/util/Set");
  iterator_mid = (*env).GetMethodID(scSetClass, "iterator", "()Ljava/util/Iterator;"); 
#else
  scSetClass = (*env)->FindClass(env, "java/util/Set");
  iterator_mid = (*env)->GetMethodID(env, scSetClass, "iterator", "()Ljava/util/Iterator;"); 
#endif
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 1 (%d) \n",iterator_mid);
#endif

#ifdef JAVA7
  scIteratorClass = (*env).FindClass("java/util/Iterator");
  hasNext_mid = (*env).GetMethodID(scIteratorClass, "hasNext", "()Z");
#else
  scIteratorClass = (*env)->FindClass(env, "java/util/Iterator");
  hasNext_mid = (*env)->GetMethodID(env, scIteratorClass, "hasNext", "()Z");
#endif

#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 2 (%d)\n",hasNext_mid);
#endif

#ifdef JAVA7
  next_mid = (*env).GetMethodID(scIteratorClass, "next", "()Ljava/lang/Object;");
#else
  next_mid = (*env)->GetMethodID(env, scIteratorClass, "next", "()Ljava/lang/Object;");
#endif
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 3 (%d)\n",next_mid);
#endif

#ifdef JAVA7
  scMapEntryClass = (*env).FindClass("java/util/Map$Entry");
  getKey_mid = (*env).GetMethodID(scMapEntryClass, "getKey", "()Ljava/lang/Object;");
#else
  scMapEntryClass = (*env)->FindClass(env, "java/util/Map$Entry");
  getKey_mid = (*env)->GetMethodID(env, scMapEntryClass, "getKey", "()Ljava/lang/Object;");
#endif
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 4 (%d)\n",getKey_mid);
#endif
#ifdef JAVA7
  getValue_mid = (*env).GetMethodID(scMapEntryClass, "getValue", "()Ljava/lang/Object;");
#else
  getValue_mid = (*env)->GetMethodID(env, scMapEntryClass, "getValue", "()Ljava/lang/Object;");
#endif
#ifdef DEBUG
  fprintf(stderr,"mapsFromHashMap 5 (%d)\n",getValue_mid);
#endif

#ifdef JAVA7
  jobject final_set=(*env).CallObjectMethod(t,entrySet_mid);
  jobject final_iterator=(*env).CallObjectMethod(final_set,iterator_mid);
#else
  jobject final_set=(*env)->CallObjectMethod(env,t,entrySet_mid);
  jobject final_iterator=(*env)->CallObjectMethod(env,final_set,iterator_mid);
#endif

  maps* final_res=NULL;
  map* res=NULL;
#ifdef JAVA7
  while((*env).CallBooleanMethod(final_iterator,hasNext_mid)){
    jobject tmp=(*env).CallObjectMethod(final_iterator,next_mid);

    jobject imap=(*env).CallObjectMethod(tmp,getValue_mid);
    jobject set=(*env).CallObjectMethod(imap,entrySet_mid);
    jobject iterator=(*env).CallObjectMethod(set,iterator_mid);

#else
  while((*env)->CallBooleanMethod(env,final_iterator,hasNext_mid)){
    jobject tmp=(*env)->CallObjectMethod(env,final_iterator,next_mid);

    jobject imap=(*env)->CallObjectMethod(env,tmp,getValue_mid);
    jobject set=(*env)->CallObjectMethod(env,imap,entrySet_mid);
    jobject iterator=(*env)->CallObjectMethod(env,set,iterator_mid);

#endif
    int size=-1;
#ifdef JAVA7
    if((*env).CallBooleanMethod(imap,containsKey_mid,(*env).NewStringUTF("size"))){
      jobject sizeV=(*env).CallObjectMethod(imap, get_mid,(*env).NewStringUTF("size"));
      const char* sizeVS=(*env).GetStringUTFChars((jstring)sizeV, NULL);
#else
    if((*env)->CallBooleanMethod(env,imap,containsKey_mid,(*env)->NewStringUTF(env,"size"))){
      jobject sizeV=(*env)->CallObjectMethod(env, imap, get_mid,(*env)->NewStringUTF(env,"size"));
      const char* sizeVS=(*env)->GetStringUTFChars(env, sizeV, NULL);
#endif
      size=atoi(sizeVS);
      fprintf(stderr,"SIZE : %s\n",sizeVS);
#ifdef JAVA7
      (*env).ReleaseStringUTFChars((jstring)sizeV, sizeVS);
#else
      (*env)->ReleaseStringUTFChars(env, sizeV, sizeVS);
#endif
    }
    
#ifdef JAVA7
    while((*env).CallBooleanMethod(iterator,hasNext_mid)){
      jobject tmp1=(*env).CallObjectMethod(iterator,next_mid);
      jobject jk=(*env).CallObjectMethod(tmp1,getKey_mid);
      jobject jv=(*env).CallObjectMethod(tmp1,getValue_mid);

      const char* jkd=(*env).GetStringUTFChars((jstring)jk, NULL);
#else
    while((*env)->CallBooleanMethod(env,iterator,hasNext_mid)){
      jobject tmp1=(*env)->CallObjectMethod(env,iterator,next_mid);
      jobject jk=(*env)->CallObjectMethod(env,tmp1,getKey_mid);
      jobject jv=(*env)->CallObjectMethod(env,tmp1,getValue_mid);

      const char* jkd=(*env)->GetStringUTFChars(env, jk, NULL);
#endif
      if(size>=0 && strcmp(jkd,"value")==0){
#ifdef JAVA7
	jobject value=(jobject)(*env).GetByteArrayElements((jbyteArray)jv, NULL);
#else
	jobject value=(*env)->GetByteArrayElements(env, jv, NULL);
#endif
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
#ifdef JAVA7
	const char* jvd=(*env).GetStringUTFChars((jstring)jv, NULL);
#else
	const char* jvd=(*env)->GetStringUTFChars(env, jv, NULL);
#endif
	if(res==NULL){
	  res=createMap(jkd,jvd);
	}else{
	  addToMap(res,jkd,jvd);
	}
#ifdef JAVA7
	(*env).ReleaseStringUTFChars((jstring)jv, jvd);
#else
	(*env)->ReleaseStringUTFChars(env, jv, jvd);
#endif
      }

#ifdef JAVA7
      (*env).ReleaseStringUTFChars((jstring)jk, jkd);
#else
      (*env)->ReleaseStringUTFChars(env, jk, jkd);
#endif

    }
#ifdef JAVA7
    jobject jk=(*env).CallObjectMethod(tmp,getKey_mid);
#else
    jobject jk=(*env)->CallObjectMethod(env,tmp,getKey_mid);
#endif
    maps* cmap=(maps*)malloc(sizeof(maps));
#ifdef JAVA7
    cmap->name=(char*)(*env).GetStringUTFChars((jstring)jk, NULL);
#else
    cmap->name=(*env)->GetStringUTFChars(env, jk, NULL);
#endif
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
