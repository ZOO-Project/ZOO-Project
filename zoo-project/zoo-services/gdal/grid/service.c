/* ****************************************************************************
 * $Id: gdal_grid.cpp 15053 2008-07-27 17:29:27Z rouault $
 *
 * Project:  GDAL Utilities
 * Purpose:  GDAL scattered data gridding (interpolation) tool
 * Author:   Andrey Kiselev, dron@ak4719.spb.edu
 *
 * ****************************************************************************
 * Copyright (c) 2007, Andrey Kiselev <dron@ak4719.spb.edu>
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

#include <vector>
#include <algorithm>

#include "cpl_string.h"
#include "gdal.h"
#include "gdal_alg.h"
#include "ogr_spatialref.h"
#include "ogr_api.h"

#ifdef ZOO_SERVICE
#include "service.h"
#endif

CPL_CVSID("$Id: gdal_grid.cpp 15053 2008-07-27 17:29:27Z rouault $");

#ifdef ZOO_SERVICE
extern "C" {
#endif

static const char szAlgNameInvDist[] = "invdist";
static const char szAlgNameAverage[] = "average";
static const char szAlgNameNearest[] = "nearest";
static const char szAlgNameMinimum[] = "minimum";
static const char szAlgNameMaximum[] = "maximum";
static const char szAlgNameRange[] = "range";

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()
{
#ifdef ZOO_SERVICE
  fprintf(stderr,
#else
    printf( 
#endif
        "Usage: gdal_grid [--help-general] [--formats]\n"
        "    [-ot {Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/\n"
        "          CInt16/CInt32/CFloat32/CFloat64}]\n"
        "    [-of format] [-co \"NAME=VALUE\"]\n"
        "    [-zfield field_name]\n"
        "    [-a_srs srs_def] [-spat xmin ymin xmax ymax]\n"
        "    [-l layername]* [-where expression] [-sql select_statement]\n"
        "    [-txe xmin xmax] [-tye ymin ymax] [-outsize xsize ysize]\n"
        "    [-a algorithm[:parameter1=value1]*]"
        "    [-quiet]\n"
        "    <src_datasource> <dst_filename>\n"
        "\n"
        "Available algorithms and parameters with their defaults:\n"
        "    Inverse distance to a power (default)\n"
        "        invdist:power=2.0:smoothing=0.0:radius1=0.0:radius2=0.0:angle=0.0:max_points=0:min_points=0:nodata=0.0\n"
        "    Moving average\n"
        "        average:radius1=0.0:radius2=0.0:angle=0.0:min_points=0:nodata=0.0\n"
        "    Nearest neighbor\n"
        "        nearest:radius1=0.0:radius2=0.0:angle=0.0:nodata=0.0\n"
        "    Various data metrics\n"
        "        <metric name>:radius1=0.0:radius2=0.0:angle=0.0:min_points=0:nodata=0.0\n"
        "        possible metrics are:\n"
        "            minimum\n"
        "            maximum\n"
        "            range\n"
        "\n");
#ifndef ZOO_SERVICE
    exit( 1 );
#endif
}

/************************************************************************/
/*                          GetAlgorithmName()                          */
/*                                                                      */
/*      Translates algortihm code into mnemonic name.                   */
/************************************************************************/

