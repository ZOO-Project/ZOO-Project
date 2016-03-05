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
   paramater is not defined, then the ZOO-Kernel won't support CORS.

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

The ``lenv`` can be used to store runtime informations automatically
set by ZOO-Kernel before and during the execution of a WPS service,
with the following parameters:

 * ``sid`` (r): The WPS Service unique identifier,
 * ``status`` (rw): The current progress value ( a value between 0 and
   100 in percent (**%**) ),
 * ``cwd`` (r): The current working directory of ZOO-Kernel,
 * ``message`` (rw): An error message used when ``SERVICE_FAILED`` is returned (optional),
 * ``cookie`` (rw): The cookie to be returned to the client (for
   example for authentication purpose).

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

      
.. rubric:: Footnotes

.. [#f1] If GET requests are passed through ``xlink:href`` to the ZOO-Kernel , the latter will execute the request the first time and store the result  on disk. The next time the same request is executed, the cached file will be used and this will make your process run much faster. If ``cachedir`` was not specified in the ``main.cfg`` then the ``tmpPath`` value will be used.
.. [#f2] Usefull when the :ref:`kernel-mapserver` is activated (available since ZOO-Project version 1.3.0).
.. [#f3] ZOO-Kernel and MapServer are sharing the same metadata for OGC Web Services if the :ref:`kernel-mapserver` is activated.

   
