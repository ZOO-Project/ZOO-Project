.. _services-create:

Create your own ZOO-Services
=========================

:ref:`services_index` are quite easy to create once you have installed the ZOO Kernel and have 
chosen code (in the language of your choice) to turn into a ZOO service. Here are some 
HelloWorlds in Python, PHP, Java, C#  and JavaScript with links to their corresponding 
``.zcfg`` files.


.. contents:: :depth: 3


General information
----------------------

The function of the process for each programming language take three arguments: the main
configuration, inputs and outputs.

.. note:: The service must return **3** if the process run successfully
	  
.. note:: The service must return **4** if the process ended with an error

Python
------

You'll find here information needed to deploy your own Python Services Provider.

Python ZCFG requirements
************************

.. Note:: For each Service provided by your ZOO Python Services Provider, the ZCFG File 
          must be named the same as the Python module function name (also the case of
          characters is important).

The ZCFG file should contain the following :


serviceType
    Python 
serviceProvider
    The name of the Python module to use as a ZOO Service Provider. For instance, if your
    script, located in the same directory as your ZOO Kernel, was named ``my_module.py`` then
    you should use ``my_module`` (the Python module name) for the serviceProvider value in ZCFG file.

Python Data Structure used
**************************
The three parameters of the function are passed to the Python module as dictionaries.

Following you'll find an example for each parameters

Main configuration
^^^^^^^^^^^^^^^^^^^^^
Main configuration contains several informations, some of them are really useful to develop your service.
Following an example ::

  {
  'main': {'lang': 'en-UK',
	   'language': 'en-US',
	   'encoding': 'utf-8',
	   'dataPath': '/var/www/tmp',
	   'tmpPath': '/var/www/tmp',
	   'version': '1.0.0',
	   'mapserverAddress': 'http://localhost/cgi-bin/mapserv',
	   'isSoap': 'false',
	   'tmpUrl': 'http://localhost/tmp/',
	   'serverAddress': 'http://localhost/zoo'
	  },
  'identification': {'keywords': 'WPS,GIS',
		     'abstract': 'WPS services for testing ZOO',
		     'fees': 'None',
		     'accessConstraints': 'none',
		     'title': 'testing services'
		    },
  'lenv': {'status': '0',
	   'soap': 'false',
	   'cwd': '/usr/lib/cgi-bin',
	   'sid': '24709'
	  },
  'env': {'DISPLAY': 'localhost:0'},
  'provider': {'addressCountry': 'it',
	       'positionName': 'Developer',
	       'providerName': 'Name of provider',
	       'addressAdministrativeArea': 'False',
	       'phoneVoice': 'False',
	       'addressCity': 'City',
	       'providerSite': 'http://www.your.site',
	       'addressPostalCode': '38122',
	       'role': 'Developer',
	       'addressDeliveryPoint': 'False',
	       'phoneFacsimile': 'False', 
	       'addressElectronicMailAddress': 'your@email.com',
	       'individualName': 'Your Name'
	      }
  }

Inputs
^^^^^^^^^^^^
The inputs are somethings like this ::

  {
  'variable_name': {'minOccurs': '1',
		    'DataType': 'string',
		    'value': 'this_is_the_value',
		    'maxOccurs': '1',
		    'inRequest': 'true'
		   }
  }

The access to the value you have to require for the ``value`` parameter, something like this ::

  yourVariable = inputs['variable_name']['value']

Outputs
^^^^^^^^^^^^^
The outputs data as a structure really similar to the inputs one ::

  {
  'result': {'DataType': 'string',
	     'inRequest': 'true',
	    }
  }

There is no ``'value'`` parameter before you assign it ::

  inputs['result']['value'] = yourOutputDataVariable

The return statement has to be an integer: corresponding to the service status code.

To add a message for the wrong result you can add the massage to ``conf["lenv"]["message"]``,
for example:

.. code-block:: python

  conf["lenv"]["message"] = 'Your module return an error'

Sample ZOO Python Services Provider
***********************************

The following code represents a simple ZOO Python Services Provider which provides only one 
Service, the HelloPy one.

.. code-block:: python

  import zoo
  import sys
  def HelloPy(conf,inputs,outputs):
     outputs["Result"]["value"]="Hello "+inputs["a"]["value"]+" from Python World !"
     return zoo.SERVICE_SUCCEEDED

PHP
---

ZOO-API
*******

The ZOO-API for the PHP language is automatically available from your
service code. Tthe following functions are defined in the ZOO-API:

int zoo_SERVICE_SUCCEEDED()
    return the value of SERVICE_SUCCEEDED
int zoo_SERVICE_FAILED()
    return the value of SERVICE_FAILED
string zoo_Translate(string a)
    return the translated string (using the "zoo-service" `textdomain
    <http://www.gnu.org/software/libc/manual/html_node/Locating-gettext-catalog.html#index-textdomain>`__)

void zoo_UpdateStatus(Array conf,string message,int pourcent)
    update the status of the running service

PHP ZCFG requirements
**********************************

The ZCFG file should contain the following :

serviceType
    PHP 
serviceProvider
    The name of the php script (ie. service.php) to use as a ZOO Service Provider.

