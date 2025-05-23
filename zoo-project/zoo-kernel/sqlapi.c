/*
 * Authors : David Saggiorato
 *           Gérald Fenoy
 *  Copyright 2015-2025 GeoLabs SARL. All rights reserved.
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

#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"
#include "response_print.h"
#if GDAL_VERSION_MAJOR >= 2
#include <gdal_priv.h>
#endif

#include <fcgi_stdio.h>
#include "sqlapi.h"
#include "service_callback.h"

/**
 * Global GDALDataset pointer
 */
#if GDAL_VERSION_MAJOR >=2
GDALDataset
#else
OGRDataSource 
#endif
 **ppoZooDS = NULL;

/**
 * Global OGRLayer pointer pointing to the lastest result set
 */
OGRLayer *poZooResultSet = NULL;

/**
 * Create a GDAL / OGR string for connecting to a db backend defined in the
 * key section.
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pccKey the name of the section containing the connection setting
 * @return the OGR connection string
 */
char* _createInitString(maps* pmsConf,const char* pccKey){
  char* pcaRes=NULL;
  int iCnt=0;
  maps* pmsCurrentConf=getMaps(pmsConf,pccKey);
  if(pmsCurrentConf==NULL){
    return zStrdup("-1");
  }
  char aacKeywords[6][14]={
    "dbname",
    "host",
    "port",
    "user",
    "password",
    "active_schema"    
  };
  char aacEnvVars[6][14]={
    "PGDATABASE",
    "PGHOST",
    "PGPORT",
    "PGUSER",
    "PGPASSWORD",
    "PGSCHEMA"
  };
  int iLen=0;
  for(iCnt=0;iCnt<6;iCnt++){
    map* pmTmp0=getMap(pmsCurrentConf->content,aacKeywords[iCnt]);
    if(pmTmp0!=NULL){
      map* pmTmp=NULL;
      if(pcaRes==NULL){
        if((pmTmp=getMapFromMaps(pmsConf,"renv",aacEnvVars[iCnt]))!=NULL) {
          pcaRes=(char*)malloc((strlen(aacKeywords[iCnt])+strlen(pmTmp->value)+4)*sizeof(char));
          sprintf(pcaRes,"%s='%s'",aacKeywords[iCnt],pmTmp->value);
          iLen+=strlen(pcaRes);
        }
        else{
          pcaRes=(char*)malloc((strlen(aacKeywords[iCnt])+strlen(pmTmp0->value)+4)*sizeof(char));
          sprintf(pcaRes,"%s='%s'",aacKeywords[iCnt],pmTmp0->value);
          iLen+=strlen(pcaRes);
        }
      }else{
        if((pmTmp=getMapFromMaps(pmsConf,"renv",aacEnvVars[iCnt]))!=NULL) {
          char* pcaRes1=(char*)malloc((strlen(aacKeywords[iCnt])+strlen(pmTmp->value)+5)*sizeof(char));
          sprintf(pcaRes1," %s='%s'",aacKeywords[iCnt],pmTmp->value);
          pcaRes=(char*)realloc(pcaRes,(iLen+strlen(aacKeywords[iCnt])+strlen(pmTmp->value)+5)*sizeof(char));
          memcpy(pcaRes+iLen,pcaRes1,(strlen(aacKeywords[iCnt])+strlen(pmTmp->value)+5)*sizeof(char));
          iLen+=strlen(pcaRes1);
          pcaRes[iLen]=0;
          free(pcaRes1);
        }
        else{
          char* pcaRes1=(char*)malloc((strlen(aacKeywords[iCnt])+strlen(pmTmp0->value)+5)*sizeof(char));
          sprintf(pcaRes1," %s='%s'",aacKeywords[iCnt],pmTmp0->value);
          pcaRes=(char*)realloc(pcaRes,(iLen+strlen(aacKeywords[iCnt])+strlen(pmTmp0->value)+5)*sizeof(char));
          memcpy(pcaRes+iLen,pcaRes1,(strlen(aacKeywords[iCnt])+strlen(pmTmp0->value)+5)*sizeof(char));
          iLen+=strlen(pcaRes1);
          pcaRes[iLen]=0;
          free(pcaRes1);
        }
      }
    }
  }
  map* pmTmp=getMap(pmsCurrentConf->content,"type");
  if(pmTmp!=NULL){
    char* pcaConnection=(char*)malloc((strlen(pcaRes)+strlen(pmTmp->value)+2)*sizeof(char));
    sprintf(pcaConnection,"%s:%s",pmTmp->value,pcaRes);
    free(pcaRes);
    return pcaConnection;
  }
  return pcaRes;
}

