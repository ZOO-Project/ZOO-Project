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
