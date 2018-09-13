/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Program to generate a UMN MapServer compatible tile index for a
 *           set of OGR data sources.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2002, Frank Warmerdam
 * Copyright (c) 2007-2010, Even Rouault <even dot rouault at mines-paris dot org>
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

#include "cpl_port.h"

#include <cassert>
#include <vector>

#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"
#include "gdal_version.h"
//#include "commonutils.h"
#if GDAL_VERSION_MAJOR >= 2
#include "gdal.h"
#include "gdal_priv.h"
#endif
#ifdef ZOO_SERVICE
#include "service.h"
#include "service_internal.h"
#endif

CPL_CVSID("$Id$")

#ifdef ZOO_SERVICE
extern "C" {
#endif

typedef enum
{
    FORMAT_AUTO,
    FORMAT_WKT,
    FORMAT_EPSG,
    FORMAT_PROJ
} SrcSRSFormat;

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf("Usage: ogrtindex [-lnum n]... [-lname name]... [-f output_format]\n"
           "                 [-write_absolute_path] [-skip_different_projection]\n"
           "                 [-t_srs target_srs]\n"
           "                 [-src_srs_name field_name] [-src_srs_format [AUTO|WKT|EPSG|PROJ]\n"
           "                 [-accept_different_schemas]\n"
           "                 output_dataset src_dataset...\n");
    printf("\n");
    printf("  -lnum n: Add layer number 'n' from each source file\n"
           "           in the tile index.\n");
    printf("  -lname name: Add the layer named 'name' from each source file\n"
           "               in the tile index.\n");
    printf("  -f output_format: Select an output format name.\n");
    printf("  -tileindex field_name: The name to use for the dataset name.\n"
           "                         Defaults to LOCATION.\n");
    printf("  -write_absolute_path: Filenames are written with absolute paths.\n");
    printf("  -skip_different_projection: Only layers with same projection ref \n"
           "        as layers already inserted in the tileindex will be inserted.\n");
    printf("  -accept_different_schemas: by default ogrtindex checks that all layers inserted\n"
           "                             into the index have the same attribute schemas. If you\n"
           "                             specify this option, this test will be disabled. Be aware that\n"
           "                             resulting index may be incompatible with MapServer!\n");
    printf(
           "  - If -t_srs is specified, geometries of input files will be transformed to the desired\n"
           "    target coordinate reference system.\n"
           "    Note that using this option generates files that are NOT compatible with MapServer < 7.2.\n"
           "  - Simple rectangular polygons are generated in the same coordinate reference system\n"
           "    as the vectors, or in target reference system if the -t_srs option is used.\n");
    printf("\n");
    printf("If no -lnum or -lname arguments are given it is assumed that\n"
           "all layers in source datasets should be added to the tile index\n"
           "as independent records.\n");
    exit(1);
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

#ifdef ZOO_SERVICE
#ifdef WIN32
  __declspec(dllexport)
#endif
  int Ogrtindex(maps*& conf,maps*& inputs,maps*& outputs)
#else
  MAIN_START(nArgc, papszArgv)
#endif
{
  FILE* log=fopen("/tmp/error.log","w+");
  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
  fflush(log);
  //return SERVICE_FAILED;
    // Check strict compilation and runtime library version as we use C++ API.
#ifdef ZOO_SERVICE
  if( !GDAL_CHECK_VERSION("ogrtindex") )
      {
	setMapInMaps(conf,"lenv","message",_("Compilation and runtime library versions differ"));
	return SERVICE_FAILED;
      }
    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);
#else
    if( !GDAL_CHECK_VERSION(papszArgv[0]) )
      exit(1);
#endif
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    OGRRegisterAll();

/* -------------------------------------------------------------------- */
/*      Processing command line arguments.                              */
/* -------------------------------------------------------------------- */
    int nFirstSourceDataset = -1;
    bool bLayersWildcarded = true;
    const char *pszFormat = nullptr;
    const char *pszTileIndexField = "LOCATION";
    const char *pszOutputName = nullptr;
    bool write_absolute_path = false;
    bool skip_different_projection = false;
    char* current_path = nullptr;
    bool accept_different_schemas = false;
    bool bFirstWarningForNonMatchingAttributes = true;
    const char *pszTargetSRS = "";
    bool bSetTargetSRS = false;
    const char* pszSrcSRSName = nullptr;
    int i_SrcSRSName = -1;
    bool bSrcSRSFormatSpecified = false;
    SrcSRSFormat eSrcSRSFormat = FORMAT_AUTO;
    size_t nMaxFieldSize = 254;

    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);
