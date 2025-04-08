FROM ubuntu:18.04
ENV PATH="/root/miniconda3/bin:${PATH}"
ARG PATH="/root/miniconda3/bin:${PATH}"
RUN apt-get update

RUN apt-get install -y wget && rm -rf /var/lib/apt/lists/*

RUN wget \
    https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh \
    && mkdir /root/.conda \
    && bash Miniconda3-latest-Linux-x86_64.sh -b \
    && rm -f Miniconda3-latest-Linux-x86_64.sh 
RUN conda --version

#create a directory where the files will be stored
RUN mkdir -p /var/zooqgis

#set the working directory
WORKDIR /var/zooqgis

#copy the python files to the working directory
copy ./ /var/zooqgis

#download numpy
RUN conda install numpy

#show hello world
RUN echo "Hello from ubuntu"

#run the python file
CMD python /var/zooqgis/helloWorld.py