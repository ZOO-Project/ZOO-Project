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

# set up environment variables
qepath = './qgis_env.json'
js = json.loads(open(qepath,'r').read())
for k, v in js.items():
    os.environ[k]=v

# In special cases, we might also need to map the PROJ_LIB to handle the projections
os.environ['PROJ_LIB'] = '/Applications/Qgis.app/Contents/Resources/proj'

import PyQt5.QtCore
from osgeo import gdal
import qgis.PyQt.QtCore
from qgis.core import (QgsApplication,
                       QgsProcessingFeedback,
                       QgsProcessingRegistry)
from qgis.analysis import QgsNativeAlgorithms


feedback = QgsProcessingFeedback()# initializing processing module
QgsApplication.setPrefixPath(js['HOMEPATH'], True)
qgs = QgsApplication([], False)
qgs.initQgis() # use qgs.exitQgis() to exit the processing module at the end of the script.# initialize processing algorithms
from processing.core.Processing import Processing
Processing.initialize()
import processing
QgsApplication.processingRegistry().addProvider(QgsNativeAlgorithms())

algs = dict()
for alg in QgsApplication.processingRegistry().algorithms():
    algs[alg.displayName()] = alg.id()

#create a csv file
#destinationFile=open('C:/Users/104D/Documents/GSoC/qgis_algorithm_detials.txt', 'w')

#write details of each algorithm to the CVS file
for key,i in zip(algs,range(len(algs))):
    currentLine=str(i)+"--"+key+"--"+algs[key]+"\n"
    #print(currentLine)
    #destinationFile.write(currentLine)

#destinationFile.close()
    
#set the algorithm to be applied
algorithmToApply="qgis:randomselection"
#processing.algorithmHelp(algorithmToApply)


#define function to apply random selection
def qgisRandomSelection(sourceFile,method,numberOrPercentParameter,destinationFile):
    #set the algorithm to apply
    algorithmToApply="qgis:randomselection"
    #check if the scale factor for X direction is 0
    if method not in [0,1]:
        print("Method must be 0 or 1.\n0-Select a given number of features.\n1-Select a given percent of features")
    #ensure that if percent is selected the perecent is between 0 and 100
    elif method==1 and (numberOrPercentParameter>100 or numberOrPercentParameter<0):
        print("Percent of features to be sampled must be between 0 and 100")
    #ensure thata if number of features is selected the number is greater than 0
    elif method==0 and numberOrPercentParameter<=0:
        print("The number of features to be sampled must be postive")
    else:
        #set the parameters for the transform
        transformParameters={
            'METHOD':method,
            'NUMBER':numberOrPercentParameter,
            'INPUT':sourceFile,
            'OUTPUT':destinationFile,  
        }
        #apply the transform
        print(processing.run(algorithmToApply,transformParameters))
        #processed message
        print("Data processed")


#read a geojson file containing the geometry on which the transform is to be applied
sourceFileAddress='F:/GSOC22/Files/Vector operations/uttarPradesh.geojson'
#set the strategy
method=1 #currently set to sampling by percent
#set the parameter for the percent of features to be sampled
numberOrPercentParameter=50
#set the destination for the transformed geometry
destinationFileAddress='F:/GSOC22/Files/Vector operations/randomFeatures_UttarPradesh.geojson'



#apply the algorithm
qgisRandomSelection(sourceFileAddress,method,numberOrPercentParameter,destinationFileAddress)