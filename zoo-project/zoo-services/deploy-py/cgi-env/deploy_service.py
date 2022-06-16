import zoo
import json

def DeployPy(conf,inputs,outputs):
    output_json ={"operation":"deploy",
                 "status":"success",
                 "applicationPackage": inputs["applicationPackage"]["value"]}
    outputs["Result"]["value"]=json.dumps( output_json )


    return zoo.SERVICE_SUCCEEDED