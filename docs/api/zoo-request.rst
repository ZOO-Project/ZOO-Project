.. _api-zoo-request:

ZOO.Request
===========

Contains convenience methods for working with ZOORequest which replace XMLHttpRequest.

Functions
---------

.. list-table::
   :widths: 30 50
   :header-rows: 1

   * - NAME
     - DESCRIPTION
   * - :ref:`GET <GET>`
     - Send an HTTP GET request.
   * - :ref:`POST <POST>`
     - Send an HTTP POST request. 
     
.. _GET:

GET	
  Send an HTTP GET request.

  *Parameters*
  
  | ``url {String}`` The URL to request.
  | ``params {Object}`` Params to add to the url

  *Returns*

  ``{String}`` Request result.
  
.. _POST:

POST	
  Send an HTTP POST request.

  *Parameters*
  
  | ``url {String}`` The URL to request.
  | ``body {String}`` The request's body to send.
  | ``headers {Object}`` A key-value object of headers to push to the request's head

  *Returns*

  ``{String}`` Request result.
