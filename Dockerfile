#
# Base: Ubuntu 18.04 with updates and external packages
#
FROM ubuntu:24.04 AS base
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    dirmngr \
    gpg-agent \
    software-properties-common \
    wget \
"
ARG RUN_DEPS=" \
    libcurl3t64-gnutls \
    libfcgi-dev \
    libfcgi-bin \
    libmapserver-dev \
    curl \
    \
    saga \
    libsaga-api9 \
    libsaga-dev \
    \
    libpq5 \
    libpython3-dev \
    libxslt1.1 \
    gdal-bin \
    gdal-data \
    python3-gdal \
    python3-pip \
    libcgal-dev \
    librabbitmq4 \
    nlohmann-json3-dev \
    python3 \
    r-base \
    libffi8 \
    libffi-dev \
    nodejs \ 
    npm \
"
RUN set -ex \
    && apt-get update \
    && apt-get install -y --no-install-recommends $BUILD_DEPS software-properties-common gnupg wget curl \
    \
    && apt-get update \
    \
    && apt-get install -y $RUN_DEPS \
    \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*

#
# builder1: base image with zoo-kernel
#
FROM base AS builder1
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    wget \
    lsb-release \
    build-essential \
    bison \
    flex \
    make \
    autoconf \
    autopoint \
    gcc \
    gettext \
    \
    # Comment lines bellow if nor OTB nor SAGA \
    libtinyxml-dev \
    libfftw3-dev \
    cmake \
    # Comment lines before this one if nor OTB nor SAGA \
    git \
    libpq-dev \
    libproj-dev \
    libcurl4-openssl-dev \
    zlib1g-dev \
    libexpat1-dev \
    libgeos-dev \
    libqt5svg5-dev \
    libgdal-dev \
    python3.12-dev \
    libncurses-dev \
    libbz2-dev \
    libreadline-dev \
    libsqlite3-dev \
    libffi-dev \
    liblzma-dev \
    uuid-dev \
    libgdbm-dev \
    libnss3-dev \
    xz-utils \
    tk-dev \
    ca-certificates \
    libwxgtk3.2-dev \
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
    nlohmann-json3-dev \
    libaprutil1-dev \
    libxslt-dev \
    libopengl-dev \
    libhdf5-openmpi-103-1 \
    libnetcdf-c++4-dev \
    libboost-filesystem-dev \
    libboost-dev \
    libboost-system-dev \
    libvtk9-dev libgdcm-dev libgdcm-java libgdcm-tools libvtkgdcm-dev libvtkgdcm-tools python3-vtkgdcm python3-gdcm \
"
WORKDIR /zoo-project
COPY . .

ENV LC_NUMERIC=C
ENV GDAL_DATA=/usr/share/gdal                                                             
ENV PROJ_LIB=/usr/share/proj                                                              
ENV OTB_APPLICATION_PATH=/opt/otb-9.1.1/lib/otb/applications                              
ENV PATH=/opt/otb-9.1.1/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin:/usr/bin:/usr/lib/saga
ENV PYTHONPATH=/opt/otb-9.1.1/lib/otb/python:/opt/otb-9.1.1/lib/otb/python
ENV OTB_INSTALL_DIR=/opt/otb-9.1.1           
ENV SAGA_MLB=/usr/lib/saga  

ENV LD_LIBRARY_PATH=/opt/otb-9.1.1/lib

RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    && wget -P /tmp/otb https://github.com/veogeo/OTB-9-ubuntu24/releases/download/9.1.1/otb-9.1.1-bin.deb \
    && wget -P /tmp/otb https://github.com/veogeo/OTB-9-ubuntu24/releases/download/9.1.1/libotb-dev.deb \
    && wget -P /tmp/otb https://github.com/veogeo/OTB-9-ubuntu24/releases/download/9.1.1/python3-otb-9.1.1.deb \
    && wget -P /tmp/node https://github.com/veogeo/mmomtchev--libnode/releases/download/node-18.x-2025.06/libnode109.deb \
    && wget -P /tmp/node https://github.com/veogeo/mmomtchev--libnode/releases/download/node-18.x-2025.06/libnode-dev.deb \
    && wget -P /tmp/node https://github.com/veogeo/mmomtchev--libnode/releases/download/node-18.x-2025.06/node-addon-api.deb \
    && dpkg -i /tmp/otb/*.deb \
    && dpkg -i --force-all /tmp/node/*.deb \
    && ln -s /usr/lib/libnode.so.108 /usr/lib/libnode.so.109 \
    && ls -l /usr/lib/x86_64-linux-gnu/libfcgi* \
    && make -C ./thirds/cgic206 libcgic.a \
    && sed -i 's|"\$OTB_INSTALL_DIR"/bin/gdal-config|/usr/bin/gdal-config|' /opt/otb-9.1.1/tools/post_install.sh \
    && sed -i 's|"\$OTB_INSTALL_DIR"/bin/curl-config|/usr/bin/curl-config|' /opt/otb-9.1.1/tools/post_install.sh \
    && sed -i 's|^ostype="\$(lsb_release -is)"|ostype="RedHatEnterprise"|' /opt/otb-9.1.1/tools/post_install.sh \
    && sed -i 's|\$OTB_INSTALL_DIR|/opt/otb-9.1.1|g' /opt/otb-9.1.1/tools/sanitize_rpath.sh \
    && cat /opt/otb-9.1.1/tools/sanitize_rpath.sh \
    && . /opt/otb-9.1.1/tools/post_install.sh \
    && cd ./zoo-project/zoo-kernel \ 
    && export CFLAGS="-I/usr/include/mapserver" \
    && export CPPFLAGS="-I/usr/include/mapserver -I/opt/otb-9.1.1/include/OTB-9.1 -I/opt/otb-9.1.1/include/ITK-4.13 -I/usr/include/mapserver -I/usr/include/node -I/usr/share/nodejs/node-addon-api -I/usr/include" \
    && export CXXFLAGS="$CPPFLAGS" \
    && autoconf \
    && autoreconf --install \
    && ./configure --with-rabbitmq=yes --with-python=/usr --with-pyvers=3.12 \
              --with-nodejs=/usr --with-mapserver=/usr --with-ms-version=8 \
              --with-json=/usr --with-r=/usr --with-db-backend --prefix=/usr \
              --with-otb=/opt/otb-9.1.1 --with-itk=/opt/otb-9.1.1 --with-otb-version=9.1 \
              --with-itk-version=4.13 --with-saga=/usr \
              --with-saga-version=9 --with-wx-config=/usr/bin/wx-config \
    && make -j$(nproc) \
    && make -n install \
    && make install \
    \
    # TODO: why not copied by 'make'?
    && cp zoo_loader_fpm zoo_loader.cgi main.cfg /usr/lib/cgi-bin/ \
    && cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/utils/open-api/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-py/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-js/cgi-env/* /usr/lib/cgi-bin/ \
    && cp -r ../zoo-services/hello-nodejs/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/linestringDem/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-r/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && cp ../zoo-api/r/minimal.r /usr/lib/cgi-bin/ \
    \
    # Install Basic Authentication sample
    && cd ../zoo-services/utils/security/basicAuth \
    && make \
    && cp cgi-env/* /usr/lib/cgi-bin \
    && cd ../../../../zoo-kernel \
    \
    && for i in  $(ls ./locale/po/*po | grep -v utf8 | grep -v message); do \
         mkdir -p /usr/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES; \
         msgfmt  $i -o /usr/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES/zoo-kernel.mo ; \
         mkdir -p /usr/local/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES; \
         msgfmt  $i -o /usr/local/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES/zoo-kernel.mo ; \
       done  \
    \
    && npm -g install gdal-async proj4 bower wps-js-52-north \
    && ( cd /usr/lib/cgi-bin/hello-nodejs && npm install ) \
    #&& for lang in fr_FR ; do msgcat $(find ../zoo-services/ -name "${lang}.po") -o ${lang}.po ; done \
    && for lang in fr_FR ; do\
       find ../zoo-services/ -name "${lang}*" ; \
       msgcat $(find ../zoo-services/ -name "${lang}*") -o ${lang}.po ; \
       msgfmt ${lang}.po -o /usr/share/locale/${lang}/LC_MESSAGES/zoo-services.mo; \
       msgfmt ${lang}.po -o /usr/local/share/locale/${lang}/LC_MESSAGES/zoo-services.mo; \
       done \
    #&& msgfmt ../zoo-services/utils/open-api/locale/po/fr_FR.po -o /usr/share/locale/fr_FR/LC_MESSAGES/zoo-services.mo \
    #&& msgfmt ../zoo-services/utils/open-api/locale/po/fr_FR.po -o /usr/local/share/locale/fr_FR/LC_MESSAGES/zoo-services.mo \
    && cp oas.cfg /usr/lib/cgi-bin/ \
    \
    # TODO: main.cfg is not processed \
    && prefix=/usr envsubst < main.cfg > /usr/lib/cgi-bin/main.cfg \
    \
    #Comment lines below from here if no OTB \
    && mkdir otb_build \
    && cd otb_build \
    && cmake ../../../thirds/otb2zcfg \
    && make \
    && mkdir OTB \
    && cd OTB \
    && ITK_AUTOLOAD_PATH="$OTB_INSTALL_DIR"/lib/otb/applications/ ../otb2zcfg \
    && mkdir /usr/lib/cgi-bin/OTB \
    && cp *zcfg /usr/lib/cgi-bin/OTB \
    #&& for i in *zcfg; do cp $i /usr/lib/cgi-bin/$i ; j="$(echo $i | sed "s:.zcfg::g")" ; sed "s:$j:$j:g" -i  /usr/lib/cgi-bin/$i ; done \
    #Comment lines before this one if no OTB \
    \
    #Comment lines below from here if no SAGA \
    && cd .. \
    && make -C ../../../thirds/saga2zcfg \
    && mkdir zcfgs \
    && cd zcfgs \
    #&& dpkg -L saga \
    && export MODULE_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/saga/ \
    && export SAGA_MLB=/usr/lib/x86_64-linux-gnu/saga/ \
    && ln -s /usr/lib/x86_64-linux-gnu/saga/ /usr/lib/saga \
    && ../../../../thirds/saga2zcfg/saga2zcfg \
    && mkdir /usr/lib/cgi-bin/SAGA \
    #&& ls \
    && cp -r * /usr/lib/cgi-bin/SAGA \
    #Remove OTB if not built or SAGA if no SAGA \
    && for j in OTB SAGA ; do for i in $(find /usr/lib/cgi-bin/$j/ -name "*zcfg"); do sed "s:image/png:image/png\n     useMapserver = true\n     msClassify = true:g;s:text/xml:text/xml\n     useMapserver = true:g;s:mimeType = application/x-ogc-aaigrid:mimeType = application/x-ogc-aaigrid\n   </Supported>\n   <Supported>\n     mimeType = image/png\n     useMapserver=true:g" -i $i; done; done \
    \
    && cd ../.. \
    #Comment lines before this one if nor OTB nor SAGA \
    \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*

# The originally Generated TrainRegression.zcfg file have issues and it don't allow the processes to run
# This file is a quick and dirty fix, and this is for override the generated file
# When it is fixed, this file and this line should be removed
# COPY ./docker/TrainRegression.zcfg /usr/lib/cgi-bin/OTB/

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
    libfcgi-bin \
    libgdal-dev \
    libxml2-dev \
    libxslt1-dev \
    libcgal-dev \
    libcgal-qt5-dev \
"
WORKDIR /zoo-project
COPY ./zoo-project/zoo-services ./zoo-project/zoo-services

# From zoo-kernel
COPY --from=builder1 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /usr/lib/libzoo_service.so.2.0 /usr/lib/libzoo_service.so.2.0
COPY --from=builder1 /usr/lib/libzoo_service.so /usr/lib/libzoo_service.so
COPY --from=builder1 /usr/com/zoo-project/ /usr/com/zoo-project/
COPY --from=builder1 /usr/include/zoo/ /usr/include/zoo/

# Additional files from bulder2
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/ZOOMakefile.opts /zoo-project/zoo-project/zoo-kernel/ZOOMakefile.opts
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/sqlapi.h /zoo-project/zoo-project/zoo-kernel/sqlapi.h
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/service.h /zoo-project/zoo-project/zoo-kernel/service.h
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/service_internal.h /zoo-project/zoo-project/zoo-kernel/service_internal.h
COPY --from=builder1 /zoo-project/zoo-project/zoo-kernel/version.h /zoo-project/zoo-project/zoo-kernel/version.h

# Node.js global node_modules
COPY --from=builder1 /usr/local/lib/node_modules/ /usr/local/lib/node_modules/

RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && cd ./zoo-project/zoo-services/utils/status \
    && make \
    && make install \
    \
    && cd ../../cgal \
    && make \
    && cp cgi-env/* /usr/lib/cgi-bin/ \
    \
    && cd .. \
    # Build OGR Services
    && for i in base-vect-ops ogr2ogr; do \
       cd ../zoo-services/ogr/$i && \
       make && \
       cp cgi-env/* /usr/lib/cgi-bin/ && \
       cd ../.. ; \
    done \
    \
    && cd ../zoo-services/gdal/ \
    && for i in contour dem grid profile translate warp ; do cd $i ; make && cp cgi-env/* /usr/lib/cgi-bin/ ; cd .. ; done \
    \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*

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
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && git clone --depth=1  https://github.com/ZOO-Project/examples.git \
    && git clone --depth=1 https://github.com/swagger-api/swagger-ui.git \
    && git clone --depth=1 https://github.com/WPS-Benchmarking/cptesting.git /testing \
    && git clone --depth=1 https://www.github.com/singularityhub/singularity-cli.git /singularity-cli \
    \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*

#
# Runtime image with apache2.
#
FROM base AS runtime
ARG DEBIAN_FRONTEND=noninteractive
ARG RUN_DEPS=" \
    nginx \
    lighttpd \
    curl \
    cgi-mapserver \
    mapserver-bin \
    xsltproc \
    libxml2-utils \
    gnuplot \
    locales \
    python3-setuptools \
    #Uncomment the line below to add vi editor \
    vim \
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
ARG SERVER_HOST="localhost"
ARG SERVER_URL="http://localhost"
ARG WS_SERVER_URL="ws://localhost"
ENV SERVER_HOST="$SERVER_HOST"
ENV SERVER_URL="$SERVER_URL"
ENV WS_SERVER_URL="$WS_SERVER_URL" 

ENV LD_LIBRARY_PATH=/opt/otb-9.1.1/lib:/usr/lib/saga
ENV GDAL_DATA=/usr/share/gdal
ENV PROJ_LIB=/usr/share/proj
ENV OTB_APPLICATION_PATH=/opt/otb-9.1.1/lib/otb/applications
ENV PATH=/opt/otb-9.1.1/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin:/usr/bin:/usr/lib/saga
ENV PYTHONPATH=/opt/otb-9.1.1/lib/otb/python:/opt/otb-9.1.1/lib/otb/python
ENV OTB_INSTALL_DIR=/opt/otb-9.1.1
ENV SAGA_MLB=/usr/lib/saga

# For using another port than 80, uncomment below.
# remember to also change the ports in docker-compose.yml
#ARG PORT=8090

WORKDIR /zoo-project
COPY ./docker/startUp.sh /
COPY ./docker/nginx-start.sh /
COPY ./docker/70-zoo-loader-cgi.conf /etc/lighttpd/conf-available/70-zoo-loader-cgi.conf

# From zoo-kernel
COPY --from=builder1 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /usr/lib/libzoo_service.so.2.0 /usr/lib/libzoo_service.so.2.0
COPY --from=builder1 /usr/lib/libzoo_service.so /usr/lib/libzoo_service.so
COPY --from=builder1 /usr/com/zoo-project/ /usr/com/zoo-project/
COPY --from=builder1 /usr/include/zoo/ /usr/include/zoo/
COPY --from=builder1 /usr/share/locale/ /usr/share/locale/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/SAGA/examples/ /var/www/html/examples/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/OTB/examples/ /var/www/html/examples/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/cgal/examples/ /var/www/html/examples/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/utils/open-api/templates/index.html /var/www/index.html
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/utils/open-api/static /var/www/html/static
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/echo-py/cgi-env/ /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/deploy-py/cgi-env/ /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/undeploy-py/cgi-env/ /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/docker/nginx-default.conf /etc/nginx/sites-available/zooproject
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/utils/open-api/server/publish.py /usr/lib/cgi-bin/publish.py

# Node.js global node_modules
COPY --from=builder1 /usr/local/lib/node_modules/ /usr/local/lib/node_modules/

# From optional zoo modules
COPY --from=builder2 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/docker/oas.cfg /usr/lib/cgi-bin/
COPY --from=builder1 /zoo-project/docker/main.cfg /usr/lib/cgi-bin/
COPY --from=builder2 /usr/com/zoo-project/ /usr/com/zoo-project/

# From optional zoo demos
COPY --from=demos /singularity-cli/ /singularity-cli/
COPY --from=demos /testing/ /testing/
COPY --from=demos /zoo-project/examples/data/ /usr/com/zoo-project/
COPY --from=demos /zoo-project/examples/ /var/www/html/
COPY --from=demos /zoo-project/swagger-ui /var/www/html/swagger-ui


RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $RUN_DEPS $BUILD_DEPS \
    \
    && sed "s=https://petstore.swagger.io/v2/swagger.json=${SERVER_URL}/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/* \
    && sed "s=localhost=$SERVER_HOST=g" -i /etc/nginx/sites-available/zooproject \
    && cp /etc/nginx/sites-available/zooproject /etc/nginx/sites-available/default \
    && sed "s=http://localhost=$SERVER_URL=g;s=publisherUr\=$SERVER_URL=publisherUrl\=http://localhost=g;s=ws://localhost=$WS_SERVER_URL=g" -i /usr/lib/cgi-bin/oas.cfg \
    && sed "s=http://localhost=$SERVER_URL=g" -i /usr/lib/cgi-bin/main.cfg \
    && for i in $(find /usr/share/locale/ -name "zoo-kernel.mo"); do \
         j=$(echo $i | sed "s:/usr/share/locale/::g;s:/LC_MESSAGES/zoo-kernel.mo::g"); \
         locale-gen $j ; \
         localedef -i $j -c -f UTF-8 -A /usr/share/locale/locale.alias ${j}.UTF-8; \
       done \
    && mv /var/www/html/swagger-ui/dist  /var/www/html/swagger-ui/oapip \
    && mkdir /tmp/zTmp \
    && ln -s /tmp/zTmp /var/www/html/temp \
    && ln -s /usr/lib/x86_64-linux-gnu/saga/ /usr/lib/saga \
    && ln -s /testing /var/www/html/cptesting \
    && rm -rf /var/lib/apt/lists/* \
    && export CPLUS_INCLUDE_PATH=/usr/include/gdal \
    && export C_INCLUDE_PATH=/usr/include/gdal 

RUN set -ex \
    && export OTB_INSTALL_DIR="/opt/otb-9.1.1" \
    && apt update \
    && apt install -y python3-spython python3-cheetah python3-redis wget libboost-filesystem-dev \
    && rm -rf /tmp/otb /tmp/node \
    && wget -P /tmp/otb https://github.com/veogeo/OTB-9-ubuntu24/releases/download/9.1.1/otb-9.1.1-bin.deb \
    && wget -P /tmp/otb https://github.com/veogeo/OTB-9-ubuntu24/releases/download/9.1.1/libotb-dev.deb \
    && wget -P /tmp/otb https://github.com/veogeo/OTB-9-ubuntu24/releases/download/9.1.1/python3-otb-9.1.1.deb \
    && wget -P /tmp/node https://github.com/veogeo/mmomtchev--libnode/releases/download/node-18.x-2025.06/libnode109.deb \
    && wget -P /tmp/node https://github.com/veogeo/mmomtchev--libnode/releases/download/node-18.x-2025.06/libnode-dev.deb \
    && wget -P /tmp/node https://github.com/veogeo/mmomtchev--libnode/releases/download/node-18.x-2025.06/node-addon-api.deb \
    && dpkg -i /tmp/otb/*.deb \
    && dpkg -i --force-all /tmp/node/*.deb \
    && ln -s /usr/lib/libnode.so.108 /usr/lib/libnode.so.109 \
    && sed -i 's|"\$OTB_INSTALL_DIR"/bin/gdal-config|/usr/bin/gdal-config|' /opt/otb-9.1.1/tools/post_install.sh \
    && sed -i 's|"\$OTB_INSTALL_DIR"/bin/curl-config|/usr/bin/curl-config|' /opt/otb-9.1.1/tools/post_install.sh \
    && sed -i 's|^ostype="\$(lsb_release -is)"|ostype="RedHatEnterprise"|' /opt/otb-9.1.1/tools/post_install.sh \
    && sed -i 's|\$OTB_INSTALL_DIR|/opt/otb-9.1.1|g' /opt/otb-9.1.1/tools/sanitize_rpath.sh \
    && cat /opt/otb-9.1.1/tools/sanitize_rpath.sh \
    && . /opt/otb-9.1.1/tools/post_install.sh \
    && rm -rf /tmp/otb /tmp/node \
    && apt remove -y wget \
    \
    && mkdir -p /tmp/zTmp/statusInfos \
    && chown www-data:www-data -R /tmp/zTmp /usr/com/zoo-project /usr/lib/cgi-bin/ \
    && chmod 755 /startUp.sh \
    && chmod +x /nginx-start.sh \
    && rm /usr/lib/cgi-bin/SAGA/grid_gridding/0.zcfg \
    && rm /usr/lib/cgi-bin/SAGA/grid_gridding/6.zcfg \
    && rm /usr/lib/cgi-bin/SAGA/grid_gridding/9.zcfg \
    && rm /usr/lib/cgi-bin/SAGA/pj_georeference/4.zcfg \
    && rm /usr/lib/cgi-bin/SAGA/pj_proj4/14.zcfg \
    && rm /usr/lib/cgi-bin/SAGA/pj_proj4/17.zcfg \
    && rm /usr/lib/cgi-bin/SAGA/pj_proj4/23.zcfg \
    && rm /usr/lib/cgi-bin/SAGA/pj_proj4/24.zcfg \
    # remove invalid zcfgs \
    ### && rm /usr/lib/cgi-bin/SAGA/db_pgsql/6.zcfg /usr/lib/cgi-bin/SAGA/imagery_tools/8.zcfg /usr/lib/cgi-bin/SAGA/grid_calculus_bsl/0.zcfg /usr/lib/cgi-bin/SAGA/grids_tools/1.zcfg /usr/lib/cgi-bin/SAGA/grid_visualisation/1.zcfg /usr/lib/cgi-bin/SAGA/ta_lighting/2.zcfg /usr/lib/cgi-bin/OTB/TestApplication.zcfg /usr/lib/cgi-bin/OTB/StereoFramework.zcfg \
    # Update SAGA zcfg \ 
    && sed "s:AllowedValues =    <Default>:AllowedValues =\n    <Default>:g" -i /usr/lib/cgi-bin/SAGA/*/*zcfg \
    && sed "s:Title = $:Title = No title found:g" -i /usr/lib/cgi-bin/SAGA/*/*.zcfg \
    \
    && sed "s=80=9090=g" -i /etc/lighttpd/lighttpd.conf \
    && sed -i '/include_shell .*use-ipv6\.pl/d' /etc/lighttpd/lighttpd.conf \
    && lighty-enable-mod zoo-loader-cgi \
    # Cleanup \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*

# service namespaces parent folder
RUN mkdir -p /opt/zooservices_namespaces && chmod -R 700 /opt/zooservices_namespaces && chown -R www-data /opt/zooservices_namespaces

# For using another port than 80, change the value below.
# remember to also change the ports in docker-compose.yml
EXPOSE 80
CMD ["/nginx-start.sh"]
