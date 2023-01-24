.. _install-debian:

Installation on Debian / Ubuntu
===============================

Use the following instructions to install `ZOO-Project <http://zoo-project.org>`__ on Debian or Ubuntu distributions. 

Prerequisites
-------------

Using Debian
......................

The following command should install all the required dependancies on Debian. See the :ref:`install-prereq` section for additional information.

.. code-block:: guess

   apt-get install flex bison libfcgi-dev libxml2 libxml2-dev curl openssl autoconf apache2 python-software-properties subversion python-dev libgdal1-dev build-essential libmozjs185-dev libxslt1-dev

Using Ubuntu
......................

On Ubuntu, use the following command first to install the required dependancies :

.. code-block:: guess

   sudo apt-get install flex bison libfcgi-dev libxml2 libxml2-dev curl openssl autoconf apache2 python-software-properties subversion libmozjs185-dev python-dev build-essential libxslt1-dev

Then add the *UbuntuGIS* repository in order to get the latest versions of libraries

.. code-block:: guess

   sudo add-apt-repository ppa:ubuntugis/ppa
   sudo apt-get update

Install the geographic library as follow:

.. code-block:: guess

   sudo apt-get install libgdal1-dev


Installation
------------

:ref:`install-download` ZOO-Project latest version from git using the following command:

.. code-block:: guess

  git clone https://github.com/ZOO-Project/ZOO-Project.git zoo-project

Install the *cgic* library from packages using the following command:

.. code-block:: guess

  cd zoo-project/thirds/cgic206/
  make

Head to the :ref:`kernel_index` directory

.. code-block:: guess

  cd ../../zoo-project/zoo-kernel/

Create a configure file as follow:

.. code-block:: guess

  autoconf

Run configure with the desired options, for example with the following command:

.. code-block:: guess

  ./configure --with-js --with-python

.. note::
   Refer to the :ref:`install-installation` section for the full list of available options

Compile ZOO-Kernel as follow:

.. code-block:: guess

  make

Install the ``libzoo_service.so.1.5`` by using the following command:

.. code-block:: guess

      sudo make install


Copy the necessary files to the `cgi-bin` directory (as administrator
user):

.. code-block:: guess

  cp main.cfg /usr/lib/cgi-bin
  cp zoo_loader.cgi /usr/lib/cgi-bin

Install ZOO ServiceProviders, for example the basic Python service (as administrator user)

.. code-block:: guess

  cp ../zoo-services/hello-py/cgi-env/*.zcfg /usr/lib/cgi-bin
  cp ../zoo-services/hello-py/cgi-env/*.py /usr/lib/cgi-bin/

Edit the *main.cfg* file as follow (example configuration):

.. code-block:: guess

  nano /usr/lib/cgi-bin/main.cfg
  - serverAddress = http://127.0.0.1


Test the ZOO-Kernel installation with the following requests:

.. code-block::

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=GetCapabilities&Version=1.0.0

.. code-block::

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=DescribeProcess&Version=1.0.0&Identifier=HelloPy

.. code-block::

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=Execute&Version=1.0.0&Identifier=HelloPy&DataInputs=a=myname


.. note:: 

   Such request should return well formed XML documents (OWS documents responses).

.. warning:: 

   The URLs provided here suppose that you have previously setup a web
   server and defined cgi-bin as a location where you can run cgi
   application.

.. warning:: 

   If ZOO-Kernel returns an error please check the :ref:`kernel_config` and beware of the :ref:`install-prereq`.

