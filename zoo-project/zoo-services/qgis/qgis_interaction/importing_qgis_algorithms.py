import os
import sys
import json
import pandas as pd
qspath='./qgis_sys_paths.csv' 
paths=pd.read_csv(qspath).paths.tolist()
sys.path+=paths
qepath='./qgis_env.json'
js=json.loads(open(qepath,'r').read())
for k, v in js.items():
    os.environ[k] = v
os.environ['PROJ_LIB']='/Applications/Qgis.app/Contents/Resources/proj'
import PyQt5.QtCore
from osgeo import gdal
import qgis.PyQt.QtCore
from qgis.core import (QgsApplication,
                       QgsProcessingFeedback,
                       QgsProcessingRegistry)
from qgis.analysis import QgsNativeAlgorithms
feedback=QgsProcessingFeedback()
QgsApplication.setPrefixPath(js['HOME'],True)
qgs=QgsApplication([],False)
qgs.initQgis()
from processing.core.Processing import Processing
Processing.initialize()
import processing
algs = dict()
for alg in QgsApplication.processingRegistry().algorithms():
    algs[alg.displayName()] = alg.id()
for i in range(len(algs)):
    print(f"{i}---{algs[i]}")