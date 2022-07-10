import sys
import pandas as pd

paths=sys.path
df=pd.DataFrame({'paths':paths})
df.to_csv('./qgis_sys_paths.csv',index=False)

import os
import json

env=dict(os.environ)
rem=['SECURITYSESSIONID', 'LaunchInstanceID', 'TMPDIR']
_=[env.pop(r,None) for r in rem]


with open('./qgis_env.json', 'w') as f:
    json.dump(env,f,ensure_ascii=False,indent=4)

from platform import python_version
print(python_version())

qspath='./qgis_sys_paths.csv'

paths=pd.read_csv(qspath).paths.tolist()
sys.path+=paths

qepath='./qgis_env.json'
js=json.loads(open(qepath,'r').read())
for k, v in js.items():
    os.environ[k]=v

os.environ['PROJ_LIB']='/usr/bin/qgis'

import PyQt5.QtCore
from osgeo import gdal
import qgis.PyQt.QtCore
from qgis.core import (QgsApplication,
                       QgsProcessingFeedback,
                       QgsProcessingRegistry)
from qgis.analysis import QgsNativeAlgorithms

feedback=QgsProcessingFeedback()
QgsApplication.setPrefixPath(js['HOME'], True)
qgs=QgsApplication([], False)
qgs.initQgis()

from processing.core.Processing import Processing
Processing.initialize()
import processing

algs = dict()
for alg in QgsApplication.processingRegistry().algorithms():
    algs[alg.displayName()] = alg.id()
print(algs)


processing.algorithmHelp("native:georeference")