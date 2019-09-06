.. _install-installation:

Installation on Unix/Linux
==========================

To build and install ZOO-Project on your Web Server you will need 4
steps :

.. contents:: 
    :local:
    :depth: 1
    :backlinks: top


Build cgic
----------

Run the following commands from the ``thirds/cgic`` directory to build
the cgic library.

::

   cd thirds/cgic
   make

The cgic library originaly come from `http://www.boutell.com/cgic
<http://www.boutell.com/cgic>`_.

.. warning:: 

   You may need to edit the ``Makefile`` in case you are using a 64 bits
   platform for building and your fcgi library is not located in ``/usr/lib64``.

Install ZOO-Kernel
------------------


For the impatient
.................

Run the following commands from the directory where you :ref:`install-download` and extracted the ZOO Kernel source code in order to build the ``zoo_loader.cgi`` CGI program with default options.

::

   cd zoo-project/zoo-kernel
   autoconf  
   ./configure
   make
   make install

This should produce executables for the *zoo_loader.cgi* CGI program
(located per default in ``/usr/lib/cgi-bin/``) and a shared library
``libzoo_service``  (located per default in ``/usr/local/lib``).

.. warning:: 

   Edit ZOO-Kernel installation settings in the ``main.cfg`` file (set
   ``tmpPath`` and ``tmpUrl`` to fit your web server configuration).


Configure options
.................

This section provides information on :ref:`kernel_index` configure options. It is recommanded to also read the :ref:`kernel_config` section for configuration technical details.


Here is the list of available options in the same order as returned by
``./configure --help`` command:

.. contents:: 
    :local:
    :depth: 2
    :backlinks: top

Specific CGI Directory
**********************

In the case your ``cgi-bin`` is not located in ``/usr/lib/`` as it is
assumed per default, then you can specify a specific target location
by using the following option:

.. code::

    ./configure --with-cgi-dir=/Lbrary/WebServer/CGI-Executables

