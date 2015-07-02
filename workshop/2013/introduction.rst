.. _introduction:

Introduction
=================

.. contents:: Table of Contents
    :depth: 5
    :backlinks: top

What is ZOO ?
---------------------

ZOO-Project is a WPS (Web Processing Service) open source project released under a `MIT/X-11 <http://zoo-project.org/trac/wiki/Licence>`__ style license. It provides an OGC WPS compliant developer-friendly framework to create and chain WPS Web services. ZOO is made of three parts:

* `ZOO Kernel <http://zoo-project.org/docs/kernel/index.html#kernel>`__ : A powerful server-side C Kernel which makes it possible to manage and chain Web services coded in different programming languages.
* `ZOO Services <http://zoo-project.org/docs/services/index.html#services>`__ : A growing suite of example Web Services based on various open source libraries.
* `ZOO API <http://zoo-project.org/docs/api/index.html#api>`__ : A server-side JavaScript API able to call and chain the ZOO Services, which makes the development and chaining processes easier and faster.

ZOO was designed to make the service creation and deployment easy, by providing a powerful system able to understand and execute WPS compliant queries. It supports seven programming languages, thus allowing you to create Web Services using the one you prefer. It also lets you use an existing code and to turn it as a WPS Service.
The current supported programming languages are the following:

* C/C++
* Python
* Perl
* Java
* Fortran
* PHP
* JavaScript

More information on the project is available on the  `ZOO Project official website <http://www.zoo-project.org/>`__ .

How does ZOO works ?
---------------------

ZOO is based on a C Kernel which is the ZOO-Project core system (aka ZOO-Kernel). The latter is able to dynamically load libraries and to handle them as on-demand Web services. 

A ZOO-Service is a link composed of a ZOO metadata file (.zcfg) and the code for the corresponding implementation. The metadata file describes all the available functions that can be called using a WPS Execute Request, as well as the desired input/output. Services contain the algorithms and functions, and can be implemented using any of the supported languages.

ZOO-Kernel works as CGI through Apache and can communicate with cartographic engines and Web mapping clients. It simply adds the WPS support to your spatial data infrastructure and your webmapping applications. It can use every GDAL/OGR supported formats as input data and create suitable vector or raster output for your cartographic engine and/or your web-mapping client application. 

What are we going to do in this workshop?
-------------------------------------

You will learn how to use ZOO Kernel and how to create ZOO Services using the OSGeo Live 7.0. Despite a pre-compiled ZOO 1.3-dev package is provided inside OSGeoLive, some optional supports are not available in the default setup. So you will be invited to download and uncompress a tarbal containing binary versions of ZOO-Kernel, so there is no need to compile anything, simple steps to install ZOO-Kernel will be introduced. Configuration file and basic ways to run ZOO Kernel and ZOO Service will be presented. Then you will be invited to start programming your own simple service using Python language. Some ZOO Services will be presented and individually tested inside a ready-to-use OpenLayers application. Finally, this services will be chained using the server-side Javascript ZOO API.

The whole workshop is organized step-by-step and numerous code snippets are available. The instructors will check the ZOO-Kernel is functioning on each machine and will assist you while coding. Technical questions are of course welcome during the workshop.

Usefull tips for reading
---------------------

.. code-block:: guess

    this is a code block

.. warning:: This is a warning message.

.. note:: This is an important note.

**Let's go !**
