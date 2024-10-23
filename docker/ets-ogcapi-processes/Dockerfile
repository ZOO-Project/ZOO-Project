FROM maven:3.8.3-jdk-8-slim AS build
ARG BUILD_DEPS=" \
    git \
    unzip \
"
WORKDIR /home/app
COPY docker/ets-ogcapi-processes/src /home/app/src
COPY docker/ets-ogcapi-processes/src1 /home/app/src1
RUN apt-get update && \
    apt-get install -y $BUILD_DEPS && \
    echo "teamengine building..." && \
    curl -o /home/app/src1/pom.xml https://raw.githubusercontent.com/opengeospatial/ets-common/ecabb653c869b36c47feed31fbd67fcbc089af81/pom.xml && \
    mvn -f /home/app/src/pom.xml clean install > log && \
    echo "common building..." && \
    mvn -f /home/app/src1/pom.xml clean install  && \
    echo "OGC API - Processes ETS building..." && \
    mvn -f /home/app/src1/ets-ogcapi-processes10/pom.xml clean install && \
    echo "WPS 2.0 ETS building..." && \
    mvn -f /home/app/src1/ets-wps20/pom.xml clean install && \
    apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS && \
    rm -rf /var/lib/apt/lists/*

#
# Create the container to be run based on tomcat
#
FROM tomcat:9.0.89-jre8
ARG BUILD_DEPS=" \
    unzip \
"
COPY --from=build /home/app/src/teamengine-web/target/teamengine*.war /root
COPY --from=build /home/app/src/teamengine-web/target/teamengine-*common-libs.zip /root
COPY --from=build /home/app/src/teamengine-console/target/teamengine-console-*-base.zip /root
COPY --from=build /home/app/src1/ets-ogcapi-processes10/target/ets-ogcapi-processes10-*-aio.jar /root
COPY --from=build /home/app/src1/ets-ogcapi-processes10/target/ets-ogcapi-processes10-*-ctl.zip /root
COPY --from=build /home/app/src1/ets-ogcapi-processes10/target/ets-ogcapi-processes10-*-deps.zip /root
COPY --from=build /home/app/src1/ets-wps20/target/ets-wps20-*-aio.jar /root
COPY --from=build /home/app/src1/ets-wps20/target/ets-wps20-*-ctl.zip /root
COPY --from=build /home/app/src1/ets-wps20/target/ets-wps20-*-deps.zip /root
ENV JAVA_OPTS="-Xms1024m -Xmx2048m -DTE_BASE=/root/te_base"
RUN cd /root && \
    apt-get update && \
    apt-get install -y gnupg2 && \
    apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 04EE7237B7D453EC 648ACFD622F3D138 0E98404D386FA1D9 EF0F382A1A7B6500 && \
    mkdir te_base && \
    mkdir te_base/scripts && \
    echo "deb http://archive.debian.org/debian stretch main" > /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y $BUILD_DEPS && \
    unzip -q -o teamengine*.war -d /usr/local/tomcat/webapps/teamengine && \
    unzip -q -o teamengine-*common-libs.zip -d /usr/local/tomcat/lib && \
    unzip -q -o teamengine-console-*-base.zip -d /root/te_base && \
    for i in ets-ogcapi-processes10 ets-wps20 ; do \
	unzip -q -o ${i}-*-ctl.zip -d /root/te_base/scripts ; \
	unzip -q -o ${i}-*-deps.zip -d /usr/local/tomcat/webapps/teamengine/WEB-INF/lib ; \
    done && \
    apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS && \
    rm -rf /var/lib/apt/lists/* /root/*zip /root/*war


# run tomcat
CMD ["catalina.sh", "jpda", "run"]
