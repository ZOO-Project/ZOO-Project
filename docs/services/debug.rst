.. _services-debug:

Debugging ZOO Services
=========================

Several methods can be used in order to debug :ref:`services_index`. The most common solutions are web or command line.

Web
----

Any problem can be checked in the Apache server log file when using http WPS requests.

On Unix, the log files is usually located in ``/var/log/apache2`` and
the relevant one is named *error_log*. A simple way to read this file is to use the ``tail`` command,
as it allows to see the file updates for each request ::

  tail -f /var/log/apache2/error_log

If the log is not clear enough, you still have the possibility to add
more debug information to your source code, writing to standard errors.

Python
********
Using Python, you can for example do this:

.. code-block:: python

  import sys
  
  #add this line when you want see an own message
  sys.stderr.write("My message")

.. _web_javascript:

Javascript
************

Using JavaScript, you can use ``alert`` to print a string to standard error, for example:

.. code-block:: javascript

  // add this line when you want to see own message
  alert('My message')
  // you can debug value of inputs, outputs or conf
  alert(inputs["S"]["value"])

.. note:: If you try to pass an object it will only return ``[object Object]``

Command line
--------------

:ref:`kernel_index` (*zoo_loader.cgi*) can also be used from command line. This is really useful for debugging services in a deeper way, for example:.

.. code-block:: bash

  # in order to use it you have to copy test_service.py and HelloPy.zcfg from
  # the example services
  ./zoo_loader.cgi "service=wps&version=1.0.0&request=execute&identifier=HelloPy&datainputs=a=your name&responsedocument=Result"

Working this way you can use the standard debug system of the actual
programming language used to develop your service.

In case you should simulate POST requests, you can use the following
command to tell the ZOO-Kernel to use the file ``/tmp/req.xml`` as the
input XML request:

.. code-block:: guess

    # Define required environment settings
    export REQUEST_METHOD=POST
    export CONTENT_TYPE=text/xml
    # Run the request stored in a file
    ./zoo_loader.cgi < /tmp/req.xml


GDB
*****
From command line you can use also the command line tool `GDB <http://www.gnu.org/software/gdb/>`_
to debug ``zoo_loader.cgi``, you have to run:

.. code-block:: bash

  # launch zoo_loader.cgi from gdb
  gdb zoo_loader.cgi
  # now run your request
  run "service=wps&version=1.0.0&request=execute&identifier=HelloPy&datainputs=a=your name&responsedocument=Result"

.. note::
    You can use the same parameter used before to simulate POST
    requests when running from gdb.

If nothing helped, you can ask help at the `ZOO mailing list
<http://lists.osgeo.org/cgi-bin/mailman/listinfo/zoo-discuss>`_
copying the result of the command.

Python
**********
For Python, you can use ``pdb``, more info at http://docs.python.org/2/library/pdb.html

.. code-block:: python

  import pdb
  
  # add this line when you want investigate your code in more detail
  pdb.set_trace()

Javascript
************

You can use ``alert`` also to print in the console, more info in the :ref:`web_javascript` web section
