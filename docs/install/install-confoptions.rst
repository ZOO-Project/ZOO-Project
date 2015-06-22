.. _install-configure:

Configure options
==============

This section provides information on :ref:`kernel_index` configure options. It is recommanded to also read the :ref:`kernel_config` section for configuration technical details.

Configure options
-----------------

Here is the list of available options as returned by *./configure --help* command:

::

  --with-PACKAGE[=ARG]    Use PACKAGE [ARG=yes]
  --without-PACKAGE       Do not use PACKAGE (same as --with-PACKAGE=no)
  --with-gdal-config=FILE Specifies an alternative gdal-config file
  --with-xml2config=FILE  Specifies an alternative xml2-config file
  --with-python=PATH      Enables python support or specifies an alternative directory for python installation (disabled by default)
  --with-pyvers=NUM       Uses a specific python version
  --with-php=PATH         Enables php support or specify an alternative directory for php installation, disabled by default
  --with-perl=PATH        Enables perl support or specifies an alternative directory for perl installation, disabled by default
  --with-java=PATH        Enables java support or specifies a JDK_HOME, disabled by default
  --with-js=PATH          Enables javascript support, disabled by default
  --with-mapserver=PATH   Specifies the path for MapServer compiled source tree
  --with-itk=PATH          Specifies an alternative location for the ITK library
  --with-itk-version=VERSION          Specifies an alternative version for the ITK library     
  --with-otb=PATH         Enables optional OrfeoToolbox support
  --with-saga=PATH        Enables optional SAGA GIS support 

All the options are described in more details in the following sections.

GDAL Support (Required) 
........................................

If gdal-config program is not found in PATH, a *--with-gdal-config* option can be used to specify its location. For instance, if gdal-config lies in ``/usr/local/bin`` which is not in PATH, you may use the following command:

::

  $ ./configure --with-gdal-config=/usr/local/bin/gdal-config

XML2 Support (Required) 
........................................


If xml2-config program is not found in PATH, a *--with-xml2config* option can be used  to specify its location. For instance, if xml2-config is installed in ``/usr/local/bin`` which is not in PATH, you may use the following command:

::

  $ ./configure --with-xml2config=/usr/local/bin/xml2-config


Python Support (Optional) 
..............................................

The *--with-python* option is required to activate the :ref:`kernel_index` Python support, using the following command:

::

  $ ./configure --with-python

This assumes that python-config is found in PATH. If not, then you can specify the Python 
installation directory using the following command (with ``/usr/local`` as example python directory):

::

  $ ./configure --with-python=/usr/local


Python Version (Optional) 
..............................................

A specific version of Python can be used, with the *--with-pyvers* option as shown bellow:
::

  $ ./configure --with-pyvers=2.6


PHP Support (Optional) 
..............................................

The *--with-php* option is required to activate the :ref:`kernel_index` PHP support, using the following command:

::

  $ ./configure --with-php

This assumes that php-config is found in PATH. If not, then you can specify the PHP installation  directory, using the following command (with ``/usr/local`` as example PHP directory)

::

  $ ./configure --with-php=/usr/local

.. warning::
    ZOO-Kernel optional PHP support requires a local PHP Embedded installation. Read more `here <http://zoo-project.org/trac/wiki/ZooKernel/Embed/PHP>`__.


Perl Support (Optional) 
..............................................

The *--with-perl* option can be used for activating the ZOO-Kernel Perl support, as follow:

::

  $ ./configure --with-perl

This assumes that perl is found in PATH. For instance, if Perl is installed in /usr/local and /usr/local/bin which is not found in PATH,
then the following command can be used (this assumes that /usr/local/bin/perl exists):

::

  $ ./configure --with-perl=/usr/local


Java Support (Optional) 
..............................................

In order to activate the Java support for ZOO-Kernel, the
*--with-java* configure option must be specified and sets the installation path of your Java SDK. For instance, 
if Java SDK is installed in the ``/usr/lib/jvm/java-6-sun-1.6.0.22/`` directory,  then the following command can be used:

::

  $ ./configure --with-java=/usr/lib/jvm/java-6-sun-1.6.0.22/

This assumes that the ``include/linux`` and ``jre/lib/i386/client/`` subdirectories exist in ``/usr/lib/jvm/java-6-sun-1.6.0.22/``, that the ``include/linux`` directory contains the jni.h headers file and that the ``jre/lib/i386/client/`` directory contains the libjvm.so file.

.. note:: 
   With Mac OS X you only have to set *macos* as the value for the *--with-java* option 
   to activate Java support. For example:

   ::

     $ ./configure --with-java=macos

.. _js-support:

JavaScript Support (Optional) 
..............................................

In order to activate the JavaScript support for ZOO-Kernel,
the *--with-js* configure option must be specified. If you are using a "Debian-like" GNU/Linux distribution then 
dpkg will be used to detect if the required packages are installed and you don't have to 
specify anything here. The following command is only needed (assuming that js_api.h and libmozjs.so are found in default directories):

::

  $ ./configure --with-js 

If youwant to use a custom installation of `SpiderMonkey <https://developer.mozilla.org/en/SpiderMonkey>`__ ,or if you are not using a Debian packaging 
system, then you'll have to specify the directory where it is installed. For  instance, if SpiderMonkey is in /usr, then the following command must be used:

::

  $ ./configure --with-js=/usr


MapServer Support (Optional) 
..............................................


In order to activate the WMS, WFS and WCS output support using
MapServer, the *--with-mapserver* option must be used. The path to
``mapserver-config`` which is located in the source code of MapServer
must also be set, using the following command:

::

  $ ./configure --with-mapserver=/path/to/your/mapserver_config/


Read more about the :ref:`kernel-mapserver`.


Orfeo Toolbox Support (Optional) 
.....................................................

In order to activate the optional Orfeo Toolbox support, the *--with-otb* option must be used, using the following command:

::

  $ ./configure --with-otb=/path/to/your/otb/


Read more about the :ref:`kernel-orfeotoolbox`.


SAGA GIS Support (Optional) 
.....................................................


In order to activate the optional SAGA GIS support, the *--with-saga* option must be used, using the following command:

::

  $ ./configure --with-saga=/path/to/your/saga/


Read more about the :ref:`kernel-sagagis`.
