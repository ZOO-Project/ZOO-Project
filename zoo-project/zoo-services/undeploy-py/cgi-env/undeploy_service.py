import zoo

def UndeployPy(conf,inputs,outputs):
    conf["lenv"]["message"]="successfully undeployed"

    # 7 is for SERVICE_UNDEPLOYED
    return 7
