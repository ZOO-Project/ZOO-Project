.. _install-installation:

Installation on Unix/Linux
==========================

To build and install ZOO-Project on your Web Server you will need 4
steps :

 * build the cgic library,
 * install the ZOO-Kernel,
 * install the ZOO-Services,
 * testing your installation.

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

Install the ZOO-Kernel
----------------------


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

Here is the list of available options as returned by ``./configure --help`` command:

.. list-table:: Configure Options
   :header-rows: 1

   * - Option
     - Description
   * - ``--with-cgi-dir=PATH``
     - Specifies an alternative cgi directory path (default:
       /usr/lib/cgi-bin)
   * - ``--with-db-backend``
     - Relies on a database for storing status messages and response
       files
   * - ``--with-yaml=PATH``
     - Specifies an alternative location for the yaml library
   * - ``--with-fastcgi=PATH``
     - Specifies an alternative location for the fastcgi library
   * - ``--with-gdal-config=FILE``
     - Specifies an alternative gdal-config file
   * - ``--with-xml2config=FILE``
     - Specifies an alternative xml2-config file
   * - ``--with-python=PATH``
     - Enables python support or specifies an alternative directory
       for python installation (disabled by default)
   * - ``--with-pyvers=NUM``
     - Uses a specific python version
   * - ``--with-js=PATH`` 
     - Enables javascript support, disabled by default
   * - ``--with-php=PATH`` 
     - Enables php support or specify an alternative directory for php
       installation, disabled by default
   * - ``--with-java=PATH``
     - Enables java support or specifies a JDK_HOME, disabled by
       default
   * - ``--with-ruby=PATH``
     - To enable ruby support or specify an alternative directory for
       ruby installation, disabled by default
   * - ``--with-rvers=NUM``
     - To use a specific ruby version
   * - ``--with-perl=PATH``
     - Enables perl support or specifies an alternative directory for
       perl installation, disabled by default
   * - ``--with-mapserver=PATH``
     - Specifies the path for MapServer compiled source tree
   * - ``--with-itk=PATH`` 
     - Specifies an alternative location for the ITK library
   * - ``--with-itk-version=VERSION``
     - Specifies an alternative version for the ITK library 
   * - ``--with-otb=PATH``
     - Enables optional OrfeoToolbox support
   * - ``--with-wx-config=PATH``
     - Specifies an alternative path for the wx-config tool
   * - ``--with-saga=PATH``
     - Enables optional SAGA GIS support 


.. code ::

  --with-cgi-dir=PATH     Specifies an alternative cgi directory path (
                          default: /usr/lib/cgi-bin)
  --with-db-backend       Relies on a database for storing status messages and
                          response files
  --with-yaml=PATH        Specifies an alternative location for the yaml
                          library
  --with-fastcgi=PATH     Specifies an alternative location for the fastcgi
                          library
  --with-xml2config=FILE  Specifies an alternative xml2-config file
  --with-xsltconfig=FILE  Specifies an alternative xslt-config file
  --with-gdal-config=FILE Specifies an alternative gdal-config file
  --with-proj=PATH        Specifies an alternative location for PROJ4 setup
  --with-geosconfig=FILE  Specifies an alternative geos-config file
  --with-cgal=PATH        Specifies an alternative location for CGAL setup
  --with-mapserver=PATH   Specifies the path for MapServer compiled source
                          tree
  --with-python=PATH      To enable python support or Specifies an alternative
                          directory for python installation, disabled by
                          default
  --with-pyvers=NUM       To use a specific python version
  --with-js=PATH          Specifies --with-js=path-to-js to enable js support,
                          specify --with-js on linux debian like, js support
                          is disabled by default
  --with-php=PATH         To enable php support or specify an alternative
                          directory for php installation, disabled by default
  --with-java=PATH        To enable java support, specify a JDK_HOME, disabled
                          by default
  --with-ruby=PATH        To enable ruby support or specify an alternative
                          directory for ruby installation, disabled by default
  --with-rvers=NUM        To use a specific ruby version
  --with-perl=PATH        To enable perl support or specify an alternative
                          directory for perl installation, disabled by default
  --with-itk=PATH         Specifies an alternative location for the itk
                          library
  --with-itk-version=VERSION
                          Specifies an alternative version for the itk library
  --with-otb=PATH         Specifies an alternative location for the otb
                          library
  --with-wx-config=PATH   Specifies an alternative path for the wx-config tool
  --with-saga=PATH        Specifies an alternative location for the SAGA-GIS
                          library




