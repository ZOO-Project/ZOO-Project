.. _install-download:

Download
=============

Several ways to download the `ZOO-Project <http://zoo-project.org>`_ source code are available and explained in this section.

.. warning::
    The ZOO-Project svn is the place where developement
    happens. Checking out svn is the best way to be always up-to-date.


ZOO-Project releases archives
-------------------------------

Each new `ZOO-Project <http://zoo-project.org>`_ major release are
available on the project official website as .zip and .tar.bz2
archives. Head to the `Downloads
<http://zoo-project.org/site/Downloads>`_ section to get the latest or
older ZOO-Project releases. 

.. warning::
    Don't use older versions of ZOO-Project if you want to use new
    features and avoid older code issues. Prefer svn or github
    instead.



ZOO-Project SVN
-------------------------------

.. _svn:

Download the `latest <http://zoo-project.org/trac/browser/trunk>`_ `ZOO-Project <http://zoo-project.org>`_  source code using the following *svn* command:

::

  svn checkout http://svn.zoo-project.org/svn/trunk zoo-src

Registered ZOO-Project developers would prefer the following:

::

  sed "s:\[tunnels\]:\[tunnels\]\nzoosvn = /usr/bin/ssh -p 1046:g" -i ~/.subversion/config
  svn co svn+zoosvn://svn.zoo-project.org/var/svn/repos/trunk zoo-src
  
.. note::
    The ZOO-Project svn server listens on the 1046 (1024+22) port
    (instead of 22 by default), so please use a specific tunnel to
    access the svn server, as shown in the command above.

ZOO-Project Github
-------------------------------

The ZOO-Project svn is mirrored in this Github `repository <https://github.com/kalxas/zoo-project/>`_ in case you would like to fork it.
