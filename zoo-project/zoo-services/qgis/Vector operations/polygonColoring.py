
#set the algorithm to be applied
algorithmToApply="qgis:topologicalcoloring"
processing.algorithmHelp(algorithmToApply)



#define function to apply topological coloring
def qgisTopologicalColouring(sourceFile,minimumColors,minimumDistance,balancingParameter,destinationFile):
    #set the algorithm to be applied
    algorithmToApply="qgis:topologicalcoloring"
    #ensure that the minimum number of colors is postive
    if minimumColors<1:
        print("Minimum number of colors cannot be negetive or 0. It must be 1 or greater")
    #ensure the minimum distance is postive
    elif minimumDistance<=0:
        print("Minimum distance between features cannot be negetive.")
    #ensure the color balancing term is between 0 and 2
    elif balancingParameter>2 or balancingParameter<0:
        print("balancing parameter must be 0,1 or 2.\nKey\n0--Feature Count\n1--Assigned Area\n2--Distance between Colors")
    else:
        #set parameters for the transform
        transformParameters={
            'INPUT':sourceFile,
            'MIN_COLORS':minimumColors,
            'MIN_DISTANCE':minimumDistance,
            'BALANCE':balancingParameter,
            'OUTPUT':destinationFile
        }
        #apply the transform
        processing.run(algorithmToApply,transformParameters)
        
        #processed message
        print("Data processed")


#read a geojson file containing the geometry that is to be coloured
sourceFileAddress='F:/GSOC22/Files/Vector operations/uttarPradesh.geojson'
#set the minimum colors
minimumNoOfColors=3
#set the minimum distance
minimumDistanceBetweenFeatures=2
#balancing parameter
parameterToBeBalanced=2
#set the destination for the transformed geometry
destinationFileAddress='F:/GSOC22/Files/Vector operations/coloured_UttarPradesh.geojson'


qgisTopologicalColouring(sourceFileAddress,
                         minimumNoOfColors,
                         minimumDistanceBetweenFeatures,
                         parameterToBeBalanced,
                         destinationFileAddress)