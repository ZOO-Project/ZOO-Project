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
import os
import shutil

def openeoFilterIn(conf,inputs,outputs):
    if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
        print("OpenEO securityIn!",file=sys.stderr)
    if "auth_env" not in conf:
        return zoo.SERVICE_SUCCEEDED
    if "renv" in conf and "REQUEST_METHOD" in conf["renv"]:
        if conf["renv"]["REQUEST_METHOD"]=="DELETE":
            return zoo.SERVICE_SUCCEEDED
    rPath=conf["auth_env"]["cwd"]
    try:
        if os.path.isdir(rPath) and not(os.path.exists(rPath+"/openeo_run.py")):
            rFiles=["openeo_run.py"]
            for i in range(len(rFiles)):
                shutil.copyfile(conf["renv"]["CONTEXT_DOCUMENT_ROOT"]+"/"+rFiles[i],rPath+"/"+rFiles[i])
    except Exception as e:
        print("ERROR OpenEO",file=sys.stderr)
        print(e,file=sys.stderr)
    try:
        if "jrequest" in conf["request"]:
            import json
            jObj=json.loads(conf["request"]["jrequest"])
            print("OK STARTUP",file=sys.stderr)
            ret=writeZcfgFromProcessDescription(conf,jObj)
            if ret==zoo.SERVICE_FAILED:
                print("ERROR OpenEO 2",file=sys.stderr)
                return ret
    except Exception as e:
        conf["lenv"]["cwd"]=rPath
        print("ERROR OpenEO 1",file=sys.stderr)
        print(e,file=sys.stderr)
    if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
        print("OpenEO securityIn end!",file=sys.stderr)
    return zoo.SERVICE_SUCCEEDED

