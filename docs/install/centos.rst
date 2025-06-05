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

Web Server Configuration
------------------------

Ensure that your web server (e.g., Apache) is configured to allow CGI scripts in `/usr/lib/cgi-bin`. You may need to enable the `cgi` module and set the appropriate permissions.

Testing the Installation
------------------------

After completing the installation, you can test the ZOO-Kernel by sending requests to the WPS server. For example:

- GetCapabilities:

  .. code-block:: none

     http://127.0.0.1/cgi-bin/zoo_loader.cgi?Service=WPS&Request=GetCapabilities&Version=1.0.0

- DescribeProcess:

  .. code-block:: none

     http://127.0.0.1/cgi-bin/zoo_loader.cgi?Service=WPS&Request=DescribeProcess&Version=1.0.0&Identifier=HelloPy

- Execute:

  .. code-block:: none

     http://127.0.0.1/cgi-bin/zoo_loader.cgi?Service=WPS&Request=Execute&Version=1.0.0&Identifier=HelloPy&DataInputs=a=myname

.. note::

    These requests should return well-formed XML documents (OWS responses).

.. warning::

     The URLs provided assume that you have set up a web server and defined `cgi-bin` as a location where you can run CGI applications.
     If ZOO-Kernel returns an error, please check the :ref:`kernel_config` and ensure all prerequisites are met.
