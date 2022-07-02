import sys
import pandas as pd
paths = sys.path
df=pd.DataFrame({'paths':paths})
df.to_csv('./qgis_sys_paths.csv',index=False)



import os
import json
env = dict(os.environ)
rem = ['SECURITYSESSIONID', 'LaunchInstanceID', 'TMPDIR']
_ = [env.pop(r,None) for r in rem]


with open('./qgis_env.json', 'w') as f:
    json.dump(env,f,ensure_ascii=False,indent=4)



from platform import python_version
print(python_version())


# set up system paths
qspath='./qgis_sys_paths.csv' 
# provide the path where you saved this file.
paths=pd.read_csv(qspath).paths.tolist()
sys.path+=paths



# In special cases, we might also need to map the PROJ_LIB to handle the projections
os.environ['PROJ_LIB'] = '/Applications/Qgis.app/Contents/Resources/proj'



import PyQt5.QtCore
from osgeo import gdal
import qgis.PyQt.QtCore
from qgis.core import (QgsApplication,
                       QgsProcessingFeedback,
                       QgsProcessingRegistry)
from qgis.analysis import QgsNativeAlgorithms


#add native algorithms from QGIS to the processing directory
feedback = QgsProcessingFeedback()# initializing processing module
QgsApplication.setPrefixPath(js['HOME'], True)
qgs = QgsApplication([], False)
qgs.initQgis() # use qgs.exitQgis() to exit the processing module at the end of the script.# initialize processing algorithms
from processing.core.Processing import Processing
Processing.initialize()
import processingQgsApplication.processingRegistry().addProvider(QgsNativeAlgorithms())


#print the list of available algorithms
algs = dict()
for alg in QgsApplication.processingRegistry().algorithms():
    algs[alg.displayName()] = alg.id()
print(algs)