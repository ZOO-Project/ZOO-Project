/*
 * Author : Gérald Fenoy
 *
 *  Copyright 2016-2023 GeoLabs SARL. All rights reserved.
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
