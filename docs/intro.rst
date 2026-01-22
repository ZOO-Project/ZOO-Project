Introduction
============

Welcome to the official documentation of the `ZOO-Project <http://zoo-project.org>`_, a powerful open source platform for creating and executing **OGC-compliant Web Processing Services (WPS)**.

.. note::

   This documentation is currently under review. Some content may be outdated, including support for certain platforms like Windows which is no longer maintained. Pages with obsolete material will be marked accordingly.


What is ZOO-Project ?
---------------------

`ZOO-Project <http://zoo-project.org>`_ is an open source implementation of the **OGC WPS standards**:  

- `WPS 1.0.0 <http://www.opengeospatial.org/standards/wps>`_  
- `WPS 2.0.0 <http://www.opengeospatial.org/standards/wps>`_  
- `OGC API - Processes - Part 1: Core <https://docs.ogc.org/is/18-062r2/18-062r2.html>`_

It provides developers with tools to **wrap existing algorithms and libraries into WPS-compliant services**, allowing them to be called over the web. These services can process **geospatial and non-geospatial data** in a flexible and scalable manner.


Key Features:
-------------

- Create services using **C, Python, JavaScript, and more** (up to 7 languages)
- Integrate with major geospatial libraries: GDAL, GRASS GIS, OTB, CGAL, etc.
- Process local or remote data
- Build complex workflows by **chaining services**
- Deploy services via a standards-based web API


Why Use ZOO-Project?
--------------------
- It's modular, extensible, and highly interoperable
- Easily integrates with **web mapping clients** and map servers
- Lets you reuse your code with minimal changes and serve it as a web service


ZOO-Project Components
----------------------

The platform consists of the following main components:

* :ref:`kernel_index` — The WPS-compliant core engine written in C. Manages service execution and chaining.
* :ref:`services_index`: — A growing suite of ready-to-use services built on robust open-source libraries (GDAL, GRASS, etc.).
* :ref:`api-index`: — A JavaScript server-side API to manage workflows and service orchestration.
* :ref:`client-index`: — A JavaScript client-side library for sending WPS requests from web apps.


Open Source and Community
-------------------------

ZOO-Project is released under the `MIT/X-11 license <http://opensource.org/licenses/MITlicense>`__ and maintained by a vibrant international community called the `ZOO-Tribe <http://zoo-project.org/about/tribe/>`__.

It is an official project of the `Open Source Geospatial Foundation (OSGeo) <http://osgeo.org>`__, reflecting its commitment to open standards and collaboration.

.. image:: https://raw.githubusercontent.com/OSGeo/osgeo/master/incubation/project/OSGeo_project.svg
   :height: 92px
   :width: 225px
   :alt: OSGeo incubation

How to Contribute
------------------

Want to get involved? You can contribute to:

- Source code
- Documentation
- Translations

Check out the :ref:`contribute_index` page to learn more. Everyone is welcome!

---