#!/usr/bin/env python
# coding: utf-8

# In[1]:


from IPython.display import display, HTML
display(HTML("<style>.container { width:100% !important; }</style>"))


# In[2]:


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


# In[3]:


feedback = QgsProcessingFeedback()# initializing processing module
QgsApplication.setPrefixPath(js['HOMEPATH'], True)
qgs = QgsApplication([], False)
qgs.initQgis() # use qgs.exitQgis() to exit the processing module at the end of the script.# initialize processing algorithms
from processing.core.Processing import Processing
Processing.initialize()
import processing
QgsApplication.processingRegistry().addProvider(QgsNativeAlgorithms())


# In[4]:


algs = dict()
for alg in QgsApplication.processingRegistry().algorithms():
    algs[alg.displayName()] = alg.id()


# In[5]:


#create a csv file
#destinationFile=open('C:/Users/104D/Documents/GSoC/qgis_algorithm_detials.txt', 'w')

#write details of each algorithm to the CVS file
for key,i in zip(algs,range(len(algs))):
    currentLine=str(i)+"--"+key+"--"+algs[key]+"\n"
    print(currentLine)
    #destinationFile.write(currentLine)

#destinationFile.close()
    


# In[6]:


#set the algorithm to be applied
algorithmToApply="native:affinetransform"
processing.algorithmHelp(algorithmToApply)


# In[23]:


#define function to apply affine transform
def qgisAffineTransform(sourceFile,deltaX,deltaY,deltaZ,deltaM,scaleX,scaleY,scaleZ,scaleM,rotationZ,destinationFile):
    #set the algorithm to apply
    algorithmToApply="native:affinetransform"
    #check if the scale factor for X direction is 0
    if scaleX==0:
        print("Scale factor for X dimension is 0")
    #check if the scale factor for Y direction is 0
    elif scaleY==0:
        print("Scale factor for Y dimension is 0")
    #check if the scale factor for Z direction is 0
    elif scaleZ==0:
        print("Scale factor for Z dimension is 0")
    #check if the scale factor for M direction is 0
    elif scaleM==0:
        print("Scale factor for M dimension is 0")
    else:
        #set the parameters for the transform
        transformParameters={
            'DELTA_X':deltaX,
            'DELTA_Y':deltaY,
            'DELTA_Z':deltaZ,
            'DELTA_M':deltaM,
            'SCALE_X':scaleX,
            'SCALE_Y':scaleY,
            'SCALE_Z':scaleZ,
            'SCALE_M':scaleM,
            'ROTATION_Z':rotationZ,
            'INPUT':sourceFile,
            'OUTPUT':destinationFile,  
        }
        #apply the transform
        processing.run(algorithmToApply,transformParameters)
        #processed message
        print("Data processed")


# In[34]:


#read a geojson file containing the geometry on which the transform is to be applied
sourceFileAddress='F:/GSOC22/Files/Affine transform/uttarPradesh.geojson'
#set translation parameters for the affine transforms
deltaXValue=1.2
deltaYValue=1.5
deltaZValue=0
deltaMValue=0
#set scale parameters for the affine transforms
scaleXValue=1
scaleYValue=1
scaleZValue=1
scaleMValue=1
#set rotation around the Z-axis
rotationZValue=0
#set the destination for the transformed geometry
destinationFileAddress='F:/GSOC22/Files/Affine transform/transformed_UttarPradesh.geojson'


# In[35]:


qgisAffineTransform(sourceFileAddress,
                    deltaXValue,
                    deltaYValue,
                    deltaZValue,
                    deltaMValue,
                    scaleXValue,
                    scaleYValue,
                    scaleZValue,
                    scaleMValue,
                    rotationZValue,
                    destinationFileAddress)


# In[36]:


#set the algorithm to be applied
algorithmToApply="native:centroids"
processing.algorithmHelp(algorithmToApply)


# In[47]:


#define function to apply affine transform
def qgisCentroids(sourceFile,parts,destinationFile):
    #set the algorithm to be applied
    algorithmToApply="native:centroids"
    #check if no centroids are to be calculated
    if parts==0:
        print("No centroids calcualted")
    else:
        #set parameters for the transform
        transformParameters={
            'INPUT':sourceFile,
            'ALL_PARTS':parts,
            'OUTPUT':destinationFile
        }
        #apply the transform
        processing.run(algorithmToApply,transformParameters)
        
        #processed message
        print("Data processed")


# In[48]:


#read a geojson file containing the geometry whose centroids are to be calculated
sourceFileAddress='F:/GSOC22/Files/Affine transform/uttarPradesh.geojson'
#set centroids are to be calcualted for which parts
allParts=True
#set the destination for the transformed geometry
destinationFileAddress='F:/GSOC22/Files/Affine transform/centroids_UttarPradesh.geojson'


# In[49]:


qgisCentroids(sourceFileAddress,
                allParts,
                destinationFileAddress)


# In[ ]:




