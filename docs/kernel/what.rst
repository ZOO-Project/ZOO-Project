.. kernel-what:

.. include:: <xhtml1-lat1.txt>
.. include:: <xhtml1-symbol.txt>

What is ZOO-Kernel ?
====================

**ZOO-Kernel** is the core of the `ZOO-Project <http://zoo-project.org>`_ WPS platform. It is a WPS-compliant implementation written in C, providing a robust and extensible WPS (Web Processing Service) server.

ZOO-Kernel enhances your system's capabilities by offering a full-featured processing engine that operates across Linux, macOS |trade|, and Windows |trade|. It functions as a CGI program compatible with standard web servers like `Apache <http://httpd.apache.org/>`_ and `IIS <http://www.iis.net/>`_ |trade|, and can be seamlessly integrated into both new and existing web platforms.

ZOO-Kernel allows the processing of geospatial and non-geospatial data using well-formed WPS requests. It supports chaining and execution of WPS services (see ZOO-Services for examples) using dynamic libraries and source code in various programming languages.  

First class WPS server
-----------------------

Simple
......

ZOO-Kernel is based on straightforward principles and aims to simplify the creation of new services. It does this by using a unified data structure across all supported programming languages.

ZOO-Kernel handles incoming WPS requests by parsing them and producing appropriate WPS responses. For *Execute* requests, it:
  1. Stores request information in a basic key-value pair (KVP) structure.
  2. Dynamically loads the appropriate Service Provider defined in the `.zcfg` file.
  3. Calls the corresponding function of the service, passing **three arguments**.

After the function executes, ZOO-Kernel checks the return value to determine if the service ran successfully. If successful, it parses the third argument (containing the result) and generates the response in the required format.

Compliant
.........

ZOO-Kernel complies with both the `WPS 1.0.0 <http://www.opengeospatial.org/standards/wps/>`_ and `WPS 2.0.0 <http://www.opengeospatial.org/standards/wps/>`_ standards from the `Open Geospatial Consortium <http://www.opengeospatial.org/>`_.

It supports the following standard WPS operations:

* **GetCapablities** - Returns service-level metadata information.It
  provides the list of available processing services.
* **DescribeProcess** - Provides a detailed description of a process, including input/output definitions.
* **Execute** -  Runs a process and returns its output.
* **GetStatus** (WPS 2.0.0 only) - It lets the client fetch
  the ongoing status of a running service.
* **GetResult** (WPS 2.0.0 only) -It lets the client fetch
  the final result of a running service.
* **Dismiss** (WPS 2.0.0 only) - It lets the client ask
  the server to stop a running service and remove any file it created.

ZOO-Kernel's compliance and performances can be tested using the
following tools:

* `cptesting <https://github.com/WPS-Benchmarking/cptesting>`_ 
* WPS Test Suite provided by the `OGC compliancy program <http://cite.opengeospatial.org/>`_
* XML responses validity can also be simply tested using `XMLint <http://xmlsoft.org/xmllint.html/>`_.

Polyglot
........

ZOO-Kernel is **polyglot**, meaning it supports service implementation in multiple programming languages. 
Developers can write services in the language they are most comfortable with. Below is the list of supported languages, service types, data structures, and return conventions:


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
Node.js      Script file 	  `Object`_	           Integer
============ =================== ========================= ============

.. _`HashMap`: http://download.oracle.com/javase/6/docs/api/java/util/HashMap.html
.. _`dictionary`: http://docs.python.org/tutorial/datastructures.html#dictionaries
.. _`Array`: http://php.net/manual/language.types.array.php
.. _`Object`: http://www.json.org/
.. _`Hash`: http://ruby-doc.org/core-2.2.0/Hash.html
.. _`ZMaps`: https://docs.microsoft.com/fr-fr/dotnet/api/system.collections.generic.dictionary-2?view=netframework-4.8
.. _`R List`: https://cran.r-project.org/doc/manuals/r-release/R-lang.html#List-objects

