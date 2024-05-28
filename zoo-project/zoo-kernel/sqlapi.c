/*
 * Authors : David Saggiorato
 *           Gérald Fenoy
 *  Copyright 2015 GeoLabs SARL. All rights reserved.
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
 **zoo_DS = NULL;

/**
 * Global OGRLayer pointer pointing to the lastest result set
 */
OGRLayer *zoo_ResultSet = NULL;

/**
 * Create a GDAL / OGR string for connecting to a db backend defined in the
 * key section.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param key the name of the section containing the connection setting
 * @return the OGR connection string
 */
char* _createInitString(maps* conf,const char* key){
  char* res=NULL;
  char keywords[6][14]={
    "dbname",
    "host",
    "port",
    "user",
    "password",
    "active_schema"    
  };
  int i=0;
  maps* cconf=getMaps(conf,key);
  if(cconf==NULL){
    return "-1";
  }
  int len=0;
  for(i=0;i<6;i++){
    map* tmp=getMap(cconf->content,keywords[i]);
    if(tmp!=NULL){
      if(res==NULL){
        res=(char*)malloc((strlen(keywords[i])+strlen(tmp->value)+4)*sizeof(char));
        sprintf(res,"%s='%s'",keywords[i],tmp->value);
        len+=strlen(res);
      }else{
        char* res1=(char*)malloc((strlen(keywords[i])+strlen(tmp->value)+5)*sizeof(char));
        sprintf(res1," %s='%s'",keywords[i],tmp->value);
        res=(char*)realloc(res,(len+strlen(keywords[i])+strlen(tmp->value)+5)*sizeof(char));
        memcpy(res+len,res1,(strlen(keywords[i])+strlen(tmp->value)+5)*sizeof(char));
        len+=strlen(res1);
        res[len]=0;
        free(res1);
      }
    }
  }
  map* tmp=getMap(cconf->content,"type");
  if(tmp!=NULL){
    char* fres=(char*)malloc((strlen(res)+strlen(tmp->value)+2)*sizeof(char));
    sprintf(fres,"%s:%s",tmp->value,res);
    free(res);
    return fres;
  }
  return res;
}

/**
 * Create a GDAL / OGR string for connecting to the db backend
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @return the OGR connection string
 */
char* createInitString(maps* conf){
  return _createInitString(conf,"database");
}

/**
 * Connect to a db backend.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @see createInitString
 */
int _init_sql(maps* conf,const char* key){
  char* sqlInitString=_createInitString(conf,key);
#ifdef SQL_DEBUG
  fprintf(stderr,"Try to connect to: %s %s !\n",key,sqlInitString);
  fflush(stderr);  
#endif
  if(strncmp(sqlInitString,"-1",2)==0)
    return -1;
  OGRSFDriver *poDriver = NULL;
  OGRRegisterAll();
  int zoo_ds_nb=0;
  map* dsNb=getMapFromMaps(conf,"lenv","ds_nb");
  if(dsNb==NULL){
    setMapInMaps(conf,"lenv","ds_nb","1");
  }else{
    zoo_ds_nb=atoi(dsNb->value);
    char* tmp=(char*)malloc(11*sizeof(char));
    sprintf(tmp,"%d",zoo_ds_nb+1);
    setMapInMaps(conf,"lenv","ds_nb",(const char*)tmp);
    free(tmp);
  }
  if(zoo_DS==NULL)
    zoo_DS=
#if GDAL_VERSION_MAJOR >= 2
      (GDALDataset**) malloc(sizeof(GDALDataset*))
#else
      (OGRDataSource**) malloc(sizeof(OGRDataSource*))
#endif
      ;
  else
    zoo_DS=      
#if GDAL_VERSION_MAJOR >= 2
      (GDALDataset**)realloc(zoo_DS,(zoo_ds_nb+1)*sizeof(GDALDataset*))
#else
      (OGRDataSource**)realloc(zoo_DS,(zoo_ds_nb+1)*sizeof(OGRDataSource*))
#endif
      ;
  
#if GDAL_VERSION_MAJOR >= 2
  zoo_DS[zoo_ds_nb] = (GDALDataset*) GDALOpenEx( sqlInitString,
				      GDAL_OF_UPDATE | GDAL_OF_VECTOR,
				      NULL, NULL, NULL );
#else
  zoo_DS[zoo_ds_nb] = OGRSFDriverRegistrar::Open(sqlInitString,false,&poDriver);
#endif
  if( zoo_DS[zoo_ds_nb] == NULL ){
#ifdef SQL_DEBUG
    fprintf(stderr,"sqlInitString: %s FAILED !\n",sqlInitString);
    fflush(stderr);
#endif
    free(sqlInitString);
    setMapInMaps(conf,"lenv","dbIssue","1");
    setMapInMaps(conf,"lenv","message",_("Failed to connect to the database backend"));
    return -2;
  }
#ifdef SQL_DEBUG
  fprintf(stderr,"sqlInitString: %s SUCEED !\n",sqlInitString);
  fflush(stderr);
#endif
  free(sqlInitString);
  zoo_ds_nb++;
  return zoo_ds_nb;
}

