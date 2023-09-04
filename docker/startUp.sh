#!/bin/bash
#
# Author : Gérald FENOY
#
# Copyright 2021 GeoLabs SARL. All rights reserved.
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

mkdir -p /tmp/zTmp/statusInfos
cp /var/www/html/data/* /usr/com/zoo-project
chown www-data:www-data -R /tmp/zTmp /usr/com/zoo-project
chmod 777 -R /tmp/zTmp

CMD="curl -o toto.out http://rabbitmq:15672"
$CMD
cat toto.out
if [ -e toto.out ]; then echo "Should start" ; else echo wait; sleep 1; $CMD ; fi 

while [ ! -e toto.out ]; do echo wait; sleep 1; $CMD ;  done


echo "START FPM in 5 seconds"

sleep 5

cd /usr/lib/cgi-bin
./zoo_loader_fpm ./main.cfg 2> /var/log/zoofpm.log >> /var/log/zoofpm.log
