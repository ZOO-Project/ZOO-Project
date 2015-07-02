.. _first_service:

Create your first ZOO Service
=====================

.. contents:: Table of Contents
    :depth: 5
    :backlinks: top

Introduction
---------------------

In this part, you will create and publish from a simple ZOO Service named 
``Hello`` which will simply return a hello message containing the input value 
provided. It will be usefull to present in deeper details general concept on how ZOO-Kernel works and handles request.

Service and publication process overview
-----------------------------------

Before starting developing a ZOO Service, you should remember that in 
ZOO-Project, a Service is a couple made of:

 * a metadata file: a ZOO Service Configuration File (ZCFG) containing metadata 
   informations about a Service (providing informations about default / supported 
   inputs and outputs for a Service)
 * a Services Provider: it depends on the programming language used, but for Python it
   is a module and for JavaScript a script file.

To publish your Service, which means make your ZOO Kernel aware of its presence,
you should copy a ZCFG file in the directory where ``zoo_loader.cgi`` is located (in this workshop, ``/usr/lib/cgi-bin``). 

.. warning:: only the ZCFG file is required  for the Service to be considerate as 
    available. So if you don't get the Service Provider, obviously your Execute 
    request will fail as we will discuss later.

Before publication, you should store your ongoing work, so you'll start by 
creating a directory to store the files of your Services Provider:

.. code-block:: none
    
    mkdir -p /home/user/zoo-ws2013/ws_sp/cgi-env

Once the ZCFG and the Python module are both ready, you can publish simply
by copying the corresponding files in the same directory as the ZOO-Kernel.

Create your first ZCFG file
--------------------------

You will start by creating the ZCFG file for the ``Hello`` Service. Edit the 
``/home/user/zoo-ws2013/ws_sp/cgi-env/Hello.zcfg`` file 
and add the following content:

.. code-block:: none
    :linenos:
    
    [Hello]
     Title = Return a hello message.
     Abstract = Create a welcome string.
     processVersion = 2
     storeSupported = true
     statusSupported = true
     serviceProvider = test_service
     serviceType = Python
     <DataInputs>
      [name]
       Title = Input string
       Abstract = The string to insert in the hello message.
       minOccurs = 1
       maxOccurs = 1
       <LiteralData>
           dataType = string
           <Default />
       </LiteralData>
     </DataInputs>
     <DataOutputs>
      [Result]
       Title = The resulting string
       Abstract = The hello message containing the input string
       <LiteralData>
           dataType = string
           <Default />
       </LiteralData>
     </DataOutputs>

.. note:: the name of the ZCFG file and the name between braket (here ``[Hello]``) 
    should be the same and correspond to the function name you will define in your 
    Services provider.

As you can see in the ZOO Service Configuration File presented above it is divided into
three distinct sections:
  #. Main Metadata information (from line 2 to 8)
  #. List of Inputs metadata information (from 9 line to 19)
  #. List of Outputs metadata information (from line 20 to 28)

You can get more informations about ZCFG from `the reference documentation 
<http://zoo-project.org/docs/services/zcfg-reference.html>`__.

If you copy the ``Hello.zcfg`` file in the same directory as your ZOO Kernel 
then you will be able to request for DescribeProcess using the ``Hello`` 
``Identifier``. The ``Hello`` service should also be listed from Capabilities 
document.

.. code-block:: none
   cp /home/user/zoo-ws2013/ws_sp/cgi-env/Hello.zcfg /usr/lib/cgi-bin

Test requests
---------------------

In this section you will tests each WPS requests : GetCapabilities, 
DescribeProcess and Execute. Note that only GetCapabilities and DescribeProcess
should work at this step.

Test the GetCapabilities request
.....................................................

If you run the ``GetCapabilities`` request:

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=GetCapabilities&service=WPS

Now, you should find your Hello Service in a ``Process`` node in 
``ProcessOfferings``:

.. code-block:: xml
    
    <wps:Process wps:processVersion="2">
     <ows:Identifier>Hello</ows:Identifier>
     <ows:Title>Return a hello message.</ows:Title>
     <ows:Abstract>Create a welcome string.</ows:Abstract>
    </wps:Process>

Test the DescribeProcess request
.....................................................

You can access the ``ProcessDescription`` of the ``Hello`` service using the 
following ``DescribeProcess`` request:

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=DescribeProcess&service=WPS&version=1.0.0&Identifier=Hello

You should get the following response:

