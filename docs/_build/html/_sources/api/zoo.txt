.. _api-zoo:

ZOO
===

The following constants and functions are available for the ZOO class:

Constants
---------

.. list-table::
   :widths: 30 50
   :header-rows: 1

   * - NAME
     - DESCRIPTION
   * - :ref:`SERVICE_ACCEPTED <service_accepted>`
     - {Integer} used for 
   * - :ref:`SERVICE_STARTED <service_started>`
     - {Integer} used for   
   * - :ref:`SERVICE_PAUSED <service_paused>` 
     - {Integer} used for 
   * - :ref:`SERVICE_SUCCEEDED <service_succeeded>`
     - {Integer} used for      
   * - :ref:`SERVICE_FAILED <service_failed>`
     - {Integer} used for        

Functions
---------

.. list-table::
   :widths: 12 50
   :header-rows: 1

   * - NAME
     - DESCRIPTION
   * - :ref:`removeItem <removeItem>`
     - Remove an object from an array.   
   * - :ref:`indexOf <indexOf>` 
     - 
   * - :ref:`extend <extend>` 
     - Copy all properties of a source object to a destination object.  
   * - :ref:`rad <rad>`  
     -     
   * - :ref:`distVincenty <distVincenty>`
     - Given two objects representing points with geographic coordinates, 
       this calculates the distance between those points on the surface of an  
       ellipsoid. 
   * - :ref:`Class <Class>` 
     - Method used to create ZOO classes. 
   * - :ref:`UpdateStatus <UpdateStatus>`
     - Method used to update the status of the process      

**Constants**

.. _service_accepted:

SERVICE_ACCEPTED	
  ``{Integer}`` used for
  
.. _service_started:

SERVICE_STARTED
  ``{Integer}`` used for
  
.. _service_paused:  

SERVICE_PAUSED	
  ``{Integer}`` used for
  
.. _service_succeeded:  

SERVICE_SUCCEEDED
  ``{Integer}`` used for
  
.. _service_failed:    
  
SERVICE_FAILED	
  ``{Integer}`` used for

**Functions**

.. _removeItem:

removeItem	
  ::
  
    removeItem: function(array,item)
  
  Remove an object from an array.  Iterates through the array to find the item, then removes it.
  
  *Parameters*
  
  | ``array {Array}``
  | ``item {Object}``
  
  *Returns*
   
  ``{Array}`` A reference to the array
  
.. _indexOf:

indexOf	
  ::
  
    indexOf: function(array,obj)

  *Parameters*
   
  | ``array {Array}``
  | ``obj {Object}``

  *Returns*

  ``{Integer}`` The index at, which the first object was found in the array.  If not found, returns -1.

.. _extend:

extend
  ::
  
    extend: function(destination,source)

  Copy all properties of a source object to a destination object.  Modifies the passed in destination object.  
  Any properties on the source object that are set to undefined will not be (re)set on the destination object.

  *Parameters*
  
  | ``destination {Object}`` The object that will be modified
  | ``source {Object}`` The object with properties to be set on the destination

  *Returns*

  ``{Object}`` The destination object.
  
.. _rad:  
  
rad	
  ::
  
    rad: function(x)

  *Parameters*
  
  | ``x {Float}``

  *Returns*

  ``{Float}``

.. _distVincenty:

distVincenty
  ::

    distVincenty: function(p1,p2)

  Given two objects representing points with geographic coordinates, this calculates the distance between 
  those points on the surface of an ellipsoid.

  *Parameters:*

  | ``p1`` :ref:`{ZOO.Geometry.Point} <api-zoo-geometry-point>` (or any object with both .x, .y properties)
  | ``p2`` :ref:`{ZOO.Geometry.Point} <api-zoo-geometry-point>` (or any object with both .x, .y properties)

.. _Class:

Class	
  ::
  
    Class: function()

  Method used to create ZOO classes.  Includes support for multiple inheritance.
  
.. _UpdateStatus:  
  
UpdateStatus	
  ::
  
    UpdateStatus: function(env,value)

  Method used to update the status of the process

  *Parameters*
  
  | ``env {Object}`` The environment object
  | ``value {Float}`` The status value between 0 to 100

