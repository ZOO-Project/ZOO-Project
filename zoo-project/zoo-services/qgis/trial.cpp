#include <iostream>
#include "qgsgcppoint.h"
#include "qgspointxy.h"
#include "qgscoordinatereferencesystem.h"
//#include "qgsgcptransformer.h"
//#inlcude "qgsgcpgeometrytransformer.h"

int main(){

	//create first GCP
 
	//specify the source coordinates of a ground control point (GCP)
	//random values chosen
	sourceX=30
	sourceY=41
	//create a QGSPoint in the source coordinate system i.e. row no. and col no.
	sourcePoint=QgsPointXY(sourceX,sourceY)
	
	//specify the destination coordinates of the ground control point
	//random values chosen
	destinationX=43.6789
	destinationY=21.4567
	//create a QGSPoint in the destination coordinate system
	destinationPoint=QgsPointXY(destinationX,destinationY)

	//specify the destination Coordinate Reference System (CRS) 
	destinationCRS=QgsCoordinateReferenceSystem.fromEpsgId()

	//create a CGP out of the source coordiantes, destination coordinates and the CRS and enable it
	gcp1=QgsGcpPoint(sourcePoint,destinationPoint,destinationCRS,true)

	std::cout << "Hello World \n";
	return 0;
}