/**
 * Connect to the db backend.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @see createInitString
 */
int init_sql(maps* conf){
  return _init_sql(conf,"database");
}

/**
 * Close any connection to the db backend.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 */
void close_sql(maps* conf,int cid){
  if(cid<0)
    return;
  if( zoo_ResultSet != NULL ){
    zoo_DS[cid]->ReleaseResultSet( zoo_ResultSet );
    zoo_ResultSet=NULL;
  }
  if(zoo_DS!=NULL && zoo_DS[cid]!=NULL){
#if GDAL_VERSION_MAJOR >= 2
    GDALClose(zoo_DS[cid]);
#else
    OGRDataSource::DestroyDataSource( zoo_DS[cid] );
#endif
    zoo_DS[cid]=NULL;
    int zoo_ds_nb=0;
    map* dsNb=getMapFromMaps(conf,"lenv","ds_nb");
    if(dsNb!=NULL){
      zoo_ds_nb=atoi(dsNb->value);
      char* tmp=(char*)malloc(11*sizeof(char));
      if(zoo_ds_nb>0)
	sprintf(tmp,"%d",zoo_ds_nb-1);
      else
	sprintf(tmp,"%d",0);
      setMapInMaps(conf,"lenv","ds_nb",(const char*)tmp);
      free(tmp);
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
 * @param conf the maps containing the setting of the main.cfg file
 * @param sqlQuery the SQL query to run
 * @return NULL in case of failure or an OGRLayer pointer if the query succeed.
 */
OGRLayer *fetchSql(maps* conf,int cid,const char* sqlQuery){
  if(zoo_DS==NULL || zoo_DS[cid]==NULL)
    return NULL;
  OGRLayer *res=NULL;
#ifdef SQL_DEBUG
  fprintf(stderr,"************************* %s %s %d\n\n",sqlQuery,__FILE__,__LINE__);
  fflush(stderr);
#endif
  res = zoo_DS[cid]->ExecuteSQL( sqlQuery, NULL, NULL);
  return res;
}

void cleanFetchSql(maps* conf,int cid,OGRLayer *objects){
  if( objects != NULL ){
    zoo_DS[cid]->ReleaseResultSet( objects );
    objects=NULL;
  }
}

/**
 * Execute a SQL query to the SQL Database Backend.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param sqlQuery the SQL query to run
 * @return -1 in case of failure and 1 if the query succeed.
 */
int execSql(maps* conf,int cid,const char* sqlQuery){
  int res=-1;
  if(zoo_DS == NULL || zoo_DS[cid]==NULL)
    return -1;
  zoo_ResultSet = zoo_DS[cid]->ExecuteSQL( sqlQuery, NULL, NULL);
  if( zoo_ResultSet != NULL ){
    res=1;
  }
  return res;
}

/**
 * Clean any memory allocated by executing a request
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param sqlQuery the SQL query to run
 * @return -1 in case of failure and 1 if the query succeed.
 */
void cleanUpResultSet(const maps* conf,int cid){
  if( zoo_ResultSet != NULL ){
    zoo_DS[cid]->ReleaseResultSet( zoo_ResultSet );
    zoo_ResultSet=NULL;
  }
}

/**
 * Get the identifier of the current database
 *
 * @param pmsConf the maps containing the setting of the main.cfg file
 */
int getCurrentId(maps* pmsConf){
  int res=0;
  map* dsNb=getMapFromMaps(pmsConf,"lenv","ds_nb");
  if(dsNb!=NULL)
    res=atoi(dsNb->value);
  return res;
}

#ifdef RELY_ON_DB

/**
 * Record a file stored during ZOO-Kernel execution
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param filename the file's name
 * @param type the type (Intput,Output,Response)
 * @param name the maps containing the setting of the main.cfg file
 */
void recordStoredFile(maps* conf,const char* filename,const char* type,const char* name){
  int zoo_ds_nb=getCurrentId(conf);
  map *uusid=getMapFromMaps(conf,"lenv","usid");
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(uusid->value)+strlen(filename)+strlen(type)+(name!=NULL?strlen(name):2)+68+1)*sizeof(char));
  if(name!=NULL)
    sprintf(sqlQuery,"INSERT INTO %s.files (uuid,filename,nature,name) VALUES ('%s','%s','%s','%s');",schema->value,uusid->value,filename,type,name);
  else
    sprintf(sqlQuery,"INSERT INTO %s.files (uuid,filename,nature,name) VALUES ('%s','%s','%s',NULL);",schema->value,uusid->value,filename,type);
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  free(sqlQuery);
  cleanUpResultSet(conf,zoo_ds_nb-1);
}

/**
 * Run a SQL query
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param query the SQL query to run
 * @return char pointer to the allocated result string
 */
char* runSqlQuery(maps* conf,char* query){
  int zoo_ds_nb=getCurrentId(conf);
  int isCreated=0;
  if( zoo_ds_nb == 0 || zoo_DS == NULL || zoo_DS[zoo_ds_nb-1]==NULL ){
    init_sql(conf);
    isCreated=1;
    zoo_ds_nb++;
  }
  if(execSql(conf,zoo_ds_nb-1,query)<0)
    return NULL;
  OGRFeature  *poFeature = NULL;
  char *tmp1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
	tmp1=zStrdup(poFeature->GetFieldAsString( iField ));
      }
      else
	tmp1=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(conf,zoo_ds_nb-1);
  if(isCreated>0)
    close_sql(conf,zoo_ds_nb-1);
  return tmp1;
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
  if(pmUserName!=NULL){
    char* pcaTmp=NULL;
    if(*pcaClauseFinal!=NULL){
      pcaTmp=zStrdup(*pcaClauseFinal);
    }
    map *pmSchema=getMapFromMaps(pmsConf,"database","schema");
    if(*pcaClauseFinal!=NULL){
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
    if(*pcaClauseFinal!=NULL){
      char* pcaTmp=zStrdup(*pcaClauseFinal);
      if(pcaClauseDate!=NULL){
	*pcaClauseFinal=(char*)realloc(*pcaClauseFinal,
				       (strlen(pcaTmp)+strlen(pcaClauseDate)+15)*sizeof(char));
	sprintf(*pcaClauseFinal,"%s AND %s AND user_id=0",
		pcaTmp,pcaClauseDate);
      }else{
	*pcaClauseFinal=(char*)realloc(*pcaClauseFinal,
				       (strlen(pcaTmp)+15)*sizeof(char));
	sprintf(*pcaClauseFinal,"%s AND user_id=0",
		pcaTmp);
      }
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
    int isCreated=0;
    char *sqlQuery=(char*)malloc((strlen(pmSchema->value)+
				  strlen(pmUserName->value)+
				  34+1)*sizeof(char));
    sprintf(sqlQuery,"SELECT id from %s.users WHERE name='%s'",pmSchema->value,pmUserName->value);
    if( iZooDsNb == 0 ){
      init_sql(pmsConf);
      isCreated=1;
      iZooDsNb++;
    }
    execSql(pmsConf,iZooDsNb-1,sqlQuery);
    OGRFeature  *poFeature = NULL;
    char *tmp1;
    while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
      for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
	if( poFeature->IsFieldSet( iField ) ){
	  tmp1=zStrdup(poFeature->GetFieldAsString( iField ));
	  OGRFeature::DestroyFeature( poFeature );
	  int res=atoi(tmp1);
	  free(tmp1);
	  cleanUpResultSet(pmsConf,iZooDsNb-1);
	  return res;
	}
	else
	  tmp1=NULL;
      }
      OGRFeature::DestroyFeature( poFeature );
    }
    free(sqlQuery);
    cleanUpResultSet(pmsConf,iZooDsNb-1);
    sqlQuery=(char*)malloc((strlen(pmSchema->value)+
			    strlen(pmUserName->value)+
			    50+1)*sizeof(char));
    sprintf(sqlQuery,"INSERT INTO %s.users (name) VALUES ('%s') RETURNING id",pmSchema->value,pmUserName->value);
    execSql(pmsConf,iZooDsNb-1,sqlQuery);
    poFeature = NULL;
    while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
      for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
	if( poFeature->IsFieldSet( iField ) ){
	  tmp1=zStrdup(poFeature->GetFieldAsString( iField ));
	  OGRFeature::DestroyFeature( poFeature );
	  int res=atoi(tmp1);
	  free(tmp1);
	  cleanUpResultSet(pmsConf,iZooDsNb-1);
	  return res;
	}
	else
	  tmp1=NULL;
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
 * @param conf the maps containing the setting of the main.cfg file
 */
void recordServiceStatus(maps* conf){
  int isCreated=0;
  int zoo_ds_nb=getCurrentId(conf);
  map *sid=getMapFromMaps(conf,"lenv","sid");
  map *osid=getMapFromMaps(conf,"lenv","osid");
  map *uusid=getMapFromMaps(conf,"lenv","usid");
  map *pmProcessId=getMapFromMaps(conf,"lenv","oIdentifier");
  map *schema=getMapFromMaps(conf,"database","schema");
  map* pmType=getMapFromMaps(conf,"main","executionType");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+
				strlen(uusid->value)+
				strlen(pmProcessId->value)+
				strlen(pmType->value)+
				strlen(sid->value)+
				strlen(osid->value)+
				strlen(wpsStatus[2])+99+9+1)*sizeof(char));
  sprintf(sqlQuery,
	  "INSERT INTO %s.services (uuid,processid,sid,osid,fstate,itype,user_id) "
	  "VALUES ('%s','%s','%s','%s','%s','%s','%d');",
	  schema->value,
	  uusid->value,
	  pmProcessId->value,
	  sid->value,
	  osid->value,
	  wpsStatus[2],
	  pmType->value,
	  getUserId(conf,zoo_ds_nb,schema));
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    isCreated=1;
    zoo_ds_nb++;
  }
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  free(sqlQuery);
  cleanUpResultSet(conf,zoo_ds_nb-1);
  if(isCreated>1)
    close_sql(conf,zoo_ds_nb-1);
}

