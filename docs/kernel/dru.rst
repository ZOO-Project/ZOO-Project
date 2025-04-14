.. _kernel-dru:
    
Optional DRU support
====================

The optional ZOO-Kernel DRU support gives you the opportunity to use
`OGC API - Processes - Part 2: Deploy, Replace, Undeploy
<https://docs.ogc.org/DRAFTS/20-044.html>`_ draft specificatoin. It
means that your server will provide new end-points to setup a new
process, update an existing one, or delete a deployed (mutable)
process.

The DRU support is designed to perform the processing function on a
`Kubernetes <https://kubernetes.io/>`_ cluster using the `Calrissian
<https://github.com/Duke-GCB/calrissian>`_, a `Common Workflow
Language runner <https://www.commonwl.org/>`_ for
Kubernetes. Calrissian uses CWL, an open standard defining Command
Line Tools and Workflow classes, over Kubernetes that enables the
implementation of each step in a workflow as a container. It provides
simple, flexible mechanisms for specifying constraints between the
steps in a workflow and artifact management for linking the output of
any step as an input to subsequent steps.

The ZOO-Project with DRU support is available as a 
`helm chart that <https://artifacthub.io/packages/helm/zoo-project/zoo-project-dru>`__
can be deployed on a Kubernetes cluster. 

.. |cwl-logo| image:: https://www.commonwl.org/assets/img/CWL-Logo-HD-cropped2.png
       :height: 100px
       :width: 100px
       :scale: 45%
       :alt: CWL logo


Installation on a local Kubernetes Cluster
------------------------------------------

Follow the steps described below in order to activate the ZOO-Project
optional DRU support.

Be sure you have all the prerequisistes setup before starting the
installation.

For deploying the ZOO-Project for developing and testing purposes
(e.g. on local machine), please proceed to the section
:ref:`Installation on a Minikube Cluster`.

For building the ZOO-Project docker image using the project source
code, please proceed to the section :ref:`Building the docker image
(for developper)`.

Prerequisites
.....................

   * `Docker <https://docs.docker.com/get-docker/>`_ (Apple silicon users should activate kubernetes from Docker Desktop, the virtual machine options should be set to Apple virtualization framework with the option "Use Rosetta for x86/64 emulation on Apple silicon" activated)
   * `Kubectl <https://kubernetes.io/fr/docs/tasks/tools/install-kubectl/>`_
   * `Helm <https://helm.sh/docs/intro/install/>`_
   * `Minikube <https://minikube.sigs.k8s.io/docs/start/>`_ (not required for Apple silicon)
   * `Skaffold <https://skaffold.dev/docs/install/>`_ (for developers)
   * latest `ZOO-Project
     <https://github.com/ZOO-Project/ZOO-Project/>`_
     version

Checking the requirements
*************************

After installing these tools, ensure they are available in your terminal by running the following commands:

.. code-block:: bash

       docker --version
       kubectl version --client
       helm version
       skaffold version
       # The following command is not required to work on Apple silicon
       minikube version


Add the helm repositories
*************************

Before starting the next steps, make sure to run the following command to
ensure that your local environment is ready to deploy the ZOO-Project-DRU.

.. code-block:: bash

       helm repo add localstack https://helm.localstack.cloud
       helm repo add zoo-project https://zoo-project.github.io/charts/


.. _Installation using skaffold:

Build and install with skaffold
......................................

.. note::
    
      💡 The `skaffold.yaml` file is located in the root directory of the
      ZOO-Project repository. It is used to build
      the docker image and deploy the ZOO-Project on a Kubernetes
      cluster. Usually, you should use a specific version of the
      ZOO-Project-DRU docker image, as defined in the Helm chart. 
      During the development phase, skaffold can be used to build and deploy
      using the current state of the local source code.

The ZOO-Project with DRU support can be setup using skaffold. There are
multiple profiles defined in the `skaffold.yaml` file. The profiles are the
following:

   * `hostpath`: 
      it define the storageClass to use for the persistent volumes. It should
      be used when deploying on Apple silicon.
   * `wes`: 
      it will deploy the ZOO-Project-DRU on a Kubernetes cluster with
      the ZOO-Project-DRU docker image built from the source code and
      the WES support (using the 
      `zoo-wes-runner <https://github.com/ZOO-Project/zoo-wes-runner>`_).
   * `argo`: 
      it will deploy the ZOO-Project-DRU on a Kubernetes cluster 
      with the ZOO-Project-DRU docker image built from the source code and
      the Argo Workflows support (using the `zoo-argowf-runner 
      <https://github.com/EOEPCA/zoo-argowf-runner/tree/develop>`_).

