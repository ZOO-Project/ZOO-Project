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
