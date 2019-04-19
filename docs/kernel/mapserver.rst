.. _kernel-mapserver:
    
Optional MapServer support
==========================

Processing geospatial data using WPS Services is usefull. Publishing their results directly as WMS, WFS or WCS ressources is even more convenient. This is possible since `ZOO-Project 1.3 <http://zoo-project.org>`__ using the **optional MapServer support**. The latter thus allows for automatic publication of WPS Service output as WMS/WFS or WCS using a :ref:`kernel_index` specific internal mechanism which is detailed in this section. 


.. note:: 

  |mapserver| `MapServer <http://mapserver.org>`__ is an open source WMS/WFS/WCS server. Learn more by reading its `documentation <http://mapserver.org/documentation.html>`__.
 

.. |mapserver| image:: ../_static/mapserver.png
       :height: 74px
       :width: 74px
       :scale: 50%
       :alt: MapServer logo


How does it work ?
-------------------------

If a request with ``mimeType=image/png`` is sent to :ref:`kernel_index`, the latter will detect that the *useMapServer* option is set to true an it will automatically:

   * Execute the service using the *<Default>* block definition (these values must be understood by `GDAL <http:/gdal.org>`__)
   * Store the resulting output on disk (in the ``[main]`` > ``dataPath`` directory)
   * Write a `mapfile <http://mapserver.org/mapfile/index.html>`__ (in the ``[main]`` > ``dataPath`` directory) using the `MapServer <http://mapserver.org>`__ C-API (this sets up both WMS and WFS services).

Existing WPS Services source code doesn't need to be modified once the MapServer support is activated. It only takes to edit their respective :ref:`services-zcfg` files accordingly.

.. note:: In case of a vector data source output, both WMS and WFS configuration are included by default in the resulting mapfile.

.. note:: In case of a raster data source output, both WMS and WCS configuration are included by default in the resulting mapfile.

Depending on the requests, ZOO-Kernel is able to return a location header and different request types:

    * ResponseDocument=XXXX@asReference=true

In this case, ZOO-Kernel will return the GetMap/GetFeature/GetCoverage request as KVP in the *href* of the result.

    * ResponseDocument=XXXX@asReference=false

In this case, ZOO-Kernel will return the result of the GetMap/GetFeature/GetCoverage request as KVP of the href used in the previous case.

    * RawDataOutput=XXXX@asReference=true/false

In this case, ZOO-Kernel will return the GetMap/GetFeature/GetCoverage request as KVP in a specific location header, which implies that the browser is supposed to request MapServer directly.

Whatever the default output *mimeType* returned by a WPS service is, it is used if the *useMapserver* option is found at runtime. As an example, if ``<Default>`` and ``<Supported>`` blocks are found in the ZOO Service configuration file as shown bellow, this means that the service returns GML 3.1.0 features by default.

.. code-block:: guess

    <Default>
     mimeType = text/xml
     encoding = UTF-8
     schema = http://schemas.opengis.net/gml/3.1.0/base/feature.xsd
    </Default>
    <Supported>
     mimeType = image/png
     useMapserver = true
    </Supported>

Installation and configuration
------------------------------

Follow the step described bellow in order to activate the ZOO-Project optional MapServer support.

Prerequisites
.............

   * latest `ZOO-Kernel <http://zoo-project.org/trac/browser/trunk/zoo-project/zoo-kernel>`__ trunk version
   * `MapServer <http://mapserver/org>`__ version >= 6.0.1

First download the lastest zoo-kernel by checking out the svn. Use the following command from do the directory where your previously checked out (in this example we will use ``$PREV_SVN_CO`` to design this directory).

.. code-block:: guess

    svn checkout http://svn.zoo-project.org/svn/trunk/ $PREV_SVN_CO
    cd $PREV_SVN_CO
    
Then uncompress the MapServer archive (ie. ``mapserver-6.0.1.tar.bz2``) into ``/tmp/zoo-ms-src``, and compile it using the following command:

.. code-block:: guess

     cd /tmp/zoo-ms-src/mapserver-6.0.1
     ./configure --with-ogr=/usr/bin/gdal-config --with-gdal=/usr/bin/gdal-config \
                    --with-proj --with-curl --with-sos --with-wfsclient --with-wmsclient \
                    --with-wcs --with-wfs --with-postgis --with-kml=yes --with-geos \
                    --with-xml --with-xslt --with-threads --with-cairo
     make
     cp mapserv /usr/lib/cgi-bin

