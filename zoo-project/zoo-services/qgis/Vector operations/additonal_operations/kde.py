#define function to apply kernel density estimation
def qgisKDE(sourceFile,radius,radiusField,pixelSize,weightField,kernelShape,decayRatio,rawOrScaled,destinationFile):
    #set the algorithm to be applied
    algorithmToApply="qgis:heatmapkerneldensityestimation"
    #ensure the radius is positive
    if radius<=0:
        print("Radius must be positve")
    #ensure the pixel size is positive
    elif pixelSize<=0:
        print("Pixel size must be positive")
    #ensure kernel shape is 0-4
    elif kernelShape<0 or kernelShape>4:
        print("Kernel Shape must be 0-4\n0->Quadratic\n1->Triangular\n2->Uniform\n3->Triweight\n4->Epanechnikov")
    #ensure the raw or scaled flag is 0 or 1:
    elif rawOrScaled<0 or rawOrScaled>1:
        pritn("Raw or Scaled flag must be 0 or 1.\n0-Raw\n1-Scaled")
    #if the kernel is triangular ensure decay ratio is positive
    elif kernelShape==1 and decayRatio<=0:
        print("For a triangular kernel, decay ratio must be positive")
    else:
        #set parameters for the transform
        transformParameters={
            'INPUT':sourceFile,
            'RADIUS':radius,
            'RADIUS_FIELD':radiusField,
            'PIXEL_SIZE':pixelSize,
            'WEIGHT_FIELD':WeightField,
            'KERNEL':kernelShape,
            'OUTPUT_VALUE':rawOrScaled,
            'DECAY':decayRatio,
            'OUTPUT':destinationFile
        }
        #apply the transform
        processing.run(algorithmToApply,transformParameters)
       
        #processed message
        print("Data processed")