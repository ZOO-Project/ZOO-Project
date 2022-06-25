#include <iostream>
#include "qgsgcppoint.h"
#include "qgspointxy.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsgcptransformer.h"
//#inlcude "qgsgcpgeometrytransformer.h"

int main(){

	//specify the source coordinates for 3 GCPs
	// format (x1,y1), (x2,y2), (x3,y3)
	sourceCoordinates=[[30,41],[45,51],[60,21]]
	//specify the destination coordinates for 3 GCPs
	// format (x1,y1), (x2,y2), (x3,y3)
	destinationCoordinates=[[43.6789,21.4567],[45.3421,23.4678],[47.7645,17.3456]]
	//specify the destination Coordinate Reference System (CRS)
	crsCode=4326 
	destinationCRS=QgsCoordinateReferenceSystem.fromEpsgId(crsCode)



	//create first GCP
 	
 	//specify point and coordiante indices
 	pointIndex=0
 	xIndex=0
 	yIndex=1
	//create a QGSPoint in the source coordinate system i.e. row no. and col no.
	sourcePoint=QgsPointXY(sourceCoordinates[pointIndex,xIndex],sourceCoordinates[pointIndex,yIndex])
	//create a QGSPoint in the destination coordinate system
	destinationPoint=QgsPointXY(destinationCoordinates[pointIndex,xIndex],destinationCoordinates[pointIndex,yIndex])
	//create a CGP out of the source coordiantes, destination coordinates and the CRS and enable it
	gcp1=QgsGcpPoint(sourcePoint,destinationPoint,destinationCRS,true)



	//create second GCP
 
	//specify point and coordiante indices
 	pointIndex=1

	//create a QGSPoint in the source coordinate system i.e. row no. and col no.
	sourcePoint=QgsPointXY(sourceCoordinates[pointIndex,xIndex],sourceCoordinates[pointIndex,xIndex])
	//create a QGSPoint in the destination coordinate system
	destinationPoint=QgsPointXY(destinationCoordinates[pointIndex,xIndex],destinationCoordinates[pointIndex,xIndex])
	//create a CGP out of the source coordiantes, destination coordinates and the CRS and enable it
	gcp3=QgsGcpPoint(sourcePoint,destinationPoint,destinationCRS,true)



	//create third GCP
 
	//specify point and coordiante indices
 	pointIndex=2

	//create a QGSPoint in the source coordinate system i.e. row no. and col no.
	sourcePoint=QgsPointXY(sourceCoordinates[pointIndex,xIndex],sourceCoordinates[pointIndex,xIndex])
	//create a QGSPoint in the destination coordinate system
	destinationPoint=QgsPointXY(destinationCoordinates[pointIndex,xIndex],destinationCoordinates[pointIndex,xIndex])
	//create a CGP out of the source coordiantes, destination coordinates and the CRS and enable it
	gcp3=QgsGcpPoint(sourcePoint,destinationPoint,destinationCRS,true)

	
	//create a GCP transformer which converts points from the source coordinate system to the destination coordinate system
	


	std::cout << "First GCP created \n";
	




	return 0;
}