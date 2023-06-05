.. include:: <xhtml1-lat1.txt>
.. include:: <xhtml1-symbol.txt>

.. _install-onwindows:

	     
Installation on Windows |trade|
===============================

Install ZOO-Project binaries
----------------------------

.. note::
   The content of the ZOO-Project Windows-Binaries is based on
   `GISInternals SDK <http://www.gisinternals.com/release.php>`__,
   make sure to refer to license informations. 

.. note::
   When using the ZOO-Project Windows-Binaries, you can decide if you
   want the Java support activated or not (which is the case per
   default). Indeed, once your installation has been done, you will
   have both a `zoo_loader.cgi` and `zoo_loader_java.cgi` which
   correspond respectively to the ZOO-Kernel without and with Java
   support activated. So, in case you want to use the Java support,
   simply rename the `zoo_loader_jave.cgi` file located in
   `c:\\inetpub\\cgi-bin` to `zoo_loader.cgi` and make sure the
   `jvm.dll` can be found.


Using the installer
...................

Prior to run the ZOO-Project-Installer, please make sure you have IIS
and `Python <https://www.python.org/downloads/windows/>`__ setup on
your machine. Then download the `ZOO-Project-Installer
<https://bintray.com/gfenoy/ZOO-Project/Windows-Binaries/view>`__
corresponding to your platform. The first time you will run the
installer binary, you may be prompted to authorize it to run. Once the
installer has been run, simply access the following link:
http://localhost/zoo-demo/ to access your local demo application.

Install by hand
...............

Prior to run the ZOO-Project-Installer, please make sure you have IIS
and `Python <https://www.python.org/downloads/windows/>`__ setup on
your machine. Then download the `ZOO-Project
<https://bintray.com/gfenoy/ZOO-Project/Windows-Binaries/view>`__
archive corresponding to your platform. Uncompress it, then move
`cgi-bin`, `data` and `tmp` from uncompressed folder to `c:\\inetpub`,
also move `wwwroot\\zoo-demo` and `wwwroot\\tmp` to
`c:\\inetpub\\wwwroot`. To finish the installation, run the folllowing
command as administrator to allow the `zoo_loader.cgi` to run from
http://localhost/cgi-bin/zoo_loader.cgi:

   ::
   
     cd C:\Windows\System32\inetsrv
     appcmd.exe add vdirs /app.name:"Default Web Site/" /path:/cgi-bin /physicalPath:c:\inetpub\cgi-bin
     appcmd set config /section:handlers /+[name='CGI-exe1',path='*.cgi',verb='*',modules='CgiModule']
     appcmd.exe set config /section:isapiCgiRestriction /+[path='c:\inetpub\cgi-bin\zoo_loader.cgi',description='ZOO-Project',allowed='True'] 



Compile ZOO-Project from source
-------------------------------

.. warning::
   Ensure to first perform the :ref:`prerequisite steps
   <install-prereq>` before compiling the ZOO Kernel.

The following steps are for use with the Microsoft Visual Studio
compiler (and tested with MSVC 2010).

1. Make sure the gnuwin32 tools ``bison.exe``  and ``flex.exe`` are found
   in your path.  You can download the GNUwin32 tools `here
   <http://www.zoo-project.org/dl/tool-win32.zip>`__.

2. Modify the ``nmake.opt`` file to point to your local libraries. Note
   that you can also use definition directly in the command line if
   you prefer. See :ref:`win_configure_options` for details about this
   options.

   
3. Execute:

   ::
   
     nmake /f makefile.vc
     
4. A file ``zoo_loader.cgi`` and ``libzoo_service.dll`` should be
   created.  Note that if another file named
   ``zoo_loader.cgi.manifest`` is also created, you will have to run
   another command:

   ::
   
     nmake /f makefile.vc embed-manifest
     
5. Copy the files ``zoo_loader.cgi``, ``libzoo_service.dll``  and
   ``main.cfg`` into your cgi-bin directory.

6. Using the command prompt, test the ZOO-Kernel by executing the
   following command:

   ::
   
     D:\ms4w\Apache\cgi-bin> zoo_loader.cgi
     
   which should display a message such as:
   
   ::
   
     Content-Type: text/xml; charset=utf-8
     Status: 200 OK
     
     <?xml version="1.0" encoding="utf-8"?>
     <ows:ExceptionReport xmlns:ows="http://www.opengis.net/ows/1.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xlink="http://www.w3.org/1999/xlink" xsi:schemaLocation="http://www.opengis.net/ows/1.1 http://schemas.opengis.net/ows/1.1.0/owsExceptionReport.xsd" xml:lang="en-US" version="1.1.0">
       <ows:Exception exceptionCode="MissingParameterValue">
         <ows:ExceptionText>Parameter &lt;request&gt; was not specified</ows:ExceptionText>
       </ows:Exception>
     </ows:ExceptionReport>
     
