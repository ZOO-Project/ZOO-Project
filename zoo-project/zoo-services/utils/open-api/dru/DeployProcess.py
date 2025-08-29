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
    from ZooStub import ZooStub
    conf = {}
    conf["lenv"] = {"message": ""}
    zoo = ZooStub()
    pass

from cookiecutter.main import cookiecutter
import os
import sys
import shutil
import json
import yaml
from pathlib import Path
from deploy_util import Process
from collections import namedtuple
from deploy_util import Services


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


class DeployService(Services):
    def __init__(self, conf, inputs, outputs):

        super().__init__(conf,inputs,outputs)

        self.cookiecutter_configuration_file = self._get_conf_value(
            key="configurationFile", section="cookiecutter"
        )
        self.cookiecutter_templates_folder = self._get_conf_value(
            key="templatesPath", section="cookiecutter"
        )
        zoo.info(f"Using templates folder {self.cookiecutter_templates_folder}")

        self.cookiecutter_template_url = self._get_conf_value(
            key="templateUrl", section="cookiecutter"
        )
        zoo.info(f"Using template url {self.cookiecutter_template_url}")

        self.cookiecutter_template_branch = self._get_conf_value_if_exists(
            key="templateBranch", section="cookiecutter"
        )
        if self.cookiecutter_template_branch is not None:
            zoo.info(f"Using template branch {self.cookiecutter_template_branch}")

        self.tmp_folder = self._get_conf_value("tmpPath")

        self.process_id = self.conf["lenv"]["usid"]
        zoo.info(f"Process ID {self.process_id}")

        self.service_tmp_folder = self.create_service_tmp_folder()

        self.cwl_content = self.get_application_package()

        if "workflow_id" in self.conf["lenv"]:
            self.service_configuration = Process.create_from_cwl(
                self.cwl_content, self.conf["lenv"]["workflow_id"]
            )
        else:
            self.service_configuration = Process.create_from_cwl(self.cwl_content)

        self.service_configuration.service_provider = (
            f"{self.service_configuration.identifier}.service"
        )
        self.service_configuration.service_type = "Python"

        self.conf["lenv"]["workflow_id"] = self.service_configuration.identifier
        self.conf["lenv"]["service_name"] = self.service_configuration.identifier

    def create_service_tmp_folder(self):
        # creating the folder where we will download the applicationPackage
        tmp_path = os.path.join(self.tmp_folder, f"DeployProcess-{self.process_id}")
        try:
            os.makedirs(tmp_path)
            zoo.info(f"Temporary folder created in {tmp_path}")
        except Exception as e:
            zoo.error(str(e))

        return tmp_path

    def get_application_package(self):

        # checking if applicationPackage exists
        if "applicationPackage" not in self.inputs.keys():
            zoo.error("applicationPackage not found in inputs")
            raise ValueError("The inputs dot not include applicationPackage")

        # loading cwl in yaml object
        if "cache_file" in self.inputs["applicationPackage"]:
            zoo.info(
                f"Loading CWL from cache file {self.inputs['applicationPackage']['cache_file']}"
            )
            cwl_content = yaml.safe_load(
                open(self.inputs["applicationPackage"]["cache_file"]).read()
            )
        else:
            zoo.info("Loading CWL from value")
            cwl_content = yaml.safe_load(self.inputs["applicationPackage"]["value"])

        return cwl_content

    def generate_service(self):

        path = None
        if "noRunSql" not in self.conf["lenv"]:
            # checking if the template location is remote or local
            if self.cookiecutter_template_url.endswith(".git"):
                zoo.info(f"Cloning {self.cookiecutter_template_url}")
                template_folder = os.path.join(
                    self.cookiecutter_templates_folder,
                    Path(self.cookiecutter_template_url).stem,
                )

                # checking if template had already been cloned
                if os.path.isdir(template_folder):
                    zoo.info(f"Template already cloned in {template_folder}, pull only")
                    os.system(
                        f"cd {template_folder}; git pull > log"
                    )
                else:
                    # retrieving the branch to clone
                    # if no branch is specified, we will clone the master branch
                    cookiecutter_template_branch = self.cookiecutter_template_branch

                    # cloning the template

                    if cookiecutter_template_branch is not None:
                        zoo.info(
                            f"Cloning {self.cookiecutter_template_url} branch {cookiecutter_template_branch}"
                        )
                        os.system(
                            f"git clone -b {cookiecutter_template_branch} {self.cookiecutter_template_url} {template_folder}"
                        )
                    else:
                        zoo.info(f"Cloning {self.cookiecutter_template_url}")
                        os.system(
                            f"git clone {self.cookiecutter_template_url} {template_folder}"
                        )

            else:
                zoo.error(
                    f"{self.cookiecutter_template_url} is not a valid git repo"
                )
                raise ValueError(
                    f"{self.cookiecutter_template_url} is not a valid git repo"
                )

            cookiecutter_values = {
                "service_name": self.service_configuration.identifier,
                "workflow_id": self.service_configuration.identifier,
                "conf": self.conf["cookiecutter"],
            }

            # Create project from template
            path = cookiecutter(
                template_folder,
                extra_context=cookiecutter_values,
                output_dir=self.service_tmp_folder,
                no_input=True,
                overwrite_if_exists=True,
                config_file=self.cookiecutter_configuration_file,
            )

            # checking if the service has been deployed
            if os.path.isdir(path):
                zoo.info(
                    f"Service {self.service_configuration.identifier} generated in {path}"
                )

        if "metadb" not in self.conf:
            zoo.info(f"Writing {self.service_configuration.identifier}.zcfg")
            zcfg_file = os.path.join(
                self.zooservices_folder, f"{self.service_configuration.identifier}.zcfg"
            )
            with open(zcfg_file, "w") as file:
                self.service_configuration.write_zcfg(file)

        # In case the service is not run asynchronously and method is PUT,
        # checking if service had already been deployed previously
        # if yes, delete it before redeploy the new one.
        old_service = os.path.join(
            self.zooservices_folder, self.service_configuration.identifier
        )
        if (
            os.path.isdir(old_service)
            and "noRunSql" not in self.conf["lenv"]
            and "orequest_method" in self.conf["lenv"]
        ):
            zoo.info(
                f"Got PUT, service {self.service_configuration.identifier} already deployed, deleting it"
            )
            shutil.rmtree(old_service)
            if "metadb" not in self.conf:
                os.remove(zcfg_file)

        if "metadb" in self.conf and not (
            "noRunSql" in self.conf["lenv"] and self.conf["lenv"]["noRunSql"] != "false"
        ):
            rSql = self.service_configuration.run_sql(self.conf)
            if rSql:
                zoo.info("Service inserted in the db")

            else:
                zoo.error("Failed to insert the service metadata in the db")
                return False

        if path is not None and "noRunSql" not in self.conf["lenv"]:

            app_package_file = os.path.join(
                path,
                f"app-package.cwl",
            )

            zoo.info(f"Storing the CWL file in {app_package_file}")
            with open(app_package_file, "w") as file:
                yaml.dump(self.cwl_content, file)

            zoo.info(f"Moving {path} to {self.zooservices_folder}")
            shutil.move(path, self.zooservices_folder)

            zoo.info("Deleting the temporary folder")
            shutil.rmtree(self.service_tmp_folder)

        self.conf["lenv"]["deployedServiceId"] = self.service_configuration.identifier
        zoo.info(f"Service {self.service_configuration.identifier} deployed")
        return True


