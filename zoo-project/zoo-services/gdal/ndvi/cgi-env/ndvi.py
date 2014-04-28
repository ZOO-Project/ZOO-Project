#
# Python Scripting for ArcGIS
# by Dr Peter Bunting is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License -
# See more at:
# http://www.landmap.ac.uk/index.php/Learning-Materials/Python-Scripting/9.4-Calculate-NDVI-using-GDAL
#

import sys, os, struct

import osgeo.gdal as gdal

# Calculate and output NDVI from raster bands
def ExtractNDVI(conf, inputs, outputs):
    
    # Open the input dataset
    gdal.FileFromMemBuffer('/vsimem//temp.tif', inputs["raster"]["value"])
    dataset = gdal.Open( '/vsimem//temp.tif')
    if dataset is None:
        conf["lenv"]["message"]="The dataset could not be openned properly"
        return 4

    # Create the output dataset
    driver = gdal.GetDriverByName( "GTiff" )
    # Get the spatial information from the input file
    geoTransform=None
    geoProjection=None
    print >> sys.stderr,dir(dataset) 
    try:
        geoTransform = dataset.GetGeoTransform()
    except:
        print >> sys.stderr, "Unable to load geotransform"
    try:
        geoProjection = dataset.GetProjection()
    except:
        print >> sys.stderr, "Unable to load projection"

    # Create an output file of the same size as the inputted 
    # image but with only 1 output image band.
    newDataset = driver.Create("/vsimem//output"+conf["lenv"]["sid"], \
                               dataset.RasterXSize, dataset.RasterYSize,1, \
                               gdal.GDT_Float32)
    # Set spatial informations of the new image.
    if geoTransform:
        newDataset.SetGeoTransform(geoTransform)
    if geoProjection:
        newDataset.SetProjection(geoProjection)
    if newDataset is None:
        conf["lenv"]["message"]='Could not create output image'
        return 4
    
    # Get the RED and NIR image bands of the image
    red_id=int(inputs["red"]["value"])
    nir_id=int(inputs["nir"]["value"])
    red_band = dataset.GetRasterBand(red_id) # RED BAND
    nir_band = dataset.GetRasterBand(nir_id) # NIR BAND

    # Loop through each line in turn.
    numLines = red_band.YSize
    for line in range(numLines):
        # Define variable for output line.
        outputLine = ''
        # Read in data for the current line from the 
        # image band representing the red wavelength
        red_scanline = red_band.ReadRaster( 0, line, red_band.XSize, 1, \
                                          red_band.XSize, 1, gdal.GDT_Float32 )
        # Unpack the line of data to be read as floating point data
        red_tuple = struct.unpack('f' * red_band.XSize, red_scanline)
            
        # Read in data for the current line from the 
        # image band representing the NIR wavelength
        nir_scanline = nir_band.ReadRaster( 0, line, nir_band.XSize, 1, \
                                          nir_band.XSize, 1, gdal.GDT_Float32 )
        # Unpack the line of data to be read as floating point data
        nir_tuple = struct.unpack('f' * nir_band.XSize, nir_scanline)

        # Loop through the columns within the image
        for i in range(len(red_tuple)):
            # Calculate the NDVI for the current pixel.
            ndvi_lower = (nir_tuple[i] + red_tuple[i])
            ndvi_upper = (nir_tuple[i] - red_tuple[i])
            ndvi = 0
            # Becareful of zero divide 
            if ndvi_lower == 0:
                ndvi = 0
            else:
                ndvi = ndvi_upper/ndvi_lower
            # Add the current pixel to the output line
            outputLine = outputLine + struct.pack('f', ndvi)
        # Write the completed line to the output image
        newDataset.GetRasterBand(1).WriteRaster(0, line, red_band.XSize, 1, \
                                         outputLine, buf_xsize=red_band.XSize, 
                                         buf_ysize=1, buf_type=gdal.GDT_Float32)
    
        # Delete the output line following write
        del outputLine
    print >> sys.stderr,'NDVI Calculated and Outputted to File'
    print >> sys.stderr,dir(newDataset)
    newDataset.FlushCache()
    vsiFile=gdal.VSIFOpenL("/vsimem//output"+conf["lenv"]["sid"],"r")
    i=0
    while gdal.VSIFSeekL(vsiFile,0,os.SEEK_END)>0:
        i+=1
    fileSize=gdal.VSIFTellL(vsiFile)
    gdal.VSIFSeekL(vsiFile,0,os.SEEK_SET)
    outputs["raster"]["value"]=gdal.VSIFReadL(fileSize,1,vsiFile)
    gdal.Unlink("/vsimem//output"+conf["lenv"]["sid"])
    return 3
