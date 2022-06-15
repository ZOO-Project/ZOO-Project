#
# Base: Ubuntu 18.04 with updates and external packages
#
FROM mmomtchev:ubuntu-libnode AS base
ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_DEPS=" \
    dirmngr \
    gpg-agent \
    software-properties-common \
    #wget \
"
ARG RUN_DEPS=" \
    libcurl3-gnutls \
    libfcgi \
    libmapserver-dev \
    curl \
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
    librabbitmq4 \
    nlohmann-json-dev \
    python3 \
    r-base \
    python3-pip\
"
RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS  \
    \
    #&& add-apt-repository ppa:osgeolive/nightly \
    #&& add-apt-repository ppa:ubuntugis/ubuntugis-unstable \
    && add-apt-repository ppa:ubuntugis/ppa \
    && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys E298A3A825C0D65DFD57CBB651716619E084DAB9 \
    && add-apt-repository 'deb https://cloud.r-project.org/bin/linux/ubuntu bionic-cran35/' \
    \
    && apt-get install -y $RUN_DEPS \
    \
    && curl -o libmozjs185-1.0_1.8.5-1.0.0+dfsg-7_amd64.deb http://launchpadlibrarian.net/309343863/libmozjs185-1.0_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && curl -o libmozjs185-dev_1.8.5-1.0.0+dfsg-7_amd64.deb http://launchpadlibrarian.net/309343864/libmozjs185-dev_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && dpkg --force-depends -i libmozjs185-1.0_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && dpkg --force-depends -i libmozjs185-dev_1.8.5-1.0.0+dfsg-7_amd64.deb \
    && apt  -y --fix-broken install \
    && rm /libmozjs185*.deb \
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
    gettext \
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
    git \
    libfcgi-dev \
    libgdal-dev \
    libwxgtk3.0-dev \
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
    nlohmann-json-dev \
"
WORKDIR /zoo-project
COPY . .

