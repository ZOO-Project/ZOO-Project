.. _process-profiles:
    
Process profiles registry
=========================

WPS Services belonging to the same Services provider often share the
same inputs and outputs. In such a case, every :ref:`ZCFG
<services-zcfg>` file would contain the same metadata information and
this may be a waste of time to write them all.

:ref:`ZOO-Kernel <kernel_index>` is able to handle metadata
inheritance from `rev. 607
<http://www.zoo-project.org/trac/changeset/607>`__, and this solves
the issue of writing many ZCFG with same input and output. A registry
can be loaded by the ZOO-Kernel (before any other ZCFG files) and
contain a set of Process Profiles organized in hierarchic levels
according to the following rules:

  * *Concept*: The higher level in the hierarchy. *Concepts* are basic
    text files containing an abstract description of a WPS Service
    (see `the OGC definition
    <http://docs.opengeospatial.org/is/14-065/14-065.html#33>`_ for
    more details).
  * *Generic*: A *Generic* profile can make reference to
    *Concepts*. It defines inputs and outputs without data format or
    maximum size limitation (see `the OGC definition
    <http://docs.opengeospatial.org/is/14-065/14-065.html#34>`_ for
    more details).
  * *Implementation*: An *Implementation* profile can inherit from a
    generic profile and make reference to concepts (see `the OGC definition
    <http://docs.opengeospatial.org/is/14-065/14-065.html#35>`_ for
    more details). It contains all the metadata information about a
    particular WPS Service (see :ref:`ZCFG reference <services-zcfg>`
    for more information).

Both *Generic* and *Implementation* process profiles are created  from
:ref:`ZCFG <services-zcfg>` files and stored in the registry
sub-directories according to their level (*Concept*, *Generic* or
*Implementation*).

To activate the registry, you have to add a ``registry`` key to the
``[main]`` section of your ``main.cfg`` file, and set its value to the
directory path used to store the profile ZCFG files. Please see
:ref:`install_gfr` for more details about the other services and
parameters required.

.. note::
    Even if the profile registry was first introduced in WPS 2.0.0, it
    can be also used in the same way for WPS 1.0.0 Services.




Generic Process Profile
-----------------------

A Generic Process Profile is a ZCFG file located in the ``generic``
sub-directory, it defines `main metadata information
<zcfg-reference.html#main-metadata-information>`__, inputs and outputs
name, basic metadata and multiplicity. It can make reference to a
concept by defining a ``concept`` key in the `main metadata
information <zcfg-reference.html#main-metadata-information>`__ part.

You can find below the `GO.zcfg` file, a typical Generic Process
Profile for Generic Geographic Operation, taking one InputPolygon
input parameter and returning a result named Result, it make reference
to the ``GOC`` concept:

.. code-block:: none
   :linenos:
   
   [GO]
    Title = Geographic Operation
    Abstract = Geographic Operation on exactly one input, returning one output
    concept = GOC
    level = generic
    statusSupported = true
    storeSupported = true
    <DataInputs>
     [InputPolygon]
      Title = the geographic data
      Abstract = the geographic data to run geographipc operation
      minOccurs = 1
      maxOccurs = 1
    </DataInputs>
    <DataOutputs>
     [Result]
      Title = the resulting data
      Abstract = the resulting data after processing the operation
    </DataOutputs>  


.. Note:: if you need to reference more than one concept, you should
    separate their names with a comma (ie. concept = GO,GB),

Process Implementation Profile
------------------------------

A Process Implementation Profile is similar to a `ZCFG file
<zcfg-reference.html>`__ located in the `implementation`
sub-directory, it defines (or inherit from its parent) all the
properties of a `Generic Process Profile <#generic-process-profile>`__
and specify `Data Format <zcfg-reference.html#type-of-data-nodes>`__
for both inputs and outputs. It can make reference to a concept by
defining a ``concept`` key in the `main metadata information
<zcfg-reference.html#main-metadata-information>`__ part.

You can find below the `VectorOperation.zcfg` file, a typical Process
Implementation Profile for Vector Geographic Operation, it inherit
from the `GP generic profile <#generic-process-profile>`__:

