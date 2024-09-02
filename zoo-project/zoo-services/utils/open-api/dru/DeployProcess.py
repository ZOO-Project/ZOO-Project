#
# Author : Blasco Brauzzi, Fabrice Brito, Frank Löschau
#
# Copyright 2023 Terradue. All rights reserved.
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

import os

try:
    import zoo
except ImportError:
    print("Not running in zoo instance, using ZooStub object for testing")

    class ZooStub(object):
        def __init__(self):
            self.SERVICE_SUCCEEDED = 3
            self.SERVICE_FAILED = 4
            self.SERVICE_DEPLOYED = 6

        def update_status(self, conf, progress):
            print(f"Status {progress}")

        def _(self, message):
            print(f"invoked _ with {message}")

    conf = {}
    conf["lenv"] = {"message": ""}
    zoo = ZooStub()
    pass

from cookiecutter.main import cookiecutter
import sys
import shutil
import json
from pathlib import Path
import sys
from deploy_util import Process
import yaml
import requests
import botocore
from urllib.parse import urlparse
from collections import namedtuple
import os


def get_s3_settings():
    # you can extend this method to get the S3 credentials 
    return namedtuple(
        "S3Settings",
        ["region_name", "endpoint_url", "aws_access_key_id", "aws_secret_access_key"],
        defaults=[
            os.getenv("S3_REGION"),
            os.getenv("SERVICE_URL"),
            os.getenv("S3_ACCESS_KEY"),
            os.getenv("S3_SECRET_KEY"),
        ],
    )


