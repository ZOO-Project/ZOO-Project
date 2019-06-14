.. _services-zcfg:
    
ZOO-Service configuration file
=========================================  

The ZOO-Service configuration file (.zcfg) describes a
WPS service. It provides metadata information on a particular WPS
Service and it is parsed by ZOO-Kernel when *DescribeProcess* and
*Execute* request are sent.

The ZOO-Service configuration file is divided into three distinct sections :

 * Main Metadata information
 * List of Inputs metadata information (optional since `rev. 469 <http://zoo-project.org/trac/changeset/469>`__)
 * List of Outputs metadata information

.. warning:: The ZOO-Service configuration file is case sensitive.

.. note:: There are many example ZCFG files in the ``cgi-env``
	  directory of the `ZOO-Project svn
	  <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-services>`__.

.. note:: A ZCFG file can be converted to the YAML syntaxe by using
	  the zcfg2yaml command line tool.

Main section
-------------------------

The fist part of the ZOO-Service configuration file is the ``main`` section,
which contains general metadata information on the related WPS
Service.

Note that the "name of your service" between brackets on the first line has to be the exact same name 
as the function you defined in your services provider code. In most cases, this name is also the name 
of the ZCFG file without the "``.zcfg``" extension.

An example of the ``main`` section  is given bellow as reference.

.. code-block:: none
   :linenos:

   [Name of WPS Service]
   Title = Title of the WPS Service
   Abstract = Description of the WPS Service
   processVersion = Version number of the WPS Service
   storeSupported = true/false
   statusSupported = true/false
   serviceType = Pprogramming language used to implement the service (C|Fortran|Python|Java|PHP|Ruby|Javascript)
   serviceProvider = Name of the Services provider (shared library|Python Module|Java Class|PHP Script|JavaScript Script)
   <MetaData>
     title = Metadata title of the WPS Service
   </MetaData>

.. warning::  'Name of WPS Service' must be the exact same name as the function defined in the WPS Service source code.

.. note:: An ``extend`` parameter may be used for the `Process profile registry <process-profiles.html>`__.

List of Inputs
--------------

The second part of the ZOO-Service configuration file is the ``<DataInputs>``
section which lists the supported inputs. Each input is defined as :

 * Name (between brackets as for the name of the service before)
 * Various medata properties (``Title``, ``Abstract``, ``minOccurs``, ``maxOccurs`` and, in case of ComplexData, the optional ``maximumMegabytes``)
 * :ref:`Type Of Data Node  <typeDataNodes>` 

A typical list of inputs (``<DataInputs>``) looks like the following:

.. code-block:: none
   :linenos:
   
   <DataInputs>
     [Name of the first input]
       Title = Title of the first input
       Abstract = Abstract describing the first input
       minOccurs = Minimum occurence of the first input
       maxOccurs = Maximum occurence of the first input
       <Type Of Data Node />
     [Name of the second input]
       Title = Title of the second input
       Abstract = Abstract describing the second input
       minOccurs = Minimum occurence of the second input
       maxOccurs = Maximum occurence of the second input
       <Type Of Data Node />
   </DataInputs>
   
.. note:: A ``<MetaData>`` node can also be added, as in the main metadata information.

List of Outputs
---------------

The third part of the ZOO Service configuration file is the ``<DataOutputs>``
section, which lists the supported outputs and is is very similar to a
list of inputs.

A typical list of outputs (``<DataOutputs>``) looks like the
following:

.. code-block:: none
   :linenos:
   
   <DataOutputs>
     [Name of the output]
       Title = Title of the output
       Abstract = Description of the output
       <Type Of Data Node />
   </DataOutputs>

.. _typeDataNodes:

Type Of Data Nodes
------------------

The *Type Of Data Nodes* describes data types for inputs and
outputs. There are three different types which are described in this
section.
 * :ref:`LiteralData <LiteralData>`
 * :ref:`BoundingBoxData <BoundingBoxData>`
 * :ref:`ComplexData <ComplexData>`

 .. warning:: Every *BoundingBoxData* and *ComplexData* must have at
	      least one ``<Default>`` node (even empty like ``<Default
	      />``)

 .. warning:: In WPS 2.0.0 version, it is possible to define `nested
	      inputs and outputs
	      <http://docs.opengeospatial.org/is/14-065/14-065.html#13>`__. So,
	      from revision `790 
	      <http://www.zoo-project.org/trac/changeset/790>`__, you
	      are allowed to use a new input/output definition here.

.. _LiteralData:

LiteralData node
****************

A ``<LiteralData>`` node contains:

- one (optional) ``AllowedValues`` key containing all value allowed for this input
- one (optional) ``range`` properties containing the range (``[``, ``]``)
- one (optional) ``rangeMin`` (``rangeMax``) properties containing the minimum (maximum) value of this range
- one (optional) ``rangeSpacing`` properties containing the regular distance or spacing between value in this range
- one (optional) ``rangeClosure`` properties containing the closure type (``c``, ``o``, ``oc``, ``co``)
- one ``<Default>`` node,
- zero or more ``<Supported>`` nodes depending on the existence or the number of supported Units Of Measure (UOM), and 
- a ``dataType`` property. The ``dataType`` property defines the type of literal data, such as a string, an interger and so on 
  (consult `the complete list <http://www.w3.org/TR/xmlschema-2/#built-in-datatypes>`__ of supported data types). 

