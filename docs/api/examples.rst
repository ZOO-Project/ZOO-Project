.. _api-examples:

Examples
========

This section gathers sample scripts using :ref:`ZOO-API <api-what>`, the `ZOO-Project <http://zoo-project.org>`__ server-side JavaScript API.

ZOO-API contains many classes and functions. You can find the description list :ref:`here <api-classes>`.

ZOO.Process example
--------------------------

::

  function SampleService(conf,inputs,outputs){
     var myProcess = new ZOO.Process('http://localhost/cgi-bin-new1/zoo_loader_new1.cgi','Boundary');
     var myInputs = {InputPolygon: { type: 'complex', value: '{"type":"Polygon","coordinates":[[[-106.993853,35.998758],[-107.407233,35.997524],[-107.430525,35.997726],[-107.4824,35.99878],[-108.37013,35.999472],[-109.043633,35.996652],[-109.096265,35.997817],[-109.148763,36.001751],[-109.200981,36.008442],[-109.252775,36.017871],[-109.304,36.030014],[-109.354516,36.044835],[-106.468201,35.991497],[-106.855511,35.989504],[-106.90933,35.990676],[-106.963008,35.994743],[-106.993853,35.998758]]]}', mimeType: "application/json"} };
     var myExecuteResult=myProcess.Execute(myInputs);
     return {result: ZOO.SERVICE_SUCCEEDED, outputs: [ {name:"Result", value: myExecuteResult} ] };
  }

In this really short example you can see how to create ``ZOO.Process`` class instance and 
call the Execute method on such an instance. Then you'll just need to return a JavaScript 
object containing the attributes result and outputs, which I'm sure you already know 
what is about. The first is about the status of the process (can be ``ZOO.SERVICE_SUCEEDED``,
``ZOO.SERVICE_FAILED`` and so on), the last is obviously the resulting maps (take a 
look at the maps internal data structure used by ZOO Kernel in ``service.h``).

ZOO.UpdateStatus example
------------------------

::

  function SampleLongService(conf,inputs,outputs){
     var my_i=0;
     while(my_i<100){
         try{
           conf["lenv"]["status"]=my_i;
         }
         catch(e){
         }
         ZOOUpdateStatus(conf,my_i);
         SampleService(conf,inputs,outputs);
         my_i+=10;
     }
     return SampleService(conf,inputs,outputs);
  }

You can see in this sample code how to use the ``ZOOUpdateStatus`` function to update the 
current status of your running process. This information will be really helpfull when the 
ZOO Kernel will run your JavaScript Service in background mode (if the user set to ``true`` 
the ``storeExecuteResponse`` parameter in his request).

Asynchronous ZOO.Process example
--------------------------------

::

  function RunAsynchonous(conf,inputs,outputs){
     var formatWPS=new ZOO.Format.WPS();
     var myProcess = new ZOO.Process(conf["main"]["serverAddress"],'Demo',true);
     var myExecuteResult  =myProcess.Execute(myInputs,myOutputs);
     var response=formatWPS.read(myExecuteResult);
     while(response.status){
         sleep(1500);
	 var response1 = ZOO.Request.Get(response.status.replace(/amp;/g,""),null);
	 response=formatWPS.read(response1);
     }
     conf["lenv"]["message"]="Asyncrhonous Process ended";
     ZOO.UpdateStatus(conf,90);
     return {result: ZOO.SERVICE_SUCCEEDED, outputs: outputs};
   }

In this sample code, by creating the ZOO.Process, providing a third
argument set to true, we ensure that the WPS service execution will be run
asynchronously. Once the Execute WPS response is fetched, until the
service execution ends, the statusLocation is polled every 1,5
second.
