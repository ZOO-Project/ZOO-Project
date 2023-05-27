#!/bin/bash
#
# Author : Gérald FENOY
#
# Copyright 2022-2023 GeoLabs SARL. All rights reserved.
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

for j in cgi-env/*zcfg ;
  do
    for i in Title Abstract;
     do
      grep $i $j | sed "s:$i = :_ss(\":g;s:\r:\"):g" ;
     done;
 done > service.c

grep -A 10 links_title ../../../../docker/oas.cfg | grep = | cut -d'=' -f2 | sed 's:V:_ss("V:g;s:$:"):g'  >> service.c 

xgettext --package-name="ZOO-Service OGC API - Processes services" \
	--package-version=1.9.0 \
	--copyright-holder="GeoLabs" \
	service.c templates/index.html -k_ss -k_ -o message.po

sed "s:SOME DESCRIPTIVE TITLE.:ZOO-Service OGC API - Processes services:g" message.po > locale/po/messages.po
rm message.po
rm service.c
