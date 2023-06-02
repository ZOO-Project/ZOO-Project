# -*- coding: utf-8 -*-
#
# Author : Gérald FENOY
#
# Copyright 2021-2023 GeoLabs SARL. All rights reserved.
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
import zoo
import sys
import osgeo.ogr
import osgeo.ogr as ogr

def echo(conf,inputs,outputs):
    print(inputs,file=sys.stderr)
    if "a" in inputs:
        if "cache_file" in inputs["a"]:
            inputs["a"]["value"]=open(inputs["a"]["cache_file"],"r").read()
        outputs["a"]["value"]=inputs["a"]["value"]
        if "mimeType" in inputs["a"]:
            outputs["a"]["mimeType"]=inputs["a"]["mimeType"]
        if "mediaType" in inputs["a"]:
            outputs["a"]["mediaType"]=inputs["a"]["mediaType"]
        if "encoding" in inputs["a"]:
            outputs["a"]["encoding"]=inputs["a"]["encoding"]
    else:
        outputs["a"]["value"]="Empty"
    print(inputs,file=sys.stderr)
    if "b" in inputs and inputs["b"]["inRequest"]=="true":
        if "cache_file" in inputs["b"]:
            try:
                ds = osgeo.ogr.Open(inputs["b"]["cache_file"])
                path=""
                if outputs["b"]["mimeType"]=="application/json":
                    out_ds  = ogr.GetDriverByName("GeoJSON").CopyDataSource(ds, conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+"_value.json")
                    if out_ds is None:
                        raise Exception("Unable to parse GeoJSON")
                    path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+"_value.json"
                else:
                    out_ds  = ogr.GetDriverByName("GML").CopyDataSource(ds, conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".xml")
                    if out_ds is None:
                        raise Exception("Unable to parse GML")
                    path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".xml"
            except Exception as e:
                print("*******",file=sys.stderr)
                print(e,file=sys.stderr)
                print("*******",file=sys.stderr)
                path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".xml"
                f = open(path, "w+")
                f.write(inputs["b"]["value"])
                f.close()
                outputs["b"]["mimeType"]="text/xml"
                outputs["b"]["value"]=inputs["b"]["value"]
        else:
            try:
                import json
                lobj=json.loads(inputs["b"]["value"])
                path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+"_value.json"
                f = open(path, "w+")
                json.dump(lobj,f)
                outputs["b"]["mimeType"]="application/json"
            except Exception as e:
                print("*******",file=sys.stderr)
                print(e,file=sys.stderr)
                print("*******",file=sys.stderr)
                path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".xml"
                f = open(path, "w+")
                f.write(inputs["b"]["value"])
                f.close()
                outputs["b"]["mimeType"]="text/xml"
                outputs["b"]["value"]=inputs["b"]["value"]
        outputs["b"]["generated_file"]=path
    else:
        outputs["b"]["value"]="Empty"
    if "c" in inputs:
        if "cache_file" in inputs["c"]:
            inputs["c"]["value"]=open(inputs["c"]["cache_file"],"r").read()
        outputs["c"]["value"]=inputs["c"]["value"]
    else:
        outputs["c"]["value"]="Empty"
    if "pause" in inputs and inputs["pause"]["inRequest"]!="false":
        import time
        nb_sleep=inputs["pause"]["value"]
        for i in range(4):
            conf["lenv"]["message"]="Sleeping for "+inputs["pause"]["value"]+" seconds ("+str(i+1)+"/4)"
            zoo.update_status(conf,25*i)
            time.sleep((float(inputs["pause"]["value"])*25)/100)
    print(outputs,file=sys.stderr)
    return zoo.SERVICE_SUCCEEDED
