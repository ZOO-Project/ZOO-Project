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
    #curl -L -o 5.5.2.zip https://github.com/opengeospatial/teamengine/archive/refs/tags/5.5.2.zip && \
    #unzip 5.5.2.zip && \
    #mv teamengine-5.5.2 src && \
    #git clone https://github.com/opengeospatial/teamengine src && \
    #git clone https://github.com/opengeospatial/ets-common.git src1 && \
    #cd src1 && \
    #git clone https://github.com/opengeospatial/ets-ogcapi-processes10.git && \
    echo "teamengine building..." && \
    mvn -f /home/app/src/pom.xml clean install && \
    echo "common building..." && \
    mvn -f /home/app/src1/pom.xml clean install  && \
    echo "teamengine building..." && \
    mvn -f /home/app/src1/ets-ogcapi-processes10/pom.xml clean install && \
    rm -f *zip && \
    apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS && \
    rm -rf /var/lib/apt/lists/*

#
# Create the container to be run based on tomcat
#
FROM tomcat:7.0-jre8
ARG BUILD_DEPS=" \
    unzip \
"
COPY --from=build /home/app/src/teamengine-web/target/teamengine*.war /root
COPY --from=build /home/app/src/teamengine-web/target/teamengine-*common-libs.zip /root
COPY --from=build /home/app/src/teamengine-console/target/teamengine-console-*-base.zip /root
COPY --from=build /home/app/src1/ets-ogcapi-processes10/target/ets-ogcapi-processes10-*-aio.jar /root
COPY --from=build /home/app/src1/ets-ogcapi-processes10/target/ets-ogcapi-processes10-*-ctl.zip /root
COPY --from=build /home/app/src1/ets-ogcapi-processes10/target/ets-ogcapi-processes10-*-deps.zip /root
ENV JAVA_OPTS="-Xms1024m -Xmx2048m -DTE_BASE=/root/te_base"
RUN cd /root && \
    mkdir te_base && \
    mkdir te_base/scripts && \
    echo "deb http://archive.debian.org/debian stretch main" > /etc/apt/sources.list && \
    apt-get update && \
    apt-get install -y $BUILD_DEPS && \
    unzip -q -o teamengine*.war -d /usr/local/tomcat/webapps/teamengine && \
    unzip -q -o teamengine-*common-libs.zip -d /usr/local/tomcat/lib && \
    unzip -q -o teamengine-console-*-base.zip -d /root/te_base && \
    unzip -q -o ets-ogcapi-processes10-*-ctl.zip -d /root/te_base/scripts && \
    unzip -q -o ets-ogcapi-processes10-*-deps.zip -d /usr/local/tomcat/webapps/teamengine/WEB-INF/lib && \
    apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false $BUILD_DEPS && \
    rm -rf /var/lib/apt/lists/* /root/*zip /root/*war


# run tomcat
CMD ["catalina.sh", "jpda", "run"]
