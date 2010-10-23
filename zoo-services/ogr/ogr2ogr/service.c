/******************************************************************************
 * $Id: ogr2ogr.cpp 15473 2008-10-07 20:59:24Z warmerdam $
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Simple client for translating between formats.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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

#include "ogrsf_frmts.h"
#include "ogr_p.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include "ogr_api.h"
#ifdef ZOO_SERVICE
#include "service.h"
#endif

CPL_CVSID("$Id: ogr2ogr.cpp 15473 2008-10-07 20:59:24Z warmerdam $");

#ifdef ZOO_SERVICE
extern "C" {
#endif

static void Usage();

static int TranslateLayer( OGRDataSource *poSrcDS, 
                           OGRLayer * poSrcLayer,
                           OGRDataSource *poDstDS,
                           char ** papszLSCO,
                           const char *pszNewLayerName,
                           int bTransform, 
                           OGRSpatialReference *poOutputSRS,
                           OGRSpatialReference *poSourceSRS,
                           char **papszSelFields,
                           int bAppend, int eGType,
                           int bOverwrite,
                           double dfMaxSegmentLength);

static int bSkipFailures = FALSE;
static int nGroupTransactions = 200;
static int bPreserveFID = FALSE;
static int nFIDToFetch = OGRNullFID;

/************************************************************************/
/*                                main()                                */
/************************************************************************/