Once done, compile ZOO-Kernel with MapServer support from the ``$PREV_SVN_CO`` directory, using the following command:

.. code-block:: guess

     cd zoo-kernel
     autoconf
     ./configure --with-python --with-mapserver=/tmp/zoo-ms-src/mapserver-6.0.1
     make
     sudo make install


.. _kernel-mapserver-main.cfg:

Main configuration file
........................

Open and edit the ``/usr/lib/cgi-bin/main.cfg`` file, by adding the following content in the ``[main]`` section:

.. code-block:: guess

      dataPath = /var/www/temp/
      mapserverAddress=http://localhost/cgi-bin/mapserv

You can also add the following lines to the ``[main]`` section, in case
you want to use a default style for Polygon, Lines and Points vector
layer.

.. code-block:: guess

      msStylePoly=STYLE COLOR 202 109 19 OUTLINECOLOR 105 105 105 WIDTH 3 END
      msStyleLine=STYLE OUTLINECOLOR 202 109 19 WIDTH 3 END
      msStylePoint=STYLE COLOR 202 109 19 OUTLINECOLOR 105 105 105 SYMBOL 0 SIZE 14 END

The ``dataPath`` directory is mandatory and must belong to the Apache user.

.. code-block:: guess

     mkdir /var/www/temp/
     chown -r apache:apache /var/www/temp/

A ``symbols.sym`` file is required in this directory. Create it and add the following content in it:

.. code-block:: guess

      SYMBOLSET
      SYMBOL
        NAME "circle"
        TYPE ellipse
        FILLED true
        POINTS
          1 1
        END
      END
      END

.. note::
  Only one symbol definition is required (with any name) for the WMS service output.

The ZOO-Project optional MapServer support is activated at this step. Don't forget to add the ``mapserverAddress`` and  ``msOgcVersion`` parameters to the ``main.cfg`` file in order to  to specify the path to MapServer and the OGC WebService version used by the Services.

.. code-block:: guess
     mapserverAddress=http://localhost/cgi-bin/mapserv.cgi
     msOgcVersion=1.0.0

.. warning::
   ZOO-kernel will segfault (checking ``NULL`` value should correct this behavior) if the ``mapserverAddress`` parameter is not found


Service configuration file
............................

useMapserver
*************

In order to activate the MapServer WMS/WFS/WCS output for a specific service, the ``useMapserver`` parameter must be added to the ``<Default>`` or ``<Supported>`` blocks of the Service `services-zcfg`. If ``useMapserver=true``, this means that the output result of the Service is a GDAL compatible datasource and that you want it to be automatically published by MapServer as WMS,WFS or WCS.

When the useMapserver option is used in a ``<Default>`` or ``<Supported>`` block, then you have to know what are the corresponding mimeType:

   * text/xml: Implies that the output data will be accessible through a WFS GetFeature request (default protocol version 1.1.0)
   * image/tiff: Implies that the output data will be accessible through a WCS GetCoverage request (default protocol version 2.0.0)
   * any other mimeType coupled with useMapserver option: Implies that the output data will be accessible through a WMS GetMap request (default protocol version 1.3.0). You can check the supported output mimeType by sending a GetCapabilities request to MapServer.


You get the same optional parameter ``msOgcVersion`` as for the ``main.cfg``. This will specify that this is the specific protocol version the service want to use (so you may set also locally to service rather than globally).

Styling
*************

You have different options to define the style of the layer created
using the data returned by your service.

msStyle
+++++++

The optional ``msStyle`` parameter can also be used to define a custom MapServer style block (used for vector datasource only), as follow:

.. code-block:: guess

     msStyle = STYLE COLOR 125 0 105 OUTLINECOLOR 0 0 0 WIDTH 3 END

msClassify
++++++++++

If a WPS service outputs a one band raster file, then it is possible
to add a ``msClassify`` parameter and set it to ``true`` in the output
ComplexData ``<Default>`` or ``<Supported>`` nodes of its zcfg
file. This allows ZOO-Kernel to use its own default style definitions
in order to classify the raster using equivalent intervals.  

.. code-block:: guess

     msClassify = true


msInclude
+++++++++

In case you want to use another layer which use the result of your
service, for instance to produce a `heatmap
<https://mapserver.org/output/kerneldensity.html>`_, then you can use
the 
``msInclude`` and ``msLayer`` options in the output ``ComplexData``
``<Default>`` or ``<Supported>`` nodes of its zcfg file. You can see below an
example of use of this two options with the associated mapfile.

