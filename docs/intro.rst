Introduction
============

This is an introduction to  the `ZOO-Project
<http://zoo-project.org>`_ open source software documentation. 


What is ZOO-Project ?
---------------------

`ZOO-Project <http://zoo-project.org>`__  is a WPS (Web Processing Service) implementation written in C, Python and JavaScript. It is an open source platform which implements the `WPS 1.0.0 <http://www.opengeospatial.org/standards/wps/>`_ and  `WPS 2.0.0 <http://www.opengeospatial.org/standards/wps/>`_ standards edited by the `Open Geospatial Consortium <http://www.opengeospatial.org/>`__ (OGC).

`ZOO-Project <http://zoo-project.org>`__ provides a developer-friendly framework for creating and chaining WPS compliant Web Services. Its main goal is to provide generic and standard-compliant methods for using existing open source librairies and algorithms as WPS. It also offers efficient tools for creating new innovative web services and applications.

`ZOO-Project <http://zoo-project.org>`_ is able to process geospatial or non geospatial data online. Its core processing engine (aka :ref:`kernel_index`) lets you execute a number of existing :ref:`ZOO-Services <services-available>` based on reliable software and libraries. It also gives you the ability to create your own WPS Services from new or existing source code, which can be written in seven different programming languages. That lets you compose or turn code as WPS Services simply, with straightforward configuration and standard coding methods.

`ZOO-Project <http://zoo-project.org>`_ is very flexible with data input and output so you can process almost any kind of data stored locally or accessed from remote servers and databases. ZOO-Project excels in data processing and integrates new or existing spatial data infrastructures, as it is able to communicate with map servers and can integrate webmapping clients.


ZOO-Project components
----------------------

The `ZOO-Project <http://zoo-project.org>`__ platform is made up of the following components:

* :ref:`kernel_index`: A  WPS compliant implementation written in C offering a powerful WPS server able to manage and chain WPS services. by loading dynamic libraries and code written in different languages.

* :ref:`services_index`: A growing collection of ready to use Web Processing Services built on top of reliable open source libraries such as GDAL, GRASS GIS, OrfeoToolbox, CGAL and SAGA GIS. 
* :ref:`api-index`: A server-side JavaScript API for creating, chaining and orchestrating the available WPS Services.

* :ref:`client-index`: A client side JavaScript API for interacting with WPS servers and executing standard requests from web applications.
  

Open Source
-----------

`ZOO-Project <http://zoo-project.org>`__  is open source and released under the terms of the `MIT/X-11 <http://opensource.org/licenses/MITlicense>`__  `license <https://github.com/ZOO-Project/ZOO-Project/blob/main/zoo-project/LICENSE>`__ . ZOO-Project activities are directed by the Project Steering Committee (PSC) and the software itself is being developed, maintained and documented by an international community of users and developers (aka `ZOO-Tribe <http://zoo-project.org/new/ZOO-Project/ZOO%20Tribe>`_). ZOO-Project API and services used external software tools that are available under their respective OSI (https://opensource.org/licenses/) certified open source licenses.

Please refer to the ZOO-Project :ref:`contribute_index` if you want to participate and contribute. It is easy to :ref:`get involved <contribute_index>`  on source code, documentation or translation. Everybody is welcome to join the `ZOO-Tribe <http://zoo-project.org/new/ZOO-Project/ZOO%20Tribe/>`__.

`ZOO-Project <http://zoo-project.org>`__  is an incubating software at the Open Source Geospatial Foundation (`OSGeo <http://osgeo.org>`__).

.. image:: _static/OSGeo_incubation.png
   :height: 92px
   :width: 225px
   :scale: 75 %
   :alt: OSGeo incubation

