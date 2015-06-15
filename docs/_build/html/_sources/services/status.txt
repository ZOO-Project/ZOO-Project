.. _services-status:

ZOO Status Service
===============================

The ZOO-Status Service is a `ZOO-Project <http://zoo-project.org>`__
utility allowing to get the status of a running WPS Service.

Description
-----------------------------

It returns the stage of completion of the ongoing Service in percentage
(%). The ZOO-Status Service is usefull to monitor :ref:`services_index`. It can
also be used to animate WPS progress bars from client-side applications.

Installation
-----------------------------

To install the ``ZOO Status Service`` you have to move in 
``/path/to/zoo/source/zoo-services/utils/status/`` and compile the source
running the ``make`` command.
If no errors are returned during compilation you can copy the content of ``cgi-env``
to ``/usr/lib/cgi-bin/`` or where you have your ``zoo_loader.cgi`` working with this
command (you need administration right):

::

   cp /path/to/zoo/source/zoo-services/utils/status/cgi-env/*{zcfg,zo,py} /usr/lib/cgi-bin

With this command you copy the code to permit to ``ZOO Status Service`` and some
example processes about how it works.

Now you have to add these two lines to ``main.cfg`` :

::

  rewriteUrl=call
  dataPath=/var/www/data

Here you define the path where the service is able to find the xsl file, specified in the dataPath
parameter. You also tell the ZOO Kernel that you want to use the `rewriteUrl <../kernel/install-debian.html#rewrite-rule-configuration>`_.

The last operation is to copy the ``updateStatus.xsl`` to ``dataPath`` directory as follow:

::

   cp /path/to/zoo/source/zoo-services/utils/status/cgi-env/*{xsl} /var/www/data
