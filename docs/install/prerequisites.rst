.. _install-prereq:

Prerequisites
=============

Mandatory
-----------------

The following libraries are required to install :ref:`kernel_index`. Please make sure they are available on your system before anything else.

- autoconf (`http://www.gnu.org/software/autoconf/
  <http://www.gnu.org/software/autoconf/>`_)
- gettext (`https://www.gnu.org/software/gettext/
  <https://www.gnu.org/software/gettext/>`_ )
- cURL (`http://curl.haxx.se <http://curl.haxx.se>`_ )
- FastCGI (`http://www.fastcgi.com <http://www.fastcgi.com>`_ )
- Flex & Bison (`http://flex.sourceforge.net/
  <http://flex.sourceforge.net/>`_ |
  `http://www.gnu.org/software/bison/
  <http://www.gnu.org/software/bison/>`_ )
- libxml2 (  http://xmlsoft.org )
- OpenSSL (  http://www.openssl.org )
- GDAL (http://gdal.org/) 

 .. warning::
    It is mandatory to install every library listed above before
    compiling and installing ZOO-Kernel

Optional
-----------------

You may also consider the following optional libraries:

- MapServer (for ZOO-Kernel optional WMS, WFS and WCS support)
  ( http://mapserver.org )
- Python (  http://www.python.org )
- PHP Embedded (for ZOO-Kernel optional PHP support)
  ( http://www.php.net )
- Java SDK (for ZOO-Kernel optional Java support) (
  http://java.sun.com )
- SpiderMonkey (for ZOO-Kernel optional Javascript support) (
  http://www.mozilla.org/js/spidermonkey/ )
- SAGA GIS (for ZOO-Kernel optional SAGA support) (
  http://www.saga-gis.org/en/index.html/ )
- OrfeoToolbox (for ZOO-Kernel optional OTB support) (
  https://www.orfeo-toolbox.org/ )
- GRASS GIS (for using it through WPSGrassBridge) (
  http://grass.osgeo.org )
- PostgreSQL support activated in GDAL to :ref:`zoo_install_db_backend`
