.. _install-installation:

Installation on Unix/Linux
=================

For the impatient
--------------

Run the following commands from the directory where you :ref:`install-download` and extracted the ZOO Kernel source code in order to build the ``zoo_loader.cgi`` CGI program with default options.

::

   cd zoo-kernel
   autoconf  
   ./configure
   make

This should produce executables for the *zoo_loader.cgi* CGI program in the ``zoo-kernel`` directory. Copy the *zoo_loader.cgi* and *main.cfg* files to the HTTP server ``cgi`` directory and start using it.

.. warning:: 

   Edit ZOO-Kernel installation settings in the *main.cfg* file (set ``tmpPath`` and ``tmpUrl`` to fit your web server configuration).

Debian / Ubuntu
----------------------

Use the following instructions to install `ZOO-Project <http://zoo-project.org>`__ on Debian or Ubuntu distributions. 

Prerequisites
......................

Using Debian
**********************

The following command should install all the required dependancies on Debian. See the :ref:`install-prereq` section for additional information.

.. code-block:: guess

   apt-get install flex bison libfcgi-dev libxml2 libxml2-dev curl openssl autoconf apache2 python-software-properties subversion python-dev libgdal1-dev build-essential libmozjs185-dev

Using Ubuntu
**********************

On Ubuntu, use the following command first to install the required dependancies :

.. code-block:: guess

   sudo apt-get install flex bison libfcgi-dev libxml2 libxml2-dev curl openssl autoconf apache2 python-software-properties subversion libmozjs185-dev python-dev build-essential

Then add the *UbuntuGIS* repository in order to get the latest versions of libraries

.. code-block:: guess

   sudo add-apt-repository ppa:ubuntugis/ppa
   sudo apt-get update

Install the geographic library as follow:

.. code-block:: guess

   sudo apt-get install libgdal1-dev


Installation
......................

:ref:`install-download` ZOO-Project latest version from svn using the following command:

.. code-block:: guess

  svn checkout http://svn.zoo-project.org/svn/trunk zoo-project

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
   Refer to the :ref:`install-configure` section for the full list of available options

Compile ZOO-Kernel as follow:

.. code-block:: guess

  make

Copy the necessary files to the `cgi-bin` directory (as administrator user)

.. code-block:: guess

  cp main.cfg /usr/lib/cgi-bin
  cp zoo_loader.cgi /usr/lib/cgi-bin

Install ZOO ServiceProviders, for example the basic Python service (as administrator user)

