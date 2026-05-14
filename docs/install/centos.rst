.. _install-centos:

Installation on CentOS
======================

Use the following instructions to install `ZOO-Project <http://zoo-project.org>`__ on CentOS distributions. 

Prerequisites
-------------

First, enable the EPEL repository and install the necessary dependencies using `yum`:

.. code-block:: bash

  sudo yum install epel-release
   sudo yum groupinstall "Development Tools"
   sudo yum install gcc-c++ zlib-devel libxml2-devel bison openssl \
       python3-devel subversion libxslt-devel libcurl-devel \
       gdal-devel proj-devel libuuid-devel openssl-devel fcgi-devel \
       java-11-openjdk-devel

.. note::

   - The ELGIS repository is deprecated and no longer maintained.
   - Ensure that `python3-devel` is installed, as Python 2 has reached end-of-life.

Installation
----------------------

Refer to general instructions from :ref:`install-installation` to
setup your ZOO-Kernel and the ZOO-Services of your choice.

.. note:: 
   In case you use the Java support, please, make sure to use the
   correct version of both java and javac using the following
   commands:
   
   .. code-block:: bash
   
     sudo alternatives --config java
     sudo alternatives --config javac
   
   Also, make sure to add the following to your `main.cfg` file before
   trying to execute any Java service:

   .. code-block:: bash
   
     [javax]
     ss=2m

