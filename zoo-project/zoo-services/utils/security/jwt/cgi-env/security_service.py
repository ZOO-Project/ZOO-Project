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

def addHeader(conf,name):
    if "headers" not in conf:
        conf["headers"]={}
    key="Powered-By"
    if "X-"+key in conf["headers"]:
        while "X-"+key in conf["headers"]:
            key="Also-"+key
    conf["headers"]["X-"+key]=name

def securityIn(main_conf,inputs,outputs):
    if "servicesNamespace" in main_conf and "debug" in main_conf["servicesNamespace"]:
        print("JWT securityIn",file=sys.stderr)
    addHeader(main_conf,"jwt.securityIn")
    hasAuth=False
    for i in main_conf["renv"].keys():
        if i.count("HTTP_AUTHORIZATION")>0:
            sToken=main_conf["renv"][i].split(' ')[1]
            print(sToken,file=sys.stderr)
            if sToken.count(".")>=2:
                cJWT=main_conf["renv"][i].split(' ')[1]
                if "osecurity" in main_conf and "realm" in main_conf["osecurity"]:
                    if main_conf["renv"][i].count("oidc/"+main_conf["osecurity"]["realm"]+"/")>0:
                        cJWT=cJWT.replace("oidc/"+main_conf["osecurity"]["realm"]+"/","")
                jsonObj=jwt.decode(cJWT, options={"verify_signature": False,"verify_aud": False})
                hasAuth=True
                myKeys=list(jsonObj.keys())
                for k in jsonObj.keys():
                    if k.count("username")>0 or k.count("user_name")>0:
                        if "osecurity" in main_conf and \
                           "allowed_users" in main_conf["osecurity"] and \
                           "preferred_username" in jsonObj and \
                           main_conf["osecurity"]["allowed_users"].split(",").count(jsonObj["preferred_username"])==0:
                            if "lenv" not in main_conf:
                                main_conf["lenv"] = {}
                            main_conf["lenv"]["message"]=zoo._("You are not authorized to perform the requested operation on the resource (jwt.securityIn).")
                            main_conf["lenv"]["code"]="Forbidden"
                            main_conf["lenv"]["status"]="403 Forbidden"
                            if "headers" in main_conf:
                                main_conf["headers"]["status"]="403 Forbidden"
                            else:
                                main_conf["headers"]={"status":"403 Forbidden"}
                        main_conf["auth_env"]={"user": jsonObj[k] }
                        break
                if "auth_env" not in main_conf:
                    main_conf["auth_env"] = {}
                if "email" in jsonObj.keys():
                    main_conf["auth_env"]["email"]=jsonObj["email"]
                for l in range(len(myKeys)):
                    main_conf["auth_env"][myKeys[l]]=str(jsonObj[myKeys[l]])
                main_conf["auth_env"]["jwt"]=cJWT
                if "lenv" not in main_conf:
                    main_conf["lenv"] = {}
                main_conf["lenv"]["json_user"]=json.dumps(jsonObj)
            else:
                import requests
                if "osecurity" in main_conf and \
                   "userinfoUrl" not in main_conf["osecurity"]:
                    import base64
                    b64 = ""
                    if "client_id" in main_conf["osecurity"] and \
                       "client_secret" in main_conf["osecurity"]:
                        try:
                            b64=base64.b64encode((main_conf["osecurity"]["client_id"]+":"+main_conf["osecurity"]["client_secret"]).encode("ascii")).decode('ascii')
                        except Exception as e:
                            print(e,file=sys.stderr)
                            if "servicesNamespace" in main_conf and "debug" in main_conf["servicesNamespace"]:
                                print("b64 encoding error: " + str(e), file=sys.stderr)
                                print(traceback.format_exc())
                    headers = {"Authorization" : "Basic "+b64, "Acccept": "application/json","Content-Type": "application/x-www-form-urlencoded"}
                    data= {"token": sToken,"token_type_hint": "access_token"}
                    response=requests.post("https://www.authenix.eu/oauth/tokeninfo",data=data,headers=headers)
                    userObject=json.loads(response.text)
                    if userObject["active"] and "sub" in userObject:
                        if "auth_env" not in main_conf:
                            main_conf["auth_env"] = {}
                        main_conf["auth_env"]={"user": userObject["sub"]}
                        hasAuth=True
                else:
                    if "osecurity" in main_conf and "userinfoUrl" in main_conf["osecurity"]:
                        headers = {"Authorization" : "Bearer "+sToken, "Acccept": "application/json"}
                        response=requests.get(main_conf["osecurity"]["userinfoUrl"],headers=headers)
                        userObject=json.loads(response.text)
                        if "auth_env" not in main_conf:
                            main_conf["auth_env"] = {}
                        main_conf["auth_env"]={"jwt": sToken}
                        for a in userObject.keys():
                            main_conf["auth_env"][a]=userObject[a]
                        if "user_name" not in main_conf["auth_env"]:
                            if "sub" in userObject:
                                main_conf["auth_env"]["user"]=userObject["sub"]
                                hasAuth=True
                        else:
                            main_conf["auth_env"]["user"]=main_conf["auth_env"]["user_name"]
                            hasAuth=True
            break
    if "auth_env" in main_conf and "user" in main_conf:
        main_conf["renv"]["SERVICES_NAMESPACE"]=main_conf["auth_env"]["user"]
    if hasAuth or \
       ("lenv" in main_conf and "secured_url" in main_conf["lenv"] and main_conf["lenv"]["secured_url"]=="false"):
        return zoo.SERVICE_SUCCEEDED
    else:
        if "headers" in main_conf:
            main_conf["headers"]["status"]="403 Forbidden"
        else:
            main_conf["headers"]={"status":"403 Forbidden"}
        if "lenv" in main_conf:
            main_conf["lenv"]["code"]="NotAllowed"
            main_conf["lenv"]["message"]="Unable to ensure that you are allowed to access the resource."
        return zoo.SERVICE_FAILED
