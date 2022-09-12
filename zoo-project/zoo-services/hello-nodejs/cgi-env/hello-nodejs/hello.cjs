/**
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

const assert = require('assert');
const proj4 = require('proj4');
const axios = require('axios');

function hellonodejs_cjs(conf, inputs, outputs) {
  ZOOUpdateStatus(conf, 0);
  outputs["result"]["value"] = "Hello " + inputs["S"]["value"] + " from the JS World !";
  ZOOUpdateStatus(conf, 100);

  assert(proj4.defs.GOOGLE.projName === 'merc');
  assert(typeof axios.get === 'function');

  alert(`ZOOTranslate ${ZOOTranslate("my error")}`);

  ZOORequest('https://www.google.com');
  ZOORequest('GET', 'https://www.google.com');
  ZOORequest('POST', 'https://www.google.com', 'formdata', ['Accept-Encoding: gzip, deflate']);
  ZOORequest('GET', 'https://www.google.com', ['Accept-Encoding: gzip, deflate']);

  return SERVICE_SUCCEEDED;
}

module.exports = { hellonodejs_cjs };
