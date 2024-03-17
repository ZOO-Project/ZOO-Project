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
import urllib.request
import sys
import json

def route(conf,path,rootUrl):
    req=urllib.request.Request(
            url=rootUrl+(conf["renv"]["REDIRECT_QUERY_STRING"].replace(path+"/","").replace("&","?",1))
            )
    try:
        response = urllib.request.urlopen(req)
        conf["headers"]["Content-Type"] = response.headers.get_content_type()
        if conf["headers"]["Content-Type"].count("image")>0:
            conf["headers"]["Content-Length"]=response.headers.get("content-length")
            #conf["lenv"]["response"]=str(response.read())
            #conf["lenv"]["response_size"]=response.headers.get("content-length")
            with open(conf["main"]["tmpPath"]+"/"+conf["lenv"]["usid"]+".data", "wb") as binary_file:
                binary_file.write(response.read())
                binary_file.close()
            conf["lenv"]["response_generated_file"]=conf["main"]["tmpPath"]+"/"+conf["lenv"]["usid"]+".data"
        else:
            conf["lenv"]["response"]=response.read().decode("utf-8").replace(conf["eoapi"]["proxyFor"],"/"+conf["openapi"]["rootPath"]+"/stac").replace(conf["eoapi"]["proxyForRaster"],"/"+conf["openapi"]["rootPath"]+"/raster").replace(conf["eoapi"]["proxyForVector"],"/"+conf["openapi"]["rootPath"]+"/vector").replace("/openapi.json","/"+conf["openapi"]["rootPath"]+"/"+path+"/openapi.json")#.replace("/collections/","/"+conf["openapi"]["rootPath"]+"/"+path+"/collections/").replace("/info","/info?assets=visual")
            if conf["renv"]["REDIRECT_QUERY_STRING"].count("openapi.json")>0 or conf["renv"]["REDIRECT_QUERY_STRING"].count("/api")>0:
                conf["lenv"]["response"]=conf["lenv"]["response"].replace("\"/","\"/"+conf["openapi"]["rootPath"]+"/"+path+"/")
            conf["lenv"]["response"]=conf["lenv"]["response"].replace("/ogc-api/raster/ogc-api/raster/","/ogc-api/raster/")
    except Exception as e:
        conf["lenv"]["message"]=str(e)
        print("---- ERROR\n",file=sys.stderr)
        print(e,file=sys.stderr)
        print("---- ERROR\n",file=sys.stderr)
        return zoo.SERVICE_FAILED  
    conf["headers"]["status"]="200 OK"
    return zoo.SERVICE_SUCCEEDED

def eoapiRoute(conf,inputs,outputs):
    import sys
    #rootUrl="https://tamn.snapplanet.io"
    rootUrl=conf["eoapi"]["proxyFor"]
    if "REDIRECT_QUERY_STRING" in conf["renv"]:
        if conf["renv"]["REDIRECT_QUERY_STRING"].count("/raster")>0:
            return route(conf,"raster",conf["eoapi"]["proxyForRaster"])
        if conf["renv"]["REDIRECT_QUERY_STRING"].count("/stac")>0:
            return route(conf,"stac",conf["eoapi"]["proxyFor"])
        if conf["renv"]["REDIRECT_QUERY_STRING"].count("/vector")>0:
            return route(conf,"vector",conf["eoapi"]["proxyForVector"])
    return zoo.SERVICE_SUCCEEDED

def securityOut(conf,inputs,outputs):
    try:
        if len(conf["renv"]["REDIRECT_QUERY_STRING"])==1:
            req=urllib.request.Request(
                    conf["openapi"]["rootUrl"]+"/conformance"
                    )
            response = urllib.request.urlopen(req)
            tmpResponseProcessing=response.read().decode("utf-8")
            req=urllib.request.Request(
                    url=conf["eoapi"]["proxyFor"]+"/"
                    )
            response = urllib.request.urlopen(req)
            tmpResponse=response.read().decode("utf-8").replace(conf["eoapi"]["proxyFor"],"/"+conf["openapi"]["rootPath"]+"/stac").replace(conf["eoapi"]["proxyForRaster"],"/"+conf["openapi"]["rootPath"]+"/raster").replace(conf["eoapi"]["proxyForVector"],"/"+conf["openapi"]["rootPath"]+"/vector")
            jsonObjectFetched=json.loads(tmpResponse)
            jsonObjectResponse=json.loads(conf["lenv"]["json_response_object"])
            jsonObjectProcesses=json.loads(tmpResponseProcessing)
            for a in range(len(jsonObjectProcesses["conformsTo"])):
                jsonObjectFetched["conformsTo"]+=[(jsonObjectProcesses["conformsTo"][a])]
            for a in jsonObjectFetched:
                if a=="conformsTo":
                    jsonObjectResponse[a]=jsonObjectFetched[a]
            conf["lenv"]["json_response_object"]=json.dumps(jsonObjectResponse)
            return zoo.SERVICE_SUCCEEDED
    except Exception as e:
        print(e,file=sys.stderr)
        conf["lenv"]["message"]=str(e)
        return zoo.SERVICE_FAILED


    return zoo.SERVICE_SUCCEEDED