/**
 * Create a GDAL / OGR string for connecting to the db backend
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @return the OGR connection string
 */
char* createInitString(maps* pmsConf){
  return _createInitString(pmsConf,"database");
}

/**
 * Connect to a db backend.
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pccKey the name of the section containing the connection setting
 * @see createInitString
 */
int _init_sql(maps* pmsConf,const char* pccKey){
  char* pcaSqlInitString=_createInitString(pmsConf,pccKey);
#ifdef SQL_DEBUG
  fprintf(stderr,"Try to connect to: %s %s !\n",pccKey,pcaSqlInitString);
  fflush(stderr);  
#endif
  if(strncmp(pcaSqlInitString,"-1",2)==0)
    return -1;
  OGRSFDriver *poDriver = NULL;
  OGRRegisterAll();
  int iZooDsNb=0;
  map* dsNb=getMapFromMaps(pmsConf,"lenv","ds_nb");
  if(dsNb==NULL){
    setMapInMaps(pmsConf,"lenv","ds_nb","1");
  }else{
    iZooDsNb=atoi(dsNb->value);
    char* pcaTmp=(char*)malloc(12*sizeof(char));
    sprintf(pcaTmp,"%d",iZooDsNb+1);
    setMapInMaps(pmsConf,"lenv","ds_nb",(const char*)pcaTmp);
    free(pcaTmp);
  }
  if(ppoZooDS==NULL)
    ppoZooDS=
#if GDAL_VERSION_MAJOR >= 2
      (GDALDataset**) malloc(sizeof(GDALDataset*))
#else
      (OGRDataSource**) malloc(sizeof(OGRDataSource*))
#endif
      ;
  else
    ppoZooDS=      
#if GDAL_VERSION_MAJOR >= 2
      (GDALDataset**)realloc(ppoZooDS,(iZooDsNb+1)*sizeof(GDALDataset*))
#else
      (OGRDataSource**)realloc(ppoZooDS,(iZooDsNb+1)*sizeof(OGRDataSource*))
#endif
      ;
  
#if GDAL_VERSION_MAJOR >= 2
  ppoZooDS[iZooDsNb] = (GDALDataset*) GDALOpenEx( pcaSqlInitString,
				      GDAL_OF_UPDATE | GDAL_OF_VECTOR,
				      NULL, NULL, NULL );
#else
  ppoZooDS[iZooDsNb] = OGRSFDriverRegistrar::Open(pcaSqlInitString,false,&poDriver);
#endif
  if( ppoZooDS[iZooDsNb] == NULL ){
#ifdef SQL_DEBUG
    fprintf(stderr,"pcaSqlInitString: %s FAILED !\n",pcaSqlInitString);
    fflush(stderr);
#endif
    free(pcaSqlInitString);
    setMapInMaps(pmsConf,"lenv","dbIssue","1");
    setMapInMaps(pmsConf,"lenv","message",_("Failed to connect to the database backend"));
    return -2;
  }
#ifdef SQL_DEBUG
  fprintf(stderr,"pcaSqlInitString: %s SUCEED !\n",pcaSqlInitString);
  fflush(stderr);
#endif
  free(pcaSqlInitString);
  iZooDsNb++;
  return iZooDsNb;
}

/**
 * Connect to the db backend.
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @see createInitString
 */
int init_sql(maps* pmsConf){
  return _init_sql(pmsConf,"database");
}

