import zoo
import sys
import osgeo.ogr
import osgeo.ogr as ogr

def echo(conf,inputs,outputs):
    if "a" in inputs:
        outputs["a"]["value"]=inputs["a"]["value"]
        if "mimeType" in inputs["a"]:
            outputs["a"]["mimeType"]=inputs["a"]["mimeType"]
        if "mediaType" in inputs["a"]:
            outputs["a"]["mediaType"]=inputs["a"]["mediaType"]
        if "encoding" in inputs["a"]:
            outputs["a"]["encoding"]=inputs["a"]["encoding"]
    else:
        outputs["a"]["value"]="Empty"
    if "b" in inputs:
        if "cache_file" in inputs["b"]:
            ds = osgeo.ogr.Open(inputs["b"]["cache_file"])
            path=""
            if outputs["b"]["mimeType"]=="application/json":
                path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".json"
                out_ds  = ogr.GetDriverByName("GeoJSON").CopyDataSource(ds, conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".json")
            else:
                path=conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".xml"
                out_ds  = ogr.GetDriverByName("GML").CopyDataSource(ds, conf["main"]["tmpPath"]+"/result-"+conf["lenv"]["usid"]+".xml")
        #outputs["b"]["value"]=open(path,"r",encoding="utf-8").read()
        outputs["b"]["generated_file"]=path
        #print(outputs["b"]["value"][:10],file=sys.stderr)
        if "dataType" in inputs["b"]:
            outputs["b"]["dataType"]=inputs["b"]["dataType"]
    else:
        outputs["b"]["value"]="Empty"
    if "c" in inputs:
        outputs["c"]["value"]=inputs["c"]["value"]
    else:
        outputs["c"]["value"]="Empty"
    print(inputs["c"],file=sys.stderr)
    return zoo.SERVICE_SUCCEEDED
