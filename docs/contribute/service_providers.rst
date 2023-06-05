.. _service_providers:

Service Providers
=================

ZOO-Project is developed and supported by a number of businesses, organizations 
and individuals around the world.

Using a service provider is a great way to get started with ZOO-Project and 
also contributes to the ongoing sustainability of the project. Services include 
(but are not limited to):

 * training
 * setup/installation/deployment
 * custom integration
 * bug fixing
 * features/enhancements
 * core development
 * maintenance/packaging/distribution
 * documentation

The section below provides a list of service providers who can help you in 
getting the best out of your ZOO-Project investment.

If you are a service provider and would like to be listed on this page, 
please feel free to `add yourself to the service provider list <https://github.com/ZOO-Project/ZOO-Project/blob/main/docs/contribute/service_providers.rst>`__.

.. container:: service-provider

  |gatewaygeo| `GatewayGeo <https://gatewaygeomatics.com/>`__ (Canada) is a company 
  on the East Coast of Canada that has long specialized in assisting organizations 
  to publish, discover and share their geospatial data through openness : OGC 
  services & leveraging FOSS4G software. GatewayGeo is known for its longtime 
  participation in the MapServer project, and has been involved in ZOO-Project 
  development, documentation, packaging, and training for a very long time 
  (including providing much of the initial documentation & workshop materials). 
  GatewayGeo's popular product, `MS4W <https://ms4w.com/>`__, is freely available 
  and comes pre-configured with various OGC services running out-of-the-box, 
  including WPS through ZOO-Project.

  .. |gatewaygeo| image:: https://gatewaygeomatics.com/images/gatewaygeo-logo.png
         :width: 200px
         :align: top	       
         :alt: GatewayGeo logo
         :class: provider-logo
         :target: https://gatewaygeomatics.com

.. container:: service-provider

  |geolabs| `GeoLabs <http://geolabs.fr/>`__ (France) is a high-tech SME leading the 
  development of numerous Open Source geospatial projects, including the ZOO-Project. 
  GeoLabs develops and provides support for the MapMint software, allowing you to 
  quickly organize, publish, and edit your GIS datasets on the web using Open 
  Standards defined by the OGC and having the ZOO-Kernel handling every processing.

  .. |geolabs| image:: https://zoo-project.github.io/workshops/_images/geolabs-logo.png
         :width: 200px
         :align: top	       
         :alt: GeoLabs logo
         :class: provider-logo
         :target: http://www.geolabs.fr


.. container:: service-provider

  |i-bitz| `i-bitz <https://i-bitz.co.th/>`__ which has highly
  experienced in geoinformatics technology and geographic information
  system based in Bangkok, Thailand. We have professional staff to
  provide inventive and innovative geographic solutions to solve your
  immediate problems related to geoinformatics technology. Vallaris Maps
  Platform is our key product, as  Geospatial data platform that
  provides tools for stored, analysis and visualize through a web
  browser. Moreover, Vallaris is following OGC Standard especially OGC
  API series and Open Data scheme.

  .. |i-bitz| image:: ../_static/i-bitz-logo.png
         :width: 200px
         :align: top	       
         :alt: i-bitz logo
         :class: provider-logo
         :target: https://i-bitz.co.th/

.. raw:: html

   <script type="text/javascript">
    // Randomize logos
    $.fn.randomize = function(selector){
        var $elems = selector ? $(this).find(selector) : $(this).children(),
            $parents = $elems.parent();

        // found at: http://stackoverflow.com/a/2450976/746961
        function shuffle(array) {
            var currentIndex = array.length, temporaryValue, randomIndex;
            // While there remain elements to shuffle...
            while (0 !== currentIndex) {
                // Pick a remaining element...
                randomIndex = Math.floor(Math.random() * currentIndex);
                currentIndex -= 1;

                // And swap it with the current element.
                temporaryValue = array[currentIndex];
                array[currentIndex] = array[randomIndex];
                array[randomIndex] = temporaryValue;
            }
            return array;
        }

        $parents.each(function(){
            var elements = $(this).children(selector);
            shuffle(elements);
            $(this).append(elements);
        });

        return this;
    };
    $('#service-providers').randomize('div.service-provider');
    $("<div />", {class:"clearer"}).insertAfter('#service-providers .service-provider');
  </script>