/**
 * Close any connection to the db backend.
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param iConnectionId the connection identifier
 */
void close_sql(maps* pmsConf,int iConnectionId){
  if(iConnectionId<0)
    return;
  if( poZooResultSet != NULL ){
    ppoZooDS[iConnectionId]->ReleaseResultSet( poZooResultSet );
    poZooResultSet=NULL;
  }
  if(ppoZooDS!=NULL && ppoZooDS[iConnectionId]!=NULL){
#if GDAL_VERSION_MAJOR >= 2
    GDALClose(ppoZooDS[iConnectionId]);
#else
    OGRDataSource::DestroyDataSource( ppoZooDS[iConnectionId] );
#endif
    ppoZooDS[iConnectionId]=NULL;
    int iZooDsNb=0;
    map* dsNb=getMapFromMaps(pmsConf,"lenv","ds_nb");
    if(dsNb!=NULL){
      iZooDsNb=atoi(dsNb->value);
      char* pcaTmp=(char*)malloc(12*sizeof(char));
      if(iZooDsNb>0)
        sprintf(pcaTmp,"%d",iZooDsNb-1);
      else
        sprintf(pcaTmp,"%d",0);
      setMapInMaps(pmsConf,"lenv","ds_nb",(const char*)pcaTmp);
      free(pcaTmp);
    }
  }
}

/**
 * Call OGRCleanupAll.
 * 
 */
void end_sql(){
  OGRCleanupAll();
}

/**
 * Fetch a tuple set by executing a SQL query to the Database Backend.
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcSqlQuery the SQL query to run
 * @return NULL in case of failure or an OGRLayer pointer if the query succeed.
 */
OGRLayer *fetchSql(maps* pmsConf,int iConnectionId,const char* pcSqlQuery){
  if(ppoZooDS==NULL || ppoZooDS[iConnectionId]==NULL)
    return NULL;
  OGRLayer *poResult=NULL;
#ifdef SQL_DEBUG
  fprintf(stderr,"************************* %s %s %d\n\n",pcSqlQuery,__FILE__,__LINE__);
  fflush(stderr);
#endif
  poResult = ppoZooDS[iConnectionId]->ExecuteSQL( pcSqlQuery, NULL, NULL);
  return poResult;
}

/**
 * Clean any memory allocated by executing a request
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param iConnectionId the connection identifier
 * @param poLayer the OGRLayer pointer to clean
 */
void cleanFetchSql(maps* pmsConf,int iConnectionId,OGRLayer *poLayer){
  if( poLayer != NULL ){
    ppoZooDS[iConnectionId]->ReleaseResultSet( poLayer );
    poLayer=NULL;
  }
}

/**
 * Execute a SQL query to the SQL Database Backend.
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcSqlQuery the SQL query to run
 * @return -1 in case of failure and 1 if the query succeed.
 */
int execSql(maps* pmsConf,int iConnectionId,const char* pcSqlQuery){
  int iResult=-1;
  if(ppoZooDS == NULL || ppoZooDS[iConnectionId]==NULL)
    return -1;
  poZooResultSet = ppoZooDS[iConnectionId]->ExecuteSQL( pcSqlQuery, NULL, NULL);
  if( poZooResultSet != NULL ){
    iResult=1;
  }
  return iResult;
}

/**
 * Clean any memory allocated by executing a request
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param iConnectionId the connection identifier
 * @return -1 in case of failure and 1 if the query succeed.
 */
void cleanUpResultSet(const maps* pmsConf,int iConnectionId){
  if( poZooResultSet != NULL ){
    ppoZooDS[iConnectionId]->ReleaseResultSet( poZooResultSet );
    poZooResultSet=NULL;
  }
}

/**
 * Get the identifier of the current database
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 */
int getCurrentId(maps* pmsConf){
  int iResult=0;
  map* pmDsNb=getMapFromMaps(pmsConf,"lenv","ds_nb");
  if(pmDsNb!=NULL)
    iResult=atoi(pmDsNb->value);
  return iResult;
}

