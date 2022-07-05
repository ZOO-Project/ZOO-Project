/*
 * Author : Momtchil MOMTCHEV
 *
 * Copyright 2022 Google (as part of GSoC 2022). All rights reserved.
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

#define NAPI_DISABLE_CPP_EXCEPTIONS
//#define NODE_ADDON_API_ENABLE_MAYBE
#define NODEJS_DEBUG
#define NAPI_EXPERIMENTAL

#include "service_internal_nodejs.h"
#include "response_print.h"
#include <js_native_api.h>

napi_platform platform = nullptr;

static const char *testScript =
    "function hello_nodejs() { outputs['result']['value']='Hello '+inputs['S']['value']+' from the JS World !'; }";

static int NodeJSInit() {
#ifdef NODEJS_DEBUG
  fprintf(stderr, "libnode init\n");
#endif
  if (napi_create_platform(0, NULL, 0, NULL, NULL, 0, &platform) != napi_ok) {
    fprintf(stderr, "Failed creating the platform\n");
    return -1;
  }

  return 0;
}

static int JSLoadScripts() {
#ifdef NODEJS_DEBUG
  fprintf(stderr, "libnode create environment\n");
#endif
  napi_env env;

  if (napi_create_environment(platform, NULL, testScript, &env) != napi_ok) {
    fprintf(stderr, "Failed running JS\n");
    return -1;
  }

  if (napi_run_environment(env) != napi_ok) {
    fprintf(stderr, "Failed flushing pending JS callbacks\n");
    return -1;
  }

  if (napi_destroy_environment(env, NULL) != napi_ok) {
    return -1;
  }

  return 0;
}

/**
 * Convert a maps to a JavaScript Object
 *
 * @param env the JavaScript context
 * @param t the maps to convert
 * @return a new JavaScript Object
 */
static Napi::Object JSObject_FromMaps(Napi::Env env, maps *t) {
  Napi::EscapableHandleScope scope(env);

  Napi::Object res = Napi::Object::New(env);

  maps *tmp = t;
  while (tmp != NULL) {
    Napi::Object obj = JSObject_FromMap(env, tmp->content);
    if (tmp->child != NULL) {
      Napi::Object child = JSObject_FromMaps(env, tmp->child);
      obj.Set("child", child);
    }
    obj.Set(tmp->name, obj);
#ifdef NODEJS_DEBUG
    fprintf(stderr, "Object : %s added \n", tmp->name);
#endif
    tmp = tmp->next;
  }
  return scope.Escape(res).ToObject();
}

/**
 * Convert a map to a JavaScript Object
 *
 * @param env the JavaScript context
 * @param t the map to convert
 * @return a new JavaScript Object
 */
Napi::Object JSObject_FromMap(Napi::Env env, map *t) {
  Napi::EscapableHandleScope scope(env);

  Napi::Object res = Napi::Object::New(env);
  map *tmpm = t;
  map *isArray = getMap(t, "isArray");
  map *isBinary = getMap(t, "size");
  map *tmap = getMapType(t);
#ifdef NODEJS_DEBUG
  if (tmap == NULL)
    fprintf(stderr, "tmap is null !\n");
  else
    fprintf(stderr, "tmap is not null ! (%s = %s)\n", tmap->name, tmap->value);
#endif
  while (isArray == NULL && tmpm != NULL) {
    Napi::Value jsstr;
    if (isBinary != NULL && strncasecmp(tmpm->name, "value", 5) == 0)
      jsstr = Napi::String::New(env, tmpm->value, atoi(isBinary->value));
    else
      jsstr = Napi::String::New(env, tmpm->value);
    res.Set(tmpm->name, jsstr);
#ifdef NODEJS_DEBUG
    fprintf(stderr, "[JS] %s => %s\n", tmpm->name, tmpm->value);
#endif
    tmpm = tmpm->next;
  }
  if (isArray != NULL) {
    map *len = getMap(t, "length");
    int cnt = atoi(len->value);
    Napi::Object values = Napi::Array::New(env);
    Napi::Object mvalues = Napi::Array::New(env);
    map *tmpm1, *tmpm2, *tmpm3;
    int i = 0;
    for (i = 0; i < cnt; i++) {
      tmpm1 = getMapArray(t, "value", i);
      tmpm2 = getMapArray(t, tmap->name, i);
      tmpm3 = getMapArray(t, "size", i);
      if (tmpm1 != NULL) {
        Napi::Value jsstr;
        if (tmpm3 != NULL)
          jsstr = Napi::String::New(env, tmpm1->value, atoi(tmpm3->value));
        else
          jsstr = Napi::String::New(env, tmpm1->value);

        values.Set(i, jsstr);
      }
      if (tmpm2 != NULL) {
        Napi::Value jsstr = Napi::String::New(env, tmpm2->value);
        mvalues.Set(i, jsstr);
      }
    }

    res.Set("value", values);
    res.Set(tmap->name, mvalues);

    while (tmpm != NULL) {
      if (strncasecmp(tmpm->name, "value", 5) != 0 && strncasecmp(tmpm->name, "size", 4) != 0 &&
          strncasecmp(tmpm->name, tmap->name, strlen(tmap->name)) != 0) {
        Napi::Value jsstr = Napi::String::New(env, tmpm->value);

        res.Set(tmpm->name, jsstr);
      }
#ifdef NODEJS_DEBUG
      fprintf(stderr, "[JS] %s => %s\n", tmpm->name, tmpm->value);
#endif
      tmpm = tmpm->next;
    }
  }
  return scope.Escape(res).ToObject();
}

/**
 * Load a JavaScript file then run the function corresponding to the service by
 * passing the conf, inputs and outputs parameters by value as JavaScript
 * Objects.
 *
 * @param main_conf the conf maps containing the main.cfg settings
 * @param request the map containing the HTTP request
 * @param s the service structure
 * @param inputs the maps containing the inputs
 * @param outputs the maps containing the outputs
 * @return SERVICE_SUCCEEDED or SERVICE_FAILED if the service run, -1
 *  if the service failed to load or throw error at runtime.
 */
extern "C" int zoo_nodejs_support(maps **main_conf, map *request, service *s, maps **inputs, maps **outputs) {
  if (platform == nullptr) {
    int r = NodeJSInit();
    if (r != 0)
      return r;
  }

  return JSLoadScripts();
}
