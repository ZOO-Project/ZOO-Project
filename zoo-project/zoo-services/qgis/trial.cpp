#include <iostream>
#include "qgsgcppoint.h"
#include "qgspointxy.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsgcptransformer.h"
//#inlcude "qgsgcpgeometrytransformer.h"

int main(){

	//specify the source coordinates for 3 GCPs with random points for testing 
	// format (x1,y1), (x2,y2), (x3,y3)
	sourceCoordinates=[[30,41],[45,51],[60,21]];
	//specify the destination coordinates for 3 GCPs
	// format (x1,y1), (x2,y2), (x3,y3)
	destinationCoordinates=[[43.6789,21.4567],[45.3421,23.4678],[47.7645,17.3456]];
	//specify the destination Coordinate Reference System (CRS)
	crsCode=4326;
	destinationCRS=QgsCoordinateReferenceSystem.fromEpsgId(crsCode);



	//create first GCP
 	
 	//specify point and coordiante indices
 	pointIndex=0;
 	xIndex=0;
 	yIndex=1;
	//create a QGSPoint in the source coordinate system i.e. row no. and col no.
	sourcePoint1=QgsPointXY(sourceCoordinates[pointIndex,xIndex],sourceCoordinates[pointIndex,yIndex]);
	//create a QGSPoint in the destination coordinate system
	destinationPoint1=QgsPointXY(destinationCoordinates[pointIndex,xIndex],destinationCoordinates[pointIndex,yIndex]);
	//create a CGP out of the source coordiantes, destination coordinates and the CRS and enable it
	gcp1=QgsGcpPoint(sourcePoint,destinationPoint,destinationCRS,true);



	//create second GCP
 
	//specify point and coordiante indices
 	pointIndex=1;

	//create a QGSPoint in the source coordinate system i.e. row no. and col no.
	sourcePoint2=QgsPointXY(sourceCoordinates[pointIndex,xIndex],sourceCoordinates[pointIndex,xIndex]);
	//create a QGSPoint in the destination coordinate system
	destinationPoint2=QgsPointXY(destinationCoordinates[pointIndex,xIndex],destinationCoordinates[pointIndex,xIndex]);
	//create a CGP out of the source coordiantes, destination coordinates and the CRS and enable it
	gcp3=QgsGcpPoint(sourcePoint,destinationPoint,destinationCRS,true);



	//create third GCP
 
	//specify point and coordiante indices
 	pointIndex=2;

	//create a QGSPoint in the source coordinate system i.e. row no. and col no.
	sourcePoint3=QgsPointXY(sourceCoordinates[pointIndex,xIndex],sourceCoordinates[pointIndex,xIndex]);
	//create a QGSPoint in the destination coordinate system
	destinationPoint3=QgsPointXY(destinationCoordinates[pointIndex,xIndex],destinationCoordinates[pointIndex,xIndex]);
	//create a CGP out of the source coordiantes, destination coordinates and the CRS and enable it
	gcp3=QgsGcpPoint(sourcePoint,destinationPoint,destinationCRS,true);

	
	//create an array form the source points
	QgsPointXY sourcePoints[3]={sourcePoint1,sourcePoint2,sourcePoint3};
	//create an array from the destination points
	QgsPointXY destinationPoints[3]={destinationPoint1,destinationPoint2,destinationPoint3};


	//create a GCP transformer which converts points from the source coordinate system to the destination coordinate system
	
	//specify the type of transformation eg. linear, Helmert, polymonial etc
	/*
		Linear, //!< Linear transform
	    Helmert, //!< Helmert transform
	    PolynomialOrder1, //!< Polynomial order 1
	    PolynomialOrder2, //!< Polyonmial order 2
	    PolynomialOrder3, //!< Polynomial order
	    ThinPlateSpline, //!< Thin plate splines
	    Projective, //!< Projective
	    InvalidTransform = 65535 //!< Invalid transform
    };
	*/
	transformType=TransformMethod(0);
	//create the transform
	gcpTransform=QgsGcpTransformerInterface.create(transformType) ;
	//use the source and destination points to get the paramters for the transformer
	parameterUpdateFlag=gcpTransform.updateParametersFromGcps(sourcePoints,destinationPoints,invertYAxis=false);

	//check if the parameters were successfully updated
	if (parameterUpdateFlag){
		std::cout << "parameters updated";
	}
	else{
		std::cout << "parameters not updated";
		return 0;
	}


	//get a transformer that can be applied to geometries (points) from the transform calculated from the GCPs
	geometryTransformer=QgsGcpGeometryTransformer(gcpTransform);

	//specfy a point which on which the transform is to be applied
	sourceX=55;
	sourceY=57;
	testPointSource=QgsPointXY(sourceX,sourceY);

	//create an array out of the test point(s)
	testPoints[1]={testPointSource};

	//create a flag that will be updated based on wether the transform was applied or not
	bool applicationFlag=false;
	//apply the transform on the test point
	transformedPoint=geometryTransformer.transform(testPoints,*applicationFlag)
	



	std::cout << "First GCP created \n";
	




	return 0;
}