.. _api-zoo-process:

ZOO.Process
===========

Used to query OGC WPS process defined by its URL and its identifier.  Useful for chaining localhost process.

Properties and Functions	
------------------------

.. list-table::
   :widths: 15 50
   :header-rows: 1

   * - NAME
     - DESCRIPTION   
   * - :ref:`schemaLocation <schemaLocation>`
     - {String} Schema location for a particular minor version. 
   * - :ref:`namespaces <namespaces>`
     - {Object} Mapping of namespace aliases to namespace URIs.
   * - :ref:`url <url>`
     - {String} The OGC's Web PRocessing Service URL, default is http://localhost/zoo.
   * - :ref:`identifier <identifier>`
     - {String} Process identifier in the OGC's Web Processing Service. 
   * - :ref:`ZOO.Process <ZOO.Process>`
     - Create a new Process 
   * - :ref:`Execute <Execute>`
     - Query the OGC's Web PRocessing Servcie to Execute the process.
   * - :ref:`buildInput <buildInput>`
     - Object containing methods to build WPS inputs. 
   * - :ref:`buildInput.complex <buildInput.complex>`
     - Given an E4XElement representing the WPS complex data input.   
   * - :ref:`buildInput.reference <buildInput.reference>`
     - Given an E4XElement representing the WPS reference input.
   * - :ref:`buildInput.literal <buildInput.literal>`
     - Given an E4XElement representing the WPS literal data input.
   * - :ref:`buildDataInputsNode <buildDataInputsNode>`
     - Method to build the WPS DataInputs element.     
     
.. _schemaLocation: 

schemaLocation	
  ``{String}`` Schema location for a particular minor version.
  
.. _namespaces:   
  
namespaces	
  ``{Object}`` Mapping of namespace aliases to namespace URIs.
  
.. _url:     
  
url	
  ``{String}`` The OGC's Web PRocessing Service URL, default is http://localhost/zoo.
  
.. _identifier:       
  
identifier	
  ``{String}`` Process identifier in the OGC's Web Processing Service.
  
.. _ZOO.Process:         
  
ZOO.Process	
  Create a new Process

  *Parameters*
  
  | ``url {String}`` The OGC's Web Processing Service URL.
  | ``identifier {String}`` The process identifier in the OGC's Web Processing Service.  

.. _Execute:           
  
Execute	
  ::
  
    Execute: function(inputs)

  Query the OGC's Web PRocessing Servcie to Execute the process.

  *Parameters*
  
  ``inputs {Object}``
  
  *Returns*

  ``{String}`` The OGC's Web processing Service XML response.  The result needs to be interpreted. 
  
.. _buildInput:             
  
buildInput	
  Object containing methods to build WPS inputs.
  
.. _buildInput.complex:               
  
buildInput.complex	
  Given an E4XElement representing the WPS complex data input.

  *Parameters*
  
  | ``identifier {String}`` the input indetifier
  | ``data {Object}`` A WPS complex data input.

  *Returns*

  ``{E4XElement}`` A WPS Input node.
  
.. _buildInput.reference:                 
  
buildInput.reference	
  Given an E4XElement representing the WPS reference input.

  *Parameters*
  
  | ``identifier {String}`` the input indetifier
  | ``data {Object}`` A WPS reference input.

  *Returns*

  ``{E4XElement}`` A WPS Input node.  
  
.. _buildInput.literal:                   
  
buildInput.literal	
  Given an E4XElement representing the WPS literal data input.

  *Parameters*
  
  | ``identifier {String}`` the input indetifier
  | ``data {Object}`` A WPS literal data input.

  *Returns*

  ``{E4XElement}`` The WPS Input node.  
  
.. _buildDataInputsNode:                     
  
buildDataInputsNode	
  ::
  
    buildDataInputsNode:function(inputs)

  Method to build the WPS DataInputs element.

  *Parameters*
  
  ``inputs {Object}``
  
  *Returns*

  ``{E4XElement}`` The WPS DataInputs node for Execute query.