.. code-block:: xml
    
    <wps:ProcessDescriptions xmlns:ows="http://www.opengis.net/ows/1.1" xmlns:wps="http://www.opengis.net/wps/1.0.0" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.opengis.net/wps/1.0.0 http://schemas.opengis.net/wps/1.0.0/wpsDescribeProcess_response.xsd" service="WPS" version="1.0.0" xml:lang="en-US">
      <ProcessDescription wps:processVersion="2" storeSupported="true" statusSupported="true">
        <ows:Identifier>Hello</ows:Identifier>
        <ows:Title>Return a hello message.</ows:Title>
        <ows:Abstract>Create a welcome string.</ows:Abstract>
        <DataInputs>
          <Input minOccurs="1" maxOccurs="1">
            <ows:Identifier>name</ows:Identifier>
            <ows:Title>Input string</ows:Title>
            <ows:Abstract>The string to insert in the hello message.</ows:Abstract>
            <LiteralData>
              <ows:DataType ows:reference="http://www.w3.org/TR/xmlschema-2/#string">string</ows:DataType>
              <ows:AnyValue/>
            </LiteralData>
          </Input>
        </DataInputs>
        <ProcessOutputs>
          <Output>
            <ows:Identifier>Result</ows:Identifier>
            <ows:Title>The resulting string</ows:Title>
            <ows:Abstract>The hello message containing the input string</ows:Abstract>
            <LiteralOutput>
              <ows:DataType ows:reference="http://www.w3.org/TR/xmlschema-2/#string">string</ows:DataType>
            </LiteralOutput>
          </Output>
        </ProcessOutputs>
      </ProcessDescription>
    </wps:ProcessDescriptions>

Test the Execute request
.....................................................

Obviously, you cannot run your Service because the Python file was not published
yet. If you try the following ``Execute`` request:

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=Hello&DataInputs=name=toto

You should get an ExceptionReport similar to the one provided in the following, 
which is normal behavior:

.. code-block:: xml

    <ows:ExceptionReport xmlns:ows="http://www.opengis.net/ows/1.1" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xlink="http://www.w3.org/1999/xlink" xsi:schemaLocation="http://www.opengis.net/ows/1.1 http://schemas.opengis.net/ows/1.1.0/owsExceptionReport.xsd" xml:lang="en-US" version="1.1.0">
      <ows:Exception exceptionCode="NoApplicableCode">
        <ows:ExceptionText>Python module test_service cannot be loaded.</ows:ExceptionText>
      </ows:Exception>
    </ows:ExceptionReport>

Implementing the Python Service
-------------------------------

General Principles
.....................................................

