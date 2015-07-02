.. _api-zoo-format-wps:

ZOO.Format.WPS
==============

Read/Write WPS.

Inherits from

- :ref:`ZOO.Format <api-zoo-format>`

Functions and Properties	
------------------------

.. list-table::
   :widths: 30 50
   :header-rows: 1

   * - NAME
     - DESCRIPTION
   * - :ref:`schemaLocation <schemaLocation>`
     - {String} Schema location for a particular minor version.
   * - :ref:`namespaces <namespaces>`
     - {Object} Mapping of namespace aliases to namespace URIs.
   * - :ref:`read <read>`
     - 
   * - :ref:`parseExecuteResponse <parseExecuteResponse>`
     -   
   * - :ref:`parseData <parseData>`
     - Object containing methods to analyse data response.
   * - :ref:`parseData.complexdata <parseData.complexdata>`
     - Given an Object representing the WPS complex data response.   
   * - :ref:`parseData.literaldata <parseData.literaldata>`
     - Given an Object representing the WPS literal data response.
   * - :ref:`parseData.reference <parseData.reference>`
     - Given an Object representing the WPS reference response. 
       
.. _schemaLocation:   

schemaLocation	
  ``{String}`` Schema location for a particular minor version.
  
.. _namespaces:   
  
namespaces	
  ``{Object}`` Mapping of namespace aliases to namespace URIs.
  
.. _read:     
  
read
  ::
  
    read:function(data)

  *Parameters*
  
  ``data {String}`` A WPS xml document
  
  *Returns*

  ``{Object}`` Execute response.

.. _parseExecuteResponse:     

parseExecuteResponse	
  ::
  
    parseExecuteResponse: function(node)

  *Parameters*
  
  ``node {E4XElement}`` A WPS ExecuteResponse document

  *Returns*

  ``{Object}`` Execute response.

.. _parseData:     

parseData	
  Object containing methods to analyse data response.
  
.. _parseData.complexdata:       
  
parseData.complexdata	
  Given an Object representing the WPS complex data response.

  *Parameters*
  
  ``node {E4XElement}`` A WPS node.
  
  *Returns*

  ``{Object}`` A WPS complex data response.  
  
.. _parseData.literaldata:         
  
parseData.literaldata	
  Given an Object representing the WPS literal data response.\
  
  *Parameters*
  
  ``node {E4XElement}`` A WPS node.
  
  *Returns*

  ``{Object}`` A WPS literal data response.  
  
.. _parseData.reference:         
  
parseData.reference	
  Given an Object representing the WPS reference response.

  *Parameters*
  
  ``node {E4XElement}`` A WPS node.
  
  *Returns*

  ``{Object}`` A WPS reference response.
