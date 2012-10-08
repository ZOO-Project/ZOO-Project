/******************************************************************************
 * $Id: gdalwarp.cpp 24214 2012-04-08 20:17:17Z etourigny $
 *
 * Project:  High Performance Image Reprojector
 * Purpose:  Test program for high performance warper API.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2002, i3 - information integration and imaging 
 *                          Fort Collin, CO
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

#include "gdalwarper.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"
#include "ogr_api.h"
#include <vector>
#include "service.h"

CPL_CVSID("$Id: gdalwarp.cpp 24214 2012-04-08 20:17:17Z etourigny $");

extern "C"{
static void
LoadCutline( const char *pszCutlineDSName, const char *pszCLayer, 
             const char *pszCWHERE, const char *pszCSQL, 
             void **phCutlineRet );
static void
TransformCutlineToSource( GDALDatasetH hSrcDS, void *hCutline,
                          char ***ppapszWarpOptions, char **papszTO );

static GDALDatasetH 
GDALWarpCreateOutput( maps*& conf,char **papszSrcFiles, const char *pszFilename, 
                      const char *pszFormat, char **papszTO,
                      char ***ppapszCreateOptions, GDALDataType eDT,
                      void ** phTransformArg,
                      GDALDatasetH* phSrcDS );

static double	       dfMinX=0.0, dfMinY=0.0, dfMaxX=0.0, dfMaxY=0.0;
static double	       dfXRes=0.0, dfYRes=0.0;
static int             bTargetAlignedPixels = FALSE;
static int             nForcePixels=0, nForceLines=0, bQuiet = FALSE;
static int             bEnableDstAlpha = FALSE, bEnableSrcAlpha = FALSE;

static int             bVRT = FALSE;

/******************************************************************************/
/*! \page gdalwarp gdalwarp

image reprojection and warping utility

\section gdalwarp_synopsis SYNOPSIS

\htmlonly
Usage: 
\endhtmlonly

\verbatim
gdalwarp [--help-general] [--formats]
    [-s_srs srs_def] [-t_srs srs_def] [-to "NAME=VALUE"]
    [-order n | -tps | -rpc | -geoloc] [-et err_threshold]
    [-refine_gcps tolerance [minimum_gcps]]
    [-te xmin ymin xmax ymax] [-tr xres yres] [-tap] [-ts width height]
    [-wo "NAME=VALUE"] [-ot Byte/Int16/...] [-wt Byte/Int16]
    [-srcnodata "value [value...]"] [-dstnodata "value [value...]"] -dstalpha
    [-r resampling_method] [-wm memory_in_mb] [-multi] [-q]
    [-cutline datasource] [-cl layer] [-cwhere expression]
    [-csql statement] [-cblend dist_in_pixels] [-crop_to_cutline]
    [-of format] [-co "NAME=VALUE"]* [-overwrite]
    srcfile* dstfile
\endverbatim

\section gdalwarp_description DESCRIPTION

<p>
The gdalwarp utility is an image mosaicing, reprojection and warping
utility. The program can reproject to any supported projection,
and can also apply GCPs stored with the image if the image is "raw"
with control information.

<p>
<dl>
<dt> <b>-s_srs</b> <em>srs def</em>:</dt><dd> source spatial reference set.
The coordinate systems that can be passed are anything supported by the
OGRSpatialReference.SetFromUserInput() call, which includes EPSG PCS and GCSes
(ie. EPSG:4296), PROJ.4 declarations (as above), or the name of a .prf file
containing well known text.</dd>
<dt> <b>-t_srs</b> <em>srs_def</em>:</dt><dd> target spatial reference set.
The coordinate systems that can be passed are anything supported by the
OGRSpatialReference.SetFromUserInput() call, which includes EPSG PCS and GCSes
(ie. EPSG:4296), PROJ.4 declarations (as above), or the name of a .prf file
containing well known text.</dd>
<dt> <b>-to</b> <em>NAME=VALUE</em>:</dt><dd> set a transformer option suitable
to pass to GDALCreateGenImgProjTransformer2(). </dd>
<dt> <b>-order</b> <em>n</em>:</dt><dd> order of polynomial used for warping
(1 to 3). The default is to select a polynomial order based on the number of
GCPs.</dd>
<dt> <b>-tps</b>:</dt><dd>Force use of thin plate spline transformer based on
available GCPs.</dd>
<dt> <b>-rpc</b>:</dt> <dd>Force use of RPCs.</dd>
<dt> <b>-geoloc</b>:</dt><dd>Force use of Geolocation Arrays.</dd>
<dt> <b>-et</b> <em>err_threshold</em>:</dt><dd> error threshold for
transformation approximation (in pixel units - defaults to 0.125).</dd>
<dt> <b>-refine_gcps</b> <em>tolerance minimum_gcps</em>:</dt><dd>  (GDAL >= 1.9.0) refines the GCPs by automatically eliminating outliers.
Outliers will be eliminated until minimum_gcps are left or when no outliers can be detected.
The tolerance is passed to adjust when a GCP will be eliminated.
Not that GCP refinement only works with polynomial interpolation.
The tolerance is in pixel units if no projection is available, otherwise it is in SRS units.
If minimum_gcps is not provided, the minimum GCPs according to the polynomial model is used.</dd>
<dt> <b>-te</b> <em>xmin ymin xmax ymax</em>:</dt><dd> set georeferenced
extents of output file to be created (in target SRS).</dd>
<dt> <b>-tr</b> <em>xres yres</em>:</dt><dd> set output file resolution (in
target georeferenced units)</dd>
<dt> <b>-tap</b>:</dt><dd> (GDAL >= 1.8.0) (target aligned pixels) align
the coordinates of the extent of the output file to the values of the -tr,
such that the aligned extent includes the minimum extent.</dd>
<dt> <b>-ts</b> <em>width height</em>:</dt><dd> set output file size in
pixels and lines. If width or height is set to 0, the other dimension will be
guessed from the computed resolution. Note that -ts cannot be used with -tr</dd>
<dt> <b>-wo</b> <em>"NAME=VALUE"</em>:</dt><dd> Set a warp options.  The 
GDALWarpOptions::papszWarpOptions docs show all options.  Multiple
 <b>-wo</b> options may be listed.</dd>
<dt> <b>-ot</b> <em>type</em>:</dt><dd> For the output bands to be of the
indicated data type.</dd>
<dt> <b>-wt</b> <em>type</em>:</dt><dd> Working pixel data type. The data type
of pixels in the source image and destination image buffers.</dd>
<dt> <b>-srcnodata</b> <em>value [value...]</em>:</dt><dd> Set nodata masking
values for input bands (different values can be supplied for each band).  If 
more than one value is supplied all values should be quoted to keep them 
together as a single operating system argument.  Masked values will not be 
used in interpolation.  Use a value of <tt>None</tt> to ignore intrinsic nodata settings on the source dataset.</dd>
<dt> <b>-dstnodata</b> <em>value [value...]</em>:</dt><dd> Set nodata values
for output bands (different values can be supplied for each band).  If more
than one value is supplied all values should be quoted to keep them together
as a single operating system argument.  New files will be initialized to this
value and if possible the nodata value will be recorded in the output
file.</dd>
<dt> <b>-dstalpha</b>:</dt><dd> Create an output alpha band to identify 
nodata (unset/transparent) pixels. </dd>
<dt> <b>-wm</b> <em>memory_in_mb</em>:</dt><dd> Set the amount of memory (in
megabytes) that the warp API is allowed to use for caching.</dd>
<dt> <b>-multi</b>:</dt><dd> Use multithreaded warping implementation.
Multiple threads will be used to process chunks of image and perform
input/output operation simultaneously.</dd>
<dt> <b>-q</b>:</dt><dd> Be quiet.</dd>
<dt> <b>-of</b> <em>format</em>:</dt><dd> Select the output format. The default is GeoTIFF (GTiff). Use the short format name. </dd>
<dt> <b>-co</b> <em>"NAME=VALUE"</em>:</dt><dd> passes a creation option to
the output format driver. Multiple <b>-co</b> options may be listed. See
format specific documentation for legal creation options for each format.
</dd>

<dt> <b>-cutline</b> <em>datasource</em>:</dt><dd>Enable use of a blend cutline from the name OGR support datasource.</dd>
<dt> <b>-cl</b> <em>layername</em>:</dt><dd>Select the named layer from the 
cutline datasource.</dd>
<dt> <b>-cwhere</b> <em>expression</em>:</dt><dd>Restrict desired cutline features based on attribute query.</dd>
<dt> <b>-csql</b> <em>query</em>:</dt><dd>Select cutline features using an SQL query instead of from a layer with -cl.</dd>
<dt> <b>-cblend</b> <em>distance</em>:</dt><dd>Set a blend distance to use to blend over cutlines (in pixels).</dd>
<dt> <b>-crop_to_cutline</b>:</dt><dd>(GDAL >= 1.8.0) Crop the extent of the target dataset to the extent of the cutline.</dd>
<dt> <b>-overwrite</b>:</dt><dd>(GDAL >= 1.8.0) Overwrite the target dataset if it already exists.</dd>

<dt> <em>srcfile</em>:</dt><dd> The source file name(s). </dd>
<dt> <em>dstfile</em>:</dt><dd> The destination file name. </dd>
</dl>

Mosaicing into an existing output file is supported if the output file 
already exists. The spatial extent of the existing file will not
be modified to accomodate new data, so you may have to remove it in that case, or
use the -overwrite option.

Polygon cutlines may be used as a mask to restrict the area of the destination file
that may be updated, including blending.  If the OGR layer containing the cutline
features has no explicit SRS, the cutline features must be in the georeferenced
units of the destination file. When outputing to a not yet existing target dataset,
its extent will be the one of the original raster unless -te or -crop_to_cutline are
specified.

<p>
\section gdalwarp_example EXAMPLE

For instance, an eight bit spot scene stored in GeoTIFF with
control points mapping the corners to lat/long could be warped to a UTM
projection with a command like this:<p>

\verbatim
gdalwarp -t_srs '+proj=utm +zone=11 +datum=WGS84' raw_spot.tif utm11.tif
\endverbatim

For instance, the second channel of an ASTER image stored in HDF with
control points mapping the corners to lat/long could be warped to a UTM
projection with a command like this:<p>

\verbatim
gdalwarp HDF4_SDS:ASTER_L1B:"pg-PR1B0000-2002031402_100_001":2 pg-PR1B0000-2002031402_100_001_2.tif
\endverbatim

\if man
\section gdalwarp_author AUTHORS
Frank Warmerdam <warmerdam@pobox.com>, Silke Reimer <silke@intevation.de>
\endif
*/

