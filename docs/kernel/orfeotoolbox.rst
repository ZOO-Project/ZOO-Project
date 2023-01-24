.. _kernel-orfeotoolbox:
    
Optional Orfeo Toolbox support
==============================

`Orfeo Toolbox <http://orfeo-toolbox.org/otb/>`_ provides simple to advanced algorithms for processing imagery available from remote sensors.
The optional Orfeo Toolbox support is available since `ZOO-Project 1.5 <http://zoo-project.org>`__. It allows to execute the `OTB Applications <http://orfeo-toolbox.org/otb/otb-applications.html>`_ directly as ZOO WPS Services thanks to a :ref:`kernel_index` specific internal mechanism which is detailed in this section.

.. note:: 

   |otb| `Orfeo Toolbox <https://www.orfeo-toolbox.org>`__ is an open source image processing library. Learn more by reading its `documentation <https://www.orfeo-toolbox.org/documentation/>`__.
 

.. |otb| image:: ../_static/orfeotoolbox.png
       :height: 115px
       :width: 150px
       :scale: 30%
       :alt: Orfeo Toolbox logo


Installation and configuration
------------------------------

Follow the step described bellow in order to activate the ZOO-Project optional Orfeo Toolbox support.

Prerequisites
.............

   * latest `ZOO-Kernel <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel>`_ trunk version
   * Orfeo Toolbox (`OTB 4.2.1 <http://orfeo-toolbox.org/otb/>`_)
   * Insight Segmentation and Registration Toolkit  (`ITK-4.7 <http://itk.org/ITK/resources/software.html/>`_)

Installation steps
..................

.. Note:: These installation steps were successfully tested on Ubuntu 14.4 LTS 

.. Note:: For OTB and ITK, the CMAKE_C_FLAGS and CMAKE_CXX_FLAGS must first be set to ``-fPIC``

Download lastest ZOO-Kernel code from SVN.

.. code-block:: guess

    svn checkout http://svn.zoo-project.org/svn/trunk/zoo-kernel zoo-kernel

Then compile ZOO-Kernel using the needed configuration options as shown bellow:

.. code-block:: guess

     cd zoo-kernel
     autoconf
     ./configure  --with-otb=/usr/local --with-itk=/usr/local --with-itk-version=4.7 
     make
     cp zoo_loader.cgi /usr/lib/cgi-bin

Configuration steps
...................

.. _kernel-orfeotoolbox-main.cfg:
    
Main configuration file
***********************

Add the following content to your ``/usr/lib/cgi-bin/main.cfg`` file 
in the ``[env]`` section:

.. code-block:: guess

       ITK_AUTOLOAD_PATH=/usr/local/lib/otb/applications

Services configuration file
***************************

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

Test requests
*************

Once done, OTB Applications should be listed as available WPS Services when runing a GetCapabilities request

.. code-block:: 

        http://localhost/cgi-bin/zoo_loader.cgi?request=GetCapabilities&service=WPS 

Each OTB Service can then be described individually using the DescribeProcess request, as for example:

.. code-block::

   http://localhost/cgi-bin/zoo_loader.cgi?request=DescribeProcess&service=WPS&version=1.0.0&Identifier=OTB.BandMath

Here is an example request executing the *OTB.BandMath* Application with the `OTB Cookbook <https://www.orfeo-toolbox.org/CookBook/CookBook.html>`_ sample data as input

.. code-block::

   http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=OTB.BandMath&DataInputs=il=Reference@xlink:href=http://hg.orfeo-toolbox.org/OTB-Data/raw-file/ca154074b282/Examples/verySmallFSATSW.tif;il=Reference@xlink:href=http://hg.orfeo-toolbox.org/OTB-Data/raw-file/ca154074b282/Examples/verySmallFSATSW_nir.tif;out=float;exp=im1b3*cos%28im1b1%29,im1b2*cos%28im1b1%29,im1b1*cos%28im1b1%29&RawDataOutput=out@mimeType=image/png


.. note::

   The usual ZOO GetStatus requests also work when using the OTB Applications as WPS Services.



    