void PrintAlgorithmAndOptions(GDALGridAlgorithm eAlgorithm, void *pOptions)
{
    switch ( eAlgorithm )
    {
        case GGA_InverseDistanceToAPower:

#ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
		   "Algorithm name: \"%s\".\n", szAlgNameInvDist );
#ifdef ZOO_SERVICE
		fprintf(stderr,
#else
			  printf( 
#endif
				 "Options are "
				 "\"power=%f:smoothing=%f:radius1=%f:radius2=%f:angle=%f"
                    ":max_points=%lu:min_points=%lu:nodata=%f\"\n",
                ((GDALGridInverseDistanceToAPowerOptions *)pOptions)->dfPower,
                ((GDALGridInverseDistanceToAPowerOptions *)pOptions)->dfSmoothing,
                ((GDALGridInverseDistanceToAPowerOptions *)pOptions)->dfRadius1,
                ((GDALGridInverseDistanceToAPowerOptions *)pOptions)->dfRadius2,
                ((GDALGridInverseDistanceToAPowerOptions *)pOptions)->dfAngle,
                (unsigned long)((GDALGridInverseDistanceToAPowerOptions *)pOptions)->nMaxPoints,
                (unsigned long)((GDALGridInverseDistanceToAPowerOptions *)pOptions)->nMinPoints,
                ((GDALGridInverseDistanceToAPowerOptions *)pOptions)->dfNoDataValue);
            break;
        case GGA_MovingAverage:
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Algorithm name: \"%s\".\n", szAlgNameAverage );
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Options are "
                    "\"radius1=%f:radius2=%f:angle=%f:min_points=%lu"
                    ":nodata=%f\"\n",
                ((GDALGridMovingAverageOptions *)pOptions)->dfRadius1,
                ((GDALGridMovingAverageOptions *)pOptions)->dfRadius2,
                ((GDALGridMovingAverageOptions *)pOptions)->dfAngle,
                (unsigned long)((GDALGridMovingAverageOptions *)pOptions)->nMinPoints,
                ((GDALGridMovingAverageOptions *)pOptions)->dfNoDataValue);
            break;
        case GGA_NearestNeighbor:
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Algorithm name: \"%s\".\n", szAlgNameNearest );
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Options are "
                    "\"radius1=%f:radius2=%f:angle=%f:nodata=%f\"\n",
                ((GDALGridNearestNeighborOptions *)pOptions)->dfRadius1,
                ((GDALGridNearestNeighborOptions *)pOptions)->dfRadius2,
                ((GDALGridNearestNeighborOptions *)pOptions)->dfAngle,
                ((GDALGridNearestNeighborOptions *)pOptions)->dfNoDataValue);
            break;
        case GGA_MetricMinimum:
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Algorithm name: \"%s\".\n", szAlgNameMinimum );
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Options are "
                    "\"radius1=%f:radius2=%f:angle=%f:min_points=%lu"
                    ":nodata=%f\"\n",
                ((GDALGridDataMetricsOptions *)pOptions)->dfRadius1,
                ((GDALGridDataMetricsOptions *)pOptions)->dfRadius2,
                ((GDALGridDataMetricsOptions *)pOptions)->dfAngle,
                (unsigned long)((GDALGridDataMetricsOptions *)pOptions)->nMinPoints,
                ((GDALGridDataMetricsOptions *)pOptions)->dfNoDataValue);
            break;
        case GGA_MetricMaximum:
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Algorithm name: \"%s\".\n", szAlgNameMaximum );
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Options are "
                    "\"radius1=%f:radius2=%f:angle=%f:min_points=%lu"
                    ":nodata=%f\"\n",
                ((GDALGridDataMetricsOptions *)pOptions)->dfRadius1,
                ((GDALGridDataMetricsOptions *)pOptions)->dfRadius2,
                ((GDALGridDataMetricsOptions *)pOptions)->dfAngle,
                (unsigned long)((GDALGridDataMetricsOptions *)pOptions)->nMinPoints,
                ((GDALGridDataMetricsOptions *)pOptions)->dfNoDataValue);
            break;
        case GGA_MetricRange:
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Algorithm name: \"%s\".\n", szAlgNameRange );
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Options are "
                    "\"radius1=%f:radius2=%f:angle=%f:min_points=%lu"
                    ":nodata=%f\"\n",
                ((GDALGridDataMetricsOptions *)pOptions)->dfRadius1,
                ((GDALGridDataMetricsOptions *)pOptions)->dfRadius2,
                ((GDALGridDataMetricsOptions *)pOptions)->dfAngle,
                (unsigned long)((GDALGridDataMetricsOptions *)pOptions)->nMinPoints,
                ((GDALGridDataMetricsOptions *)pOptions)->dfNoDataValue);
            break;
        default:
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Algorithm unknown.\n" );
            break;
    }
}

/************************************************************************/
/*                      ParseAlgorithmAndOptions()                      */
/*                                                                      */
/*      Translates mnemonic gridding algorithm names into               */
/*      GDALGridAlgorithm code, parse control parameters and assign     */
/*      defaults.                                                       */
/************************************************************************/

