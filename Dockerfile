#
# Base: Ubuntu 18.04 with updates and external packages
#
FROM ubuntu:bionic-20201119 AS base
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    dirmngr \
    gpg-agent \
    software-properties-common \
"
ARG RUN_DEPS=" \
    libcurl3-gnutls \
    libfcgi \
    libmapserver-dev \
    libmozjs185-dev \
    \
    saga \
    libsaga-api-7.3.0 \
    libotb \
    otb-bin \
    \
    libpq5 \
    libpython3.6 \
    libxslt1.1 \
    gdal-bin \
    libcgal13 \
    python3 \
    r-base \
    python3-pip\
"
RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && add-apt-repository ppa:osgeolive/nightly \
    && add-apt-repository ppa:ubuntugis/ppa \
    && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9 \
    && add-apt-repository 'deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran35/' \
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
    bison \
    flex \
    make \
    autoconf \
    gcc \
    gettext-base \
    \
    # Comment lines bellow if nor OTB nor SAGA \
    libotb-dev \
    otb-qgis \
    otb-bin-qt \
    qttools5-dev \
    qttools5-dev-tools \
    qtbase5-dev \
    libqt5opengl5-dev \
    libtinyxml-dev \
    libfftw3-dev \
    cmake \
    libsaga-dev \
    # Comment lines before this one if nor OTB nor SAGA \
    \
    libfcgi-dev \
    libgdal-dev \
    libwxgtk3.0-dev \
    libjson-c-dev \
    libssh2-1-dev \
    libssl-dev \
    libxml2-dev \
    libxslt1-dev \
    python3-dev \
    uuid-dev \
    r-base-dev \
"
WORKDIR /zoo-project
COPY . .

RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && make -C ./thirds/cgic206 libcgic.a \
    \
    && cd ./zoo-project/zoo-kernel \
    && autoconf \
    && ./configure --with-python=/usr --with-pyvers=3.6 --with-js=/usr --with-mapserver=/usr --with-ms-version=7 --with-json=/usr --with-r=/usr --with-db-backend --prefix=/usr --with-otb=/usr/ --with-itk=/usr --with-otb-version=6.6 --with-itk-version=4.12 --with-saga=/usr --with-saga-version=7.2 --with-wx-config=/usr/bin/wx-config \
    && make \
    && make install \
    \
    # TODO: why not copied by 'make'?
    && cp zoo_loader.cgi main.cfg /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-py/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-js/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/utils/open-api/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-py/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-js/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-r/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && cp ../zoo-api/r/minimal.r /usr/lib/cgi-bin/ \
    \
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
    && ITK_AUTOLOAD_PATH=/usr/lib/x86_64-linux-gnu/otb/applications/ ../otb2zcfg \
    && mkdir /usr/lib/cgi-bin/OTB \
    && cp *zcfg /usr/lib/cgi-bin/OTB \
    #&& for i in *zcfg; do cp $i /usr/lib/cgi-bin/$i ; j="$(echo $i | sed "s:.zcfg::g")" ; sed "s:$j:OTB_$j:g" -i  /usr/lib/cgi-bin/OTB_$i ; done \
    #Comment lines before this one if no OTB \
    \
    #Comment lines below from here if no SAGA \
    && cd .. \
    && make -C ../../../thirds/saga2zcfg \
    && mkdir zcfgs \
    && cd zcfgs \
    && dpkg -L saga \
    && export MODULE_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/saga/ \
    && export SAGA_MLB=/usr/lib/x86_64-linux-gnu/saga/ \
    && ln -s /usr/lib/x86_64-linux-gnu/saga/ /usr/lib/saga \
    && ../../../../thirds/saga2zcfg/saga2zcfg \
    && mkdir /usr/lib/cgi-bin/SAGA \
    && ls \
    && cp -r * /usr/lib/cgi-bin/SAGA \
    #Remove OTB if not built or SAGA if no SAGA \
    && for j in OTB SAGA ; do for i in $(find /usr/lib/cgi-bin/$j/ -name "*zcfg"); do sed "s:image/png:image/png\n     useMapserver = true\n     msClassify = true:g;s:text/xml:text/xml\n     useMapserver = true:g;s:mimeType = application/x-ogc-aaigrid:mimeType = application/x-ogc-aaigrid\n   </Supported>\n   <Supported>\n     mimeType = image/png\n     useMapserver=true:g" -i $i; done; done \
    \
    && cd ../.. \
    #Comment lines before this one if nor OTB nor SAGA \
    \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*

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
    libgdal-dev \
    libxml2-dev \
    libxslt1-dev \
    libcgal-dev \
"
WORKDIR /zoo-project
COPY ./zoo-project/zoo-services ./zoo-project/zoo-services

# From zoo-kernel
COPY --from=builder1 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /usr/lib/libzoo_service.so.1.8 /usr/lib/libzoo_service.so.1.8
COPY --from=builder1 /usr/lib/libzoo_service.so /usr/lib/libzoo_service.so
COPY --from=builder1 /usr/com/zoo-project/ /usr/com/zoo-project/
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
    && make install \
    \
    && cd ../../cgal \
    && make \
    && cp cgi-env/* /usr/lib/cgi-bin/ \
    \
    && cd .. \
    && cd ../zoo-services/ogr/base-vect-ops \
    && make \
    && cp cgi-env/* /usr/lib/cgi-bin/ \
    && cd ../.. \
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
    && git clone https://github.com/ZOO-Project/examples.git \
    && git clone https://github.com/swagger-api/swagger-ui.git

#
# Runtime image with apache2.
#
FROM base AS runtime
ARG DEBIAN_FRONTEND=noninteractive
ARG RUN_DEPS=" \
    apache2 \
    curl \
    cgi-mapserver \
    mapserver-bin \
    #Uncomment the line below to add vi editor \
    #vim \
    #Uncomment the lines below to add debuging \
    #valgrind \
    #gdb \
"

# From zoo-kernel
COPY --from=builder1 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /usr/lib/libzoo_service.so.1.8 /usr/lib/libzoo_service.so.1.8
COPY --from=builder1 /usr/lib/libzoo_service.so /usr/lib/libzoo_service.so
COPY --from=builder1 /usr/com/zoo-project/ /usr/com/zoo-project/
COPY --from=builder1 /usr/include/zoo/ /usr/include/zoo/

# From optional zoo modules
COPY --from=builder2 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder2 /usr/com/zoo-project/ /usr/com/zoo-project/

# From optional zoo demos
COPY --from=demos /zoo-project/examples/data/ /usr/com/zoo-project/
COPY --from=demos /zoo-project/examples/ /var/www/html/
COPY --from=demos /zoo-project/swagger-ui /var/www/html/swagger-ui


RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $RUN_DEPS \
    \
    && sed "s=https://petstore.swagger.io/v2/swagger.json=http://localhost/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/index.html \
    && mv  /var/www/html/swagger-ui/dist  /var/www/html/swagger-ui/oapip \
    && ln -s /tmp/ /var/www/html/temp \
    && ln -s /usr/lib/x86_64-linux-gnu/saga/ /usr/lib/saga \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install Cheetah3 redis\
    && sed "s:AllowOverride None:AllowOverride All:g" -i /etc/apache2/apache2.conf \
    && mkdir -p /tmp/statusInfos \
    && chown www-data:www-data -R /tmp/statusInfos /usr/com/zoo-project \
    && a2enmod cgi rewrite

EXPOSE 80
CMD /usr/sbin/apache2ctl -D FOREGROUND