def writeZcfgFromProcessDescription(conf,obj):
    processDescription=obj["processDescription"]
    executionUnit=obj["executionUnit"]
    processName=processDescription["id"]
    prefixPath="/"
    if processDescription["id"].count(".")>0:
        tmp=processDescription["id"].split(".")
        prefixPath="/"+tmp[0]+"/"
        processName=tmp[1]
        if not(os.path.isdir(conf["auth_env"]["cwd"]+prefixPath)):
            os.mkdir(conf["auth_env"]["cwd"]+prefixPath)
    if os.path.exists(conf["auth_env"]["cwd"] + "/" + processDescription["id"] + ".json") and not(conf["renv"]["REQUEST_METHOD"]=="PUT"):
        if "headers" not in conf:
            conf["headers"]={}
        conf["headers"]["status"]="409 Conflict"
        conf["lenv"]["code"]="DuplicatedProcess"
        conf["lenv"]["ecode"]="http://www.opengis.net/def/exceptions/ogcapi-processes-2/1.0/duplicated-process"
        conf["lenv"]["message"]=zoo._("The process is already deployed (from OpenEO).")
        return zoo.SERVICE_FAILED
    zcfgFile=open(conf["auth_env"]["cwd"]+prefixPath+processName+".zcfg","w")
    zcfgFile.write("["+processName+"]\n")
    if "title" in processDescription:
        zcfgFile.write(" Title = "+ processDescription["title"] + "\n")
    else:
        zcfgFile.write(" Title = Not provided\n")
    if "description" in processDescription:
        zcfgFile.write(" Abstract = "+ processDescription["description"] + "\n")
    else:
        zcfgFile.write(" Abstract = Not provided\n")
    if "version" in processDescription:
        zcfgFile.write(" processVersion = "+processDescription["version"] + "\n")
    else:
        zcfgFile.write(" processVersion = 0.0.1 \n")
    if "type" in executionUnit and executionUnit["type"]!="application/cwl":
        if executionUnit["type"]=="SLURM":
            zcfgFile.write(" serviceType = HPC \n")
            zcfgFile.write(" serviceProvider = "+ processDescription["id"] + "\n")
            # We should invoke the DeployOnHpc service asynchronously from here
            # An option would be to use WPS 1.0.0 and the following URL:
            # zoo_loader.cgi.resquest=Execute&service=WPS&version=1.0.0&\
            # Identifier=DeployOnHpc&DataInputs=image=executionUnit["image"]&\
            # storeExecuteReponse=true&status=true
            # Another option would be to send a message to RabbitMQ to invoke the
            # execution only on the ZOO-FPM.
        else:
            if executionUnit["type"].count("openeo")>0:
                zcfgFile.write(" serviceType = Python \n")
                zcfgFile.write(" serviceProvider = "+ processDescription["id"] + "\n")
                zcfgFile.write(" serviceSubType = OpenEO\n")
                # Write the graph if any
                if "value" in executionUnit:
                    import json
                    graphFile=open(conf["auth_env"]["cwd"]+prefixPath+processName+"_graph.json","w")
                    graphFile.write(json.dumps(executionUnit["value"]))
                    graphFile.close()
    zcfgFile.write(" mutable = true\n")
    if "additionalParameters" in processDescription:
        zcfgFile.write(" <AdditionalParameters>\n")
        for c in range(len(processDescription["additionalParameters"]["parameters"])):
            zcfgFile.write("  "+processDescription["additionalParameters"]["parameters"][c]["name"] + \
                               " = "+ \
                               str(processDescription["additionalParameters"]["parameters"][c]["value"][0]) \
                               +"\n")
        zcfgFile.write(" </AdditionalParameters>\n")
    zcfgFile.write(" <DataInputs>\n")
    writeZcfgInputs(zcfgFile,processDescription["inputs"])
    zcfgFile.write(" </DataInputs>\n")
    zcfgFile.write(" <DataOutputs>\n")
    writeZcfgOutputs(zcfgFile,processDescription["outputs"])
    zcfgFile.write(" </DataOutputs>\n")
    zcfgFile.close()
    # Set the desired lenv keys to bypass the execution of the Deploy process
    conf["lenv"]["isDeployed"]="true"
    conf["lenv"]["Identifier"]=conf["servicesNamespace"]["deploy_service_provider"]
    conf["lenv"]["deployedServiceId"]=processDescription["id"]
    conf["lenv"]["toDeploy"]=processDescription["id"]
    if conf["renv"]["REQUEST_METHOD"]=="PUT":
        if "headers" not in conf:
            conf["headers"]={}
        conf["headers"]["status"]="204 No Content"
    return 0

def writeZcfgOutputs(zcfgFile,outputs):
    for id in outputs:
        zcfgFile.write(" ["+id+"]\n")
        if "title" in outputs[id]:
            zcfgFile.write(" Title = "+ outputs[id]["title"] + "\n")
        else:
            zcfgFile.write(" Title = Not provided\n")
        if "description" in outputs[id]:
            zcfgFile.write(" Abstract = "+ outputs[id]["title"] + "\n")
        else:
            zcfgFile.write(" Abstract = Not provided\n")
        if "additionalParameters" in outputs[id]:
            zcfgFile.write(" <AdditionalParameters>\n")
            for c in range(len(outputs[id]["additionalParameters"]["parameters"])):
                zcfgFile.write("  "+outputs[id]["additionalParameters"]["parameters"][c]["name"] + \
                                " = "+ \
                                outputs[id]["additionalParameters"]["parameters"][c]["value"][0] \
                                +"\n")
            zcfgFile.write(" </AdditionalParameters>\n")
        writeZcfgDataType(zcfgFile,outputs[id]["schema"])

