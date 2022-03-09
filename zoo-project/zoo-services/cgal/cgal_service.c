/**
 * Author : Gérald FENOY
 *
 * Copyright 2009-2013 GeoLabs SARL. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "cgal_service.h"

int parseInput(maps* conf,maps* inputs, std::vector<Pointz>* points,char* filename){
  map* tmpm=NULL;
  tmpm=getMapFromMaps(inputs,"InputPoints","cache_file");
#if GDAL_VERSION_MAJOR >= 2
  GDALDataset *ipoDS;
#else
  OGRDataSource* ipoDS;
#endif
  if(tmpm==NULL){
    tmpm=getMapFromMaps(inputs,"InputPoints","value");
    VSILFILE *ifile=VSIFileFromMemBuffer(filename,(GByte*)tmpm->value,strlen(tmpm->value),FALSE);
    VSIFCloseL(ifile);
#if GDAL_VERSION_MAJOR >= 2
  ipoDS = (GDALDataset*) GDALOpenEx( filename,
				     GDAL_OF_READONLY | GDAL_OF_VECTOR,
				     NULL, NULL, NULL );
#else
  ipoDS = OGRSFDriverRegistrar::Open(filename,FALSE);
#endif
  }else
#if GDAL_VERSION_MAJOR >= 2
    ipoDS = (GDALDataset*) GDALOpenEx( tmpm->value,
				       GDAL_OF_READONLY | GDAL_OF_VECTOR,
				       NULL, NULL, NULL );
#else
    ipoDS = OGRSFDriverRegistrar::Open(tmpm->value,FALSE);
#endif
    
  if( ipoDS == NULL )
    {
#if GDAL_VERSION_MAJOR >= 2
      GDALDriverManager* poR=GetGDALDriverManager();
#else
      OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
#endif
      
      fprintf( stderr, "FAILURE:\n"
	       "Unable to open datasource `%s' with the following drivers.\n",
	       filename );
      
      for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
        {
#if GDAL_VERSION_MAJOR >= 2
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetDescription() );
#else
	    fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
#endif
        }
      char tmp[1024];
      sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",filename);
      setMapInMaps(conf,"lenv","message",tmp);
      return SERVICE_FAILED;
    }
  for( int iLayer = 0; iLayer < ipoDS->GetLayerCount();
       iLayer++ )
    {
      OGRLayer        *poLayer = ipoDS->GetLayer(iLayer);
      
      if( poLayer == NULL )
	{
	  fprintf( stderr, "FAILURE: Couldn't fetch advertised layer %d!\n",
		   iLayer );
#ifdef ZOO_SERVICE
	  char tmp[1024];
	  sprintf(tmp,"Couldn't fetch advertised layer %d!",iLayer);
	  setMapInMaps(conf,"lenv","message",tmp);
	  return SERVICE_FAILED;
#else
	  exit( 1 );
#endif
	}
      
      OGRFeature  *poFeature;
      while(TRUE){
	poFeature = poLayer->GetNextFeature();
	if( poFeature == NULL )
	  break;
	if(poFeature->GetGeometryRef() != NULL){
	  points->push_back(Pointz(OGR_G_GetX(poFeature->GetGeometryRef(),0),OGR_G_GetY(poFeature->GetGeometryRef(),0)));
	}
      }
    }
  return SERVICE_SUCCEEDED;
}