#ifdef ZOO_SERVICE
      // f / of
      map* tmpMap=getMapFromMaps(inputs,"f","value");
      if(tmpMap==NULL)
	tmpMap=getMapFromMaps(inputs,"of","value");
      if(tmpMap!=NULL)
	pszFormat = zStrdup(tmpMap->value);
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);

      // write_absolute_path
      tmpMap=getMapFromMaps(inputs,"write_absolute_path","value");
      if(tmpMap!=NULL && strncasecmp(tmpMap->value,"true",4)==0)
	write_absolute_path = true;
      // skip_different_projection
      tmpMap=getMapFromMaps(inputs,"skip_different_projection","value");
      if(tmpMap!=NULL && strncasecmp(tmpMap->value,"true",4)==0)
	skip_different_projection = true;
      // accept_different_schemas
      tmpMap=getMapFromMaps(inputs,"accept_different_schemas","value");
      if(tmpMap!=NULL && strncasecmp(tmpMap->value,"true",4)==0)
	accept_different_schemas = true;
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      // tileindex
      tmpMap=getMapFromMaps(inputs,"tileindex","value");
      if(tmpMap!=NULL)
	pszTileIndexField = zStrdup(tmpMap->value);
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      // lname / lnum
      tmpMap=getMapFromMaps(inputs,"lname","value");
      if(tmpMap==NULL)
	tmpMap=getMapFromMaps(inputs,"lnum","value");
      if(tmpMap!=NULL)
	bLayersWildcarded = false;
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      //dumpMaps(inputs);
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      // t_srs
      tmpMap=getMapFromMaps(inputs,"t_srs","value");
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      if(tmpMap!=NULL){
	pszTargetSRS = zStrdup(tmpMap->value);
	bSetTargetSRS = true;
      }
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      // src_srs_name
      //dumpMaps(inputs);
      tmpMap=getMapFromMaps(inputs,"src_srs_name","value");
      if(tmpMap!=NULL){
	pszSrcSRSName = zStrdup(tmpMap->value);
      }
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      // src_srs_format
      tmpMap=getMapFromMaps(inputs,"src_srs_format","value");
      if(tmpMap!=NULL){
	bSrcSRSFormatSpecified = true;
	const char* pszSRSFormat = zStrdup(tmpMap->value);
	if( EQUAL(pszSRSFormat, "AUTO") )
	  eSrcSRSFormat = FORMAT_AUTO;
	else if( EQUAL(pszSRSFormat, "WKT") )
	  eSrcSRSFormat = FORMAT_WKT;
	else if( EQUAL(pszSRSFormat, "EPSG") )
	  eSrcSRSFormat = FORMAT_EPSG;
	else if( EQUAL(pszSRSFormat, "PROJ") )
	  eSrcSRSFormat = FORMAT_PROJ;	
      }
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      // OutputName
      tmpMap=getMapFromMaps(inputs,"OutputName","value");
      if(tmpMap!=NULL){
	pszOutputName = zStrdup(tmpMap->value);
      }
      nFirstSourceDataset = 0;
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
#else      
    for( int iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and "
                   "is running against GDAL %s\n",
                   papszArgv[0], GDAL_RELEASE_NAME,
                   GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        else if( iArg < nArgc-1 &&
                 (EQUAL(papszArgv[iArg],"-f") || EQUAL(papszArgv[iArg],"-of")) )
        {
            pszFormat = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-write_absolute_path"))
        {
            write_absolute_path = true;
        }
        else if( EQUAL(papszArgv[iArg],"-skip_different_projection"))
        {
            skip_different_projection = true;
        }
        else if( EQUAL(papszArgv[iArg],"-accept_different_schemas"))
        {
            accept_different_schemas = true;
        }
        else if( iArg < nArgc-1 && EQUAL(papszArgv[iArg],"-tileindex") )
        {
            pszTileIndexField = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-lnum")
                 || EQUAL(papszArgv[iArg],"-lname") )
        {
            iArg++;
            bLayersWildcarded = false;
        }
        else if( iArg < nArgc-1 && strcmp(papszArgv[iArg],"-t_srs") == 0 )
        {
            pszTargetSRS = papszArgv[++iArg];
            bSetTargetSRS = true;
        }
        else if( iArg < nArgc-1 &&
                 strcmp(papszArgv[iArg], "-src_srs_name") == 0 )
        {
            pszSrcSRSName = papszArgv[++iArg];
        }
        else if( iArg < nArgc-1 &&
                 strcmp(papszArgv[iArg], "-src_srs_format") == 0 )
        {
            bSrcSRSFormatSpecified = true;
            const char* pszSRSFormat = papszArgv[++iArg];
            if( EQUAL(pszSRSFormat, "AUTO") )
                eSrcSRSFormat = FORMAT_AUTO;
            else if( EQUAL(pszSRSFormat, "WKT") )
                eSrcSRSFormat = FORMAT_WKT;
            else if( EQUAL(pszSRSFormat, "EPSG") )
                eSrcSRSFormat = FORMAT_EPSG;
            else if( EQUAL(pszSRSFormat, "PROJ") )
                eSrcSRSFormat = FORMAT_PROJ;
        }
        else if( papszArgv[iArg][0] == '-' )
        {
            Usage();
        }
        else if( pszOutputName == nullptr )
        {
            pszOutputName = papszArgv[iArg];
        }
        else if( nFirstSourceDataset == -1 )
        {
            nFirstSourceDataset = iArg;
        }
    }
#endif
    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);
    if( pszOutputName == nullptr || nFirstSourceDataset == -1 )
        Usage();
    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);

    if( bSrcSRSFormatSpecified && pszSrcSRSName == nullptr )
    {
        fprintf(log,
                "-src_srs_name must be specified when -src_srs_format is "
                "specified.\n");
        Usage();
    }
    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);

