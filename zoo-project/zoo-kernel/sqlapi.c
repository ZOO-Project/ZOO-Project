/*
 * Author : David Saggiorato
 *          Gérald Fenoy
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

#ifdef RELY_ON_DB
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"

#include "sqlapi.h"
#include <fcgi_stdio.h>
#include <stdlib.h>

/**
 * Global GDALDataset pointer
 */
OGRDataSource *zoo_DS = NULL;

/**
 * Global OGRLayer pointer pointing to the lastest result set
 */
OGRLayer *zoo_ResultSet = NULL;

/**
 * Create a GDAL / OGR string for connecting to the db backend
 * (do not support active_schema)
 * 
 * @param conf the maps containing the setting of the main.cfg file
 */
char* createInitString(maps* conf){
  char* res=NULL;
  char keywords[5][9]={
    "dbname",
    "host",
    "port",
    "user",
    "password"
  };
  int i=0;
  maps* cconf=getMaps(conf,"database");
  int len=0;
  for(i=0;i<5;i++){
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
 * Connect to the db backend.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @see createInitString
 */
void init_sql(maps* conf){
  char* sqlInitString=createInitString(conf);
  OGRSFDriver *poDriver = NULL;
  OGRRegisterAll();
  zoo_DS = OGRSFDriverRegistrar::Open(sqlInitString,false,&poDriver);
  if( zoo_DS == NULL ){
#ifdef DEBUG
    fprintf(stderr,"sqlInitString: %s FAILED !\n",sqlInitString);
    fflush(stderr);
#endif
    free(sqlInitString);
    setMapInMaps(conf,"lenv","message","Failed to connect to the database backend");
    return;
  }
#ifdef DEBUG
  fprintf(stderr,"sqlInitString: %s SUCEED !\n",sqlInitString);
  fflush(stderr);
#endif
  free(sqlInitString);
}

/**
 * Close any connection to the db backend.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 */
void close_sql(maps* conf){
  if( zoo_ResultSet != NULL )
    zoo_DS->ReleaseResultSet( zoo_ResultSet );
  if(zoo_DS!=NULL){
    OGRDataSource::DestroyDataSource( zoo_DS );
    zoo_DS=NULL;
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
 * Execute a SQL query to the SQL Database Backend.
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param sqlQuery the SQL query to run
 * @return -1 in case of failure and 1 if the query succeed.
 */
int execSql(maps* conf,const char* sqlQuery){
  zoo_ResultSet = zoo_DS->ExecuteSQL( sqlQuery, NULL, NULL);
  if( zoo_ResultSet != NULL ){
    return 1;
  }
  return -1;
}

/**
 * Clean any memory allocated by executing a request
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param sqlQuery the SQL query to run
 * @return -1 in case of failure and 1 if the query succeed.
 */
void cleanUpResultSet(const maps* conf){
  if( zoo_ResultSet != NULL ){
    zoo_DS->ReleaseResultSet( zoo_ResultSet );
    zoo_ResultSet=NULL;
  }
}

/**
 * Record a file stored during ZOO-Kernel execution
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param filename the file's name
 * @param type the type (Intput,Output,Response)
 * @param name the maps containing the setting of the main.cfg file
 */
void recordStoredFile(maps* conf,const char* filename,const char* type,const char* name){
  map *uusid=getMapFromMaps(conf,"lenv","usid");
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(uusid->value)+strlen(filename)+strlen(type)+(name!=NULL?strlen(name):2)+68+1)*sizeof(char));
  if(name!=NULL)
    sprintf(sqlQuery,"INSERT INTO %s.files (uuid,filename,nature,name) VALUES ('%s','%s','%s','%s');",schema->value,uusid->value,filename,type,name);
  else
    sprintf(sqlQuery,"INSERT INTO %s.files (uuid,filename,nature,name) VALUES ('%s','%s','%s',NULL);",schema->value,uusid->value,filename,type);
  execSql(conf,sqlQuery);
  free(sqlQuery);
  cleanUpResultSet(conf);
}

/**
 * Insert the reference tuple corresponding to the running service
 * 
 * @param conf the maps containing the setting of the main.cfg file
 */
void recordServiceStatus(maps* conf){
  map *sid=getMapFromMaps(conf,"lenv","sid");
  map *osid=getMapFromMaps(conf,"lenv","osid");
  map *uusid=getMapFromMaps(conf,"lenv","usid");
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+
				strlen(uusid->value)+
				strlen(osid->value)+
				strlen(sid->value)+56+1)*sizeof(char));
  sprintf(sqlQuery,
	  "INSERT INTO %s.services (uuid,sid,osid)"
	  "VALUES ('%s','%s','%s');",
	  schema->value,
	  uusid->value,sid->value,osid->value);
  execSql(conf,sqlQuery);
  free(sqlQuery);
  cleanUpResultSet(conf);
}

/**
 * Store the content of the result file
 * 
 * @param conf the maps containing the setting of the main.cfg file
 * @param filename the file's name
 */
void recordResponse(maps* conf,char* filename){
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
  char *sqlQuery=(char*)malloc((strlen(schema->value)+flen+strlen(sid->value)+51+1)*sizeof(char));
  sprintf(sqlQuery,"INSERT INTO %s.responses (content,uuid) VALUES ($$%s$$,$$%s$$);",schema->value,tmps,sid->value);
  execSql(conf,sqlQuery);
  free(sqlQuery);
  free(tmps);
  cleanUpResultSet(conf);
}