static CPLErr ParseAlgorithmAndOptions( const char *pszAlgoritm,
                                        GDALGridAlgorithm *peAlgorithm,
                                        void **ppOptions )
{
    char **papszParms = CSLTokenizeString2( pszAlgoritm, ":", FALSE );

    if ( CSLCount(papszParms) < 1 )
        return CE_Failure;

    if ( EQUAL(papszParms[0], szAlgNameInvDist) )
        *peAlgorithm = GGA_InverseDistanceToAPower;
    else if ( EQUAL(papszParms[0], szAlgNameAverage) )
        *peAlgorithm = GGA_MovingAverage;
    else if ( EQUAL(papszParms[0], szAlgNameNearest) )
        *peAlgorithm = GGA_NearestNeighbor;
    else if ( EQUAL(papszParms[0], szAlgNameMinimum) )
        *peAlgorithm = GGA_MetricMinimum;
    else if ( EQUAL(papszParms[0], szAlgNameMaximum) )
        *peAlgorithm = GGA_MetricMaximum;
    else if ( EQUAL(papszParms[0], szAlgNameRange) )
        *peAlgorithm = GGA_MetricRange;
    else
    {
        fprintf( stderr, "Unsupported gridding method \"%s\".\n",
                 papszParms[0] );
        CSLDestroy( papszParms );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Parse algorithm parameters and assign defaults.                 */
/* -------------------------------------------------------------------- */
    const char  *pszValue;

    switch ( *peAlgorithm )
    {
        case GGA_InverseDistanceToAPower:
        default:
            *ppOptions =
                CPLMalloc( sizeof(GDALGridInverseDistanceToAPowerOptions) );

            pszValue = CSLFetchNameValue( papszParms, "power" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                dfPower = (pszValue) ? atof(pszValue) : 2.0;

            pszValue = CSLFetchNameValue( papszParms, "smoothing" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                dfSmoothing = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "radius1" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                dfRadius1 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "radius2" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                dfRadius2 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "angle" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                dfAngle = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "max_points" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                nMaxPoints = (pszValue) ? atol(pszValue) : 0;

            pszValue = CSLFetchNameValue( papszParms, "min_points" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                nMinPoints = (pszValue) ? atol(pszValue) : 0;

            pszValue = CSLFetchNameValue( papszParms, "nodata" );
            ((GDALGridInverseDistanceToAPowerOptions *)*ppOptions)->
                dfNoDataValue = (pszValue) ? atof(pszValue) : 0.0;
            break;

        case GGA_MovingAverage:
            *ppOptions =
                CPLMalloc( sizeof(GDALGridMovingAverageOptions) );

            pszValue = CSLFetchNameValue( papszParms, "radius1" );
            ((GDALGridMovingAverageOptions *)*ppOptions)->
                dfRadius1 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "radius2" );
            ((GDALGridMovingAverageOptions *)*ppOptions)->
                dfRadius2 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "angle" );
            ((GDALGridMovingAverageOptions *)*ppOptions)->
                dfAngle = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "min_points" );
            ((GDALGridMovingAverageOptions *)*ppOptions)->
                nMinPoints = (pszValue) ? atol(pszValue) : 0;

            pszValue = CSLFetchNameValue( papszParms, "nodata" );
            ((GDALGridMovingAverageOptions *)*ppOptions)->
                dfNoDataValue = (pszValue) ? atof(pszValue) : 0.0;
            break;

        case GGA_NearestNeighbor:
            *ppOptions =
                CPLMalloc( sizeof(GDALGridNearestNeighborOptions) );

            pszValue = CSLFetchNameValue( papszParms, "radius1" );
            ((GDALGridNearestNeighborOptions *)*ppOptions)->
                dfRadius1 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "radius2" );
            ((GDALGridNearestNeighborOptions *)*ppOptions)->
                dfRadius2 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "angle" );
            ((GDALGridNearestNeighborOptions *)*ppOptions)->
                dfAngle = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "nodata" );
            ((GDALGridNearestNeighborOptions *)*ppOptions)->
                dfNoDataValue = (pszValue) ? atof(pszValue) : 0.0;
            break;

        case GGA_MetricMinimum:
        case GGA_MetricMaximum:
        case GGA_MetricRange:
            *ppOptions =
                CPLMalloc( sizeof(GDALGridDataMetricsOptions) );

            pszValue = CSLFetchNameValue( papszParms, "radius1" );
            ((GDALGridDataMetricsOptions *)*ppOptions)->
                dfRadius1 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "radius2" );
            ((GDALGridDataMetricsOptions *)*ppOptions)->
                dfRadius2 = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "angle" );
            ((GDALGridDataMetricsOptions *)*ppOptions)->
                dfAngle = (pszValue) ? atof(pszValue) : 0.0;

            pszValue = CSLFetchNameValue( papszParms, "min_points" );
            ((GDALGridDataMetricsOptions *)*ppOptions)->
                nMinPoints = (pszValue) ? atol(pszValue) : 0;

            pszValue = CSLFetchNameValue( papszParms, "nodata" );
            ((GDALGridDataMetricsOptions *)*ppOptions)->
                dfNoDataValue = (pszValue) ? atof(pszValue) : 0.0;
            break;

   }

    CSLDestroy( papszParms );
    return CE_None;
}

/************************************************************************/
/*                            ProcessLayer()                            */
/*                                                                      */
/*      Process all the features in a layer selection, collecting       */
/*      geometries and burn values.                                     */
/************************************************************************/

static void ProcessLayer( OGRLayerH hSrcLayer, GDALDatasetH hDstDS,
                          GUInt32 nXSize, GUInt32 nYSize, int nBand,
                          int& bIsXExtentSet, int& bIsYExtentSet,
                          double& dfXMin, double& dfXMax,
                          double& dfYMin, double& dfYMax,
                          const char *pszBurnAttribute,
                          GDALDataType eType,
                          GDALGridAlgorithm eAlgorithm, void *pOptions,
                          int bQuiet, GDALProgressFunc pfnProgress )

{
/* -------------------------------------------------------------------- */
/*      Get field index, and check.                                     */
/* -------------------------------------------------------------------- */
    int iBurnField = -1;

    if ( pszBurnAttribute )
    {
        iBurnField = OGR_FD_GetFieldIndex( OGR_L_GetLayerDefn( hSrcLayer ),
                                           pszBurnAttribute );
        if( iBurnField == -1 )
        {
            #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Failed to find field %s on layer %s, skipping.\n",
                    pszBurnAttribute, 
                    OGR_FD_GetName( OGR_L_GetLayerDefn( hSrcLayer ) ) );
            return;
        }
    }