The most important thing you must know when implementing a new ZOO-Services 
using the Python language is that the function corresponding to your Service 
returns an integer value representing the status of execution 
(``SERVICE_FAILED`` [#f1]_ or ``SERVICE_SUCCEEDED`` [#f2]_) and takes three 
arguments (`Python dictionaries
<http://docs.python.org/tutorial/datastructures.html#dictionaries>`__): 

  -  ``conf`` : the main environment configuration (corresponding to the main.cfg content) 
  - ``inputs`` : the requested / default inputs (used to access input values)
  - ``outputs`` : the requested / default outputs (used to store computation result)

.. note:: when your service return ``SERVICE_FAILED`` you can set 
    ``conf["lenv"]["message"]`` to add a personalized message in the ExceptionReport 
    returned by the ZOO Kernel in such case.

You get in the following a sample ``conf`` value based on the ``main.cfg`` file you 
saw `before <using_zoo_from_osgeolivevm.html#zoo-kernel-configuration>`__.

.. code-block:: javascript
    :linenos:    

    {
      "main": {
        language: "en-US",
        lang: "fr-FR,ja-JP",
        version: "1.0.0",
        encoding: "utf-8",
        serverAddress: "http://localhost/cgi-bin/zoo_loader.cgi",
        dataPath: "/var/www/zoows-demo/map/data",
        tmpPath: "/var/www/temp",
        tmpUrl: "../temp",
        cacheDir: "/var/www/temp/"
      },
      "identification": {
        title: "The ZOO-Project WPS Server FOSS4G 2013 Nottingham Workshop",
        keywords: "WPS,GIS,buffer",
        abstract: "Demo version of Zoo-Project for OSGeoLiveDVD 2013. See http://www.zoo-project.org",
        accessConstraints: "none",
        fees: "None"
      },
      "provider": {
        positionName: "Developer",
    	providerName: "ZOO-Project",
    	addressAdministrativeArea: "Lattes",
    	addressCountry: "fr",
    	phoneVoice: "False",
    	addressPostalCode: "34970",
    	role: "Dev",
    	providerSite: "http://www.zoo-project.org",
    	phoneFacsimile: "False",
    	addressElectronicMailAddress: "gerald.fenoy@geolabs.fr",
    	addressCity: "Denver",
    	individualName: "Gérald FENOY"
      }

In the following you get a sample outputs value passed to a Python or a JavaScript Service:

.. code-block:: javascript
    :linenos:    

    {
      'Result': {
        'mimeType': 'application/json', 
	'inRequest': 'true', 
	'encoding': 'UTF-8'
      }
    }

.. note:: the ``inRequest`` value is set internally by the ZOO-Kernel and can be    used to determine from the Service if the key was provided in the request.

ZOO-Project provide a ZOO-API which was originally only available for
JavaScript services, but thanks to the work of the ZOO-Project
community, now you have also access to a ZOO-API when using
the Python language. Thanks to the Python ZOO-API you don't have to remember anymore
the value of SERVICE_SUCCEDED and SERVICE_FAILED, you
have the capability to translate any string from your Python service
by calling the ``_`` function (ie: ``zoo._('My string to
translate')``) or to update the current status of a running service by
using the ``update_status`` function the same way you use it from
JavaScript or C services.

The Hello Service
.....................................................

You can copy and paste the following into the 
``/home/user/zoo-ws2013/ws_sp/cgi-env/test_service.py`` file.

.. code-block:: python
    
    import zoo
    def Hello(conf,inputs,outputs):
        outputs["Result"]["value"]=\
		"Hello "+inputs["name"]["value"]+" from the ZOO-Project Python world !"
        return zoo.SERVICE_SUCCEEDED

Once you finish editing the file, you should copy it in the ``/usr/lib/cgi-bin`` directory: 

.. code-block:: none
    
    sudo cp /home/user/zoo-ws2013/ws_sp/cgi-env/* /usr/lib/cgi-bin


Interracting with your service using Execute requests
----------------------------------------------

Now, you can request for Execute using the following basic url:

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=Hello&DataInputs=name=toto

You can request the WPS Server to return a XML WPS Response containing the result of 
your computation, requesting for ResponseDocument or you can access the data directly
requesting for RawDataOutput. 

* Sample request using the RawDataOutput parameter:

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=Hello&DataInputs=name=toto&RawDataOutput=Result

* Sample request using the default ResponseDocument parameter:

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=Hello&DataInputs=name=toto&ResponseDocument=Result

When you are using ResponseDocument there is specific attribut you can use to ask 
the ZOO Kernel to store the result: ``asReference``. You can use the following example:

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=Hello&DataInputs=name=toto&ResponseDocument=Result@asReference=true

When computation take long time, the client should request the
execution of a Service by setting both ``storeExecuteResponse`` and
``status`` parameter to true to force asynchronous execution. This
will make the ZOO-Kernel return, without waiting for the Service execution
completion but after starting another ZOO-Kernel process responsible
of the Service execution, a ResponseDocument containing a ``statusLocation``
attribute which can be used to access the status of an ongoing service
or the result when the process ended [#f3]_.

.. code-block:: none
    
    http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=Hello&DataInputs=name=toto&ResponseDocument=Result&storeExecuteResponse=true&status=true

Conclusion
-----------------

Even if this first service was really simple it was useful to illustrate how the 
ZOO-Kernel fill ``conf``, ``inputs`` and ``outputs`` parameter prior to load 
and run your function service, how to write a ZCFG file, how to publish a Services 
Provider by placing the ZCFG and Python files in the same directory as the 
ZOO-Kernel, then how to interract with your service using both 
``GetCapabilities``, ``DescribeProcess`` and ``Execute`` requesr. We will see 
in the `next section <building_blocks_presentation.html>`__ how to write similar requests 
using the XML syntax.

.. rubric:: Footnotes

.. [#f1] ``SERVICE_FAILED=4``
.. [#f2] ``SERVICE_SUCCEEDED=3``
.. [#f3]  To get on-going status url in ``statusLocation``, you'll need to setup the `utils/status <http://www.zoo-project.org/trac/browser/trunk/zoo-project/zoo-services/utils/status>`_ Service. If you don't get this service available, the ZOO-Kernel will simply give the url to a flat XML file stored on the server which will contain, at the end of the execution, the result of the Service execution.