This way, when you will run the ``make install`` command, the
ZOO-Kernel will be deployed in the specified directory (so,
`/Lbrary/WebServer/CGI-Executables`` in this example).

Specific main.cfg location  (Optional)
**************************************

Per default, the ZOO-Kernel search for the ``main.cfg`` file from its
installation directory but, in case you want to store this file in
another place, then you can use the ``--with-etc-dir`` option so it
will search for the ``main.cfg`` file in the ``sysconfdir`` directory.

For instance, you can define that the directory to store the
``main.cfg`` file is the ``/etc/zoo-project`` directory, by using the
following command:

.. code::

    ./configure --with-etc-dir=yes --sysconfdir=/etc/zoo-project


.. _zoo_install_db_backend:

Use a Database Backend (Optional) 
**********************************

If you want to share the ongoing informations of running services
between various ZOO-Kernel instances then you should use this
option: ``--with-db-backend``. This way, both the *GetStatus*,
*GetResult* and *Dismiss* requests can be run from any host accessing
the same database. Obviously, this will require that the ZOO-Kernel is
able to access the Database server. To learn how to configure this
connection and how to create this database please refer to :ref:`[1]
<zoo_activate_db_backend>` and :ref:`[2] <zoo_create_db_backend>`
respectively.

.. note::
    By now, the ZOO-Kernel is not able to handle correctly the
    *Dismiss* request from any host. Nevertheless, it will provide
    valid response from any host, but only the host which is really
    handling the service will be able to stop it and remove all the
    linked files.

.. _zoo_create_db_backend:

To create a new database to be used by the ZOO-Kernel, you have
to load the `schema.sql
<http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel/sql/schema.sql>`_ 
file. For instance, you may run the following:

.. code::

    createdb zoo_project
    psql zoo_project -f zoo-project/zoo-kernel/sql/schema.sql

.. note::
    You can choose another schema to store ZOO-Kernel specific
    informations. In such a case, you would need to edit the
    schema.sql file to uncomment line `33
    <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel/sql/schema.sql#L33>`_
    and `34
    <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel/sql/schema.sql#L34>`_.

.. _zoo_create_metadb:

Metadata Database (Optional)
*****************************


It is possible to use a PostgreSQL database to store metadata
information about WPS Services. This support is optional and require
to be activated by using the ``--with-metadb=yes`` option.

To create the database for storing the metadata informations about the
WPS Services, you may use the following command:

.. code::

    createdb zoo_metadb
    psql zoo_metadb -f zoo-project/zoo-kernel/sql/zoo_collectiondb.sql

In case you want to convert an existing zcfg file then, you can use
the ``zcfg2sql`` tool from the command line. It can be found in
``thirds/zcfg2sql`` and can be build simply by running the ``make``
command. After compilation you only need to give it the path of the
zcfg file you want to obtain the SQL queries required to store the
metadata informations in the database rather than in zcfg file.

For instance you may use the following command:

.. code::

    #Direct import in the zoo_metadb database
    ./zcfg2sql /Path/To/MyZCFGs/myService.zcfg | psql zoo_metadb
    #Create a SQL file for a futur import
    ./zcfg2sql /Path/To/MyZCFGs/myService.zcfg > myService.sql



YAML Support (Optional) 
************************

If ``yaml.h`` file is not found in your ``/usr/include`` directory and
``libyaml.so`` is not found in ``/usr/lib``, a ``--with-yaml`` option
can be used to specify its location. For instance, if the header file
lies in ``/usr/local/include`` and the shared library is located in
``/usr/local/lib``, you may use the following command:

::

  $ ./configure --with-yaml=/usr/local


FastCGI Support (Required) 
***************************

If your FastCGI library is not available in the default search path, a
``--with-fastcgi`` option can be used to specify its location. For
instance, if ``libfcgi.so`` lies in ``/usr/local/lib`` which is not in
your ``LD_SEARCH_PATH``, you may use the following command:

::

  $ ./configure --with-fastcgi=/usr/local



GDAL Support (Required) 
************************

If gdal-config program is not found in your ``PATH``, a
``--with-gdal-config`` option can be used to specify its location. For
instance, if ``gdal-config`` lies in ``/usr/local/bin`` which is not in
your ``PATH``, you may use the following command:

::

  $ ./configure --with-gdal-config=/usr/local/bin/gdal-config


GEOS Support (Optional) 
************************

If ``geos-config`` program is not found in your ``PATH``, a
``--with-geosconfig`` option can be used to specify its location. For
instance, if ``geos-config`` lies in ``/usr/local/bin`` which is not in
your ``PATH``, you may use the following command:

::

  $ ./configure --with-geosconfig=/usr/local/bin/geos-config


CGAL Support (Optional) 
************************

If ``CGAL/Delaunay_triangulation_2.h`` program is not found in your
``/usr/include`` directory, a ``--with-cgal`` option can be used to
specify its location. For instance, if the file lies in
``/usr/local/include`` which is not in your PATH, you may use the
following command:

::

  $ ./configure --with-cgal=/usr/local



MapServer Support (Optional) 
*****************************


In order to activate the WMS, WFS and WCS output support using
MapServer, the ``--with-mapserver`` option must be used. The path to
``mapserver-config`` which is located in the source code of MapServer
must also be set, using the following command:

::

  $ ./configure --with-mapserver=/path/to/your/mapserver_config/


Read more about the :ref:`kernel-mapserver`.

XML2 Support (Required) 
************************

If xml2-config program is not found in PATH, a *--with-xml2config* option can be used  to specify its location. For instance, if xml2-config is installed in ``/usr/local/bin`` which is not in PATH, you may use the following command:

::

  $ ./configure --with-xml2config=/usr/local/bin/xml2-config

OGC API - Processing Support (Optional) 
****************************************

In case you want to activate the support for `OGC API - Processing
<https://github.com/opengeospatial/wps-rest-binding>`__, you 
can do so by using the *--with-json*: You will then need to coy the 
``oas.cfg`` file in the same directory as your ``main.cfg``. For
instance, one may use the following command:

::

  $ ./configure --with-json=/usr/
  
Python Support (Optional) 
**************************

The ``--with-python=yes`` option is required to activate the :ref:`kernel_index` Python support, using the following command:

::

  $ ./configure --with-python=yes

This assumes that python-config is found in your ``PATH``. If not,
then you can specify the Python installation directory using the
following command (with Python installed in the ``/usr/local``
directory):

::

  $ ./configure --with-python=/usr/local


Python Version
##############

If multiple Python versions are available and you want to use a
specific one, then you can use the ``--with-pyvers`` option as shown
bellow:

::

  $ ./configure --with-pyvers=2.7


.. _js-support:

JavaScript Support (Optional) 
******************************

In order to activate the JavaScript support for ZOO-Kernel,
the ``--with-js=yes`` configure option must be specified. If you are using
a "Debian-like" GNU/Linux distribution then  dpkg will be used to
detect if the required packages are installed and you don't have to
specify anything here. The following command is only needed (assuming
that js_api.h and libmozjs.so are found in default directories):


::

  $ ./configure --with-js=yes

If you want to use a custom installation of `SpiderMonkey
<https://developer.mozilla.org/en/SpiderMonkey>`__ , or if you are not
using a Debian packaging  system, then you'll have to specify the
directory where it is installed. For  instance, if SpiderMonkey is in
``/usr/local/``, then the following command must be used:

::

  $ ./configure --with-js=/usr/local


PHP Support (Optional) 
***********************

The ``--with-php=yes`` option is required to activate the
:ref:`kernel_index` PHP support`, using the following command:

::

  $ ./configure --with-php=yes

This assumes that ``php-config`` can be found in the ``<PATH>/bin``
directory . So, supposing the your ``php-config`` can be found in
``/usr/local/bin``, then use the following command:

::

  $ ./configure --with-php=/usr/local

.. warning::
    ZOO-Kernel optional PHP support requires a local PHP Embedded installation. Read more `here <http://zoo-project.org/trac/wiki/ZooKernel/Embed/PHP>`__.


Java Support (Optional) 
************************

In order to activate the Java support for ZOO-Kernel, the
`--with-java` configure option must be specified and sets the
installation path of your Java SDK. For instance,  if Java SDK is
installed in the ``/usr/lib/jvm/java-6-sun-1.6.0.22/`` directory,
then the following command can be used:

::

  $ ./configure --with-java=/usr/lib/jvm/java-6-sun-1.6.0.22/

This assumes that the ``include/linux`` and ``jre/lib/i386/client/``
subdirectories exist in ``/usr/lib/jvm/java-6-sun-1.6.0.22/``, and
that the ``include/linux`` directory contains the ``jni.h`` headers file
and that the ``jre/lib/i386/client/`` directory contains the ``libjvm.so``
file.

.. note:: 
   You can use the `--with-java-rpath` option to produce a binary
   aware of the libjvm location.


.. note:: 
   With Mac OS X you only have to set *macos* as the value for the
   ``--with-java`` option to activate Java support. For example:

   ::

     $ ./configure --with-java=macos


Perl Support (Optional) 
************************

The ``--with-perl=yes`` option can be used for activating the
ZOO-Kernel Perl support, as follow:

::

  $ ./configure --with-perl=yes

This assumes that perl is found in your PATH. For instance, if Perl is
installed in ``/usr/local`` and ``/usr/local/bin`` is not found in
your ``PATH``, then the following command can be used (this assumes
that ``/usr/local/bin/perl`` exists):

::

  $ ./configure --with-perl=/usr/local


Orfeo Toolbox Support (Optional) 
*********************************

In order to activate the optional Orfeo Toolbox support, the
``--with-otb`` option must be used, using the following command:

::

  $ ./configure --with-otb=/path/to/your/otb/


Read more about the :ref:`kernel-orfeotoolbox`.

.. warning::
    To build the Orfeo Toolbox support you will require ITK, the
    default version of ITK is 4.5, in case you use another version,
    please make sure to use the ``--with-itk-version`` to specificy
    what is the version available on your system.

SAGA GIS Support (Optional) 
****************************


In order to activate the optional SAGA GIS support, the *--with-saga* option must be used, using the following command:

::

  $ ./configure --with-saga=/path/to/your/saga/


Read more about the :ref:`kernel-sagagis`.

.. warning::
    In case wx-config is not in your ``PATH`` please, make sure to use
    the ``--with-wx-config``  to specify its location.

Translation support (Optional)
******************************

The ZOO-Kernel is able to translate the messages it produces in different
natural languages. This requires that you download `the messages file
<https://www.transifex.com/projects/p/zoo-kernel-internationalization/>`_
translated in your language, if any. Then, for this translation
support to work, you have to generate manually the requested file on
your system. For instance for the French translation, you may use the
following command:

.. code::

    msgfmt messagespo_fr_FR.utf8.po -o /usr/share/locale/fr/LC_MESSAGES/zoo-kernel.mo

The ZOO-Kernel is also able to handle translation of
ZOO-Services. Please, refer to :ref:`this document
<service_translation>` for more details on the procedure to add new
ZOO-Service translation files.

.. warning::
    The location of the final ``.mo`` file may vary depending on your
    system setup.


Install ZOO-Services
--------------------

.. warning::
    We present here a global installation procedure for basics
    ZOO-Services, for details about automatic installation of services
    provided by :ref:`kernel-orfeotoolbox` or :ref:`kernel-sagagis`,
    please refer to there specific documentations.

Depending on the programming language used to implement the
ZOO-Services you want to install, you will need to build a
Services Provider. In the case of *C* and *Fotran*, you would create a
shared library exporting the functions corresponding to all the
ZOO-Services provided by this Services Provider. In case of *Java*,
you will need to build a Java Class. In any other programming
language, you should simply have to install the ServiceProvider and
the zcfg files.

If building a Shared library or a Java class is required, then you
should find a ``Makefile`` in the service directory which is
responsible to help you build this Services Provider. So you should
simply run the `make` command from the Service directory to generate
the required file.

Then you simply need to copy the content of the ``cgi-env`` directory
in ``cgi-bin``.

To install the ``ogr/base-vect-ops`` Services Provider, supposing that
your ``cgi-bin`` directory is ``/usr/local/lib`` use the following
commands:

.. code::

    cd zoo-project/zoo-services/ogr/base-vect-ops
    make
    cp cgi-env/*.* /usr/lib/cgi-bin

.. note::
    You may also run ``make install`` directly after ``make``.


To install the hello-py Services Provider, use the following commands:

.. code::

    cd zoo-project/zoo-services/hello-py/
    cp cgi-env/* /usr/lib/cgi-bin


Testing your installation
-------------------------

To test your installation yous should first be able to run the
following command from the ``cgi-bin`` directory:

.. code::

    ./zoo_loader.cgi "request=GetCapabilities&service=WPS"


