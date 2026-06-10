Installation Guide
==================

This section explains how to install and run ZOO-Project.

For most users, the recommended way to start is with Docker,
as it provides a consistent environment across platforms.

Quick Start (Recommended)
-------------------------

Requirements
^^^^^^^^^^^^

Before starting, ensure the following tools are installed:

- Docker
- Docker Compose

Start ZOO-Project
^^^^^^^^^^^^^^^^^

Run the following commands:

.. code-block:: bash

   git clone https://github.com/ZOO-Project/ZOO-Project.git
   cd ZOO-Project
   mkdir -p docker/tmp
   docker compose up -d

.. note::

   Ensure the ``docker/`` directory is writable by the container user
   if your system uses restricted file permissions.

.. note::

   The Docker image used by default is defined in the ``docker-compose.yml`` file.
   You can modify the image tag to use a different version or flavor of ZOO-Project
   (for example, a specific release or a custom build).
   
   Available image tags can be found on Docker Hub:
   https://hub.docker.com/r/zooproject/zoo-project/tags

Verify Installation
^^^^^^^^^^^^^^^^^^^

After startup, verify that the service is available:

- Main interface:
  http://localhost/

- OGC API endpoint:
  http://localhost/ogc-api/

- WPS GetCapabilities:
  http://localhost/cgi-bin/zoo_loader.cgi?Request=GetCapabilities&Service=WPS


Configuration
-------------

The default Docker setup uses the following files:

- ``docker-compose.yml``
- ``docker/main.cfg``
- ``docker/oas.cfg``

These files can be adjusted to customize your local deployment.


Additional Installation Methods
-------------------------------

For manual installation or custom deployments, see:

.. toctree::
   :maxdepth: 1

   advanced
