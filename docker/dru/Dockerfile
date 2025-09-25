FROM ubuntu:22.04

# to build:
# docker build --rm -t ades:latest .
# to run:
# docker run --rm -ti -p 80:80  ades:latest

# procadesdev:latest
ENV DEBIAN_FRONTEND noninteractive

########################################
### DEV TOOLS
RUN apt-get update -qqy --no-install-recommends \
    # Required for security fixes
    && apt-get upgrade -y \
    # Various cli tools
    && apt-get install -qqy --no-install-recommends wget mlocate tree \
    # C++ and CMAKE
    gcc cmake build-essential \
    #Install Docker CE CLI
    curl apt-transport-https ca-certificates gnupg2 lsb-release \
    && curl -fsSL https://download.docker.com/linux/$(lsb_release -is | tr '[:upper:]' '[:lower:]')/gpg | apt-key add - 2>/dev/null \
    && echo "deb [arch=amd64] https://download.docker.com/linux/$(lsb_release -is | tr '[:upper:]' '[:lower:]') $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list \
    && apt-get update -qqy --no-install-recommends \
    && apt-get install -qqy --no-install-recommends docker-ce-cli && \
    apt-get clean -qqy

ARG PY_VER=3.10
RUN curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
    install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl


########################################
# ZOO_Prerequisites

ARG RUN_DEPS="\
    git\
    wget\
    vim\
    flex\
    bison\
    libxml2\
    curl\
    autoconf\
    apache2\
    subversion\
    s3cmd \
    python3-setuptools\
    pip \
    #valgrind \
    libapache2-mod-fcgid\
    libapache2-mod-auth-openidc \
    wget \
    pkg-config\
    gettext \
    locales \
"

ARG BUILD_DEPS="libfcgi-dev\
    libxml2-dev\
    libssl-dev\
    libmapserver-dev\
    libxslt1-dev\
    uuid-dev\
    libjson-c-dev\
    libproj-dev\
    libgdal-dev\
    libaprutil1-dev \
    python3-dev\
    build-essential\
    librabbitmq-dev\
    libcairo2-dev"

