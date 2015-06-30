.. _install-centos:

Installation on CentOS
======================

Use the following instructions to install `ZOO-Project <http://zoo-project.org>`__ on CentOS distributions. 

Prerequisites
----------------------

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


Installation
----------------------

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
  
