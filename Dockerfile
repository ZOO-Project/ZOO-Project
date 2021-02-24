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
    libfcgi-dev \
    libgdal-dev \
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
    && ./configure --with-python=/usr --with-pyvers=3.6 --with-js=/usr --with-mapserver=/usr --with-ms-version=7 --with-json=/usr --with-db-backend --prefix=/usr \
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
    && cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    \
    && cp oas.cfg /usr/lib/cgi-bin/ \
    \
    # TODO: main.cfg is not processed \
    && prefix=/usr envsubst < main.cfg > /usr/lib/cgi-bin/main.cfg \
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
"
WORKDIR /zoo-project

RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && git clone https://github.com/ZOO-Project/examples.git 

#
# Runtime image with apache2.
#
FROM base AS runtime
ARG DEBIAN_FRONTEND=noninteractive
ARG RUN_DEPS=" \
    apache2 \
    curl \
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


RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $RUN_DEPS \
    \
    && rm -rf /var/lib/apt/lists/* \
    && pip3 install Cheetah3 redis\
    && sed "s:AllowOverride None:AllowOverride All:g" -i /etc/apache2/apache2.conf \
    && mkdir -p /tmp/statusInfos \
    && chown www-data:www-data -R /tmp/statusInfos \
    && a2enmod cgi rewrite

EXPOSE 80
CMD /usr/sbin/apache2ctl -D FOREGROUND
