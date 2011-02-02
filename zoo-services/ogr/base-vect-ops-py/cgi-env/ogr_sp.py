import osgeo.ogr
import libxml2
import os
import sys

def createGeometryFromWFS(my_wfs_response):
    doc=libxml2.parseMemory(my_wfs_response,len(my_wfs_response))
    ctxt = doc.xpathNewContext()
    res=ctxt.xpathEval("/*/*/*/*/*[local-name()='Polygon' or local-name()='MultiPolygon' or local-name()='Point' or local-name()='MultiPoint' or local-name()='MultiLineString' or local-name()='LineString' ]")
    geometry=[]
    try:
        for node in res:
            geometry_as_string=node.serialize()
            geometry+=[osgeo.ogr.CreateGeometryFromGML(geometry_as_string)]
    except:
        print >> sys.stderr,"Error"
    return geometry

def extractInputs(obj):
    if obj["mimeType"]=="application/json":
	return [osgeo.ogr.CreateGeometryFromJson(obj["value"])]
    else:
	try:
        	return createGeometryFromWFS(obj["value"])
	except:
		return [osgeo.ogr.CreateGeometryFromJson(obj["value"])]
    return null
    
def outputResult(conf,obj,geom):
    driverName = "GML"
    extension = [ ".xml" , ".xsd" ]
    if obj["mimeType"]=="application/json":
        driverName = "GeoJSON"
        extension = [ ".js" ]
    drv = osgeo.ogr.GetDriverByName( driverName )
    ds = drv.CreateDataSource( conf["main"]["tmpPath"]+"/store"+conf["lenv"]["sid"]+extension[0] )
    lyr = ds.CreateLayer( "Result", None, osgeo.ogr.wkbUnknown )
    field_defn = osgeo.ogr.FieldDefn( "Name", osgeo.ogr.OFTString )
    field_defn.SetWidth( len("Result10000") )
    lyr.CreateField ( field_defn )
    i=0
    while i < len(geom):
        feat = osgeo.ogr.Feature( lyr.GetLayerDefn())
        feat.SetField( "Name", "Result"+str(i) )
        feat.SetGeometry(geom[i])
        lyr.CreateFeature(feat)
        feat.Destroy()
        geom[i].Destroy()
        i+=1
    ds.Destroy()
    obj["value"]=open(conf["main"]["tmpPath"]+"/store"+conf["lenv"]["sid"]+extension[0],"r").read()
    os.unlink(conf["main"]["tmpPath"]+"/store"+conf["lenv"]["sid"]+extension[0])
    if len(extension)>1:
        os.unlink(conf["main"]["tmpPath"]+"/store"+conf["lenv"]["sid"]+extension[1])

def BoundaryPy(conf,inputs,outputs):
    geometry=extractInputs(inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        rgeometries+=[geometry[i].GetBoundary()]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

def CentroidPy(conf,inputs,outputs):
    geometry=extractInputs(inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        if geometry[i].GetGeometryType()!=3:
            geometry[i]=geometry[i].ConvexHull()
        rgeometries+=[geometry[i].Centroid()]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

def ConvexHullPy(conf,inputs,outputs):
    geometry=extractInputs(inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        rgeometries+=[geometry[i].ConvexHull()]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3

def BufferPy(conf,inputs,outputs):
    try:
        bdist=float(inputs["BufferDistance"]["value"])
    except:
        bdist=10
    geometry=extractInputs(inputs["InputPolygon"])
    i=0
    rgeometries=[]
    while i < len(geometry):
        rgeometries+=[geometry[i].Buffer(bdist)]
        geometry[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    i=0
    return 3

def UnionPy(conf,inputs,outputs):
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
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
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
    rgeometries=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            tres=geometry1[i].Intersection(geometry2[j])
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

def DifferencePy(conf,inputs,outputs):
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
    rgeometries=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            tres=geometry1[i].Difference(geometry2[j])
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

def SymDifferencePy(conf,inputs,outputs):
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
    rgeometries=[]
    i=0
    while i < len(geometry1):
        j=0
        while j < len(geometry2):
            rgeometries+=[geometry1[i].SymmetricDifference(geometry2[j])]
            j+=1
        geometry1[i].Destroy()
        i+=1
    i=0
    while i < len(geometry2):
        geometry2[i].Destroy()
        i+=1
    outputResult(conf,outputs["Result"],rgeometries)
    return 3
