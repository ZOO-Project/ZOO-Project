import zoo
import json

def DeployPy(conf,inputs,outputs):

    # here you deploy the application
    service_id = "myNewService"


    conf["lenv"]["deployedServiceId"] = service_id

    # optional: response body
    output_json ={"operation":"deploy",
                  "status":"success",
                  "applicationPackage": inputs["applicationPackage"]["value"]}
    outputs["Result"]["value"]=json.dumps( output_json )

    # returning empty body response
    #outputs["Result"]["value"]=""

    # 6 is for SERVICE_DEPLOYED
    return 6
