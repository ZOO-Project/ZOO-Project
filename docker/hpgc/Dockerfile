FROM ubuntu:20.04

# to build:
# docker build --rm -t zooproject/zoo-project:hpgc-latest . -f docker/hpgc/Dockerfile

# procadesdev:latest
ENV DEBIAN_FRONTEND noninteractive

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
 && install -m 0755 -d /etc/apt/keyrings \
 && curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /etc/apt/keyrings/docker.gpg \
 && chmod a+r /etc/apt/keyrings/docker.gpg \
 && echo \
  "deb [arch="$(dpkg --print-architecture)" signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu \
  "$(. /etc/os-release && echo "$VERSION_CODENAME")" stable" | \
   tee /etc/apt/sources.list.d/docker.list > /dev/null \
 && apt-get update -qqy --no-install-recommends \
 && apt-cache search docker \
 && for pkg in docker.io docker-doc docker-compose podman-docker containerd runc; do apt-get remove $pkg; done\
 && apt-get install -qqy --no-install-recommends docker-ce-cli  \
 && apt-get clean -qqy

ARG MINICONDA=Miniconda3-latest-Linux-x86_64.sh
ARG PY_VER=3.10
# Miniconda
RUN wget -nv \
    https://repo.anaconda.com/miniconda/$MINICONDA --no-check-certificate \
    && bash $MINICONDA -b -p /usr/miniconda3 \
    && rm -f $MINICONDA
ENV PATH="/usr/miniconda3/envs/ades-dev/bin:/usr/miniconda3/bin:${PATH}"
ENV CONDA_PLUGINS_AUTO_ACCEPT_TOS=true
COPY docker/hpgc/ades-dev_env.yaml /tmp/ades-dev_env.yaml
RUN conda install mamba -n base -c conda-forge && \
    mamba env create --file /tmp/ades-dev_env.yaml &&\
    rm /tmp/ades-dev_env.yaml
RUN ln -s /usr/miniconda3/envs/ades-dev/bin/kubectl /usr/bin/kubectl

########################################
# ZOO_Prerequisites

