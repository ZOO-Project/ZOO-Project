# Build using the command bellow:
# docker build . -t zooproject/zoo-project:ciab-0.0.1
#FROM osgeo/gdal:ubuntu-small-3.6.2 as base
FROM osgeo/gdal:ubuntu-full-3.6.2 as base

ENV DEBIAN_FRONTEND=noninteractive \
    LC_ALL=C.UTF-8 \
    LANG=C.UTF-8 \
    TINI_VERSION=v0.19.0

#ADD https://github.com/krallin/tini/releases/download/${TINI_VERSION}/tini /tini
#RUN chmod +x /tini

RUN find /etc/ -name "source*" && \
    apt-get update -o Acquire::AllowInsecureRepositories=true \
-o Acquire::AllowDowngradeToInsecureRepositories=true  && \
    apt-get install -y \
      build-essential \
      git \
      # For Psycopg2
      libpq-dev python3-dev \
      python3-pip \
      wget \
    && apt-get autoclean \
    && apt-get autoremove \
    && rm -rf /var/lib/{apt,dpkg,cache,log}

COPY docker/ciab/requirements.txt /conf/
COPY docker/ciab/products.csv /conf/
RUN pip3 install --no-cache-dir --requirement /conf/requirements.txt \
    # TODO: review why this should be done \
    && sed "s:from odc.aws.queue import publish_to_topic:#from odc.aws.queue import publish_to_topic:g" -i /usr/local/lib/python3.10/dist-packages/odc/apps/dc_tools/utils.py 


#RUN git clone -b feature/deploy-undeploy-ogcapi-route https://github.com/Terradue/ZOO-Project.git /opt/ZOO-Project
COPY . /opt/ZOO-Project
WORKDIR /opt/ZOO-Project

