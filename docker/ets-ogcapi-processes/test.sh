#!/bin/bash
#
# Author : Gérald FENOY
#
# Copyright 2023 GeoLabs SARL. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

export TE_BASE=/root/te_base/
java -jar $(find /root -name "ets-ogcapi-processes10-*aio.jar") -o /tmp /root/test-run-ogcapi-processes-1.xml
cat $(find /tmp -name "*results.xml")
echo Success: $(grep PASS $(find /tmp -name "*results.xml") | wc -l)
echo Failed: $(grep FAIL $(find /tmp -name "*results.xml") | grep -v FAILURE | wc -l)

mkdir /tmp/tmp
java -jar $(find /root -name "ets-wps20-*aio.jar") -o /tmp/tmp /root/test-run-wps20.xml
cat $(find /tmp/tmp -name "*results.xml")
echo Success: $(grep PASS $(find /tmp/tmp -name "*results.xml") | wc -l)
echo Failed: $(grep FAIL $(find /tmp/tmp -name "*results.xml") | grep -v FAILURE | wc -l)
