# -*- coding: utf-8 -*-
###############################################################################
#  Author:   Gérald Fenoy, gerald.fenoy@geolabs.fr
#  Copyright (c) 2020-2026, GeoLabs SARL.
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
import re
import hashlib

_NAMESPACE_RE = re.compile(r"^(?!-)(?!\.\.?$)[A-Za-z0-9._@+-]+$")

def normalize_namespace(value):
    """Return a safe, single-segment POSIX-portable namespace for value.
    """
    if not value:
        return None
    if "/" in value or "\x00" in value or ".." in value:
        return None
    if _NAMESPACE_RE.match(value):
        return value
    return "u_" + hashlib.sha256(value.encode("utf-8")).hexdigest()[:32]

def securityIn(conf,inputs,outputs):
    import sys,os,shutil
    if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
        zoo.info("securityIn!")
    try:
        if "has_jwt_service" in conf["servicesNamespace"] and conf["servicesNamespace"]["has_jwt_service"]=="true":
            import jwts.security_service as s
            res=s.securityIn(conf,inputs,outputs)
            s.addHeader(conf,"dru.securityIn")
            if res==zoo.SERVICE_FAILED:
                zoo.error("dru.securityIn has failed")
                return res
    except Exception as e:
        if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
            zoo.error(f"No JWT service available: {str(e)}")
    rPath=conf["servicesNamespace"]["path"]+"/"
    authenticated_user=None
    if "auth_env" in conf and "user" in conf["auth_env"]:
        authenticated_user=normalize_namespace(conf["auth_env"]["user"])
        if authenticated_user is None:
            zoo.error(zoo._("Authenticated user name contains an illegal path construct"))
            conf["lenv"]["code"]="NotAllowed"
            conf["lenv"]["message"]=zoo._("Invalid user namespace")
            return zoo.SERVICE_FAILED
    url_namespace=None
    for key in ("REDIRECT_REDIRECT_SERVICES_NAMESPACE",
                "REDIRECT_SERVICES_NAMESPACE",
                "SERVICES_NAMESPACE"):
        val=conf["renv"].get(key,"")
        if val!="":
            url_namespace=normalize_namespace(val)
            if url_namespace is None:
                zoo.error(f"Rejected illegal namespace value in {key}")
                conf["lenv"]["code"]="BadRequest"
                conf["lenv"]["message"]="Invalid namespace"
                return zoo.SERVICE_FAILED
            break
    global_namespace="anonymous"
    if "openapi" in conf and "globalNamespace" in conf["openapi"]:
        _gn=normalize_namespace(conf["openapi"]["globalNamespace"])
        if _gn is not None:
            global_namespace=_gn
    auth_active=("osecurity" in conf)
    if url_namespace is None:
        # No namespace in the URL: use the authenticated identity, else shared.
        namespace=authenticated_user if authenticated_user is not None else global_namespace
    elif url_namespace==global_namespace:
        # Shared namespace: open to everyone (read/write).
        namespace=global_namespace
    elif authenticated_user is not None and url_namespace==authenticated_user:
        # A user's own namespace.
        namespace=authenticated_user
    elif not auth_active:
        # Deployment without authentication: URL namespace stays free.
        namespace=url_namespace
    else:
        # Auth active: an unauthenticated request to a private namespace, or an
        # authenticated user requesting someone else's namespace.
        zoo.error("Not authorized to access the requested namespace")
        conf["lenv"]["code"]="NotAllowed"
        conf["lenv"]["message"]=zoo._("You are not authorized to access this namespace")
        return zoo.SERVICE_FAILED

    rPath += namespace
    conf["auth_env"] = {"user": namespace, "cwd": rPath}
    conf["lenv"]["fpm_user"] = namespace
    conf["lenv"]["fpm_cwd"] = rPath
    conf["zooServicesNamespace"] = {"namespace": namespace, "cwd": rPath}
    conf["main"]["tmpPath"] = rPath + "/temp"

    for i in conf["renv"]:
        if i.count("QUERY_STRING") and conf["renv"].get("REDIRECT_QUERY_STRING","").count("/package")>0:
            # In case the client application requests for the CWL in JSON format,
            # we need to inform the ZOO-Kernel by setting the
            # require_conversion_to_json variabe to true in the lenv section.
            if conf["renv"]["HTTP_ACCEPT"]=="application/cwl+json":
                zoo.info("Conversion to cwl+json should happen in securityOut")
                conf["renv"]["HTTP_ACCEPT_ORIGIN"]="application/cwl+json"
                conf["renv"]["HTTP_ACCEPT"]="application/cwl"
                conf["lenv"]["require_conversion_to_json"]="true"

    if not(os.path.isdir(rPath)):
        zoo.info(f"Creating directory {rPath}")
        os.mkdir(rPath)
        os.mkdir(rPath+"/temp") # Create temporary directory for run informations specific to a user
        if "required_files" in conf["servicesNamespace"]:
            rFiles=conf["servicesNamespace"]["required_files"].split(',')
            for i in range(len(rFiles)):
                zoo.info(f"Copy file {rFiles[i]}")
                shutil.copyfile(conf["renv"]["CONTEXT_DOCUMENT_ROOT"]+"/"+rFiles[i],rPath+"/"+rFiles[i])
    conf["renv"]["FAKE_HTTP_ACCEPT_PROFILE"] = False
    if "REDIRECT_QUERY_STRING" in conf["renv"]:
        from urllib.parse import urlparse, parse_qs, parse_qsl
        params = parse_qs(conf["renv"]["REDIRECT_QUERY_STRING"])
        profile_value = params.get('profile', [''])[0]
        if profile_value != '':
            if "HTTP_ACCEPT_PROFILE" not in conf["renv"]:
                conf["renv"]["HTTP_ACCEPT_PROFILE"] = profile_value
                conf["renv"]["FAKE_HTTP_ACCEPT_PROFILE"] = True
    if "HTTP_ACCEPT_PROFILE" in conf["renv"] and conf["renv"]["HTTP_ACCEPT_PROFILE"].count("openeo") > 0:
        import openeo_processes
        openeo_processes.convert(conf,{},outputs)
    return zoo.SERVICE_SUCCEEDED

