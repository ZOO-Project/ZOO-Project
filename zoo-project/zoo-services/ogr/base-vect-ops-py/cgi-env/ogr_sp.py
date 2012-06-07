from osgeo import *
import osgeo.ogr
import osgeo.gdal
import libxml2
import os
import sys

def readFileFromBuffer(data,ext):
    geometry=[]
    osgeo.gdal.FileFromMemBuffer('/vsimem//temp1'+ext,data)
    ds = osgeo.ogr.Open('/vsimem//temp1'+ext)
    lyr = ds.GetLayer(0)
    feat = lyr.GetNextFeature()
    while feat is not None:
        geometry+=[feat.Clone()]
        feat.Destroy()
        feat = lyr.GetNextFeature()
    ds.Destroy()
    osgeo.gdal.Unlink('/vsimem//temp1'+ext)
    return geometry

def buildFeatureFromGeomtry(conf,geom,driverName,ext):
    drv = osgeo.ogr.GetDriverByName( driverName )
    ds = drv.CreateDataSource( "/vsimem//store"+conf["lenv"]["sid"]+"0."+ext )
    lyr = ds.CreateLayer( "Result", None, osgeo.ogr.wkbUnknown )
    field_defn = osgeo.ogr.FieldDefn( "Name", osgeo.ogr.OFTString )
    field_defn.SetWidth( len("Result10000") )
    lyr.CreateField ( field_defn )
    feat = osgeo.ogr.Feature(lyr.GetLayerDefn())
    feat.SetField( "Name", "Input0" )
    feat.SetGeometry(geom)
    lyr.CreateFeature(feat)
    ds.Destroy()
    return [feat]

def createGeometryFromWFS(conf,my_wfs_response):
    try:
        geom=osgeo.ogr.CreateGeometryFromGML(my_wfs_response.replace('<?xml version="1.0" encoding="utf-8"?>\n',''))
    except:
        geom=None
    try:
        if geom is None:
            return readFileFromBuffer(my_wfs_response,"")
        else:
            return buildFeatureFromGeomtry(conf,geom,"GML","xml")
    except:
        print >> sys.stderr,"Unable to load file input data !!!\n\n\n"

def createLayerFromJson(conf,obj):
    geom=osgeo.ogr.CreateGeometryFromJson(obj)
    if geom is None:
        return readFileFromBuffer(obj,".json")
    else:
        return buildFeatureFromGeomtry(conf,geom,"GeoJSON","json")

def extractInputs(conf,obj):
    if obj["mimeType"]=="application/json":
        return createLayerFromJson(conf,obj["value"])
    else:
        return createGeometryFromWFS(conf,obj["value"])
    
def outputResult(conf,obj,geom):
    driverName = "GML"
    extension = [ ".xml" , ".xsd" ]
    if obj["mimeType"]=="application/json":
        driverName = "GeoJSON"
        extension = [ ".js" ]
    if obj.keys().count("schema")>0 and \
            obj["schema"]=="http://schemas.opengis.net/kml/2.2.0/ogckml22.xsd":
        driverName = "KML"
        extension = [ ".kml" ]
    drv = osgeo.ogr.GetDriverByName( driverName )
    # Create virtual file 
    ds = drv.CreateDataSource( "/vsimem/store"+conf["lenv"]["sid"]+extension[0] )
    lyr = ds.CreateLayer( "Result", None, osgeo.ogr.wkbUnknown )
    i=0
    while i < len(geom):
        lyr.CreateFeature(geom[i])
        geom[i].Destroy()
        i+=1
    ds.Destroy()
    vsiFile=osgeo.gdal.VSIFOpenL("/vsimem/store"+conf["lenv"]["sid"]+extension[0],"r")
    i=0
    while osgeo.gdal.VSIFSeekL(vsiFile,0,os.SEEK_END)>0:
        i+=1
    fileSize=osgeo.gdal.VSIFTellL(vsiFile)
    osgeo.gdal.VSIFSeekL(vsiFile,0,os.SEEK_SET)
    obj["value"]=osgeo.gdal.VSIFReadL(fileSize,1,vsiFile)
    osgeo.gdal.Unlink("/vsimem/store"+conf["lenv"]["sid"]+extension[0])

