.. _install-opensuse:

Install on OpenSUSE
===================

ZOO-Project's :ref:`kernel_index` can be installed on recent OpenSUSE distributions either via pre-built packages from the OpenSUSE Build Service (OBS) or by compiling from source.

.. note::
   These instructions are updated for OpenSUSE Leap 15.4+ or Tumbleweed. Older instructions for 11.4 releases are obsolete.

Stable release
---------------

To install the latest stable version of ZOO-Kernel on OpenSUSE:

Via YaST Software Manager
.........................

1. Open YaST.
2. Navigate to *Software > Software Repositories* and add the following repository:

    - Repository Name: `Application:Geo`
    - URL: `http://download.opensuse.org/repositories/Application:/Geo/openSUSE_Leap_15.4/` (replace version if needed)

3. Refresh repositories and search for `zoo-kernel` in the Software Management tool.


Via Command Line
................

.. code-block:: bash

  zypper ar http://download.opensuse.org/repositories/Application:/Geo/openSUSE_Leap_15.4/ Application:Geo
  zypper refresh
  zypper install zoo-kernel

.. note::

   You can check the latest available versions here: https://software.opensuse.org/package/zoo-kernel

Developement version
********************

For the most recent development version of ZOO-Kernel, use the testing OBS repository maintained by developers:

.. warning::

   Development versions may be unstable and should only be used for testing purposes.


Via Command Line
****************

Install latest ZOO-Kernel trunk version with the following command:

.. code-block:: bash

  zypper ar http://download.opensuse.org/repositories/home:/tzotsos/openSUSE_Leap_15.4/ ZOO-Dev
  zypper refresh
  zypper install zoo-kernel
  zypper install zoo-kernel-grass-bridge

.. note::

   The `zoo-kernel-grass-bridge` package requires GRASS GIS (GRASS7) and enables integration between ZOO and GRASS.