/**
 * Store the content of the result file
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param filename the file's name
 */
void recordResponse(maps* conf,char* filename){
  int zoo_ds_nb=getCurrentId(conf);
  FILE *file = fopen (filename, "rb");
  fseek (file, 0, SEEK_END);
  long flen = ftell (file);
  fseek (file, 0, SEEK_SET);
  char *tmps = (char *) malloc ((flen + 1) * sizeof (char));
  fread (tmps, flen, 1, file);
  tmps[flen]=0;
  fclose(file);
  map *sid=getMapFromMaps(conf,"lenv","usid");
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+flen+strlen(sid->value)+57+1)*sizeof(char));
  sprintf(sqlQuery,"INSERT INTO %s.responses (content,uuid) VALUES ($$%s$$,$$%s$$);",schema->value,tmps,sid->value);
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  free(sqlQuery);
  free(tmps);
  cleanUpResultSet(conf,zoo_ds_nb-1);
}

/**
 * Update the current status of the running service.
 *
 * @param conf the map containing the setting of the main.cfg file
 * @return 0 on success, -2 if shmget failed, -1 if shmat failed
 */
int _updateStatus(maps* conf){
  int zoo_ds_nb=getCurrentId(conf);
  map *sid=getMapFromMaps(conf,"lenv","usid");
  map *p=getMapFromMaps(conf,"lenv","status");
  map *msg=getMapFromMaps(conf,"lenv","message");
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(msg->value)+strlen(p->value)+strlen(sid->value)+81+1)*sizeof(char));
  sprintf(sqlQuery,"UPDATE %s.services set status=$$%s$$,message=$$%s$$,updated_time=now() where uuid=$$%s$$;",schema->value,p->value,msg->value,sid->value);
  if( zoo_ds_nb == 0 ){
    if(getMapFromMaps(conf,"lenv","file.log")==NULL){
      free(sqlQuery);
      return 1;
    }
    init_sql(conf);
    zoo_ds_nb++;
  }
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  cleanUpResultSet(conf,zoo_ds_nb-1);
  free(sqlQuery);