If you don't specify any profile, skaffold will deploy the ZOO-Project-DRU on
a Kubernetes cluster with the ZOO-Project-DRU docker image built from the
source code and the default `zoo-calrissian-runner 
<https://github.com/EOEPCA/zoo-calrissian-runner>`_.

For instance, to deploy the ZOO-Project-DRU on a x86/amd64 Kubernetes cluster
with the default zoo-calrissian-runner, use the following command:

.. code-block:: bash

       git clone https://github.com/ZOO-Project/ZOO-Project.git
       cd ZOO-Project
       skaffold dev

For deploying on arm64 (Apple silicon) Kubernetes cluster, use the following
command:

.. code-block:: bash

       git clone https://github.com/ZOO-Project/ZOO-Project.git
       cd ZOO-Project
       # Pull the latest Docker image used from the latest Helm chart 
       docker pull \
          zooproject/zoo-project:$(curl \
            https://raw.githubusercontent.com/ZOO-Project/charts/refs/heads/main/zoo-project-dru/values.yaml \
            | grep tag | grep dru | head -1 | awk {'print $2'}) \
          --platform linux/amd64
       skaffold dev -p hostpath \
          --platform=linux/amd64 \
          --enable-platform-node-affinity=true


.. note::
    
      💡 You can combine multiple profiles, so using `-p hostpath,wes` it will
      deploy the WES support using hostpath as the storageClass.

For deploying the ZOO-Project-DRU using Argo Workflows, you don't need to
deploy any object storage, the ZOO-Project will use the one created by the Argo
server. In consequence, we also provide an optional `skaffold-argo.yaml` file
that can be used to deploy the ZOO-Project-DRU using Argo Workflows without
extra object storage. To deplot this version on a x86/amd64 Kubernetes cluster,
use the following command:

.. code-block:: bash

       git clone https://github.com/ZOO-Project/ZOO-Project.git
       de ZOO-Project
       skaffold dev -f docker/dru/skaffold-argo.yaml


.. _Installation on a Minikube Cluster:

Installation on a Minikube Cluster
.................................

ZOO-Project DRU Helm chart
****************************

The current helm chart is available from here:
https://github.com/ZOO-Project/charts.git. You will need to clone
this repository and make the following modifications to it.

.. code-block:: guess

     git clone https://github.com/ZOO-Project/charts.git


Create a dedicated namespace for the ZOO-Project
################################################

This namespace will be reserved for the operation of the ZOO-Project
in order to separate the ZOO-Project proper execution from its
processings job on Kubernetes. 

For instance, let's create a `zoo-project-dru` namespace.

.. code-block:: guess

       kubectl create ns zoo

.. _Deploy using Helm:

Deploy using Helm
#################

The ZOO-Project with DRU support is setup via helm specifying an
optional "release name".

For instance, we will install the ZOO-Project with the name `zoo-project-dru`.

.. code-block:: guess

       helm upgrade --install zoo-project-dru zoo-project-dru -f zoo-project-dru/values_minikube.yaml -n zoo
       

.. literalinclude:: ../_static/minikube_upgrade_output.txt 


.. note::

   💡 detailed information of all the chart parameter is available in
   the `chart README
   <https://github.com/ZOO-Project/charts/blob/main/zoo-project-dru/README.md>`_

Check that service is available

.. code-block:: guess

       kubectl get service zoo-project-dru-service --namespace zoo

Should return something like:

.. code-block:: guess

       NAME                      TYPE        CLUSTER-IP       EXTERNAL-IP   PORT(S)   AGE
       zoo-project-dru-service   ClusterIP   10.104.248.111   <none>        80/TCP    8m

Get the application URL by running these commands:

.. code-block:: guess

       export POD_WS_NAME=$(kubectl get pods --namespace zoo -l "app.kubernetes.io/name=zoo-project-dru-websocketd,app.kubernetes.io/instance=zoo-project-dru-websocketd" -o jsonpath="{.items[0].metadata.name}")
       export CONTAINER_WS_PORT=$(kubectl get pod --namespace zoo $POD_WS_NAME -o jsonpath="{.spec.containers[0].ports[0].containerPort}")
       echo "Visit ws://127.0.0.1:8888 to use your application"
       kubectl --namespace zoo port-forward $POD_WS_NAME 8888:$CONTAINER_WS_PORT &
       export POD_NAME=$(kubectl get pods --namespace zoo -l "app.kubernetes.io/name=zoo-project-dru,app.kubernetes.io/instance=zoo-project-dru" -o jsonpath="{.items[0].metadata.name}")
       export CONTAINER_PORT=$(kubectl get pod --namespace zoo $POD_NAME -o jsonpath="{.spec.containers[0].ports[0].containerPort}")
       echo "Visit http://127.0.0.1:8080 to use your application"
       kubectl --namespace zoo port-forward $POD_NAME 8080:$CONTAINER_PORT

