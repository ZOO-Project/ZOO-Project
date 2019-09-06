
.. _kernel_config:

ZOO-Kernel configuration
========================

Main configuration file
-----------------------

ZOO-Kernel general settings are defined in a configuration file called
``main.cfg``. This file is stored in the same directory as ZOO-Kernel
(``/usr/lib/cgi-bin/`` in most cases). It provides usefull metadata information on your ZOO-Kernel installation.     

.. warning:: 
  ZOO-Kernel (``/usr/lib/cgi-bin/zoo_loader.cgi``) and its
  configuration file (``/usr/lib/cgi-bin/main.cfg``) must be in the
  same directory.
  
.. note:: 
  Information contained by ``/usr/lib/cgi-bin/main.cfg`` is accessible from WPS Services at runtime, so when *Execute* requests are used.

Default main.cfg
...............................

An example *main.cfg* file is given here as reference.

.. code-block:: guess
    :linenos:
    
    [headers]
    X-Powered-By=ZOO@ZOO-Project
    
    [main]
    version=1.0.0
    encoding=utf-8
    dataPath=/var/data
    tmpPath=/var/www/temp
    cacheDir=/var/www/cache
    sessPath=/tmp
    serverAddress=http://localhost/cgi-bin/zoo_loader.cgi
    lang=fr-FR,ja-JP
    language=en-US
    mapserverAddress=http://localhost/cgi-bin/mapserv.cgi
    msOgcVersion=1.0.0
    tmpUrl=http:/localhost/temp/
    cors=false
    
    [identification]
    keywords=t,ZOO-Project, ZOO-Kernel,WPS,GIS
    title=ZOO-Project demo instance
    abstract= This is ZOO-Project, the Open WPS platform. 
    accessConstraints=none
    fees=None
    
    [provider]
    positionName=Developer
    providerName=GeoLabs SARL
    addressAdministrativeArea=False
    addressDeliveryPoint=1280, avenue des Platanes
    addressCountry=fr
    phoneVoice=+33467430995
    addressPostalCode=34970
    role=Dev
    providerSite=http://geolabs.fr
    phoneFacsimile=False
    addressElectronicMailAddress=gerald@geolabs.fr
    addressCity=Lattes
    individualName=Gerald FENOY


Main section 
...............................

