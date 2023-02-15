import zoo
import sys
import osgeo.ogr
import osgeo.ogr as ogr

def EchoProcess(conf,inputs,outputs):
    print(inputs,file=sys.stderr)
    if "literalInput" in inputs:
        outputs["literalOutput"]["value"]=inputs["literalInput"]["value"]
        if "mimeType" in inputs["literalInput"]:
            outputs["literalOutput"]["mimeType"]=inputs["literalInput"]["mimeType"]
        if "mediaType" in inputs["literalInput"]:
            outputs["literalOutput"]["mediaType"]=inputs["literalInput"]["mediaType"]
        if "encoding" in inputs["literalInput"]:
            outputs["literalOutput"]["encoding"]=inputs["literalInput"]["encoding"]
    else:
        outputs["literalOutput"]["value"]="Empty"
    print(inputs,file=sys.stderr)
    if "complexInput" in inputs and inputs["complexInput"]["inRequest"]=="true":
        if "cache_file" in inputs["complexInput"]:
            try:
                ds = osgeo.ogr.Open(inputs["complexInput"]["cache_file"])
                path=""
                if outputs["complexOutput"]["mimeType"]=="application/json":
                    out_ds  = ogr.GetDriverByName("GeoJSON").CopyDataSource(ds, conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".json")
                    if out_ds is None:
                        raise Exception("Unable to parse GeoJSON")
                    path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".json"
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
                f.write(inputs["complexInput"]["value"])
                f.close()
                outputs["complexOutput"]["mimeType"]="text/xml"
                outputs["complexOutput"]["value"]=inputs["complexInput"]["value"]
        else:
            try:
                import json
                lobj=json.loads(inputs["complexInput"]["value"])
                path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".json"
                f = open(path, "w+")
                json.dump(lobj,f)
                outputs["complexOutput"]["mimeType"]="application/json"
            except Exception as e:
                print("*******",file=sys.stderr)
                print(e,file=sys.stderr)
                print("*******",file=sys.stderr)
                path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".xml"
                f = open(path, "w+")
                f.write(inputs["complexInput"]["value"])
                f.close()
                outputs["complexOutput"]["mimeType"]="text/xml"
                outputs["complexOutput"]["value"]=inputs["complexInput"]["value"]
        outputs["complexOutput"]["generated_file"]=path
    else:
        outputs["complexOutput"]["value"]="Empty"
    if "c" in inputs:
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
