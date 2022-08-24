#get the latest python base image from the docker hub
FROM python:latest

#create a directory where the files will be stored
RUN mkdir -p /var/zooqgis

#set the working directory
WORKDIR /var/zooqgis

#copy the python files to the working directory
copy ./ /var/zooqgis

#download numpy
RUN pip install numpy

#run the python file
CMD ["python","/var/zooqgis/helloWorld.py"]