/* -------------------------------------------------------------------- */
/*      Collect the geometries from this layer, and build list of       */
/*      values to be interpolated.                                      */
/* -------------------------------------------------------------------- */
    OGRFeatureH hFeat;
    std::vector<double> adfX, adfY, adfZ;

    OGR_L_ResetReading( hSrcLayer );

    while( (hFeat = OGR_L_GetNextFeature( hSrcLayer )) != NULL )
    {
        OGRGeometryH hGeom;

        hGeom = OGR_F_GetGeometryRef( hFeat );

        // FIXME: handle collections
        if ( hGeom != NULL &&
             (OGR_G_GetGeometryType( hGeom ) == wkbPoint
              || OGR_G_GetGeometryType( hGeom ) == wkbPoint25D) )
        {
            adfX.push_back( OGR_G_GetX( hGeom, 0 ) );
            adfY.push_back( OGR_G_GetY( hGeom, 0 ) );
            if ( iBurnField < 0 )
                adfZ.push_back( OGR_G_GetZ( hGeom, 0 ) );
            else
                adfZ.push_back( OGR_F_GetFieldAsDouble( hFeat, iBurnField ) );
        }

        
        OGR_F_Destroy( hFeat );
    }

    if (adfX.size() == 0)
    {
        #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "No point geometry found on layer %s, skipping.\n",
                OGR_FD_GetName( OGR_L_GetLayerDefn( hSrcLayer ) ) );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Compute grid geometry.                                          */
