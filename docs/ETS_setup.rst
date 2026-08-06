How to setup ETS to run tests of the OGC API - Processes exposed by the ZOO-Project using Docker
================================================================================================

From a Web Browser
---------------------

1. Setup ZOO-Project on the local machine

..

   .. image:: media/image1.png
      :width: 6.26806in
      :height: 3.02431in


2. Setup the OGC Cite test server on a local machine using the
   documentation included within the Dockerfile [1]

..

   .. image:: media/image2.png
      :width: 6.26806in
      :height: 1.94583in

3. Open localhost:8080/teamengine in your web browser and login to the
   OGC Validator portal [2]

..

   .. image:: media/image3.png
      :width: 4.49888in
      :height: 2.39394in

4. Run the tests for OGC API - Processes - Part 1: Core from local
   machine, cite test instance using local ZOO-Project instance as
   Instance Under Test (IUT in the JAVA code associated with the CITE
   team code available from here [3]

..

   .. image:: media/image4.png
      :width: 4.5506in
      :height: 4.0303in

5. To debug the errors while building docker and setting up and running
   the team engine on a local server following steps can be followed:

-  Modify the source code file [4] for that particular error and add
   debugging information

-  Build the docker file and execute the test again.

-  Get the request body content and the URL to which it is sent using
   docker logs.

..

   .. image:: media/image5.png
      :width: 6.26806in
      :height: 0.81667in

-  Then, the part where the issue is can be identified, and the error
   can be debugged.


From the Command Line Interface
---------------------

1. Go to ZOO-Project main repository [5] and switch to Action tab [6]

.. image:: media/image6.png
   :width: 6.26806in
   :height: 1.23403in

2. Select the Docker Image CI workflow to run the docker image container
   [7]

.. image:: media/image7.png
   :width: 6.26806in
   :height: 0.56806in

3. Execute the test by running build.sh file [8]

.. image:: media/image8.png
   :width: 6.26806in
   :height: 1.23497in

4. The executed test results can be observed after executing step 14,
   i.e., “Run the ETS for OGC-API Processes section”. [9]

.. image:: media/image9.png
   :width: 6.26806in
   :height: 2.6875in


[1] `https://github.com/opengeospatial/ets-ogcapi-processes10/blob/master/Dockerfile <https://github.com/opengeospatial/ets-ogcapi-processes10/blob/master/Dockerfile>`__

[2] `localhost:8080/teamengine <localhost:8080/teamengine>`__

[3] `https://github.com/opengeospatial/ets-ogcapi-processes10 <https://github.com/opengeospatial/ets-ogcapi-processes10>`__

[4] `https://github.com/opengeospatial/ets-ogcapi-processes10/blob/master/src/main/java/org/opengis/cite/ogcapiprocesses10/jobs/Jobs.java <https://github.com/opengeospatial/ets-ogcapi-processes10/blob/master/src/main/java/org/opengis/cite/ogcapiprocesses10/jobs/Jobs.java>`__

[5] `https://github.com/ZOO-Project/ZOO-Project <https://github.com/ZOO-Project/ZOO-Project>`__

[6] `https://github.com/ZOO-Project/ZOO-Project/actions <https://github.com/ZOO-Project/ZOO-Project/actions>`__

[7] `https://github.com/ZOO-Project/ZOO-Project/actions/workflows/docker-image.yml <https://github.com/ZOO-Project/ZOO-Project/actions/workflows/docker-image.yml>`__

[8] `https://github.com/ZOO-Project/ZOO-Project/actions/runs/5811142262/job/15753671952 <https://github.com/ZOO-Project/ZOO-Project/actions/runs/5811142262/job/15753671952>`__

[9] `https://github.com/ZOO-Project/ZOO-Project/actions/runs/5811142262/job/15753671952#step:14:1 <https://github.com/ZOO-Project/ZOO-Project/actions/runs/5811142262/job/15753671952#step:14:1>`__
