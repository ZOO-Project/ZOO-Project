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
    print("securityIn!",file=sys.stderr)
    rPath=conf["servicesNamespace"]["path"]+"/"
    for i in conf["renv"]:
        if i.count("SERVICES_NAMESPACE"):
            rPath+=conf["renv"][i]
            conf["auth_env"]={"user": conf["renv"][i],"cwd": rPath}
            conf["lenv"]["fpm_user"]=conf["renv"][i]
            conf["lenv"]["fpm_cwd"]=rPath
            conf["zooServicesNamespace"]={"namespace": conf["renv"][i],"cwd": rPath}
            break
    if not(os.path.isdir(rPath)):
        os.mkdir(rPath)
        rFiles=["DeployProcess.py","DeployProcess.zcfg","UndeployProcess.py","UndeployProcess.zcfg","service.py","security_service.py","securityOut.zcfg","deploy_util.py","securityIn.zcfg"]
        for i in range(len(rFiles)):
            shutil.copyfile(conf["renv"]["CONTEXT_DOCUMENT_ROOT"]+"/"+rFiles[i],rPath+"/"+rFiles[i])
    print("Ok exit from securityIn!",file=sys.stderr)
    return zoo.SERVICE_SUCCEEDED

def securityOut(conf,inputs,outputs):
    import sys
    print("securityOut!",file=sys.stderr)
    return zoo.SERVICE_SUCCEEDED