#ifdef RELY_ON_DB

/**
 * Record a file stored during ZOO-Kernel execution
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pccFileName the file's name
 * @param pccType the type (Intput,Output,Response)
 * @param pccName the maps containing the setting of the main.cfg file
 */
void recordStoredFile(maps* pmsConf,const char* pccFileName,const char* pccType,const char* pccName){
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmUsid=getMapFromMaps(pmsConf,"lenv","usid");
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pmUsid->value)+strlen(pccFileName)+strlen(pccType)+(pccName!=NULL?strlen(pccName):2)+68+1)*sizeof(char));
  if(pccName!=NULL)
    sprintf(pcaSqlQuery,"INSERT INTO %s.files (uuid,filename,nature,name) VALUES ('%s','%s','%s','%s');",pmSchema->value,pmUsid->value,pccFileName,pccType,pccName);
  else
    sprintf(pcaSqlQuery,"INSERT INTO %s.files (uuid,filename,nature,name) VALUES ('%s','%s','%s',NULL);",pmSchema->value,pmUsid->value,pccFileName,pccType);
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  free(pcaSqlQuery);
  cleanUpResultSet(pmsConf,iZooDsNb-1);
}

/**
 * Run a SQL query
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param query the SQL query to run
 * @return char pointer to the allocated result string
 */
char* runSqlQuery(maps* pmsConf,char* query){
  int iZooDsNb=getCurrentId(pmsConf);
  int iCreated=0;
  if( iZooDsNb == 0 || ppoZooDS == NULL || ppoZooDS[iZooDsNb-1]==NULL ){
    init_sql(pmsConf);
    iCreated=1;
    iZooDsNb++;
  }
  if(execSql(pmsConf,iZooDsNb-1,query)<0)
    return NULL;
  OGRFeature  *poFeature = NULL;
  char *pcaTmp;
  while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
        pcaTmp=zStrdup(poFeature->GetFieldAsString( iField ));
      }
      else
        pcaTmp=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  if(iCreated>0)
    close_sql(pmsConf,iZooDsNb-1);
  return pcaTmp;
}

/**
 * Filter jobs list based on the potential user_id
 *
 * @param pmsConf the conf maps
 * @param pcaClauseFinal the string to update
 * @param pcaClauseDate the string containing the filter by date
 */
void filterJobByUser(maps* pmsConf,char** pcaClauseFinal,char* pcaClauseDate){
  map* pmUserName=getMapFromMaps(pmsConf,"auth_env","user");
  char* pcClause=*pcaClauseFinal;
  if(pmUserName!=NULL){
    char* pcaTmp=NULL;
    if(pcClause!=NULL){
      pcaTmp=zStrdup(pcClause);
    }else
      pcaTmp=zStrdup("true");
    map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
    if(pcClause!=NULL){
      *pcaClauseFinal=(char*)realloc(*pcaClauseFinal,
                                      (strlen(pcaTmp)+
                                      strlen(pmSchema->value)+
                                      strlen(pmUserName->value)+
                                      68)*sizeof(char));
      sprintf(*pcaClauseFinal,"%s AND (user_id=0 or user_id=(SELECT id FROM %s.users WHERE name='%s'))",
        pcaTmp,pmSchema->value,pmUserName->value);
      free(pcaTmp);
    }else{
      *pcaClauseFinal=(char*)malloc((strlen(pmSchema->value)+strlen(pmUserName->value)+68)*sizeof(char));
      sprintf(*pcaClauseFinal,"(user_id=0 or user_id=(SELECT id FROM %s.users WHERE name='%s'))",
        pmSchema->value,pmUserName->value);
    }
  }else{
    if(pcClause!=NULL){
      char* pcaTmp=zStrdup(pcClause);
      *pcaClauseFinal=(char*)realloc(*pcaClauseFinal,
                  (strlen(pcaTmp)+15)*sizeof(char));
      sprintf(*pcaClauseFinal,"%s AND user_id=0",
        pcaTmp);
      free(pcaTmp);
    }else{
      *pcaClauseFinal=(char*)malloc(10*sizeof(char));
      sprintf(*pcaClauseFinal,"user_id=0");
    }
  }
}