PHP Data Structure used
********************************

The three parameters are passed to the PHP function as 
`Arrays <php.net/manual/language.types.array.php>`__.

Sample ZOO PHP Services Provider
******************************************

.. code-block:: php

  <?
  function HelloPHP(&$main_conf,&$inputs,&$outputs){
     $tmp="Hello ".$inputs[S][value]." from PHP world !";
     $outputs["Result"]["value"]=zoo_Translate($tmp);
     return zoo_SERVICE_SUCCEEDED();
  }
  ?>

Java
----

Specifically for the Java support, you may add the following three
sections to your ``main.cfg`` file:

:[java]:
   This section is used to pass -D* parameters to the JVM  created by the
   ZOO-Kernel to handle your ZOO-Service (see `ref. 1
   <http://www.oracle.com/technetwork/java/javase/tech/vmoptions-jsp-140102.html#BehavioralOptions>`__
   or `ref. 2
   <http://www.oracle.com/technetwork/java/javase/tech/vmoptions-jsp-140102.html#PerformanceTuning>`__
   for sample available). 
   For each map ``a = b`` available in the ``[java]`` section, the
   option ``-Da=b`` will be passed to the JVM. 
:[javax]:
   The section is used to pass -X* options to the JVM (see
   `ref. <http://docs.oracle.com/cd/E22289_01/html/821-1274/configuring-the-default-jvm-and-java-arguments.html>`__). For
   each map ``a = b`` available in the ``[javax]`` section, the option
   ``-Xab`` will be passed to the JVM (ie. set ``mx=2G`` to pass
   ``-Xmx2G``).
:[javaxx]:
   This section is used to pass -XX:* parameters to the JVM  created by the
   ZOO-Kernel to handle your ZOO-Service (see `ref. 1
   <http://www.oracle.com/technetwork/java/javase/tech/vmoptions-jsp-140102.html#BehavioralOptions>`__
   or `ref. 2
   <http://www.oracle.com/technetwork/java/javase/tech/vmoptions-jsp-140102.html#PerformanceTuning>`__
   for sample available). 
   For each map ``a = b`` available in the ``[javaxx]`` section, the
   option ``-XX:a=b`` will be passed to the JVM. In case of a map ``a =
   minus`` (respectively ``a=plus``) then the option ``-XX:-a``
   (respectivelly ``-XX:+a``) will be passed.

ZOO-API
*******

Before you build your first ZOO-Service implemented in Java, it is
recommended that you first build the ZOO class of the Java ZOO-API.

.. Note:: You should build ZOO-Kernel prior to follow this instructions.

To build the ZOO.class of the ZOO-API for Java, use the following
command:

.. code-block:: guess

  cd zoo-api/java
  make

.. Note:: running the previous commands will require that both
          ``javac`` and ``javah`` are in your PATH.

You should copy the ``libZOO.so`` in a place Java can find it. In case you
have defined the ``java.library.path`` key as ``/usr/lib/cgi-bin``
(in the ``[java]`` section), then you should copy it there. 

.. code-block:: guess

  cp libZOO.so /usr/lib/cgi-bin

The ZOO-API provides the following functions:

:String translate(String s):
   This function call the internal ZOO-Kernel function responsible for
   searching a translation of ``s`` in the zoo-services dictionary.

:void updateStatus(Hashmap conf,String pourcent,String message):
   This function call the updateStatus ZOO-Kernel function responsible
   for updating the status of the running service (only usefull when
   the service has been called asynchronously).



Java ZCFG requirements
**********************************

.. Note:: For each Service provided by your ZOO Java Services Provider
          (your corresponding Java class), the ZCFG File should have
          the name of the Java public method corresponding to the
          service (case-sensitive).

The ZCFG file should contain the following :

serviceType
    Java 
serviceProvider
    The name of the Java class to use as a ZOO Service Provider. For instance, if your
    java class, located in the same directory as your ZOO-Kernel, was
    named ``HelloJava.class`` then you should use ``HelloJava``.

Java Data Structure used
********************************

The three parameters are passed to the Java function as 
`java.util.HashMap <http://docs.oracle.com/javase/8/docs/api/java/util/HashMap.html>`__.

Sample ZOO Java Services Provider
******************************************

.. code-block:: java

  import java.util.*;
  public class HelloJava {
    public static int HelloWorldJava(HashMap conf,HashMap inputs, HashMap outputs) {
       HashMap hm1 = new HashMap();
       hm1.put("dataType","string");
       HashMap tmp=(HashMap)(inputs.get("S"));
       java.lang.String v=tmp.get("value").toString();
       hm1.put("value","Hello "+v+" from JAVA WOrld !");
       outputs.put("Result",hm1);
       System.err.println("Hello from JAVA WOrld !");
       return ZOO.SERVICE_SUCCEEDED;
    }
  }

C#
----

Specifically for the C# support, you should add the following
section to your ``main.cfg`` file.


:[mono]:
   This section is used to define both ``libPath`` and ``etcPath``
   required by the Mono .NET Framework.

ZOO-API
*******

