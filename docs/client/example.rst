.. _client-example:

Example application
=====================

This section gives a detailed example of ZOO-Client based JavaScript appliclation.

.. note::
   For this example application, first setup a ``/zoo-client-demo`` directory accessible from your web server at `http://localhost/zoo-client-demo`.

The following subdirectories must be created in the ``/zoo-client-demo`` directory:

:: 

    assets
    assets/js
    assets/js/lib
    assets/js/lib/hogan
    assets/js/lib/jquery
    assets/js/lib/query-string
    assets/js/lib/xml2json
    assets/js/lib/zoo
    assets/tpl

You will need to copy your node_modules javascript files copied in the
`hogan` and `query-string` directories. First, you wil need to install
query-string.

:: 

    npm install query-string

Then you will copy `query-string.js` and `hogan-3.0.2.js` files in
your `zoo-client-demo` web directory. Those files are located in your
`~/node_modules` directory.

For other libraries, you will need to download them from their
official web sites and uncompress them in the corresponding
directories.

Loading the modules from your web application
*********************************************

Before using the ZOO-Client, you will first have to include the
javascript files from your web page. With the use of requirejs you
will need only one line in your HTML page to include everything at
once. This line will look like the following:

::

    <script data-main="assets/js/first" src="assets/js/lib/require.js"></script>

In this example, we suppose that you have created a `first.js` file
in the `assets/js` directory containing your main application
code. First, you define there the required JavaScript libraries and
potentially their configuration, then you can add any relevant code.

.. code-block:: javascript
    :linenos:

    requirejs.config({
        baseUrl: 'assets/js',
        paths: {
            jquery: 'lib/jquery/jquery-1.11.0.min',
            hogan: 'lib/hogan/hogan-3.0.2',
            xml2json: 'lib/xml2json/xml2json.min',
            queryString: 'lib/query-string/query-string',
            wpsPayloads: 'lib/zoo/payloads',
            wpsPayload: 'lib/zoo/wps-payload',
            utils: 'lib/zoo/utils',
            zoo: 'lib/zoo/zoo',
            domReady: 'lib/domReady',
            app: 'first-app',
        },
        shim: {
            wpsPayloads: {
                deps: ['hogan'],
            },
            wpsPayload: {
                deps: ['wpsPayloads'],
                exports: 'wpsPayload',
            },
            hogan: {
                exports: 'Hogan',
            },
            xml2json: {
              exports: "X2JS",
            },
            queryString: {
                exports: 'queryString',
            },
        },
    });
    
    requirejs.config({ 
        config: {
            app: {
                url: '/cgi-bin/zoo_loader.cgi',
                delay: 2000,
            }
        } 
    });
    
    require(['domReady', 'app'], function(domReady, app) {
        domReady(function() {
            app.initialize();
        });
    });

On line 2, you define the url where your files are located on the web
server, in `assets/js`. From line 3 to 14, you define the JavaScript
files to be loaded. From line 15 to 21, you configure the dependencies
and exported symbols. From line 35 to 42, you configure your main
application.

In this application, we use the `domReady
<http://github.com/requirejs/domReady>`__ module to call the
`initialize` function defined in the `app` module, which is defined in
the `first-app.js` file as defined on line 13.



.. code-block:: javascript
    :linenos:

    define([
        'module','zoo','wpsPayload'
    ], function(module, ZooProcess, wpsPayload) {
        
        var myZooObject = new ZooProcess({
            url: module.config().url,
            delay: module.config().delay,
        });
        
        var initialize = function() {
            self = this;        
            myZooObject.getCapabilities({
               type: 'POST',
                 success: function(data){
                         console.log(data);
                }
            });

            myZooObject.describeProcess({
                type: 'POST',
                identifier: "all",
                success: function(data){
                    console.log(data);
                }
            });

           myZooObject.execute({
               identifier: "Buffer",
               dataInputs: [{"identifier":"InputPolygon","href":"XXX","mimeType":"text/xml"}],
               dataOutputs: [{"identifier":"Result","mimeType":"application/json","type":"raw"}],
               type: 'POST',
               success: function(data) {
                    console.log(data);
               },
               error: function(data){
                    console.log(data);
               }
            });
        }
    
        // Return public methods
        return {
            initialize: initialize
        };
    
    });

On line 5 you create a "global" `ZooProcess` instance named
`myZooObject`, you set the `url` and `delay` to the values defined in
`first.js` on line 35. From line 10 to 40,  you define a simple
`initialize` function which will invoke the `getCapabilities` (line
12 to 18), `describeProcess` (from line 20 to 26) and `execute` (from
line 28 to 39) methods. For each you define a callback function which
will simply display the resulting data in the browser's console.


