# -*- coding: utf-8 -*-
###############################################################################
#  Author:   Gérald Fenoy, gerald.fenoy@geolabs.fr
#  Copyright (c) 2023, GeoLabs SARL. 
############################################################################### 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
################################################################################
import zoo
import sys
print("OK",file=sys.stderr)

def generateInputs(inputs):
    ret={}
    for i in inputs:
        if "dataType" in inputs[i]:
            if inputs[i]["dataType"]=="number":
                try:
                    ret[i]=float(inputs[i]["value"])
                except:
                    ret[i]=int(inputs[i]["value"])
    return ret

def run(conf,inputs,outputs):
    print(inputs,file=sys.stderr)
    try:
        print(inputs,file=sys.stderr)
        import subprocess,json
        print(json.dumps(generateInputs(inputs)),file=sys.stderr)
        resultStorage=conf["main"]["tmpPath"]+"/"+conf["lenv"]["usid"]
        command = 'python '+conf["lenv"]["cwd"]+"/openeo_run.py " + resultStorage + " "+inputs["OpenEOGraph"]["cache_file"]
        process = subprocess.Popen(command.split()+[json.dumps(generateInputs(inputs))], stdout=subprocess.PIPE)
        output, error = process.communicate()
        print("OUTPUT",file=sys.stderr)
        print(output,file=sys.stderr)
        print("ERROR",file=sys.stderr)
        print(error,file=sys.stderr)
        outputs["result"]["value"]=open(resultStorage,"r").read()
        print(outputs,file=sys.stderr)
        return zoo.SERVICE_SUCCEEDED
    except Exception as e:
        print(e,file=sys.stderr)
        conf["lenv"]["message"]=e
        return zoo.SERVICE_FAILED

