/**
 * Author : Gérald FENOY
 *
 * Copyright 2009-2013 GeoLabs SARL. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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