/**
 * Run SQL query to fetch the user identifier
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param iZooDsNb the SQL connexion identifier
 * @return pmSchema the map pointing the database schema
 */
int getUserId(maps* pmsConf,int iZooDsNb,map* pmSchema){
  map* pmUserName=getMapFromMaps(pmsConf,"auth_env","user");
  if(pmUserName!=NULL){
    int iCreated=0;
    char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+
				  strlen(pmUserName->value)+
				  36+1)*sizeof(char));
    sprintf(pcaSqlQuery,"SELECT id from %s.users WHERE name='%s'",pmSchema->value,pmUserName->value);
    if( iZooDsNb == 0 ){
      init_sql(pmsConf);
      iCreated=1;
      iZooDsNb++;
    }
    execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
    OGRFeature  *poFeature = NULL;
    char *pcaTmp;
    while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
      for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
        if( poFeature->IsFieldSet( iField ) ){
          pcaTmp=zStrdup(poFeature->GetFieldAsString( iField ));
          OGRFeature::DestroyFeature( poFeature );
          int iResult=atoi(pcaTmp);
          free(pcaTmp);
          cleanUpResultSet(pmsConf,iZooDsNb-1);
          return iResult;
        }
        else
          pcaTmp=NULL;
      }
      OGRFeature::DestroyFeature( poFeature );
    }
    free(pcaSqlQuery);
    cleanUpResultSet(pmsConf,iZooDsNb-1);
    pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+
			    strlen(pmUserName->value)+
			    50+1)*sizeof(char));
    sprintf(pcaSqlQuery,"INSERT INTO %s.users (name) VALUES ('%s') RETURNING id",pmSchema->value,pmUserName->value);
    execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
    poFeature = NULL;
    while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
      for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
        if( poFeature->IsFieldSet( iField ) ){
          pcaTmp=zStrdup(poFeature->GetFieldAsString( iField ));
          OGRFeature::DestroyFeature( poFeature );
          int iResult=atoi(pcaTmp);
          free(pcaTmp);
          cleanUpResultSet(pmsConf,iZooDsNb-1);
          return iResult;
        }
        else
          pcaTmp=NULL;
      }
      OGRFeature::DestroyFeature( poFeature );
    }
    cleanUpResultSet(pmsConf,iZooDsNb-1);
    return 0;
  }
  else
    return 0;
}

/**
 * Insert the reference tuple corresponding to the running service
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 */
void recordServiceStatus(maps* pmsConf){
  int iCreated=0;
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmSid=getMapFromMaps(pmsConf,"lenv","sid");
  map *pmOsid=getMapFromMaps(pmsConf,"lenv","osid");
  map *pmUsid=getMapFromMaps(pmsConf,"lenv","usid");
  map *pmProcessId=getMapFromMaps(pmsConf,"lenv","oIdentifier");
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  map* pmType=getMapFromMaps(pmsConf,"main","executionType");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+
				strlen(pmUsid->value)+
				strlen(pmProcessId->value)+
				strlen(pmType->value)+
				strlen(pmSid->value)+
				strlen(pmOsid->value)+
				strlen(wpsStatus[2])+99+9+1)*sizeof(char));
  sprintf(pcaSqlQuery,
	  "INSERT INTO %s.services (uuid,processid,sid,osid,fstate,itype,user_id) "
	  "VALUES ('%s','%s','%s','%s','%s','%s','%d');",
	  pmSchema->value,
	  pmUsid->value,
	  pmProcessId->value,
	  pmSid->value,
	  pmOsid->value,
	  wpsStatus[2],
	  pmType->value,
	  getUserId(pmsConf,iZooDsNb,pmSchema));
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iCreated=1;
    iZooDsNb++;
  }
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  free(pcaSqlQuery);
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  if(iCreated>1)
    close_sql(pmsConf,iZooDsNb-1);
}

