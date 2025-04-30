
.. include:: <xhtml1-lat1.txt>
.. include:: <xhtml1-symbol.txt>

.. _install-ondocker:

Installation using Docker 
=================================

This guide provides step-by-step instructions to install and run the ZOO-Project using Docker. It utilizes the official ZOO-Project Docker images and configurations.

Prerequisites
-------------

- **Docker**: Ensure Docker is installed on your system. You can download it from `Docker's official website <https://www.docker.com/get-started>`__.

- **Docker Compose**: Required for orchestrating multi-container Docker applications. Installation instructions are available at `Docker Compose Installation <https://docs.docker.com/compose/install/>`__.

Installation Steps
------------------

1. **Clone the ZOO-Project Repository**

   Open a terminal and execute:

   .. code-block:: bash

       git clone https://github.com/ZOO-Project/ZOO-Project.git
       cd ZOO-Project

2. **Start the Docker Containers**

   Launch the containers in detached mode:

   .. code-block:: bash

       docker-compose up -d

   This command builds and starts the ZOO-Project services as defined in the `docker-compose.yml` file.

Accessing the ZOO-Project Services
----------------------------------

Once the containers are running, you can access the ZOO-Project services:

- **ZOO-Kernel WPS Endpoint**:

  Access the Web Processing Service (WPS) endpoint at:

  .. code-block:: none

      http://localhost/cgi-bin/zoo_loader.cgi

- **ZOO-Project Demo Application**:

  Explore the demo application at:

  .. code-block:: none

      http://localhost/zoows-2024/

  This demo showcases various capabilities and services provided by the ZOO-Project.


Stopping the Services
---------------------

To stop the running containers, execute:

.. code-block:: bash

    docker-compose down

This command stops and removes the containers defined in the `docker-compose.yml` file.

Troubleshooting
---------------

- **Port Conflicts**: If port 80 is already in use, modify the `docker-compose.yml` file to map the container's port to an available host port.

- **Data Persistence**: To persist data across container restarts, ensure volumes are correctly mapped to host directories.

- **Logs and Debugging**: Use `docker-compose logs` to view logs from the containers for debugging purposes.

For more detailed information and advanced configurations, refer to the official ZOO-Project documentation and Docker resources.