class DeployService(object):
    def __init__(self, conf, inputs, outputs):

        self.conf = conf
        self.inputs = inputs
        self.outputs = outputs

        self.zooservices_folder = self.get_zoo_services_folder()

        self.cookiecutter_configuration_file = self._get_conf_value(
            key="configurationFile", section="cookiecutter"
        )
        self.cookiecutter_templates_folder = self._get_conf_value(
            key="templatesPath", section="cookiecutter"
        )
        self.cookiecutter_template_url = self._get_conf_value(
            key="templateUrl", section="cookiecutter"
        )

        self.cookiecutter_template_branch = self._get_conf_value_if_exists(
            key="templateBranch", section="cookiecutter"
        )

        self.tmp_folder = self._get_conf_value("tmpPath")

        self.process_id = self.conf["lenv"]["usid"]

        self.service_tmp_folder = self.create_service_tmp_folder()

        self.cwl_content = self.get_application_package()

        if "workflow_id" in self.conf["lenv"]:
            self.service_configuration = Process.create_from_cwl(self.cwl_content,self.conf["lenv"]["workflow_id"])
        else:
            self.service_configuration = Process.create_from_cwl(self.cwl_content)

        self.service_configuration.service_provider = (
            f"{self.service_configuration.identifier}.service"
        )
        self.service_configuration.service_type = "Python"

        self.conf["lenv"]["workflow_id"] = self.service_configuration.identifier
        self.conf["lenv"]["service_name"] = self.service_configuration.identifier

    def get_zoo_services_folder(self):

        # checking for namespace
        if "zooServicesNamespace" in self.conf and \
                "namespace" in self.conf["zooServicesNamespace"] and \
                "servicesNamespace" in self.conf and \
                "path" in self.conf["servicesNamespace"]:
            zooservices_folder = os.path.join(self.conf["servicesNamespace"]["path"],
                                              self.conf["zooServicesNamespace"]["namespace"])
        else:
        # if no namespace is used, we will use the default services path
            print(self.conf["renv"], file=sys.stderr)
            zooservices_folder = self._get_conf_value(
                key="CONTEXT_DOCUMENT_ROOT", section="renv"
            )

        # Checking if zoo can write in the servicePath
        self.check_write_permissions(zooservices_folder)

        return zooservices_folder

    def _get_conf_value(self, key, section="main"):

        print(section, file=sys.stderr)
        if key in self.conf[section].keys():
            return self.conf[section][key]
        else:
            raise ValueError(f"{key} not set, check configuration")

    def _get_conf_value_if_exists(self, key, section="main"):

        print(section, file=sys.stderr)
        if key in self.conf[section].keys():
            return self.conf[section][key]
        else:
            return None

    @staticmethod
    def check_write_permissions(folder):

        if not os.access(folder, os.W_OK):
            errorMsg = f"Cannot write to {folder}. Please check folder"
            print(errorMsg, file=sys.stderr)
            raise Exception(errorMsg)

    def create_service_tmp_folder(self):
        # creating the folder where we will download the applicationPackage
        tmp_path = os.path.join(self.tmp_folder, f"DeployProcess-{self.process_id}")
        try:
            os.makedirs(tmp_path)
        except Exception as e:
            print(e,file=sys.stderr)

        return tmp_path

    def get_application_package(self):

        # checking if applicationPackage exists
        if "applicationPackage" not in self.inputs.keys():
            raise ValueError("The inputs dot not include applicationPackage")

        # loading cwl in yaml object
        if "cache_file" in self.inputs["applicationPackage"]:
            cwl_content = yaml.safe_load(open(self.inputs["applicationPackage"]["cache_file"]).read())
        else:
            cwl_content = yaml.safe_load(self.inputs["applicationPackage"]["value"])

        return cwl_content

    def generate_service(self):

        path=None
        print(self.conf["lenv"],file=sys.stderr)
        if "noRunSql" in self.conf["lenv"]:
            # checking if the template location is remote or local
            if self.cookiecutter_template_url.endswith(".git"):

                template_folder = os.path.join(
                    self.cookiecutter_templates_folder,
                    Path(self.cookiecutter_template_url).stem,
                )

                # checking if template had already been cloned
                if os.path.isdir(template_folder):
                    shutil.rmtree(template_folder)

                # retrieving the branch to clone
                # if no branch is specified, we will clone the master branch
                cookiecutter_template_branch = self.cookiecutter_template_branch

                # cloning the template
                if cookiecutter_template_branch is not None:
                    os.system(
                        f"git clone -b {cookiecutter_template_branch} {self.cookiecutter_template_url} {template_folder}"
                    )
                else:
                    os.system(f"git clone {self.cookiecutter_template_url} {template_folder}")

            else:
                raise ValueError(
                    f"{self.cookiecutter_template_url} is not a valid git repo"
                )

            cookiecutter_values = {"service_name": self.service_configuration.identifier,
                                  "workflow_id": self.service_configuration.identifier,
                                  "conf": self.conf["cookiecutter"]}

            # Create project from template
            path = cookiecutter(
                template_folder,
                extra_context=cookiecutter_values,
                output_dir=self.service_tmp_folder,
                no_input=True,
                overwrite_if_exists=True,
                config_file=self.cookiecutter_configuration_file
            )

        if "metadb" not in self.conf:
            zcfg_file = os.path.join(
                self.zooservices_folder, f"{self.service_configuration.identifier}.zcfg"
            )
            with open(zcfg_file, "w") as file:
                self.service_configuration.write_zcfg(file)

        # checking if service had already been deployed previously
        # if yes, delete it before redeploy the new one
        old_service = os.path.join(self.zooservices_folder,self.service_configuration.identifier)
        if os.path.isdir(old_service):
            shutil.rmtree(old_service)
            if "metadb" not in self.conf:
                os.remove(zcfg_file)

        if "metadb" in self.conf and not("noRunSql" in self.conf["lenv"] and self.conf["lenv"]["noRunSql"] != "false"):
            rSql=self.service_configuration.run_sql(self.conf)
            if not(rSql):
                return False

        if path is not None:
            app_package_file = os.path.join(
                path,
                f"app-package.cwl",
            )

            with open(app_package_file, "w") as file:
                yaml.dump(self.cwl_content, file)

            shutil.move(path, self.zooservices_folder)

            shutil.rmtree(self.service_tmp_folder)

        self.conf["lenv"]["deployedServiceId"] = self.service_configuration.identifier

        return True