/* -------------------------------------------------------------------- */
/*      Create and validate target SRS if given.                        */
/* -------------------------------------------------------------------- */
    OGRSpatialReference* poTargetSRS = nullptr;
    if( bSetTargetSRS )
    {
        if( skip_different_projection )
        {
            fprintf(log,
                    "Warning : -skip_different_projection does not apply "
                    "when -t_srs is requested.\n");
        }
        poTargetSRS = new OGRSpatialReference();
        // coverity[tainted_data]
        if( poTargetSRS->SetFromUserInput( pszTargetSRS ) != CE_None )
        {
            delete poTargetSRS;
            fprintf(log, "Invalid target SRS `%s'.\n",
                    pszTargetSRS);
            exit(1);
        }
    }
    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);

/* -------------------------------------------------------------------- */
/*      Try to open as an existing dataset for update access.           */
/* -------------------------------------------------------------------- */
    GDALDataset *poDstDS = reinterpret_cast<GDALDataset*>(
        OGROpen(pszOutputName, TRUE, nullptr));
    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);

/* -------------------------------------------------------------------- */
/*      If that failed, find the driver so we can create the tile index.*/
/* -------------------------------------------------------------------- */
    OGRLayer *poDstLayer = nullptr;
    int                      nArgc;

    fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
    fflush(log);
    if( poDstDS == nullptr )
    {
        CPLString osFormat;
	
	OGRSFDriverRegistrar     *poR = OGRSFDriverRegistrar::GetRegistrar();
        GDALDriver              *poDriver = NULL;
        int                      iDriver;
	
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);
        for( iDriver = 0;
             iDriver < poR->GetDriverCount() && poDriver == NULL;
             iDriver++ )
        {
            if( EQUAL(poR->GetDriver(iDriver)->GetDescription(),pszFormat) )
            {
	      osFormat=pszFormat;
                poDriver = poR->GetDriver(iDriver);
            }
        }

	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);
        if( poDriver == NULL )
        {
            fprintf( log, "Unable to find driver `%s'.\n", pszFormat );
            fprintf( log, "The following drivers are available:\n" );

            for( iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
            {
                fprintf( log, "  -> `%s'\n", poR->GetDriver(iDriver)->GetDescription() );
            }
#ifdef ZOO_SERVICE
	    setMapInMaps(conf,"lenv","message",_("Unable to find driver!"));	    
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);

	/*        if( !CPLTestBool( CSLFetchNameValueDef(poDriver->GetMetadata(), GDAL_DCAP_CREATE, "FALSE") ) )
        {
            fprintf( log, "%s driver does not support data source creation.\n",
                    pszFormat );
#ifdef ZOO_SERVICE
	    setMapInMaps(conf,"lenv","message",_("This driver does not support data source creation!"));	    
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
}*/
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);

