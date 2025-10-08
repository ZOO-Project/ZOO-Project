#
# Base: mdl4eo/otbtf:4.0.0-cpu based on Ubuntu 22.04.2 with updates and external packages
# the base image would benefit to be parameter of this Dockerfile
# 
# Usage:
# docker build . -f Dockerfile-OTBTF -t zookernel-ml4eo:latest
#
FROM mdl4eo/otbtf:4.2.1-cpu AS base 
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    dirmngr \
    gpg-agent \
    software-properties-common \
"
ARG RUN_DEPS=" \
    libcurl3-gnutls \
    libfcgi \
    curl \
    \
    #saga \
    #libsaga-api-7.3.0 \
    #libotb \
    #otb-bin \
    \
    libpq5 \
    libpython3.6 \
    libxslt1.1 \
    gdal-bin \
    #libcgal13 \
    librabbitmq4 \
    nlohmann-json3-dev \
    python3 \
    r-base \
    python3-pip\
    libfribidi0 \
    libprotobuf-c1\
"

USER root

RUN set -ex \
    && apt-get update \
    && ln -fs /usr/share/zoneinfo/Europe/Paris /etc/localtime \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends tzdata \
    && dpkg-reconfigure --frontend noninteractive tzdata \
    && apt-get install -y --no-install-recommends $BUILD_DEPS  \
    && apt-cache search nlohmann \
    \
    #&& add-apt-repository ppa:ubuntugis/ppa \
    \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y $RUN_DEPS \
    \
    && curl -o libmozjs185-1.0_1.8.5-1.0.0+dfsg-7_amd64.deb http://launchpadlibrarian.net/309343863/libmozjs185-1.0_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && curl -o libmozjs185-dev_1.8.5-1.0.0+dfsg-7_amd64.deb http://launchpadlibrarian.net/309343864/libmozjs185-dev_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && dpkg --force-depends -i libmozjs185-1.0_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && dpkg --force-depends -i libmozjs185-dev_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && apt  -y --fix-broken install \
    #&& rm /libmozjs185*.deb \
    \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && sudo rm -rf /var/lib/apt/lists/*

#
# builder1: base image with zoo-kernel
#
FROM base AS builder1
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    bison \
    flex \
    make \
    autoconf \
    automake \
    libtool \
    gcc \
    gettext \
    \
    cmake \
    git \
    libfcgi-dev \
    libjson-c-dev \
    libssh2-1-dev \
    libssl-dev \
    libxml2-dev \
    libxslt1-dev \
    python3-dev \
    python3-setuptools \
    uuid-dev \
    r-base-dev \
    librabbitmq-dev \
    libkrb5-dev \
    #nlohmann-json-dev \
    libaprutil1-dev\
    protobuf-c-compiler\
    libprotobuf-c-dev\
    libfreetype6-dev\
    libcairo2-dev\
    libfribidi-dev\
    libharfbuzz-dev\
"
WORKDIR /zoo-project
COPY . .

RUN set -ex \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive sudo apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && curl -o mapserver-8.0.1.tar.gz -L https://github.com/MapServer/MapServer/releases/download/rel-8-0-1/mapserver-8.0.1.tar.gz \
    && tar -xf mapserver-8.0.1.tar.gz \
    && mkdir msbuild \
    && cd msbuild \
    && cmake ../mapserver-8.0.1 \
       -DWITH_PYTHON=0 \
       -DWITH_CLIENT_WMS=1 \
       -DWITH_CLIENT_WFS=1 \
       -DCMAKE_PREFIX_PATH=/opt/otbtf/ \
       -DWITH_KML=1 \
       -DWITH_POSTGIS=0 \
       -DWITH_GIF=0 \
       -DWITH_OGCAPI=1 \
       -DPROTOBUF_INCLUDE_DIR=/usr/include/google//protobuf-c \
       -DPROTOBUFC_LIBRARY=/usr/lib/x86_64-linux-gnu/libprotobuf-c.so \
       -DPROTOBUF_COMPILER=/usr/bin/protoc-c \
       -DPYTHON_INCLUDE_DIRS=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())")  \
       -DPYTHON_LIBRARY=$(python -c "import distutils.sysconfig as sysconfig; print(sysconfig.get_config_var('LIBDIR'))") \
       -DCMAKE_INSTALL_PREFIX=/opt/otbtf/ \
    && make -j 10 \
    && make install \
    && find /opt/otbtf/ -name "*mapserv*" \
    && cd .. \
    \
    && make -C ./thirds/cgic206 libcgic.a \
    \
    && cd ./zoo-project/zoo-kernel \
    && autoreconf --install && autoconf && autoreconf --install \
    && find / -name proj.h \
    && cat $(find / -name proj.h) \
    # Comments below
    # Issue with GDALAllRegister available from the container
    # It generate a segmentation violation
    # TODO: investigate?
    && ./configure --with-proj=/opt/otbtf \
       --with-run-path=/opt/otbtf/lib \
       --with-gdal-config=/opt/otbtf/bin/gdal-config \
       --with-mapserver=/opt/otbtf --with-ms-version=7 \
       --with-python=/usr --with-pyvers=3.10 \
       --with-json=/usr \
       --with-r=/usr \
       --prefix=/usr \
       --with-otb=/opt/otbtf --with-itk=/usr --with-otb-version=8.1 --with-itk-version=4.13 \
    && find /opt/otbtf -name mapserver.h \
    && make -j4 \
    && sudo make install \
    \
    # TODO: why not copied by 'make'?
    #&& sudo cp zoo_loader_fpm zoo_loader.cgi main.cfg /usr/lib/cgi-bin/ \
    && sudo cp zoo_loader.cgi main.cfg /usr/lib/cgi-bin/ \
    && sudo cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && sudo cp ../zoo-services/utils/open-api/cgi-env/* /usr/lib/cgi-bin/ \
    && sudo cp ../zoo-services/hello-py/cgi-env/* /usr/lib/cgi-bin/ \
    && sudo cp ../zoo-services/hello-js/cgi-env/* /usr/lib/cgi-bin/ \
    && sudo cp ../zoo-services/hello-r/cgi-env/* /usr/lib/cgi-bin/ \
    && sudo cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && sudo cp ../zoo-api/r/minimal.r /usr/lib/cgi-bin/ \
    \
    # Install Basic Authentication sample
    && cd ../zoo-services/utils/security/basicAuth \
    && make \
    && sudo cp cgi-env/* /usr/lib/cgi-bin \
    && cd ../../../../zoo-kernel \
    \
    && for i in  $(ls ./locale/po/*po | grep -v utf8 | grep -v message); do \
         sudo mkdir -p /usr/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES; \
         sudo msgfmt  $i -o /usr/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES/zoo-kernel.mo ; \
         sudo mkdir -p /usr/local/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES; \
         sudo msgfmt  $i -o /usr/local/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES/zoo-kernel.mo ; \
       done  \
    \
    #&& for lang in fr_FR ; do msgcat $(find ../zoo-services/ -name "${lang}.po") -o ${lang}.po ; done \
    && for lang in fr_FR ; do\
       find ../zoo-services/ -name "${lang}*" ; \
       sudo msgcat $(find ../zoo-services/ -name "${lang}*") -o ${lang}.po ; \
       sudo msgfmt ${lang}.po -o /usr/share/locale/${lang}/LC_MESSAGES/zoo-services.mo; \
       sudo msgfmt ${lang}.po -o /usr/local/share/locale/${lang}/LC_MESSAGES/zoo-services.mo; \
       done \
    #&& msgfmt ../zoo-services/utils/open-api/locale/po/fr_FR.po -o /usr/share/locale/fr_FR/LC_MESSAGES/zoo-services.mo \
    #&& msgfmt ../zoo-services/utils/open-api/locale/po/fr_FR.po -o /usr/local/share/locale/fr_FR/LC_MESSAGES/zoo-services.mo \
    && sudo cp oas.cfg /usr/lib/cgi-bin/ \
    \
    # TODO: main.cfg is not processed \
    #&& prefix=/usr sudo envsubst < main.cfg > /usr/lib/cgi-bin/main.cfg \
    \
    #Comment lines below from here if no OTB \
    && mkdir otb_build \
    && cd otb_build \
    && cmake ../../../thirds/otb2zcfg \
    && make \
    && mkdir OTB \
    && cd OTB \
    && ITK_AUTOLOAD_PATH=/opt/otbtf/lib/otb/applications/ ../otb2zcfg \
    && sudo mkdir /usr/lib/cgi-bin/OTB \
    && sudo cp *zcfg /usr/lib/cgi-bin/OTB \
    #&& for i in *zcfg; do cp $i /usr/lib/cgi-bin/$i ; j="$(echo $i | sed "s:.zcfg::g")" ; sed "s:$j:OTB_$j:g" -i  /usr/lib/cgi-bin/OTB_$i ; done \
    #Comment lines before this one if no OTB \
    \
    #Comment lines below from here if no SAGA \
    #&& cd .. \
    #&& make -C ../../../thirds/saga2zcfg \
    #&& mkdir zcfgs \
    #&& cd zcfgs \
    #&& dpkg -L saga \
    #&& export MODULE_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/saga/ \
    #&& export SAGA_MLB=/usr/lib/x86_64-linux-gnu/saga/ \
    #&& ssudo ln -s /usr/lib/x86_64-linux-gnu/saga/ /usr/lib/saga \
    #&& ../../../../thirds/saga2zcfg/saga2zcfg \
    #&& sudo mkdir /usr/lib/cgi-bin/SAGA \
    #&& ls \
    #&& sudo cp -r * /usr/lib/cgi-bin/SAGA \
    #Remove OTB if not built or SAGA if no SAGA \
    #&& for j in OTB ; do for i in $(find /usr/lib/cgi-bin/$j/ -name "*zcfg"); do sudo sed "s:image/png:image/png\n     useMapserver = true\n     msClassify = true:g;s:text/xml:text/xml\n     useMapserver = true:g;s:mimeType = application/x-ogc-aaigrid:mimeType = application/x-ogc-aaigrid\n   </Supported>\n   <Supported>\n     mimeType = image/png\n     useMapserver=true:g" -i $i; done; done \
    \
    && cd ../.. \
    #Comment lines before this one if nor OTB nor SAGA \
    \
    && sudo apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && sudo rm -rf /var/lib/apt/lists/*

#
# Optional zoo modules build.
#
FROM base AS builder2
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    bison \
    flex \
    make \
    autoconf \
    g++ \
    gcc \
    libc-dev \
    libfcgi-dev \
    #libgdal-dev \
    libxml2-dev \
    libxslt1-dev \
    libcgal-dev \
"
WORKDIR /zoo-project
COPY ./zoo-project/zoo-services ./zoo-project/zoo-services

# From zoo-kernel
COPY --from=builder1 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /opt/otbtf/ /opt/otbtf/
COPY --from=builder1 /usr/lib/libzoo_service.so.2.0 /usr/lib/libzoo_service.so.2.0
COPY --from=builder1 /usr/lib/libzoo_service.so /usr/lib/libzoo_service.so
#COPY --from=builder1 /usr/com/zoo-project/ /usr/com/zoo-project/
COPY --from=builder1 /usr/include/zoo/ /usr/include/zoo/

# Additional files from bulder2
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/ZOOMakefile.opts /zoo-project/zoo-project/zoo-kernel/ZOOMakefile.opts
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/sqlapi.h /zoo-project/zoo-project/zoo-kernel/sqlapi.h
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/service.h /zoo-project/zoo-project/zoo-kernel/service.h
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/service_internal.h /zoo-project/zoo-project/zoo-kernel/service_internal.h
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/version.h /zoo-project/zoo-project/zoo-kernel/version.h

RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && cd ./zoo-project/zoo-services/utils/status \
    && make \
    && mkdir -p /usr/com/zoo-project \
    && make install \
    \
    #&& cd ../../cgal \
    #&& make \
    #&& cp cgi-env/* /usr/lib/cgi-bin/ \
    \
    && cd ../.. \
    # Build OGR Services
    && for i in base-vect-ops ogr2ogr; do \
       cd ../zoo-services/ogr/$i && \
       make && \
       sudo cp cgi-env/* /usr/lib/cgi-bin/ && \
       cd ../.. ; \
    done \
    \
    && cd ../zoo-services/gdal/ \
    && for i in contour dem grid profile translate warp ; do cd $i ; make && sudo cp cgi-env/* /usr/lib/cgi-bin/ ; cd .. ; done \
    \
    && sudo apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && sudo rm -rf /var/lib/apt/lists/*

#
# Optional zoo demos download.
#
FROM base AS demos
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    git \
    \
"
WORKDIR /zoo-project

RUN set -ex \
    && sudo apt-get update && sudo apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && git clone --depth=1  https://github.com/ZOO-Project/examples.git \
    && git clone --depth=1 https://github.com/swagger-api/swagger-ui.git \
    && sudo git clone --depth=1 https://github.com/WPS-Benchmarking/cptesting.git /testing \
    && sudo git clone --depth=1 https://www.github.com/singularityhub/singularity-cli.git /singularity-cli \
    \
    && sudo apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && sudo rm -rf /var/lib/apt/lists/*

#
# Runtime image with apache2.
#
FROM base AS runtime
ARG DEBIAN_FRONTEND=noninteractive
ARG RUN_DEPS=" \
    apache2 \
    curl \
    #cgi-mapserver \
    #mapserver-bin \
    xsltproc \
    libxml2-utils \
    gnuplot \
    locales \
    libapache2-mod-fcgid \
    libssl3 \
    python3-setuptools \
    #Uncomment the line below to add vi editor \
    #vim \
    #Uncomment the lines below to add debuging \
    #valgrind \
    #gdb \
"
ARG BUILD_DEPS=" \
    make \
    g++ \
    gcc \
    libgdal-dev \
    python3-dev \
"
# For Azure use, uncomment bellow
#ARG SERVER_URL="http://zooprojectdemo.azurewebsites.net/"
#ARG WS_SERVER_URL="ws://zooprojectdemo.azurewebsites.net"
# For basic usage
ARG SERVER_URL="http://localhost/"
ARG WS_SERVER_URL="ws://localhost"

# For using another port than 80, uncomment below.
# remember to also change the ports in docker-compose.yml
#ARG PORT=8090

WORKDIR /zoo-project
COPY ./docker/startUp.sh /

# From zoo-kernel
COPY --from=builder1 /opt/otbtf/ /opt/otbtf/
COPY --from=builder1 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /usr/lib/libzoo_service.so.2.0 /usr/lib/libzoo_service.so.2.0
COPY --from=builder1 /usr/lib/libzoo_service.so /usr/lib/libzoo_service.so
#COPY --from=builder1 /usr/com/zoo-project/ /usr/com/zoo-project/
COPY --from=builder1 /usr/include/zoo/ /usr/include/zoo/
COPY --from=builder1 /usr/share/locale/ /usr/share/locale/
#COPY --from=builder1 /zoo-project/zoo-project/zoo-services/SAGA/examples/ /var/www/html/examples/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/OTB/examples/ /var/www/html/examples/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/cgal/examples/ /var/www/html/examples/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/utils/open-api/templates/index.html /var/www/index.html
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/utils/open-api/static /var/www/html/static
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/echo-py/cgi-env/ /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/docker/.htaccess /var/www/html/.htaccess
COPY --from=builder1 /zoo-project/docker/default.conf /000-default.conf
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/utils/open-api/server/publish.py /usr/lib/cgi-bin/publish.py

# From optional zoo modules
COPY --from=builder2 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/docker/oas.cfg /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/docker/main.cfg /usr/lib/cgi-bin/
#COPY --from=builder2 /usr/com/zoo-project/ /usr/com/zoo-project/

# From optional zoo demos
COPY --from=demos /singularity-cli/ /singularity-cli/
COPY --from=demos /testing/ /testing/
COPY --from=demos /zoo-project/examples/data/ /usr/com/zoo-project/
COPY --from=demos /zoo-project/examples/ /var/www/html/
COPY --from=demos /zoo-project/swagger-ui /var/www/html/swagger-ui

COPY ./docker/mdl4eo/requirements.txt /tmp


RUN set -ex \
    && sudo apt-get update && sudo apt-get install -y --no-install-recommends $RUN_DEPS $BUILD_DEPS \
    \
    && sudo sed "s=https://petstore.swagger.io/v2/swagger.json=${SERVER_URL}/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/* \
    && sudo sed "s=http://localhost=$SERVER_URL=g" -i /var/www/html/.htaccess \
    && sudo sed "s=http://localhost=$SERVER_URL=g;s=publisherUr\=$SERVER_URL=publisherUrl\=http://localhost=g;s=ws://localhost=$WS_SERVER_URL=g" -i /usr/lib/cgi-bin/oas.cfg \
    && sudo sed "s=http://localhost=$SERVER_URL=g" -i /usr/lib/cgi-bin/main.cfg \
    && for i in $(find /usr/share/locale/ -name "zoo-kernel.mo"); do \
         j=$(echo $i | sed "s:/usr/share/locale/::g;s:/LC_MESSAGES/zoo-kernel.mo::g"); \
         sudo locale-gen $j ; \
         sudo localedef -i $j -c -f UTF-8 -A /usr/share/locale/locale.alias ${j}.UTF-8; \
       done \
    && sudo mv  /var/www/html/swagger-ui/dist  /var/www/html/swagger-ui/oapip \
    && mkdir /tmp/zTmp \
    && sudo ln -s /tmp/zTmp /var/www/html/temp \
    #&& sudo ln -s /usr/lib/x86_64-linux-gnu/saga/ /usr/lib/saga \
    && sudo ln -s /testing /var/www/html/cptesting \
    && sudo rm -rf /var/lib/apt/lists/* \
    && sudo cp /000-default.conf /etc/apache2/sites-available/ \
    #&& export CPLUS_INCLUDE_PATH=/usr/include/gdal \
    #&& export C_INCLUDE_PATH=/usr/include/gdal \
    && sudo pip3 install --upgrade pip setuptools wheel \
    # see various issue reported about _2to3 invocation and setuptools < 58.0 \
    #&& sudo python3 -m pip install --upgrade --no-cache-dir setuptools==57.5.0 \
    #&& sudo pip3 install GDAL==2.4.2 \
    && sudo pip3 install GDAL Cheetah3 redis spython onnxruntime tensorflow  tf2onnx \
    && sudo pip3 install -r /tmp/requirements.txt \
    && sudo sed "s:AllowOverride None:AllowOverride All:g" -i /etc/apache2/apache2.conf \
    \
    # For using another port than 80, uncomment below. \
    # remember to also change the ports in docker-compose.yml \
    # && sed "s:Listen 80:Listen $PORT:g" -i /etc/apache2/ports.conf \
    \
    && mkdir -p /tmp/zTmp/statusInfos \
    && sudo chown www-data:www-data -R /tmp/zTmp /usr/com/zoo-project \
    && sudo chmod 755 /startUp.sh \
    \
    # remove invalid zcfgs \
    && sudo rm /usr/lib/cgi-bin/OTB/TestApplication.zcfg /usr/lib/cgi-bin/OTB/StereoFramework.zcfg \
    \
    # Enable apache modules
    && sudo a2enmod cgi rewrite ssl \
    \
    # Cleanup \
    && sudo apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && sudo rm -rf /var/lib/apt/lists/*


# service namespaces parent folder
RUN sudo mkdir -p /opt/zooservices_namespaces \
    && sudo chmod -R 700 /opt/zooservices_namespaces \
    && sudo chown -R www-data /opt/zooservices_namespaces

USER root
# For using another port than 80, change the value below.
# remember to also change the ports in docker-compose.yml
EXPOSE 80
CMD /usr/sbin/apache2ctl -D FOREGROUND