#ifdef USE_JSON  
  invokeBasicCallback(conf,SERVICE_STARTED);
#endif
  return 0;
}

/**
 * Get the ongoing status of a running service 
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (MESSAGE|POURCENTAGE)
 */
char* _getStatus(maps* conf,char* pid){
  int zoo_ds_nb=getCurrentId(conf);
  int created=-1;
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+104+1)*sizeof(char));
  sprintf(sqlQuery,"select CASE WHEN message is null THEN '-1' ELSE status||'|'||message END from %s.services where uuid=$$%s$$;",schema->value,pid);
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    zoo_ds_nb++;
    created=1;
  }
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  OGRFeature  *poFeature = NULL;
  const char *tmp1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
	tmp1=zStrdup(poFeature->GetFieldAsString( iField ));
      }
      else
	tmp1=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(conf,zoo_ds_nb-1);
  free(sqlQuery);
  return (char*)tmp1;
}


/**
 * Get the ongoing status field of a running service
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (MESSAGE|POURCENTAGE)
 */
char* _getStatusField(maps* conf,char* pid,const char* field){
  int zoo_ds_nb=getCurrentId(conf);
  int created=-1;
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=NULL;
  if(strstr(field,"_time")!=NULL){
    sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+strlen(field)+strlen(field)+99+1)*sizeof(char));
    sprintf(sqlQuery,"select CASE WHEN %s is null THEN '-1' ELSE display_date_rfc3339(%s) END from %s.services where uuid=$$%s$$;",field,field,schema->value,pid);
  }else{
    if(strstr(field,"itype")!=NULL){
      sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+(2*strlen(field))+strlen(field)+137+1)*sizeof(char));
      sprintf(sqlQuery,"select CASE WHEN %s is null  THEN 'unknown' ELSE CASE WHEN %s = 'json' THEN 'process' ELSE 'unknown' END END from %s.services where uuid=$$%s$$;",field,field,schema->value,pid);
    }else{
      sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+strlen(field)+strlen(field)+83+1)*sizeof(char));
      sprintf(sqlQuery,"select CASE WHEN %s is null THEN '-1' ELSE %s::text END from %s.services where uuid=$$%s$$;",field,field,schema->value,pid);
    }
  }
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    zoo_ds_nb++;
    created=1;
  }
  if(execSql(conf,zoo_ds_nb-1,sqlQuery)<0)
    return NULL;
  OGRFeature  *poFeature = NULL;
  const char *tmp1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
	tmp1=zStrdup(poFeature->GetFieldAsString( iField ));
      }
      else
	tmp1=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(conf,zoo_ds_nb-1);
  free(sqlQuery);
  return (char*)tmp1;
}
/**
 * Read the cache file of a running service 
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the reported status char* (temporary/final result)
 */
