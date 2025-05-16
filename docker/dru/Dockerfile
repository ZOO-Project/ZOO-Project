FROM ubuntu:20.04

# to build:
# docker build --rm -t ades:latest .
# to run:
# docker run --rm -ti -p 80:80  ades:latest

# procadesdev:latest
ENV DEBIAN_FRONTEND noninteractive
ARG CONDA_ENV_FILE=""
ARG CONDA_ENV_NAME=ades-dev

########################################
### DEV TOOLS
RUN apt-get update -qqy --no-install-recommends \
# Various cli tools
 && apt-get install -qqy --no-install-recommends wget mlocate tree \
# C++ and CMAKE
 gcc mono-mcs cmake \
 build-essential libcgicc-dev gdb \
#Install Docker CE CLI
 curl apt-transport-https ca-certificates gnupg2 lsb-release \
 && curl -fsSL https://download.docker.com/linux/$(lsb_release -is | tr '[:upper:]' '[:lower:]')/gpg | apt-key add - 2>/dev/null \
 && echo "deb [arch=amd64] https://download.docker.com/linux/$(lsb_release -is | tr '[:upper:]' '[:lower:]') $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list \
 && apt-get update -qqy --no-install-recommends \
 && apt-get install -qqy --no-install-recommends docker-ce-cli && \
 apt-get clean -qqy

ARG PY_VER=3.8
ARG MINICONDA=Miniconda3-latest-Linux-x86_64.sh
RUN curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
    install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl


########################################
# ZOO_Prerequisites

ARG BUILD_DEPS="libfcgi-dev\
    libxml2-dev\
    libssl-dev\
    libmapserver-dev\
    libmozjs185-dev\
    libxslt1-dev\
    uuid-dev\
    libjson-c-dev\
    libproj-dev\
    libgdal-dev\
    libaprutil1-dev \
    python3-dev\
    build-essential\
    librabbitmq-dev"

ARG DEPS_TO_REMOVE="\
    dpkg-dev\
    unixodbc-dev\
    python3.8-dev\
    libzstd-dev\
    libxerces-c-dev\
    liburiparser-dev\
    libwebp-dev\
    libsuperlu-dev\
    libstdc++-9-dev\
    libsctp-dev\
    librttopo-dev\
    libpython3-dev\
    libpython3.8-dev\
    libqhull-dev\
    libpoppler-dev\
    libpoppler-private-dev\
    libpq-dev\
    libpng-dev\
    libpcre2-dev\
    libnetcdf-dev\
    libnspr4-dev\
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
    libffi-dev\
    libfreexl-dev\
    libexpat1-dev\
    libdeflate-dev\
    libcfitsio-dev\
    libcharls-dev\
    libarmadillo-dev\
    libarpack2-dev\
    libblas-dev\
    libblosc-dev\
    libboost-dev\
    libapr1-dev\
    libaec-dev\
    g++\
    g++-9\
    zlib1g-dev\
    libjpeg-turbo8-dev \
    libjpeg8-dev\
    libcurl4-gnutls-dev\
"

RUN apt-get update -qqy  --no-install-recommends  && \
apt-get install -qqy --no-install-recommends  software-properties-common && \
add-apt-repository ppa:ubuntugis/ubuntugis-unstable && \
add-apt-repository ppa:ubuntugis/ppa && \
apt-get update -qqy  --no-install-recommends && \
apt-get install -qqy  --no-install-recommends software-properties-common\
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
	wget \
	pkg-config\
    ${BUILD_DEPS}\
