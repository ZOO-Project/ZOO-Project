# -*- coding: utf-8 -*-
#
# Author : Gérald FENOY
#
# Copyright 2023 GeoLabs SARL. All rights reserved.
# 
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
import jwt
import sys
import json

def securityIn(main_conf,inputs,outputs):
    hasAuth=False
    for i in main_conf["renv"].keys():
        if i.count("HTTP_AUTHORIZATION")>0:
            jsonObj=jwt.decode(main_conf["renv"][i].split(' ')[1], options={"verify_signature": False,"verify_aud": False})
            hasAuth=True
            myKeys=list(jsonObj.keys())
            if "preferred_username" in jsonObj.keys():
                main_conf["auth_env"]={"user": jsonObj["preferred_username"]}
            if "email" in jsonObj.keys():
                main_conf["auth_env"]["email"]=jsonObj["email"]
            for l in range(len(myKeys)):
                main_conf["auth_env"][myKeys[l]]=str(jsonObj[myKeys[l]])
    if "auth_env" in main_conf:
        print(main_conf["auth_env"],file=sys.stderr)
    if hasAuth or main_conf["lenv"]["secured_url"]=="false":
        return zoo.SERVICE_SUCCEEDED
    else:
        if "headers" in main_conf:
            main_conf["headers"]["status"]="403 Forbidden"
        else:
            main_conf["headers"]={"status":"403 Forbidden"}
        main_conf["lenv"]["code"]="NotAllowed"
        main_conf["lenv"]["message"]="Unable to ensure that you are allowed to access the resource."
        return zoo.SERVICE_FAILED
