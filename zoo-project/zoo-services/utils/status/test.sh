#!/bin/bash
#
# Author : Gérald FENOY
#
# Copyright 2011 GeoLabs SARL. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including with
# out limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

rm -f log log1

./zoo_loader.cgi "request=Execute&service=WPS&version=1.0.0&Identifier=longProcess&DataInputs=&storeExecuteResponse=true&status=true" > log

if [ -z "$(grep "ows:ExceptionReport" log)" ]; then
    while [ -z "$(grep "wps:ProcessSucceeded" log1)" ]; 
    do 
	./zoo_loader.cgi $(grep statusLocation= ./log | cut -d'?' -f2 | cut -d'"' -f1 | sed "s:amp;::g") > log1 ;
	cat log1 ; 
    done
    cat log1
else
    echo "Service failed, please make sure that your main.cfg file contains"
    echo "in the [main] section valid values for both tmpPath and dataPath."
    echo 
    cat log
fi