7. Edit the ``main.cfg`` file so that it contains values describing
   your WPS service.  An example of such a file running on Windows is:
   
   ::
   
     [main]
     encoding = utf-8
     version = 1.0.0
     serverAddress = http://localhost/
     lang = en-CA
     tmpPath=/ms4w/tmp/ms_tmp/
     tmpUrl = /ms_tmp/
     
     [identification]
     title = The Zoo WPS Development Server
     abstract = Development version of ZooWPS. See http://www.zoo-project.org
     fees = None
     accessConstraints = none
     keywords = WPS,GIS,buffer
     
     [provider]
     providerName=Gateway Geomatics
     providerSite=http://www.gatewaygeomatics.com
     individualName=Jeff McKenna
     positionName=Director
     role=Dev
     adressDeliveryPoint=1101 Blue Rocks Road
     addressCity=Lunenburg
     addressAdministrativeArea=False
     addressPostalCode=B0J 2C0
     addressCountry=ca
     addressElectronicMailAddress=info@gatewaygeomatics.com
     phoneVoice=False
     phoneFacsimile=False
     
8. Open a web browser window, and execute a GetCapababilites request on your WPS service: http://localhost/cgi-bin/zoo_loader.cgi?request=GetCapabilities&service=WPS

   The response should be displayed in your browser, such as:
   
   ::
   
     <wps:Capabilities xsi:schemaLocation="http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsGetCapabilities_response.xsd" service="WPS" xml:lang="en-US" version="1.0.0">
     <ows:ServiceIdentification>
       <ows:Title>The Zoo WPS Development Server</ows:Title>
       <ows:Abstract>
         Development version of ZooWPS. See http://www.zoo-project.org
       </ows:Abstract>
       <ows:Keywords>
         <ows:Keyword>WPS</ows:Keyword>
         <ows:Keyword>GIS</ows:Keyword>
         <ows:Keyword>buffer</ows:Keyword>
       </ows:Keywords>
       <ows:ServiceType>WPS</ows:ServiceType>
       <ows:ServiceTypeVersion>1.0.0</ows:ServiceTypeVersion>
       ...
       

.. _win_configure_options:

Build options
.............

Various build options can be set in the ``nmake.opt`` file to define
the location of the built libraries you want to use to build your
ZOO-Kernel. Some are optional and some are required, they are listed
below exhaustively:

.. contents:: 
    :local:
    :depth: 1
    :backlinks: top


gettext (Required)
******************

The location of the libintl (built when building gettext) should be
specified by defining the ``INTL_DIR`` environment variable. It
supposes that the header and the ``intl.lib`` file are available.

So for instance, in case you build the gettext in
``\buildkit\srcs\gettext-0.14.6``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set INTL_DIR=\buildkit\srcs\gettext-0.14.6\gettext-runtime\intl


libCURL (Required)
******************

The location of the libCURL should be specified by defining
the ``CURL_DIR`` environment variable. It supposes that there are 2
sub-directory ``include`` containing the libCURL header and ``lib``
which contains the ``libcurl.lib`` file.

So for instance, in case you build the libCURL in
``\buildkit\srcs\curl-7.38.0``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set CURL_DIR=\buildkit\srcs\curl-7.38.0\builds\libcurl-vc10-x86-release-dll-ssl-dll-zlib-dll-ipvs6-sspi


libFCGI (Required)
******************

The location of the libFCGI should be specified by defining the
``FCGI_DIR`` environment variable. It supposes that there are 2
sub-directory ``include`` containing the FastCGI header and
``libfcgi/Release`` which contains the ``libfcgi.lib`` file.

So for instance, in case you build the libXML2 library in
``\buildkit\srcs\fcgi-2.4.1``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set FCGI_DIR=\buildkit\srcs\fcgi-2.41.1

libXML2 (Required)
******************

The location of the libXML2 should be specified by defining the
``XML2_DIR`` environment variable. It supposes that there are 2
sub-directory ``include`` containing the libXML2 header and
``win32\bin.msvc`` which contains the ``libxml2.lib`` file.

