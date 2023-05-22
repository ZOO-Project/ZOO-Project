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