RUN set -ex \
    && apt-get update && apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && make -C ./thirds/cgic206 libcgic.a \
    \
    && cd ./zoo-project/zoo-kernel \
    #&& git clone https://github.com/json-c/json-c.git \
    #&& mkdir json-c-build \
    #&& cd json-c-build \
    #&& cmake ../json-c -DCMAKE_INSTALL_PREFIX=/usr/local \
    #&& make && make install \
    #&& cd .. \
    #&& sed "s:-ljson-c:-Wl,-rpath,/usr/local/lib /usr/local/lib/libjson-c.so.5 :g" -i configure.ac \
    && autoconf \
    && find /usr -name otbWrapperApplication.h \
    && ./configure --with-rabbitmq=yes --with-python=/usr --with-pyvers=3.6 --with-js=/usr --with-mapserver=/usr --with-ms-version=7 --with-json=/usr --with-r=/usr --with-db-backend --prefix=/usr --with-otb=/usr/ --with-itk=/usr --with-otb-version=6.6 --with-itk-version=4.12 --with-saga=/usr --with-saga-version=7.2 --with-wx-config=/usr/bin/wx-config \
    && make -j4 \
    && make install \
    \
    # TODO: why not copied by 'make'?
    && cp zoo_loader_fpm zoo_loader.cgi main.cfg /usr/lib/cgi-bin/ \
    && cp zoo_loader.cgi main.cfg /usr/lib/cgi-bin/ \
    && cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/utils/open-api/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-py/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-js/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-services/hello-r/cgi-env/* /usr/lib/cgi-bin/ \
    && cp ../zoo-api/js/* /usr/lib/cgi-bin/ \
    && cp ../zoo-api/r/minimal.r /usr/lib/cgi-bin/ \
    && for i in  $(ls ./locale/po/*po | grep -v utf8 | grep -v message); do \
         mkdir -p /usr/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES; \
         msgfmt  $i -o /usr/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES/zoo-kernel.mo ; \
         mkdir -p /usr/local/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES; \
         msgfmt  $i -o /usr/local/share/locale/$(echo $i| sed "s:./locale/po/::g;s:.po::g")/LC_MESSAGES/zoo-kernel.mo ; \
       done  \
    \
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
    && git clone https://github.com/swagger-api/swagger-ui.git \
    && git clone https://github.com/WPS-Benchmarking/cptesting.git /testing \
    && git clone https://www.github.com/singularityhub/singularity-cli.git /singularity-cli \
    \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*

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
    xsltproc \
    libxml2-utils \
    gnuplot \
    locales \
    libapache2-mod-fcgid \
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
ARG SERVER_URL="http://localhost/"
ARG WS_SERVER_URL="ws://localhost"

# For using another port than 80, uncomment below.
# remember to also change the ports in docker-compose.yml
#ARG PORT=8090

WORKDIR /zoo-project
COPY ./docker/startUp.sh /

# From zoo-kernel
COPY --from=builder1 /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=builder1 /usr/lib/libzoo_service.so.1.8 /usr/lib/libzoo_service.so.1.8
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
COPY --from=builder1 /zoo-project/docker/.htaccess /var/www/html/.htaccess
COPY --from=builder1 /zoo-project/docker/default.conf /000-default.conf
COPY --from=builder1 /zoo-project/zoo-project/zoo-services/utils/open-api/server/publish.py /usr/lib/cgi-bin/publish.py

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
    && sed "s=https://petstore.swagger.io/v2/swagger.json=${SERVER_URL}/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/index.html \
    && sed "s=http://localhost=$SERVER_URL=g" -i /var/www/html/.htaccess \
    && sed "s=http://localhost=$SERVER_URL=g;s=publisherUr\=$SERVER_URL=publisherUrl\=http://localhost=g;s=ws://localhost=$WS_SERVER_URL=g" -i /usr/lib/cgi-bin/oas.cfg \
    && sed "s=http://localhost=$SERVER_URL=g" -i /usr/lib/cgi-bin/main.cfg \
    && for i in $(find /usr/share/locale/ -name "zoo-kernel.mo"); do \
         j=$(echo $i | sed "s:/usr/share/locale/::g;s:/LC_MESSAGES/zoo-kernel.mo::g"); \
         locale-gen $j ; \
         localedef -i $j -c -f UTF-8 -A /usr/share/locale/locale.alias ${j}.UTF-8; \
       done \
    && mv  /var/www/html/swagger-ui/dist  /var/www/html/swagger-ui/oapip \
    && mkdir /tmp/zTmp \
    && ln -s /tmp/zTmp /var/www/html/temp \
    && ln -s /usr/lib/x86_64-linux-gnu/saga/ /usr/lib/saga \
    && ln -s /testing /var/www/html/cptesting \
    && rm -rf /var/lib/apt/lists/* \
    && cp /000-default.conf /etc/apache2/sites-available/ \
    && export CPLUS_INCLUDE_PATH=/usr/include/gdal \
    && export C_INCLUDE_PATH=/usr/include/gdal \
    && pip3 install --upgrade pip setuptools wheel \
    # see various issue reported about _2to3 invocation and setuptools < 58.0 \
    && python3 -m pip install --upgrade --no-cache-dir setuptools==57.5.0 \
    && pip3 install GDAL==2.4.2 \
    && pip3 install Cheetah3 redis spython \
    && sed "s:AllowOverride None:AllowOverride All:g" -i /etc/apache2/apache2.conf \
    \
    # For using another port than 80, uncomment below. \
    # remember to also change the ports in docker-compose.yml \
    # && sed "s:Listen 80:Listen $PORT:g" -i /etc/apache2/ports.conf \
    \
    && mkdir -p /tmp/zTmp/statusInfos \
    && chown www-data:www-data -R /tmp/zTmp /usr/com/zoo-project \
    && chmod 755 /startUp.sh \
    \
    # remove invalid zcfgs \
    && rm /usr/lib/cgi-bin/SAGA/db_pgsql/6.zcfg /usr/lib/cgi-bin/SAGA/imagery_tools/8.zcfg /usr/lib/cgi-bin/SAGA/grid_calculus_bsl/0.zcfg /usr/lib/cgi-bin/SAGA/grids_tools/1.zcfg /usr/lib/cgi-bin/SAGA/grid_visualisation/1.zcfg /usr/lib/cgi-bin/SAGA/ta_lighting/2.zcfg /usr/lib/cgi-bin/OTB/TestApplication.zcfg /usr/lib/cgi-bin/OTB/StereoFramework.zcfg \
    # Update SAGA zcfg
    && sed "s:AllowedValues =    <Default>:AllowedValues =\n    <Default>:g" -i /usr/lib/cgi-bin/SAGA/*/*zcfg \
    && sed "s:Title = $:Title = No title found:g" -i /usr/lib/cgi-bin/SAGA/*/*.zcfg \
    # Enable apache modules
    \
    && a2enmod cgi rewrite \
    \
    # Cleanup \
    && apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS \
    && rm -rf /var/lib/apt/lists/*


# service namespaces parent folder
RUN mkdir -p /opt/zooservices_namespaces && chmod -R 700 /opt/zooservices_namespaces && chown -R www-data /opt/zooservices_namespaces

# For using another port than 80, change the value below.
# remember to also change the ports in docker-compose.yml
EXPOSE 80
CMD /usr/sbin/apache2ctl -D FOREGROUND
