.. _kernel-sagagis:
    
Optional SAGA GIS support
======================

`SAGA GIS <http://orfeo-toolbox.org/otb/>`_ provides a comprehensive set of geoscientific methods and spatial algorithms. The optional SAGA GIS support is available since `ZOO-Project 1.5 <http://zoo-project.org>`__. It allows to execute the `SAGA Modules <http://www.saga-gis.org/saga_module_doc/2.1.4/index.html>`_ directly as ZOO WPS Services thanks to a :ref:`kernel_index` specific internal mechanism which is detailed in this section.

.. note:: 

   |saga| `SAGA GIS <https://www.orfeo-toolbox.org>`__ is the System for Automated Geoscientific Analyses. Learn more on official `website <http://www.saga-gis.org/en/index.html>`__.
 

.. |saga| image:: ../_static/sagagis.png
       :height: 100px
       :width: 100px
       :scale: 45%
       :alt: SAGA GIS logo


How does it work ?
-------------------------



Installation and configuration
------------------------------

Follow the step described bellow in order to activate the ZOO-Project optional SAGA GIS support.

Prerequisites
.............

   * latest `ZOO-Kernel <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel>`_ trunk version
   * SAGA GIS (`SAGA-GIS 2.1.4  <http://saga-gis.org>`_ )
   * libLAS-1.2 (`LibLAS-1.2  <https://github.com/libLAS/libLAS-1.2>`_ )

Installation steps
...........................

.. Note:: These installation steps were successfully tested on Ubuntu 14.4 LTS 

Download lastest ZOO-Kernel code from SVN.

.. code-block:: guess

    svn checkout http://svn.zoo-project.org/svn/trunk/zoo-kernel zoo-kernel

Then compile ZOO-Kernel using the needed configuration options as shown bellow:

.. code-block:: guess

     cd zoo-kernel
     autoconf
     ./configure  --with-saga=/usr/local/
     make

And copy the newly created zoo_loader.cgi to ``/usr/lib/cgi-bin`` :

.. code-block:: guess

     cp zoo_loader.cgi /usr/lib/cgi-bin

Configuration steps
*************************

Main configuration file
^^^^^^^^^^^^^^^^^^^^^^^

Add the following content to your ``/usr/lib/cgi-bin/main.cfg`` file 
in the ``[env]`` section:

.. code-block:: guess

       ITK_AUTOLOAD_PATH=/usr/local/lib/otb/applications

Services configuration file
^^^^^^^^^^^^^^^^^^^^^^^^^^

The build of the `otb2zcfg  <http://zoo-project.org/trac/browser/trunk/thirds/otb2zcfg>`_ utility is required to activate the available OTB Applications as WPS services. This can be done using the following command: 

.. code-block:: guess
	
	 mkdir build
	 cd build
	 ccmake ..
	 make
	
Run the following command to generate all the needed zcfg files for the available OTB Application:

.. code-block:: guess
	
	 mkdir zcfgs
	 cd zcfgs
	 export ITK_AUTOLOAD_PATH=/your/path/to/otb/applications
	 ../build/otb2zcfg
         mkdir /location/to/your/cgi-bin/OTB
	 cp *zcfg /location/to/your/cgi-bin/OTB
	
.. warning 

     The ITK_AUTOLOAD_PATH environment variable is required in the [env] section of your main.cfg.

Test the ZOO SAGA support
^^^^^^^^^^^^^^^^^^^^^^^

Once done, OTB Applications should be listed as available WPS Services when runing a GetCapabilities request

.. code-block:: guess 

        http://localhost/cgi-bin/zoo_loader.cgi?request=GetCapabilities&service=WPS 

Each OTB Service can then be described individually using the DescribeProcess request, as for example:

.. code-block:: guess

   http://localhost/cgi-bin/zoo_loader.cgi?request=DescribeProcess&service=WPS&version=1.0.0&Identifier=OTB.BandMath

And executed according to your needs, as for the following example executing OTB.BandMath with the OTB sample data as input

.. code-block:: guess

   http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=OTB.BandMath&DataInputs=il=Reference@xlink:href=http://hg.orfeo-toolbox.org/OTB-Data/raw-file/ca154074b282/Examples/verySmallFSATSW.tif;il=Reference@xlink:href=http://hg.orfeo-toolbox.org/OTB-Data/raw-file/ca154074b282/Examples/verySmallFSATSW_nir.tif;out=float;exp=im1b3*cos%28im1b1%29,im1b2*cos%28im1b1%29,im1b1*cos%28im1b1%29&RawDataOutput=out@mimeType=image/png

When executing OTB applications as WPS Services, it is also possible to check the OTB process status, using the usual ZOO GetStatus request.



    






