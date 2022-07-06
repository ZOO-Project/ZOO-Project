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

#define NODEJS_DEBUG
#define NAPI_EXPERIMENTAL

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include "response_print.h"
#include "service_internal_nodejs.h"

napi_platform platform = nullptr;

static const char *js_loader_prologue = "require('vm').runInThisContext(`";
static const char *js_loader_epilogue = "`);";

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
    res.Set(tmp->name, obj);
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
 * Convert a JavaScript Object to a maps
 *
 * @param env the JavaScript context
 * @param t the JavaScript Object to convert
 * @return a new maps containing the JavaScript Object
 */
maps *mapsFromJSObject(Napi::Env env, Napi::Value t) {
  Napi::HandleScope scope(env);

  maps *res = NULL;
  maps *tres = NULL;
  if (!t.IsObject()) {
    fprintf(stderr, "JS value is not an object");
    return NULL;
  }
  Napi::Object obj = t.ToObject();

  if (obj.IsArray()) {
#ifdef NODEJS_DEBUG
    fprintf(stderr, "Is finally an array !\n");
#endif
  } else {
#ifdef NODEJS_DEBUG
    fprintf(stderr, "Is not an array !\n");
#endif
    Napi::Array propNames = obj.GetPropertyNames();
    if (propNames.Length() > 0) {
      size_t index, argNum = propNames.Length();
#ifdef NODEJS_DEBUG
      fprintf(stderr, "Properties length :  %lu\n", argNum);
#endif

      for (index = 0, argNum = propNames.Length(); index < argNum; index++) {
        Napi::Value _name = propNames.Get(index);

        if (!_name.IsString()) {
          fprintf(stderr, "Property name is not a string: %s\n", _name.ToString());
          return NULL;
        }

        std::string name = _name.ToString().Utf8Value();
        tres = createMaps(name.c_str());

        Napi::Value value = obj.Get(name);
        if (value.IsBoolean() && value.ToBoolean() == false) {
#ifdef NODEJS_DEBUG
          fprintf(stderr, "Enumerate id : %d => %s => No more value\n", index, name.c_str());
#endif
        }
        if (value.IsObject()) {
          tres->content = mapFromJSObject(env, value);
        } else if (value.IsString()) {
          tres->content = createMap(name.c_str(), value.ToString().Utf8Value().c_str());
        }

        if (res == NULL)
          res = dupMaps(&tres);
        else
          addMapsToMaps(&res, tres);
        freeMaps(&tres);
        free(tres);
        tres = NULL;
      }
    }
  }

#ifdef NODEJS_DEBUG
  dumpMaps(res);
#endif
  return res;
}

/**
 * Convert a JavaScript Object to a map
 *
 * @param env the JavaScript context
 * @param t the JavaScript Object to convert
 * @return a new map containing the JavaScript Object
 */
map *mapFromJSObject(Napi::Env env, Napi::Value t) {
  Napi::HandleScope scope(env);

  map *res = NULL;

  if (!t.IsObject()) {
    fprintf(stderr, "JS value is not an object");
    return NULL;
  }
  Napi::Object obj = t.ToObject();

  Napi::Array propNames = obj.GetPropertyNames();
  if (propNames.Length() > 0) {
    size_t index, argNum = propNames.Length();
#ifdef NODEJS_DEBUG
    fprintf(stderr, "Properties length :  %lu\n", argNum);
#endif

    for (index = 0, argNum = propNames.Length(); index < argNum; index++) {
      Napi::Value _name = propNames.Get(index);

      if (!_name.IsString()) {
        fprintf(stderr, "Property name is not a string: %s\n", _name.ToString());
        return NULL;
      }
      std::string name = _name.ToString().Utf8Value();

      Napi::Value value = obj.Get(name);
      if (value.IsBoolean() && value.ToBoolean() == false) {
#ifdef NODEJS_DEBUG
        fprintf(stderr, "Enumerate id : %d => %s => No more value\n", index, name.c_str());
#endif
      } else if (value.IsString()) {
        if (res != NULL) {
#ifdef NODEJS_DEBUG
          fprintf(stderr, "%s - %s\n", name.c_str(), value.ToString().Utf8Value().c_str());
#endif
          addToMap(res, name.c_str(), value.ToString().Utf8Value().c_str());
        } else {
          res = createMap(name.c_str(), value.ToString().Utf8Value().c_str());
          res->next = NULL;
        }
      }
    }
#ifdef NODEJS_DEBUG
    dumpMap(res);
#endif
  }
  return res;
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
#ifdef NODEJS_DEBUG
    fprintf(stderr, "libnode init\n");
#endif
    if (napi_create_platform(0, NULL, 0, NULL, NULL, 0, &platform) != napi_ok) {
      fprintf(stderr, "Failed creating the platform\n");
      return -1;
    }
  }

#ifdef NODEJS_DEBUG
  fprintf(stderr, "libnode create environment\n");
#endif
  napi_env _env;

  map *path_meta = getMap(request, "metapath");
  map *path_service = getMap(s->content, "serviceProvider");

  std::string full_path;
  map *cwdMap = getMapFromMaps(*main_conf, "main", "servicePath");
  if (cwdMap != nullptr)
    full_path =
        std::string(cwdMap->value) + '/' + std::string(path_meta->value) + '/' + std::string(path_service->value);
  else {
    char *cwd = get_current_dir_name();
    full_path = std::string(cwd) + '/' + std::string(path_meta->value) + '/' + std::string(path_service->value);
    free(cwd);
  }

  struct stat js_stat;
  if (stat(full_path.c_str(), &js_stat) != 0) {
    fprintf(stderr, "Failed opening %s\n", full_path.c_str());
    return -1;
  }

  std::ifstream js_scipt(full_path);
  std::stringstream js_text;
  js_text << js_loader_prologue << js_scipt.rdbuf() << js_loader_epilogue;
  std::string js_escaped = std::regex_replace(js_text.str(), std::regex("`"), "\`");

  if (napi_create_environment(platform, NULL, js_escaped.c_str(), &_env) != napi_ok) {
    fprintf(stderr, "Failed running JS\n");
    return -1;
  }

  int ret;
  {
    Napi::Env env(_env);
    Napi::HandleScope scope(env);

    try {
      Napi::Value _fn = env.Global().Get(s->name);
      if (!_fn.IsFunction()) {
        fprintf(stderr, "%s is not a function: %s\n", s->name, _fn.ToString().Utf8Value().c_str());
        return -1;
      }
      Napi::Function fn = _fn.As<Napi::Function>();

      Napi::Object js_main_conf = JSObject_FromMaps(env, *main_conf);
      Napi::Object js_inputs = JSObject_FromMaps(env, *inputs);
      Napi::Object js_outputs = JSObject_FromMaps(env, *outputs);

      Napi::Value status = fn.Call({js_main_conf, js_inputs, js_outputs});

      *outputs = mapsFromJSObject(env, js_outputs);

      if (!status.IsNumber()) {
        fprintf(stderr, "Service returned a non-number value: %s\n", status.ToString().Utf8Value().c_str());
        return -1;
      }
      ret = status.ToNumber().Int32Value();
    } catch (const Napi::Error &e) {
      fprintf(stderr, "Caught a JS exception: %s\n", e.what());
    }
  }

  if (napi_destroy_environment(_env, NULL) != napi_ok) {
    fprintf(stderr, "Failed destroying JS environment\n");
    ret = -1;
  }

  return ret;
}