::

  --with-cgi-dir=PATH     Specifies an alternative cgi directory path (default: /usr/lib/cgi-bin)
  --with-db-backend       Relies on a database for storing status messages and response files
  --with-yaml=PATH        Specifies an alternative location for the yaml library
  --with-fastcgi=PATH     Specifies an alternative location for the fastcgi library
  --with-gdal-config=FILE Specifies an alternative gdal-config file
  --with-xml2config=FILE  Specifies an alternative xml2-config file
  --with-python=PATH      Enables python support or specifies an alternative directory for python installation (disabled by default)
  --with-pyvers=NUM       Uses a specific python version
  --with-js=PATH          Enables javascript support, disabled by default
  --with-php=PATH         Enables php support or specify an alternative directory for php installation, disabled by default
  --with-java=PATH        Enables java support or specifies a JDK_HOME, disabled by default
  --with-ruby=PATH        To enable ruby support or specify an alternative directory for ruby installation, disabled by default
  --with-rvers=NUM        To use a specific ruby version
  --with-perl=PATH        Enables perl support or specifies an alternative directory for perl installation, disabled by default
  --with-mapserver=PATH   Specifies the path for MapServer compiled source tree
  --with-itk=PATH          Specifies an alternative location for the ITK library
  --with-itk-version=VERSION          Specifies an alternative version for the ITK library     
  --with-otb=PATH         Enables optional OrfeoToolbox support
  --with-wx-config=PATH   Specifies an alternative path for the wx-config tool
  --with-saga=PATH        Enables optional SAGA GIS support 

All the options are described in more details in the following sections.

GDAL Support (Required) 
........................

If gdal-config program is not found in your ``PATH``, a
``--with-gdal-config`` option can be used to specify its location. For
instance, if ``gdal-config`` lies in ``/usr/local/bin`` which is not in
your PATH, you may use the following command:

::

  $ ./configure --with-gdal-config=/usr/local/bin/gdal-config

XML2 Support (Required) 
........................

If xml2-config program is not found in PATH, a *--with-xml2config* option can be used  to specify its location. For instance, if xml2-config is installed in ``/usr/local/bin`` which is not in PATH, you may use the following command:

::

  $ ./configure --with-xml2config=/usr/local/bin/xml2-config

Use a Database Backend (Optional) 
..................................

If you want to share the ongoing informations of running services
between various ZOO-Kernel instances then you should activate this
option. This way, both the *GetStatus*, *GetResult* and *Dismiss*
requests can be run from any host accessing the same database.

.. note::
    By now, the ZOO-Kernel is not able to handle correctly the
    *Dismiss* request from any host. Nevertheless, it will provide
    valid response from any host, but only the host which is really
    handling the service will be able to stop it and remove all the
    linked files.



Python Support (Optional) 
..............................................

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
**************

If multiple Python versions are available and you want to use a
specific one, then you can use the ``--with-pyvers`` option as shown
bellow:

::

  $ ./configure --with-pyvers=2.7


PHP Support (Optional) 
..............................................

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


Perl Support (Optional) 
..............................................

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


Java Support (Optional) 
..............................................

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
   With Mac OS X you only have to set *macos* as the value for the
   ``--with-java`` option to activate Java support. For example:

   ::

     $ ./configure --with-java=macos

.. _js-support:

JavaScript Support (Optional) 
..............................................

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


MapServer Support (Optional) 
..............................................


In order to activate the WMS, WFS and WCS output support using
MapServer, the ``--with-mapserver`` option must be used. The path to
``mapserver-config`` which is located in the source code of MapServer
must also be set, using the following command:

::

  $ ./configure --with-mapserver=/path/to/your/mapserver_config/


Read more about the :ref:`kernel-mapserver`.


Orfeo Toolbox Support (Optional) 
.....................................................

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
.....................................................


In order to activate the optional SAGA GIS support, the *--with-saga* option must be used, using the following command:

::

  $ ./configure --with-saga=/path/to/your/saga/


Read more about the :ref:`kernel-sagagis`.

.. warning::
    In case wx-config is not in your ``PATH`` please, make sure to use
    the ``--with-wx-config``  to specify its location.

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


