/*
 * Author : Momtchil MOMTCHEV
 *
 * Copyright (c) 2022
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

#include "service_internal_nodejs.h"
#include "response_print.h"

#include <node.h>
#include <uv.h>

using node::CommonEnvironmentSetup;
using node::Environment;
using node::MultiIsolatePlatform;
using v8::Context;
using v8::HandleScope;
using v8::Isolate;
using v8::Locker;
using v8::MaybeLocal;
using v8::V8;
using v8::Value;

static int RunNodeInstance(MultiIsolatePlatform *platform, const std::vector<std::string> &args, const std::vector<std::string> &exec_args);

static std::vector<std::string> args = {"zoo_loader"};
static std::vector<std::string> exec_args = {};
static std::unique_ptr<MultiIsolatePlatform> platform = nullptr;

static const char *testScript = "console.log('hello world')";

int init() {
    std::vector<std::string> errors;
    int exit_code = node::InitializeNodeWithArgs(&args, &exec_args, &errors);
    for (const std::string &error : errors)
        fprintf(stderr, "%s\n", error.c_str());
    if (exit_code != 0) {
        return exit_code;
    }

    platform = MultiIsolatePlatform::Create(4);
    V8::InitializePlatform(platform.get());
    V8::Initialize();

    return 0;
}

int JSLoadScripts() {
    int exit_code = 0;

    std::vector<std::string> errors;
    std::unique_ptr<CommonEnvironmentSetup> setup = CommonEnvironmentSetup::Create(platform.get(), &errors, args, exec_args);
    if (!setup) {
        for (const std::string &err : errors)
            fprintf(stderr, "%s: %s\n", args[0].c_str(), err.c_str());
        return 1;
    }

    Isolate *isolate = setup->isolate();
    Environment *env = setup->env();

    {
        Locker locker(isolate);
        Isolate::Scope isolate_scope(isolate);
        HandleScope handle_scope(isolate);
        Context::Scope context_scope(setup->context());

        MaybeLocal<Value> loadenv_ret = node::LoadEnvironment(env, testScript);

        if (loadenv_ret.IsEmpty()) // There has been a JS exception.
            return 2;

        exit_code = node::SpinEventLoop(env).FromMaybe(1);
        node::Stop(env);
    }

    return exit_code;
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
        int r = init();
        if (r != 0)
            return r;
    }

    return JSLoadScripts();
}
