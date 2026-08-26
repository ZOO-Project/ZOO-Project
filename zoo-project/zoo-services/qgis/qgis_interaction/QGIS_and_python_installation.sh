#!/bin/sh
#install curl to download miniconda
apt-get install curl
#download and install miniconda
curl -o Miniconda3-latest-Linux-x86_64.sh https://repo.anaconda.com/miniconda/Miniconda3-py39_4.12.0-Linux-x86_64.sh
#install miniconda
bash Miniconda3-latest-Linux-x86_64.sh
#install pyqgis
conda install -c conda-forge qgis
#install pip
apt-get install python3-pip
#install scipy and matplotlib
pip install scipy
pip install matplotlib
#import pandas
pip install pandas
#import qgis algorithms in python
python3 importing_qgis_algorithms.py
