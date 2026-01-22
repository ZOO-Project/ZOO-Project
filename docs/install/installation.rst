.. _install-installation:

Installation on Unix/Linux
==========================

To build and install ZOO-Project on your Web Server follow these 4 steps:

.. contents:: 
    :local:
    :depth: 1
    :backlinks: top


Build the cgic Library
----------------------

Navigate to the `thirds/cgic206` directory and run:

::

   make

The cgic library originaly comes from `http://www.boutell.com/cgic <http://www.boutell.com/cgic>`_.

.. warning:: 

   If you're building on a 64-bit platform and your fcgi library is not in ``/usr/lib64``, you may need to manually edit the ``Makefile``.

Install the ZOO-Kernel
----------------------


For a quick installation
........................

Navigate to the directory where you :ref:`install-download` the ZOO-Kernel source, then run:

::

   cd zoo-project/zoo-kernel
   autoconf  
   ./configure
   make
   sudo make install

This builds the ``zoo_loader.cgi`` CGI binary (usually installed in ``/usr/lib/cgi-bin/``) and the ``libzoo_service`` shared library (usually in ``/usr/local/lib``).

.. warning:: 

   Update the ``main.cfg`` file after installation. Set the ``tmpPath`` and ``tmpUrl`` values according to your web server setup.


Configure options
.................

This section provides information on :ref:`kernel_index` configure options. It is recommanded to also read the :ref:`kernel_config` section for configuration technical details.


Here is the list of available options in the same order as returned by
``./configure --help`` command:

.. contents:: 
    :local:
    :depth: 2
    :backlinks: top

**Specify a custom CGI directory**
**********************************

If your web server uses a different CGI path:


.. code::

    ./configure --with-cgi-dir=/Lbrary/WebServer/CGI-Executables

This will install ``zoo_loader.cgi`` in the specified directory after ``make install``.

**Custom main.cfg location (Optional)**
**************************************

To place your ``main.cfg`` in a different directory (e.g., ``/etc/zoo-project``):

.. code::

    ./configure --with-etc-dir=yes --sysconfdir=/etc/zoo-project


.. _zoo_install_db_backend:

Use a Database Backend (Optional)
*********************************

To enable shared state across multiple ZOO-Kernel instances:

.. code::

    ./configure --with-db-backend

This enables support for *GetStatus*, *GetResult*, and *Dismiss* from multiple hosts accessing the same DB.

.. note::

    Only the host running the service can actually stop and clean it up when a *Dismiss* request is made.

To set up the database, create it and load the schema:

.. code::
    createdb zoo_project
    psql zoo_project -f zoo-project/zoo-kernel/sql/schema.sql

If using a custom schema name, uncomment lines 33 and 34 in `schema.sql <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel/sql/schema.sql>`_ accordingly.

.. _zoo_create_metadb:


Metadata Database (Optional)
****************************


To enable metadata storage in PostgreSQL:

.. code::
    ./configure --with-metadb=yes

Create the database and load the metadata schema:

.. code::

    createdb zoo_metadb
    psql zoo_metadb -f zoo-project/zoo-kernel/sql/zoo_collectiondb.sql

To import `.zcfg` metadata directly:


.. code::

    cd thirds/zcfg2sql
    make
    ./zcfg2sql /path/to/service.zcfg | psql zoo_metadb
  
Or create a SQL file:

.. code::

    ./zcfg2sql /path/to/service.zcfg > service.sql


See :doc:`optional_features` for details on enabling additional support like GDAL, Python, Java, and other extensions.


Install ZOO-Services
--------------------

.. warning::
    We present here a global installation procedure for basics
    ZOO-Services, for details about automatic installation of services
    provided by :ref:`kernel-orfeotoolbox` or :ref:`kernel-sagagis`,
    please refer to there specific documentations.

Depending on the programming language used to implement the
ZOO-Services you want to install, you will need to build a
Services Provider. In the case of *C* and *Fortran*, you would create a
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

Then, copy the contents of the ``cgi-env`` directory to ``cgi-bin``.

To install the ``ogr/base-vect-ops`` Services Provider, supposing that
your ``cgi-bin`` directory is ``/usr/local/lib`` use the following
commands:

.. code::

    cd zoo-project/zoo-services/ogr/base-vect-ops
    make
    cp cgi-env/*.* /usr/lib/cgi-bin

.. note::
    You may also run ``make install`` directly after ``make``.


To install the `hello-py` Services Provider:

.. code::

    cd zoo-project/zoo-services/hello-py/
    cp cgi-env/* /usr/lib/cgi-bin


Testing your installation
-------------------------

To test your installation, run the following command from the ``cgi-bin`` directory:

.. code::

    ./zoo_loader.cgi "request=GetCapabilities&service=WPS"

You should receive an XML response describing available services, which confirms that your ZOO-Kernel and services are properly installed.
