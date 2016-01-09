.. _install-centos:

Installation on CentOS
======================

Use the following instructions to install `ZOO-Project <http://zoo-project.org>`__ on CentOS distributions. 

Prerequisites
----------------------

First you should add the `ELGIS Repository <http://elgis.argeo.org>`__ then install the
dependencies by using `yum` commands.

.. code-block:: guess

  rpm -Uvh http://elgis.argeo.org/repos/6/elgis-release-6-6_0.noarch.rpm
  rpm -Uvh \
    http://download.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm
  wget\
    http://proj.badc.rl.ac.uk/cedaservices/raw-attachment/ticket/670/armadillo-3.800.2-1.el6.x86_64.rpm
  yum install armadillo-3.800.2-1.el6.x86_64.rpm
  yum install hdf5.so.6
  yum install gcc-c++ zlib-devel libxml2-devel bison openssl \
    python-devel subversion libxslt-devel libcurl-devel \
    gdal-devel proj-devel libuuid-devel openssl-devel fcgi-devel
  yum install java-1.7.0-openjdk-devel


Installation
----------------------

Now refer to general instructions from :ref:`install-installation` to
setup your ZOO-Kernel and the ZOO-Services of your choice.

.. note:: 
   In case you use the Java support, please, make sure to use the
   correct version of both java and javac using the following
   commands:
   
   .. code-block:: guess
   
     update-alternatives --config java
     update-alternatives --config javac
   
   Also, make sure to add the following to your `main.cfg` file before
   trying to execute any Java service:

   .. code-block:: guess
   
     [javax]
     ss=2m