/* -------------------------------------------------------------------- */

    if ( !bIsXExtentSet )
    {
        dfXMin = *std::min_element(adfX.begin(), adfX.end());
        dfXMax = *std::max_element(adfX.begin(), adfX.end());
        bIsXExtentSet = TRUE;
    }

    if ( !bIsYExtentSet )
    {
        dfYMin = *std::min_element(adfY.begin(), adfY.end());
        dfYMax = *std::max_element(adfY.begin(), adfY.end());
        bIsYExtentSet = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Perform gridding.                                               */
/* -------------------------------------------------------------------- */

    const double    dfDeltaX = ( dfXMax - dfXMin ) / nXSize;
    const double    dfDeltaY = ( dfYMax - dfYMin ) / nYSize;

    if ( !bQuiet )
    {
        #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Grid data type is \"%s\"\n", GDALGetDataTypeName(eType) );
        #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Grid size = (%lu %lu).\n",
                (unsigned long)nXSize, (unsigned long)nYSize );
        #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Corner coordinates = (%f %f)-(%f %f).\n",
                dfXMin - dfDeltaX / 2, dfYMax + dfDeltaY / 2,
                dfXMax + dfDeltaX / 2, dfYMin - dfDeltaY / 2 );
        #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Grid cell size = (%f %f).\n", dfDeltaX, dfDeltaY );
        #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
 "Source point count = %lu.\n", (unsigned long)adfX.size() );
        PrintAlgorithmAndOptions( eAlgorithm, pOptions );
        #ifdef ZOO_SERVICE
	  fprintf(stderr,
#else
            printf( 
#endif
"\n");
    }

    GDALRasterBandH hBand = GDALGetRasterBand( hDstDS, nBand );

    if (adfX.size() == 0)
    {
        // FIXME: Shoulda' set to nodata value instead
        GDALFillRaster( hBand, 0.0 , 0.0 );
        return;
    }

    GUInt32 nXOffset, nYOffset;
    int     nBlockXSize, nBlockYSize;

    GDALGetBlockSize( hBand, &nBlockXSize, &nBlockYSize );
    void    *pData =
        CPLMalloc( nBlockXSize * nBlockYSize * GDALGetDataTypeSize(eType) );

    GUInt32 nBlock = 0;
    GUInt32 nBlockCount = ((nXSize + nBlockXSize - 1) / nBlockXSize)
        * ((nYSize + nBlockYSize - 1) / nBlockYSize);

    for ( nYOffset = 0; nYOffset < nYSize; nYOffset += nBlockYSize )
    {
        for ( nXOffset = 0; nXOffset < nXSize; nXOffset += nBlockXSize )
        {
            void *pScaledProgress;
            pScaledProgress =
                GDALCreateScaledProgress( 0.0,
                                          (double)++nBlock / nBlockCount,
                                          pfnProgress, NULL );

            int nXRequest = nBlockXSize;
            if (nXOffset + nXRequest > nXSize)
                nXRequest = nXSize - nXOffset;

            int nYRequest = nBlockYSize;
            if (nYOffset + nYRequest > nYSize)
                nYRequest = nYSize - nYOffset;

            GDALGridCreate( eAlgorithm, pOptions,
                            adfX.size(), &(adfX[0]), &(adfY[0]), &(adfZ[0]),
                            dfXMin + dfDeltaX * nXOffset,
                            dfXMin + dfDeltaX * (nXOffset + nXRequest),
                            dfYMin + dfDeltaY * nYOffset,
                            dfYMin + dfDeltaY * (nYOffset + nYRequest),
                            nXRequest, nYRequest, eType, pData,
                            GDALScaledProgress, pScaledProgress );

            GDALRasterIO( hBand, GF_Write, nXOffset, nYOffset,
                          nXRequest, nYRequest, pData,
                          nXRequest, nYRequest, eType, 0, 0 );

            GDALDestroyScaledProgress( pScaledProgress );
        }
    }

    CPLFree( pData );
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/
#ifdef ZOO_SERVICE
int Gdal_Grid(maps*& conf,maps*& inputs,maps*& outputs)
#else
int main( int argc, char ** argv )
#endif
{
    GDALDriverH     hDriver;
    const char      *pszSource=NULL, *pszDest=NULL, *pszFormat = "GTiff";
    char            **papszLayers = NULL;
    const char      *pszBurnAttribute = NULL;
    const char      *pszWHERE = NULL, *pszSQL = NULL;
    GDALDataType    eOutputType = GDT_Float64;
    char            **papszCreateOptions = NULL;
    GUInt32         nXSize = 0, nYSize = 0;
    double          dfXMin = 0.0, dfXMax = 0.0, dfYMin = 0.0, dfYMax = 0.0;
    int             bIsXExtentSet = FALSE, bIsYExtentSet = FALSE;
    GDALGridAlgorithm eAlgorithm = GGA_InverseDistanceToAPower;
    void            *pOptions = NULL;
    char            *pszOutputSRS = NULL;
    int             bQuiet = FALSE;
    GDALProgressFunc pfnProgress = GDALTermProgress;
    int             i;
    OGRGeometryH    hSpatialFilter = NULL;

    /* Check that we are running against at least GDAL 1.5 */
    /* Note to developers : if we use newer API, please change the requirement */
    if (atoi(GDALVersionInfo("VERSION_NUM")) < 1500)
    {
#ifdef ZOO_SERVICE
        fprintf(stderr, "At least, GDAL >= 1.5.0 is required for this version of this ZOO ServiceProvider, "
                "which was compiled against GDAL %s\n", GDAL_RELEASE_NAME);
#else
        fprintf(stderr, "At least, GDAL >= 1.5.0 is required for this version of %s, "
                "which was compiled against GDAL %s\n", argv[0], GDAL_RELEASE_NAME);
#endif
	return SERVICE_FAILED;
    }

    GDALAllRegister();
    OGRRegisterAll();

#ifdef ZOO_SERVICE
    bQuiet = TRUE;
    pfnProgress = GDALDummyProgress;
    map* tmpMap=NULL;

    char dataPath[1024];
    tmpMap=getMapFromMaps(conf,"main","dataPath");
    if(tmpMap!=NULL)
      sprintf(dataPath,"%s",tmpMap->value);
    tmpMap=NULL;

    char tempPath[1024];
    tmpMap=getMapFromMaps(conf,"main","tmpPath");
    if(tmpMap!=NULL){
      sprintf(tempPath,"%s",tmpMap->value);
    }
    tmpMap=NULL;

    tmpMap=getMapFromMaps(inputs,"OF","value");
    if(tmpMap!=NULL){
      pszFormat=tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"OT","value");
    if(tmpMap!=NULL){
      int iType;      
      for( iType = 1; iType < GDT_TypeCount; iType++ )
	{
	  if( GDALGetDataTypeName((GDALDataType)iType) != NULL
	      && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
                             tmpMap->value) )
	    {
	      eOutputType = (GDALDataType) iType;
	    }
	}
      if( eOutputType == GDT_Unknown )
	{
	  fprintf( stderr, "Unknown output pixel type: %s\n", tmpMap->value );
	  Usage();
	  return SERVICE_FAILED;
	}
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"TXE","value");
    if(tmpMap!=NULL){
      char *tmp=tmpMap->value;
      char *t=strtok(tmp,",");
      int cnt=0;
      while(t!=NULL){
        switch(cnt){
        case 0:
          dfXMin = atof(t);
          break;
        case 1:
          dfXMax = atof(t);
          break;
        }
	t=strtok(NULL,",");
	cnt++;
      }
      bIsXExtentSet = TRUE;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"TYE","value");
    if(tmpMap!=NULL){
      char *tmp=tmpMap->value;
      char *t=strtok(tmp,",");
      int cnt=0;
      while(t!=NULL){
        switch(cnt){
        case 0:
          dfYMin = atof(t);
          break;
        case 1:
          dfYMax = atof(t);
          break;
        }
	t=strtok(NULL,",");
	cnt++;
	
      }
      bIsYExtentSet = TRUE;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"OUTSIZE","value");
    if(tmpMap!=NULL){
      char *tmp=tmpMap->value;
      char *t=strtok(tmp,",");
      int cnt=0;
      while(t!=NULL){
        switch(cnt){
        case 0:
          nXSize = atoi(t);
          break;
        case 1:
          nYSize = atoi(t);
          break;
        }
	cnt++;
      }
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"CO","value");
    if(tmpMap!=NULL){
      papszCreateOptions = CSLAddString( papszCreateOptions, tmpMap->value );
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"ZFIELD","value");
    if(tmpMap!=NULL){
      pszBurnAttribute = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"WHERE","value");
    if(tmpMap!=NULL){
      pszWHERE = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"L","value");
    if(tmpMap!=NULL){
      papszLayers = CSLAddString( papszLayers, tmpMap->value );
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"SQL","value");
    if(tmpMap!=NULL){
      pszSQL = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"A","value");
    if(tmpMap!=NULL){
      if ( ParseAlgorithmAndOptions(tmpMap->value, &eAlgorithm, &pOptions )
	   != CE_None )
	{
	  fprintf( stderr,
		   "Failed to process algoritm name and parameters.\n" );
	  return SERVICE_FAILED;
	}
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"SPAT","value");
    if(tmpMap!=NULL){
      char *tmp=tmpMap->value;
      char *t=strtok(tmp,",");
      int cnt=0;
      double dfULX, dfULY, dfLRX, dfLRY;
      while(t!=NULL){
        switch(cnt){
        case 0:
          dfULX = atof(t);
          break;
        case 1:
          dfULY = atof(t);
          break;
        case 2:
          dfLRX = atof(t);
          break;
        case 3:
          dfLRY = atof(t);
          break;
        }
        fprintf(stderr,"%s\n\n",t);
        fprintf(stderr,"%f - %f - %f - %f\n\n",dfULX,dfULY,dfLRX,dfLRY);
        t=strtok(NULL,",");
        cnt++;
      }
      OGRGeometryH hRing = OGR_G_CreateGeometry( wkbLinearRing );
      
      OGR_G_AddPoint_2D( hRing, dfULX, dfULY );
      OGR_G_AddPoint_2D( hRing, dfULX, dfLRY );
      OGR_G_AddPoint_2D( hRing, dfLRY, dfLRY );
      OGR_G_AddPoint_2D( hRing, dfLRY, dfULY );
      OGR_G_AddPoint_2D( hRing, dfULX, dfULY );
      
      hSpatialFilter = OGR_G_CreateGeometry( wkbPolygon );
      OGR_G_AddGeometry( hSpatialFilter, hRing );
	
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"A_SRS","value");
    if(tmpMap!=NULL){
      OGRSpatialReference oOutputSRS;
      
      if( oOutputSRS.SetFromUserInput( tmpMap->value ) != OGRERR_NONE )
	{
	  fprintf( stderr, "Failed to process SRS definition: %s\n", 
		   tmpMap->value );
	  GDALDestroyDriverManager();
	  return SERVICE_FAILED;
	}
      
      oOutputSRS.exportToWkt( &pszOutputSRS );
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"InputDSN","value");
    if(tmpMap!=NULL){
      pszSource=(char*)malloc(sizeof(char)*(strlen(dataPath)+strlen(tmpMap->value)+1));
      sprintf((char*)pszSource,"%s/%s",dataPath,tmpMap->value);
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"OutputDSN","value");
    if(tmpMap!=NULL){
      pszDest=(char*)malloc(sizeof(char)*(strlen(tempPath)+strlen(tmpMap->value)+4));
      char *ext=new char[4];
      ext="tif";
      if(strncasecmp(pszFormat,"AAIGRID",7)==0)
        ext="csv";
      else 
        if(strncasecmp(pszFormat,"PNG",3)==0)
          ext="png";
        else
          if(strncasecmp(pszFormat,"GIF",3)==0)
            ext="gif";
          else
            if(strncasecmp(pszFormat,"JPEG",4)==0)
              ext="jpg";
      sprintf((char*)pszDest,"%s/%s.%s",tempPath,tmpMap->value,ext);
    }

