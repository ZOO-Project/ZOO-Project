/*
 * Author : Gérald FENOY
 *
 * Copyright (c) 2009-2010 GeoLabs SARL
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

#ifndef ZOO_SERVICE_INTERNAL_R_H
#define ZOO_SERVICE_INTERNAL_R_H 1

#pragma once 

#include <Rinternals.h>
#include <Rembedded.h>
#include "service_internal.h"
#include "response_print.h"
#include "cgic.h"

SEXP RList_FromMaps(maps* t);
SEXP RList_FromMap(map* t);

maps* mapsFromRList(SEXP t);
map* mapFromRList(SEXP t);

int zoo_r_support(maps**,map*,service*,maps**,maps**);

SEXP RTranslate(SEXP);
SEXP RUpdateStatus(SEXP,SEXP);

#endif