char* _getStatusFile(maps* conf,char* pid){
  int zoo_ds_nb=getCurrentId(conf);
  map *schema=getMapFromMaps(conf,"database","schema");
  OGRFeature  *poFeature = NULL;
  const char *tmp1=NULL;
  int hasRes=-1;
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+82+1)*sizeof(char));
  sprintf(sqlQuery,
	  "select content from %s.responses where uuid=$$%s$$"
	  " order by creation_time desc limit 1",schema->value,pid);
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    zoo_ds_nb++;
  }
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  if(zoo_ResultSet!=NULL){
      while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
	for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
	  if( poFeature->IsFieldSet( iField ) ){
	    tmp1=zStrdup(poFeature->GetFieldAsString( iField ));
	    hasRes=1;
	  }
	  else
	    tmp1=NULL;
	}
	OGRFeature::DestroyFeature( poFeature );
      }
  }
  if(hasRes<0)
    tmp1=NULL;
  cleanUpResultSet(conf,zoo_ds_nb-1);
  free(sqlQuery);
  return (char*)tmp1;
}

/**
 * Delete a service reference from the database.
 *
 * @param conf the map containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 */
void removeService(maps* conf,char* pid){
  int zoo_ds_nb=getCurrentId(conf);
  int iCreated=0;
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)
    malloc((strlen(pid)+strlen(schema->value)+38+1)
	   *sizeof(char));
  sprintf(sqlQuery,
	  "DELETE FROM %s.services where uuid=$$%s$$;",
	  schema->value,pid);
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    zoo_ds_nb++;
    iCreated=1;
  }
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  cleanUpResultSet(conf,zoo_ds_nb-1);
  if(iCreated>0)
    close_sql(conf,zoo_ds_nb-1);
  free(sqlQuery);
  //end_sql();
}