/************************************************************************/
/*                               GDALExit()                             */
/*  This function exits and cleans up GDAL and OGR resources            */
/*  Perhaps it should be added to C api and used in all apps?           */
/************************************************************************/

static int GDALExit( int nCode )
{
  const char  *pszDebug = CPLGetConfigOption("CPL_DEBUG",NULL);
  if( pszDebug && (EQUAL(pszDebug,"ON") || EQUAL(pszDebug,"") ) )
  {  
    GDALDumpOpenDatasets( stderr );
    CPLDumpSharedList( NULL );
  }

  GDALDestroyDriverManager();

#ifdef OGR_ENABLED
  OGRCleanupAll();
#endif

  return SERVICE_FAILED;
}

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( 
        "Usage: gdalwarp [--help-general] [--formats]\n"
        "    [-s_srs srs_def] [-t_srs srs_def] [-to \"NAME=VALUE\"]\n"
        "    [-order n | -tps | -rpc | -geoloc] [-et err_threshold]\n"
        "    [-refine_gcps tolerance [minimum_gcps]]\n"
        "    [-te xmin ymin xmax ymax] [-tr xres yres] [-tap] [-ts width height]\n"
        "    [-wo \"NAME=VALUE\"] [-ot Byte/Int16/...] [-wt Byte/Int16]\n"
        "    [-srcnodata \"value [value...]\"] [-dstnodata \"value [value...]\"] -dstalpha\n" 
        "    [-r resampling_method] [-wm memory_in_mb] [-multi] [-q]\n"
        "    [-cutline datasource] [-cl layer] [-cwhere expression]\n"
        "    [-csql statement] [-cblend dist_in_pixels] [-crop_to_cutline]\n"
        "    [-of format] [-co \"NAME=VALUE\"]* [-overwrite]\n"
        "    srcfile* dstfile\n"
        "\n"
        "Available resampling methods:\n"
        "    near (default), bilinear, cubic, cubicspline, lanczos.\n" );
    GDALExit( 1 );
}

/************************************************************************/
/*                             SanitizeSRS                              */
/************************************************************************/

char *SanitizeSRS( const char *pszUserInput )

{
    OGRSpatialReferenceH hSRS;
    char *pszResult = NULL;

    CPLErrorReset();
    
    hSRS = OSRNewSpatialReference( NULL );
    if( OSRSetFromUserInput( hSRS, pszUserInput ) == OGRERR_NONE )
        OSRExportToWkt( hSRS, &pszResult );
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Translating source or target SRS failed:\n%s",
                  pszUserInput );
        GDALExit( 1 );
    }
    
    OSRDestroySpatialReference( hSRS );

    return pszResult;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

#ifdef WIN32
__declspec(dllexport)
#endif
int Gdal_Warp( maps*& conf,maps*& inputs,maps*& outputs )

{
    GDALDatasetH	hDstDS;
    const char         *pszFormat = "GTiff";
    int bFormatExplicitelySet = FALSE;
    char              **papszSrcFiles = NULL;
    char               *pszDstFilename = NULL;
    int                 bCreateOutput = FALSE, i;
    void               *hTransformArg, *hGenImgProjArg=NULL, *hApproxArg=NULL;
    char               **papszWarpOptions = NULL;
    double             dfErrorThreshold = 0.125;
    double             dfWarpMemoryLimit = 0.0;
    GDALTransformerFunc pfnTransformer = NULL;
    char                **papszCreateOptions = NULL;
    GDALDataType        eOutputType = GDT_Unknown, eWorkingType = GDT_Unknown; 
    GDALResampleAlg     eResampleAlg = GRA_NearestNeighbour;
    const char          *pszSrcNodata = NULL;
    const char          *pszDstNodata = NULL;
    int                 bMulti = FALSE;
    char                **papszTO = NULL;
    char                *pszCutlineDSName = NULL;
    char                *pszCLayer = NULL, *pszCWHERE = NULL, *pszCSQL = NULL;
    void                *hCutline = NULL;
    int                  bHasGotErr = FALSE;
    int                  bCropToCutline = FALSE;
    int                  bOverwrite = FALSE;


    /* Check that we are running against at least GDAL 1.6 */
    /* Note to developers : if we use newer API, please change the requirement */
    if (atoi(GDALVersionInfo("VERSION_NUM")) < 1600)
    {
        fprintf(stderr, "At least, GDAL >= 1.6.0 is required for this version of warp_service.zo, "
                "which was compiled against GDAL %s\n", GDAL_RELEASE_NAME);
        GDALExit(1);
    }

/* -------------------------------------------------------------------- */
/*      Register standard GDAL drivers, and process generic GDAL        */
/*      command options.                                                */
/* -------------------------------------------------------------------- */
    GDALAllRegister();

/* -------------------------------------------------------------------- */
/*      Parse arguments.                                                */
/* -------------------------------------------------------------------- */
    char *dataPath;
    map* tmpMap=getMapFromMaps(conf,"main","dataPath");
    if(tmpMap!=NULL)
      dataPath=strdup(tmpMap->value);
    tmpMap=NULL;

    char *tempPath;
    tmpMap=getMapFromMaps(conf,"main","tmpPath");
    if(tmpMap!=NULL){
      tempPath=strdup(tmpMap->value);
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"InputDSN","value");
    if(tmpMap!=NULL){
      map* length=getMapFromMaps(inputs,"InputDSN","length");
      if(length!=NULL){
	int i=0;
	int len=atoi(length->value);
	maps* currentMaps=getMaps(inputs,"InputDSN");
	for(i=0;i<len;i++){
	  tmpMap=getMapArray(currentMaps->content,"value",i);
	  papszSrcFiles[i]=(char*)CPLMalloc(sizeof(char)*(strlen(dataPath)+strlen(tmpMap->value)+4));
	  sprintf((char*)papszSrcFiles[i],"%s/%s",dataPath,tmpMap->value);
	}
      }else{
	char *tmp=(char*)CPLMalloc(sizeof(char)*(strlen(dataPath)+strlen(tmpMap->value)+4));
	sprintf(tmp,"%s/%s",dataPath,tmpMap->value);
	papszSrcFiles=CSLAddString( papszSrcFiles, tmp );
	free(tmp);
      }
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"OutputDSN","value");
    if(tmpMap!=NULL){
      pszDstFilename=(char*)CPLMalloc(sizeof(char)*(strlen(tempPath)+strlen(tmpMap->value)+4));
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
      sprintf(pszDstFilename,"%s/%s.%s",tempPath,tmpMap->value,ext);
    }


    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"r","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      if ( EQUAL(tmpMap->value, "near") )
	eResampleAlg = GRA_NearestNeighbour;
      else if ( EQUAL(tmpMap->value, "bilinear") )
	eResampleAlg = GRA_Bilinear;
      else if ( EQUAL(tmpMap->value, "cubic") )
	eResampleAlg = GRA_Cubic;
      else if ( EQUAL(tmpMap->value, "cubicspline") )
	eResampleAlg = GRA_CubicSpline;
      else if ( EQUAL(tmpMap->value, "lanczos") )
	eResampleAlg = GRA_Lanczos;
      else
	{
	  char tmp[1024];
	  sprintf(tmp,"Unknown resampling method: \"%s\".", tmpMap->value );
	  setMapInMaps(conf,"lenv","message",tmp);
	  return SERVICE_FAILED;
	}
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"t_srs","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      char *pszSRS = SanitizeSRS(tmpMap->value);
      papszTO = CSLSetNameValue( papszTO, "DST_SRS", pszSRS );
      CPLFree( pszSRS );
    }

    tmpMap=getMapFromMaps(inputs,"s_srs","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      char *pszSRS = SanitizeSRS(tmpMap->value);
      papszTO = CSLSetNameValue( papszTO, "SRC_SRS", pszSRS );
      CPLFree( pszSRS );
    }

    tmpMap=getMapFromMaps(inputs,"order","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      const char* pszMethod = CSLFetchNameValue(papszTO, "METHOD");
      if (pszMethod)
	fprintf(stderr, "Warning: only one METHOD can be used. Method %s is already defined\n",
		pszMethod);
      papszTO = CSLSetNameValue( papszTO, "MAX_GCP_ORDER", tmpMap->value );
    }
    
    tmpMap=getMapFromMaps(inputs,"co","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      papszCreateOptions = CSLAddString( papszCreateOptions, tmpMap->value );
      bCreateOutput = TRUE;
    }   

    tmpMap=getMapFromMaps(inputs,"wo","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      papszWarpOptions = CSLAddString( papszWarpOptions, tmpMap->value );
    }   

    tmpMap=getMapFromMaps(inputs,"multi","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      bMulti = TRUE;
    }   

    tmpMap=getMapFromMaps(inputs,"dstalpha","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      bEnableDstAlpha = TRUE;
    }

    tmpMap=getMapFromMaps(inputs,"srcalpha","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      bEnableSrcAlpha = TRUE;
    }

    tmpMap=getMapFromMaps(inputs,"tap","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      bTargetAlignedPixels = TRUE;
    }


    tmpMap=getMapFromMaps(inputs,"crop_to_cutline","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      bCropToCutline = TRUE;
      bCreateOutput = TRUE;
    }


    tmpMap=getMapFromMaps(inputs,"tps","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      papszTO = CSLSetNameValue( papszTO, "METHOD", "GCP_TPS" );
    }

    tmpMap=getMapFromMaps(inputs,"rpc","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      papszTO = CSLSetNameValue( papszTO, "METHOD", "RPC" );
    }

    tmpMap=getMapFromMaps(inputs,"geoloc","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"TRUE",4)==0 ){
      papszTO = CSLSetNameValue( papszTO, "METHOD", "GEOLOC_ARRAY" );
    }

    tmpMap=getMapFromMaps(inputs,"wt","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      int	iType;
      
      for( iType = 1; iType < GDT_TypeCount; iType++ )
	{
	  if( GDALGetDataTypeName((GDALDataType)iType) != NULL
	      && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
		       tmpMap->value) )
	    {
	      eWorkingType = (GDALDataType) iType;
	    }
	}
      
      if( eWorkingType == GDT_Unknown )
	{
	  char tmp[1024];
	  sprintf(tmp,"Unknown output pixel type: %s\n", tmpMap->value );
	  setMapInMaps(conf,"lenv","message",tmp);
	  return SERVICE_FAILED;
	}
    }

    tmpMap=getMapFromMaps(inputs,"srcnodata","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      pszSrcNodata = strdup(tmpMap->value);
    }

    tmpMap=getMapFromMaps(inputs,"dstnodata","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      pszDstNodata = strdup(tmpMap->value);
    }


    tmpMap=getMapFromMaps(inputs,"tr","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      char *tmpV=strdup(tmpMap->value);
      char *res=strtok(tmpV,",");
      int i=0;
      while(res!=NULL){
	if(i==0)
	  dfXRes = CPLAtofM(res);
	else
	  dfYRes = fabs(CPLAtofM(res));
	res=strtok(NULL,",");
	i++;
      }
      if( dfXRes == 0 || dfYRes == 0 )
	{
	  setMapInMaps(conf,"lenv","message","Wrong value for TR parameters");
	}
      bCreateOutput = TRUE;
    }

    tmpMap=getMapFromMaps(inputs,"cblend","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      papszWarpOptions = 
	CSLSetNameValue( papszWarpOptions, 
			 "CUTLINE_BLEND_DIST", tmpMap->value );
    }
    
    tmpMap=getMapFromMaps(inputs,"ot","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      int	iType;
      
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
	  char tmp[128];
	  sprintf( tmp,"Unknown output pixel type: %s\n", tmpMap->value );
	  setMapInMaps(conf,"lenv","message",tmp);
	  return SERVICE_FAILED;
	}
      bCreateOutput = TRUE;      
    }


    tmpMap=getMapFromMaps(inputs,"ts","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      char *tmpV=strdup(tmpMap->value);
      char *res=strtok(tmpV,",");
      int i=0;
      while(res!=NULL){
	if(i==0)
	  nForcePixels = atoi(res);	  
	else
	  nForceLines = atoi(res);	  
	res=strtok(NULL,",");
	i++;
      }
      free(tmpV);
      bCreateOutput = TRUE;      
    }

    tmpMap=getMapFromMaps(inputs,"te","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      char *tmpV=strdup(tmpMap->value);
      char *res=strtok(tmpV,",");
      int i=0;
      while(res!=NULL){
	switch(i){
	case 0:
	  dfMinX = CPLAtofM(res);
	  break;
	case 1:
	  dfMinY = CPLAtofM(res);
	  break;
	case 2:
	  dfMaxX = CPLAtofM(res);
	  break;
	case 3:
	  dfMaxY = CPLAtofM(res);
	  break;
	}
	if(i==0)
	  nForcePixels = atoi(res);	  
	else
	  nForceLines = atoi(res);	  
	res=strtok(NULL,",");
	i++;
      }
      free(tmpV);
      bCreateOutput = TRUE;
    }

    tmpMap=getMapFromMaps(inputs,"te","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      pszCutlineDSName = strdup(tmpMap->value);
    }

    tmpMap=getMapFromMaps(inputs,"cwhere","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      pszCWHERE = strdup(tmpMap->value);
    }

    tmpMap=getMapFromMaps(inputs,"cl","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      pszCLayer = strdup(tmpMap->value);
    }

    tmpMap=getMapFromMaps(inputs,"csql","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      pszCSQL = strdup(tmpMap->value);
    }
    
    tmpMap=getMapFromMaps(inputs,"of","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      pszFormat = strdup(tmpMap->value);
      bFormatExplicitelySet = TRUE;
      bCreateOutput = TRUE;
      if( EQUAL(pszFormat,"VRT") )
	bVRT = TRUE;
    }

    tmpMap=getMapFromMaps(inputs,"to","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      papszTO = CSLAddString( papszTO, tmpMap->value );
    }

    tmpMap=getMapFromMaps(inputs,"et","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      dfErrorThreshold = CPLAtofM(tmpMap->value);
    }
    

    tmpMap=getMapFromMaps(inputs,"refine_gpcs","value");
    if(tmpMap!=NULL && strncasecmp(tmpMap->value,"NULL",4)!=0 ){
      char *tmpV=strdup(tmpMap->value);
      char *res=strtok(tmpV,",");
      int i=0;
      while(res!=NULL){
	if(i==0){
	  papszTO = CSLSetNameValue( papszTO, "REFINE_TOLERANCE", res );
	  if(atof(res) < 0){
	    setMapInMaps(conf,"lenv","message","The tolerance for -refine_gcps may not be negative\n");
	    return SERVICE_FAILED;
	  }
	}else if(i==1){
	  if(atoi(res) >= 0 && isdigit(res[0]))
	    papszTO = CSLSetNameValue( papszTO, "REFINE_MINIMUM_GCPS", res );
	  else
	    papszTO = CSLSetNameValue( papszTO, "REFINE_MINIMUM_GCPS", "-1" );
	}
	res=strtok(NULL,",");
	i++;
      }
    }

    bQuiet = TRUE;
    