The main.cfg ``[main]`` section parameters are explained bellow.

 * ``version``: Supported WPS version.
 * ``encoding``: Default encoding of WPS Responses.
 * ``dataPath``: Path to the directory where data files are stored (used to store mapfiles and data when MapServer support is activated).
 * ``tmpPath``: Path to the directory where temporary files are stored (such as *ExecuteResponse* when *storeExecuteResponse* is set to true).
 * ``tmpUrl``: URL to access the temporary files directory (cf. ``tmpPath``).
 * ``cacheDir``: Path to  the directory where cached request files [#f1]_ are stored (optional).
 * ``serverAddress``: URL to the ZOO-Kernel instance.
 * ``mapservAddress``: URL to the MapServer instance (optional).
 * ``msOgcVersion``: Version of all supported OGC Web Services output [#f2]_
   (optional).
 * ``lang``: Supported natural languages separated by a coma (the first is the default one),
 * ``cors``: Define if the ZOO-Kernel should support `Cross-Origin
   Resource Sharing <https://www.w3.org/TR/cors/>`__. If this
   parameter is not defined, then the ZOO-Kernel won't support CORS.
 * ``servicePath``: Define a specific location to search for services
   rather than using the ZOO-Kernel directory. If this parameter is
   not defined, then the ZOO-Kernel will search for services using its
   directory.
 * ``libPath``: (Optional) Path to a directory where the ZOO-kernel should search for
   service providers, e.g., shared libraries with service implementations 
   (the ``serviceProvider`` parameter in the service configuration (.zcfg) file).   
 * ``memory``: (Optional) can take the value ``load`` to ensure that
   the value field of the inputs data will be filled by the ZOO-Kernel
   or ``protected`` to have only the ``cache_file`` filled.
 * ``handleText``: (Optional) set it to ``true`` to get your Complex data
   nodes containing text not requiring a single CDATA node. Be aware
   that in case you use any HTML or XML there, you will then need to
   rebuild the string to get the original format of the text from your
   service code. In case you do not add handleText or set its value to
   true, you will simply need to use the value as it was provided in
   a single CDATA node provided in the Execute request.

.. warning:: 
  The ``libPath`` parameter is currently only recognized by services implemented
  in C/C++ or PHP, and may be moved to another section in future versions.


.. warning:: 
  Depending on the ``memory`` parameter the WPS Service will receive
  different fields (``value`` or ``cache_file``).
   
In case you have activated the MapServer support, please refer to
:ref:`this specific section <kernel-mapserver-main.cfg>`. 


Identification and Provider 
..........................................

The ``[identification]`` and ``[provider]`` sections are not ZOO-Project
specific. They provide OGC metadata [#f3]_ and should be set according
to the `XML Schema Document
<http://schemas.opengis.net/ows/1.1.0/ows19115subset.xsd>`__ which
encodes the parts of ISO 19115 used by the common
*ServiceIdentification* and *ServiceProvider* sections of the
*GetCapabilities* operation response, known as the service metadata
XML document.

Details of the common OWS 1.1.0 *ServiceIdentification* section can be
found in this `XML Schema Document
<http://schemas.opengis.net/ows/1.1.0/owsServiceIdentification.xsd>`__.

Details of the common OWS 1.1.0 *ServiceProvider* section can be
found in this `XML Schema Document
<http://schemas.opengis.net/ows/1.1.0/owsServiceProvider.xsd>`__.


Additional sections
--------------------------------

All the additional sections discribed in the following section are
optional.

Headers section
...............................

The ``[headers]`` section can be set in order to define a specific HTTP
Response header, which will be used for every response. As an example,
you can check http://zoo-project.org using *curl* command line tool
and notice the specific header *X-Powered-By: Zoo-Project@Trac*.

In case you want to allow CORS support for POST requests coming from
``myhost.net``, then you should define the following minimal
parameters in this section:

.. code-block:: guess
    :linenos:
    
    Access-Control-Allow-Origin=myhost.net
    Access-Control-Allow-Methods=POST
    Access-Control-Allow-Headers=content-type

curl section
...............................

The `[curl]` section is used on windows platform to specify, using the
`cainfo` parameter, where is located the
`cacert.pem <https://curl.haxx.se/docs/caextract.html>`__ file on your
machine. An example bellow is provided to illustrate sur a setting. 

.. code-block:: guess
    :linenos:
    
    [curl]
    cainfo=./cacert.pem



env section
...............................

The ``[env]`` section can be used to store specific environment
variables to be set prior the loading of *Services Provider* and Service execution.

A typical example is when a Service requires the access to a X server
running on *framebuffer*, which takes to set the DISPLAY environnement
variable, as follow:

.. code-block:: guess
    :linenos:
    
    [env]
    DISPLAY=:1

In case you have activated the OTB support, please refer to :ref:`this
specific section <kernel-orfeotoolbox-main.cfg>`.

lenv section
...............................

The ``lenv`` section is used by the ZOO-Kernel to store runtime informations
before the execution of a WPS service, it contains the following
parameters:

 * ``sid`` (r): The WPS Service unique identifier,
 * ``status`` (rw): The current progress value ( a value between 0 and
   100 in percent (**%**) ),
 * ``cwd`` (r): The current working directory of ZOO-Kernel,
 * ``message`` (rw): An error message used when ``SERVICE_FAILED`` is returned (optional),
 * ``cookie`` (rw): The cookie to be returned to the client (for
   example for authentication purpose).
 * ``file.pid`` (r): The file used by the ZOO-Kernel to store process identifier.
 * ``file.sid`` (r): The file used by the ZOO-Kernel to store service identifier.
 * ``file.responseInit`` (r): The file used by the ZOO-Kernel to store
   the initial (then final) WPS response.
 * ``file.responseFinal`` (r): The file used by the ZOO-Kernel to
   temporary store the final WPS response.

renv section
...............................

The ``renv`` section is automatically created by the ZOO-Kernel before
the execution of a WPS service, it contains all the environment
variables available at runtime (so including the header fields in case
it is used through http, refer to [https://tools.ietf.org/html/rfc3875
rfc3875] for more details).


senv section
...............................

The ``senv`` section can be used to store sessions information on the
server side. Such information can then be accessed automatically from
the Service if the server is requested using a valid cookie (as
defined in ``lenv`` section). ZOO-Kernel will store the values set in the
``senv`` maps on disk, load it and dynamically replace its content to
the one in the ``main.cfg``. The ``senv`` section must contain the
following parameter at least:

 * ``XXX``: The session unique identifier where ``XXX`` is the name
   included in the cookie which is returned.

.. _cookie_example:

For instance, adding the following in the Service source code  :

.. code:: python
    
    conf["lenv"]["cookie"]="XXX=XXX1000000; path=/" 
    conf["senv"]={"XXX": "XXX1000000","login": "demoUser"}

means that ZOO-Kernel will create a file named ``sess_XXX1000000.cfg``
in the ``cacheDir`` directory, and will return the specified cookie to the client. Each time the client will 
request ZOO-Kernel using this cookie, it will automatically load the
value stored before the Service execution.

Security section
...............................

The ``[security]`` section can be used to define what headers, the
ZOO-Kernel has initially received in the request, should be passed
to other servers for accessing resources (such as WMS, WFS, WCS
or any other file passed as a reference). This section contains two
parameters:

 * ``attributes``: The header to pass to other servers (such as
   Authorization, Cookie, User-Agent ...),
 * ``hosts``: The host for wich the restriction apply (can be "*" to
   forward header to every server or a coma separated list of host
   names, domain, IP).

Both parameters are mandatory.

Suppose you need to share Authorization, Cookie and User-Agent to
every server for accessing ressources, then yo ucan use the following
section definition:

.. code:: 

    [security]
    attributes=Authorization,Cookie,User-Agent
    hosts=*

In case only local servers require such header forwarding, you may use
the following definition:

.. code:: 

    [security]
    attributes=Authorization,Cookie,User-Agent
    hosts=localhost,127.0.0.1

Optionaly, you can also define the shared url(s), meaning that even if
the ressource requires authentication to be accessed, this
authentifcation won't be used to define the name for storing the
file. Hence, two user with different authentication will use the same
file as it is considerated as shared. You can find bellow a sample
security section containing the shared parameter. In this example,
every requests to access the coverage using the url defined in the
shared parameter (``myHost/cgi-bin/WCS_Server``) will be shared
between users.

.. code::

    [security]
    attributes=Authorization,Cookie,User-Agent
    hosts=localhost,127.0.0.1
    shared=myHost/cgi-bin/WCS_Server

.. _zoo_activate_db_backend:

Database section
...............................

The database section allows to configure the
:ref:`ZOO-Kernel optional database support <zoo_install_db_backend>`. 

.. code-block:: guess

	[database]
	dbname=zoo_project
	port=5432
	user=username
	host=127.0.0.1
	type=PG
	schema=public

This will generate strings to be passed to GDAL to connect the
database server:

.. code-block:: guess
   
    <type>:host=<host> port=<port>  user=<user> dbname=<dbname>


With the previous database section, it will give the following:

.. code-block:: guess

    PG:"dbname=zoo_project host=127.0.0.1 port=5432 user=username"

Please refer to this `section <zoo_create_db_backend>`_ to learn how
to setup the database.

.. _zoo_activate_metadb:

Metadb section
...............................

The metadb section allows to configure the ZOO-Kernel to access :ref:`the
metadata information about WPS Services <zoo_create_metadb>` by using
a PostgreSQL database in addition to the zcfg files.

.. code-block:: guess

	[metadb]
	dbname=zoo_metadb
	port=5432
	user=username
	host=127.0.0.1
	type=PG

This will generate strings to be passed to GDAL to connect the
database server:

.. code-block:: guess
   
    <type>:host=<host> port=<port>  user=<user> dbname=<dbname>


With the previous database section, it will give the following:

.. code-block:: guess

    PG:"dbname=zoo_metadb host=127.0.0.1 port=5432 user=username"

Please refer to this `section <zoo_create_metadb>`_ to learn how
to setup the database.

Include section
...............................

The ``[include]`` section (optional) lists explicitely a set of service configuration files
the the ZOO-Kernel should parse, e.g.,


.. code-block:: guess
    :linenos:
    
    [include]
    servicename1 = /my/service/repository/service1.zcfg
    servicename2 = /my/service/repository/service2.zcfg

The ``[include]`` section may be used to control which services are exposed to particular user groups.
While service configuration files (.zcfg) may be located in a common repository or in arbitrary folders,
main.cfg files at different URLs may include different subsets of services. 

When the ZOO-Kernel handles a request, it will first check if there is an ``[include]`` 
section in main.cfg and then search for other .zcfg files in the current working directory (CWD) and 
subdirectories. If an included service happens to be located in a CWD (sub)directory, 
it will be published by its name in the ``[include]`` section. For example, the service
``/[CWD]/name/space/myService.zcfg``
would normally be published as name.space.myService, but if it is listed in the ``[include]`` section 
it will be published simply as myService: 

.. code-block:: guess
    :linenos:
    
    [include]
    myService =  /[CWD]/name/space/myService.zcfg

On the other hand, with

.. code-block:: guess
    :linenos:
    
    [include]
    myService =  /some/other/dir/myService.zcfg

there would be two distinct services published as myService and name.space.myService, respectively, 
with two different zcfg files.

.. note:: 
  As currently implemented, the ZOO-Kernel searches the CWD for the library files of 
  included services if the ``libPath`` parameter is not set.


OpenAPI Specification configuration file
-----------------------------------------

Since revision 949 of the ZOO-Kernel, you can now activate the OGC
API - Processing support. In such a case you will need to have an
``oas.cfg`` file located in tne same directory where the ``main.cfg`` is.

This ``oas.cfg`` file gets the same syntactic rules than the
``main.cfg``. The ZOO-Kernel uses this file to produce information
about the open API specification it is referring to. 

The first section to be found in the ``oas.cfg`` file should be the
``[openapi]``. It contains the following parameters:

 * ``rootUrl``: the URL to access the ZOO-Kernel using OGC API - Processing
 * ``links``: the links provided from the root
 * ``paths``: the full paths list
 * ``parameters``: the parameters list defined in paths
 * ``header_parameters``: the parameters list client applications can send as header
 * ``version``: the Open API Specification version
   
For any links and paths ``/A`` defined, you will have a corresponding
``[/A]`` and ``[A]`` sections. In the ``[/A]`` section you will define
the rel, type and title used from the root URL to produce the `links
<https://github.com/opengeospatial/wps-rest-binding/blob/master/core/openapi/schemas/link.yaml>`__
list and the `paths object
<https://github.com/OAI/OpenAPI-Specification/blob/master/versions/3.0.0.md#pathsObject>`__
from. In the corresponding ``[A]`` section, you will define the
following parameters:

 * ``method``: the HTTP method to use to access this resource
 * ``title``: resource title
 * ``abstract``: resource description 
 * ``tags``: tags to classify this resource
 * ``tags_description``: the tag description
 * ``schema``: the schema specifying the resource

In case you want to define multiple methods to access a resource,  you
can then use the length parameter to define the number of parameters
group to parse, then use ``method_1``, ``title_1``, ``abstract_1``,
``tags_1``, ``tags_description_1`` and ``schema_1`` and so on to add
one or more access method (and other attributes) to this resource.

When declaring resource access you may want to add a parameter to your
request. In such a case, you can add a parameter named "parameters"
which contain the supported parameters list. All parameters defined
should be rooted in the components field. Parameters which can be used
in the path will be identified by ``{NAME}`` and will get a specific
section for its definition:

 * ``type``: nature of the parameter (i.e. string)
 * ``title``: parameter title
 * ``abstract``: parameter description
 * ``in``: where the parameter can be used (ie. path, header, query)
 * ``required``: define if the parameter is optional (false) or not (true)
 * ``example``: (optional) provide an example value / URL
   
In addition to the sections defined previously, there are three other
sections that we did not cover yet. Theses sections are:

 * ``[requestBody]``: defining the request body abstract (description), type (application/json) and schema (reference).
 * ``[exception]``: defining the exception bastract (description), type (application/json) and schema (reference).
 * ``[conformTo]``: referring to links list of the requirements classes the server implements and conforms to

For more information on how to interact with this WPS REST Binding, please refer
to this `page
<https://github.com/opengeospatial/wps-rest-binding#overview>`__ or use
the `Swagger UI <https://swagger.io/tools/swagger-ui/>`__. A live
instance is available `here <https://demo.mapmint.com/swagger-ui/dist/>`__.
  
.. rubric:: Footnotes

.. [#f1] If GET requests are passed through ``xlink:href`` to the ZOO-Kernel , the latter will execute the request the first time and store the result  on disk. The next time the same request is executed, the cached file will be used and this will make your process run much faster. If ``cachedir`` was not specified in the ``main.cfg`` then the ``tmpPath`` value will be used.
.. [#f2] Usefull when the :ref:`kernel-mapserver` is activated (available since ZOO-Project version 1.3.0).
.. [#f3] ZOO-Kernel and MapServer are sharing the same metadata for OGC Web Services if the :ref:`kernel-mapserver` is activated.

   