Before you build your first ZOO-Service implemented in Mono, you
should first build the ``ZMaps.dll`` containing the Mono ZOO-API.

.. Note:: You should build ZOO-Kernel prior to follow this instructions.

.. code-block:: guess

  cd zoo-api/mono
  make

Then you should copy the ``ZMaps.dll`` in your ``servicePath`` or in
the directory where your ``zoo_loader.cgi`` file is stored.

The ZOO-API is available from a C# class named ZOO_API and provides
the following static variables:

:int SERVICE_SUCCEEDED:
   Value to return in case your service end successfully.
:int SERVICE_FAILED:
   Value to retrun in case of failure.

The ZOO-API provides the following static functions:

:string Translate(String s):
   This function call the internal ZOO-Kernel function responsible for
   searching a translation of ``s`` in the zoo-services dictionary.

:void UpdateStatus(ZMaps conf,String pourcent,String message):
   This function call the updateStatus ZOO-Kernel function responsible
   for updating the status of the running service (only usefull when
   the service has been called asynchronously).


C# ZCFG requirements
**********************************

.. Note:: For each Service provided by your ZOO Mono Services Provider
          (your corresponding Mono class), the ZCFG File should have
          the name of the Mono public static function corresponding to the
          service (case-sensitive).

The ZCFG file should contain the following :

serviceType
    Mono 
serviceProvider
    The full name of the C# dll containing the ZOO-Service Provider
    (including ``.dll``).
serviceNameSpace
    The namespace of the C# class containing the ZOO-Service Provider.
serviceClass
    The name of the C# class containing the ZOO-Service Provider definition.

C# Data Structure used
********************************

The three parameters of the function are passed to the Mono static
function as ``ZMaps`` which are basically
``Dictionary<String,_ZMaps>``.

Sample ZOO C# Services Provider
******************************************

.. literalinclude:: ../../zoo-project/zoo-services/hello-mono/test.cs
   :language: csharp
   :lines: 24-100

Javascript
----------

ZOO API
*********

If you need to use :ref:`ZOO API <api>` in your service, you have first to copy ``zoo-api.js``
and ``zoo-proj4js.js`` where your services are located (for example in Unix system probably in
``/usr/lib/cgi-bin/``

Javascript ZCFG requirements
**********************************

.. Note:: For each Service provided by your ZOO Javascript Services Provider, the ZCFG File 
          must be named the same as the Javascript function name (also the case of
          characters is important).

The ZCFG file should contain the following :

serviceType
    JS 
serviceProvider
    The name of the JavaScript file to use as a ZOO Service Provider. For instance, if your
    script, located in the same directory as your ZOO Kernel, was named ``my_module.js`` then
    you should use ``my_module.js``.


Javascript Data Structure used
********************************

The three parameters of the function are passed to the JavaScript function as Object.

Sample ZOO Javascript Services Provider
******************************************

.. code-block:: javascript

  function hellojs(conf,inputs,outputs){
     outputs=new Array();
     outputs={};
     outputs["result"]["value"]="Hello "+inputs["S"]["value"]+" from JS World !";
     return Array(3,outputs);
  }


R
----------

ZOO API
*********

For using the R language from the ZOO-Project, you have first to copy
``minimal.r`` in the same directory as the ZOO-Kernel.

The ZOO-API is available from a R script and provide access to a
global zoo environment which contains both static variables and also
the dictionaries for outputs and conf:

:int zoo[["SERVICE_SUCCEEDED"]]:
   Value to return in case your service end successfully.
:int zoo[["SERVICE_FAILED"]]:
   Value to retrun in case of failure.

The ZOO-API provides the following functions:

:string ZOOTranslate(String s):
   This function call the internal ZOO-Kernel function responsible for
   searching a translation of ``s`` in the zoo-services dictionary.

:void ZOOUpdateStatus(ZMaps conf,String pourcent):
   This function call the updateStatus ZOO-Kernel function responsible
   for updating the status of the running service (only usefull when
   the service has been called asynchronously).


R ZCFG requirements
**********************************

.. Note:: For each Service provided by your ZOO R Services Provider,
	  the ZCFG File must be named the same as the R function name
	  (it is case-sensitive).

The ZCFG file should contain the following :

serviceType
   R 
serviceProvider
    The name of the R file to use as a ZOO Service Provider. For instance, if your
    script, located in the same directory as your ZOO Kernel, was named ``my_module.r`` then
    you should use ``my_module.r``.


R Data Structure used
********************************

The three parameters of the function are passed to the R function as
R dictionaries.

The specificity of the R language make that it was easier to use
global variales than passing parameters by reference as we do in
other progamming languages. It is the reason why you will have to
access outpus by using the global variable as for the main
configuration dictionary. 

Sample ZOO R Services Provider
******************************************

.. code-block:: javascript

    source("minimal.r")
		
    hellor <- function(a,b,c) {
        # Set the result
        zoo[["outputs"]][["Result"]][["value"]] <<- ZOOTranslate(paste("Hello",b[["S"]][["value"]],"from the R World!",sep=" "))
        # Return SERVICE_SUCCEEDEED
        return(zoo[["SERVICE_SUCCEEDEED"]])
    }

