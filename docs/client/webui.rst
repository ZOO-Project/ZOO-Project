.. _webui:

ZOO-Project WebUI
=================

The **WebUI** (`nuxt-client <https://github.com/ZOO-Project/nuxt-client>`__) is a web
application for OGC API - Processes servers. It lets users browse and execute the processes
published by the ZOO-Project, deploy, replace and undeploy CWL application packages
(OGC API - Processes Part 2), and follow jobs with their results and logs. It is written with
Nuxt 3, Vue 3 and Quasar, and is published as the ``zooproject/nuxt-client`` container image.

.. contents::
   :local:
   :depth: 2

Overview
--------

The application is organised in a few pages:

* **Home**: service metadata (OpenAPI ``info``), capabilities badges built from the conformance
  classes, landing page links.
* **Processes**: list of the processes with identifier, title, description, version and kind
  (*Built-in* services shipped with the server or *Deployed* through the API), search, CWL
  workflow preview, application package download, **Add a process** (deploy from a file, a
  pasted document or a URL), **Replace** and **Delete** for deployed processes.
* **Process page**: form generated from the process description (literal and complex inputs,
  multiple values, bounding boxes drawn on a map, output transmission and format), request
  preview with *Copy as curl*, synchronous or asynchronous execution with live progress.
* **Jobs**: paginated list with filters by status and process, auto-refresh, durations, bulk
  delete; a job page with timeline, progress, results rendered by type (values, references,
  images, GeoJSON and bounding boxes on a map), logs published by the server and the raw
  documents.
* **Swagger** (Swagger UI on the ``/ogc-api/api`` document, assembled by the WebUI server so
  that the external OGC schema files it references resolve correctly) and **Conformance**.
* **Settings**: English or French, light or dark theme (follows the system setting by default,
  ``?theme=dark`` or ``?theme=light`` forces one), layout.

Requests go directly from the browser to the ZOO-Project (``<NUXT_ZOO_BASEURL>/ogc-api``), so
CORS must be enabled on the server (``cors=true`` in the ``[main]`` section of ``main.cfg``,
which the Helm chart does by default).

Authentication
--------------

The WebUI can run **anonymously** or with an **OpenID Connect** login (Keycloak layout):

* Anonymous: set ``ZOO_OGCAPI_REQUIRES_BEARER_TOKEN=false`` (chart value
  ``webui.requiresBearerToken: "false"``). Every page works without signing in. The login
  button is hidden when no OIDC provider is configured.
* With login: configure ``NUXT_OIDC_ISSUER``, ``NUXT_OIDC_CLIENT_ID``,
  ``NUXT_OIDC_CLIENT_SECRET`` and ``AUTH_ORIGIN`` (the public URL of the WebUI). The identity
  provider must allow the redirect URI ``<AUTH_ORIGIN>/api/auth/callback/keycloak``.

When the access token expires and cannot be refreshed (the identity provider closed the
session), the WebUI shows *Session expired* next to the sign-in button and falls back to
anonymous access where the server allows it.

When a user is signed in, every request to the ZOO-Project carries the bearer token. The
kernel's JWT filter (``jwts/security_service.py``) validates it against ``ZOO_KEYCLOAK_ISSUER``
(chart value ``iam.issuer``) and ``ZOO_KEYCLOAK_AUDIENCE`` (``iam.audience``), **even when**
``iam.enabled`` is ``false``: a token issued by another provider is rejected with ``401``. The
WebUI and the kernel must therefore point to the same issuer, and the access token must carry
the expected audience (an *Audience* mapper in Keycloak). Once authenticated, the DRU serves the
user's own namespace of processes and jobs instead of ``anonymous``.

Deployment with the Helm chart
------------------------------