/**
 * Stop handling status repport.
 *
 * @param conf the map containing the setting of the main.cfg file
 */
void unhandleStatus(maps* conf){
  int zoo_ds_nb=getCurrentId(conf);
  map *schema=getMapFromMaps(conf,"database","schema");
  map *sid=getMapFromMaps(conf,"lenv","usid");
  map *fstate=getMapFromMaps(conf,"lenv","fstate");
  char *sqlQuery=(char*)malloc((strlen(sid->value)+
				strlen(schema->value)+
				(fstate!=NULL?
				 strlen(fstate->value):
				 4)
				+65+1)*sizeof(char));
  sprintf(sqlQuery,
	  "UPDATE %s.services set end_time=now(), fstate=$$%s$$"
	  " where uuid=$$%s$$;",
	  schema->value,(fstate!=NULL?fstate->value:"Failed"),sid->value);
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  cleanUpResultSet(conf,zoo_ds_nb-1);
  //close_sql(conf,zoo_ds_nb-1);
  free(sqlQuery);
  //end_sql();
}

/**
 * Read the sid identifier attached of a service if any
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the sid value
 */
char* getStatusId(maps* conf,char* pid){
  int zoo_ds_nb=getCurrentId(conf);
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+42+1)*sizeof(char));
  sprintf(sqlQuery,
	  "select osid from %s.services where uuid=$$%s$$",
	  schema->value,pid);
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    zoo_ds_nb++;
  }
  if(execSql(conf,zoo_ds_nb-1,sqlQuery)<0)
    return NULL;
  OGRFeature  *poFeature = NULL;
  char *tmp1;
  int hasRes=-1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
	tmp1=zStrdup(poFeature->GetFieldAsString( iField ));
	hasRes=1;
	break;
      }
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  if(hasRes<0)
    tmp1=NULL;
  free(sqlQuery);
  cleanUpResultSet(conf,zoo_ds_nb-1);
  return (char*)tmp1;
}

/**
 * Read the Result file (.res).
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 */
void readFinalRes(maps* conf,char* pid,map* statusInfo){
  int zoo_ds_nb=getCurrentId(conf);
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+44+1)*sizeof(char));
  sprintf(sqlQuery,
	  "select fstate from %s.services where uuid=$$%s$$",
	  schema->value,pid);
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    zoo_ds_nb++;
  }
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  OGRFeature  *poFeature = NULL;
  int hasRes=-1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
	addToMap(statusInfo,"Status",poFeature->GetFieldAsString( iField ));
	hasRes=1;
	break;
      }
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(conf,zoo_ds_nb-1);
  if(hasRes<0)
    addToMap(statusInfo,"Status","Failed");
  free(sqlQuery);
  return;
}

/**
 * Check if a service is running.
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the unique service identifier (usid from the lenv section)
 * @return 1 in case the service is still running, 0 otherwise
 */
int isRunning(maps* conf,char* pid){
  int res=0;
  int zoo_ds_nb=getCurrentId(conf);
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+73+1)*sizeof(char));
  sprintf(sqlQuery,"select count(*) as t from %s.services where uuid=$$%s$$ and end_time is null;",schema->value,pid);
  if( zoo_ds_nb == 0 ){
    init_sql(conf);
    zoo_ds_nb++;
  }
  execSql(conf,zoo_ds_nb-1,sqlQuery);
  OGRFeature  *poFeature = NULL;
  const char *tmp1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) && 
	  atoi(poFeature->GetFieldAsString( iField ))>0 ){
	res=1;
	break;
      }
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(conf,zoo_ds_nb-1);
  free(sqlQuery);
  return res;
}

#endif