/* -------------------------------------------------------------------- */
/*      Check that incompatible options are not used                    */
/* -------------------------------------------------------------------- */

    if ((nForcePixels != 0 || nForceLines != 0) && 
        (dfXRes != 0 && dfYRes != 0))
    {
      setMapInMaps(conf,"lenv","message","tr and ts options cannot be used at the same time\n");
      return SERVICE_FAILED;
    }
    
    if (bTargetAlignedPixels && dfXRes == 0 && dfYRes == 0)
    {
      setMapInMaps(conf,"lenv","message","tap option cannot be used without using tr");
      return SERVICE_FAILED;
    }


/* -------------------------------------------------------------------- */
/*      Does the output dataset already exist?                          */
/* -------------------------------------------------------------------- */

    /* FIXME ? source filename=target filename and -overwrite is definitely */
    /* an error. But I can't imagine of a valid case (without -overwrite), */
    /* where it would make sense. In doubt, let's keep that dubious possibility... */
    if ( CSLCount(papszSrcFiles) == 1 &&
         strcmp(papszSrcFiles[0], pszDstFilename) == 0 && bOverwrite)
    {
      setMapInMaps(conf,"lenv","message","Source and destination datasets must be different.");
      return GDALExit(1);
    }

    CPLPushErrorHandler( CPLQuietErrorHandler );
    hDstDS = GDALOpen( pszDstFilename, GA_Update );
    CPLPopErrorHandler();

    if( hDstDS != NULL && bOverwrite )
    {
        GDALClose(hDstDS);
        hDstDS = NULL;
    }

    if( hDstDS != NULL && bCreateOutput )
    {
      char tmp[024];
      sprintf( tmp, 
	       "Output dataset %s exists,\n"
	       "but some commandline options were provided indicating a new dataset\n"
	       "should be created.  Please delete existing dataset and run again.\n",
	       pszDstFilename );
      setMapInMaps(conf,"lenv","message",tmp);
      return GDALExit(1);
    }

    /* Avoid overwriting an existing destination file that cannot be opened in */
    /* update mode with a new GTiff file */
    if ( hDstDS == NULL && !bOverwrite )
    {
        CPLPushErrorHandler( CPLQuietErrorHandler );
        hDstDS = GDALOpen( pszDstFilename, GA_ReadOnly );
        CPLPopErrorHandler();
        
        if (hDstDS)
        {
	  char tmp[1024];
	  sprintf( tmp, 
		   "Output dataset %s exists, but cannot be opened in update mode\n",
		   pszDstFilename );
	  GDALClose(hDstDS);
	  setMapInMaps(conf,"lenv","message",tmp);
	  return GDALExit( 1 );
        }
    }

/* -------------------------------------------------------------------- */
/*      If we have a cutline datasource read it and attach it in the    */
/*      warp options.                                                   */
/* -------------------------------------------------------------------- */
    if( pszCutlineDSName != NULL )
    {
        LoadCutline( pszCutlineDSName, pszCLayer, pszCWHERE, pszCSQL,
                     &hCutline );
    }

