.. _install-debian:

Installation on Debian / Ubuntu
===============================

Use the following instructions to install `ZOO-Project <http://zoo-project.org>`__ on Debian or Ubuntu distributions. 

Prerequisites
-------------

Using Debian
.............

The following command should install all the required dependancies on Debian. See the :ref:`install-prereq` section for additional information.

.. code-block:: bash

   sudo apt-get install flex bison libfcgi-dev libxml2 libxml2-dev curl openssl autoconf apache2 python-software-properties subversion python-dev libgdal1-dev build-essential libmozjs185-dev libxslt1-dev

Using Ubuntu
.............

On Ubuntu, use the following command first to install the required dependancies :

.. code-block:: bash

   sudo apt-get install flex bison libfcgi-dev libxml2 libxml2-dev curl openssl autoconf apache2 python-software-properties subversion libmozjs185-dev python-dev build-essential libxslt1-dev

Then add the *UbuntuGIS* repository to get the latest versions of geographic libraries:

.. code-block:: bash

   sudo add-apt-repository ppa:ubuntugis/ppa
   sudo apt-get update

Install the GDAL library:

.. code-block:: bash

   sudo apt-get install libgdal1-dev


Installation
------------

:ref:`install-download` Download the latest version of ZOO-Project from Git:

.. code-block:: bash

  git clone https://github.com/ZOO-Project/ZOO-Project.git zoo-project

Build and install the `cgic` library:

.. code-block:: bash

  cd zoo-project/thirds/cgic206/
  make

Navigate to the ZOO-Kernel :ref:`kernel_index` directory:

.. code-block:: bash

  cd ../../zoo-project/zoo-kernel/

Generate the configure script:

.. code-block:: bash

  autoconf

Run the configure script with the desired options:

.. code-block:: bash

  ./configure --with-js --with-python

.. note::
   Refer to the :ref:`install-installation` section for a complete list of available options

Compile ZOO-Kernel:

.. code-block:: bash

  make

Install the compiled library:

.. code-block:: bash

      sudo make install

Deployment
----------

Copy necessary files to the `cgi-bin` directory (requires administrative privileges):

.. code-block:: bash

  sudo cp main.cfg /usr/lib/cgi-bin
  sudo cp zoo_loader.cgi /usr/lib/cgi-bin

Install example ZOO ServiceProviders (basic Python services):

.. code-block:: bash

  sudo cp ../zoo-services/hello-py/cgi-env/*.zcfg /usr/lib/cgi-bin
  sudo cp ../zoo-services/hello-py/cgi-env/*.py /usr/lib/cgi-bin/

Edit the `main.cfg` file for configuration:

.. code-block:: bash

  sudo nano /usr/lib/cgi-bin/main.cfg

Example modification:

::

   serverAddress = http://127.0.0.1

Testing
-------

Test the ZOO-Kernel installation by accessing the following URLs in your browser:

**GetCapabilities**

::

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=GetCapabilities&Version=1.0.0

**DescribeProcess**

::

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=DescribeProcess&Version=1.0.0&Identifier=HelloPy

**Execute**

::

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=Execute&Version=1.0.0&Identifier=HelloPy&DataInputs=a=myname


.. note:: 

   These requests should return valid XML documents (OWS-compliant responses).

.. warning:: 

   Ensure a web server is properly configured to run CGI scripts under the `/usr/lib/cgi-bin` path.

.. warning:: 

  If ZOO-Kernel returns an error, review the :ref:`kernel_config` section and verify all :ref:`install-prereq` dependencies are correctly installed.

