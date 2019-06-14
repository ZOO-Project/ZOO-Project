.. kernel-what:

.. include:: <xhtml1-lat1.txt>
.. include:: <xhtml1-symbol.txt>

What is ZOO-Kernel ?
====================

ZOO-Kernel is the heart of the `ZOO-Project <http://zoo-project.org>`_ WPS platform. It is a WPS compliant implementation written in C language which provides a powerful and extensible WPS server. 

ZOO-Kernel is an extensible WPS server that makes your system more
powerful. It provides a full-featured processing engine which runs on
Linux, Mac OSX |trade| and Windows |trade| operating
systems. ZOO-Kernel is a CGI program which works on common web servers
(namely `Apache <http://httpd.apache.org/>`_ or `IIS
<http://www.iis.net/>`_ |trade|). It can be seamlessly integrated to
new or existing web platforms.

ZOO-Kernel lets you process geospatial or non geospatial data using
well formed WPS requests. The WPS server is able to manage and chain
WPS Services (see ZOO-Services for examples) by loading dynamic
libraries and source code written in different programming languages.  

First class WPS server
-----------------------

Simple
......

The ZOO-Kernel rely on simple principles and tends to ease the
implementation of new services by sharing similar data structures for
every supported programming languages. The ZOO-Kernel is responsible
to parse the requests it receives and return the corresponding WPS
response. 

In case of an *Execute* request, the ZOO-Kernel stores informations in
a basic KVP data structure for the programming language used to
implement the service, dynamically load the Service Provider defined
in the zcfg file and run a specific function corresponding to the
service, passing three arguments. Once the function return, ZOO-Kernel
knows if the service run succeessfuly or failed by checking the
returned value. In the case it succeeded, the ZOO-Kernel then parse
the third arguments containing the result and produce the output in
the desired format.



Compliant
........................................................

ZOO-Kernel implements and complies with the `WPS 1.0.0
<http://www.opengeospatial.org/standards/wps/>`_ and the `WPS 2.0.0
<http://www.opengeospatial.org/standards/wps/>`_ standards edited by
the `Open Geospatial Consortium <http://www.opengeospatial.org/>`_. It
is able to perform the WPS operations defined in the OpenGIS |reg|
specification, such as:

* **GetCapablities**: Returns service-level metadata information.It
  provides the list of available processing services.
* **DescribeProcess**: Returns a description of a process, including
  its supported input and output.
* **Execute**:  Launches computation and returns the output produced
  by a particular process.
* **GetStatus**:  only available in WPS 2.0.0, it lets the client fetch
  the ongoing status of a running service.
* **GetResult**: only available in WPS 2.0.0, it lets the client fetch
  the final result of a running service.
* **Dismiss**: only available in WPS 2.0.0, it lets the client ask
  the server to stop a running service and remove any file it created.

ZOO-Kernel compliancy and performances can be tested using the
following tools:
 * `cptesting <https://github.com/WPS-Benchmarking/cptesting>`_ 
 * WPS Test Suite provided by the `OGC compliancy program <http://cite.opengeospatial.org/>`_
 * XML responses validity can also be simply tested using `XMLint <http://xmlsoft.org/xmllint.html/>`_.

Polyglot
........................................................

ZOO-Kernel is a **polyglot**. The software is written in a valid form
of multiple programming languages, which performs the same operations
independent of the programming language used to compile or interpret
it. The supported programming languages are listed bellow:

============ =================== ========================= ============
**Language** **ServiceProvider** **DataStructure**         **Return**
------------ ------------------- ------------------------- ------------
C / C++      Shared Library      maps* M 	           integer
Java 	     Class File 	 `HashMap`_ 	           integer
C# 	     Class File 	 `ZMaps`_	           integer
Python 	     Module File 	 `Dictionary`_ 	           integer
PHP 	     Script File 	 `Array`_ 	           integer
Perl 	     Script File 	  	                   integer
Ruby 	     Script File 	 `Hash`_	           integer
Fortran      Shared Library      CHARACTER*(1024) M(10,30) integer
R	     Script file 	 `R List`_	           integer
JavaScript   Script file 	 `Object`_ or Array	   Object/Array
============ =================== ========================= ============

.. _`HashMap`: http://download.oracle.com/javase/6/docs/api/java/util/HashMap.html
.. _`dictionary`: http://docs.python.org/tutorial/datastructures.html#dictionaries
.. _`Array`: http://php.net/manual/language.types.array.php
.. _`Object`: http://www.json.org/
.. _`Hash`: http://ruby-doc.org/core-2.2.0/Hash.html
.. _`ZMaps`: https://docs.microsoft.com/fr-fr/dotnet/api/system.collections.generic.dictionary-2?view=netframework-4.8
.. _`R List`: https://cran.r-project.org/doc/manuals/r-release/R-lang.html#List-objects

