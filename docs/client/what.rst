.. _client-what:

What is ZOO-Client ?
=========================

ZOO-Client is a client-side JavaScript API which provides simple methods
for interacting with `WPS
<http://www.opengeospatial.org/standards/wps/>`__ server from web
applications. It is helpful for sending requests to any WPS compliant
server (such as :ref:`kernel_index`) and to parse the output responses
using simple JavaScript.


JavaScript
-------------------------------

ZOO-Client relies on modern JavaScript libraries and can be seamlessly
integrated in new or existing web platforms or
applications. ZOO-Client works by expanding the tags available in WPS
specific templates using values provided by a JavaScript hash or
object. It allows to build valid WPS requests and to send them to a
WPS server. It also provides functions to easily parse and reuse the
output XML responses. Read the :ref:`next section <client-howto>` to
get started.

Please, refer to the `ZOO-Client API documentation
<https://zoo-project.github.io/docs/JS_API/index.html>`__ for accessing the
up-to-date documentation.


Templates
-------------------------------

ZOO-Client uses logic-less `Mustache <http://mustache.github.io/>`__
templates for creating well-formed WPS requests. Please, refer to the
`ZOO-Client API documentation
<https://zoo-project.github.io/docs/JS_API/module-wpsPayload.html>`__ for more
details about the functions using the templates. 


GetCapabilities
..........................................................

*GetCapabilities* requests are created using the following template:

.. include:: ../../zoo-project/zoo-client/lib/tpl/payload_GetCapabilities.mustache
    :code: xml



DescribeProcess
..........................................................

*DescribeProcess* requests are created using the following template:

.. include:: ../../zoo-project/zoo-client/lib/tpl/payload_DescribeProcess.mustache
    :code: xml


Execute
..........................................................

*Execute* requests are created using a more complex template, as shown bellow:

.. include:: ../../zoo-project/zoo-client/lib/tpl/payload_Execute.mustache
    :code: xml