#ifdef OGR_ENABLED
    if ( bCropToCutline && hCutline != NULL )
    {
      
      
        OGRGeometryH hCutlineGeom = OGR_G_Clone( (OGRGeometryH) hCutline );
        OGRSpatialReferenceH hCutlineSRS = OGR_G_GetSpatialReference( hCutlineGeom );
        const char *pszThisTargetSRS = CSLFetchNameValue( papszTO, "DST_SRS" );
        OGRCoordinateTransformationH hCT = NULL;
        if (hCutlineSRS == NULL)
        {
            /* We suppose it is in target coordinates */
        }
        else if (pszThisTargetSRS != NULL)
        {
            OGRSpatialReferenceH hTargetSRS = OSRNewSpatialReference(NULL);
            if( OSRImportFromWkt( hTargetSRS, (char **)&pszThisTargetSRS ) != CE_None )
            {
	      setMapInMaps(conf,"lenv","message","Cannot compute bounding box of cutline.\n");
                return GDALExit(1);
            }

            hCT = OCTNewCoordinateTransformation(hCutlineSRS, hTargetSRS);

            OSRDestroySpatialReference(hTargetSRS);
        }
        else if (pszThisTargetSRS == NULL)
        {
            if (papszSrcFiles[0] != NULL)
            {
                GDALDatasetH hSrcDS = GDALOpen(papszSrcFiles[0], GA_ReadOnly);
                if (hSrcDS == NULL)
                {
		  setMapInMaps(conf,"lenv","message","Cannot compute bounding box of cutline.");
		  return GDALExit(1);
                }

                OGRSpatialReferenceH  hRasterSRS = NULL;
                const char *pszProjection = NULL;

                if( GDALGetProjectionRef( hSrcDS ) != NULL
                    && strlen(GDALGetProjectionRef( hSrcDS )) > 0 )
                    pszProjection = GDALGetProjectionRef( hSrcDS );
                else if( GDALGetGCPProjection( hSrcDS ) != NULL )
                    pszProjection = GDALGetGCPProjection( hSrcDS );

                if( pszProjection == NULL )
                {
		  setMapInMaps(conf,"lenv","message","Cannot compute bounding box of cutline.");
		  return GDALExit(1);
                }

                hRasterSRS = OSRNewSpatialReference(NULL);
                if( OSRImportFromWkt( hRasterSRS, (char **)&pszProjection ) != CE_None )
                {
		  setMapInMaps(conf,"lenv","message","Cannot compute bounding box of cutline.");
		  return GDALExit(1);
                }

                hCT = OCTNewCoordinateTransformation(hCutlineSRS, hRasterSRS);

                OSRDestroySpatialReference(hRasterSRS);

                GDALClose(hSrcDS);
            }
            else
            {
	      setMapInMaps(conf,"levn","message", "Cannot compute bounding box of cutline.\n");
	      return GDALExit(1);
            }
        }

        if (hCT)
        {
            OGR_G_Transform( hCutlineGeom, hCT );

            OCTDestroyCoordinateTransformation(hCT);
        }

        OGREnvelope sEnvelope;
        OGR_G_GetEnvelope(hCutlineGeom, &sEnvelope);

        dfMinX = sEnvelope.MinX;
        dfMinY = sEnvelope.MinY;
        dfMaxX = sEnvelope.MaxX;
        dfMaxY = sEnvelope.MaxY;
        
        OGR_G_DestroyGeometry(hCutlineGeom);
    }
#endif
    
/* -------------------------------------------------------------------- */
/*      If not, we need to create it.                                   */
/* -------------------------------------------------------------------- */
    int   bInitDestSetForFirst = FALSE;

    void* hUniqueTransformArg = NULL;
    GDALDatasetH hUniqueSrcDS = NULL;

    if( hDstDS == NULL )
    {

      hDstDS = GDALWarpCreateOutput( conf,papszSrcFiles, pszDstFilename,pszFormat,
                                       papszTO, &papszCreateOptions, 
                                       eOutputType, &hUniqueTransformArg,
                                       &hUniqueSrcDS);
        bCreateOutput = TRUE;


        if( CSLFetchNameValue( papszWarpOptions, "INIT_DEST" ) == NULL 
            && pszDstNodata == NULL )
        {
            papszWarpOptions = CSLSetNameValue(papszWarpOptions,
                                               "INIT_DEST", "0");
            bInitDestSetForFirst = TRUE;
        }
        else if( CSLFetchNameValue( papszWarpOptions, "INIT_DEST" ) == NULL )
        {
            papszWarpOptions = CSLSetNameValue(papszWarpOptions,
                                               "INIT_DEST", "NO_DATA" );
            bInitDestSetForFirst = TRUE;
        }

        CSLDestroy( papszCreateOptions );
        papszCreateOptions = NULL;
    }

 
    if( hDstDS == NULL )
        GDALExit( 1 );

/* -------------------------------------------------------------------- */
/*      Loop over all source files, processing each in turn.            */
/* -------------------------------------------------------------------- */
    int iSrc;

    for( iSrc = 0; papszSrcFiles[iSrc] != NULL; iSrc++ )
    {
        GDALDatasetH hSrcDS;
       
/* -------------------------------------------------------------------- */
/*      Open this file.                                                 */
/* -------------------------------------------------------------------- */
        if (hUniqueSrcDS)
            hSrcDS = hUniqueSrcDS;
        else
            hSrcDS = GDALOpen( papszSrcFiles[iSrc], GA_ReadOnly );
    
        if( hSrcDS == NULL ){
	  setMapInMaps(conf,"lenv","message",CPLGetLastErrorMsg());
	  return GDALExit( 2 );
	}

/* -------------------------------------------------------------------- */
/*      Check that there's at least one raster band                     */
/* -------------------------------------------------------------------- */
        if ( GDALGetRasterCount(hSrcDS) == 0 )
        {    
	  char tmp[1024];
	  sprintf(tmp, "Input file %s has no raster bands.\n", papszSrcFiles[iSrc] );
	  setMapInMaps(conf,"lenv","message",tmp);
	  return GDALExit( 1 );
        }
	
        if( !bQuiet )
            printf( "Processing input file %s.\n", papszSrcFiles[iSrc] );

/* -------------------------------------------------------------------- */
/*      Warns if the file has a color table and something more          */
/*      complicated than nearest neighbour resampling is asked          */
/* -------------------------------------------------------------------- */
 
	if ( eResampleAlg != GRA_NearestNeighbour &&
             GDALGetRasterColorTable(GDALGetRasterBand(hSrcDS, 1)) != NULL)
        {
                fprintf( stderr, "Warning: Input file %s has a color table, which will likely lead to "
                        "bad results when using a resampling method other than "
                        "nearest neighbour. Converting the dataset prior to 24/32 bit "
                        "is advised.\n", papszSrcFiles[iSrc] );
        }

/* -------------------------------------------------------------------- */
/*      Do we have a source alpha band?                                 */
/* -------------------------------------------------------------------- */
        if( GDALGetRasterColorInterpretation( 
                GDALGetRasterBand(hSrcDS,GDALGetRasterCount(hSrcDS)) ) 
            == GCI_AlphaBand 
            && !bEnableSrcAlpha )
        {
            bEnableSrcAlpha = TRUE;
            if( !bQuiet )
                printf( "Using band %d of source image as alpha.\n", 
                        GDALGetRasterCount(hSrcDS) );
        }

/* -------------------------------------------------------------------- */
/*      Create a transformation object from the source to               */
/*      destination coordinate system.                                  */
/* -------------------------------------------------------------------- */
        if (hUniqueTransformArg)
            hTransformArg = hGenImgProjArg = hUniqueTransformArg;
        else
            hTransformArg = hGenImgProjArg =
                GDALCreateGenImgProjTransformer2( hSrcDS, hDstDS, papszTO );

        if( hTransformArg == NULL ){
	  setMapInMaps(conf,"lenv","message",CPLGetLastErrorMsg());
	  return GDALExit( 1 );
	}
        
        pfnTransformer = GDALGenImgProjTransform;

/* -------------------------------------------------------------------- */
/*      Warp the transformer with a linear approximator unless the      */
/*      acceptable error is zero.                                       */
/* -------------------------------------------------------------------- */
        if( dfErrorThreshold != 0.0 )
        {
            hTransformArg = hApproxArg = 
                GDALCreateApproxTransformer( GDALGenImgProjTransform, 
                                             hGenImgProjArg, dfErrorThreshold);
            pfnTransformer = GDALApproxTransform;
        }

/* -------------------------------------------------------------------- */
/*      Clear temporary INIT_DEST settings after the first image.       */
/* -------------------------------------------------------------------- */
        if( bInitDestSetForFirst && iSrc == 1 )
            papszWarpOptions = CSLSetNameValue( papszWarpOptions, 
                                                "INIT_DEST", NULL );

/* -------------------------------------------------------------------- */
/*      Setup warp options.                                             */
/* -------------------------------------------------------------------- */
        GDALWarpOptions *psWO = GDALCreateWarpOptions();

        psWO->papszWarpOptions = CSLDuplicate(papszWarpOptions);
        psWO->eWorkingDataType = eWorkingType;
        psWO->eResampleAlg = eResampleAlg;

        psWO->hSrcDS = hSrcDS;
        psWO->hDstDS = hDstDS;

        psWO->pfnTransformer = pfnTransformer;
        psWO->pTransformerArg = hTransformArg;

        if( !bQuiet )
            psWO->pfnProgress = GDALTermProgress;

        if( dfWarpMemoryLimit != 0.0 )
            psWO->dfWarpMemoryLimit = dfWarpMemoryLimit;

/* -------------------------------------------------------------------- */
/*      Setup band mapping.                                             */
/* -------------------------------------------------------------------- */
        if( bEnableSrcAlpha )
            psWO->nBandCount = GDALGetRasterCount(hSrcDS) - 1;
        else
            psWO->nBandCount = GDALGetRasterCount(hSrcDS);

        psWO->panSrcBands = (int *) CPLMalloc(psWO->nBandCount*sizeof(int));
        psWO->panDstBands = (int *) CPLMalloc(psWO->nBandCount*sizeof(int));

        for( i = 0; i < psWO->nBandCount; i++ )
        {
            psWO->panSrcBands[i] = i+1;
            psWO->panDstBands[i] = i+1;
        }

/* -------------------------------------------------------------------- */
/*      Setup alpha bands used if any.                                  */
/* -------------------------------------------------------------------- */
        if( bEnableSrcAlpha )
            psWO->nSrcAlphaBand = GDALGetRasterCount(hSrcDS);

        if( !bEnableDstAlpha 
            && GDALGetRasterCount(hDstDS) == psWO->nBandCount+1 
            && GDALGetRasterColorInterpretation( 
                GDALGetRasterBand(hDstDS,GDALGetRasterCount(hDstDS))) 
            == GCI_AlphaBand )
        {
            if( !bQuiet )
                printf( "Using band %d of destination image as alpha.\n", 
                        GDALGetRasterCount(hDstDS) );
                
            bEnableDstAlpha = TRUE;
        }

        if( bEnableDstAlpha )
            psWO->nDstAlphaBand = GDALGetRasterCount(hDstDS);

/* -------------------------------------------------------------------- */
/*      Setup NODATA options.                                           */
/* -------------------------------------------------------------------- */
        if( pszSrcNodata != NULL && !EQUALN(pszSrcNodata,"n",1) )
        {
            char **papszTokens = CSLTokenizeString( pszSrcNodata );
            int  nTokenCount = CSLCount(papszTokens);

            psWO->padfSrcNoDataReal = (double *) 
                CPLMalloc(psWO->nBandCount*sizeof(double));
            psWO->padfSrcNoDataImag = (double *) 
                CPLMalloc(psWO->nBandCount*sizeof(double));

            for( i = 0; i < psWO->nBandCount; i++ )
            {
                if( i < nTokenCount )
                {
                    CPLStringToComplex( papszTokens[i], 
                                        psWO->padfSrcNoDataReal + i,
                                        psWO->padfSrcNoDataImag + i );
                }
                else
                {
                    psWO->padfSrcNoDataReal[i] = psWO->padfSrcNoDataReal[i-1];
                    psWO->padfSrcNoDataImag[i] = psWO->padfSrcNoDataImag[i-1];
                }
            }

            CSLDestroy( papszTokens );

            psWO->papszWarpOptions = CSLSetNameValue(psWO->papszWarpOptions,
                                               "UNIFIED_SRC_NODATA", "YES" );
        }

/* -------------------------------------------------------------------- */
/*      If -srcnodata was not specified, but the data has nodata        */
/*      values, use them.                                               */
/* -------------------------------------------------------------------- */
        if( pszSrcNodata == NULL )
        {
            int bHaveNodata = FALSE;
            double dfReal = 0.0;

            for( i = 0; !bHaveNodata && i < psWO->nBandCount; i++ )
            {
                GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, i+1 );
                dfReal = GDALGetRasterNoDataValue( hBand, &bHaveNodata );
            }

            if( bHaveNodata )
            {
                if( !bQuiet )
                {
                    if (CPLIsNan(dfReal))
                        printf( "Using internal nodata values (eg. nan) for image %s.\n",
                                papszSrcFiles[iSrc] );
                    else
                        printf( "Using internal nodata values (eg. %g) for image %s.\n",
                                dfReal, papszSrcFiles[iSrc] );
                }
                psWO->padfSrcNoDataReal = (double *) 
                    CPLMalloc(psWO->nBandCount*sizeof(double));
                psWO->padfSrcNoDataImag = (double *) 
                    CPLMalloc(psWO->nBandCount*sizeof(double));
                
                for( i = 0; i < psWO->nBandCount; i++ )
                {
                    GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, i+1 );

                    dfReal = GDALGetRasterNoDataValue( hBand, &bHaveNodata );

                    if( bHaveNodata )
                    {
                        psWO->padfSrcNoDataReal[i] = dfReal;
                        psWO->padfSrcNoDataImag[i] = 0.0;
                    }
                    else
                    {
                        psWO->padfSrcNoDataReal[i] = -123456.789;
                        psWO->padfSrcNoDataImag[i] = 0.0;
                    }
                }
            }
        }