/* -------------------------------------------------------------------- */
/*      Now create it.                                                  */
/* -------------------------------------------------------------------- */

        poDstDS = reinterpret_cast<GDALDataset*>(
            GDALCreate(poDriver, pszOutputName, 0, 0, 0, GDT_Unknown, nullptr));
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);
        if( poDstDS == nullptr )
        {
            fprintf(log, "%s driver failed to create %s\n",
                    osFormat.c_str(), pszOutputName);
#ifdef ZOO_SERVICE
	    setMapInMaps(conf,"lenv","message",_("This driver failed to create!"));
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);

        if( poDstDS->GetLayerCount() == 0 )
        {
#ifndef ZOO_SERVICE
	  if( nFirstSourceDataset < nArgc &&
	      papszArgv[nFirstSourceDataset][0] == '-' )
            {
	      nFirstSourceDataset++;
	    }
#else
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);
	  map* lengthMap=getMapFromMaps(inputs,"files","length");
	  maps* tmpMaps=getMaps(inputs,"files");
	  if(lengthMap!=NULL)
	    nArgc=atoi(lengthMap->value);
	  else
	    nArgc=1;
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);
#endif	  
            OGRSpatialReference* poSrcSpatialRef = nullptr;
            if( bSetTargetSRS )
            {
                // Fetches the SRS from target SRS (if set), or from the SRS of
                // the first layer and use it when creating the
                // tileindex layer.
                poSrcSpatialRef = poTargetSRS->Clone();
            }
            else if( nFirstSourceDataset < nArgc )
            {
#ifdef ZOO_SERVICE
	  fprintf(log,"**** %s %d %s \n",__FILE__,__LINE__,nFirstSourceDataset);
	  fflush(log);
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);
	  tmpMap=getMapArray(tmpMaps->content,"value",0);
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);
	  //dumpMap(tmpMap);
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);
#endif
	  GDALDataset* poDS =
	    reinterpret_cast<GDALDataset*>(
#ifndef ZOO_SERVICE
                    OGROpen(papszArgv[nFirstSourceDataset], FALSE, nullptr)
#else
                    OGROpen(tmpMap->value, FALSE, nullptr)
#endif
	    );
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);
                if( poDS != nullptr )
                {
                    for( int iLayer = 0;
                         iLayer < poDS->GetLayerCount();
                         iLayer++ )
                    {
                        bool bRequested = bLayersWildcarded;
                        OGRLayer *poLayer = poDS->GetLayer(iLayer);

                        if( !bRequested )
                            continue;

                        if( poLayer->GetSpatialRef() )
                            poSrcSpatialRef = poLayer->GetSpatialRef()->Clone();
                        break;
                    }
                }

                GDALClose(poDS);
            }
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);

            poDstLayer = poDstDS->CreateLayer("tileindex", poSrcSpatialRef);

            OGRFieldDefn oLocation(pszTileIndexField, OFTString);
            oLocation.SetWidth(200);
            poDstLayer->CreateField(&oLocation);

            if( pszSrcSRSName != nullptr )
            {
                OGRFieldDefn oSrcSRSNameField(pszSrcSRSName, OFTString);
                poDstLayer->CreateField(&oSrcSRSNameField);
            }

            if( poSrcSpatialRef )
                poSrcSpatialRef->Release();
	  fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	  fflush(log);
        }
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);
    }