So for instance, in case you build the libXML2 library in
``\buildkit\srcs\libxml2-2.9.0``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set XML2_DIR=\buildkit\srcs\libxml2-2.9.0

OpenSSL (Required)
******************

The location of the OpenSSL library should be specified by defining
the ``SSL_DIR`` environment variable. It supposes that there are 2
sub-directory ``inc32`` containing the header files and
``out32dll`` which contains the ``ssleay32.lib`` file.

So for instance, in case you build the libXML2 library in
``\buildkit\srcs\openssl-1.0.2c``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set SSL_DIR=\buildkit\srcs\openssl-1.0.2c

GDAL (Required)
******************

The location of the GDAL library should be specified by defining
the ``GDAL_DIR`` environment variable. It corresponds to the path
where you uncompress and built GDAL, it supposes that you have the
``gdal_i.lib`` file available in this directory.

So for instance, in case you build the libXML2 library in
``\buildkit\srcs\gdal-1.10.1``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set GDAL_DIR=\buildkit\srcs\gdal-1.10.1

MapServer (Optional)
********************

The location of the MapServer library path should be specified by
defining the ``MS_DIR`` environment variable. It corresponds to the
path where you build MapServer on your system, this directory should
contain the ``nmake.opt`` file used. 

So for instance, in case you build Python in
``\buildkit\srcs\mapserver-6.2.0``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set MS_DIR=\buildkit\srcs\mapserver-6.2.0


Python (Optional)
*****************

The location of the Python binaries path should be specified by
defining the ``PY_DIR`` environment variable. It corresponds to the
path where you build Python on your system. The location of the
``pythonXX.lib`` files should be specified by setting the
``PY_LIBRARY`` environment variable.

So for instance, in case you build Python in
``\buildkit\srcs\Python-2.7``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set PY_DIR=\buildkit\srcs\Python-2.7
    set PY_LIBRARY=\buildkit\srcs\Python-2.7\PCBuild\python27.lib

JavaScript (Optional)
*********************

The location of libmozjs should be specified by defining the
``JS_DIR`` environment variable. It corresponds to the path where you
build libmozjs on your system, it supposes that the header and
the ``mozjs185-1.0.lib`` file are available in this directory.

So for instance, in case you build libmozjs in
``\buildkit\srcs\js-1.8.5``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set JS_DIR=\buildkit\srcs\js-1.8.5

PHP (Optional)
*****************

The location of PHP should be specified by defining the ``PHP_DIR``
environment variable. It corresponds to the path where you build PHP
on your system. The location of the ``php5embed.lib`` files should be
specified by setting the ``PHP_LIB`` environment variable.

So for instance, in case you build PHP in
``\buildkit\srcs\php-5.5.10``, you may define the following before
running ``nmake /f makefile.vc``:

.. code::

    set PHP_DIR=\buildkit\srcs\php-5.5.10
    set PHP_LIB=\buildkit\srcs\php-5.5.10\Release_TS\php5embed.lib

Database backend (Optional)
***************************

ZOO-Kernel can use a database backend to store ongoing status
informations of running services, for activating this operation mode,
you should define the evironment variable ``DB`` and set it to any
value. So, to activate this option, you may use the following before
running ``nmake /f makefile.vc``:

.. code::

    set DB=activated

.. note:: 
    To learn how to setup the corresponding database, please refer to
    :ref:`this section <zoo_create_db_backend>`.




Optionally Compile Individual Services
......................................

An example could be the *OGR base-vect-ops* provider located in the ``zoo-project\zoo-services\ogr\base-vect-ops`` directory.  

1. First edit the *makefile.vc* located in that directory, and execute:

   ::
   
     nmake /f makefile.vc
     
   Inside that same directory, the *ogr_service.zo* file should be created.
   
2. Copy all the files inside ``zoo-services\ogr\base-vect-ops\cgi-env`` into your ``cgi-bin`` directory

3. Test this service provider through the following URL:

http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=Buffer&DataInputs=BufferDistance=1@datatype=interger;InputPolygon=Reference@xlink:href=http%3A%2F%2Fwww.zoo-project.org%3A8082%2Fgeoserver%2Fows%3FSERVICE%3DWFS%26REQUEST%3DGetFeature%26VERSION%3D1.0.0%26typename%3Dtopp%3Astates%26SRS%3DEPSG%3A4326%26FeatureID%3Dstates.15
   
   The response displayed in your browser should contain:
   
   ::
   
     <wps:ProcessSucceeded>Service "Buffer" run successfully.</wps:ProcessSucceeded>