/**
 * Run INSERT query for content and usid in a given table
 * INSERT INTO $schema.%pcTableName (content,usid) VALUE (%CONTENT,$usid)
 * 
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcTableName the table name
 * @param content the content to insert
 * @param len the content length
 */
void recordRequestResponse(maps* pmsConf,const char* pcTableName,const char* content,const int len){
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmUsid=getMapFromMaps(pmsConf,"lenv","usid");
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+len+strlen(pmUsid->value)+strlen(pcTableName)+49+1)*sizeof(char));
  sprintf(pcaSqlQuery,"INSERT INTO %s.%s (content,uuid) VALUES ($$%s$$,$$%s$$);",pmSchema->value,pcTableName,content,pmUsid->value);
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  free(pcaSqlQuery);
  cleanUpResultSet(pmsConf,iZooDsNb-1);
}

/**
 * Store the content of the result file
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pccFileName the file's name
 */
void recordResponse(maps* pmsConf,char* pccFileName){
  FILE *pfFile = fopen (pccFileName, "rb");
  char *pcaTmps = NULL;
  long lFileLen;
  if(pfFile!=NULL){
    fseek (pfFile, 0, SEEK_END);
    lFileLen = ftell (pfFile);
    fseek (pfFile, 0, SEEK_SET);
    pcaTmps = (char *) malloc ((lFileLen + 1) * sizeof (char));
    fread (pcaTmps, lFileLen, 1, pfFile);
    pcaTmps[lFileLen]=0;
    fclose(pfFile);
  }else{
    fprintf(stderr,"Error parsing response file!");
    fflush(stderr);
    return;
  }
  recordRequestResponse(pmsConf,"responses",pcaTmps,lFileLen);
  free(pcaTmps);
}

/**
 * Store the content of the request
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcaInputs the maps containing the request parameters
 */
void recordRequest(maps* pmsConf,map* pcaInputs){
  char *pcaTmps = NULL;
  map* pmRequest=getMap(pcaInputs,"jrequest");
  if(pmRequest!=NULL){
    pcaTmps = zStrdup(pmRequest->value);
  }else{
    pmRequest=getMap(pcaInputs,"xrequest");
    if(pmRequest!=NULL){
      pcaTmps = zStrdup(pmRequest->value);
    }else{
      fprintf(stderr,"Error parsing response file!");
      fflush(stderr);
      return;
    }
  }
  recordRequestResponse(pmsConf,"requests",pcaTmps,strlen(pcaTmps));
  free(pcaTmps);
}

/**
 * Update the current status of the running service.
 *
 * @param pmsConf the map containing the setting of the main.cfg file
 * @return 0 on success, -2 if shmget failed, -1 if shmat failed
 */
int _updateStatus(maps* pmsConf){
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmUsid=getMapFromMaps(pmsConf,"lenv","usid");
  map *pmStatus=getMapFromMaps(pmsConf,"lenv","status");
  map *pmMessage=getMapFromMaps(pmsConf,"lenv","message");
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pmMessage->value)+strlen(pmStatus->value)+strlen(pmUsid->value)+81+1)*sizeof(char));
  sprintf(pcaSqlQuery,"UPDATE %s.services set status=$$%s$$,message=$$%s$$,updated_time=now() where uuid=$$%s$$;",pmSchema->value,pmStatus->value,pmMessage->value,pmUsid->value);
  if( iZooDsNb == 0 ){
    if(getMapFromMaps(pmsConf,"lenv","file.log")==NULL){
      free(pcaSqlQuery);
      return 1;
    }
    init_sql(pmsConf);
    iZooDsNb++;
  }
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  free(pcaSqlQuery);
#ifdef USE_JSON  
  invokeBasicCallback(pmsConf,SERVICE_STARTED);
#endif
  return 0;
}

/**
 * Get the ongoing status of a running service 
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcPid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (MESSAGE|POURCENTAGE)
 */