def duplicateMessage(conf, deploy_process):
    sLocation = (
        conf["openapi"]["rootUrl"]
        + "/processes/"
        + deploy_process.service_configuration.identifier
    )
    if "headers" in conf:
        conf["headers"]["Location"] = sLocation
    else:
        conf["headers"] = {"Location": sLocation}
    conf["lenv"]["code"] = "DuplicatedProcess"
    conf["lenv"]["message"] = zoo._(
        "A service with the same identifier is already deployed"
    )
    zoo.error(conf["lenv"]["message"])
    return zoo.SERVICE_FAILED


def storeCwl(conf, inputs, outputs):
    # saves addional cwl files in the same location as the deployed process
    cwl_content = yaml.safe_load(inputs["applicationPackage"]["value"])
    if (
        "zooServicesNamespace" in conf
        and "namespace" in conf["zooServicesNamespace"]
        and "servicesNamespace" in conf
        and "path" in conf["servicesNamespace"]
    ):
        zooservices_folder = os.path.join(
            conf["servicesNamespace"]["path"], conf["zooServicesNamespace"]["namespace"]
        )
    else:
        # if no namespace is used, we will use the default services path
        if "CONTEXT_DOCUMENT_ROOT" in conf["renv"]:
            zooservices_folder = conf["renv"]["CONTEXT_DOCUMENT_ROOT"]

    app_package_file = os.path.join(
        zooservices_folder,
        conf["lenv"]["deployedServiceId"],
        f"{cwl_content['id']}.cwl",
    )

    try:
        zoo.info(f"Storing the CWL file in {app_package_file}")
        with open(app_package_file, "w") as file:
            yaml.dump(cwl_content, file)
    except Exception as e:
        zoo.error(str(e))


