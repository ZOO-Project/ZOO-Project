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

.. |slurm| image:: https://slurm.schedmd.com/slurm_logo.png
       :height: 100px
       :width: 100px
       :scale: 45%
       :alt: Slurm logo


Installation on a Minikube Cluster
------------------------------

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

   * `Docker <https://docs.docker.com/get-docker/>`_
   * `Kubectl <https://kubernetes.io/fr/docs/tasks/tools/install-kubectl/>`_
   * `Helm <https://helm.sh/docs/intro/install/>`_
   * `Minikube <https://minikube.sigs.k8s.io/docs/start/>`_
   * latest `ZOO-Project
     <https://github.com/ZOO-Project/ZOO-Project/>`_
     version

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
##########################################

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
   * - `float`
     - .. code-block:: guess

                    "schema": {
                      "type": "number"
                     }
   * - `double`
     - .. code-block:: guess

                    "schema": {
                      "type": "double"
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
       value_3=https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/geometrycollectionGeoJSON.yaml
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
                          "$ref": "https://schemas.opengis.net/ogcapi/features/part1/1.0/openapi/schemas/geometrycollectionGeoJSON.yaml"
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


