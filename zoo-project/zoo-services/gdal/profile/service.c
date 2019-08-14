/* ****************************************************************************
 * $Id$
 *
 * Project:  GdalExtractProfile
 * Purpose:  Extract Profile from a Raster file for an Input Geometry (LINE)
 * Author:   Gérald Fenoy, gerald.fenoy@geolabs.fr
 *
 * ****************************************************************************
 * Copyright (c) 2010-2011, GeoLabs SARL
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifdef ZOO_SERVICE
#include "service.h"
#include "service_internal.h"
#endif
#include "gdal.h"
#include "cpl_conv.h"
#include "ogr_api.h"

#ifdef ZOO_SERVICE
extern "C" {
#endif

#ifdef WIN32
__declspec(dllexport)
#endif
#ifdef ZOO_SERVICE
int GdalExtractProfile(maps*& conf,maps*& inputs,maps*& outputs)
#else
int main(int argc,char** argv)
#endif
{
  char *pszFilename;
#ifdef ZOO_SERVICE
  map* tmp=NULL;
  map* tmp1=NULL;
  tmp=getMapFromMaps(conf,"main","dataPath");
  tmp1=getMapFromMaps(inputs,"RasterFile","value");
  pszFilename=(char *)malloc((2+strlen(tmp->value)+strlen(tmp1->value))*sizeof(char));
  sprintf(pszFilename,"%s/%s",tmp->value,tmp1->value);
#else
  pszFilename=argv[1];
#endif
  GDALDatasetH  hDataset;  
  GDALAllRegister();
  OGRRegisterAll();
 
  hDataset = GDALOpen( pszFilename, GA_ReadOnly );
  free(pszFilename);
  if( hDataset != NULL )
    {
      GDALDriverH   hDriver;
      double        adfGeoTransform[6];

      if( GDALGetGeoTransform( hDataset, adfGeoTransform ) == CE_None )
	{


        GDALRasterBandH hBand;
        int             nBlockXSize, nBlockYSize;
        int             bGotMin, bGotMax;
        double          adfMinMax[2];
        
        hBand = GDALGetRasterBand( hDataset, 1 );

        adfMinMax[0] = GDALGetRasterMinimum( hBand, &bGotMin );
        adfMinMax[1] = GDALGetRasterMaximum( hBand, &bGotMax );
        if( ! (bGotMin && bGotMax) )
	  GDALComputeRasterMinMax( hBand, TRUE, adfMinMax );
#ifdef ZOO_SERVICE
	  tmp1=getMapFromMaps(inputs,"Geometry","value");
	  OGRGeometryH geometry=OGR_G_CreateGeometryFromJson(tmp1->value);
#else
	  OGRGeometryH geometry=OGR_G_CreateGeometryFromJson(argv[2]);
#endif

	  OGR_G_Segmentize(geometry, adfGeoTransform[1]);
	  int nbGeom=OGR_G_GetPointCount(geometry);
	  int k=0;
	  double ppx=0,ppy=0;
	  double value;
	  char *buffer=NULL;
	  int length=0;
	  buffer=(char*)malloc(37*sizeof(char));
	  sprintf(buffer,"{\"type\":\"LineString\",\"coordinates\":[");
	  length+=strlen(buffer);
	  for(k=0;k<nbGeom;k++){
	    //OGRGeometryH point;
	    double prx,pry,prz;
	    OGR_G_GetPoint(geometry,k,&prx,&pry,&prz);
	    float *pafScanline;
	    pafScanline = (float *) CPLMalloc(sizeof(float));
	    int px=(int)floor((prx-adfGeoTransform[0])/adfGeoTransform[1]);
	    int py=(int)floor((pry-adfGeoTransform[3])/adfGeoTransform[5]);
	    if(px!=ppx || py!=ppy){
	      if(GDALRasterIO( hBand, GF_Read, px, py, 1, 1, 
			    pafScanline, 1, 1, GDT_Float32, 
			       0, 0 ) != CE_None){
		char *tmp;
		tmp=(char*) malloc(300*sizeof(char));
		sprintf(tmp,"GDALRasterIO failed for point (%d,%d)",px,py);
		setMapInMaps(conf,"lenv","message",_ss(tmp));
		CPLFree(pafScanline);
		free(tmp);
		return SERVICE_FAILED;
	      }
	      if(buffer!=NULL){
		int len=strlen(buffer);
		buffer=(char*)realloc(buffer,(len+50+1)*sizeof(char));
	      }
	      else
		buffer=(char*)malloc((51)*sizeof(char));
	      char *tmpValue=(char *)malloc(50*sizeof(char));
	      sprintf(tmpValue,"[%.6f,%.6f,%.6f]%c",prx,pry,pafScanline[0],(k+1==nbGeom?' ':','));
	      strncpy(buffer+length,tmpValue,strlen(tmpValue));
	      length+=strlen(tmpValue);
	      buffer[length]=0;
	      value=pafScanline[0];
	      free(tmpValue);
	      //Usefull if we can export 3D JSON string at the end
	      //OGR_G_SetPoint(geometry,k,prx,pry,pafScanline[0]);	      
	    }
	    else{
	      if(buffer!=NULL)
		buffer=(char*)realloc(buffer,(strlen(buffer)+50+1)*sizeof(char));
	      else
		buffer=(char*)malloc((51)*sizeof(char));
	      char *tmpValue=(char *)malloc(50*sizeof(char));
	      sprintf(tmpValue,"[%.6f,%.6f,%.6f]%c",prx,pry,value,(k+1==nbGeom?' ':','));
	      strncpy(buffer+length,tmpValue,strlen(tmpValue));
	      length+=strlen(tmpValue);
	      buffer[length]=0;
	      free(tmpValue);
	      value=value;
	    }
	    CPLFree(pafScanline);
	    ppx=px;
	    ppy=py;
	  }
	  buffer=(char*)realloc(buffer,(strlen(buffer)+3)*sizeof(char));
	  char *tmpValue=(char *)malloc(3*sizeof(char));
	  sprintf(tmpValue,"]}");
	  tmpValue[2]=0;
	  strncpy(buffer+length,tmpValue,strlen(tmpValue));
	  length+=strlen(tmpValue);
	  buffer[length]=0;
#ifdef ZOO_SERVICE
	  setMapInMaps(outputs,"Profile","value",buffer);
	  setMapInMaps(outputs,"Profile","mimeType","text/plain");
#else
	  fprintf(stderr,"%s\n",buffer);
#endif
	  free(buffer);
	  free(tmpValue);
	  OGR_G_DestroyGeometry(geometry);
	}
    }
  else{
#ifdef ZOO_SERVICE
    setMapInMaps(conf,"lenv","message",_ss("Unable to load your raster file !"));
    return SERVICE_FAILED;
#else
    printf("Unable to load your raster file %s !\n",argv[1]);
#endif
  }
  GDALClose(hDataset);
  OGRCleanupAll();
  GDALDestroyDriverManager();
#ifdef ZOO_SERVICE
  return SERVICE_SUCCEEDED;
#endif
}

#ifdef ZOO_SERVICE
}
#endif
