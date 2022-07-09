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

//#define NODEJS_DEBUG
//#define ULINET_DEBUG
#define NAPI_EXPERIMENTAL

#include <fstream>
#include <regex>
#include <sstream>
#include <string>

#include "response_print.h"
#include "service_internal_nodejs.h"

#include "ulinet.h"

napi_platform platform = nullptr;

static const char *js_loader =
    "global.nativeRequire = require; global.require = require('module').createRequire(process.cwd() + '/');";

#define ARG_IS_STRING(env, arg)                                                                                        \
  if (!arg.IsString())                                                                                                 \
    throw Napi::TypeError::New(env, arg.ToString().Utf8Value() + " is not a string");
#define ARG_IS_OBJECT(env, arg)                                                                                        \
  if (!arg.IsObject())                                                                                                 \
    throw Napi::TypeError::New(env, arg.ToString().Utf8Value() + " is not an object");
#define ARG_IS_NUMBER(env, arg)                                                                                        \
  if (!arg.IsNumber())                                                                                                 \
    throw Napi::TypeError::New(env, arg.ToString().Utf8Value() + " is not an number");

static Napi::Object JSObject_FromMap(Napi::Env env, map *t);
static map *mapFromJSObject(Napi::Env env, Napi::Value t);

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
  while (tmp != nullptr) {
    Napi::Object obj = JSObject_FromMap(env, tmp->content);
    if (tmp->child != nullptr) {
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
static Napi::Object JSObject_FromMap(Napi::Env env, map *t) {
  Napi::EscapableHandleScope scope(env);

  Napi::Object res = Napi::Object::New(env);
  map *tmpm = t;
  map *isArray = getMap(t, "isArray");
  map *isBinary = getMap(t, "size");
  map *tmap = getMapType(t);
#ifdef NODEJS_DEBUG
  if (tmap == nullptr)
    fprintf(stderr, "tmap is null !\n");
  else
    fprintf(stderr, "tmap is not null ! (%s = %s)\n", tmap->name, tmap->value);
#endif
  while (isArray == nullptr && tmpm != nullptr) {
    Napi::Value jsstr;
    if (isBinary != nullptr && strncasecmp(tmpm->name, "value", 5) == 0)
      jsstr = Napi::String::New(env, tmpm->value, atoi(isBinary->value));
    else
      jsstr = Napi::String::New(env, tmpm->value);
    res.Set(tmpm->name, jsstr);
#ifdef NODEJS_DEBUG
    fprintf(stderr, "[JS] %s => %s\n", tmpm->name, tmpm->value);
#endif
    tmpm = tmpm->next;
  }
  if (isArray != nullptr) {
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
      if (tmpm1 != nullptr) {
        Napi::Value jsstr;
        if (tmpm3 != nullptr)
          jsstr = Napi::String::New(env, tmpm1->value, atoi(tmpm3->value));
        else
          jsstr = Napi::String::New(env, tmpm1->value);

        values.Set(i, jsstr);
      }
      if (tmpm2 != nullptr) {
        Napi::Value jsstr = Napi::String::New(env, tmpm2->value);
        mvalues.Set(i, jsstr);
      }
    }

    res.Set("value", values);
    res.Set(tmap->name, mvalues);

    while (tmpm != nullptr) {
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
static maps *mapsFromJSObject(Napi::Env env, Napi::Value t) {
  Napi::HandleScope scope(env);

  maps *res = nullptr;
  maps *tres = nullptr;
  if (!t.IsObject()) {
    throw Napi::TypeError::New(env, "JS value is not an object");
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
          throw Napi::TypeError::New(env, "Property name is not a string: " + _name.ToString().Utf8Value());
        }

        std::string name = _name.ToString().Utf8Value();
        tres = createMaps(name.c_str());

        Napi::Value value = obj.Get(name);
        if (value.IsBoolean() && value.ToBoolean() == false) {
#ifdef NODEJS_DEBUG
          fprintf(stderr, "Enumerate id : %ld => %s => No more value\n", index, name.c_str());
#endif
        }
        if (value.IsObject()) {
          tres->content = mapFromJSObject(env, value);
        } else if (value.IsString()) {
          tres->content = createMap(name.c_str(), value.ToString().Utf8Value().c_str());
        }

        if (res == nullptr)
          res = dupMaps(&tres);
        else
          addMapsToMaps(&res, tres);
        freeMaps(&tres);
        free(tres);
        tres = nullptr;
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
static map *mapFromJSObject(Napi::Env env, Napi::Value t) {
  Napi::HandleScope scope(env);

  map *res = nullptr;

  if (!t.IsObject()) {
    throw Napi::Error::New(env, "JS value is not an object");
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
        throw Napi::Error::New(env, "Property name is not a string: " + _name.ToString().Utf8Value());
      }
      std::string name = _name.ToString().Utf8Value();

      Napi::Value value = obj.Get(name);
      if (value.IsBoolean() && value.ToBoolean() == false) {
#ifdef NODEJS_DEBUG
        fprintf(stderr, "Enumerate id : %ld => %s => No more value\n", index, name.c_str());
#endif
      } else if (value.IsString()) {
        if (res != nullptr) {
#ifdef NODEJS_DEBUG
          fprintf(stderr, "%s - %s\n", name.c_str(), value.ToString().Utf8Value().c_str());
#endif
          addToMap(res, name.c_str(), value.ToString().Utf8Value().c_str());
        } else {
          res = createMap(name.c_str(), value.ToString().Utf8Value().c_str());
          res->next = nullptr;
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
 * The function used as ZOOUpdateStatus from the JavaScript environment
 * (ZOO-API).
 *
 * @param conf Configuration object
 * @param status Completion
 * @return true
 * @see setHeader,_updateStatus
 */
static Napi::Boolean ZOOUpdateStatus(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  Napi::EscapableHandleScope scope(env);

  if (info.Length() > 2) {
#ifdef NODEJS_DEBUG
    fprintf(stderr, "Number of arguments used to call the function : %ld", info.Length());
#endif
    throw Napi::Error::New(env, "Too many arguments in call to ZOOUpdateStatus");
  }

  ARG_IS_OBJECT(env, info[0]);
  maps *conf = mapsFromJSObject(env, info[0]);

  ARG_IS_NUMBER(env, info[1]);
  std::string status = std::to_string(info[1].ToNumber().Int32Value());

#ifdef NODEJS_DEBUG
  fprintf(stderr, "ZOOUpdateStatus: %s\n", status.c_str());
#endif
  if (getMapFromMaps(conf, "lenv", "status") != nullptr) {
    if (status.size() > 0) {
      setMapInMaps(conf, "lenv", "status", status.c_str());
    } else {
      setMapInMaps(conf, "lenv", "status", "15");
    }
    _updateStatus(conf);
  }
  freeMaps(&conf);
  free(conf);
  return scope.Escape(Napi::Boolean::New(env, true)).ToBoolean();
}

/**
 * The function used as ZOOTranslate from the JavaScript environment.
 * Use the ZOO-Services messages translation function from the Python
 * environment (ZOO-API)
 *
 * @param str String to translate
 * @return Translated string
 */
static Napi::String ZOOTranslate(const Napi::CallbackInfo &info) {
  Napi::Env env(info.Env());
  Napi::EscapableHandleScope scope(env);

  if (info.Length() != 1 || !info[0].IsString())
    throw Napi::Error::New(env, "Invalid argument for ZOOTranslate");

  std::string str = info[0].ToString().Utf8Value();
  return scope.Escape(Napi::String::New(env, _ss(str.c_str()))).ToString();
}

/**
 * Set the HTTP header of a request
 *
 * @param handle the HINTERNET handle
 * @param env the JavaScript context
 * @param header the JavaScript Array containing the headers to send
 * @return the HINTERNET handle
 */
static HINTERNET setHeader(HINTERNET *handle, Napi::Env env, Napi::Array header) {
#ifdef ULINET_DEBUG
  fprintf(stderr, "setHeader\n");
#endif
  size_t length = header.Length();
#ifdef ULINET_DEBUG
  fprintf(stderr, "header is an array of %ld elements\n", length);
#endif
  handle->ihandle[handle->nb].header = nullptr;
  for (size_t i = 0; i < length; i++) {
    Napi::Value val = header.Get(i);
    if (!val.IsString())
      throw Napi::TypeError::New(env, val.ToString().Utf8Value() + " is not a string");
    std::string s = val.ToString().Utf8Value();
#ifdef ULINET_DEBUG
    curl_easy_setopt(handle->ihandle[handle->nb].handle, CURLOPT_VERBOSE, 1);
    fprintf(stderr, "Element of array n° %ld, value : %s\n", i, s.c_str());
#endif
    handle->ihandle[handle->nb].header = curl_slist_append(handle->ihandle[handle->nb].header, s.c_str());
  }
  return *handle;
}

/**
 * The function used as ZOORequest from the JavaScript environment (ZOO-API)
 *
 * @param method Optional method or otherwise GET
 * @param url URL
 * @param body Optional body
 * @param headers Optional headers
 * @return true
 * @see setHeader
 */
static Napi::Value ZOORequest(const Napi::CallbackInfo &info) {
  Napi::Env env(info.Env());
  Napi::EscapableHandleScope scope(env);

  HINTERNET hInternet;
  Napi::Object header;
  std::string url;
  std::string method;
  std::string body;
  size_t dwRead;

  maps *tmpConf = createMaps("main");
  tmpConf->content = createMap("memory", "load");
  hInternet = InternetOpen(const_cast<char *>("ZooWPSClient\0"), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);

  if (!CHECK_INET_HANDLE(hInternet))
    return scope.Escape(Napi::Boolean::New(env, false));

  if (info.Length() >= 2) {
    ARG_IS_STRING(env, info[0]);
    method = info[0].ToString().Utf8Value();
    ARG_IS_STRING(env, info[1]);
    url = info[1].ToString().Utf8Value();
  } else {
    method = std::string("GET");
    ARG_IS_STRING(env, info[0]);
    url = info[0].ToString().Utf8Value();
  }
  hInternet.waitingRequests[hInternet.nb] = strdup(url.c_str());

  if (info.Length() == 4) {
    std::string body;
    ARG_IS_STRING(env, info[2])
    body = info[2].ToString().Utf8Value();
    ARG_IS_OBJECT(env, info[3]);
    header = info[3].ToObject();
#ifdef ULINET_DEBUG
    fprintf(stderr, "URL (%s) \nBODY (%s)\n", url.c_str(), body.c_str());
#endif
    if (header.IsArray())
      setHeader(&hInternet, env, header.As<Napi::Array>());
#ifdef ULINET_DEBUG
    fprintf(stderr, "BODY (%s)\n", body.c_str());
#endif
    InternetOpenUrl(&hInternet, hInternet.waitingRequests[hInternet.nb], const_cast<char *>(body.c_str()),
                    body.length(), INTERNET_FLAG_NO_CACHE_WRITE, 0, tmpConf);
    processDownloads(&hInternet);
  } else {
    if (info.Length() == 3) {
      if (method == "GET") {
        ARG_IS_OBJECT(env, info[2]);
        header = info[2].ToObject();
        if (header.IsArray()) {
          setHeader(&hInternet, env, header.As<Napi::Array>());
        } else {
          throw Napi::TypeError::New(env, "header must be an array");
        }
        InternetOpenUrl(&hInternet, hInternet.waitingRequests[hInternet.nb], nullptr, 0, INTERNET_FLAG_NO_CACHE_WRITE,
                        0, tmpConf);
        processDownloads(&hInternet);
      } else {
        ARG_IS_OBJECT(env, info[2]);
        body = info[2].ToString();
        InternetOpenUrl(&hInternet, hInternet.waitingRequests[hInternet.nb], const_cast<char *>(body.c_str()),
                        body.length(), INTERNET_FLAG_NO_CACHE_WRITE, 0, tmpConf);
        processDownloads(&hInternet);
      }
    } else {
      InternetOpenUrl(&hInternet, hInternet.waitingRequests[hInternet.nb], nullptr, 0, INTERNET_FLAG_NO_CACHE_WRITE, 0,
                      tmpConf);
      processDownloads(&hInternet);
    }
  }
  char *tmpValue = (char *)malloc((hInternet.ihandle[0].nDataLen + 1) * sizeof(char));
  InternetReadFile(hInternet.ihandle[0], (LPVOID)tmpValue, hInternet.ihandle[0].nDataLen, &dwRead);
#ifdef ULINET_DEBUG
  fprintf(stderr, "content downloaded (%ld) (%s) \n", dwRead, tmpValue);
#endif
  if (dwRead == 0) {
    return scope.Escape(Napi::String::New(env, "Unable to access the file."));
  }

#ifdef ULINET_DEBUG
  fprintf(stderr, "content downloaded (%ld) (%s) \n", dwRead, tmpValue);
#endif

  freeMaps(&tmpConf);
  free(tmpConf);
  InternetCloseHandle(&hInternet);
  return scope.Escape(Napi::String::New(env, tmpValue));
}

static const char *ZooStatus[] = {"SERVICE_ACCEPTED", "SERVICE_STARTED", "SERVICE_PAUSED", "SERVICE_SUCCEEDED",
                                  "SERVICE_FAILED"};

/**
 * Construct the ZOO environment
 *
 * @param env The JavaScript context
 */
static void CreateZOOEnvironment(Napi::Env env) {
  Napi::HandleScope scope(env);
  auto ZOOUpdateStatusRef = Napi::Function::New(env, ZOOUpdateStatus, "ZOOUpdateStatus");
  auto ZOOTranslateRef = Napi::Function::New(env, ZOOTranslate, "ZOOTranslate");
  auto ZOORequestRef = Napi::Function::New(env, ZOORequest, "ZOORequest");

  for (size_t i = 0; i < sizeof(ZooStatus) / sizeof(ZooStatus[0]); i++)
    env.Global().Set(ZooStatus[i], i);
  env.Global().Set("ZOOUpdateStatus", ZOOUpdateStatusRef);
  env.Global().Set("ZOOTranslate", ZOOTranslateRef);
  env.Global().Set("ZOORequest", ZOORequestRef);
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
    char *argv[5];
    int argc = 0;
    map *inspector = getMap(s->content, "inspector");
    if (inspector != nullptr && inspector->value != nullptr && !strncmp(inspector->value, "true", strlen("true"))) {
      argv[argc++] = const_cast<char *>("--inspect");
      argv[argc++] = const_cast<char *>("--inspect-brk");
    }

    if (napi_create_platform(argc, argv, 0, nullptr, nullptr, 0, &platform) != napi_ok) {
      errorException(*main_conf, "Failed initializing Node.js/V8", "NoApplicableCode", nullptr);
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
    std::string err = "Failed opening " + full_path;
    errorException(*main_conf, full_path.c_str(), "NoApplicableCode", nullptr);
    return -1;
  }

  std::ifstream js_script(full_path);
  std::stringstream js_text;
  js_text << js_script.rdbuf();

  if (napi_create_environment(platform, nullptr, js_loader, &_env) != napi_ok) {
    errorException(*main_conf, "Failed creating environment", "NoApplicableCode", nullptr);
    return -1;
  }

  int ret = -1;
  {
    Napi::Env env(_env);
    Napi::HandleScope scope(env);

    try {
      CreateZOOEnvironment(env);

      Napi::Function nativeRequire = env.Global().Get("nativeRequire").As<Napi::Function>();
      Napi::Object vm = nativeRequire.Call({Napi::String::New(env, "vm")}).ToObject();
      vm.Get("runInThisContext").As<Napi::Function>().Call(vm, {Napi::String::New(env, js_text.str())});

      Napi::Value _fn = env.Global().Get(s->name);
      if (!_fn.IsFunction()) {
        std::string err = std::string(s->name) + " is not a function: " + _fn.ToString().Utf8Value();
        errorException(*main_conf, err.c_str(), "NoApplicableCode", nullptr);
        return ret;
      }
      Napi::Function fn = _fn.As<Napi::Function>();

      Napi::Object js_main_conf = JSObject_FromMaps(env, *main_conf);
      Napi::Object js_inputs = JSObject_FromMaps(env, *inputs);
      Napi::Object js_outputs = JSObject_FromMaps(env, *outputs);

      Napi::Value status = fn.Call({js_main_conf, js_inputs, js_outputs});

      *outputs = mapsFromJSObject(env, js_outputs);

      if (status.IsNumber()) {
        ret = status.ToNumber().Int32Value();
      } else {
        std::string err = "Service returned a non-number value: " + status.ToString().Utf8Value();
        errorException(*main_conf, err.c_str(), "NoApplicableCode", nullptr);
      }
    } catch (const Napi::Error &e) {
      std::string err = "Caught a JS exception: " + e.Message();
      errorException(*main_conf, err.c_str(), "NoApplicableCode", nullptr);
    }
  }

  if (napi_destroy_environment(_env, nullptr) != napi_ok) {
    errorException(*main_conf, "Failed destroying JS environment", "NoApplicableCode", nullptr);
  }

  return ret;
}