def duplicateMessage(conf,deploy_process):
    sLocation=conf["openapi"]["rootUrl"]+"/processes/"+deploy_process.service_configuration.identifier
    if "headers" in conf:
        conf["headers"]["Location"]=sLocation
    else:
        conf["headers"]={"Location": sLocation }
    conf["lenv"]["code"]="DuplicatedProcess"
    conf["lenv"]["message"]=zoo._("A service with the same identifier is already deployed")
    return zoo.SERVICE_FAILED

def storeCwl(conf,inputs,outputs):
    #if "metadb" not in self.conf or "noRunSql" in self.conf["lenv"] or self.conf["lenv"]["noRunSql"] != "false":
    #    return zoo.SERVICE_SUCCEEDED
    cwl_content=yaml.safe_load(inputs["applicationPackage"]["value"])
    if "zooServicesNamespace" in conf and \
            "namespace" in conf["zooServicesNamespace"] and \
            "servicesNamespace" in conf and \
            "path" in conf["servicesNamespace"]:
        zooservices_folder = os.path.join(conf["servicesNamespace"]["path"],
                                            conf["zooServicesNamespace"]["namespace"])
    else:
    # if no namespace is used, we will use the default services path
        if "CONTEXT_DOCUMENT_ROOT" in conf["renv"]:
            zooservices_folder=conf["renv"]["CONTEXT_DOCUMENT_ROOT"]
    app_package_file = os.path.join(
        zooservices_folder, conf["lenv"]["deployedServiceId"], f"{cwl_content['id']}.cwl"
    )
    try:
        with open(app_package_file, "w") as file:
            yaml.dump(cwl_content, file)
    except Exception as e:
        print(e,file=sys.stderr)
    return zoo.SERVICE_SUCCEEDED

def DeployProcess(conf, inputs, outputs):
    try:
        if "applicationPackage" in inputs.keys() and "isArray" in inputs["applicationPackage"].keys() and inputs["applicationPackage"]["isArray"]=="true":
            print("MULTIPLE execution untis",file=sys.stderr)
            for i in range(int(inputs["applicationPackage"]["length"])):
                if i==0:
                    lInputs = {
                        "applicationPackage": {
                            "value": inputs["applicationPackage"]["value"][0]
                        }
                    }
                    lInputs["applicationPackage"]["mimeType"] = inputs["applicationPackage"]["mimeType"][0]
                    deploy_process = DeployService(conf, lInputs, outputs)
                    res=deploy_process.generate_service()
                    if not(res):
                        return duplicateMessage(conf,deploy_process)
                if int(inputs["applicationPackage"]["length"])-1-i>0:
                    val="value_"+str(int(inputs["applicationPackage"]["length"])-1-i)
                    lInputs = {
                        "applicationPackage": {
                            "value":
                                inputs["applicationPackage"][val]
                        }
                    }
                else:
                    break
                lInputs["applicationPackage"]["mimeType"] = inputs["applicationPackage"]["mimeType"][0]
                if i<int(inputs["applicationPackage"]["length"])-1 and "noRunSql" in conf["lenv"] and conf["lenv"]["noRunSql"] != "false":
                    # Store the cwl file in the same location as the deployed process
                    storeCwl(conf, lInputs, outputs)
        else:
            deploy_process = DeployService(conf, inputs, outputs)
            res=deploy_process.generate_service()
            if not(res):
                return duplicateMessage(conf,deploy_process)
        response_json = {
            "message": f"Service {deploy_process.service_configuration.identifier} version {deploy_process.service_configuration.version} successfully deployed.",
            "service": deploy_process.service_configuration.identifier,
            "status": "success"
        }
        outputs["Result"]["value"] = json.dumps(response_json)
        return zoo.SERVICE_DEPLOYED
    except Exception as e:
        print("Exception in Python service",file=sys.stderr)
        print(e,file=sys.stderr)
        conf["lenv"]["message"]=str(e)
        return zoo.SERVICE_FAILED