.. code-block:: guess

  cp ../zoo-services/hello-py/cgi-env/*.zcfg /usr/lib/cgi-bin
  cp ../zoo-services/hello-py/*.py /usr/lib/cgi-bin/

Edit the *main.cfg* file as follow (example configuration):

.. code-block:: guess

  nano /usr/lib/cgi-bin/main.cfg
  - serverAddress = http://127.0.0.1


Test the ZOO-Kernel installation with the following requests:

.. code-block:: guess

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=GetCapabilities&Version=1.0.0

.. code-block:: guess

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=DescribeProcess&Version=1.0.0&Identifier=HelloPy

.. code-block:: guess

   http://127.0.0.1/cgi-bin/zoo_loader.cgi?ServiceProvider=&metapath=&Service=WPS&Request=Execute&Version=1.0.0&Identifier=HelloPy&DataInputs=a=myname


.. note:: 

   Such request should return well formed XML documents (OWS documents responses).

.. warning:: 

   If ZOO-Kernel returns an error please check the :ref:`kernel_config` and beware of the :ref:`install-prereq`.


OpenSUSE
----------------------

:ref:`kernel_index` is maintained as a package in `OpenSUSE Build Service (OBS) <https://build.opensuse.org/package/show?package=zoo-kernel&project=Application%3AGeo>`__. RPM are thus provided for all versions of OpenSUSE Linux (11.2, 11.3, 11.4, Factory).

Stable release
......................

Use the following instructions to install ZOO-Project latetst release on OpenSUSE distribution.

One-click installer
*******************

A one-click installer is available `here <http://software.opensuse.org/search?q=zoo-kernel&baseproject=openSUSE%3A11.4&lang=en&exclude_debug=true>`__. 
For openSUSE 11.4, follow this direct `link <http://software.opensuse.org/ymp/Application:Geo/openSUSE_11.4/zoo-kernel.ymp?base=openSUSE%3A11.4&query=zoo-kernel>`__.

Yast software manager
*********************

Add the `Application:Geo <http://download.opensuse.org/repositories/Application:/Geo/>`__ repository to the software repositories and then ZOO-Kernel can then be found in Software Management using the provided search tool.

Command line (as root for openSUSE 11.4)
****************************************

Install ZOO-Kernel package by yourself using the following command:

.. code-block:: guess

  zypper ar http://download.opensuse.org/repositories/Application:/Geo/openSUSE_11.4/
  zypper refresh
  zypper install zoo-kernel

Developement version
......................

The latest development version of ZOO-Kernel can be found in OBS under the project `home:tzotsos <https://build.opensuse.org/project/show?project=home%3Atzotsos>`__. ZOO-Kernel packages are maintained and tested there before being released to the Application:Geo repository. Installation methods are identical as for the stable version. Make sure to use `this <http://download.opensuse.org/repositories/home:/tzotsos/>`__ repository instead.

Command line (as root for openSUSE 11.4)
********************************************

Install latest ZOO-Kernel trunk version with the following command:

.. code-block:: guess

  zypper ar http://download.opensuse.org/repositories/home:/tzotsos/openSUSE_11.4/
  zypper refresh
  zypper install zoo-kernel
  zypper install zoo-kernel-grass-bridge

Note that there is the option of adding the zoo-wps-grass-bridge package. This option will automatically install grass7 (svn trunk).


CentOS
----------------------

Use the following instructions to install `ZOO-Project <http://zoo-project.org>`__ on CentOS distributions. 

Prerequisites
......................

.. code-block:: guess

  yum install apache2
  yum install build-essentials
  yum install gcc-c++
  yum install zlib-devel
  yum install libxml2-devel
  yum install bison
  yum install openssl 
  yum install python-devel
  yum install subversion


Compile then install FastCGI library from source

::

  wget http://www.fastcgi.com/dist/fcgi.tar.gz
  tar xzf fcgi-2.4.0.tar.gz 
  ./configure
  make
  make install
  echo /usr/local/lib >> /etc/ld.so.conf.d/local.conf
  ldconfig

Compile then install the autoconf tools :

::

  wget http://ftp.gnu.org/gnu/autoconf/autoconf-latest.tar.gz
  tar xzf autoconf-latest.tar.gz
  ./configure --prefix=/usr
  make 
  make install

Compile then install the flex tool :

::

  wget http://downloads.sourceforge.net/project/flex/flex/flex-2.5.35/flex-2.5.35.tar.gz?r=http%3A%2F%2Fflex.sourceforge.net%2F&ts=1292529005&use_mirror=switch
  tar xzf flex-2.5.35.tar.gz
  cd flex-2.5.35
  ./configure --prefix=/usr
  make
  make install

Using the curl provided in the CentOS distribution will produce a ZOO-Kernel unable to run any 
Service. Indeed, some segmentation faults occur when trying to run ``Execute`` requests on the ZOO-Kernel, 
compiling the ZOO-Kernel setting ``USE_GDB`` flag in the ``CFLAGS`` of your ``Makefile`` will let you run 
ZOO-Kernel from gdb and be able to get more information on what is going wrong with your ZOO-Kernel. 
Doing this we can figure out that code on `line 173 <http://zoo-project.org/trac/browser/trunk/zoo-kernel/ulinet.c#L173>`__ 
and `line 175 <http://zoo-project.org/trac/browser/trunk/zoo-kernel/ulinet.c#L175>`__ have to be commented in the 
``ulinet.c`` file to get a ZOO-Kernel working using the curl available in CentOS (curl version 7.15.5). 
If you don't apply the modification, you will get an error from a gdb session pointing 
segfault in ``Curl_cookie_clearall``.

You can optionally compile then install curl from source :

::

  wget http://curl.haxx.se/download/curl-7.21.3.tar.bz2
  tar xjf curl-7.21.3.tar.bz2
  cd curl-7.21.3
  ./configure --prefix=/usr
  make
  make install

Compile then install Python :

::
 
  wget http://www.python.org/ftp/python/2.6.6/Python-2.6.6.tar.bz2
  tar xjf Python-2.6.6.tar.bz2
  cd Python-2.6.6
  ./configure
  make
  make install

Compile then install your own GDAL library :

::

  wget http://download.osgeo.org/gdal/gdal-1.7.3.tar.gz
  tar xzf gdal-1.7.3.tar.gz
  cd gdal-1.7.3
  ./configure  # add your options here
  make
  make install

Install the Sun JAVA SDK into ``/usr/share`` then use the following command to ensure that the ``libjvm.so`` 
will be found at runtime from any context.

::

  echo /usr/share/java-1.6.0-openjdk-1.6.0.0/jre/lib/i386/client/ >> /etc/ld.so.conf.d/jvm.conf
  ldconfig
  