/**
 * Update the current status of the running service.
 *
 * @param conf the map containing the setting of the main.cfg file
 * @return 0 on success, -2 if shmget failed, -1 if shmat failed
 */
int _updateStatus(maps* conf){
  map *sid=getMapFromMaps(conf,"lenv","usid");
  map *p=getMapFromMaps(conf,"lenv","status");
  map *msg=getMapFromMaps(conf,"lenv","message");
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(msg->value)+strlen(p->value)+strlen(sid->value)+64+1)*sizeof(char));
  sprintf(sqlQuery,"UPDATE %s.services set status=$$%s$$,message=$$%s$$ where uuid=$$%s$$;",schema->value,p->value,msg->value,sid->value);
  fflush(stderr);
  if( zoo_DS == NULL )
    init_sql(conf);
  execSql(conf,sqlQuery);
  cleanUpResultSet(conf);
  free(sqlQuery);
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
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+58+1)*sizeof(char));
  sprintf(sqlQuery,"select status||'|'||message from %s.services where uuid=$$%s$$;",schema->value,pid);
  if( zoo_DS == NULL )
    init_sql(conf);
  execSql(conf,sqlQuery);
  OGRFeature  *poFeature = NULL;
  const char *tmp1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
	tmp1=strdup(poFeature->GetFieldAsString( iField ));
      }
      else
	tmp1=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  cleanUpResultSet(conf);
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
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+82+1)*sizeof(char));
  sprintf(sqlQuery,
	  "select content from %s.responses where uuid=$$%s$$"
	  " order by creation_time desc limit 1",schema->value,pid);
  if( zoo_DS == NULL )
    init_sql(conf);
  execSql(conf,sqlQuery);
  OGRFeature  *poFeature = NULL;
  const char *tmp1;
  int hasRes=-1;
  while( (poFeature = zoo_ResultSet->GetNextFeature()) != NULL ){
    for( int iField = 0; iField < poFeature->GetFieldCount(); iField++ ){
      if( poFeature->IsFieldSet( iField ) ){
	tmp1=strdup(poFeature->GetFieldAsString( iField ));
	hasRes=1;
      }
      else
	tmp1=NULL;
    }
    OGRFeature::DestroyFeature( poFeature );
  }
  if(hasRes<0)
    tmp1=NULL;
  cleanUpResultSet(conf);
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
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)
    malloc((strlen(pid)+strlen(schema->value)+38+1)
	   *sizeof(char));
  if( zoo_DS == NULL )
    init_sql(conf);
  sprintf(sqlQuery,
	  "DELETE FROM %s.services where uuid=$$%s$$;",
	  schema->value,pid);
  execSql(conf,sqlQuery);
  cleanUpResultSet(conf);
  close_sql(conf);
  free(sqlQuery);
  end_sql();
}

/**
 * Stop handling status repport.
 *
 * @param conf the map containing the setting of the main.cfg file
 */
void unhandleStatus(maps* conf){
  map *schema=getMapFromMaps(conf,"database","schema");
  map *sid=getMapFromMaps(conf,"lenv","usid");
  map *fstate=getMapFromMaps(conf,"lenv","fstate");
  char *sqlQuery=(char*)malloc((strlen(sid->value)+
				strlen(schema->value)+
				(fstate!=NULL?
				 strlen(fstate->value):
				 6)
				+66+1)*sizeof(char));
  sprintf(sqlQuery,
	  "UPDATE %s.services set end_time=now(), fstate=$$%s$$"
	  " where uuid=$$%s$$;",
	  schema->value,(fstate!=NULL?fstate->value:"Failed"),sid->value);
  execSql(conf,sqlQuery);
  cleanUpResultSet(conf);
  close_sql(conf);
  free(sqlQuery);
  end_sql();
}

/**
 * Read the sid identifier attached of a service if any
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 * @return the sid value
 */
char* getStatusId(maps* conf,char* pid){
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+58+1)*sizeof(char));
  sprintf(sqlQuery,
	  "select osid from %s.services where uuid=$$%s$$",
	  schema->value,pid);
  if( zoo_DS == NULL )
    init_sql(conf);
  execSql(conf,sqlQuery);
  OGRFeature  *poFeature = NULL;
  const char *tmp1;
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
  return (char*)tmp1;
}

/**
 * Read the Result file (.res).
 *
 * @param conf the maps containing the setting of the main.cfg file
 * @param pid the service identifier (usid key from the [lenv] section)
 */
void readFinalRes(maps* conf,char* pid,map* statusInfo){
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+58+1)*sizeof(char));
  sprintf(sqlQuery,
	  "select fstate from %s.services where uuid=$$%s$$",
	  schema->value,pid);
  if( zoo_DS == NULL )
    init_sql(conf);
  execSql(conf,sqlQuery);
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
  map *schema=getMapFromMaps(conf,"database","schema");
  char *sqlQuery=(char*)malloc((strlen(schema->value)+strlen(pid)+73+1)*sizeof(char));
  sprintf(sqlQuery,"select count(*) as t from %s.services where uuid=$$%s$$ and end_time is null;",schema->value,pid);
  if( zoo_DS == NULL )
    init_sql(conf);
  execSql(conf,sqlQuery);
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
  cleanUpResultSet(conf);
  free(sqlQuery);
  return res;
}

#endif