``<Default>`` and ``<Supported>`` nodes can contain the ``uom`` property to define which UOM has to be used for 
this input value.

For input ``<LiteralData>`` nodes, you can add the ``value`` property to the ``<Default>`` node to define a default 
value for this input. This means that, when your Service will be run, even if the input wasn't defined, this default 
value will be set as the current value for this input.

A typical ``<LiteralData>`` node, defining a ``float`` data type using meters or degrees for its UOM, looks like the 
following:

.. code-block:: guess
   :linenos:
   
   <LiteralData>
     dataType = float
     <Default>
       uom = meters
     </Default>
     <Supported>
       uom = feet
     </Supported>
   </LiteralData>


A typical ``<LiteralData>`` node, defining a ``float`` data type which
should take values contained in ``[0.0,100.0]``, looks like the following:

.. code-block:: guess
   :linenos:
   
   <LiteralData>
     dataType = float
     rangeMin = 0.0
     rangeMax = 100.0
     rangeClosure = c
     <Default />
   </LiteralData>

Or more simply:

.. code-block:: guess
   :linenos:
   
   <LiteralData>
     dataType = float
     range = [0.0,100.0]
     <Default />
   </LiteralData>

A typical ``<LiteralData>`` node, defining a ``string`` data type which
support values ``hillshade``, ``slope``, ``aspect``, ``TRI``, ``TPI``
and ``roughness``, looks like the following:

.. code-block:: guess
   :linenos:
   
   <LiteralData>
     dataType = string
     AllowedValues = hillshade,slope,aspect,TRI,TPI,roughness
     <Default />
   </LiteralData>

Properties ``AllowedValues`` and ``range*`` can be conbined with both ``<Default>`` and
``<Supported>`` nodes in the same was as ``<LiteralData>`` node. For
instance, the following is supported:

.. code-block:: guess
   :linenos:
   
   <LiteralData>
     dataType = int
     <Default>
       value = 11
       AllowedValues = -10,-8,-7,-5,-1
       rangeMin = 0
       rangeMax = 100
       rangeClosure = co
     </Default>
     <Supported>
       rangeMin = 200
       rangeMax = 600
       rangeClosure = co
     </Supported>
     <Supported>
       rangeMin = 750
       rangeMax = 990
       rangeClosure = co
       rangeSpacing = 10
     </Supported>
   </LiteralData>

.. _BoundingBoxData:

BoundingBoxData node
********************

A ``<BoundingBoxData>`` node contains:

- one ``<Default>`` node with a CRS property defining the default Coordinate Reference Systems (CRS), and 
- one or more ``<Supported>`` nodes depending on the number of CRS your service supports (note that you can 
  alternatively use a single ``<Supported>`` node with a comma-separated list of supported CRS).

A typical ``<BoundingBoxData>`` node, for two supported CRS (`EPSG:4326 <http://www.epsg-registry.org/indicio/query?request=GetRepositoryItem&id=urn:ogc:def:crs:EPSG::4326>`__ 
and `EPSG:3785 <http://www.epsg-registry.org/indicio/query?request=GetRepositoryItem&id=urn:ogc:def:crs:EPSG::3785>`__), 
looks like the following:

.. code-block:: guess
   :linenos:
   
   <BoundingBoxData>
     <Default>
       CRS = urn:ogc:def:crs:EPSG:6.6:4326
     </Default>
     <Supported>
       CRS = urn:ogc:def:crs:EPSG:6.6:4326
     </Supported>
     <Supported>
       CRS = urn:ogc:def:crs:EPSG:6.6:3785
     </Supported>
   </BoundingBoxData>

.. _ComplexData:

ComplexData node
****************

A ComplexData node contains:

- a ``<Default>`` node and 
- one or more ``<Supported>`` nodes depending on the number of supported formats. A format is made up of this 
  set of properties : ``mimeType``, ``encoding`` and optionaly ``schema``.

For output ComplexData nodes, you can add the ``extension`` property to define what extension to use to name 
the file when storing the result is required. Obviously, you'll have to add the ``extension`` property to each 
supported format (for the ``<Default>`` and ``<Supported>`` nodes). 

You can also add the ``asReference`` property to the ``<Default>`` node to define if the output should be 
stored on server side per default. 

.. Note:: the client can always modify this behavior by setting ``asReference`` attribute to ``true`` or ``false`` 
          for this output in the request ``ResponseDocument`` parameter.

You can see below a sample ComplexData node for default ``application/json`` and ``text/xml`` (encoded in UTF-8 
or base64) mimeTypes support:

.. code-block:: guess
   :linenos:
   
   <ComplexData>
     <Default>
       mimeType = application/json
       encoding = UTF-8
     </Default>
     <Supported>
       mimeType = text/xml
       encoding = base64
       schema = http://fooa/gml/3.1.0/polygon.xsd
     </Supported>
     <Supported>
       mimeType = text/xml
       encoding = UTF-8
       schema = http://fooa/gml/3.1.0/polygon.xsd
     </Supported>
   </ComplexData>