At this step, the ZOO-Project-DRU deployment using helm chart should
be accesible from this address:
http://127.0.0.1:8080/ogc-api/api.html.


Remove ZOO-Project-DRU from Minikube
####################################

In case you want to remove the ZOO-Project-DRU from your Minikube, you
can use the commands bellow.

.. code-block:: guess

       helm uninstall zoo-project-dru -n zoo && \
       kubectl delete pvc data-zoo-project-dru-postgresql-0 \
                          data-zoo-project-dru-rabbitmq-0 \
                          redis-data-zoo-project-dru-redis-master-0\
                          redis-data-zoo-project-dru-redis-replicas-0\
                          -n zoo


.. _Building the docker image (for developper):

Building the docker image (for developper)
##########################################

For a developper it may be convenient to test a new functionality
on Minikube. To do so, use the following commands to build a dedicated
ZOO-Project (with DRU support enabled) binary image to be used to
deploy using helm.

In case you have modified the ZOO-Project source code and want to test
it on your Minikube, you can then update the `Dockerfile
<https://github.com/EOEPCA/proc-ades-dev/blob/develop/Dockerfile#L84>`_
to use a modified copy of the ZOO-Project located in the `ZPGIT`
directory by commenting line 83 and uncommenting line 84.

.. code-block:: guess

       eval $(minikube docker-env)
       docker build . -t zoo-project/zoo-project-dru:myversion-X.Y.Z


To ensure that helm will use this specific image during the
deployment, you can change the following values from the
`zoo-project-dru/values.yaml 
<https://github.com/ZOO-Project/charts/blob/main/zoo-project-dru/values.yaml>`_
file.

Replace the following (for both `zookernel` and `zoofpm`):

.. code-block:: guess

       pullPolicy: IfNotPresent

By:

.. code-block:: guess

       pullPolicy: Never

Also, make sure to update the tag for both `zoofpm` and `zookernel`
images to the one defined during the `docker build` command used to
build the docker image locally.


Now you can deploy on Minikube as defined here: :ref:`Deploy using Helm`


.. _CWL Supported types:

CWL Supported types
-------------------

The ZOO-Project handle `three kind of input types <../services/zcfg-reference.html#type-of-data-nodes>`_ for every process/service.
One of them are the LiteralData, which are simple values like strings, numbers, etc.

You can find below the exhaustive list of supported types for the LiteralData 
depending on the type defined in the CWL used to deploy a process.

.. list-table:: CWL Types for LiteralData
   :widths: 15 65
   :align: center
   :header-rows: 1

   * - CWL Type
     - Schema Definition
   * - `string`
     - .. code-block:: guess

                    "schema": {
                      "type": "string"
                     }
   * - `int`
     - .. code-block:: guess

                    "schema": {
                      "type": "integer"
                     }
   * - `long`
     - .. code-block:: guess

                    "schema": {
                      "type": "integer"
                     }
   * - `float`
     - .. code-block:: guess

                    "schema": {
                      "type": "number",
                      "format": "float"
                     }
   * - `double`
     - .. code-block:: guess

                    "schema": {
                      "type": "number",
                      "format": "double"
                     }
   * - `boolean`
     - .. code-block:: guess

                    "schema": {
                      "type": "boolean"
                     }


OGC API - Processes - Part 1: Core introduced the `format` key in the `schema` 
property giving the opportunity to provide additional semantic context that 
can aid in the interpretation of the data (cf. 
`Table 13 Additional values for the JSON schema format key for OGC Process Description <https://docs.ogc.org/DRAFTS/18-062.html#format-key-values>`_).
The `format` key can be particularly useful when dealing with complex data 
types (especially when they have the same media type) or when integrating with 
other systems that require specific data formats.

A set of `CWL Custom Types <https://eoap.github.io/schemas/>`_ has beend 
defined by the `Earth Observation Application Package (EOAP) GitHub 
Organization <https://github.com/eoap>`_. 
They helps to define the data structure used in the CWL workflow description
for the additional values to the JSON Schema formats key for OGC Process 
Descripton. They are managed on the 
`Schemas EOAP GitHub repository <https://github.com/eoap/schemas>`_.

They are organized in the following three definition files:

- **OGC**: `ogc.yaml <https://raw.githubusercontent.com/eoap/schemas/refs/heads/main/ogc.yaml>`_
- **GeoJSON**: `geojson.yaml <https://raw.githubusercontent.com/eoap/schemas/refs/heads/main/geojson.yaml>`_
- **STAC**: `stac.yaml <https://raw.githubusercontent.com/eoap/schemas/refs/heads/main/stac.yaml>`_