FROM base AS zookernel
RUN apt-get install add-apt-key \
    #&&  add-apt-key 9CBA4EF977CA20B8 \
    && apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 9CBA4EF977CA20B8 \
    &&  apt-get update --allow-unauthenticated --allow-insecure-repositories \
    && apt-get install -qqy  --no-install-recommends software-properties-common\
	git\
	wget\
    	gettext \
	vim\
	flex\
	bison\
	libfcgi-dev\
	libproj-dev\
	libxml2\
	libxml2-dev\
	curl\
	libssl-dev\
	autoconf\
	automake\
	apache2\
	subversion\
	python3-dev\
	python3-setuptools\
	build-essential\
	libxslt1-dev\
	uuid-dev\
	libjson-c-dev\
	libmapserver-dev\
	#libgdal-dev\
	libaprutil1-dev \
	librabbitmq-dev\
	libapache2-mod-fcgid\
	wget \
	pkg-config \
     && make -C ./thirds/cgic206 libcgic.a \
     && cd ./zoo-project/zoo-kernel \
     #&& dpkg -L python3-dev \
     #&& find / -name python-config \
     #&& man python3-config \
     #&& autoconf \
     #&& grep MS_VERSION_ -rni /usr/ \
     && autoreconf --install && autoconf && autoreconf --install \
     && grep json_bool -rni /usr/include \
     && grep "FALSE" -rni /usr/include/json-c \
     && ./configure --with-dru=yes \
     		    --with-python=yes \
		    --with-pyvers=3.10 \
		    --with-mapserver=/usr \
		    --with-ms-version=7 \
		    --with-json=/usr \
		    --prefix=/usr \
		    # Issue with the GDAL version included in the docker image
		    # 
		    #--with-metadb=yes \
		    #--with-db-backend \
		    #--with-rabbitmq=yes \
     && sed -i "s/-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H/-DPROJ_VERSION_MAJOR=8/g" ./ZOOMakefile.opts \
     && make -j4 \
     && make install \
     && cp ../zoo-services/utils/open-api/cgi-env/* /usr/lib/cgi-bin/ \
     && cp ../zoo-services/utils/security/dru/* /usr/lib/cgi-bin/ \
     && cp ../zoo-services/utils/security/openeo-udp/* /usr/lib/cgi-bin/ \
     && mkdir /usr/lib/cgi-bin/jwts/ \
     && cp ../zoo-services/utils/security/jwt/cgi-env/* /usr/lib/cgi-bin/jwts/ \
     && touch /usr/lib/cgi-bin/jwts/__init__.py \
     && cp ../zoo-services/utils/openeo/*.* /usr/lib/cgi-bin/ \
     && cp ../zoo-services/utils/open-api/dru/* /usr/lib/cgi-bin/ \
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
    && cp main.cfg /usr/lib/cgi-bin/ \
    && cp oas.cfg /usr/lib/cgi-bin/

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
    && apt-get update -o Acquire::AllowInsecureRepositories=true \
	-o Acquire::AllowDowngradeToInsecureRepositories=true && \
    apt-get install -y --no-install-recommends $BUILD_DEPS \
    \
    && git clone --depth=1 https://github.com/ZOO-Project/examples.git \
    && git clone --depth=1 https://github.com/swagger-api/swagger-ui.git \
    && git clone --depth=1 https://github.com/WPS-Benchmarking/cptesting.git /testing \
    #&& git clone --depth=1 https://www.github.com/singularityhub/singularity-cli.git /singularity-cli \
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
    librabbitmq4 \
    #python3-setuptools \
    #libotbapplicationengine-8.1 \
    #Uncomment the line below to add vi editor \
    #vim \
    #Uncomment the lines below to add debuging \
    #valgrind \
    #gdb \
    #strace \
"
ARG BUILD_DEPS=" \
    make \
    g++ \
    gcc \
    #libgdal-dev \
    python3-dev \
"
# For Azure use, uncomment bellow
#ARG SERVER_URL="http://zooprojectdemo.azurewebsites.net/"
#ARG WS_SERVER_URL="ws://zooprojectdemo.azurewebsites.net"
# For basic usage
ARG WS_SERVER_URL="ws://localhost"
ARG SERVER_URL="http://localhost/"

# For using another port than 80, uncomment below.
# remember to also change the ports in docker-compose.yml
#ARG SERVER_URL="https://testbed19.geolabs.fr:8705/"
#ARG PORT=8704

WORKDIR /opt/ZOO-Project
COPY ./docker/startUp.sh /

# From zoo-kernel
COPY --from=zookernel /usr/lib/cgi-bin/ /usr/lib/cgi-bin/
COPY --from=zookernel /usr/lib/libzoo_service.so* /usr/lib/
#COPY --from=zookernel /usr/lib/libzoo_service.so /usr/lib/libzoo_service.so
COPY --from=zookernel /usr/com/zoo-project/ /usr/com/zoo-project/
COPY --from=zookernel /usr/include/zoo/ /usr/include/zoo/
COPY --from=zookernel /usr/share/locale/ /usr/share/locale/
#COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/SAGA/examples/ /var/www/html/examples/
#COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/OTB/examples/ /var/www/html/examples/
#COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/cgal/examples/ /var/www/html/examples/
COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/utils/open-api/templates/index.html /var/www/index.html
COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/utils/open-api/static /var/www/html/static
COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/echo-py/cgi-env/ /usr/lib/cgi-bin/
COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/utils/openeo/examples/deployment-openeo-udp.json /var/www/html/examples/
COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/utils/openeo/examples/run-process.json /var/www/html/examples/fahrenheit_to_celsius/
COPY --from=zookernel /opt/ZOO-Project/docker/.htaccess /var/www/html/.htaccess
COPY --from=zookernel /opt/ZOO-Project/docker/default.conf /000-default.conf
COPY --from=zookernel /opt/ZOO-Project/zoo-project/zoo-services/utils/open-api/server/publish.py /usr/lib/cgi-bin/publish.py

# From optional zoo demos
#COPY --from=demos /singularity-cli/ /singularity-cli/
COPY --from=demos /testing/ /testing/
COPY --from=demos /zoo-project/examples/data/ /usr/com/zoo-project/
COPY --from=demos /zoo-project/examples/ /var/www/html/
COPY --from=demos /zoo-project/swagger-ui /var/www/html/swagger-ui


RUN set -ex \
    && apt-get update -o Acquire::AllowInsecureRepositories=true \
	-o Acquire::AllowDowngradeToInsecureRepositories=true && \
    apt-get install -y --no-install-recommends $RUN_DEPS $BUILD_DEPS \
    \
    && sed "s=https://petstore.swagger.io/v2/swagger.json=${SERVER_URL}/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/* \
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
    #&& pip3 install GDAL==2.4.2 \
    && pip3 install Cheetah3 redis spython \
    && pip uninstall jwt \
    && pip install PyJWT \
    && sed "s:AllowOverride None:AllowOverride All:g" -i /etc/apache2/apache2.conf \
    \
    # For using another port than 80, uncomment below. \
    # remember to also change the ports in docker-compose.yml \
    && a2enmod ssl \
    #&& sed "s:Listen 80:Listen $PORT:g;s:443:$(expr $PORT + 1):g" -i /etc/apache2/ports.conf \
    \
    && chown www-data:www-data -R /tmp/zTmp /usr/com/zoo-project \
    && echo "Mutex posixsem" >> /etc/apache2/apache2.conf \
    \
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
#EXPOSE 8701
CMD /usr/sbin/apache2ctl -D FOREGROUND


#WORKDIR /notebooks

#ENTRYPOINT ["/tini", "--"]

#CMD ["jupyter", "notebook", "--allow-root", "--ip='0.0.0.0'", "--NotebookApp.token='secretpassword'"]
