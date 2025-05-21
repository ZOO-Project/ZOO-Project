Optional and Required Build Features
=====================================

This section provides instructions on enabling optional and required components during the build process using the `./configure` script.


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

Logs on console  (Optional)
************************

If needed (typically in case of Docker deployment), it is possible to force the logs
to be written on the console (stderr) :

::

  $ ./configure --with-log-console=yes

.. warning::
    Logging all services executions on the console can be tricky to troubleshoot in case
    of parallel executions. A good solution is to log the job id in the code of the service
    depending the language chosen.


