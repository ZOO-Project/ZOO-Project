.. _install-opensuse:

Install on OpenSUSE
===================

:ref:`kernel_index` is maintained as a package in `OpenSUSE Build Service (OBS) <https://build.opensuse.org/package/show?package=zoo-kernel&project=Application%3AGeo>`__. RPM are thus provided for all versions of OpenSUSE Linux (11.2, 11.3, 11.4, Factory).

Stable release
----------------------

Use the following instructions to install ZOO-Project latetst release on OpenSUSE distribution.

One-click installer
...................

A one-click installer is available `here <http://software.opensuse.org/search?q=zoo-kernel&baseproject=openSUSE%3A11.4&lang=en&exclude_debug=true>`__. 
For openSUSE 11.4, follow this direct `link <http://software.opensuse.org/ymp/Application:Geo/openSUSE_11.4/zoo-kernel.ymp?base=openSUSE%3A11.4&query=zoo-kernel>`__.

Yast software manager
.....................

Add the `Application:Geo <http://download.opensuse.org/repositories/Application:/Geo/>`__ repository to the software repositories and then ZOO-Kernel can then be found in Software Management using the provided search tool.

Command line (as root for openSUSE 11.4)
........................................

Install ZOO-Kernel package by yourself using the following command:

.. code-block:: guess

  zypper ar http://download.opensuse.org/repositories/Application:/Geo/openSUSE_11.4/
  zypper refresh
  zypper install zoo-kernel

Developement version
********************

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