/* -------------------------------------------------------------------- */
/*      If the output dataset was created, and we have a destination    */
/*      nodata value, go through marking the bands with the information.*/
/* -------------------------------------------------------------------- */
        if( pszDstNodata != NULL )
        {
            char **papszTokens = CSLTokenizeString( pszDstNodata );
            int  nTokenCount = CSLCount(papszTokens);

            psWO->padfDstNoDataReal = (double *) 
                CPLMalloc(psWO->nBandCount*sizeof(double));
            psWO->padfDstNoDataImag = (double *) 
                CPLMalloc(psWO->nBandCount*sizeof(double));

            for( i = 0; i < psWO->nBandCount; i++ )
            {
                if( i < nTokenCount )
                {
                    CPLStringToComplex( papszTokens[i], 
                                        psWO->padfDstNoDataReal + i,
                                        psWO->padfDstNoDataImag + i );
                }
                else
                {
                    psWO->padfDstNoDataReal[i] = psWO->padfDstNoDataReal[i-1];
                    psWO->padfDstNoDataImag[i] = psWO->padfDstNoDataImag[i-1];
                }
                
                GDALRasterBandH hBand = GDALGetRasterBand( hDstDS, i+1 );
                int bClamped = FALSE, bRounded = FALSE;

#define CLAMP(val,type,minval,maxval) \
    do { if (val < minval) { bClamped = TRUE; val = minval; } \
    else if (val > maxval) { bClamped = TRUE; val = maxval; } \
    else if (val != (type)val) { bRounded = TRUE; val = (type)(val + 0.5); } } \
    while(0)

                switch(GDALGetRasterDataType(hBand))
                {
                    case GDT_Byte:
                        CLAMP(psWO->padfDstNoDataReal[i], GByte,
                              0.0, 255.0);
                        break;
                    case GDT_Int16:
                        CLAMP(psWO->padfDstNoDataReal[i], GInt16,
                              -32768.0, 32767.0);
                        break;
                    case GDT_UInt16:
                        CLAMP(psWO->padfDstNoDataReal[i], GUInt16,
                              0.0, 65535.0);
                        break;
                    case GDT_Int32:
                        CLAMP(psWO->padfDstNoDataReal[i], GInt32,
                              -2147483648.0, 2147483647.0);
                        break;
                    case GDT_UInt32:
                        CLAMP(psWO->padfDstNoDataReal[i], GUInt32,
                              0.0, 4294967295.0);
                        break;
                    default:
                        break;
                }
                    
                if (bClamped)
                {
                    printf( "for band %d, destination nodata value has been clamped "
                           "to %.0f, the original value being out of range.\n",
                           i + 1, psWO->padfDstNoDataReal[i]);
                }
                else if(bRounded)
                {
                    printf("for band %d, destination nodata value has been rounded "
                           "to %.0f, %s being an integer datatype.\n",
                           i + 1, psWO->padfDstNoDataReal[i],
                           GDALGetDataTypeName(GDALGetRasterDataType(hBand)));
                }

                if( bCreateOutput )
                {
                    GDALSetRasterNoDataValue( 
                        GDALGetRasterBand( hDstDS, psWO->panDstBands[i] ), 
                        psWO->padfDstNoDataReal[i] );
                }
            }

            CSLDestroy( papszTokens );
        }

/* -------------------------------------------------------------------- */
/*      If we have a cutline, transform it into the source              */
/*      pixel/line coordinate system and insert into warp options.      */
/* -------------------------------------------------------------------- */
        if( hCutline != NULL )
        {
            TransformCutlineToSource( hSrcDS, hCutline, 
                                      &(psWO->papszWarpOptions), 
                                      papszTO );
        }

/* -------------------------------------------------------------------- */
/*      If we are producing VRT output, then just initialize it with    */
/*      the warp options and write out now rather than proceeding       */
/*      with the operations.                                            */
/* -------------------------------------------------------------------- */
        if( bVRT )
        {
            if( GDALInitializeWarpedVRT( hDstDS, psWO ) != CE_None )
                GDALExit( 1 );

            GDALClose( hDstDS );
            GDALClose( hSrcDS );

            /* The warped VRT will clean itself the transformer used */
            /* So we have only to destroy the hGenImgProjArg if we */
            /* have wrapped it inside the hApproxArg */
            if (pfnTransformer == GDALApproxTransform)
            {
                if( hGenImgProjArg != NULL )
                    GDALDestroyGenImgProjTransformer( hGenImgProjArg );
            }

            GDALDestroyWarpOptions( psWO );

	    setMapInMaps(outputs,"Result","value",pszDstFilename);

            CPLFree( pszDstFilename );
            CSLDestroy( papszSrcFiles );
            CSLDestroy( papszWarpOptions );
            CSLDestroy( papszTO );
    
            GDALDumpOpenDatasets( stderr );
        
            GDALDestroyDriverManager();
        
            return SERVICE_SUCCEEDED;
        }

/* -------------------------------------------------------------------- */
/*      Initialize and execute the warp.                                */
/* -------------------------------------------------------------------- */
        GDALWarpOperation oWO;

        if( oWO.Initialize( psWO ) == CE_None )
        {
            CPLErr eErr;
            if( bMulti )
                eErr = oWO.ChunkAndWarpMulti( 0, 0, 
                                       GDALGetRasterXSize( hDstDS ),
                                       GDALGetRasterYSize( hDstDS ) );
            else
                eErr = oWO.ChunkAndWarpImage( 0, 0, 
                                       GDALGetRasterXSize( hDstDS ),
                                       GDALGetRasterYSize( hDstDS ) );
            if (eErr != CE_None){
                bHasGotErr = TRUE;
	    }
        }

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
        if( hApproxArg != NULL )
            GDALDestroyApproxTransformer( hApproxArg );
        
        if( hGenImgProjArg != NULL )
            GDALDestroyGenImgProjTransformer( hGenImgProjArg );
        
        GDALDestroyWarpOptions( psWO );

        GDALClose( hSrcDS );
    }

/* -------------------------------------------------------------------- */
/*      Final Cleanup.                                                  */
/* -------------------------------------------------------------------- */
    CPLErrorReset();
    GDALFlushCache( hDstDS );
    if( CPLGetLastErrorType() != CE_None )
        bHasGotErr = TRUE;
    GDALClose( hDstDS );

    setMapInMaps(outputs,"Result","value",pszDstFilename);
    
    CPLFree( pszDstFilename );
    CSLDestroy( papszSrcFiles );
    CSLDestroy( papszWarpOptions );
    CSLDestroy( papszTO );

    GDALDumpOpenDatasets( stderr );

    GDALDestroyDriverManager();
    
#ifdef OGR_ENABLED
    if( hCutline != NULL )
        OGR_G_DestroyGeometry( (OGRGeometryH) hCutline );
    OGRCleanupAll();
#endif


