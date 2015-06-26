.. _introduction:

Introduction
############

.. contents:: Table of Contents
    :depth: 5
    :backlinks: top

What is ZOO ?
*************

ZOO is a WPS (Web Processing Service) open source project recently released under a `MIT/X-11 <http://zoo-project.org/trac/wiki/Licence>`__ style license. It provides an OGC WPS compliant developer-friendly framework to create and chain WPS Web services. ZOO is made of three parts:

  - `ZOO Kernel <http://zoo-project.org/trac/wiki/ZooWebSite/ZooKernel/Introduction>`__ : A powerful server-side C Kernel which makes it possible to manage and chain Web services coded in different programming languages.
  - `ZOO Services <http://zoo-project.org/trac/wiki/ZooWebSite/ZooServices/Introduction>`__ : A growing suite of example Web Services based on various open source libraries.
  - `ZOO API <http://zoo-project.org/trac/wiki/ZooWebSite/ZOOAPI/Introduction>`__ : A server-side JavaScript API able to call and chain the ZOO Services, which makes the development and chaining processes easier.

ZOO is designed to make the service development easier by providing a powerful system 
able to understand and execute WPS compliant queries. It supports several programming 
languages, thus allowing you to create Web Services in your favorite one and from an 
already existing code. Further information on the project is available on the  
`ZOO Project official website <http://www.zoo-project.org/>`__ .

How does ZOO works ?
********************

ZOO is based on a 'WPS Service Kernel' which constitutes the ZOO's core system 
(aka ZOO Kernel). The latter is able to load dynamic libraries and to handle them 
as on-demand Web services. The ZOO Kernel is written in C language, but supports 
several common programming languages for creating ZOO Services.

A ZOO Service is a link composed of a ZOO metadata file (.zcfg) and the code for 
the corresponding implementation. The metadata file describes all the available 
functions which can be called using a WPS Exec Request, as well as the desired 
input/output. Services contain the algorithms and functions, and can now be 
implemented in C/C++, Fortran, Java, Python, Perl, PHP and JavaScript.

ZOO Kernel works with Apache and can communicate with cartographic engines and 
Web mapping clients. It simply adds the WPS support to your spatial data infrastructure 
and your Web mapping application. It can use every GDAL/OGR supported formats as input 
data and create suitable vector or raster output for your cartographic engine and/or 
your web-mapping client application.

What are we going to do in this workshop?
*****************************************

This workshop aims to present the ZOO Project and its features, and to explain its 
capabilities regarding the  `OGC WPS 1.0.0 specification <http://www.opengeospatial.org/standards/wps>`__. 
The participants will learn in 3 hours how to use ZOO Kernel, how to create 
ZOO Services and their configuration files and finally how to link the created 
Service with a client-side webmapping application. A pre-compiled ZOO 1.0 version 
is provided inside OSGeoLive, the OSGeo official Live DVD. For the sack of simplicity, 
an OSGeoLive Virtual Machine image disk is already installed on your computers. 
This will be used during this workshop, so the participants won't have to compile 
and install ZOO Kernel manually. Running and testing ZOO Kernel from this OSGeoLive 
image disk is thus the first step of the workshop, and every participants should 
get a working ZOO Kernel in less than 30 minutes.

Once ZOO Kernel will be tested from a Web browser using GetCapabilities requests, 
participants will be invited to create an OGR based ZOO Service Provider aiming to 
enable simple spatial operations on vector data. Participants will first have to 
choose whether they will create the service using C or Python language. Every programming 
step of the ZOO Service Provider and the related Services will be each time detailed in 
C and Python. Once the ZOO Services will be ready and callable by ZOO Kernel, participants 
will finally learn how to use its different functions from an  OpenLayers simple application. 
A sample dataset was providen by Orkney and included in the OSGeoLiveDVD, data are 
available trough OGC WMS/WFS WebServices using  MapServer and will be displayed on a 
simple map and used as input data by the ZOO Services. Then, some specific selection 
and execution controls will be added in the JavaScript code in order to execute single 
and multiple geometries on the displayed polygons.

Once again, the whole procedure will be organized step-by-step and detailed with 
numerous code snippets and their respective explanations. The instructors will check 
the ZOO Kernel functioning on each machine and will assist you while coding. Technical 
questions are of course welcome during the workshop.

Usefull tips for reading :
**************************

.. code-block:: guess

    this is a code block

.. warning:: This is a warning message.

.. note:: This is an important note.




**Let's go !**
