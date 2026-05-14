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
git clone https://github.com/opengeospatial/teamengine.git src

git clone https://github.com/opengeospatial/ets-common.git src1
#Cannot clone the repo anymore, better to work with a target version (1.2)
git clone https://github.com/opengeospatial/ets-ogcapi-processes10-part2.git src1/ets-ogcapi-processes10-part2

docker build . -f src1/ets-ogcapi-processes10-part2/Dockerfile --progress plain -t zooproject/ets-ogcapi-processes10-part2:latest

cd ../..

curl -L -o test.cwl https://github.com/Terradue/ogc-eo-application-package-hands-on/releases/download/1.5.0/app-water-bodies-cloud-native.1.5.0.cwl

curl -X POST -H "Content-Type: application/cwl+yaml" --data-binary @test.cwl http://localhost:8080/anonymous/ogc-api/processes

docker run -v "$(pwd):/tmp" zooproject/ets-ogcapi-processes10-part2:latest /tmp/docker/ets-ogcapi-processes/test-part2.sh