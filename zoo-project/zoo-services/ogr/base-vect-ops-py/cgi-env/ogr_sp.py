# -*- coding: utf-8 -*-
#
# Author : Gérald FENOY
#
# Copyright 2009-2013 GeoLabs SARL. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including with
# out limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

from osgeo import *
import osgeo.ogr
import osgeo.gdal
import libxml2
import os
import sys
import zoo

def readFileFromBuffer(data,ext):
    try:
        geometry=[]
        print >> sys.stderr,'/vsimem//temp1'+ext
        #print >> sys.stderr,data
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
    except Exception,e:
        print >> sys.stderr,e
        return []
    
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
        geom=osgeo.ogr.CreateGeometryFromGML(my_wfs_response.replace('<?xml version="1.0" encoding="utf-8"?>\n','').replace('<?xml version=\'1.0\' encoding="utf-8"?>\n',''))
    except Exception,e:
        print >> sys.stderr,"**"
        print >> sys.stderr,e
        geom=None
    try:
        print >> sys.stderr,geom is None
        if geom is None:
            if not(conf["lenv"].has_key("cnt")):
                conf["lenv"]["cnt"]=0
            else:
                conf["lenv"]["cnt"]+=1
            return readFileFromBuffer(my_wfs_response,str(conf["lenv"]["cnt"]))
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
    if obj.keys().count("cache_file"):
        print >> sys.stderr,obj
        geometry=[]
        ds = osgeo.ogr.Open(obj["cache_file"])
        lyr = ds.GetLayer(0)
        feat = lyr.GetNextFeature()
        while feat is not None:
            geometry+=[feat.Clone()]
            feat.Destroy()
            feat = lyr.GetNextFeature()
        ds.Destroy()
        return geometry    
    if obj["mimeType"]=="application/json":
        return createLayerFromJson(conf,obj["value"])
    else:
        return createGeometryFromWFS(conf,obj["value"])
    
def outputResult(conf,obj,geom):
    if obj["mimeType"].count("text/xml")>0:
        driverName = "GML"
        extension = [ ".xml" , ".xsd" ]
        format_list = { "2.": 'GML2', "3.1.1": 'GML3', "3.1": 'GML3Deegree', "3.2": 'GML3.2' }
        opts=['FORMAT=%s,GML3_LONGSRS=YES',format_list["3.2"]]
        for i in format_list:
            if obj["mimeType"].count(i)>0:
                opts=['FORMAT=%s,GML3_LONGSRS=YES',format_list[i]]
    if obj["mimeType"]=="application/json":
        driverName = "GeoJSON"
        extension = [ ".js" ]
        opts=None
    if obj.keys().count("schema")>0 and \
            obj["schema"]=="http://schemas.opengis.net/kml/2.2.0/ogckml22.xsd":
        driverName = "KML"
        extension = [ ".kml" ]
        opts=None
    drv = osgeo.ogr.GetDriverByName( driverName )
    print >> sys.stderr,drv
    # Create virtual file
    ds = drv.CreateDataSource( "/vsimem/store"+conf["lenv"]["sid"]+extension[0],options = opts)
    print >> sys.stderr,ds
    lyr = ds.CreateLayer( "Result", None, osgeo.ogr.wkbUnknown )
    print >> sys.stderr,lyr
    i=0
    print >> sys.stderr,driverName
    print >> sys.stderr,extension
    while i < len(geom):
        if i==0 and driverName!="GeoJSON":
            poDstFDefn=geom[i].GetDefnRef()
            if poDstFDefn is not None:
                nDstFieldCount = poDstFDefn.GetFieldCount()
                for iField in range(nDstFieldCount):
                    poSrcFieldDefn = poDstFDefn.GetFieldDefn(iField)
                    oFieldDefn = osgeo.ogr.FieldDefn(poSrcFieldDefn.GetNameRef(),poSrcFieldDefn.GetType())
                    oFieldDefn.SetWidth( poSrcFieldDefn.GetWidth() )
                    oFieldDefn.SetPrecision( poSrcFieldDefn.GetPrecision() )
                    lyr.CreateField( oFieldDefn )
        try:
            lyr.CreateFeature(geom[i])
        except:
            pass
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
    print >> sys.stderr, "Starting service ..."
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
    return zoo.SERVICE_SUCCEEDED

def Clean(conf,inputs,outputs):
    from shapely.wkb import loads
    print >> sys.stderr, "Starting service ..."
    features=extractInputs(conf,inputs["InputData"])
    i=0
    rgeometries=[]
    while i < len(features):
        tmp=features[i].Clone()
        resg=features[i].GetGeometryRef()
        if resg is not None:
            geom = loads(resg.ExportToWkb())
            if geom.is_valid:
                tmp.SetGeometryDirectly(resg)
                rgeometries+=[tmp]
                print >> sys.stderr,"valid !"
            else:
                print >> sys.stderr,"invalid !"
                print >> sys.stderr,geom.wkt[0:50]
        features[i].Destroy()
        resg.thisown=False 
        tmp.thisown=False
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    i=0
    print >> sys.stderr,"Return"
    return zoo.SERVICE_SUCCEEDED

def TransformService(conf,inputs,outputs):
    from osgeo import osr
    geometry=extractInputs(conf,inputs["InputData"])
    sourceRef = osr.SpatialReference()
    tmp=inputs["SourceCRS"]["value"].split(":")
    sourceRef.ImportFromEPSG(int(tmp[len(tmp)-1]))
    targetRef = osr.SpatialReference()    
    tmp=inputs["TargetCRS"]["value"].split(":")
    targetRef.ImportFromEPSG(int(tmp[len(tmp)-1]))
    transform = osr.CoordinateTransformation(sourceRef, targetRef)
    i=0
    rgeometries=[]
    while i < len(geometry):
        tmp=geometry[i].Clone()
        resg=geometry[i].GetGeometryRef()
        resg.Transform(transform)
        tmp.SetGeometryDirectly(resg.Clone())
        rgeometries+=[tmp]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["TransformedData"],rgeometries)
    return zoo.SERVICE_SUCCEEDED

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
    return zoo.SERVICE_SUCCEEDED

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
    return zoo.SERVICE_SUCCEEDED

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
    return zoo.SERVICE_SUCCEEDED



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
            tmp=geometry1[i].Clone()
            resg=geometry2[j].GetGeometryRef()
            resg=resg.Union(geometry1[i].GetGeometryRef())
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

def IntersectionPy(conf,inputs,outputs):

    geometry1=extractInputs(conf,inputs["InputEntity1"])
    geometry2=extractInputs(conf,inputs["InputEntity2"])

    print >> sys.stderr,str(len(geometry1))+" "+str(len(geometry2))

    rgeometries=[]
    fids=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            tmp=geometry2[j].Clone()
            resg=geometry2[j].GetGeometryRef()
            #resg=resg.Intersection(geometry1[i].GetGeometryRef())
            resg=geometry1[i].GetGeometryRef().Intersection(resg)
            tmp.SetGeometryDirectly(resg)
            if resg is not None and not(resg.IsEmpty()) and fids.count(tmp.GetFID())==0:
                rgeometries+=[tmp]
                fids+=[tmp.GetFID()]
            else:
                tmp.Destroy()
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