/* -------------------------------------------------------------------- */
/*      Identify target layer and field.                                */
/* -------------------------------------------------------------------- */

	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);
    poDstLayer = poDstDS->GetLayer(0);
    if( poDstLayer == nullptr )
    {
        fprintf(log, "Can't find any layer in output tileindex!\n");
        exit(1);
    }
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);

    const int iTileIndexField =
        poDstLayer->GetLayerDefn()->GetFieldIndex(pszTileIndexField);
    if( iTileIndexField == -1 )
    {
        fprintf(log, "Can't find %s field in tile index dataset.\n",
                pszTileIndexField);
        exit(1);
    }
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);

    if( pszSrcSRSName != nullptr )
        i_SrcSRSName = poDstLayer->GetLayerDefn()->GetFieldIndex(pszSrcSRSName);

    OGRFeatureDefn* poFeatureDefn = nullptr;

    // Load in memory existing file names in SHP.
    char **existingLayersTab = nullptr;
    OGRSpatialReference* alreadyExistingSpatialRef = nullptr;
    bool alreadyExistingSpatialRefValid = false;
    const int nExistingLayers = static_cast<int>(poDstLayer->GetFeatureCount());
    if( nExistingLayers )
    {
        existingLayersTab = static_cast<char **>(
            CPLMalloc(nExistingLayers * sizeof(char*)));
        for( int i = 0; i < nExistingLayers; i++ )
        {
            OGRFeature* feature = poDstLayer->GetNextFeature();
            existingLayersTab[i] =
                CPLStrdup(feature->GetFieldAsString( iTileIndexField));
            if( i == 0 )
            {
                char* filename = CPLStrdup(existingLayersTab[i]);
                // j used after for.
                int j = static_cast<int>(strlen(filename)) - 1;
                for( ; j >= 0; j-- )
                {
                    if( filename[j] == ',' )
                        break;
                }
                GDALDataset *poDS = nullptr;
                if( j >= 0 )
                {
                    const int iLayer = atoi(filename + j + 1);
                    filename[j] = 0;
                    poDS = reinterpret_cast<GDALDataset *>(
                        OGROpen(filename, FALSE, nullptr));
                    if( poDS != nullptr )
                    {
                        OGRLayer *poLayer = poDS->GetLayer(iLayer);
                        if( poLayer )
                        {
                            alreadyExistingSpatialRefValid = true;
                            alreadyExistingSpatialRef =
                                poLayer->GetSpatialRef() ?
                                poLayer->GetSpatialRef()->Clone() : nullptr;

                            if( poFeatureDefn == nullptr )
                                poFeatureDefn =
                                    poLayer->GetLayerDefn()->Clone();
                        }
                        GDALClose(poDS);
                    }
                }
            }
        }
    }

	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);
    if( write_absolute_path )
    {
        current_path = CPLGetCurrentDir();
        if( current_path == nullptr )
        {
            fprintf(log,
                    "This system does not support the CPLGetCurrentDir call. "
                    "The option -write_absolute_path will have no effect\n");
            write_absolute_path = false;
        }
    }
	fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
	fflush(log);
/* ==================================================================== */
/*      Process each input datasource in turn.                          */
/* ==================================================================== */
#ifdef ZOO_SERVICE
    maps* tmpMaps=getMaps(inputs,"files");
    nFirstSourceDataset=0;
#endif
    for( ; nFirstSourceDataset < nArgc; nFirstSourceDataset++ )
    {
#ifndef ZOO_SERVICE
      if( papszArgv[nFirstSourceDataset][0] == '-' )
        {
            nFirstSourceDataset++;
            continue;
        }
#else
      tmpMap=getMapArray(tmpMaps->content,"value",nFirstSourceDataset);
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);
      //dumpMap(tmpMap);
      fprintf(log,"**** %s %d \n",__FILE__,__LINE__);
      fflush(log);

#endif
      char* fileNameToWrite = nullptr;
      VSIStatBuf sStatBuf;
      if( write_absolute_path &&
#ifdef ZOO_SERVICE
            CPLIsFilenameRelative( tmpMap->value ) &&
            VSIStat( tmpMap->value, &sStatBuf ) == 0
#else
            CPLIsFilenameRelative( papszArgv[nFirstSourceDataset] ) &&
            VSIStat( papszArgv[nFirstSourceDataset], &sStatBuf ) == 0	    
#endif
	    )
        {
            fileNameToWrite =
                CPLStrdup(CPLProjectRelativeFilename(
						     current_path,
#ifdef ZOO_SERVICE
						     tmpMap->value
#else
						     papszArgv[nFirstSourceDataset]
#endif
						     ));
        }
        else
        {
#ifdef ZOO_SERVICE
	  fileNameToWrite = CPLStrdup(tmpMap->value);
#else
	  fileNameToWrite = CPLStrdup(papszArgv[nFirstSourceDataset]);
#endif
        }

        GDALDataset *poDS = reinterpret_cast<GDALDataset*>(
							   OGROpen(
#ifdef ZOO_SERVICE
								   tmpMap->value
#else
								   papszArgv[nFirstSourceDataset]
#endif		    
								   ,
								   FALSE,
								   nullptr )
							   );

        if( poDS == nullptr )
        {
	  fprintf(log, "Failed to open dataset %s, skipping.\n",
#ifndef ZOO_SERVICE
		  papszArgv[nFirstSourceDataset]
#else
		  tmpMap->value
#endif
		  );
            CPLFree(fileNameToWrite);
            continue;
        }

