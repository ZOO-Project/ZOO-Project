#!/bin/sh

#build the docker image
docker build . -t zooqgis
#run the docker image
docker run zooqgis 