def BufferPy(conf,inputs,outputs):
    print >> sys.stderr, inputs
    print >> sys.stderr, outputs
    try:
        bdist=float(inputs["BufferDistance"]["value"])
    except:
        bdist=1
    print >> sys.stderr, bdist
    
    geometry=extractInputs(conf,inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        tmp=geometry[i].Clone()
        resg=geometry[i].GetGeometryRef().Buffer(bdist)
        tmp.SetGeometryDirectly(resg)
        rgeometries+=[tmp]
        geometry[i].Destroy()
        resg.thisown=False 
        tmp.thisown=False
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    i=0
    return 3

def BoundaryPy(conf,inputs,outputs):
    geometry=extractInputs(conf,inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        tmp=geometry[i].Clone()
        resg=geometry[i].GetGeometryRef()
        resg=resg.GetBoundary()
        tmp.SetGeometryDirectly(resg)
        rgeometries+=[tmp]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

def CentroidPy(conf,inputs,outputs):
    geometry=extractInputs(conf,inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        tmp=geometry[i].Clone()
        resg=geometry[i].GetGeometryRef()
        if resg.GetGeometryType()!=3:
            resg=resg.ConvexHull()
        resg=resg.Centroid()
        tmp.SetGeometryDirectly(resg)
        rgeometries+=[tmp]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

def ConvexHullPy(conf,inputs,outputs):
    geometry=extractInputs(conf,inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        tmp=geometry[i].Clone()
        resg=geometry[i].GetGeometryRef().ConvexHull()
        tmp.SetGeometryDirectly(resg)
        rgeometries+=[tmp]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3



def EnvelopePy(conf,inputs,outputs):
    print >> sys.stderr, inputs
    try:
        bdist=float(inputs["BufferDistance"]["value"])
    except:
        bdist=10
    geometry=extractInputs(conf,inputs["InputPolygon"])
    tmp=geometry[0].GetGeometryRef().GetEnvelope()
    outputs["Result"]["value"]=str(tmp[0])+','+str(tmp[2])+','+str(tmp[1])+','+str(tmp[3])+','+'urn:ogc:def:crs:OGC:1.3:CRS84'
    print >> sys.stderr,outputs["Result"]
    return 3

def UnionPy(conf,inputs,outputs):
    geometry1=extractInputs(conf,inputs["InputEntity1"])
    geometry2=extractInputs(conf,inputs["InputEntity2"])
    rgeometries=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            tres=geometry1[i].Union(geometry2[j])
            if not(tres.IsEmpty()):
                rgeometries+=[tres]
            j+=1
        geometry1[i].Destroy()
        i+=1
    i=0
    while i < len(geometry2):
        geometry2[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

def IntersectionPy(conf,inputs,outputs):

    geometry1=extractInputs(conf,inputs["InputEntity1"])
    print >> sys.stderr,inputs["InputEntity2"]
    geometry2=extractInputs(conf,inputs["InputEntity2"])

    rgeometries=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            tmp=geometry2[j].Clone()
            resg=geometry2[j].GetGeometryRef()
            #resg=resg.Intersection(geometry1[i].GetGeometryRef())
            resg=geometry1[i].GetGeometryRef().Intersection(resg)
            tmp.SetGeometryDirectly(resg)
            if not(resg.IsEmpty()):
                rgeometries+=[tmp]
            j+=1
        geometry1[i].Destroy()
        i+=1
    i=0
    while i < len(geometry2):
        geometry2[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    print >> sys.stderr,"/outputResult"
    return 3

def DifferencePy(conf,inputs,outputs):
    geometry1=extractInputs(conf,inputs["InputEntity1"])
    geometry2=extractInputs(conf,inputs["InputEntity2"])
    rgeometries=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            tmp=geometry2[j].Clone()
            resg=geometry1[i].GetGeometryRef()
            resg=resg.Difference(geometry2[i].GetGeometryRef())
            tmp.SetGeometryDirectly(resg)
            if not(resg.IsEmpty()):
                rgeometries+=[tmp]
            j+=1
        geometry1[i].Destroy()
        i+=1
    i=0
    while i < len(geometry2):
        geometry2[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

def SymDifferencePy(conf,inputs,outputs):
    geometry1=extractInputs(conf,inputs["InputEntity1"])
    geometry2=extractInputs(conf,inputs["InputEntity2"])
    rgeometries=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            tmp=geometry2[j].Clone()
            resg=geometry1[i].GetGeometryRef()
            resg=resg.SymmetricDifference(geometry2[i].GetGeometryRef())
            tmp.SetGeometryDirectly(resg)
            rgeometries+=[tmp]
            j+=1
        geometry1[i].Destroy()
        i+=1
    i=0
    while i < len(geometry2):
        geometry2[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