# if you remove --with-db-backend from the configure command, uncomment the following line
#RUN ln -s /usr/lib/x86_64-linux-gnu/libfcgi.a /usr/lib/
&& a2enmod actions fcgid alias proxy_fcgi \
&& /etc/init.d/apache2 restart \
&& rm -rf /var/lib/apt/lists/*

########################################
# ZOO_KERNEL
#ARG ZOO_PRJ_GIT_BRANCH='feature/deploy-undeploy-ogcapi-route'
#RUN cd /opt && git clone --depth 1 https://github.com/terradue/ZOO-Project.git -b $ZOO_PRJ_GIT_BRANCH
COPY . /opt/ZOO-Project
WORKDIR /opt/ZOO-Project
RUN make -C ./thirds/cgic206 libcgic.a
RUN cd ./zoo-project/zoo-kernel \
     && autoconf \
     && ./configure --with-dru=yes \
       --with-python=/usr/ \
	--with-pyvers=$PY_VER \
	--with-js=/usr \
	--with-mapserver=/usr \
	--with-ms-version=7 \
	--with-json=/usr \
	--prefix=/usr \
	--with-metadb=yes \
	--with-db-backend \
	--with-rabbitmq=yes \
     && sed -i "s/-DACCEPT_USE_OF_DEPRECATED_PROJ_API_H/-DPROJ_VERSION_MAJOR=8/g" ./ZOOMakefile.opts \
     #&& sed -i "s:LDFLAGS=:LDFLAGS=-Wl,-rpath,/usr/miniconda3/lib/ :g" ./ZOOMakefile.opts \
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
     && echo "\n[env]\nPYTHONPATH=/usr/miniconda3/envs/${CONDA_ENV_NAME}/lib/python${PY_VER}/site-packages"\
          >> /usr/lib/cgi-bin/main.cfg \
     && a2enmod cgi rewrite \
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

COPY docker/default.conf /etc/apache2/sites-available/000-default.conf
COPY zoo-project/zoo-services/utils/open-api/dru/DeployProcess.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/DeployProcess.zcfg /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/UndeployProcess.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/UndeployProcess.zcfg /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/deploy_util.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/ZooStub.py /usr/lib/cgi-bin

RUN chmod -R 777 /usr/lib/cgi-bin

COPY docker/dru/requirements.txt /tmp/

# Force running the following run in case anything changed in
# https://github.com/EOEPCA/zoo-argowf-runner.git (branch: develop) 
# and / or 
# https://github.com/EOEPCA/zoo-calrissian-runner.git 
# (branch: feature/access-log-status-failed)
RUN date && mkdir /tmp/cookiecutter-templates && \
    pip install --upgrade pip && \
    pip install hatch && \
    # Using the latest version of pycalrissian with the fix for the RBAC
    cd /tmp && \
    git clone -b develop https://github.com/Terradue/pycalrissian.git && \
    cd pycalrissian && \
    hatch build && \
    pip install ./dist/pycalrissian-0.8.0-py3-none-any.whl && \
    cd .. && \
    pip install -r /tmp/requirements.txt && \
    cd /tmp && \
    git clone https://github.com/ZOO-Project/zoo-wes-runner.git && \
    cd zoo-wes-runner && \
    poetry build && \
    ls dist && \
    pip install dist/zoo_wes_runner-0.1.0-py3-none-any.whl && \
    cd .. && \
    git clone https://github.com/EOEPCA/cwl-wrapper.git && \
    cd cwl-wrapper/ && \
    python3 setup.py install && \
    cd .. && \
    apt-get -qqy remove python3-setuptools python3-wheel python3-urllib3 python3-urllib3 python3-idna python3-certifi && \
    rm -r pycalrissian cwl-wrapper /tmp/requirements.txt && \
    echo /usr/miniconda3/envs/${CONDA_ENV_NAME}/lib/ > /etc/ld.so.conf.d/zoo-project.conf && \
    ldconfig && \
    if [ -z "$CONDA_ENV_FILE" ] ; then \
        echo "No modification required" ; \
    else \
        echo "Modify charset_normalizer" ; \
        sed "s:from .md:#from .md:g" -i /usr/miniconda3/envs/env_zoo_calrissian/lib/python3.10/site-packages/charset_normalizer/api.py ; \
        sed "s:from .md:#from .md:g" -i /usr/miniconda3/envs/env_zoo_calrissian/lib/python3.10/site-packages/charset_normalizer/cd.py ; \
    fi && \
    chmod -R 777 /tmp/cookiecutter-templates && \
    rm /var/log/apache2/error.log && \
    ln -s /dev/stdout /var/log/apache2/error.log && \
    #apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS && \
    apt-get purge -y $DEPS_TO_REMOVE $BUILD_DEPS && \
    rm -rf /var/lib/apt/lists/*



COPY docker/dru/index.html /var/www/html/index.html

EXPOSE 80
CMD ["apachectl", "-D", "FOREGROUND"]