The `zoo-project-dru chart <https://github.com/ZOO-Project/charts>`__ deploys the WebUI next to
the kernel when ``webui.enabled`` is ``true``. The main values are:

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - Value
     - Description
   * - ``webui.enabled``
     - Deploy the ``<release>-webui`` Deployment and Service (port 3000).
   * - ``webui.url``
     - Public URL of the WebUI (``AUTH_ORIGIN``), ``http://localhost:3058`` by default.
   * - ``webui.wsurl``
     - WebSocket URL of ``websocketd`` seen from the browser, ``ws://localhost:8888`` by default.
   * - ``webui.requiresBearerToken``
     - ``"false"`` to use the UI without login. **Must be a string**: Helm's ``--set`` turns
       ``false`` into a boolean rejected by the chart schema, so pass it through a values file.
   * - ``webui.oidc.issuer``, ``webui.oidc.clientId``, ``webui.oidc.clientSecret``
     - OpenID Connect client used by the WebUI.
   * - ``webui.image.repository``, ``webui.image.tag``
     - Container image (``zooproject/nuxt-client``).
   * - ``iam.issuer``, ``iam.audience``
     - Issuer and audience used by the kernel to validate bearer tokens.
   * - ``ingress.hosturl`` / ``ingress.enabled``
     - Public URL of the API (``NUXT_ZOO_BASEURL``); ``http://localhost:8080`` when no ingress.
   * - ``webui.extraEnv``
     - Additional variables, notably ``NUXT_ZOO_INTERNAL_BASEURL`` (``http://<release>-service``):
       the kernel address as seen from the WebUI pod, used to assemble the OpenAPI document when
       the public URL is not reachable from inside the cluster.

The browser must reach the API (``ingress.hosturl`` or the port-forward on 8080), the WebUI
(``webui.url``) and, with login, the identity provider. The WebUI pod itself needs outbound
HTTPS access to ``schemas.opengis.net`` and ``raw.githubusercontent.com`` to assemble the
OpenAPI document for Swagger; without it the raw document is shown with a banner listing the
references Swagger cannot resolve.

Local development stack
-----------------------

The ``skaffold.yaml`` of this repository provides composable profiles:

* ``webui``: enables the WebUI, forwards it to http://localhost:3058 and, through
  ``values-webui.yaml``, sets ``requiresBearerToken`` to ``"false"`` and
  ``NUXT_ZOO_INTERNAL_BASEURL`` to ``http://zoo-project-dru-service``.
* ``keycloak``: wires the WebUI login and the kernel's JWT validation to a local Keycloak
  started with ``docker/keycloak/start.sh`` (realm ``zooproject``, client ``webapp``, user
  ``jules`` / ``zoo``, admin console ``admin`` / ``admin``). The Keycloak URL must be reachable
  from the browser **and** from the pods, hence the machine's LAN IP (``KEYCLOAK_URL``).
* ``webui-local``: uses an image built from a local checkout of ``nuxt-client``
  (``docker build -f Dockerfile.nuxt -t zooproject/nuxt-client:boost .``).

.. code-block:: bash

   # 1. optional: identity provider
   docker/keycloak/start.sh
   export KEYCLOAK_URL=http://<lan-ip>:8085

   # 2. the stack (first run builds the kernel image, ~30 min)
   skaffold dev -p hostpath,webui,keycloak --platform=linux/amd64

   # or redeploy without rebuilding the kernel image
   skaffold deploy -p hostpath,webui,keycloak \
       --images zoodruimage=zoodruimage:<tag> --port-forward=user

Then open http://localhost:3058 (API on http://localhost:8080, websocketd on 8888).

.. note::

   ``skaffold dev`` uninstalls the Helm releases when it exits, **including when the image
   build fails**. The ``upgradeOnChange: true`` setting on the ``zoo-project-dru`` release is
   required for ``skaffold deploy`` to upgrade an already installed remote chart.

Running the WebUI outside Kubernetes
------------------------------------

.. code-block:: bash

   docker run --rm -p 3000:3000 \
     -e NUXT_ZOO_BASEURL=http://localhost:8080 \
     -e NUXT_WS_URL=ws://localhost:8888 \
     -e ZOO_OGCAPI_REQUIRES_BEARER_TOKEN=false \
     -e AUTH_ORIGIN=http://localhost:3000 -e NEXTAUTH_URL=http://localhost:3000 \
     -e NUXT_AUTH_SECRET=change-me \
     zooproject/nuxt-client:latest

The full list of variables, the identity provider requirements and the developer guide are
maintained in the ``nuxt-client`` repository (``README.md`` and ``docs/``).

Troubleshooting
---------------

* **"Session expired" in the header / 401 Invalid JWT token** – the access token expired and
  the identity provider no longer accepts the refresh token: sign in again. The bundled Keycloak
  realm uses 1 h access tokens and 12 h idle sessions.
* **The jobs page is empty / asks to sign in** – the server requires a token
  (``ZOO_OGCAPI_REQUIRES_BEARER_TOKEN=true``) and you are not signed in, or the token was
  rejected (check the issuer and audience on both sides).
* **Login fails after Keycloak** – the redirect URI is not allowed on the client, or
  ``AUTH_ORIGIN`` differs from the URL used in the browser.
* **"ImmutableProcess" when deleting a process** – the kernel needs the copy of the package in
  ``/usr/lib/cgi-bin/<id>.cwl``; it is lost when the kernel pod is recreated without a
  persistent volume. Copy the package back (``kubectl cp``) and retry.
* **Requests blocked by the browser** – CORS is disabled on the kernel, or the API URL
  (``NUXT_ZOO_BASEURL``) is not the one reachable from the browser.
