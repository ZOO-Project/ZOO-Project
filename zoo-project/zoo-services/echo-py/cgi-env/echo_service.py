import zoo
import sys

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
        outputs["b"]["value"]=inputs["b"]["value"]
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
