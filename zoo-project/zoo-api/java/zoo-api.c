#include "ZOO.h"
#include "service.h"
#include "service_internal.h"
#include "service_internal_java.h"

JNIEXPORT jstring JNICALL Java_ZOO__1
  (JNIEnv *env, jclass class, jstring msg){
  const char *message = (*env)->GetStringUTFChars(env,msg, 0);
  return (*env)->NewStringUTF(env, _(message));

}

JNIEXPORT jobject JNICALL Java_ZOO_updateStatus
  (JNIEnv *env, jclass class, jobject conf, jstring pourc, jstring msg){

  jclass scHashMapClass,scHashMap_class;
#ifdef JAVA7
  scHashMapClass = (*env).FindClass("java/util/HashMap");
#else
  scHashMapClass = (*env)->FindClass(env, "java/util/HashMap");
#endif
  const char *pourcent = (*env)->GetStringUTFChars(env,pourc, 0);
  const char *message = (*env)->GetStringUTFChars(env,msg, 0);
  maps* m = mapsFromHashMap(env,conf,scHashMapClass);
  setMapInMaps(m,"lenv","status",pourcent);
  setMapInMaps(m,"lenv","message",message);
  _updateStatus(m);
  freeMaps(&m);
  free(m);
  return (jint)0;
}
