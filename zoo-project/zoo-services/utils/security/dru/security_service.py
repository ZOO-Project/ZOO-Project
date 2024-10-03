# -*- coding: utf-8 -*-
###############################################################################
#  Author:   Gérald Fenoy, gerald.fenoy@geolabs.fr
#  Copyright (c) 2020-2023, GeoLabs SARL. 
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

def securityIn(conf,inputs,outputs):
    import sys,os,shutil
    if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
        print("securityIn!",file=sys.stderr)
    try:
        if "has_jwt_service" in conf["servicesNamespace"] and conf["servicesNamespace"]["has_jwt_service"]=="true":
            import jwts.security_service as s
            res=s.securityIn(conf,inputs,outputs)
            s.addHeader(conf,"dru.securityIn")
            if res==zoo.SERVICE_FAILED:
                return res
    except Exception as e:
        if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
            print("No JWT service available: "+str(e),file=sys.stderr)
    rPath=conf["servicesNamespace"]["path"]+"/"
    for i in conf["renv"]:
        if i.count("SERVICES_NAMESPACE"):
            rPath+=conf["renv"][i]
            conf["auth_env"]={"user": conf["renv"][i],"cwd": rPath}
            conf["lenv"]["fpm_user"]=conf["renv"][i]
            conf["lenv"]["fpm_cwd"]=rPath
            conf["zooServicesNamespace"]={"namespace": conf["renv"][i],"cwd": rPath}
            conf["main"]["tmpPath"]=rPath+"/temp"
        if i.count("QUERY_STRING") and conf["renv"]["REDIRECT_QUERY_STRING"].count("/package")>0:
            # In case the client application requests for the CWL in JSON format,
            # we need to inform the ZOO-Kernel by setting the
            # require_conversion_to_json variabe to true in the lenv section.
            if conf["renv"]["HTTP_ACCEPT"]=="application/cwl+json":
                print("Conversion to cwl+json should happen in securityOut",file=sys.stderr)
                conf["renv"]["HTTP_ACCEPT_ORIGIN"]="application/cwl+json"
                conf["renv"]["HTTP_ACCEPT"]="application/cwl"
                conf["lenv"]["require_conversion_to_json"]="true"
    if not(os.path.isdir(rPath)):
        os.mkdir(rPath)
        os.mkdir(rPath+"/temp") # Create temporary directory for run informations specific to a user
        if "required_files" in conf["servicesNamespace"]:
            rFiles=conf["servicesNamespace"]["required_files"].split(',')
            for i in range(len(rFiles)):
                shutil.copyfile(conf["renv"]["CONTEXT_DOCUMENT_ROOT"]+"/"+rFiles[i],rPath+"/"+rFiles[i])
    return zoo.SERVICE_SUCCEEDED

def securityOut(conf,inputs,outputs):
    import sys
    try:
        if "has_jwt_service" in conf["servicesNamespace"] and conf["servicesNamespace"]["has_jwt_service"]=="true":
            import jwts.security_service as s
            s.addHeader(conf,"dru.securityOut")
    except Exception as e:
        if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
            print("No JWT service available: "+str(e),file=sys.stderr)
    if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
        print("securityOut!",file=sys.stderr)
    if "require_conversion_to_json" in conf["lenv"] and conf["lenv"]["require_conversion_to_json"]=="true":
        import json
        import yaml
        if "require_conversion_to_ogcapppkg" in conf["lenv"]:
            # Convert the CWL to ogcapppkg+json format
            conf["lenv"]["json_response_object"]=json.dumps({"executionUnit": {"value": yaml.safe_load(conf["lenv"]["json_response_object"]),"mediaType": "application/cwl+json" } }, indent=2)
        else:
            # Convert the CWL to JSON format
            conf["lenv"]["json_response_object"]=json.dumps(yaml.safe_load(conf["lenv"]["json_response_object"]), indent=2)
    return zoo.SERVICE_SUCCEEDED

def runDismiss(conf,inputs,outputs):
    import sys
    print(conf["lenv"],file=sys.stderr)
    print(outputs,file=sys.stderr)
    import json
    import os
    from loguru import logger
    from zoo_calrissian_runner import ZooCalrissianRunner
    from pycalrissian.context import CalrissianContext

    logger.remove()
    logger.add(sys.stderr, level="INFO")
    try:
        if "param" in inputs:
            print(inputs,file=sys.stderr)
            json_object=json.loads(inputs["param"]["value"])
            session = CalrissianContext(
                namespace=ZooCalrissianRunner.shorten_namespace(json_object["processID"].replace("_","-")+"-"+conf["lenv"]["gs_usid"]),
                storage_class=os.environ.get("STORAGE_CLASS", "openebs-nfs-test"),
                volume_size="10Mi",
            )
        print("DISPOSE NAMESPACE",file=sys.stderr)
        session.dispose()
        print("DISPOSED NAMESPACE",file=sys.stderr)
    except Exception as e:
        print(e,file=sys.stderr)

    return zoo.SERVICE_SUCCEEDED

def browse(conf,inputs,outputs):
    import sys
    print(inputs,file=sys.stderr)
    print(conf["renv"],file=sys.stderr)
    f=open(conf["servicesNamespace"]["path"]+"/"+conf["renv"]["REDIRECT_REDIRECT_SERVICES_NAMESPACE"]+"/temp/"+inputs["directory"]["value"],"r", encoding="utf-8")
    if f is not None:
        if "result" not in outputs:
            outputs["result"]={}
        outputs["result"]["value"]=f.read()
        return zoo.SERVICE_SUCCEEDED
    conf["lenv"]["message"]="Unable to access the file"
    return zoo.SERVICE_FAILED