/* -------------------------------------------------------------------- */
/*      Check all layers, and see if they match requests.               */
/* -------------------------------------------------------------------- */
        for( int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ )
        {
            bool bRequested = bLayersWildcarded;
            OGRLayer *poLayer = poDS->GetLayer(iLayer);

#ifndef ZOO_SERVICE
            for( int iArg = 1; iArg < nArgc && !bRequested; iArg++ )
            {
                if( EQUAL(papszArgv[iArg], "-lnum")
                    && atoi(papszArgv[iArg+1]) == iLayer )
                    bRequested = true;
                else if( EQUAL(papszArgv[iArg], "-lname" )
                         && EQUAL(papszArgv[iArg+1],
                                  poLayer->GetLayerDefn()->GetName()) )
                    bRequested = true;
            }
#endif
	    
            if( !bRequested )
                continue;

            // Checks that the layer is not already in tileindex.
            int i = 0;  // Used after for.
            for( ; i < nExistingLayers; i++ )
            {
                // TODO(schwehr): Move this off of the stack.
                char szLocation[5000] = {};
                snprintf(szLocation, sizeof(szLocation), "%s,%d",
                         fileNameToWrite, iLayer);
                if( EQUAL(szLocation, existingLayersTab[i]) )
                {
                    fprintf(log, "Layer %d of %s is already in tileindex. "
                            "Skipping it.\n",
                            iLayer, szLocation);
                    break;
                }
            }
            if( i != nExistingLayers )
            {
                continue;
            }

            OGRSpatialReference* spatialRef = poLayer->GetSpatialRef();
            // If not set target srs, test that the current file uses same
            // projection as others.
            if( !bSetTargetSRS )
            {
                if( alreadyExistingSpatialRefValid )
                {
                    if( (spatialRef != nullptr &&
                         alreadyExistingSpatialRef != nullptr &&
                        spatialRef->IsSame(alreadyExistingSpatialRef) ==
                          FALSE) ||
                        ((spatialRef != nullptr) !=
                        (alreadyExistingSpatialRef != nullptr)) )
                    {
                        fprintf(
                            log,
                            "Warning : layer %d of %s is not using the same "
                            "projection system as other files in the "
                            "tileindex. This may cause problems when using it "
                            "in MapServer for example.%s\n",
                            iLayer,
#ifdef ZOO_SERVICE
			    tmpMap->value
#else
			    papszArgv[nFirstSourceDataset]
#endif
			    ,
                            skip_different_projection ? " Skipping it" : "");
                        if( skip_different_projection )
                        {
                            continue;
                        }
                    }
                }
                else
                {
                    alreadyExistingSpatialRefValid = true;
                    alreadyExistingSpatialRef =
                        spatialRef ? spatialRef->Clone() : nullptr;
                }
            }

/* -------------------------------------------------------------------- */
/*      Check if all layers in dataset have the same attributes schema. */
/* -------------------------------------------------------------------- */
            if( poFeatureDefn == nullptr )
            {
                poFeatureDefn = poLayer->GetLayerDefn()->Clone();
            }
            else if( !accept_different_schemas )
            {
                OGRFeatureDefn* poFeatureDefnCur = poLayer->GetLayerDefn();
                assert(nullptr != poFeatureDefnCur);

                const int fieldCount = poFeatureDefnCur->GetFieldCount();

                if( fieldCount != poFeatureDefn->GetFieldCount())
                {
                    fprintf( log, "Number of attributes of layer %s of %s "
                             "does not match ... skipping it.\n",
                             poLayer->GetLayerDefn()->GetName(),
#ifdef ZOO_SERVICE
			     tmpMap->value
#else		     
                             papszArgv[nFirstSourceDataset]
#endif			     
			     );
                    if( bFirstWarningForNonMatchingAttributes )
                    {
                        fprintf(
                            log, "Note : you can override this "
                            "behaviour with -accept_different_schemas option\n"
                            "but this may result in a tileindex incompatible "
                            "with MapServer\n");
                        bFirstWarningForNonMatchingAttributes = false;
                    }
                    continue;
                }

                bool bSkip = false;
                for( int fn = 0; fn < poFeatureDefnCur->GetFieldCount(); fn++ )
                {
                    OGRFieldDefn* poField = poFeatureDefn->GetFieldDefn(fn);
                    OGRFieldDefn* poFieldCur =
                        poFeatureDefnCur->GetFieldDefn(fn);

                    // XXX - Should those pointers be checked against NULL?
                    assert(nullptr != poField);
                    assert(nullptr != poFieldCur);

                    if( poField->GetType() != poFieldCur->GetType()
                        || poField->GetWidth() != poFieldCur->GetWidth()
                        || poField->GetPrecision() != poFieldCur->GetPrecision()
                        || !EQUAL( poField->GetNameRef(),
                                   poFieldCur->GetNameRef() ) )
                    {
                        fprintf(
                            log, "Schema of attributes of layer %s of %s "
                            "does not match. Skipping it.\n",
                            poLayer->GetLayerDefn()->GetName(),
#ifdef ZOO_SERVICE
			    tmpMap->value
#else		     
			    papszArgv[nFirstSourceDataset]
#endif
				);
                        if( bFirstWarningForNonMatchingAttributes )
                        {
                            fprintf(
                                log, "Note : you can override this "
                                "behaviour with -accept_different_schemas "
                                "option,\nbut this may result in a tileindex "
                                "incompatible with MapServer\n");
                            bFirstWarningForNonMatchingAttributes = false;
                        }
                        bSkip = true;
                        break;
                    }
                }

                if( bSkip )
                    continue;
            }

/* -------------------------------------------------------------------- */
/*      Get layer extents, and create a corresponding polygon           */
/*      geometry.                                                       */
/* -------------------------------------------------------------------- */
            OGREnvelope sExtents;

            if( poLayer->GetExtent( &sExtents, TRUE ) != OGRERR_NONE )
            {
                fprintf(log,
                        "GetExtent() failed on layer %s of %s, skipping.\n",
                        poLayer->GetLayerDefn()->GetName(),
#ifdef ZOO_SERVICE
			tmpMap->value
#else		     
			papszArgv[nFirstSourceDataset]
#endif
			);
                continue;
            }

            OGRLinearRing oRing;
            oRing.addPoint(sExtents.MinX, sExtents.MinY);
            oRing.addPoint(sExtents.MinX, sExtents.MaxY);
            oRing.addPoint(sExtents.MaxX, sExtents.MaxY);
            oRing.addPoint(sExtents.MaxX, sExtents.MinY);
            oRing.addPoint(sExtents.MinX, sExtents.MinY);

            OGRPolygon oRegion;
            oRegion.addRing(&oRing);

            // If set target srs, do the forward transformation of all points.
            if( bSetTargetSRS && spatialRef != nullptr )
            {
                OGRCoordinateTransformation* poCT = nullptr;
                if( !spatialRef->IsSame(poTargetSRS) )
                {
                    poCT = OGRCreateCoordinateTransformation(spatialRef,
                                                             poTargetSRS);
                    if( poCT == nullptr ||
                        oRegion.transform(poCT) == OGRERR_FAILURE )
                    {
                        char* pszSourceWKT = nullptr;
                        spatialRef->exportToWkt(&pszSourceWKT);
                        fprintf(
                            log,
                            "Warning : unable to transform points from source "
                            "SRS `%s' to target SRS `%s'\n"
                            "for file `%s' - file skipped\n",
                            pszSourceWKT, pszTargetSRS,
#ifdef ZOO_SERVICE
			    tmpMap->value
#else		     
			    papszArgv[nFirstSourceDataset]
#endif
				);
                        CPLFree(pszSourceWKT);
                        delete poCT;
                        continue;
                    }
                    delete poCT;
                }
            }

/* -------------------------------------------------------------------- */
/*      Add layer to tileindex.                                         */
/* -------------------------------------------------------------------- */
            OGRFeature oTileFeat(poDstLayer->GetLayerDefn());

            // TODO(schwehr): Move this off of the stack.
            char szLocation[5000] = {};
            snprintf(szLocation, sizeof(szLocation), "%s,%d",
                     fileNameToWrite, iLayer);
            oTileFeat.SetGeometry(&oRegion);
            oTileFeat.SetField(iTileIndexField, szLocation);

            if( i_SrcSRSName >= 0 && spatialRef != nullptr )
            {
                const char* pszAuthorityCode =
                    spatialRef->GetAuthorityCode(nullptr);
                const char* pszAuthorityName =
                    spatialRef->GetAuthorityName(nullptr);
                char* pszWKT = nullptr;
                spatialRef->exportToWkt(&pszWKT);
                if( eSrcSRSFormat == FORMAT_AUTO )
                {
                    if( pszAuthorityName != nullptr &&
                        pszAuthorityCode != nullptr )
                    {
                        oTileFeat.SetField(i_SrcSRSName,
                            CPLSPrintf("%s:%s",
                                       pszAuthorityName, pszAuthorityCode));
                    }
                    else if( nMaxFieldSize == 0 ||
                            strlen(pszWKT) <= nMaxFieldSize )
                    {
                        oTileFeat.SetField(i_SrcSRSName, pszWKT);
                    }
                    else
                    {
                        char* pszProj4 = nullptr;
                        if( spatialRef->exportToProj4(&pszProj4) ==
                              OGRERR_NONE )
                        {
                            oTileFeat.SetField(i_SrcSRSName, pszProj4);
                            CPLFree(pszProj4);
                        }
                        else
                        {
                            oTileFeat.SetField(i_SrcSRSName, pszWKT);
                        }
                    }
                }
                else if( eSrcSRSFormat == FORMAT_WKT )
                {
                    if( nMaxFieldSize == 0 ||
                        strlen(pszWKT) <= nMaxFieldSize )
                    {
                        oTileFeat.SetField(i_SrcSRSName, pszWKT);
                    }
                    else
                    {
                        fprintf(
                            log,
                            "Cannot write WKT for file %s as it is too long!\n",
                            fileNameToWrite);
                    }
                }
                else if( eSrcSRSFormat == FORMAT_PROJ )
                {
                    char* pszProj4 = nullptr;
                    if( spatialRef->exportToProj4(&pszProj4) == OGRERR_NONE )
                    {
                        oTileFeat.SetField(i_SrcSRSName, pszProj4);
                        CPLFree(pszProj4);
                    }
                }
                else if( eSrcSRSFormat == FORMAT_EPSG )
                {
                    if( pszAuthorityName != nullptr &&
                        pszAuthorityCode != nullptr )
                        oTileFeat.SetField(i_SrcSRSName,
                            CPLSPrintf("%s:%s",
                                       pszAuthorityName, pszAuthorityCode));
                }
                CPLFree(pszWKT);
            }
            if( poDstLayer->CreateFeature(&oTileFeat) != OGRERR_NONE )
            {
                fprintf(log,
                        "Failed to create feature on tile index. "
                        "Terminating.");
                GDALClose(poDstDS);
                exit(1);
            }
        }

/* -------------------------------------------------------------------- */
/*      Cleanup this data source.                                       */
/* -------------------------------------------------------------------- */
        CPLFree(fileNameToWrite);
        GDALClose(poDS);
    }

/* -------------------------------------------------------------------- */
/*      Close tile index and clear buffers.                             */
/* -------------------------------------------------------------------- */
    GDALClose(poDstDS);
    OGRFeatureDefn::DestroyFeatureDefn(poFeatureDefn);

    if( alreadyExistingSpatialRef != nullptr )
        alreadyExistingSpatialRef->Release();
    delete poTargetSRS;

    CPLFree(current_path);

    if( nExistingLayers )
    {
        for( int i = 0; i < nExistingLayers; i++ )
        {
            CPLFree(existingLayersTab[i]);
        }
        CPLFree(existingLayersTab);
    }

    OGRCleanupAll();
#ifdef ZOO_SERVICE
    //setMapInMaps(outputs,"Result","generated_file",pszOutputName);
    setMapInMaps(outputs,"Result","value",pszOutputName);
    return  SERVICE_SUCCEEDED;
#else
    return 0;
#endif
}
#ifndef ZOO_SERVICE
MAIN_END
#endif
  
#ifdef ZOO_SERVICE
}
#endif
