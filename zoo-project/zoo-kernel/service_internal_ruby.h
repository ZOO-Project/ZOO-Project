/**
 * Author : Gérald FENOY
 *
 * Copyright (c) 2014 GeoLabs SARL
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

#ifndef ZOO_SERVICE_INTERNAL_RUBY_H
#define ZOO_SERVICE_INTERNAL_RUBY_H 1

#pragma once 

#define RUBY_GLOBAL_SETUP
#undef NEED_STRLCAT
#undef NEED_STRLCPY
#include "service.h"
#include "service_internal.h"
#undef RUBY_EXPORT
#undef STRCASECMP
#undef STRNCASECMP
#undef _
#include <ruby.h>
#if ZRUBY_VERSION < 2
#include <version.h>
#else
#include <ruby/version.h>
#endif
#if RUBY_VERSION_MINOR==8
#include <st.h>
#else
#include <ruby/st.h>
#endif

#define MAX_ARGS 3
struct my_callback {
  VALUE obj;
  ID method_id;
  int nargs;
  VALUE args[MAX_ARGS];
};

void ruby_trace_error(maps*);
VALUE FunCallWrap(VALUE);
VALUE LoadWrap(VALUE);

VALUE RubyHash_FromMaps(maps*);
VALUE RubyHash_FromMap(map*);

maps* mapsFromRubyHash(VALUE);
void createMapsFromRubyHash(maps**,VALUE);
map* mapFromRubyHash(VALUE);

int zoo_ruby_support(maps**,map*,service*,maps**,maps**);

VALUE RubyTranslate(int,VALUE*,VALUE);
VALUE RubyUpdateStatus(int,VALUE*,VALUE);

#endif
