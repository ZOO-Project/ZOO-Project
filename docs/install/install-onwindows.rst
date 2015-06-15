.. include:: <xhtml1-lat1.txt>
.. include:: <xhtml1-symbol.txt>

.. _install-onwindows:

	     
Installation on Windows |trade|
====================

Using OSGeo4W
--------------

Install OSGeo4W
..........................................

First download the OSGeo4W installer from  http://trac.osgeo.org/osgeo4w/, and install it with all the dependencies needed by your 
WPS services such as GDAL for example.

.. warning::
    FastCGI, libxml, Python and cURL are mandatory

Install ZOO4W
..........................................

Once OSGeo4W installed on your platform, you will need more GNU tools and libraries. `This package <http://www.zoo-project.org/dl/tool-win32.zip>`__  contains full dependencies required to compile on a WIN32 platform and this one contains `full runtime dependencies <http://www.zoo-project.org/dl/zoo-runtime.zip>`__ . Place it to your ``C:\OSGeo4W\bin``.

Download the `binary version <http://www.zoo-project.org/dl/zoo_loader.cgi>`__  of ZOO Kernel for WIN32, then place it in the ``C:\OSGeo4W\bin`` directory. Don't forget to place the *main.cfg* file in the same directory, you can use a modified copy of  `this file <http://www.zoo-project.org/trac/browser/trunk/zoo-kernel/main.cfg>`__.

Additionaly, the binary version of the OGR Services Provider available from `here <http://www.zoo-project.org/dl/zoo-services-win32.zip>`__ can be used directly. Place the two libraries with their respective .zcfg files in your local ``C:\OSGeo4W\bin`` directory to do so.

Compile ZOO from source
---------------------

.. warning::
   Ensure to first perform the :ref:`prerequisite steps <kernel-installation-prereq>` before compiling the ZOO Kernel.

The following steps are for use with the Microsoft Visual Studio compiler (and tested with MSVC 2008).

1. Make sure the gnuwin32 tools *bison.exe*  and *flex.exe* are found in your path.  You can download the GNUwin32 tools `here <http://www.zoo-project.org/dl/tool-win32.zip>`__.

2. Modify the *nmake.opt* file to point to your local libraries.  You can find a modified nmake.opt that points to local libs `here <http://www.zoo-project.org/trac/attachment/ticket/27/nmake.opt>`__. 
   You can also find a modified ``zoo-project\zoo-kernel\makefile.vc`` file `here <http://www.zoo-project.org/trac/attachment/ticket/27/makefile.vc>`__.
   
3. Execute:

   ::
   
     nmake /f makefile.vc
     
4. A file *zoo_loader.cgi* should be created.  Note that if another file named *zoo_loader.cgi.manifest* is also created, you
   will have to run another command:
   
   ::
   
     nmake /f makefile.vc embed-manifest
     
5. Copy the files *zoo_loader.cgi*  and *main.cfg* into your cgi-bin directory.

6. Using the command prompt, test the zoo-kernel by executing the following command:

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
     
7. Edit the *main.cfg* file so that it contains values describing your WPS service.  An example of such 
   a file running on Windows is:
   
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
       
Optionally Compile Individual Services
.............................................................

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