def DeployProcess(conf, inputs, outputs):
    try:
        # If noRunSql is true, we will try to detect the entrypoint of the docker image(s)
        if "noRunSql" in conf["lenv"]:
            if os.environ.get("ZOOFPM_DETECT_ENTRYPOINT","false") == "false":
                zoo.info("Nothing to do here.")
                return zoo.SERVICE_SUCCEEDED
            from DetectEntrypoint import KubernetesClient
            # Here we need to invoke the crane pod to detect potential entrypoint
            zooservices_folder = os.path.join(
                conf["auth_env"]["cwd"],
                conf["lenv"]["deployedServiceId"]
            )
            f = open(os.path.join(zooservices_folder, "app-package.cwl"),"r")
            cwl_content = yaml.safe_load(f.read())
            f.close()
            try:
                has_updated_values = False
                k8s_client = KubernetesClient(conf)
                # for each class: CommandLineTool
                for graph_item in cwl_content.get("$graph",[]):
                    if graph_item.get("class") == "CommandLineTool":
                        zoo.info(f"Found CommandLineTool: {graph_item.get('hints', {}).get('DockerRequirement', {}).get('dockerPull')}")
                        image_config = k8s_client.get_image_entrypoint(graph_item.get("hints", {}).get("DockerRequirement", {}).get("dockerPull"))
                        # Detect entry point using the remote service
                        if image_config["config"].get("Entrypoint") is not None:
                            has_updated_values = True
                            zoo.info(f"Entrypoint found: {image_config['config']['Entrypoint']}")
                            if graph_item.get("baseCommand") is not None:
                                graph_item["baseCommand"] = graph_item.get("baseCommand").prepend(image_config['config']['Entrypoint'])
                            else:
                                graph_item["baseCommand"] = image_config['config']['Entrypoint']
                        else:
                            zoo.info("No Entrypoint found")
                k8s_client.delete_crane_pod()
                # Here we need to save the updated CWL graph
                if has_updated_values:
                    with open(os.path.join(zooservices_folder, "app-package.cwl"), "w") as f:
                        yaml.dump(cwl_content, f)
                        f.close()
                    zoo.success("CWL file updated successfully.")
                else:
                    zoo.success("No updates were made to the CWL file.")
            except Exception as e:
                zoo.error(f"Failed to get the image entrypoint(s): {e}")
                try:
                    k8s_client.delete_crane_pod()
                except Exception as e:
                    zoo.error(f"Failed to delete the crane pod: {e}")
                return zoo.SERVICE_FAILED
            return zoo.SERVICE_SUCCEEDED

        if (
            "applicationPackage" in inputs.keys()
            and "isArray" in inputs["applicationPackage"].keys()
            and inputs["applicationPackage"]["isArray"] == "true"
        ):

            for i in range(int(inputs["applicationPackage"]["length"])):
                if i == 0:

                    zoo.info("Deploying the Application Package")

                    lInputs = {
                        "applicationPackage": {
                            "value": inputs["applicationPackage"]["value"][0]
                        }
                    }
                    lInputs["applicationPackage"]["mimeType"] = inputs[
                        "applicationPackage"
                    ]["mimeType"][0]

                    deploy_process = DeployService(conf, lInputs, outputs)
                    res = deploy_process.generate_service()
                    if res:
                        zoo.info(
                            f"Service {deploy_process.service_configuration.identifier} deployed"
                        )
                    else:
                        zoo.info(
                            f"Service {deploy_process.service_configuration.identifier} already deployed"
                        )
                        return duplicateMessage(conf, deploy_process)

                if int(inputs["applicationPackage"]["length"]) - 1 - i > 0:
                    val = "value_" + str(
                        int(inputs["applicationPackage"]["length"]) - 1 - i
                    )
                    lInputs = {
                        "applicationPackage": {
                            "value": inputs["applicationPackage"][val]
                        }
                    }
                else:
                    break

                lInputs["applicationPackage"]["mimeType"] = inputs[
                    "applicationPackage"
                ]["mimeType"][0]

                if (
                    i < int(inputs["applicationPackage"]["length"]) - 1
                    and "noRunSql" in conf["lenv"]
                    and conf["lenv"]["noRunSql"] != "false"
                ):
                    # Store the cwl file in the same location as the deployed process
                    zoo.info("Storing the additional CWL file(s)")
                    storeCwl(conf, lInputs, outputs)
        else:

            zoo.info("Got an OGC EO Application Package with a reference to a CWL")

            deploy_process = DeployService(conf, inputs, outputs)
            res = deploy_process.generate_service()
            if res:
                zoo.info(
                    f"Service {deploy_process.service_configuration.identifier} deployed"
                )
            else:
                zoo.info(
                    f"Service {deploy_process.service_configuration.identifier} already deployed"
                )
                return duplicateMessage(conf, deploy_process)

        zoo.info(
            f"Service {deploy_process.service_configuration.identifier} version {deploy_process.service_configuration.version} successfully deployed."
        )

        response_json = {
            "message": f"Service {deploy_process.service_configuration.identifier} version {deploy_process.service_configuration.version} successfully deployed.",
            "service": deploy_process.service_configuration.identifier,
            "status": "success",
        }
        outputs["Result"]["value"] = json.dumps(response_json)

        return zoo.SERVICE_DEPLOYED

    except Exception as e:
        zoo.error("Failed to deploy the service")
        zoo.error(str(e))
        if "headers" not in conf:
            conf["headers"]={}
        conf["headers"]["status"]="400 Bad Request"
        conf["lenv"]["code"]="WorkflowNotFound"
        #conf["lenv"]["exception"]="http://www.opengis.net/def/exceptions/ogcapi-processes-2/1.0/workflow-not-found"
        conf["lenv"]["message"] = str(e)
        return zoo.SERVICE_FAILED
