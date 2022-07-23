#set the algorithm to be applied
algorithmToApply="qgis:randompointsinsidepolygons"
processing.algorithmHelp(algorithmToApply)

#define function to apply topological coloring
def qgisRandomPointsInsidePolygons(sourceFile,samplingStrategy,strategyValue,expression,minimumDistance,destinationFile):
    #set the algorithm to be applied
    algorithmToApply="qgis:randompointsinsidepolygons"
    #esnure sample strategy is 0 or  1
    if not(samplingStrategy==1 or samplingStrategy==0):
        print("Sampling strategy should be 0 or 1")
    #ensure stategy value is positive
    elif strategyValue<=0:
        print("Minimum value cannot be negetive.")
    #ensure the minimum distance is greater than 0
    elif minimumDistance<=0:
        print("Distance between points must be Postive")
    else:
        #set parameters for the transform
        transformParameters={
            'INPUT':sourceFile,
            'STRATEGY':samplingStrategy,
            'VALUE':strategyValue,
            'EXPRESSION':expression,
            'MIN_DISTANCE':minimumDistance,
            'OUTPUT':destinationFile
        }
        #apply the transform
        processing.run(algorithmToApply,transformParameters)
        
        #processed message
        print("Data processed")


#read a geojson file containing the geometry that is to be coloured
sourceFileAddress='F:/GSOC22/Files/Vector operations/uttarPradesh.geojson'
#set the sampling strategy flag
samplingStrategyFlag=3
#set the value for the strategy
samplingValue=2
#balancing parameter
minimumDistanece=0.05
#set the destination for the transformed geometry
destinationFileAddress='F:/GSOC22/Files/Vector operations/pointsInside_UttarPradesh.geojson'


qgisRandomPointsInsidePolygons(sourceFileAddress,
                               samplingStrategy,
                               strategyValue,
                               expression,
                               minimumDistance,
                               destinationFileAddress)