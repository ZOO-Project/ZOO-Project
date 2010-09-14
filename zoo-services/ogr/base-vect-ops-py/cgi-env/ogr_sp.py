import osgeo.ogr
import libxml2

def createGeometryFromWFS(my_wfs_response):
    doc=libxml2.parseMemory(my_wfs_response,len(my_wfs_response))
    ctxt = doc.xpathNewContext()
    res=ctxt.xpathEval("/*/*/*/*/*[local-name()='Polygon' or local-name()='MultiPolygon']")
    for node in res:
        geometry_as_string=node.serialize()
        geometry=osgeo.ogr.CreateGeometryFromGML(geometry_as_string)
	return geometry
    return geometry

def extractInputs(obj):
    if obj["mimeType"]=="application/json":
	return osgeo.ogr.CreateGeometryFromJson(obj["value"])
    else:
	try:
        	return createGeometryFromWFS(obj["value"])
	except:
		return osgeo.ogr.CreateGeometryFromJson(obj["value"])
    return null
    
def outputResult(obj,geom):
    if obj["mimeType"]=="application/json":
        obj["value"]=geom.ExportToJson()
        obj["mimeType"]="text/plain"
    else:
        obj["value"]=geom.ExportToGML()

def BoundaryPy(conf,inputs,outputs):
    geometry=extractInputs(inputs["InputPolygon"])
    rgeom=geometry.GetBoundary()
    if outputs["Result"]["mimeType"]=="application/json":
        outputs["Result"]["value"]=rgeom.ExportToJson()
        outputs["Result"]["mimeType"]="text/plain"
    else:
        outputs["Result"]["value"]=rgeom.ExportToGML()
    return 3

def CentroidPy(conf,inputs,outputs):
    geometry=extractInputs(inputs["InputPolygon"])
    if geometry.GetGeometryType()!=3:
        geometry=geometry.ConvexHull()
    rgeom=geometry.Centroid()
    outputResult(outputs["Result"],rgeom)
    return 3

def ConvexHullPy(conf,inputs,outputs):
    geometry=extractInputs(inputs["InputPolygon"])
    rgeom=geometry.ConvexHull()
    outputResult(outputs["Result"],rgeom)
    return 3

def BufferPy(conf,inputs,outputs):
    geometry=createGeometryFromWFS(inputs["InputPolygon"]["value"])
    try:
        bdist=int(inputs["BufferDistance"]["value"])
    except:
        bdist=10
    rgeom=geometry.Buffer(bdist)
    if outputs["Result"]["mimeType"]=="application/json":
        outputs["Result"]["value"]=rgeom.ExportToJson()
        outputs["Result"]["mimeType"]="text/plain"
    else:
        outputs["Result"]["value"]=rgeom.ExportToGML()
    return 3

def UnionPy(conf,inputs,outputs):
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
    rgeom=geometry1.Union(geometry2)
    outputResult(outputs["Result"],rgeom)
    return 3

def IntersectionPy(conf,inputs,outputs):
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
    rgeom=geometry1.Intersection(geometry2)
    outputResult(outputs["Result"],rgeom)
    return 3

def DifferencePy(conf,inputs,outputs):
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
    rgeom=geometry1.Difference(geometry2)
    outputResult(outputs["Result"],rgeom)
    return 3

def SymDifferencePy(conf,inputs,outputs):
    geometry1=extractInputs(inputs["InputEntity1"])
    geometry2=extractInputs(inputs["InputEntity2"])
    rgeom=geometry1.SymmetricDifference(geometry2)
    outputResult(outputs["Result"],rgeom)
    return 3