#else
    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 1 )
        exit( -argc );

/* -------------------------------------------------------------------- */
/*      Parse arguments.                                                */
/* -------------------------------------------------------------------- */
    for( i = 1; i < argc; i++ )
    {
        if( EQUAL(argv[i], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                   argv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        else if( EQUAL(argv[i],"-of") && i < argc-1 )
        {
            pszFormat = argv[++i];
        }

        else if( EQUAL(argv[i],"-quiet") )
        {
            bQuiet = TRUE;
            pfnProgress = GDALDummyProgress;
        }

        else if( EQUAL(argv[i],"-ot") && i < argc-1 )
        {
            int	iType;
            
            for( iType = 1; iType < GDT_TypeCount; iType++ )
            {
                if( GDALGetDataTypeName((GDALDataType)iType) != NULL
                    && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
                             argv[i+1]) )
                {
                    eOutputType = (GDALDataType) iType;
                }
            }

            if( eOutputType == GDT_Unknown )
            {
                fprintf( stderr, "Unknown output pixel type: %s\n", argv[i+1] );
                Usage();
                exit( 2 );
            }
            i++;
        }

        else if( EQUAL(argv[i],"-txe") && i < argc-2 )
        {
            dfXMin = atof(argv[++i]);
            dfXMax = atof(argv[++i]);
            bIsXExtentSet = TRUE;
        }   

        else if( EQUAL(argv[i],"-tye") && i < argc-2 )
        {
            dfYMin = atof(argv[++i]);
            dfYMax = atof(argv[++i]);
            bIsYExtentSet = TRUE;
        }   

        else if( EQUAL(argv[i],"-outsize") && i < argc-2 )
        {
            nXSize = atoi(argv[++i]);
            nYSize = atoi(argv[++i]);
        }   

        else if( EQUAL(argv[i],"-co") && i < argc-1 )
        {
            papszCreateOptions = CSLAddString( papszCreateOptions, argv[++i] );
        }   

        else if( EQUAL(argv[i],"-zfield") && i < argc-1 )
        {
            pszBurnAttribute = argv[++i];
        }

        else if( EQUAL(argv[i],"-where") && i < argc-1 )
        {
            pszWHERE = argv[++i];
        }

        else if( EQUAL(argv[i],"-l") && i < argc-1 )
        {
            papszLayers = CSLAddString( papszLayers, argv[++i] );
        }

        else if( EQUAL(argv[i],"-sql") && i < argc-1 )
        {
            pszSQL = argv[++i];
        }

        else if( EQUAL(argv[i],"-spat") 
                 && argv[i+1] != NULL 
                 && argv[i+2] != NULL 
                 && argv[i+3] != NULL 
                 && argv[i+4] != NULL )
        {
            OGRGeometryH hRing = OGR_G_CreateGeometry( wkbLinearRing );

            OGR_G_AddPoint_2D( hRing, atof(argv[i+1]), atof(argv[i+2]) );
            OGR_G_AddPoint_2D( hRing, atof(argv[i+1]), atof(argv[i+4]) );
            OGR_G_AddPoint_2D( hRing, atof(argv[i+3]), atof(argv[i+4]) );
            OGR_G_AddPoint_2D( hRing, atof(argv[i+3]), atof(argv[i+2]) );
            OGR_G_AddPoint_2D( hRing, atof(argv[i+1]), atof(argv[i+2]) );

            hSpatialFilter = OGR_G_CreateGeometry( wkbPolygon );
            OGR_G_AddGeometry( hSpatialFilter, hRing );
            i += 4;
        }

        else if( EQUAL(argv[i],"-a_srs") && i < argc-1 )
        {
            OGRSpatialReference oOutputSRS;

            if( oOutputSRS.SetFromUserInput( argv[i+1] ) != OGRERR_NONE )
            {
                fprintf( stderr, "Failed to process SRS definition: %s\n", 
                         argv[i+1] );
                GDALDestroyDriverManager();
                exit( 1 );
            }

            oOutputSRS.exportToWkt( &pszOutputSRS );
            i++;
        }   

        else if( EQUAL(argv[i],"-a") && i < argc-1 )
        {
            if ( ParseAlgorithmAndOptions( argv[++i], &eAlgorithm, &pOptions )
                 != CE_None )
            {
                fprintf( stderr,
                         "Failed to process algoritm name and parameters.\n" );
                exit( 1 );
            }
        }

        else if( argv[i][0] == '-' )
        {
            fprintf( stderr, "Option %s incomplete, or not recognised.\n\n", 
                    argv[i] );
            Usage();
            GDALDestroyDriverManager();
            exit( 2 );
        }

        else if( pszSource == NULL )
        {
            pszSource = argv[i];
        }

        else if( pszDest == NULL )
        {
            pszDest = argv[i];
        }

        else
        {
            fprintf( stderr, "Too many command options.\n\n" );
            Usage();
            GDALDestroyDriverManager();
            exit( 2 );
        }
    }
#endif

    if( pszSource == NULL || pszDest == NULL
        || (pszSQL == NULL && papszLayers == NULL) )
    {
        Usage();
        GDALDestroyDriverManager();
        exit( 2 );
    }

/* -------------------------------------------------------------------- */
/*      Find the output driver.                                         */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName( pszFormat );
    if( hDriver == NULL )
    {
        int	iDr;
        
        fprintf( stderr, "Output driver `%s' not recognised.\n", pszFormat );
        fprintf( stderr,
        "The following format drivers are configured and support output:\n" );
        for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
        {
            GDALDriverH hDriver = GDALGetDriver(iDr);

            if( GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) != NULL
                || GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATECOPY,
                                        NULL ) != NULL )
            {
                fprintf( stderr, "  %s: %s\n",
                         GDALGetDriverShortName( hDriver  ),
                         GDALGetDriverLongName( hDriver ) );
            }
        }
        printf( "\n" );
        Usage();
        
        GDALDestroyDriverManager();
