/******************************************************************************
 *
 * Project:  GDAL Utilities
 * Purpose:  Command line application to convert a multidimensional raster
 * Author:   Even Rouault,<even.rouault at spatialys.com>
 *
 * ****************************************************************************
 * Copyright (c) 2019, Even Rouault <even.rouault at spatialys.com>
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

#include "cpl_string.h"
#include "gdal_version.h"
#include "commonutils.h"
#include "gdal_utils_priv.h"
#include "gdal_priv.h"
#include "service.h"
#include "service_internal_gdal.h"

extern "C" {

    /************************************************************************/
    /*                                main()                                */
    /************************************************************************/

    int gdalmdimtranslate(maps*& pmsConf, maps*& pmsInputs, maps*& pmsOutputs)
    {
        int argc = 0;
        char **argv = nullptr;
        GdalZOOServiceInit("gdalmdimtranslate",&argc,&argv,pmsConf,pmsInputs,pmsOutputs);
        EarlySetConfigOptions(argc, argv);

        /* -------------------------------------------------------------------- */
        /*      Generic arg processing.                                         */
        /* -------------------------------------------------------------------- */
        GDALAllRegister();
        argc = GDALGeneralCmdLineProcessor(argc, &argv, 0);
        if (argc < 1){
            return SERVICE_FAILED;
        }

        GDALMultiDimTranslateOptionsForBinary sOptionsForBinary;
        // coverity[tainted_data]
        GDALMultiDimTranslateOptions *psOptions =
            GDALMultiDimTranslateOptionsNew(argv + 1, &sOptionsForBinary);
        CSLDestroy(argv);

        if (psOptions == nullptr)
        {
            return SERVICE_FAILED;
        }

        if (!(sOptionsForBinary.bQuiet))
        {
            GDALMultiDimTranslateOptionsSetProgress(psOptions, GDALTermProgress,
                                                    nullptr);
        }

        if (sOptionsForBinary.osSource.empty()){
            setMapInMaps(pmsConf,"lenv","message","No input file specified.");
            return SERVICE_FAILED;
        }

        if (sOptionsForBinary.osDest.empty()){
            setMapInMaps(pmsConf,"lenv","message","No output file specified.");
            return SERVICE_FAILED;
        }

        /* -------------------------------------------------------------------- */
        /*      Open input file.                                                */
        /* -------------------------------------------------------------------- */
        GDALDatasetH hInDS = GDALOpenEx(
            sOptionsForBinary.osSource.c_str(),
            GDAL_OF_RASTER | GDAL_OF_MULTIDIM_RASTER | GDAL_OF_VERBOSE_ERROR,
            sOptionsForBinary.aosAllowInputDrivers.List(),
            sOptionsForBinary.aosOpenOptions.List(), nullptr);

        if (hInDS == nullptr){
            setMapInMaps(pmsConf,"lenv","message","Unable to open input file.");
            return SERVICE_FAILED;
        }

        /* -------------------------------------------------------------------- */
        /*      Open output file if in update mode.                             */
        /* -------------------------------------------------------------------- */
        GDALDatasetH hDstDS = nullptr;
        if (sOptionsForBinary.bUpdate)
        {
            CPLPushErrorHandler(CPLQuietErrorHandler);
            hDstDS = GDALOpenEx(sOptionsForBinary.osDest.c_str(),
                                GDAL_OF_RASTER | GDAL_OF_MULTIDIM_RASTER |
                                    GDAL_OF_VERBOSE_ERROR | GDAL_OF_UPDATE,
                                nullptr, nullptr, nullptr);
            CPLPopErrorHandler();
        }

        int bUsageError = FALSE;
        GDALDatasetH hRetDS =
            GDALMultiDimTranslate(sOptionsForBinary.osDest.c_str(), hDstDS, 1,
                                &hInDS, psOptions, &bUsageError);
        if (bUsageError == TRUE){
            setMapInMaps(pmsConf,"lenv","message","Usage error.");
            return SERVICE_FAILED;
        }
        int nRetCode = hRetDS ? 0 : 1;
        GDALClose(hRetDS);

        char* pcaResult=(char*) malloc((35+10)*sizeof(char));
        sprintf(pcaResult,"Dataset successfully converted (%d)",nRetCode);
        setMapInMaps(pmsOutputs,"Result","value",pcaResult);
        free(pcaResult);

        GDALClose(hInDS);
        GDALMultiDimTranslateOptionsFree(psOptions);

        GDALDestroyDriverManager();

        return SERVICE_SUCCEEDED;
    }
}