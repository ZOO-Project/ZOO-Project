import zoo
import json

def UndeployPy(conf,inputs,outputs):
    outputs["Result"]["value"]="successfully undeployed"
    return zoo.SERVICE_SUCCEEDED