.. kernel-what:

.. include:: <xhtml1-lat1.txt>
.. include:: <xhtml1-symbol.txt>

What is ZOO-Kernel ?
====================

ZOO-Kernel is the heart of the `ZOO-Project <http://zoo-project.org>`_ WPS platform. It is a WPS compliant implementation written in C language which provides a powerful and extensible WPS server. 

ZOO-Kernel implements and complies with the `WPS 1.0.0 <http://www.opengeospatial.org/standards/wps/>`_ standard edited by the `Open Geospatial Consortium <http://www.opengeospatial.org/>`_. It is able to perform the WPS operations as indicated in the OpenGIS |reg| specification, as listed bellow:

* **GetCapablities**: Returns service-level metadata information.It provides the list of available processing services.
* **DescribeProcess**: Returns a description of a process, including its supported input and output.
* **Execute**:  Launches computation and returns the output produced by a particular process.


First class WPS server
-----------------------

ZOO-Kernel is an extensible WPS server that makes your system more powerful. It provides a full-featured processing engine which runs on Linux, Mac OSX |trade| and Windows |trade| operating systems. ZOO-Kernel works on common web servers (namely `Apache <http://httpd.apache.org/>`_ or `IIS <http://www.iis.net/>`_ |trade|) and can be seamlessly integrated to new or existing web platforms. 

ZOO-Kernel lets you process geospatial or non geospatial data using well formed WPS requests. The WPS server is able to manage and chain WPS Services (see ZOO-Services for examples) by loading dynamic libraries and source code written in different programming languages.  

Supported programming languages
...............................

ZOO-Kernel is a **polyglot**. The software is written in a valid form of multiple programming languages, which performs the same operations independent of the programming language used to compile or interpret it.


============ =================== ========================= ============
**Language** **ServiceProvider** **DataStructure**         **Return**
------------ ------------------- ------------------------- ------------
C / C++      Shared Library      maps* M 	           integer
Java 	     Class File 	 `HashMap`_ 	           integer
Python 	     Module File 	 `Dictionary`_ 	           integer
PHP 	     Script File 	 `Array`_ 	           integer
Perl 	     Script File 	  	                   integer
Ruby 	     Script File 	 `Hash`_	           integer
Fortran      Shared Library      CHARACTER*(1024) M(10,30) integer
JavaScript   Script file 	 `Object`_ or Array	   Object/Array
============ =================== ========================= ============

.. _`HashMap`: http://download.oracle.com/javase/6/docs/api/java/util/HashMap.html
.. _`dictionary`: http://docs.python.org/tutorial/datastructures.html#dictionaries
.. _`Array`: http://php.net/manual/language.types.array.php
.. _`Object`: http://www.json.org/
.. _`Hash`: http://ruby-doc.org/core-2.2.0/Hash.html