.. code-block:: none
   :linenos:
   
   [VectorOperation]
    Title = Vector Geographic Operation
    Abstract = Apply a Vector Geographic Operation on a features collection and return the resulting features collection
    extend = GO
    level = profile
    <DataInputs>
     [InputPolygon]
      Title = the vector data
      Abstract = the vector data to run geographic operation
      <ComplexData>
       <Default>
        mimeType = text/xml
        encoding = UTF-8
        schema = http://fooa/gml/3.1.0/polygon.xsd
       </Default>
       <Supported>
        mimeType = application/json
        encoding = UTF-8
        extension = js
       </Supported>
    </DataInputs>
    <DataOutputs>
     [Result]
      Title = the resulting data
      Abstract = the resulting geographic data after processing the operation
      <ComplexData>
       <Default>
        mimeType = text/xml
        encoding = UTF-8
        schema = http://fooa/gml/3.1.0/polygon.xsd
       </Default>
       <Supported>
        mimeType = application/json
        encoding = UTF-8
        extension = js
       </Supported>
      </ComplexData>
    </DataOutputs>  


ZCFG inheritance
----------------------------------

For the ZCFG files at the service level, you can inherit the metadata
from a Process Implementation Profile available in the registry. As
before, you simply need to add a ``extend`` key refering the ZCFG you
want to inherit from and a ``level`` key taking the `ìmplementation``
value to your main metadata informations.

So, for example, the original `ConvexHull.zcfg
<http://www.zoo-project.org/trac/browser/trunk/zoo-project/zoo-services/ogr/base-vect-ops/cgi-env/ConvexHull.zcfg?rev=491>`__
may be rewritten as:

.. code-block:: none
   :linenos:
   
   [ConvexHull]
    Title = Compute convex hull.
    Abstract = Return a feature collection that represents the convex hull of each geometry from the input collection.
    serviceProvider = ogr_service.zo
    serviceType = C
    extend = VectorOperation
    level = implementation

Now, suppose that your service is able to return the result in KML
format, then you may write the following:

.. code-block:: none
   :linenos:
   
   [ConvexHull]
    Title = Compute convex hull.
    Abstract = Return a feature collection that represents the convex hull of each geometry from the input collection.
    serviceProvider = ogr_service.zo
    serviceType = C
    extend = VectorOperation
    level = implementation
    <DataOutputs>
     [Result]
        <Supported>
         mimeType = application/vnd.google-earth.kml+xml
         encoding = utf-8
        </Supported>
    </DataOutputs>

.. _install_gfr:

Setup registry browser
----------------------

In the ``zoo-project/zoo-services/utils/registry``  you can find the
source code and the ``Makefile`` required to build the Registry Browser
Services Provider. To build and install this service, use the
following comands:

.. code::

     cd zoo-project/zoo-services/utils/registry
     make
     cp cgi-env/* /usr/lib/cgi-bin


To have valid
``href`` in the metadata children of a ``wps:Process``, you have to
define the ``registryUrl`` to point to the path to browse the
registry. For this you have two different options, the first one is to
install the ``GetFromRegistry`` ZOO-Service and to use a WPS 1.0.0
Execute request as ``registryUrl`` to dynamically generate `Process
Concept <http://docs.opengeospatial.org/is/14-065/14-065.html#33>`__,
`Generic Process Profile
<http://docs.opengeospatial.org/is/14-065/14-065.html#34>`__ and
`Process Implementation Profile
<http://docs.opengeospatial.org/is/14-065/14-065.html#35>`__.
You also have to add a ``registryUrl`` to the ``[main]`` section to
inform the ZOO-Kernel that it should use the Registry Browser to
create the href attribute of Metadata nodes. So by adding the
following line:

.. code::

    registryUrl = http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=GetFromRegistry&RawDataOutput=Result&DataInputs=id=

The second option is to pre-generate each level of the hierarchy by
running shell commands then set ``registryUrl`` to the URL to browse
the generated files. In such a case, you will also have to define the
``registryExt`` and set it to the file extension you used to generate
your registry cache.

To generate the cache in ``/opt/zoo/registry/``, use the following command:

.. code::

    cd /usr/lib/cgi-bin
    mkdir /opt/zoo/regcache/{concept,generic,implementation}
    for i in $(find /opt/zoo/registry/ -name "*.*") ; 
    do  
        j=$(echo $i | sed "s:../registry//::g;s:.zcfg::g;s:.txt::g") ; 
       if [ -z "$(echo $j | grep concept)" ]; 
       then 
           ext="xml" ; 
       else 
           ext="txt"; 
       fi
        ./zoo_loader.cgi "request=Execute&service=wps&version=1.0.0&Identifier=GetFromRegistry&RawDataOutput=Result&DataInputs=id=$j" | grep "<" > /opt/zoo/regcache/$j.$ext; 
    done