char* _getStatus(maps* pmsConf,char* pcPid){
  int iZooDsNb=getCurrentId(pmsConf);
  int iCreated=-1;
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+104+1)*sizeof(char));
  sprintf(pcaSqlQuery,"select CASE WHEN message is null THEN '-1' ELSE status||'|'||message END from %s.services where uuid=$$%s$$;",pmSchema->value,pcPid);
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iZooDsNb++;
    iCreated=1;
  }
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  OGRFeature  *poFeature = NULL;
  const char *pccTmp;
  while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
        pccTmp=zStrdup(poFeature->GetFieldAsString( iField ));
      }
      else
        pccTmp=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  free(pcaSqlQuery);
  return (char*)pccTmp;
}


/**
 * Get the ongoing status field of a running service
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcPid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (MESSAGE|POURCENTAGE)
 */
char* _getStatusField(maps* pmsConf,char* pcPid,const char* field){
  int iZooDsNb=getCurrentId(pmsConf);
  int iCreated=-1;
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=NULL;
  if(strstr(field,"_time")!=NULL){
    pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+strlen(field)+strlen(field)+99+1)*sizeof(char));
    sprintf(pcaSqlQuery,"select CASE WHEN %s is null THEN '-1' ELSE display_date_rfc3339(%s) END from %s.services where uuid=$$%s$$;",field,field,pmSchema->value,pcPid);
  }else{
    if(strstr(field,"itype")!=NULL){
      pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+(2*strlen(field))+strlen(field)+137+1)*sizeof(char));
      sprintf(pcaSqlQuery,"select CASE WHEN %s is null  THEN 'unknown' ELSE CASE WHEN %s = 'json' THEN 'process' ELSE 'unknown' END END from %s.services where uuid=$$%s$$;",field,field,pmSchema->value,pcPid);
    }else{
      pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+strlen(field)+strlen(field)+83+1)*sizeof(char));
      sprintf(pcaSqlQuery,"select CASE WHEN %s is null THEN '-1' ELSE %s::text END from %s.services where uuid=$$%s$$;",field,field,pmSchema->value,pcPid);
    }
  }
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iZooDsNb++;
    iCreated=1;
  }
  if(execSql(pmsConf,iZooDsNb-1,pcaSqlQuery)<0)
    return NULL;
  OGRFeature  *poFeature = NULL;
  const char *pccTmp;
  while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
        pccTmp=zStrdup(poFeature->GetFieldAsString( iField ));
      }
      else
        pccTmp=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  free(pcaSqlQuery);
  return (char*)pccTmp;
}
/**
 * Read the cache file of a running service 
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcPid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (temporary/final result)
 */
char* _getStatusFile(maps* pmsConf,char* pcPid){
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  OGRFeature  *poFeature = NULL;
  const char *pccTmp=NULL;
  int iHasRes=-1;
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+82+1)*sizeof(char));
  sprintf(pcaSqlQuery,
	  "select content from %s.responses where uuid=$$%s$$"
	  " order by creation_time desc limit 1",pmSchema->value,pcPid);
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iZooDsNb++;
  }
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  if(poZooResultSet!=NULL){
      while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
        for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
          if( poFeature->IsFieldSet( iField ) ){
            pccTmp=zStrdup(poFeature->GetFieldAsString( iField ));
            iHasRes=1;
          }
          else
            pccTmp=NULL;
        }
        OGRFeature::DestroyFeature( poFeature );
      }
  }
  if(iHasRes<0)
    pccTmp=NULL;
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  free(pcaSqlQuery);
  return (char*)pccTmp;
}

/**
 * Delete a service reference from the database.
 *
 * @param pmsConf the map containing the setting of the main.cfg file
 * @param pcPid the service identifier (usid key from the [lenv] section)
 */
void removeService(maps* pmsConf,char* pcPid){
  int iZooDsNb=getCurrentId(pmsConf);
  int iCreated=0;
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)
    malloc((strlen(pcPid)+strlen(pmSchema->value)+38+1)
	   *sizeof(char));
  sprintf(pcaSqlQuery,
	  "DELETE FROM %s.services where uuid=$$%s$$;",
	  pmSchema->value,pcPid);
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iZooDsNb++;
    iCreated=1;
  }
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  if(iCreated>0)
    close_sql(pmsConf,iZooDsNb-1);
  free(pcaSqlQuery);
  //end_sql();
}

