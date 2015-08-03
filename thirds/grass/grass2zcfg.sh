#!/bin/bash
#
# Author : Gérald FENOY
# 
# Copyright 2015 GeoLabs SARL. All rights reserved.
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




# Set below the location of the xsl file 
XSL=/Users/djay/MapMint/zoo-project-1.5.0/trunk/thirds/grass/xml2zcfg.xsl

# Set below the location of your GRASS 7.0.X installation
GRASSROOT=/Applications/GRASS-7.0.app/Contents/MacOS

for i in $(ls $GRASSROOT/bin/{v,r}.* | grep -v "v\.in" | grep -v "v\.out" | grep -v "r\.in" | grep -v "r\.out" ); 
do 
    j=$(echo $i | sed "s:$GRASSROOT/bin/::g")
    zcfg=$(echo $j | sed "s:\.:_:g")
    $i --wps-process-description  > $zcfg.xml
    xsltproc $XSL $zcfg.xml > $zcfg.zcfg
    rm $zcfg.xml
    echo "#####################################################
# This service was generated using wps-grass-bridge #
#####################################################
import ZOOGrassModuleStarter as zoo
def $zcfg(m, inputs, outputs):
    service = zoo.ZOOGrassModuleStarter()
    service.fromMaps(\"$j\", inputs, outputs)
    return 3
" > $zcfg.py
done