def writeZcfgInputs(zcfgFile,inputs):
    for id in inputs:
        zcfgFile.write(" ["+id+"]\n")
        if "title" in inputs[id]:
            zcfgFile.write("  Title = "+ inputs[id]["title"] + "\n")
        else:
            zcfgFile.write("  Title = Not provided\n")
        if "description" in inputs[id]:
            zcfgFile.write("  Abstract = "+ inputs[id]["title"] + "\n")
        else:
            zcfgFile.write("  Abstract = Not provided\n")
        if not("minOccurs" in inputs and "maxOccurs" in inputs):
            zcfgFile.write("  minOccurs = 1\n")
            zcfgFile.write("  maxOccurs = 1\n")
        else:
            if not("minOccurs" in inputs[id]) and not("schema" in inputs[id] and "nullable" in inputs[id]["schema"]):
                zcfgFile.write("  minOccurs = 1\n")
            else:
                if not("schema" in inputs[id] and "nullable" in inputs[id]["schema"]):
                    zcfgFile.write("  minOccurs = "+ str(inputs[id]["minOccurs"]) +" \n")
                else:
                    zcfgFile.write("  minOccurs = 0 \n")
            if not("maxOccurs" in inputs):
                zcfgFile.write("  minOccurs = 1\n")
            else:
                zcfgFile.write("  maxOccurs = "+ str(inputs[id]["maxOccurs"]) +" \n")
        if "additionalParameters" in inputs[id]:
            zcfgFile.write("  <AdditionalParameters>\n")
            for c in range(len(inputs[id]["additionalParameters"]["parameters"])):
                zcfgFile.write("   "+inputs[id]["additionalParameters"]["parameters"][c]["name"] + \
                                " = "+ \
                                inputs[id]["additionalParameters"]["parameters"][c]["value"][0] \
                                +"\n")
            zcfgFile.write("  </AdditionalParameters>\n")
        writeZcfgDataType(zcfgFile,inputs[id]["schema"],prefix=1)

def writeZcfgDataType(zcfgFile,schemaObj,prefix=0):
    if "type" in schemaObj:
        for c in range(prefix):
            zcfgFile.write(" ")
        zcfgFile.write(" <LiteralData>\n")
        for c in range(prefix):
            zcfgFile.write(" ")
        zcfgFile.write("  dataType = " + schemaObj["type"] + "\n")
        if "enum" in schemaObj:
            for c in range(prefix):
                zcfgFile.write(" ")
            zcfgFile.write("  AllowedValues = ")
            for i in range(len(schemaObj["enum"])):
                if i >0:
                    zcfgFile.write(",")
                zcfgFile.write(str(schemaObj["enum"][i]))
            zcfgFile.write("\n")
        if "default" in schemaObj:
            for c in range(prefix):
                zcfgFile.write(" ")
            zcfgFile.write("   <Default>\n")
            for c in range(prefix):
                zcfgFile.write(" ")
            zcfgFile.write("    value = " + str(schemaObj["default"]) + "\n")
            for c in range(prefix):
                zcfgFile.write(" ")
            zcfgFile.write("   </Default>\n")
        else:
            for c in range(prefix):
                zcfgFile.write(" ")
            zcfgFile.write("   <Default/>\n")
        for c in range(prefix):
            zcfgFile.write(" ")
        zcfgFile.write(" </LiteralData>\n")
    else:
        # Guessing that there is no BBOX so we directly utput a ComplexData here
        for c in range(prefix):
            zcfgFile.write(" ")
        zcfgFile.write(" <ComplexData>\n")
        if "oneOf" in schemaObj:
            for i in range(len(schemaObj["oneOf"])):
                for c in range(prefix):
                    zcfgFile.write(" ")
                if i==0:
                    zcfgFile.write("  <Default>\n")
                else:
                    zcfgFile.write("  <Supported>\n")
                for c in range(prefix):
                    zcfgFile.write(" ")
                if "contentMediaType" in schemaObj["oneOf"][i]:
                    zcfgFile.write("   mimeType = "+schemaObj["oneOf"][i]["contentMediaType"]+"\n")
                for c in range(prefix):
                    zcfgFile.write(" ")
                if i==0:
                    zcfgFile.write("  </Default>\n")
                else:
                    zcfgFile.write("  </Supported>\n")
        for c in range(prefix):
            zcfgFile.write(" ")
        zcfgFile.write(" </ComplexData>\n")
