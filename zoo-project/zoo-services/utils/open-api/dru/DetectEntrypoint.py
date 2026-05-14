#
# Author : Gérald Fenoy
#
# Copyright 2025 GeoLabs. All rights reserved.
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

from kubernetes import client, config
import time
import requests
import zoo
import os

class KubernetesClient:
    def __init__(self,conf):
        config.load_incluster_config()
        self.clientV1 = client.CoreV1Api()
        self.namespace = os.getenv("ORIGIN_NAMESPACE", "zoo")
        self.conf=conf
        self.create_crane_pod()

    def create_crane_pod(self):
        # Logic to create the crane pod
        pod_manifest = {
            "apiVersion": "v1",
            "kind": "Pod",
            "metadata": {
                "name": f"flask-{self.conf['lenv']['usid']}",
                "namespace": self.namespace,
                "labels": {
                    "app": f"flask-{self.conf['lenv']['usid']}",  # This label must match the Service selector
                },
            },
            "spec": {
                "containers": [
                    {
                        "name": "flask-container",
                        "image": "zooproject/crane-minimal:latest",  # Replace with your Flask image
                        "ports": [
                            {
                                "containerPort": 5000,  # Port on which Flask listens
                            }
                        ],
                        "env": [
                            {
                                "name": "IMAGE_TO_INSPECT",
                                "value": "nginx:latest",  # Example environment variable
                            }
                        ],
                    }
                ],
                "restartPolicy": "Never",  # The Pod does not restart after execution
            },
        }

        # Create the Flask Pod
        zoo.info("Creating Flask Pod...")
        self.clientV1.create_namespaced_pod(namespace=self.namespace, body=pod_manifest)
        zoo.success("Flask Pod created successfully.")

        # Wait for the Pod to be running
        zoo.info("Waiting for the Pod to be running...")
        while True:
            pod_status = self.clientV1.read_namespaced_pod_status(name=f"flask-{self.conf['lenv']['usid']}", namespace=self.namespace)
            if pod_status.status.phase == "Running":
                break
            time.sleep(1)
        zoo.success(f"Pod flask-{self.conf['lenv']['usid']} is running.")

        # Define the Service manifest
        service_manifest = {
            "apiVersion": "v1",
            "kind": "Service",
            "metadata": {
                "name": f"flask-service-{self.conf['lenv']['usid']}",
                "namespace": self.namespace,
            },
            "spec": {
                "selector": {
                    "app": f"flask-{self.conf['lenv']['usid']}",  # This label must match the Flask Pod label
                },
                "ports": [
                    {
                        "protocol": "TCP",
                        "port": 80,        # External port of the Service
                        "targetPort": 5000, # Internal port of the Pod (the one on which Flask listens)
                    }
                ],
            },
        }

        # Create the Service
        zoo.debug("Creating Service to expose Flask server...")
        self.clientV1.create_namespaced_service(namespace=self.namespace, body=service_manifest)
        zoo.success("Service created successfully.")

        zoo.info("Waiting for the Service to be ready...")
        while True:
            service_info = self.clientV1.read_namespaced_service(name=f"flask-service-{self.conf['lenv']['usid']}", namespace=self.namespace)
            if service_info.spec.cluster_ip:
                zoo.success(f"Service flask-service-{self.conf['lenv']['usid']} is up with IP : {service_info.spec.cluster_ip}")
                break
            time.sleep(1)
        zoo.success("Flask Service is ready.")
        return zoo.SERVICE_SUCCEEDED

    def delete_crane_pod(self):
        # Logic to delete the crane pod
        pod_name = f"flask-{self.conf['lenv']['usid']}"
        service_name = f"flask-service-{self.conf['lenv']['usid']}"
        zoo.info(f"Deleting Pod {pod_name}...")
        try:
            self.clientV1.delete_namespaced_pod(name=pod_name, namespace=self.namespace)
            zoo.success(f"Pod {pod_name} deleted successfully.")
        except Exception as e:
            zoo.error(f"Error deleting Pod {pod_name}: {e}")

        # Delete the Service
        zoo.info(f"Deleting Service {service_name}...")
        try:
            self.clientV1.delete_namespaced_service(name=service_name, namespace=self.namespace)
            zoo.success(f"Service {service_name} deleted successfully.")
        except Exception as e:
            zoo.error(f"Error deleting Service {service_name}: {e}")
        return zoo.SERVICE_SUCCEEDED

    def get_image_entrypoint(self,image_name):
        service_name = f"flask-service-{self.conf['lenv']['usid']}"
        zoo.info(f"Requesting entrypoint from {service_name}...")
        os.environ.pop("HTTP_PROXY", None)
        result = requests.get(f"http://{service_name}/{image_name}")
        # zoo.debug(json.dumps(result.json(), indent=4))
        return result.json()