.. code-block:: guess

     msInclude = /var/data/template.map
     msLayer = heatmap

You can find below a sample ``/var/data/template.map``:

.. code-block:: guess

     MAP
       SIZE 1000 500
       EXTENT -180 -90 180 90
       NAME "test heat"
       IMAGETYPE "png"

       WEB
         METADATA
           "ows_srs" "epsg:4326  epsg:3857 epsg:900913"
           "ows_enable_request" "*"
	 END # METADATA
       END # WEB
       
       PROJECTION
         "+init=epsg:4326"
       END # PROJECTION

       LAYER
         NAME "heatmap" # Corresponding to the msLayer defined
	 TYPE raster
	 CONNECTIONTYPE kerneldensity
	 CONNECTION "Result"
	 STATUS on
	 PROCESSING "RANGE_COLORSPACE=HSL"
	 PROCESSING "KERNELDENSITY_RADIUS=20"
	 PROCESSING "KERNELDENSITY_COMPUTE_BORDERS=ON"
	 PROCESSING "KERNELDENSITY_NORMALIZATION=AUTO"
	 OFFSITE 0 0 0
	 CLASS
	   STYLE
	     COLORRANGE  "#0000ff00"  "#0000ffff"
	     DATARANGE 0 32
	   END # STYLE
	   STYLE
	     COLORRANGE  "#0000ffff"  "#ff0000ff"
	     DATARANGE 32 255
	   END # STYLE
	 END # CLASS
       END # LAYER
       
       LAYER
         NAME "points"
	 STATUS on
	 TYPE POINT
	 #DATA "/Library/WebServer/cache//ef76ee6642c1ea704e847e28120ba1ca.zca"
       END # LAYER
     END # MAPFILE




Example
**************

An example :ref:`services-zcfg` file configured for the optional MapServer support is shown bellow: 

.. code-block:: guess

    <Default>
     mimeType = text/xml
     encoding = UTF-8
     schema = http://schemas.opengis.net/gml/3.1.0/base/feature.xsd
     useMapserver = true
    </Default>
    <Supported>
     mimeType = image/png
     useMapserver = true
     asReference = true
     msStyle = STYLE COLOR 125 0 105 OUTLINECOLOR 0 0 0 WIDTH 3 END
    </Supported>
    <Supported>
     mimeType = application/vnd.google-earth.kmz
     useMapserver = true
     asReference = true
     msStyle = STYLE COLOR 125 0 105 OUTLINECOLOR 0 0 0 WIDTH 3 END
    </Supported>
    <Supported>
     mimeType = image/tif
     useMapserver = true
     asReference = true
     msClassify = true
    </Supported>

In this example, the default output ``mimeType`` is ``image/png``, so a WMS GetMap request will be returned, or the resulting ``image/tiff`` will be returned as WCS GetCoverage request.


Test requests
--------------

The optional MapServer support can be tested using any service. The
simple *HelloPy* Service is used in the following example requests.

.. note::
  The following examples require a zip file containing a Shapefile (http://localhost/data/data.zip) and a tif file (http://localhost/data/demo.tif)

Accessing a remote Zipped Shapefile as WFS GetFeatures Request:

.. code-block:: guess

     http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=HelloPy&DataInputs=a=Reference@xlink:href=http://localhost/data/data.zip&ResponseDocument=Result@asReference=true@mimetype=text/xml

Accessing a remote Zipped Shapefile as WMS GetMap Request:

.. code-block:: guess

     http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=HelloPy&DataInputs=a=Reference@xlink:href=http://localhost/data/data.zip&ResponseDocument=Result@asReference=true@mimetype=image/png

Accessing a remote tiff as WMS GetMap Request:

.. code-block:: guess

     http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=HelloPy&DataInputs=a=Reference@xlink:href=http://localhost/data/data.tiff&ResponseDocument=Result@asReference=true@mimetype=image/png

Accessing a remote tiff as WCS GetMap Request:

.. code-block:: guess

     http://localhost/cgi-bin/zoo_loader.cgi?request=Execute&service=WPS&version=1.0.0&Identifier=HelloPy&DataInputs=a=Reference@xlink:href=http://localhost/data/data.tiff&ResponseDocument=Result@asReference=true@mimetype=image/tiff