/**
 * Stop handling status repport.
 *
 * @param pmsConf the map containing the setting of the main.cfg file
 */
void unhandleStatus(maps* pmsConf){
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  map *pmUsid=getMapFromMaps(pmsConf,"lenv","usid");
  map *fstate=getMapFromMaps(pmsConf,"lenv","fstate");
  char *pcaSqlQuery=(char*)malloc((strlen(pmUsid->value)+
				strlen(pmSchema->value)+
				(fstate!=NULL?
				 strlen(fstate->value):
				 4)
				+65+1)*sizeof(char));
  sprintf(pcaSqlQuery,
	  "UPDATE %s.services set end_time=now(), fstate=$$%s$$"
	  " where uuid=$$%s$$;",
	  pmSchema->value,(fstate!=NULL?fstate->value:"Failed"),pmUsid->value);
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  //close_sql(pmsConf,iZooDsNb-1);
  free(pcaSqlQuery);
  //end_sql();
}

/**
 * Read the sid identifier attached of a service if any
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcPid the service identifier (usid key from the [lenv] section)
 * @return the sid value
 */
char* getStatusId(maps* pmsConf,char* pcPid){
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+42+1)*sizeof(char));
  sprintf(pcaSqlQuery,
	  "select osid from %s.services where uuid=$$%s$$",
	  pmSchema->value,pcPid);
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iZooDsNb++;
  }
  if(execSql(pmsConf,iZooDsNb-1,pcaSqlQuery)<0)
    return NULL;
  OGRFeature  *poFeature = NULL;
  char *pccTmp;
  int iHasRes=-1;
  while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
        pccTmp=zStrdup(poFeature->GetFieldAsString( iField ));
        iHasRes=1;
        break;
      }
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  if(iHasRes<0)
    pccTmp=NULL;
  free(pcaSqlQuery);
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  return (char*)pccTmp;
}

/**
 * Read the Result file (.res).
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcPid the service identifier (usid key from the [lenv] section)
 */
void readFinalRes(maps* pmsConf,char* pcPid,map* pmStatus){
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+44+1)*sizeof(char));
  sprintf(pcaSqlQuery,
	  "select fstate from %s.services where uuid=$$%s$$",
	  pmSchema->value,pcPid);
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iZooDsNb++;
  }
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  OGRFeature  *poFeature = NULL;
  int iHasRes=-1;
  while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
        addToMap(pmStatus,"Status",poFeature->GetFieldAsString( iField ));
        iHasRes=1;
        break;
      }
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  if(iHasRes<0)
    addToMap(pmStatus,"Status","Failed");
  free(pcaSqlQuery);
  return;
}

/**
 * Check if a service is running.
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 * @param pcPid the unique service identifier (usid from the lenv section)
 * @return 1 in case the service is still running, 0 otherwise
 */
int isRunning(maps* pmsConf,char* pcPid){
  int iResult=0;
  int iZooDsNb=getCurrentId(pmsConf);
  map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
  char *pcaSqlQuery=(char*)malloc((strlen(pmSchema->value)+strlen(pcPid)+73+1)*sizeof(char));
  sprintf(pcaSqlQuery,"select count(*) as t from %s.services where uuid=$$%s$$ and end_time is null;",pmSchema->value,pcPid);
  if( iZooDsNb == 0 ){
    init_sql(pmsConf);
    iZooDsNb++;
  }
  execSql(pmsConf,iZooDsNb-1,pcaSqlQuery);
  OGRFeature  *poFeature = NULL;
  while( (poFeature = poZooResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) && 
          atoi(poFeature->GetFieldAsString( iField ))>0 ){
        iResult=1;
        break;
      }
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(pmsConf,iZooDsNb-1);
  free(pcaSqlQuery);
  return iResult;
}

#endif