RUN apt-get update -qqy  --no-install-recommends  && \
    apt-get install -qqy --no-install-recommends  software-properties-common && \
    add-apt-repository ppa:ubuntugis/ubuntugis-unstable && \
    add-apt-repository ppa:ubuntugis/ppa && \
    apt-get update -qqy  --no-install-recommends && \
    apt-get install -qqy  --no-install-recommends software-properties-common\
        ${RUN_DEPS}\
        ${BUILD_DEPS}\
    && a2enmod cgi rewrite headers auth_openidc proxy proxy_http actions fcgid alias proxy_fcgi \
    && /etc/init.d/apache2 restart \
    && rm -rf /var/lib/apt/lists/*

########################################
# ZOO_KERNEL
#ARG ZOO_PRJ_GIT_BRANCH='feature/deploy-undeploy-ogcapi-route'
#RUN cd /opt && git clone --depth 1 https://github.com/terradue/ZOO-Project.git -b $ZOO_PRJ_GIT_BRANCH
COPY ./docker/.htaccess /opt/ZOO-Project/docker/.htaccess
COPY ./thirds /opt/ZOO-Project/thirds
COPY ./zoo-project /opt/ZOO-Project/zoo-project
WORKDIR /opt/ZOO-Project
RUN make -C ./thirds/cgic206 libcgic.a
RUN cd ./zoo-project/zoo-kernel \
     && autoreconf -i \
     && autoreconf --install \
     && ./configure --with-dru=yes \
       --with-python=/usr/ \
	--with-pyvers=$PY_VER \
	--with-mapserver=/usr \
	--with-ms-version=7 \
	--with-json=/usr \
	--prefix=/usr \
	--with-metadb=yes \
	--with-db-backend \
	--with-rabbitmq=yes \
     && sed -i "s/-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H/-DPROJ_VERSION_MAJOR=8/g" ./ZOOMakefile.opts \
     && make -j4\
     && make install \
     && cp main.cfg /usr/lib/cgi-bin \
     && cp zoo_loader.cgi /usr/lib/cgi-bin \
     && cp zoo_loader_fpm /usr/lib/cgi-bin \
     && cp oas.cfg /usr/lib/cgi-bin \
    \
    # Install Basic Authentication sample
    # TODO: is this still required?
    && cd ../zoo-services/utils/security/basicAuth \
    && make \
    && cp cgi-env/* /usr/lib/cgi-bin \
    \
     && sed -i "s%http://www.zoo-project.org/zoo/%http://127.0.0.1%g" /usr/lib/cgi-bin/main.cfg \
     && sed -i "s%../tmpPathRelativeToServerAdress/%http://localhost/temp/%g" /usr/lib/cgi-bin/main.cfg \
     && sed "s:AllowOverride None:AllowOverride All:g" -i /etc/apache2/apache2.conf \
     && cd /opt/ZOO-Project \
     && cp ./docker/.htaccess /var/www/html/.htaccess \
     && cp -r zoo-project/zoo-services/utils/open-api/templates/index.html /var/www/index.html \
     && cp -r zoo-project/zoo-services/utils/open-api/static /var/www/html/ \
     && cp zoo-project/zoo-services/utils/open-api/cgi-env/* /usr/lib/cgi-bin/ \
     && cp zoo-project/zoo-services/utils/security/dru/* /usr/lib/cgi-bin/ \
     && rm /usr/lib/cgi-bin/securityInFailed.zcfg \
     && cp zoo-project/zoo-services/echo-py/cgi-env/echo.zcfg /usr/lib/cgi-bin/ \
     && cp zoo-project/zoo-services/echo-py/cgi-env/echo_service.py /usr/lib/cgi-bin/ \
     && mkdir /usr/lib/cgi-bin/jwts \
     && cp zoo-project/zoo-services/utils/security/jwt/cgi-env/security_service.py /usr/lib/cgi-bin/jwts/ \
     && ln -s /tmp/zTmp /var/www/html/temp \
     && mkdir /var/www/html/examples/ \
     # update the securityIn.zcfg
     && sed "s:serviceType = C:serviceType = Python:g;s:serviceProvider = security_service.zo:serviceProvider = service:g" -i /usr/lib/cgi-bin/securityIn.zcfg \
     && curl -o /var/www/html/examples/deployment-job.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-deploy-body.json \
     && curl -o /var/www/html/examples/deployment-job1.json https://raw.githubusercontent.com/EOEPCA/proc-ades/1b55873dad2684f3333842aea77efb6fb33aa210/test/sample_apps/dNBR/app-deploy-body1.json \
     && curl -o /var/www/html/examples/deployment-job2.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/dNBR/app-deploy-body.json \
     && curl -o /var/www/html/examples//app-package.cwl https://raw.githubusercontent.com/EOEPCA/app-snuggs/main/app-package.cwl \
     # Add snuggs examples in the correct location
     && mkdir /var/www/html/examples/snuggs \
     && curl -o /var/www/html/examples/snuggs/job_order1.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-execute-body.json \
     && curl -o /var/www/html/examples/snuggs/job_order2.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-execute-body2.json \
     && curl -o /var/www/html/examples/snuggs/job_order3.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-execute-body3.json \
     && cd .. && rm -rf ZOO-Project

#
# Install Swagger-ui
#
RUN git clone --depth 1 https://github.com/swagger-api/swagger-ui.git \
    && mv swagger-ui /var/www/html/swagger-ui \
    && sed "s=https://petstore.swagger.io/v2/swagger.json=http://localhost:8080/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/* \
    && mv /var/www/html/swagger-ui/dist /var/www/html/swagger-ui/oapip

COPY docker/dru/default.conf /etc/apache2/sites-available/000-default.conf
COPY zoo-project/zoo-services/utils/open-api/dru/DetectEntrypoint.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/DeployProcess.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/DeployProcess.zcfg /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/UndeployProcess.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/UndeployProcess.zcfg /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/deploy_util.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/ZooStub.py /usr/lib/cgi-bin

RUN chmod -R 777 /usr/lib/cgi-bin


ARG DEPS_TO_REMOVE="\
    libc6-dev\
    libcrypt-dev\
    icu-devtools\
    libc-dev-bin\
    libgcc-11-dev\
    libglib2.0-dev-bin\
    libnsl-dev\
    libtirpc-dev\
    linux-libc-dev\
    python3-zipp\
    python3-pip\
    dpkg-dev\
    unixodbc-dev\
    python3.10-dev\
    libzstd-dev\
    libxerces-c-dev\
    liburiparser-dev\
    libwebp-dev\
    libsuperlu-dev\
    libsctp-dev\
    librttopo-dev\
    libpython3-dev\
    libpython3.10-dev\
    libqhull-dev\
    libpoppler-dev\
    libpoppler-private-dev\
    libpq-dev\
    libpng-dev\
    libpcre2-dev\
    libnetcdf-dev\
    libogdi-dev\
    libopenjp2-7-dev\
    libminizip-dev\
    liblapack-dev\
    libldap2-dev\
    libltdl-dev\
    liblz4-dev\
    libkml-dev\
    libhdf5-dev\
    libheif-dev\
    libicu-dev\
    libhdf4-alt-dev\
    libgeos-dev\
    libgeotiff-dev\
    libgif-dev\
    libfyba-dev\
    libfreexl-dev\
    libexpat1-dev\
    libdeflate-dev\
    libcfitsio-dev\
    libarmadillo-dev\
    libarpack2-dev\
    libblas-dev\
    libblosc-dev\
    libboost-dev\
    libapr1-dev\
    libaec-dev\
    g++\
    g++-11\
    zlib1g-dev\
    libjpeg-turbo8-dev \
    libjpeg8-dev\
    libxcb-shm0-dev \
    libxext-dev \
    libxrender-dev \
    xtrans-dev \
    x11proto-dev \
    libxdmcp-dev \
    libxcb1-dev \
    libxcb-render0-dev \
    libxau-dev \
    libx265-dev \
    libx11-dev \
    libstdc++-11-dev \
    libsqlite3-dev \
    libsm-dev \
    libsepol-dev \
    libpthread-stubs0-dev \
    libpixman-1-dev \
    libpcre3-dev \
    liblzma-dev \
    libldap-dev \
    libjbig-dev \
    libice-dev \
    libffi-dev \
    libde265-dev \
    libdav1d-dev \
    libcurl4-openssl-dev \
    libbrotli-dev \
    libboost1.74-dev \
    libaom-dev \
    autotools-dev \
    autoconf \
    automake \
"

COPY docker/startUp.sh /startUp.sh
COPY docker/dru/requirements.txt /tmp/
COPY zoo-project/zoo-kernel/locale /tmp/locale

RUN date && mkdir /tmp/cookiecutter-templates && \
    pip install --upgrade pip && \
    pip install hatch && \
    # Using the latest version of pycalrissian with the fix for the RBAC
    cd /tmp && \
    git clone -b develop https://github.com/Terradue/pycalrissian.git && \
    cd pycalrissian && \
    #sed "s:cwltool==:cwltool>=:g;s:setuptools==:setuptools>=:g;s:" -i pyproject.toml && \
    sed "s:cwltool==:cwltool>=:g;s:kubernetes==:kubernetes>=:g;s:typing-extensions==:typing-extensions>=:g;s:setuptools==:setuptools>=:g" -i pyproject.toml && \
    hatch build && \
    pip install ./dist/pycalrissian-0.8.0-py3-none-any.whl && \
    # cd .. && \
    pip install -r /tmp/requirements.txt && \
    cd /tmp && \
    git clone https://github.com/EOEPCA/cwl-wrapper.git && \
    cd cwl-wrapper/ && \
    python3 setup.py install && \
    pip install --upgrade setuptools cryptography PyJWT pip zipp urllib3 && \
    cd .. && \
    rm -r pycalrissian cwl-wrapper /tmp/requirements.txt && \
    chmod -R 777 /tmp/cookiecutter-templates && \
    rm /var/log/apache2/error.log && \
    ln -s /dev/stdout /var/log/apache2/error.log && \
    # Add minimal translation support for the ZOO-Kernel internal messages
    cd /tmp && for i in  $(ls ./locale/po/*po | grep -v utf8 | grep -v message); do \
        j=$(echo $i | sed "s:./locale/po/::g;s:.po::g"); \
        case "$j" in \
            cs) j_full="cs_CZ" ;; \
            de) j_full="de_DE" ;; \
            es) j_full="es_ES" ;; \
            fi) j_full="fi_FI" ;; \
            ja) j_full="ja_JP" ;; \
            messages) \
            echo "Skipping $j (not a real locale)" \
            continue \
            ;; \
            *) j_full="$j" ;; \
        esac ; \
        echo "🔹 Processing $file -> $j_full" ; \
        mkdir -p /usr/share/locale/$j_full/LC_MESSAGES; \
        msgfmt  $i -o /usr/share/locale/$j_full/LC_MESSAGES/zoo-kernel.mo ; \
        mkdir -p /usr/local/share/locale/$j_full/LC_MESSAGES; \
        msgfmt  $i -o /usr/local/share/locale/$j_full/LC_MESSAGES/zoo-kernel.mo ; \
        locale-gen $j_full ; \
        localedef -i $j_full -c -f UTF-8 -A /usr/share/locale/locale.alias ${j_full}.UTF-8; \
    done  && \
    \
    chown www-data:www-data -R /var/www && \
    apt-get purge -y $DEPS_TO_REMOVE $BUILD_DEPS && \
    rm -rf /var/lib/apt/lists/* /tmp/*

COPY docker/dru/index.html /var/www/html/index.html

EXPOSE 80
CMD ["apachectl", "-D", "FOREGROUND"]