def securityOut(conf,inputs,outputs):
    import sys
    try:
        if "has_jwt_service" in conf["servicesNamespace"] and conf["servicesNamespace"]["has_jwt_service"]=="true":
            import jwts.security_service as s
            s.addHeader(conf,"dru.securityOut")
    except Exception as e:
        if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
            zoo.debug("No JWT service available: "+str(e))
    if "servicesNamespace" in conf and "debug" in conf["servicesNamespace"]:
        zoo.debug("securityOut!")
    if "json_response_object" in conf["lenv"] and "require_conversion_to_json" in conf["lenv"] and conf["lenv"]["require_conversion_to_json"]=="true":
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
    import json
    import os
    from zoo_calrissian_runner import ZooCalrissianRunner
    from pycalrissian.context import CalrissianContext

    zoo.info(f"runDismiss {str(conf['lenv'])}!")
    try:
        import configparser
        lenv_path=os.path.join(
            conf["main"]["tmpPath"],
            f"{conf['lenv']['gs_usid']}_lenv.cfg"
            )
        config=configparser.ConfigParser()
        config.read(lenv_path)
        if "run_id" in config["lenv"]:
            from zoo_wes_runner import ZooWESRunner
            wes=ZooWESRunner(
                cwl=None,
                conf=conf,
                inputs=inputs,
                outputs=outputs,
                execution_handler=None,
            )
            conf["lenv"]["run_id"]=config["lenv"]["run_id"]
            wes.dismiss()
            return zoo.SERVICE_SUCCEEDED
    except Exception as e:
        zoo.error(str(e))

    try:
        if "param" in inputs:
            json_object=json.loads(inputs["param"]["value"])
            session = CalrissianContext(
                namespace=ZooCalrissianRunner.shorten_namespace(json_object["processID"].replace("_","-")+"-"+conf["lenv"]["gs_usid"]),
                storage_class=os.environ.get("STORAGE_CLASS", "openebs-nfs-test"),
                volume_size="10Mi",
            )
            zoo.info(f"Dispose namespace {session.namespace}")
            session.dispose()
    except Exception as e:
        zoo.error(str(e))

    return zoo.SERVICE_SUCCEEDED

def browse(conf,inputs,outputs):
    import os

    user_namespace = None
    if "zooServicesNamespace" in conf and "namespace" in conf["zooServicesNamespace"]:
        user_namespace = conf["zooServicesNamespace"]["namespace"]

    if user_namespace is None:
        conf["lenv"]["message"] = "Unable to resolve user namespace"
        return zoo.SERVICE_FAILED

    if "directory" not in inputs or "value" not in inputs["directory"]:
        conf["lenv"]["message"] = "Missing directory input"
        return zoo.SERVICE_FAILED

    requested_path = inputs["directory"]["value"]
    if "\x00" in requested_path:
        conf["lenv"]["message"] = "Invalid path"
        return zoo.SERVICE_FAILED

    base_temp_dir = os.path.realpath(
        os.path.join(conf["servicesNamespace"]["path"], user_namespace, "temp")
    )
    target_path = os.path.realpath(os.path.join(base_temp_dir, requested_path))

    try:
        if os.path.commonpath([base_temp_dir, target_path]) != base_temp_dir:
            conf["lenv"]["message"] = "Access denied"
            return zoo.SERVICE_FAILED
    except ValueError:
        conf["lenv"]["message"] = "Access denied"
        return zoo.SERVICE_FAILED

    if not os.path.isfile(target_path):
        conf["lenv"]["message"] = "Unable to access the file"
        return zoo.SERVICE_FAILED

    with open(target_path, "r", encoding="utf-8") as f:
        if "result" not in outputs:
            outputs["result"] = {}
        outputs["result"]["value"] = f.read()
        return zoo.SERVICE_SUCCEEDED

    conf["lenv"]["message"] = "Unable to access the file"
    return zoo.SERVICE_FAILED