In the `oas.cfg` file, the `schemas` property can be defined to list the 
predefined schemas definition used in the exposed OpenAPI.
The value explicitely linked to the `format` key in the `schema` property is 
the URL of the OpenAPI or JSON Schema definition file.

Below is the list of the predefined format names and their corresponding 
schemas.

.. code-block:: guess

       [schemas]
       length=7
       value=https://schemas.opengis.net/ogcapi/processes/part1/1.0/openapi/schemas/bbox.yaml
       name=ogc-bbox
       value_1=https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/featureCollectionGeoJSON.yaml
       name_1=geojson-feature-collection
       value_2=https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/featureGeoJSON.yaml
       name_2=geojson-feature
       value_3=https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/geometryGeoJSON.yaml
       name_3=geojson-geometry
       value_4=https://raw.githubusercontent.com/radiantearth/stac-api-spec/refs/heads/release/v1.0.0/stac-spec/catalog-spec/json-schema/catalog.json
       name_4=stac-catalog
       value_5=https://raw.githubusercontent.com/radiantearth/stac-api-spec/refs/heads/release/v1.0.0/stac-spec/collection-spec/json-schema/collection.json
       name_5=stac-collection
       value_6=https://raw.githubusercontent.com/radiantearth/stac-api-spec/refs/heads/release/v1.0.0/stac-spec/item-spec/json-schema/item.json
       name_6=stac-item


The table below provides the list of the predefined supported CWL Custom Types 
and their corresponding format key values and OGC Processes Description.

.. list-table:: CWL Custom Types and OGC Processes Description
   :widths: 15 10 75
   :align: center
   :header-rows: 1

   * - CWL Custom Type
     - Short code
     - OGC Processes Description
   * - geojson.yaml#FeatureCollection
     - geojson-feature-collection
     - .. code-block:: guess

                    "schema": {
                      "allOf": [
                        {
                          "format": "geojson-feature-collection"
                        },
                        {
                          "$ref": "https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/featureCollectionGeoJSON.yaml"
                        }
                      ]
                     }
   * - geojson.yaml#Feature
     - geojson-feature
     - .. code-block:: guess

                    "schema": {
                      "allOf": [
                        {
                          "format": "geojson-feature"
                        },
                        {
                          "$ref": "https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/featureGeoJSON.yaml"
                        }
                      ]
                     }
   * - * geojson.yaml#GeometryCollection
       * geojson.yaml#Polygon
       * geojson.yaml#LineString
       * geojson.yaml#Point
       * geojson.yaml#MultiPolygon
       * geojson.yaml#MultiLineString
       * geojson.yaml#MultiPoint
     - geojson-geometry
     - .. code-block:: guess

                    "schema": {
                      "allOf": [
                        {
                          "format": "geojson-geometry"
                        },
                        {
                          "$ref": "https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/geometryGeoJSON.yaml"
                        }
                      ]
                     }
   * - ogc.yaml#BBox
     - ogc-bbox
     - .. code-block:: guess

                    "schema": {
                      "allOf": [
                        {
                          "format": "ogc-bbox"
                        },
                        {
                          "$ref": "https://schemas.opengis.net/ogcapi/processes/part1/1.0/openapi/schemas/bbox.yaml"
                        }
                      ]
                     }
   * - None
     - epsg-code
     - None
   * - None
     - wkt2-def
     - None
   * - None
     - cql2-text
     - None
   * - None
     - cql2-json  
     - None
   * - None
     - collection-id
     - None
   * - stac.yaml#Collection
     - stac-collection
     - .. code-block:: guess

                    "schema": {
                      "allOf": [
                        {
                          "format": "stac-collection"
                        },
                        {
                          "$ref": "https://raw.githubusercontent.com/radiantearth/stac-api-spec/refs/heads/release/v1.0.0/stac-spec/collection-spec/json-schema/collection.json"
                        }
                      ]
                     }
   * - stac.yaml#Catalog
     - stac-catalog
     - .. code-block:: guess

                    "schema": {
                      "allOf": [
                        {
                          "format": "stac-catalog"
                        },
                        {
                          "$ref": "https://raw.githubusercontent.com/radiantearth/stac-api-spec/refs/heads/release/v1.0.0/stac-spec/catalog-spec/json-schema/catalog.json"
                        }
                      ]
                     }
   * - None
     - stac-itemCollection
     - None
   * - stac.yaml#Item
     - stac-item
     - .. code-block:: guess

                    "schema": {
                      "allOf": [
                        {
                          "format": "stac-item"
                        },
                        {
                          "$ref": "https://raw.githubusercontent.com/radiantearth/stac-api-spec/refs/heads/release/v1.0.0/stac-spec/item-spec/json-schema/item.json"
                        }
                      ]
                     }
   * - None
     - ogc-feature-collection
     - None
   * - None
     - ogc-coverage-collection
     - None