RUN apt-get install -qqy --no-install-recommends  software-properties-common && \
add-apt-repository ppa:ubuntugis/ubuntugis-unstable && \
add-apt-repository ppa:ubuntugis/ppa && \
apt-get update -qqy  --no-install-recommends && \
apt-get install -qqy  --no-install-recommends software-properties-common\
	git\
	wget\
	vim\
	flex\
	bison\
	libfcgi-dev\
	libxml2\
	libxml2-dev\
	curl\
	libssl-dev\
	autoconf\
	apache2\
	subversion\
	libmozjs185-dev\
	python3-dev\
	#libssh2-1-dev \
	python3-setuptools\
	build-essential\
	libxslt1-dev\
	uuid-dev\
	libjson-c-dev\
	mapserver-bin \
	libmapserver-dev\
	libgdal-dev\
	libaprutil1-dev \
	librabbitmq-dev\
	libapache2-mod-fcgid\
	wget \
	pkg-config\
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
RUN curl -o libssh2-1.11.0.tar.bz2 https://github.com/libssh2/libssh2/releases/download/libssh2-1.11.0/libssh2-1.11.0.tar.bz2 -L \
    && tar -xf libssh2-1.11.0.tar.bz2 \
    && cd libssh2-1.11.0 \
    && autoreconf \
    && ./configure --with-sysroot=/usr/miniconda3/envs/ades-dev \
       --with-libssl-prefix=/usr/miniconda3/envs/ades-dev \
       --prefix=/usr \
    && make \
    && make install \
    && cd ../ \
    && cd ./zoo-project/zoo-kernel \
     && autoconf \
     #&& grep MS_VERSION_ -rni /usr/ \
     && ./configure --with-dru=yes \
     	--with-python=/usr/miniconda3/envs/ades-dev \
	--with-pyvers=$PY_VER \
	--with-hpc=yes \
	--with-ssh2=/usr \
	--with-callback=yes \
	--with-js=/usr \
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
     && echo "/usr/miniconda3/envs/ades-dev/lib" > /etc/ld.so.conf.d/zoo-project.conf \
     && ldconfig \
     && cp main.cfg /usr/lib/cgi-bin \
     && cp zoo_loader.cgi /usr/lib/cgi-bin \
     && cp zoo_loader_fpm /usr/lib/cgi-bin \
     && cp oas.cfg /usr/lib/cgi-bin \
    \
    # Install Basic Authentication sample
    # TODO: is this still required?
    && cp ../zoo-services/utils/open-api/server/publish.py /usr/lib/cgi-bin/ \
    && cp ../zoo-services/utils/open-api/server/subscriber.py /usr/lib/cgi-bin/ \
    && cp ../zoo-services/utils/hpc/examples/callback.py /usr/lib/cgi-bin/ \
    && chmod 755 /usr/lib/cgi-bin/callback.py \
    && cd ../zoo-services/utils/security/basicAuth \
    && make \
    && cp cgi-env/* /usr/lib/cgi-bin \
    && cd ../../hpc \
    && make \
    && cp cgi-env/* /usr/lib/cgi-bin \
    \
     && sed -i "s%http://www.zoo-project.org/zoo/%http://127.0.0.1%g" /usr/lib/cgi-bin/main.cfg \
     && sed -i "s%../tmpPathRelativeToServerAdress/%http://localhost/temp/%g" /usr/lib/cgi-bin/main.cfg \
     && echo "\n[env]\nPYTHONPATH=/usr/miniconda3/envs/ades-dev/lib/python${PY_VER}/site-packages"\
          >> /usr/lib/cgi-bin/main.cfg \
     && a2enmod cgi rewrite \
     && sed "s:AllowOverride None:AllowOverride All:g" -i /etc/apache2/apache2.conf \
     && cd /opt/ZOO-Project \
     && cp ./docker/.htaccess /var/www/html/.htaccess \
     && cp -r zoo-project/zoo-services/utils/open-api/templates/index.html /var/www/index.html \
     && cp -r zoo-project/zoo-services/utils/open-api/static /var/www/html/ \
     && cp zoo-project/zoo-services/utils/open-api/cgi-env/* /usr/lib/cgi-bin/ \
     && cp zoo-project/zoo-services/utils/security/dru/* /usr/lib/cgi-bin/ \
     && cp -r zoo-project/zoo-services/utils/security/jwt /usr/lib/cgi-bin/jwts \
     && ln -s /tmp/zTmp /var/www/html/temp \
     && mkdir /var/www/html/examples/ \
     # update the securityIn.zcfg
     && sed "s:serviceType = C:serviceType = Python:g;s:serviceProvider = security_service.zo:serviceProvider = service:g" -i /usr/lib/cgi-bin/securityIn.zcfg \
     && curl -o /var/www/html/examples/deployment-job.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-deploy-body.json \
     && curl -o /var/www/html/examples/deployment-job1.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/dNBR/app-deploy-body.json \
     && cp zoo-project/zoo-services/utils/hpc/examples/deployment-job.json /var/www/html/examples/deployment-job2.json\
     && mkdir -p /var/www/html/examples/BandMath \
     && cp zoo-project/zoo-services/OTB/examples/OTB.BandMath/sample1.json /var/www/html/examples/BandMath/job_order.json \
     && curl -o /var/www/html/examples/job_order1.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-execute-body.json \
     && curl -o /var/www/html/examples/job_order2.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-execute-body2.json \
     && curl -o /var/www/html/examples/job_order3.json https://raw.githubusercontent.com/EOEPCA/proc-ades/master/test/sample_apps/v2/snuggs/app-execute-body3.json \
     && curl -o /var/www/html/examples//app-package.cwl https://raw.githubusercontent.com/EOEPCA/app-snuggs/main/app-package.cwl \
     && mkdir /opt/zooservices_namespaces \
     && sed "s=/usr/bin/python3=/usr/miniconda3/envs/ades-dev/bin/python=g" -i /usr/lib/cgi-bin/subscriber.py /usr/lib/cgi-bin/publish.py \
     && ln -s $(which mapserv) /usr/lib/cgi-bin/ \
     && chown www-data:www-data -R /opt/zooservices_namespaces\
     && a2enmod ssl \
     #&& sed "s:Listen 80:Listen 8702:g;s:443:8703:g" -i /etc/apache2/ports.conf \
     && cd .. && rm -rf ZOO-Project

#
# Install Swagger-ui
#
RUN git clone --depth 1 https://github.com/swagger-api/swagger-ui.git \
    && mv swagger-ui /var/www/html/swagger-ui \
    #&& sed "s=https://petstore.swagger.io/v2/swagger.json=https://testbed19.geolabs.fr:8703/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/* \
    && sed "s=https://petstore.swagger.io/v2/swagger.json=http://localhost/ogc-api/api=g" -i /var/www/html/swagger-ui/dist/* \
    && mv /var/www/html/swagger-ui/dist /var/www/html/swagger-ui/oapip

COPY docker/default.conf /etc/apache2/sites-available/000-default.conf
COPY docker/hpgc/main.cfg /usr/lib/cgi-bin/main.cfg
COPY docker/hpgc/oas.cfg /usr/lib/cgi-bin/oas.cfg
COPY zoo-project/zoo-services/utils/open-api/dru/DeployProcess.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/DeployProcess.zcfg /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/UndeployProcess.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/UndeployProcess.zcfg /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/open-api/dru/deploy_util.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/security/openeo-udp/openeoFilterIn.zcfg /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/security/openeo-udp/openeo_filter.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/openeo/openeo_run.py /usr/lib/cgi-bin
COPY zoo-project/zoo-services/utils/openeo/OpenEO.py /usr/lib/cgi-bin

RUN chmod -R 777 /usr/lib/cgi-bin

RUN mkdir /tmp/cookiecutter-templates && \
    chmod -R 777 /tmp/cookiecutter-templates

EXPOSE 80
CMD ["apachectl", "-D", "FOREGROUND"]