#ifndef ZOO_SERVICE
        CSLDestroy( argv );
#endif
        CSLDestroy( papszCreateOptions );
	return SERVICE_FAILED;
    }

/* -------------------------------------------------------------------- */
/*      Open input datasource.                                          */
/* -------------------------------------------------------------------- */
    OGRDataSourceH hSrcDS;

    hSrcDS = OGROpen( pszSource, FALSE, NULL );
    if( hSrcDS == NULL )
    {
        fprintf( stderr, "Unable to open input datasource \"%s\".\n",
                 pszSource );
        fprintf( stderr, "%s\n", CPLGetLastErrorMsg() );
	return SERVICE_FAILED;
    }

/* -------------------------------------------------------------------- */
/*      Create target raster file.                                      */
/* -------------------------------------------------------------------- */
    GDALDatasetH    hDstDS;
    int             nLayerCount = CSLCount(papszLayers);
    int             nBands = nLayerCount;

    if ( pszSQL )
        nBands++;

    // FIXME
    if ( nXSize == 0 )
        nXSize = 256;
    if ( nYSize == 0 )
        nYSize = 256;

    hDstDS = GDALCreate( hDriver, pszDest, nXSize, nYSize, nBands,
                         eOutputType, papszCreateOptions );
    if ( hDstDS == NULL )
    {
        fprintf( stderr, "Unable to create target dataset \"%s\".\n",
                 pszDest );
        fprintf( stderr, "%s\n", CPLGetLastErrorMsg() );
	return SERVICE_FAILED;
    }

