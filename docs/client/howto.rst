.. _client-howto:

Using ZOO-Client
==================

This section will help you to get started using :ref:`ZOO-Client <client-what>`.

Prerequisites
----------------------

ZOO-Client is based on the following Javascript libraries

- jQuery (`http://www.jquery.com <http://www.jquery.com>`_)
- x2js (`https://code.google.com/p/x2js <https://code.google.com/p/x2js/>`_)
- Require.js (`http://requirejs.org <http://requirejs.org/>`_ )
- Hogan.js (`http://twitter.github.io/hogan.js <http://twitter.github.io/hogan.js>`_ )
- query-string (`https://github.com/sindresorhus/query-string <https://github.com/sindresorhus/query-string/>`_ )

.. warning::

     `Node.js <http://nodejs.org/>`__ is also required on your system
     for compiling ZOO-Client templates. 

Download
-----------------------

If you did not :ref:`download <install-download>` the ZOO-Project
source code already, please proceed to a svn checkout with the
following command:

::

  svn checkout https://github.com/ZOO-Project/ZOO-Project/trunk/zoo-project/zoo-client


.. warning::
   You do not necessarily need to :ref:`install <install-installation>` the ZOO-Project server for using ZOO-Client. The corresponding svn `directory <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-client>`__ is needed only.


Compiling ZOO-Client templates
------------------------------

In order to work with ZOO-Client, you will first need to compile the
provided `Mustache <http://mustache.github.io/>`_ templates using
`Node.js <http://nodejs.org/>`__. The ZOO-Client templates are located
in the ``/zoo-project/zoo-client/lib/tpl`` directory downloaded from
svn.

* Install Node.js (see related `documentation <https://github.com/joyent/node/wiki/Installing-Node.js-via-package-manager>`__.)
* Install Hogan, the JavaScript templating engine, using the following command:

  ::
 
     sudo npm install hogan


*  Use Hulk (Hogan's command line utility) for compiling the tempaltes
   using the following command:

   ::
 
     hulk zoo-client/lib/tpl/*mustache > \ zoo-client/lib/js/wps-client/payloads.js

.. warning:: Using different versions of Hogan to compile and to use in a web application may lead to compatibility issue.

Everything is now ready to work with :ref:`ZOO-Client
<client-what>`. Read the :ref:`next section <client-example>` for an
example JavaScript application. 

Building ZOO-Client API documentation
-------------------------------------

You may also build the ZOO-Client API documentation using `jsDoc
<http://usejsdoc.org/>`__, with the following command:

::

    npm install jsdoc
    ~/node_modules/.bin/jsdoc zoo-client/lib/js/wps-client/* -p

This will build HTML documentation in a new directory named ``/out`` in
your working directory. 

.. note:: 
    Building the ZOO-Client API documentation is optional, please
    refer to `the up-to-date ZOO-Client API Documentation
    <https://zoo-project.github.io/docs/JS_API/index.html>`__ for the current
    API version. 