/* -------------------------------------------------------------------- */
/*      Clean allocated arguments.                                      */
/* -------------------------------------------------------------------- */
    free(dataPath);
    free(tempPath);

    tmpMap=getMapFromMaps(inputs,"te","value");
    if(tmpMap!=NULL){
      free(pszCutlineDSName);
    }

    tmpMap=getMapFromMaps(inputs,"cwhere","value");
    if(tmpMap!=NULL){
      free(pszCWHERE);
    }

    tmpMap=getMapFromMaps(inputs,"cl","value");
    if(tmpMap!=NULL){
      free(pszCLayer);
    }

    tmpMap=getMapFromMaps(inputs,"csql","value");
    if(tmpMap!=NULL){
      free(pszCSQL);
    }

    return (bHasGotErr) ? SERVICE_FAILED : SERVICE_SUCCEEDED;
}

/************************************************************************/
/*                        GDALWarpCreateOutput()                        */
/*                                                                      */
/*      Create the output file based on various commandline options,    */
/*      and the input file.                                             */
/*      If there's just one source file, then *phTransformArg and       */
/*      *phSrcDS will be set, in order them to be reused by main        */
/*      function. This saves dataset re-opening, and above all transform*/
/*      recomputation, which can be expensive in the -tps case          */
/************************************************************************/

static GDALDatasetH 
GDALWarpCreateOutput( maps*& conf,char **papszSrcFiles, const char *pszFilename, 
                      const char *pszFormat, char **papszTO, 
                      char ***ppapszCreateOptions, GDALDataType eDT,
                      void ** phTransformArg,
                      GDALDatasetH* phSrcDS)