#ifdef ZOO_SERVICE
#ifdef WIN32
__declspec(dllexport)
#endif
int Ogr2Ogr(maps*& conf,maps*& inputs,maps*& outputs)
#else
int main( int nArgc, char ** papszArgv )
#endif
{
    const char  *pszFormat = "ESRI Shapefile";
    const char  *pszDataSource = NULL;
    const char  *pszDestDataSource = NULL;
    char        **papszLayers = NULL;
    char        **papszDSCO = NULL, **papszLCO = NULL;
    int         bTransform = FALSE;
    int         bAppend = FALSE, bUpdate = FALSE, bOverwrite = FALSE;
    const char  *pszOutputSRSDef = NULL;
    const char  *pszSourceSRSDef = NULL;
    OGRSpatialReference *poOutputSRS = NULL;
    OGRSpatialReference *poSourceSRS = NULL;
    const char  *pszNewLayerName = NULL;
    const char  *pszWHERE = NULL;
    OGRGeometry *poSpatialFilter = NULL;
    const char  *pszSelect;
    char        **papszSelFields = NULL;
    const char  *pszSQLStatement = NULL;
    int         eGType = -2;
    double       dfMaxSegmentLength = 0;

    /* Check strict compilation and runtime library version as we use C++ API */
    if (! GDAL_CHECK_VERSION("ogr2ogr"))
#ifdef ZOO_SERVICE
	{
		fprintf(stderr,"Not correct version of the gdal library\n");
		setMapInMaps(conf,"lenv","message","Unable to check gdal version for ogr2ogr_service.zo");
		return SERVICE_FAILED;
	}
#else
        exit(1);
#endif
/* -------------------------------------------------------------------- */
/*      Register format(s).                                             */
/* -------------------------------------------------------------------- */
    OGRRegisterAll();

#ifdef ZOO_SERVICE
    map *tmpMap=NULL;
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
    tmpMap=getMapFromMaps(inputs,"F","value");
    if(tmpMap!=NULL){
      pszFormat=tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"DSCO","value");
    if(tmpMap!=NULL){
	  papszDSCO = CSLAddString(papszDSCO, tmpMap->value );
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"LCO","value");
    if(tmpMap!=NULL){
	  papszLCO = CSLAddString(papszLCO, tmpMap->value );
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"preserve_fid","value");
    if(tmpMap!=NULL){
	  bPreserveFID = TRUE;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"skipfailure","value");
    if(tmpMap!=NULL){
	  bPreserveFID = TRUE;
	  bSkipFailures = TRUE;
	  nGroupTransactions = 1; /* #2409 */
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"append","value");
    if(tmpMap!=NULL){
	  bAppend = TRUE;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"overwrite","value");
    if(tmpMap!=NULL){
	  bOverwrite = TRUE;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"update","value");
    if(tmpMap!=NULL){
	  bUpdate = TRUE;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"fid","value");
    if(tmpMap!=NULL){
	  nFIDToFetch = atoi(tmpMap->value);
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"sql","value");
    if(tmpMap!=NULL){
	  pszSQLStatement = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"nln","value");
    if(tmpMap!=NULL){
	  pszNewLayerName = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"nlt","value");
    if(tmpMap!=NULL){
	  pszNewLayerName = tmpMap->value;
	  if( EQUAL(tmpMap->value,"NONE") )
		  eGType = wkbNone;
	  else if( EQUAL(tmpMap->value,"GEOMETRY") )
		  eGType = wkbUnknown;
	  else if( EQUAL(tmpMap->value,"POINT") )
		  eGType = wkbPoint;
	  else if( EQUAL(tmpMap->value,"LINESTRING") )
		  eGType = wkbLineString;
	  else if( EQUAL(tmpMap->value,"POLYGON") )
		  eGType = wkbPolygon;
	  else if( EQUAL(tmpMap->value,"GEOMETRYCOLLECTION") )
		  eGType = wkbGeometryCollection;
	  else if( EQUAL(tmpMap->value,"MULTIPOINT") )
		  eGType = wkbMultiPoint;
	  else if( EQUAL(tmpMap->value,"MULTILINESTRING") )
		  eGType = wkbMultiLineString;
	  else if( EQUAL(tmpMap->value,"MULTIPOLYGON") )
		  eGType = wkbMultiPolygon;
	  else if( EQUAL(tmpMap->value,"GEOMETRY25D") )
		  eGType = wkbUnknown | wkb25DBit;
	  else if( EQUAL(tmpMap->value,"POINT25D") )
		  eGType = wkbPoint25D;
	  else if( EQUAL(tmpMap->value,"LINESTRING25D") )
		  eGType = wkbLineString25D;
	  else if( EQUAL(tmpMap->value,"POLYGON25D") )
		  eGType = wkbPolygon25D;
	  else if( EQUAL(tmpMap->value,"GEOMETRYCOLLECTION25D") )
		  eGType = wkbGeometryCollection25D;
	  else if( EQUAL(tmpMap->value,"MULTIPOINT25D") )
		  eGType = wkbMultiPoint25D;
	  else if( EQUAL(tmpMap->value,"MULTILINESTRING25D") )
		  eGType = wkbMultiLineString25D;
	  else if( EQUAL(tmpMap->value,"MULTIPOLYGON25D") )
		  eGType = wkbMultiPolygon25D;
	  else	  
	  {
		  fprintf( stderr, "-nlt %s: type not recognised.\n", 
			  tmpMap->value );
		  exit( 1 );
	  }
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"tg","value");
    if(tmpMap!=NULL){
	  nGroupTransactions = atoi(tmpMap->value);
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"s_srs","value");
    if(tmpMap!=NULL){
	  pszSourceSRSDef = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"a_srs","value");
    if(tmpMap!=NULL){
	  pszOutputSRSDef = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"t_srs","value");
    if(tmpMap!=NULL){
	  pszOutputSRSDef = tmpMap->value;
	  bTransform = TRUE;
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

      OGRLinearRing  oRing;
      
      oRing.addPoint( dfULX, dfULY );
      oRing.addPoint( dfULX, dfLRY );
      oRing.addPoint( dfLRX, dfLRY );
      oRing.addPoint( dfLRX, dfULY );
      oRing.addPoint( dfULX, dfULY );
      poSpatialFilter = new OGRPolygon();
      ((OGRPolygon *) poSpatialFilter)->addRing( &oRing );
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"where","value");
    if(tmpMap!=NULL){
	  pszWHERE = tmpMap->value;
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"select","value");
    if(tmpMap!=NULL){
	  pszSelect = tmpMap->value;
	  papszSelFields = CSLTokenizeStringComplex(pszSelect, " ,", 
		  FALSE, FALSE );
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"segmentize","value");
    if(tmpMap!=NULL){
	  dfMaxSegmentLength = atof(tmpMap->value);
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"segmentize","value");
    if(tmpMap!=NULL){
	  dfMaxSegmentLength = atof(tmpMap->value);
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"InputDSN","value");
    if(tmpMap!=NULL){
      pszDataSource=(char*)malloc(sizeof(char)*(strlen(dataPath)+strlen(tmpMap->value)+1));
      sprintf((char*)pszDataSource,"%s/%s",dataPath,tmpMap->value);
    }

    tmpMap=NULL;
    tmpMap=getMapFromMaps(inputs,"OutputDSN","value");
    if(tmpMap!=NULL){
      pszDestDataSource=(char*)malloc(sizeof(char)*(strlen(tempPath)+strlen(tmpMap->value)+4));
      sprintf((char*)pszDestDataSource,"%s/%s",tempPath,tmpMap->value/*,ext*/);
    }

#else
/* -------------------------------------------------------------------- */
/*      Processing command line arguments.                              */
/* -------------------------------------------------------------------- */
    nArgc = OGRGeneralCmdLineProcessor( nArgc, &papszArgv, 0 );
    
    if( nArgc < 1 )
        exit( -nArgc );

    for( int iArg = 1; iArg < nArgc; iArg++ )
    {
        if( EQUAL(papszArgv[iArg], "--utility_version") )
        {
				printf("%s was compiled against GDAL %s and is running against GDAL %s\n",
                   papszArgv[0], GDAL_RELEASE_NAME, GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        else if( EQUAL(papszArgv[iArg],"-f") && iArg < nArgc-1 )
        {
            pszFormat = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-dsco") && iArg < nArgc-1 )
        {
            papszDSCO = CSLAddString(papszDSCO, papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-lco") && iArg < nArgc-1 )
        {
            papszLCO = CSLAddString(papszLCO, papszArgv[++iArg] );
        }
        else if( EQUAL(papszArgv[iArg],"-preserve_fid") )
        {
            bPreserveFID = TRUE;
        }
        else if( EQUALN(papszArgv[iArg],"-skip",5) )
        {
            bSkipFailures = TRUE;
            nGroupTransactions = 1; /* #2409 */
        }
        else if( EQUAL(papszArgv[iArg],"-append") )
        {
            bAppend = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-overwrite") )
        {
            bOverwrite = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-update") )
        {
            bUpdate = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-fid") && papszArgv[iArg+1] != NULL )
        {
            nFIDToFetch = atoi(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-sql") && papszArgv[iArg+1] != NULL )
        {
            pszSQLStatement = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-nln") && iArg < nArgc-1 )
        {
            pszNewLayerName = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-nlt") && iArg < nArgc-1 )
        {
            if( EQUAL(papszArgv[iArg+1],"NONE") )
                eGType = wkbNone;
            else if( EQUAL(papszArgv[iArg+1],"GEOMETRY") )
                eGType = wkbUnknown;
            else if( EQUAL(papszArgv[iArg+1],"POINT") )
                eGType = wkbPoint;
            else if( EQUAL(papszArgv[iArg+1],"LINESTRING") )
                eGType = wkbLineString;
            else if( EQUAL(papszArgv[iArg+1],"POLYGON") )
                eGType = wkbPolygon;
            else if( EQUAL(papszArgv[iArg+1],"GEOMETRYCOLLECTION") )
                eGType = wkbGeometryCollection;
            else if( EQUAL(papszArgv[iArg+1],"MULTIPOINT") )
                eGType = wkbMultiPoint;
            else if( EQUAL(papszArgv[iArg+1],"MULTILINESTRING") )
                eGType = wkbMultiLineString;
            else if( EQUAL(papszArgv[iArg+1],"MULTIPOLYGON") )
                eGType = wkbMultiPolygon;
            else if( EQUAL(papszArgv[iArg+1],"GEOMETRY25D") )
                eGType = wkbUnknown | wkb25DBit;
            else if( EQUAL(papszArgv[iArg+1],"POINT25D") )
                eGType = wkbPoint25D;
            else if( EQUAL(papszArgv[iArg+1],"LINESTRING25D") )
                eGType = wkbLineString25D;
            else if( EQUAL(papszArgv[iArg+1],"POLYGON25D") )
                eGType = wkbPolygon25D;
            else if( EQUAL(papszArgv[iArg+1],"GEOMETRYCOLLECTION25D") )
                eGType = wkbGeometryCollection25D;
            else if( EQUAL(papszArgv[iArg+1],"MULTIPOINT25D") )
                eGType = wkbMultiPoint25D;
            else if( EQUAL(papszArgv[iArg+1],"MULTILINESTRING25D") )
                eGType = wkbMultiLineString25D;
            else if( EQUAL(papszArgv[iArg+1],"MULTIPOLYGON25D") )
                eGType = wkbMultiPolygon25D;
            else
            {
                fprintf( stderr, "-nlt %s: type not recognised.\n", 
                         papszArgv[iArg+1] );
                exit( 1 );
            }
            iArg++;
        }
        else if( (EQUAL(papszArgv[iArg],"-tg") ||
                  EQUAL(papszArgv[iArg],"-gt")) && iArg < nArgc-1 )
        {
            nGroupTransactions = atoi(papszArgv[++iArg]);
        }
        else if( EQUAL(papszArgv[iArg],"-s_srs") && iArg < nArgc-1 )
        {
            pszSourceSRSDef = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-a_srs") && iArg < nArgc-1 )
        {
            pszOutputSRSDef = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-t_srs") && iArg < nArgc-1 )
        {
            pszOutputSRSDef = papszArgv[++iArg];
            bTransform = TRUE;
        }
        else if( EQUAL(papszArgv[iArg],"-spat") 
                 && papszArgv[iArg+1] != NULL 
                 && papszArgv[iArg+2] != NULL 
                 && papszArgv[iArg+3] != NULL 
                 && papszArgv[iArg+4] != NULL )
        {
            OGRLinearRing  oRing;

            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+2]) );
            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+4]) );
            oRing.addPoint( atof(papszArgv[iArg+3]), atof(papszArgv[iArg+4]) );
            oRing.addPoint( atof(papszArgv[iArg+3]), atof(papszArgv[iArg+2]) );
            oRing.addPoint( atof(papszArgv[iArg+1]), atof(papszArgv[iArg+2]) );

            poSpatialFilter = new OGRPolygon();
            ((OGRPolygon *) poSpatialFilter)->addRing( &oRing );
            iArg += 4;
        }
        else if( EQUAL(papszArgv[iArg],"-where") && papszArgv[iArg+1] != NULL )
        {
            pszWHERE = papszArgv[++iArg];
        }
        else if( EQUAL(papszArgv[iArg],"-select") && papszArgv[iArg+1] != NULL)
        {
            pszSelect = papszArgv[++iArg];
            papszSelFields = CSLTokenizeStringComplex(pszSelect, " ,", 
                                                      FALSE, FALSE );
        }
        else if( EQUAL(papszArgv[iArg],"-segmentize") && iArg < nArgc-1 )
        {
            dfMaxSegmentLength = atof(papszArgv[++iArg]);
        }
        else if( papszArgv[iArg][0] == '-' )
        {
            Usage();
        }
        else if( pszDestDataSource == NULL )
            pszDestDataSource = papszArgv[iArg];
        else if( pszDataSource == NULL )
            pszDataSource = papszArgv[iArg];
        else
            papszLayers = CSLAddString( papszLayers, papszArgv[iArg] );
    }
#endif

    if( pszDataSource == NULL )
#ifdef ZOO_SERVICE
	{
#endif
	  Usage();
#ifdef ZOO_SERVICE
	  setMapInMaps(conf,"lenv","message","Wrong parameter");
	  return SERVICE_FAILED;
	}
#endif

/* -------------------------------------------------------------------- */
/*      Open data source.                                               */
/* -------------------------------------------------------------------- */
    OGRDataSource       *poDS;
        
    poDS = OGRSFDriverRegistrar::Open( pszDataSource, FALSE );

/* -------------------------------------------------------------------- */
/*      Report failure                                                  */
/* -------------------------------------------------------------------- */
    if( poDS == NULL )
    {
        OGRSFDriverRegistrar    *poR = OGRSFDriverRegistrar::GetRegistrar();
        
        fprintf( stderr, "FAILURE:\n"
                "Unable to open datasource `%s' with the following drivers.\n",
                pszDataSource );

        for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
        {
            fprintf( stderr, "  -> %s\n", poR->GetDriver(iDriver)->GetName() );
        }
#ifdef ZOO_SERVICE
	char tmp[1024];
	sprintf(tmp,"Unable to open datasource `%s' with the following drivers.",pszDataSource);
	setMapInMaps(conf,"lenv","message",tmp);
	return SERVICE_FAILED;
#else
        exit( 1 );
#endif
    }

/* -------------------------------------------------------------------- */
/*      Try opening the output datasource as an existing, writable      */
/* -------------------------------------------------------------------- */
    OGRDataSource       *poODS;
    
    if( bUpdate )
    {
        poODS = OGRSFDriverRegistrar::Open( pszDestDataSource, TRUE );
        if( poODS == NULL )
        {
            fprintf( stderr, "FAILURE:\n"
                    "Unable to open existing output datasource `%s'.\n",
                    pszDestDataSource );
#ifdef ZOO_SERVICE
	    char tmp[1024];
	    sprintf(tmp,"Unable to open existing output datasource `%s'.",pszDestDataSource);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
#else
        exit( 1 );
#endif
        }

        if( CSLCount(papszDSCO) > 0 )
        {
            fprintf( stderr, "WARNING: Datasource creation options ignored since an existing datasource\n"
                    "         being updated.\n" );
        }
    }

/* -------------------------------------------------------------------- */
/*      Find the output driver.                                         */
/* -------------------------------------------------------------------- */
    else
    {
        OGRSFDriverRegistrar *poR = OGRSFDriverRegistrar::GetRegistrar();
        OGRSFDriver          *poDriver = NULL;
        int                  iDriver;

        for( iDriver = 0;
             iDriver < poR->GetDriverCount() && poDriver == NULL;
             iDriver++ )
        {
            if( EQUAL(poR->GetDriver(iDriver)->GetName(),pszFormat) )
            {
                poDriver = poR->GetDriver(iDriver);
            }
        }

        if( poDriver == NULL )
        {
            fprintf( stderr, "Unable to find driver `%s'.\n", pszFormat );
            fprintf( stderr,  "The following drivers are available:\n" );
        
            for( iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
            {
                fprintf( stderr,  "  -> `%s'\n", poR->GetDriver(iDriver)->GetName() );
            }
#ifdef ZOO_SERVICE
	    char tmp[1024];
	    sprintf(tmp,"Unable to find driver `%s'.",pszFormat);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }

        if( !poDriver->TestCapability( ODrCCreateDataSource ) )
        {
            fprintf( stderr,  "%s driver does not support data source creation.\n",
                    pszFormat );
#ifdef ZOO_SERVICE
	    char tmp[1024];
	    sprintf(tmp,"%s driver does not support data source creation.",pszFormat);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }

/* -------------------------------------------------------------------- */
/*      Create the output data source.                                  */
/* -------------------------------------------------------------------- */
        poODS = poDriver->CreateDataSource( pszDestDataSource, papszDSCO );
        if( poODS == NULL )
        {
            fprintf( stderr,  "%s driver failed to create %s\n", 
                    pszFormat, pszDestDataSource );
#ifdef ZOO_SERVICE
	    char tmp[1024];
	    sprintf(tmp,"%s driver failed to create %s",pszFormat, pszDestDataSource);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }
    }

/* -------------------------------------------------------------------- */
/*      Parse the output SRS definition if possible.                    */
/* -------------------------------------------------------------------- */
    if( pszOutputSRSDef != NULL )
    {
        poOutputSRS = new OGRSpatialReference();
        if( poOutputSRS->SetFromUserInput( pszOutputSRSDef ) != OGRERR_NONE )
        {
            fprintf( stderr,  "Failed to process SRS definition: %s\n", 
                    pszOutputSRSDef );
#ifdef ZOO_SERVICE
	    char tmp[1024];
	    sprintf(tmp,"Failed to process SRS definition: %s",pszOutputSRSDef);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }
    }

/* -------------------------------------------------------------------- */
/*      Parse the source SRS definition if possible.                    */
/* -------------------------------------------------------------------- */
    if( pszSourceSRSDef != NULL )
    {
        poSourceSRS = new OGRSpatialReference();
        if( poSourceSRS->SetFromUserInput( pszSourceSRSDef ) != OGRERR_NONE )
        {
            fprintf( stderr,  "Failed to process SRS definition: %s\n", 
                    pszSourceSRSDef );
#ifdef ZOO_SERVICE
	    char tmp[1024];
	    sprintf(tmp,"Failed to process SRS definition: %s",pszOutputSRSDef);
	    setMapInMaps(conf,"lenv","message",tmp);
	    return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }
    }

/* -------------------------------------------------------------------- */
/*      Special case for -sql clause.  No source layers required.       */
/* -------------------------------------------------------------------- */
    if( pszSQLStatement != NULL )
    {
        OGRLayer *poResultSet;

        if( pszWHERE != NULL )
            fprintf( stderr,  "-where clause ignored in combination with -sql.\n" );
        if( CSLCount(papszLayers) > 0 )
            fprintf( stderr,  "layer names ignored in combination with -sql.\n" );
        
        poResultSet = poDS->ExecuteSQL( pszSQLStatement, poSpatialFilter, 
                                        NULL );

        if( poResultSet != NULL )
        {
            if( !TranslateLayer( poDS, poResultSet, poODS, papszLCO, 
                                 pszNewLayerName, bTransform, poOutputSRS,
                                 poSourceSRS, papszSelFields, bAppend, eGType,
                                 bOverwrite, dfMaxSegmentLength ))
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Terminating translation prematurely after failed\n"
                          "translation from sql statement." );

                exit( 1 );
            }
            poDS->ReleaseResultSet( poResultSet );
        }
    }

/* -------------------------------------------------------------------- */
/*      Process each data source layer.                                 */
/* -------------------------------------------------------------------- */
    for( int iLayer = 0; 
         pszSQLStatement == NULL && iLayer < poDS->GetLayerCount(); 
         iLayer++ )
    {
        OGRLayer        *poLayer = poDS->GetLayer(iLayer);

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

        if( CSLCount(papszLayers) == 0
            || CSLFindString( papszLayers,
                              poLayer->GetLayerDefn()->GetName() ) != -1 )
        {
            if( pszWHERE != NULL )
                poLayer->SetAttributeFilter( pszWHERE );
            
            if( poSpatialFilter != NULL )
                poLayer->SetSpatialFilter( poSpatialFilter );
            
            if( !TranslateLayer( poDS, poLayer, poODS, papszLCO, 
                                 pszNewLayerName, bTransform, poOutputSRS,
                                 poSourceSRS, papszSelFields, bAppend, eGType,
                                 bOverwrite, dfMaxSegmentLength ) 
                && !bSkipFailures )
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Terminating translation prematurely after failed\n"
                          "translation of layer %s (use -skipfailures to skip errors)\n", 
                          poLayer->GetLayerDefn()->GetName() );

#ifdef ZOO_SERVICE
		char tmp[1024];
		sprintf(tmp,"Terminating translation prematurely after failed of layer %s",poLayer->GetLayerDefn()->GetName() );
		setMapInMaps(conf,"lenv","message",tmp);
		return SERVICE_FAILED;
#else
                exit( 1 );
#endif
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Close down.                                                     */
/* -------------------------------------------------------------------- */
    delete poOutputSRS;
    delete poSourceSRS;
    delete poODS;
    delete poDS;

    CSLDestroy(papszSelFields);
#ifndef ZOO_SERVICE
	CSLDestroy( papszArgv );
#endif
    CSLDestroy( papszLayers );
    CSLDestroy( papszDSCO );
    CSLDestroy( papszLCO );

    OGRCleanupAll();

#ifdef DBMALLOC
    malloc_dump(1);
#endif
    
#ifdef ZOO_SERVICE
	outputs=(maps*)malloc(sizeof(maps*));
    outputs->name="GeneratedFile";
    outputs->content=createMap("value",(char*)pszDestDataSource);
    addMapToMap(&outputs->content,createMap("dataType","string"));
    outputs->next=NULL;
    return SERVICE_SUCCEEDED;
#else
	return 0;
#endif
}

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    OGRSFDriverRegistrar        *poR = OGRSFDriverRegistrar::GetRegistrar();

#ifdef ZOO_SERVICE
	fprintf(stderr,
#else
	printf(
#endif
		"Usage: ogr2ogr [--help-general] [-skipfailures] [-append] [-update] [-gt n]\n"
		"               [-select field_list] [-where restricted_where] \n"
		"               [-sql <sql statement>] \n" 
		"               [-spat xmin ymin xmax ymax] [-preserve_fid] [-fid FID]\n"
		"               [-a_srs srs_def] [-t_srs srs_def] [-s_srs srs_def]\n"
		"               [-f format_name] [-overwrite] [[-dsco NAME=VALUE] ...]\n"
		"               [-segmentize max_dist]\n"
		"               dst_datasource_name src_datasource_name\n"
		"               [-lco NAME=VALUE] [-nln name] [-nlt type] [layer [layer ...]]\n"
		"\n"
		" -f format_name: output file format name, possible values are:\n");
    
    for( int iDriver = 0; iDriver < poR->GetDriverCount(); iDriver++ )
    {
        OGRSFDriver *poDriver = poR->GetDriver(iDriver);

        if( poDriver->TestCapability( ODrCCreateDataSource ) )
            printf( "     -f \"%s\"\n", poDriver->GetName() );
    }

#ifdef ZOO_SERVICE
	fprintf(stderr,
#else
	printf(
#endif
		" -append: Append to existing layer instead of creating new if it exists\n"
		" -overwrite: delete the output layer and recreate it empty\n"
		" -update: Open existing output datasource in update mode\n"
		" -select field_list: Comma-delimited list of fields from input layer to\n"
		"                     copy to the new layer (defaults to all)\n" 
		" -where restricted_where: Attribute query (like SQL WHERE)\n" 
		" -sql statement: Execute given SQL statement and save result.\n"
		" -skipfailures: skip features or layers that fail to convert\n"
		" -gt n: group n features per transaction (default 200)\n"
		" -spat xmin ymin xmax ymax: spatial query extents\n"
		" -segmentize max_dist: maximum distance between 2 nodes.\n"
		"                       Used to create intermediate points\n"
		" -dsco NAME=VALUE: Dataset creation option (format specific)\n"
		" -lco  NAME=VALUE: Layer creation option (format specific)\n"
		" -nln name: Assign an alternate name to the new layer\n"
		" -nlt type: Force a geometry type for new layer.  One of NONE, GEOMETRY,\n"
		"      POINT, LINESTRING, POLYGON, GEOMETRYCOLLECTION, MULTIPOINT,\n"
		"      MULTIPOLYGON, or MULTILINESTRING.  Add \"25D\" for 3D layers.\n"
		"      Default is type of source layer.\n" );

#ifdef ZOO_SERVICE
	fprintf(stderr,
#else
	printf(
#endif
		" -a_srs srs_def: Assign an output SRS\n"
		" -t_srs srs_def: Reproject/transform to this SRS on output\n"
		" -s_srs srs_def: Override source SRS\n"
		"\n" 
		" Srs_def can be a full WKT definition (hard to escape properly),\n"
		" or a well known definition (ie. EPSG:4326) or a file with a WKT\n"
		" definition.\n" );


#ifndef ZOO_SERVICE
	exit( 1 );
#endif
}

/************************************************************************/
/*                           TranslateLayer()                           */
/************************************************************************/

static int TranslateLayer( OGRDataSource *poSrcDS, 
                           OGRLayer * poSrcLayer,
                           OGRDataSource *poDstDS,
                           char **papszLCO,
                           const char *pszNewLayerName,
                           int bTransform, 
                           OGRSpatialReference *poOutputSRS,
                           OGRSpatialReference *poSourceSRS,
                           char **papszSelFields,
                           int bAppend, int eGType, int bOverwrite,
                           double dfMaxSegmentLength)
		
{
    OGRLayer    *poDstLayer;
    OGRFeatureDefn *poFDefn;
    OGRErr      eErr;
    int         bForceToPolygon = FALSE;
    int         bForceToMultiPolygon = FALSE;

    if( pszNewLayerName == NULL )
        pszNewLayerName = poSrcLayer->GetLayerDefn()->GetName();

    if( wkbFlatten(eGType) == wkbPolygon )
        bForceToPolygon = TRUE;
    else if( wkbFlatten(eGType) == wkbMultiPolygon )
        bForceToMultiPolygon = TRUE;

/* -------------------------------------------------------------------- */
/*      Setup coordinate transformation if we need it.                  */
/* -------------------------------------------------------------------- */
    OGRCoordinateTransformation *poCT = NULL;

    if( bTransform )
    {
        if( poSourceSRS == NULL )
            poSourceSRS = poSrcLayer->GetSpatialRef();

        if( poSourceSRS == NULL )
        {
            fprintf( stderr, "Can't transform coordinates, source layer has no\n"
                    "coordinate system.  Use -s_srs to set one.\n" );
#ifdef ZOO_SERVICE
            return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }

        CPLAssert( NULL != poSourceSRS );
        CPLAssert( NULL != poOutputSRS );

        poCT = OGRCreateCoordinateTransformation( poSourceSRS, poOutputSRS );
        if( poCT == NULL )
        {
            char        *pszWKT = NULL;

            fprintf( stderr, "Failed to create coordinate transformation between the\n"
                   "following coordinate systems.  This may be because they\n"
                   "are not transformable, or because projection services\n"
                   "(PROJ.4 DLL/.so) could not be loaded.\n" );
            
            poSourceSRS->exportToPrettyWkt( &pszWKT, FALSE );
            fprintf( stderr,  "Source:\n%s\n", pszWKT );
            
            poOutputSRS->exportToPrettyWkt( &pszWKT, FALSE );
            fprintf( stderr,  "Target:\n%s\n", pszWKT );
#ifdef ZOO_SERVICE
            return SERVICE_FAILED;
#else
            exit( 1 );
#endif
        }
    }
    
/* -------------------------------------------------------------------- */
/*      Get other info.                                                 */
/* -------------------------------------------------------------------- */
    poFDefn = poSrcLayer->GetLayerDefn();
    
    if( poOutputSRS == NULL )
        poOutputSRS = poSrcLayer->GetSpatialRef();

/* -------------------------------------------------------------------- */
/*      Find the layer.                                                 */
/* -------------------------------------------------------------------- */
    int iLayer = -1;
    poDstLayer = NULL;

    for( iLayer = 0; iLayer < poDstDS->GetLayerCount(); iLayer++ )
    {
        OGRLayer        *poLayer = poDstDS->GetLayer(iLayer);

        if( poLayer != NULL 
            && EQUAL(poLayer->GetLayerDefn()->GetName(),pszNewLayerName) )
        {
            poDstLayer = poLayer;
            break;
        }
    }
    
/* -------------------------------------------------------------------- */
/*      If the user requested overwrite, and we have the layer in       */
/*      question we need to delete it now so it will get recreated      */
/*      (overwritten).                                                  */
/* -------------------------------------------------------------------- */
    if( poDstLayer != NULL && bOverwrite )
    {
        if( poDstDS->DeleteLayer( iLayer ) != OGRERR_NONE )
        {
            fprintf( stderr, 
                     "DeleteLayer() failed when overwrite requested.\n" );
            return FALSE;
        }
        poDstLayer = NULL;
    }

/* -------------------------------------------------------------------- */
/*      If the layer does not exist, then create it.                    */
/* -------------------------------------------------------------------- */
    if( poDstLayer == NULL )
    {
        if( eGType == -2 )
            eGType = poFDefn->GetGeomType();

        if( !poDstDS->TestCapability( ODsCCreateLayer ) )
        {
            fprintf( stderr, 
              "Layer %s not found, and CreateLayer not supported by driver.", 
                     pszNewLayerName );
            return FALSE;
        }

        CPLErrorReset();

        poDstLayer = poDstDS->CreateLayer( pszNewLayerName, poOutputSRS,
                                           (OGRwkbGeometryType) eGType, 
                                           papszLCO );

        if( poDstLayer == NULL )
            return FALSE;

        bAppend = FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise we will append to it, if append was requested.        */
/* -------------------------------------------------------------------- */
    else if( !bAppend )
    {
        fprintf( stderr, "FAILED: Layer %s already exists, and -append not specified.\n"
                "        Consider using -append, or -overwrite.\n",
                pszNewLayerName );
        return FALSE;
    }
    else
    {
        if( CSLCount(papszLCO) > 0 )
        {
            fprintf( stderr, "WARNING: Layer creation options ignored since an existing layer is\n"
                    "         being appended to.\n" );
        }
    }

/* -------------------------------------------------------------------- */
/*      Add fields.  Default to copy all field.                         */
/*      If only a subset of all fields requested, then output only      */
/*      the selected fields, and in the order that they were            */
/*      selected.                                                       */
/* -------------------------------------------------------------------- */
    int         iField;

    if (papszSelFields && !bAppend )
    {
        for( iField=0; papszSelFields[iField] != NULL; iField++)
        {
            int iSrcField = poFDefn->GetFieldIndex(papszSelFields[iField]);
            if (iSrcField >= 0)
                poDstLayer->CreateField( poFDefn->GetFieldDefn(iSrcField) );
            else
            {
                fprintf( stderr, "Field '%s' not found in source layer.\n", 
                        papszSelFields[iField] );
                if( !bSkipFailures )
                    return FALSE;
            }
        }
    }
    else if( !bAppend )
    {
        for( iField = 0; iField < poFDefn->GetFieldCount(); iField++ )
            poDstLayer->CreateField( poFDefn->GetFieldDefn(iField) );
    }

/* -------------------------------------------------------------------- */
/*      Transfer features.                                              */
/* -------------------------------------------------------------------- */
    OGRFeature  *poFeature;
    int         nFeaturesInTransaction = 0;
    
    poSrcLayer->ResetReading();

    if( nGroupTransactions )
        poDstLayer->StartTransaction();

    while( TRUE )
    {
        OGRFeature      *poDstFeature = NULL;

        if( nFIDToFetch != OGRNullFID )
        {
            // Only fetch feature on first pass.
            if( nFeaturesInTransaction == 0 )
                poFeature = poSrcLayer->GetFeature(nFIDToFetch);
            else
                poFeature = NULL;
        }
        else
            poFeature = poSrcLayer->GetNextFeature();
        
        if( poFeature == NULL )
            break;

        if( ++nFeaturesInTransaction == nGroupTransactions )
        {
            poDstLayer->CommitTransaction();
            poDstLayer->StartTransaction();
            nFeaturesInTransaction = 0;
        }

        CPLErrorReset();
        poDstFeature = OGRFeature::CreateFeature( poDstLayer->GetLayerDefn() );

        if( poDstFeature->SetFrom( poFeature, TRUE ) != OGRERR_NONE )
        {
            if( nGroupTransactions )
                poDstLayer->CommitTransaction();
            
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Unable to translate feature %ld from layer %s.\n",
                      poFeature->GetFID(), poFDefn->GetName() );
            
            OGRFeature::DestroyFeature( poFeature );
            OGRFeature::DestroyFeature( poDstFeature );
            return FALSE;
        }

        if( bPreserveFID )
            poDstFeature->SetFID( poFeature->GetFID() );

#ifndef GDAL_1_5_0
        if (poDstFeature->GetGeometryRef() != NULL && dfMaxSegmentLength > 0)
            poDstFeature->GetGeometryRef()->segmentize(dfMaxSegmentLength);
#endif

        if( poCT && poDstFeature->GetGeometryRef() != NULL )
        {
            eErr = poDstFeature->GetGeometryRef()->transform( poCT );
            if( eErr != OGRERR_NONE )
            {
                if( nGroupTransactions )
                    poDstLayer->CommitTransaction();

                fprintf( stderr, "Failed to reproject feature %d (geometry probably out of source or destination SRS).\n", 
                        (int) poFeature->GetFID() );
                if( !bSkipFailures )
                {
                    OGRFeature::DestroyFeature( poFeature );
                    OGRFeature::DestroyFeature( poDstFeature );
                    return FALSE;
                }
            }
        }

        if( poDstFeature->GetGeometryRef() != NULL && bForceToPolygon )
        {
            poDstFeature->SetGeometryDirectly( 
                OGRGeometryFactory::forceToPolygon(
                    poDstFeature->StealGeometry() ) );
        }
                    
        if( poDstFeature->GetGeometryRef() != NULL && bForceToMultiPolygon )
        {
            poDstFeature->SetGeometryDirectly( 
                OGRGeometryFactory::forceToMultiPolygon(
                    poDstFeature->StealGeometry() ) );
        }
                    
        OGRFeature::DestroyFeature( poFeature );

        CPLErrorReset();
        if( poDstLayer->CreateFeature( poDstFeature ) != OGRERR_NONE 
            && !bSkipFailures )
        {
            if( nGroupTransactions )
                poDstLayer->RollbackTransaction();

            OGRFeature::DestroyFeature( poDstFeature );
            return FALSE;
        }

        OGRFeature::DestroyFeature( poDstFeature );
    }

    if( nGroupTransactions )
        poDstLayer->CommitTransaction();

/* -------------------------------------------------------------------- */
/*      Cleaning                                                        */
/* -------------------------------------------------------------------- */
    delete poCT;

    return TRUE;
}

#ifdef ZOO_SERVICE
}
#endif
