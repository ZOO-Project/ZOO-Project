#include "org_zoo_project_ZOO.h"
#include "service.h"
#include "service_internal.h"
#include "service_internal_java.h"

JNIEXPORT jstring JNICALL Java_org_zoo_1project_ZOO_translate
  (JNIEnv *env, jclass _class, jstring msg){
  const char *message = 
#ifdef JAVA7
    (*env).GetStringUTFChars(msg,0);
  return (*env).NewStringUTF(_(message));
#else
    (*env)->GetStringUTFChars(env,msg, 0);
  return (*env)->NewStringUTF(env, _(message));
#endif

}

JNIEXPORT jobject JNICALL Java_org_zoo_1project_ZOO_updateStatus
  (JNIEnv *env, jclass _class, jobject conf, jstring pourc, jstring msg){

  jclass scHashMapClass,scHashMap_class;
#ifdef JAVA7
  scHashMapClass = (*env).FindClass("java/util/HashMap");
  const char *pourcent = (*env).GetStringUTFChars(pourc,0);
  const char *message = (*env).GetStringUTFChars(msg,0);
#else
  scHashMapClass = (*env)->FindClass(env, "java/util/HashMap");
  const char *pourcent = (*env)->GetStringUTFChars(env,pourc, 0);
  const char *message = (*env)->GetStringUTFChars(env,msg, 0);
#endif
  maps* m = mapsFromHashMap(env,conf,scHashMapClass);
  setMapInMaps(m,"lenv","status",pourcent);
  setMapInMaps(m,"lenv","message",message);
  _updateStatus(m);
  freeMaps(&m);
  free(m);
  return (jint)0;
}