{
    GDALDriverH hDriver;
    GDALDatasetH hDstDS;
    void *hTransformArg;
    GDALColorTableH hCT = NULL;
    double dfWrkMinX=0, dfWrkMaxX=0, dfWrkMinY=0, dfWrkMaxY=0;
    double dfWrkResX=0, dfWrkResY=0;
    int nDstBandCount = 0;
    std::vector<GDALColorInterp> apeColorInterpretations;

    *phTransformArg = NULL;
    *phSrcDS = NULL;

/* -------------------------------------------------------------------- */
/*      Find the output driver.                                         */
/* -------------------------------------------------------------------- */
    hDriver = GDALGetDriverByName( pszFormat );
    if( hDriver == NULL 
        || GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL ) == NULL )
    {
        int	iDr;
        char tmp[2048];
        sprintf( tmp, "Output driver `%s' not recognised or does not support\n"
		 "direct output file creation.  The following format drivers are configured\n"
		 "and support direct output:\n", 
		 pszFormat );

        for( iDr = 0; iDr < GDALGetDriverCount(); iDr++ )
        {
            GDALDriverH hDriver = GDALGetDriver(iDr);

            if( GDALGetMetadataItem( hDriver, GDAL_DCAP_CREATE, NULL) != NULL )
            {
	      char *_tmp=strdup(tmp);
	      sprintf( tmp,"%s  %s: %s\n",
		       _tmp,
		       GDALGetDriverShortName( hDriver  ),
		       GDALGetDriverLongName( hDriver ) );
	      free(_tmp);
            }
        }
	setMapInMaps(conf,"lenv","message",tmp);
        GDALExit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      For virtual output files, we have to set a special subclass     */
/*      of dataset to create.                                           */
/* -------------------------------------------------------------------- */
    if( bVRT )
        *ppapszCreateOptions = 
            CSLSetNameValue( *ppapszCreateOptions, "SUBCLASS", 
                             "VRTWarpedDataset" );

/* -------------------------------------------------------------------- */
/*      Loop over all input files to collect extents.                   */
/* -------------------------------------------------------------------- */
    int     iSrc;
    char    *pszThisTargetSRS = (char*)CSLFetchNameValue( papszTO, "DST_SRS" );
    if( pszThisTargetSRS != NULL )
        pszThisTargetSRS = CPLStrdup( pszThisTargetSRS );

    for( iSrc = 0; papszSrcFiles[iSrc] != NULL; iSrc++ )
    {
        GDALDatasetH hSrcDS;
        const char *pszThisSourceSRS = CSLFetchNameValue(papszTO,"SRC_SRS");

        hSrcDS = GDALOpen( papszSrcFiles[iSrc], GA_ReadOnly );
        if( hSrcDS == NULL )
            GDALExit( 1 );

/* -------------------------------------------------------------------- */
/*      Check that there's at least one raster band                     */
/* -------------------------------------------------------------------- */
        if ( GDALGetRasterCount(hSrcDS) == 0 )
        {
            fprintf(stderr, "Input file %s has no raster bands.\n", papszSrcFiles[iSrc] );
            GDALExit( 1 );
        }

        if( eDT == GDT_Unknown )
            eDT = GDALGetRasterDataType(GDALGetRasterBand(hSrcDS,1));

/* -------------------------------------------------------------------- */
/*      If we are processing the first file, and it has a color         */
/*      table, then we will copy it to the destination file.            */
/* -------------------------------------------------------------------- */
        if( iSrc == 0 )
        {
            nDstBandCount = GDALGetRasterCount(hSrcDS);
            hCT = GDALGetRasterColorTable( GDALGetRasterBand(hSrcDS,1) );
            if( hCT != NULL )
            {
                hCT = GDALCloneColorTable( hCT );
                if( !bQuiet )
                    printf( "Copying color table from %s to new file.\n", 
                            papszSrcFiles[iSrc] );
            }

            for(int iBand = 0; iBand < nDstBandCount; iBand++)
            {
                apeColorInterpretations.push_back(
                    GDALGetRasterColorInterpretation(GDALGetRasterBand(hSrcDS,iBand+1)) );
            }
        }

/* -------------------------------------------------------------------- */
/*      Get the sourcesrs from the dataset, if not set already.         */
/* -------------------------------------------------------------------- */
        if( pszThisSourceSRS == NULL )
        {
            const char *pszMethod = CSLFetchNameValue( papszTO, "METHOD" );

            if( GDALGetProjectionRef( hSrcDS ) != NULL 
                && strlen(GDALGetProjectionRef( hSrcDS )) > 0
                && (pszMethod == NULL || EQUAL(pszMethod,"GEOTRANSFORM")) )
                pszThisSourceSRS = GDALGetProjectionRef( hSrcDS );
            
            else if( GDALGetGCPProjection( hSrcDS ) != NULL
                     && strlen(GDALGetGCPProjection(hSrcDS)) > 0 
                     && GDALGetGCPCount( hSrcDS ) > 1 
                     && (pszMethod == NULL || EQUALN(pszMethod,"GCP_",4)) )
                pszThisSourceSRS = GDALGetGCPProjection( hSrcDS );
            else if( pszMethod != NULL && EQUAL(pszMethod,"RPC") )
                pszThisSourceSRS = SRS_WKT_WGS84;
            else
                pszThisSourceSRS = "";
        }

        if( pszThisTargetSRS == NULL )
            pszThisTargetSRS = CPLStrdup( pszThisSourceSRS );
        
/* -------------------------------------------------------------------- */
/*      Create a transformation object from the source to               */
/*      destination coordinate system.                                  */
/* -------------------------------------------------------------------- */
        hTransformArg = 
            GDALCreateGenImgProjTransformer2( hSrcDS, NULL, papszTO );
        
        if( hTransformArg == NULL )
        {
            CPLFree( pszThisTargetSRS );
            GDALClose( hSrcDS );
            return NULL;
        }
        
        GDALTransformerInfo* psInfo = (GDALTransformerInfo*)hTransformArg;

/* -------------------------------------------------------------------- */
/*      Get approximate output definition.                              */
/* -------------------------------------------------------------------- */
        double adfThisGeoTransform[6];
        double adfExtent[4];
        int    nThisPixels, nThisLines;

        if( GDALSuggestedWarpOutput2( hSrcDS, 
                                      psInfo->pfnTransform, hTransformArg, 
                                      adfThisGeoTransform, 
                                      &nThisPixels, &nThisLines, 
                                      adfExtent, 0 ) != CE_None )
        {
            CPLFree( pszThisTargetSRS );
            GDALClose( hSrcDS );
            return NULL;
        }
        
        if (CPLGetConfigOption( "CHECK_WITH_INVERT_PROJ", NULL ) == NULL)
        {
            double MinX = adfExtent[0];
            double MaxX = adfExtent[2];
            double MaxY = adfExtent[3];
            double MinY = adfExtent[1];
            int bSuccess = TRUE;
            
            /* Check that the the edges of the target image are in the validity area */
            /* of the target projection */
#define N_STEPS 20
            int i,j;
            for(i=0;i<=N_STEPS && bSuccess;i++)
            {
                for(j=0;j<=N_STEPS && bSuccess;j++)
                {
                    double dfRatioI = i * 1.0 / N_STEPS;
                    double dfRatioJ = j * 1.0 / N_STEPS;
                    double expected_x = (1 - dfRatioI) * MinX + dfRatioI * MaxX;
                    double expected_y = (1 - dfRatioJ) * MinY + dfRatioJ * MaxY;
                    double x = expected_x;
                    double y = expected_y;
                    double z = 0;
                    /* Target SRS coordinates to source image pixel coordinates */
                    if (!psInfo->pfnTransform(hTransformArg, TRUE, 1, &x, &y, &z, &bSuccess) || !bSuccess)
                        bSuccess = FALSE;
                    /* Source image pixel coordinates to target SRS coordinates */
                    if (!psInfo->pfnTransform(hTransformArg, FALSE, 1, &x, &y, &z, &bSuccess) || !bSuccess)
                        bSuccess = FALSE;
                    if (fabs(x - expected_x) > (MaxX - MinX) / nThisPixels ||
                        fabs(y - expected_y) > (MaxY - MinY) / nThisLines)
                        bSuccess = FALSE;
                }
            }
            
            /* If not, retry with CHECK_WITH_INVERT_PROJ=TRUE that forces ogrct.cpp */
            /* to check the consistency of each requested projection result with the */
            /* invert projection */
            if (!bSuccess)
            {
                CPLSetConfigOption( "CHECK_WITH_INVERT_PROJ", "TRUE" );
                CPLDebug("WARP", "Recompute out extent with CHECK_WITH_INVERT_PROJ=TRUE");

                if( GDALSuggestedWarpOutput2( hSrcDS, 
                                      psInfo->pfnTransform, hTransformArg, 
                                      adfThisGeoTransform, 
                                      &nThisPixels, &nThisLines, 
                                      adfExtent, 0 ) != CE_None )
                {
                    CPLFree( pszThisTargetSRS );
                    GDALClose( hSrcDS );
                    return NULL;
                }
            }
        }

/* -------------------------------------------------------------------- */
/*      Expand the working bounds to include this region, ensure the    */
/*      working resolution is no more than this resolution.             */
/* -------------------------------------------------------------------- */
        if( dfWrkMaxX == 0.0 && dfWrkMinX == 0.0 )
        {
            dfWrkMinX = adfExtent[0];
            dfWrkMaxX = adfExtent[2];
            dfWrkMaxY = adfExtent[3];
            dfWrkMinY = adfExtent[1];
            dfWrkResX = adfThisGeoTransform[1];
            dfWrkResY = ABS(adfThisGeoTransform[5]);
        }
        else
        {
            dfWrkMinX = MIN(dfWrkMinX,adfExtent[0]);
            dfWrkMaxX = MAX(dfWrkMaxX,adfExtent[2]);
            dfWrkMaxY = MAX(dfWrkMaxY,adfExtent[3]);
            dfWrkMinY = MIN(dfWrkMinY,adfExtent[1]);
            dfWrkResX = MIN(dfWrkResX,adfThisGeoTransform[1]);
            dfWrkResY = MIN(dfWrkResY,ABS(adfThisGeoTransform[5]));
        }

        if (iSrc == 0 && papszSrcFiles[1] == NULL)
        {
            *phTransformArg = hTransformArg;
            *phSrcDS = hSrcDS;
        }
        else
        {
            GDALDestroyGenImgProjTransformer( hTransformArg );
            GDALClose( hSrcDS );
        }
    }

/* -------------------------------------------------------------------- */
/*      Did we have any usable sources?                                 */
/* -------------------------------------------------------------------- */
    if( nDstBandCount == 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "No usable source images." );
        CPLFree( pszThisTargetSRS );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Turn the suggested region into a geotransform and suggested     */
/*      number of pixels and lines.                                     */
/* -------------------------------------------------------------------- */
    double adfDstGeoTransform[6];
    int nPixels, nLines;

    adfDstGeoTransform[0] = dfWrkMinX;
    adfDstGeoTransform[1] = dfWrkResX;
    adfDstGeoTransform[2] = 0.0;
    adfDstGeoTransform[3] = dfWrkMaxY;
    adfDstGeoTransform[4] = 0.0;
    adfDstGeoTransform[5] = -1 * dfWrkResY;

    nPixels = (int) ((dfWrkMaxX - dfWrkMinX) / dfWrkResX + 0.5);
    nLines = (int) ((dfWrkMaxY - dfWrkMinY) / dfWrkResY + 0.5);

/* -------------------------------------------------------------------- */
/*      Did the user override some parameters?                          */
/* -------------------------------------------------------------------- */
    if( dfXRes != 0.0 && dfYRes != 0.0 )
    {
        if( dfMinX == 0.0 && dfMinY == 0.0 && dfMaxX == 0.0 && dfMaxY == 0.0 )
        {
            dfMinX = adfDstGeoTransform[0];
            dfMaxX = adfDstGeoTransform[0] + adfDstGeoTransform[1] * nPixels;
            dfMaxY = adfDstGeoTransform[3];
            dfMinY = adfDstGeoTransform[3] + adfDstGeoTransform[5] * nLines;
        }
        
        if ( bTargetAlignedPixels )
        {
            dfMinX = floor(dfMinX / dfXRes) * dfXRes;
            dfMaxX = ceil(dfMaxX / dfXRes) * dfXRes;
            dfMinY = floor(dfMinY / dfYRes) * dfYRes;
            dfMaxY = ceil(dfMaxY / dfYRes) * dfYRes;
        }

        nPixels = (int) ((dfMaxX - dfMinX + (dfXRes/2.0)) / dfXRes);
        nLines = (int) ((dfMaxY - dfMinY + (dfYRes/2.0)) / dfYRes);
        adfDstGeoTransform[0] = dfMinX;
        adfDstGeoTransform[3] = dfMaxY;
        adfDstGeoTransform[1] = dfXRes;
        adfDstGeoTransform[5] = -dfYRes;
    }

    else if( nForcePixels != 0 && nForceLines != 0 )
    {
        if( dfMinX == 0.0 && dfMinY == 0.0 && dfMaxX == 0.0 && dfMaxY == 0.0 )
        {
            dfMinX = dfWrkMinX;
            dfMaxX = dfWrkMaxX;
            dfMaxY = dfWrkMaxY;
            dfMinY = dfWrkMinY;
        }

        dfXRes = (dfMaxX - dfMinX) / nForcePixels;
        dfYRes = (dfMaxY - dfMinY) / nForceLines;

        adfDstGeoTransform[0] = dfMinX;
        adfDstGeoTransform[3] = dfMaxY;
        adfDstGeoTransform[1] = dfXRes;
        adfDstGeoTransform[5] = -dfYRes;

        nPixels = nForcePixels;
        nLines = nForceLines;
    }

    else if( nForcePixels != 0 )
    {
        if( dfMinX == 0.0 && dfMinY == 0.0 && dfMaxX == 0.0 && dfMaxY == 0.0 )
        {
            dfMinX = dfWrkMinX;
            dfMaxX = dfWrkMaxX;
            dfMaxY = dfWrkMaxY;
            dfMinY = dfWrkMinY;
        }

        dfXRes = (dfMaxX - dfMinX) / nForcePixels;
        dfYRes = dfXRes;

        adfDstGeoTransform[0] = dfMinX;
        adfDstGeoTransform[3] = dfMaxY;
        adfDstGeoTransform[1] = dfXRes;
        adfDstGeoTransform[5] = -dfYRes;

        nPixels = nForcePixels;
        nLines = (int) ((dfMaxY - dfMinY + (dfYRes/2.0)) / dfYRes);
    }

    else if( nForceLines != 0 )
    {
        if( dfMinX == 0.0 && dfMinY == 0.0 && dfMaxX == 0.0 && dfMaxY == 0.0 )
        {
            dfMinX = dfWrkMinX;
            dfMaxX = dfWrkMaxX;
            dfMaxY = dfWrkMaxY;
            dfMinY = dfWrkMinY;
        }

        dfYRes = (dfMaxY - dfMinY) / nForceLines;
        dfXRes = dfYRes;

        adfDstGeoTransform[0] = dfMinX;
        adfDstGeoTransform[3] = dfMaxY;
        adfDstGeoTransform[1] = dfXRes;
        adfDstGeoTransform[5] = -dfYRes;

        nPixels = (int) ((dfMaxX - dfMinX + (dfXRes/2.0)) / dfXRes);
        nLines = nForceLines;
    }

    else if( dfMinX != 0.0 || dfMinY != 0.0 || dfMaxX != 0.0 || dfMaxY != 0.0 )
    {
        dfXRes = adfDstGeoTransform[1];
        dfYRes = fabs(adfDstGeoTransform[5]);

        nPixels = (int) ((dfMaxX - dfMinX + (dfXRes/2.0)) / dfXRes);
        nLines = (int) ((dfMaxY - dfMinY + (dfYRes/2.0)) / dfYRes);

        dfXRes = (dfMaxX - dfMinX) / nPixels;
        dfYRes = (dfMaxY - dfMinY) / nLines;

        adfDstGeoTransform[0] = dfMinX;
        adfDstGeoTransform[3] = dfMaxY;
        adfDstGeoTransform[1] = dfXRes;
        adfDstGeoTransform[5] = -dfYRes;
    }

/* -------------------------------------------------------------------- */
/*      Do we want to generate an alpha band in the output file?        */
/* -------------------------------------------------------------------- */
    if( bEnableSrcAlpha )
        nDstBandCount--;

    if( bEnableDstAlpha )
        nDstBandCount++;

/* -------------------------------------------------------------------- */
/*      Create the output file.                                         */
/* -------------------------------------------------------------------- */
    if( !bQuiet )
        printf( "Creating output file that is %dP x %dL.\n", nPixels, nLines );

    hDstDS = GDALCreate( hDriver, pszFilename, nPixels, nLines, 
                         nDstBandCount, eDT, *ppapszCreateOptions );
    
    if( hDstDS == NULL )
    {
        CPLFree( pszThisTargetSRS );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Write out the projection definition.                            */
/* -------------------------------------------------------------------- */
    GDALSetProjection( hDstDS, pszThisTargetSRS );
    GDALSetGeoTransform( hDstDS, adfDstGeoTransform );

    if (*phTransformArg != NULL)
        GDALSetGenImgProjTransformerDstGeoTransform( *phTransformArg, adfDstGeoTransform);

/* -------------------------------------------------------------------- */
/*      Try to set color interpretation of source bands to target       */
/*      dataset.                                                        */
/*      FIXME? We should likely do that for other drivers than VRT      */
/*      but it might create spurious .aux.xml files (at least with HFA, */
/*      and netCDF)                                                     */
/* -------------------------------------------------------------------- */
    if( bVRT )
    {
        int nBandsToCopy = (int)apeColorInterpretations.size();
        if ( bEnableSrcAlpha )
            nBandsToCopy --;
        for(int iBand = 0; iBand < nBandsToCopy; iBand++)
        {
            GDALSetRasterColorInterpretation(
                GDALGetRasterBand( hDstDS, iBand + 1 ),
                apeColorInterpretations[iBand] );
        }
    }
    
/* -------------------------------------------------------------------- */
/*      Try to set color interpretation of output file alpha band.      */
/* -------------------------------------------------------------------- */
    if( bEnableDstAlpha )
    {
        GDALSetRasterColorInterpretation( 
            GDALGetRasterBand( hDstDS, nDstBandCount ), 
            GCI_AlphaBand );
    }

/* -------------------------------------------------------------------- */
/*      Copy the color table, if required.                              */
/* -------------------------------------------------------------------- */
    if( hCT != NULL )
    {
        GDALSetRasterColorTable( GDALGetRasterBand(hDstDS,1), hCT );
        GDALDestroyColorTable( hCT );
    }

    CPLFree( pszThisTargetSRS );
    return hDstDS;
}

/************************************************************************/
/*                      GeoTransform_Transformer()                      */
/*                                                                      */
/*      Convert points from georef coordinates to pixel/line based      */
/*      on a geotransform.                                              */
/************************************************************************/

class CutlineTransformer : public OGRCoordinateTransformation
{
public:

    void         *hSrcImageTransformer;

    virtual OGRSpatialReference *GetSourceCS() { return NULL; }
    virtual OGRSpatialReference *GetTargetCS() { return NULL; }

    virtual int Transform( int nCount, 
                           double *x, double *y, double *z = NULL ) {
        int nResult;

        int *pabSuccess = (int *) CPLCalloc(sizeof(int),nCount);
        nResult = TransformEx( nCount, x, y, z, pabSuccess );
        CPLFree( pabSuccess );

        return nResult;
    }

    virtual int TransformEx( int nCount, 
                             double *x, double *y, double *z = NULL,
                             int *pabSuccess = NULL ) {
        return GDALGenImgProjTransform( hSrcImageTransformer, TRUE, 
                                        nCount, x, y, z, pabSuccess );
    }
};


/************************************************************************/
/*                            LoadCutline()                             */
/*                                                                      */
/*      Load blend cutline from OGR datasource.                         */
/************************************************************************/

static void
LoadCutline( const char *pszCutlineDSName, const char *pszCLayer, 
             const char *pszCWHERE, const char *pszCSQL, 
             void **phCutlineRet )

{
#ifndef OGR_ENABLED
    CPLError( CE_Failure, CPLE_AppDefined, 
              "Request to load a cutline failed, this build does not support OGR features.\n" );
    GDALExit( 1 );
#else // def OGR_ENABLED
    OGRRegisterAll();

/* -------------------------------------------------------------------- */
/*      Open source vector dataset.                                     */
/* -------------------------------------------------------------------- */
    OGRDataSourceH hSrcDS;

    hSrcDS = OGROpen( pszCutlineDSName, FALSE, NULL );
    if( hSrcDS == NULL )
        GDALExit( 1 );

/* -------------------------------------------------------------------- */
/*      Get the source layer                                            */
/* -------------------------------------------------------------------- */
    OGRLayerH hLayer = NULL;

    if( pszCSQL != NULL )
        hLayer = OGR_DS_ExecuteSQL( hSrcDS, pszCSQL, NULL, NULL ); 
    else if( pszCLayer != NULL )
        hLayer = OGR_DS_GetLayerByName( hSrcDS, pszCLayer );
    else
        hLayer = OGR_DS_GetLayer( hSrcDS, 0 );

    if( hLayer == NULL )
    {
        fprintf( stderr, "Failed to identify source layer from datasource.\n" );
        GDALExit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Apply WHERE clause if there is one.                             */
/* -------------------------------------------------------------------- */
    if( pszCWHERE != NULL )
        OGR_L_SetAttributeFilter( hLayer, pszCWHERE );

/* -------------------------------------------------------------------- */
/*      Collect the geometries from this layer, and build list of       */
/*      burn values.                                                    */
/* -------------------------------------------------------------------- */
    OGRFeatureH hFeat;
    OGRGeometryH hMultiPolygon = OGR_G_CreateGeometry( wkbMultiPolygon );

    OGR_L_ResetReading( hLayer );
    
    while( (hFeat = OGR_L_GetNextFeature( hLayer )) != NULL )
    {
        OGRGeometryH hGeom = OGR_F_GetGeometryRef(hFeat);

        if( hGeom == NULL )
        {
            fprintf( stderr, "ERROR: Cutline feature without a geometry.\n" );
            GDALExit( 1 );
        }
        
        OGRwkbGeometryType eType = wkbFlatten(OGR_G_GetGeometryType( hGeom ));

        if( eType == wkbPolygon )
            OGR_G_AddGeometry( hMultiPolygon, hGeom );
        else if( eType == wkbMultiPolygon )
        {
            int iGeom;

            for( iGeom = 0; iGeom < OGR_G_GetGeometryCount( hGeom ); iGeom++ )
            {
                OGR_G_AddGeometry( hMultiPolygon, 
                                   OGR_G_GetGeometryRef(hGeom,iGeom) );
            }
        }
        else
        {
            fprintf( stderr, "ERROR: Cutline not of polygon type.\n" );
            GDALExit( 1 );
        }

        OGR_F_Destroy( hFeat );
    }

    if( OGR_G_GetGeometryCount( hMultiPolygon ) == 0 )
    {
        fprintf( stderr, "ERROR: Did not get any cutline features.\n" );
        GDALExit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Ensure the coordinate system gets set on the geometry.          */
/* -------------------------------------------------------------------- */
    OGR_G_AssignSpatialReference(
        hMultiPolygon, OGR_L_GetSpatialRef(hLayer) );

    *phCutlineRet = (void *) hMultiPolygon;

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    if( pszCSQL != NULL )
        OGR_DS_ReleaseResultSet( hSrcDS, hLayer );

    OGR_DS_Destroy( hSrcDS );
#endif
}

/************************************************************************/
/*                      TransformCutlineToSource()                      */
/*                                                                      */
/*      Transform cutline from its SRS to source pixel/line coordinates.*/
/************************************************************************/
static void
TransformCutlineToSource( GDALDatasetH hSrcDS, void *hCutline,
                          char ***ppapszWarpOptions, char **papszTO_In )

{
#ifdef OGR_ENABLED
    OGRGeometryH hMultiPolygon = OGR_G_Clone( (OGRGeometryH) hCutline );
    char **papszTO = CSLDuplicate( papszTO_In );

/* -------------------------------------------------------------------- */
/*      Checkout that SRS are the same.                                 */
/* -------------------------------------------------------------------- */
    OGRSpatialReferenceH  hRasterSRS = NULL;
    const char *pszProjection = NULL;

    if( GDALGetProjectionRef( hSrcDS ) != NULL 
        && strlen(GDALGetProjectionRef( hSrcDS )) > 0 )
        pszProjection = GDALGetProjectionRef( hSrcDS );
    else if( GDALGetGCPProjection( hSrcDS ) != NULL )
        pszProjection = GDALGetGCPProjection( hSrcDS );

    if( pszProjection != NULL )
    {
        hRasterSRS = OSRNewSpatialReference(NULL);
        if( OSRImportFromWkt( hRasterSRS, (char **)&pszProjection ) != CE_None )
        {
            OSRDestroySpatialReference(hRasterSRS);
            hRasterSRS = NULL;
        }
    }

    OGRSpatialReferenceH hCutlineSRS = OGR_G_GetSpatialReference( hMultiPolygon );
    if( hRasterSRS != NULL && hCutlineSRS != NULL )
    {
        /* ok, we will reproject */
    }
    else if( hRasterSRS != NULL && hCutlineSRS == NULL )
    {
        fprintf(stderr,
                "Warning : the source raster dataset has a SRS, but the cutline features\n"
                "not.  We assume that the cutline coordinates are expressed in the destination SRS.\n"
                "If not, cutline results may be incorrect.\n");
    }
    else if( hRasterSRS == NULL && hCutlineSRS != NULL )
    {
        fprintf(stderr,
                "Warning : the input vector layer has a SRS, but the source raster dataset does not.\n"
                "Cutline results may be incorrect.\n");
    }

    if( hRasterSRS != NULL )
        OSRDestroySpatialReference(hRasterSRS);

/* -------------------------------------------------------------------- */
/*      Extract the cutline SRS WKT.                                    */
/* -------------------------------------------------------------------- */
    if( hCutlineSRS != NULL )
    {
        char *pszCutlineSRS_WKT = NULL;

        OSRExportToWkt( hCutlineSRS, &pszCutlineSRS_WKT );
        papszTO = CSLSetNameValue( papszTO, "DST_SRS", pszCutlineSRS_WKT );
        CPLFree( pszCutlineSRS_WKT );
    }

/* -------------------------------------------------------------------- */
/*      It may be unwise to let the mask geometry be re-wrapped by      */
/*      the CENTER_LONG machinery as this can easily screw up world     */
/*      spanning masks and invert the mask topology.                    */
/* -------------------------------------------------------------------- */
    papszTO = CSLSetNameValue( papszTO, "INSERT_CENTER_LONG", "FALSE" );

/* -------------------------------------------------------------------- */
/*      Transform the geometry to pixel/line coordinates.               */
/* -------------------------------------------------------------------- */
    CutlineTransformer oTransformer;

    /* The cutline transformer will *invert* the hSrcImageTransformer */
    /* so it will convert from the cutline SRS to the source pixel/line */
    /* coordinates */
    oTransformer.hSrcImageTransformer = 
        GDALCreateGenImgProjTransformer2( hSrcDS, NULL, papszTO );

    CSLDestroy( papszTO );

    if( oTransformer.hSrcImageTransformer == NULL )
        GDALExit( 1 );

    OGR_G_Transform( hMultiPolygon, 
                     (OGRCoordinateTransformationH) &oTransformer );

    GDALDestroyGenImgProjTransformer( oTransformer.hSrcImageTransformer );

/* -------------------------------------------------------------------- */
/*      Convert aggregate geometry into WKT.                            */
/* -------------------------------------------------------------------- */
    char *pszWKT = NULL;

    OGR_G_ExportToWkt( hMultiPolygon, &pszWKT );
    OGR_G_DestroyGeometry( hMultiPolygon );

    *ppapszWarpOptions = CSLSetNameValue( *ppapszWarpOptions, 
                                          "CUTLINE", pszWKT );
    CPLFree( pszWKT );
#endif
}

}