/* -------------------------------------------------------------------- */
/*      If algorithm was not specified assigh default one.              */
/* -------------------------------------------------------------------- */
    if ( !pOptions )
        ParseAlgorithmAndOptions( szAlgNameInvDist, &eAlgorithm, &pOptions );

/* -------------------------------------------------------------------- */
/*      Process SQL request.                                            */
/* -------------------------------------------------------------------- */
    if( pszSQL != NULL )
    {
        OGRLayerH hLayer;

        hLayer = OGR_DS_ExecuteSQL( hSrcDS, pszSQL, hSpatialFilter, NULL ); 
        if( hLayer != NULL )
        {
            // Custom layer will be rasterized in the first band.
            ProcessLayer( hLayer, hDstDS, nXSize, nYSize, 1,
                          bIsXExtentSet, bIsYExtentSet,
                          dfXMin, dfXMax, dfYMin, dfYMax, pszBurnAttribute,
                          eOutputType, eAlgorithm, pOptions,
                          bQuiet, pfnProgress );
        }
    }

/* -------------------------------------------------------------------- */
/*      Process each layer.                                             */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nLayerCount; i++ )
    {
        OGRLayerH hLayer = OGR_DS_GetLayerByName( hSrcDS, papszLayers[i] );
        if( hLayer == NULL )
        {
            fprintf( stderr, "Unable to find layer \"%s\", skipping.\n", 
                     papszLayers[i] );
            continue;
        }

        if( pszWHERE )
        {
            if( OGR_L_SetAttributeFilter( hLayer, pszWHERE ) != OGRERR_NONE )
                break;
        }

        if( hSpatialFilter != NULL )
          OGR_L_SetSpatialFilter( hLayer, hSpatialFilter );

        // Fetch the first meaningful SRS definition
        if ( !pszOutputSRS )
        {
            OGRSpatialReferenceH hSRS = OGR_L_GetSpatialRef( hLayer );
            if ( hSRS )
                OSRExportToWkt( hSRS, &pszOutputSRS );
        }

        ProcessLayer( hLayer, hDstDS, nXSize, nYSize,
                      i + 1 + nBands - nLayerCount,
                      bIsXExtentSet, bIsYExtentSet,
                      dfXMin, dfXMax, dfYMin, dfYMax, pszBurnAttribute,
                      eOutputType, eAlgorithm, pOptions,
                      bQuiet, pfnProgress );
    }

/* -------------------------------------------------------------------- */
/*      Apply geotransformation matrix.                                 */
/* -------------------------------------------------------------------- */
    double  adfGeoTransform[6];
    adfGeoTransform[0] = dfXMin;
    adfGeoTransform[1] = (dfXMax - dfXMin) / nXSize;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = dfYMin;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = (dfYMax - dfYMin) / nYSize;
    GDALSetGeoTransform( hDstDS, adfGeoTransform );

/* -------------------------------------------------------------------- */
/*      Apply SRS definition if set.                                    */
/* -------------------------------------------------------------------- */
    if ( pszOutputSRS )
    {
        GDALSetProjection( hDstDS, pszOutputSRS );
        CPLFree( pszOutputSRS );
    }

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    CSLDestroy( papszCreateOptions );
    CPLFree( pOptions );
    OGR_DS_Destroy( hSrcDS );
    GDALClose( hDstDS );
#ifndef ZOO_SERVICE
    CSLDestroy( argv );
#endif
    CSLDestroy( papszLayers );
    OGRCleanupAll();

    GDALDestroyDriverManager();
 
    outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="OutputedPolygon";
    outputs->content=createMap("value",(char*)pszDest);
    addMapToMap(&outputs->content,createMap("dataType","string"));
    outputs->next=NULL;
    return SERVICE_SUCCEEDED;
}


#ifdef ZOO_SERVICE
}
#endif
