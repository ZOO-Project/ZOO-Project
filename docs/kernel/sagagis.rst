.. _kernel-sagagis:
    
Optional SAGA GIS support
======================

`SAGA GIS <http://www.saga-gis.org/>`_ provides a comprehensive set of geoscientific methods and spatial algorithms. The optional SAGA GIS support is available since `ZOO-Project 1.5 <http://zoo-project.org>`__. It allows to execute the `SAGA Modules <http://www.saga-gis.org/saga_module_doc/2.1.4/index.html>`_ directly as ZOO WPS Services thanks to a :ref:`kernel_index` specific internal mechanism which is detailed in this section.

.. note:: 

   |saga| `SAGA GIS <http://www.saga-gis.org/>`__ is the System for Automated Geoscientific Analyses. Learn more on official `website <http://www.saga-gis.org/en/index.html>`__.
 

.. |saga| image:: ../_static/sagagis.png
       :height: 100px
       :width: 100px
       :scale: 45%
       :alt: SAGA GIS logo


Installation and configuration
------------------------------

Follow the step described bellow in order to activate the ZOO-Project optional SAGA GIS support.

Prerequisites
.....................

   * latest `ZOO-Kernel <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel>`_ trunk version
   * `SAGA GIS  <http://saga-gis.org>`_  (7.2.0)

Installation steps
...........................

Compile ZOO-Kernel using the configuration options as shown bellow:

.. code-block:: guess

     cd zoo-kernel
     autoconf
     ./configure  --with-saga=/usr/local/ --with-saga-version=7
     make

And copy the newly created zoo_loader.cgi to ``/usr/lib/cgi-bin``.
     
.. note::
   
    The ``--with-saga-version`` option let you set the major
    version number of SAGA-GIS.  
     
.. code-block:: guess

     cp zoo_loader.cgi /usr/lib/cgi-bin

Configuration steps
...............................

Services configuration file
****************************

Building the
`saga2zcfg <http://zoo-project.org/trac/browser/trunk/thirds/saga2zcfg>`_
utility is required to activate the available SAGA-GIS Modules as WPS
Services. This can be done using the following command: 

.. code-block:: guess

    cd thirds/saga2zcfg
    make

The following commands will then generate all the needed zcfg files for the available SAGA-GIS Modules:

.. code-block:: guess
		
    mkdir zcfgs
    cd zcfgs
    ../saga2zcfg
    mkdir /location/to/your/cgi-bin/SAGA
    cp *zcfg /location/to/your/cgi-bin/SAGA


Test requests
*****************

The SAGA-GIS Modules should be listed as available WPS Services when
runing a GetCapabilities request, as follow:

http://localhost/cgi-bin/zoo_loader.cgi?request=GetCapabilities&service=WPS

Each SAGA-GIS Service can then be described individually using the DescribeProcess request, as for example:

http://localhost/cgi-bin/zoo_loader.cgi?request=DescribeProcess&service=WPS&version=1.0.0&Identifier=SAGA.garden_fractals.1

And executed according to your needs. The following example executes *SAGA.garden_fractals.1* with no optional parameter:

http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=SAGA.garden_fractals.1&DataInputs=&ResponseDocument=RESULT@mimeType=application/json@asReference=true

.. note::
   
  The common ZOO GetStatus requests also work when using the SAGA-GIS Modules as WPS Services.




