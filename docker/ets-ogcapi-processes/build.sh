#!/usr/bin/env bash
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

cd docker/ets-ogcapi-processes
curl -L -o 5.5.2.zip https://github.com/opengeospatial/teamengine/archive/refs/tags/5.5.2.zip
unzip 5.5.2.zip
mv teamengine-5.5.2 src

git clone https://github.com/opengeospatial/ets-common.git src1
cd src1
git clone https://github.com/opengeospatial/ets-ogcapi-processes10.git
git clone https://github.com/opengeospatial/ets-wps20.git

cd ../../..

docker build . -f docker/ets-ogcapi-processes/Dockerfile --progress plain -t zooproject/ets-ogcapi-processes10:latest

sed "s/localhost/zookernel/g" -i docker/*.cfg
sed "s=<organization>=<config><organization>=g;s=</organization>=</organization></config>=g" -i docker/ets-ogcapi-processes/src1/ets-ogcapi-processes10/src/main/config/teamengine/config.xml
