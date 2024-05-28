/*
 * Author : Gérald Fenoy
 *
 * Copyright 2017-2022 GeoLabs SARL. All rights reserved.
 *
 * This work was supported by public funds received in the framework of GEOSUD,
 * a project (ANR-10-EQPX-20) of the program "Investissements d'Avenir" managed
 * by the French National Research Agency
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

#ifdef META_DB
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"
#if GDAL_VERSION_MAJOR >= 2
#include <gdal_priv.h>
#endif

#include "meta_sql.h"
#include "sqlapi.h"
#include "response_print.h"
#ifdef USE_HPC
#include "service_internal_hpc.h"
#endif
#define META_SERVICES_LIST_ALL \
  "select id,identifier,title,abstract,version,service_type,service_provider,conf_id,mutable,user_id"\
  " from ows_process"
#define META_SERVICES_LIST_ALL_LENGTH strlen(META_SERVICES_LIST_ALL)

#define META_SERVICES_KEYWORDS_FROM_PROCESS \
  "SELECT keyword FROM CollectionDB.ows_Keywords where id in"\
  " (SELECT keywords_id FROM CollectionDB.DescriptionsKeywordsAssignment"\
  " where descriptions_id=%s) "
#define META_SERVICES_KEYWORDS_FROM_PROCESS_LENGTH strlen(META_SERVICES_KEYWORDS_FROM_PROCESS)

#define META_SERVICES_META_FROM_ANYTHING \
  "SELECT id, title, role, href FROM CollectionDB.ows_Metadata where id in"\
  " (SELECT metadata_id FROM CollectionDB.DescriptionsMetadataAssignment"\
  " where descriptions_id=%s) "
#define META_SERVICES_META_FROM_ANYTHING_LENGTH strlen(META_SERVICES_META_FROM_ANYTHING)

#define META_SERVICES_INNER_NB_META \
  "SELECT COUNT(*) FROM"\
  " (SELECT NULL FROM Collectiondb.ows_Metadata where pid=%s group by index) as a"
#define META_SERVICES_INNER_NB_META_LENGTH strlen(META_SERVICES_INNER_NB_META)

#define META_SERVICES_INNER_META_FROM_ANYTHING \
  "SELECT id, title, role, href FROM CollectionDB.ows_Metadata where pid=%s and index=%d"
#define META_SERVICES_INNER_META_FROM_ANYTHING_LENGTH strlen(META_SERVICES_INNER_META_FROM_ANYTHING)

#define META_SERVICES_AP_FROM_ANYTHING \
  "SELECT id,title,role,href FROM CollectionDB.ows_AdditionalParameters where id in"\
  " (SELECT additional_parameters_id FROM CollectionDB.DescriptionsAdditionalParametersAssignment"\
  " where descriptions_id=%s) "
#define META_SERVICES_AP_FROM_ANYTHING_LENGTH strlen(META_SERVICES_AP_FROM_ANYTHING)

#define META_SERVICES_AP_FROM_AP \
  "SELECT key,value FROM CollectionDB.ows_AdditionalParameter where additional_parameters_id =$q$%s$q$"
#define META_SERVICES_AP_FROM_AP_LENGTH strlen(META_SERVICES_AP_FROM_AP)

#define META_SERVICES_LIST_INPUTS_FROM_PROCESS				\
  "select id, identifier,title,abstract,min_occurs,max_occurs from CollectionDB.ows_Input where id in (SELECT input_id from CollectionDB.ProcessInputAssignment where process_id=%s) order by id"
#define META_SERVICES_LIST_INPUTS_FROM_PROCESS_LENGTH strlen(META_SERVICES_LIST_INPUTS_FROM_PROCESS)

#define META_SERVICES_LIST_INPUTS_FROM_INPUT				\
  "select id, identifier,title,abstract,min_occurs,max_occurs from CollectionDB.ows_Input where id in (SELECT child_input from CollectionDB.InputInputAssignment where parent_input=%s) order by id"
#define META_SERVICES_LIST_INPUTS_FROM_INPUT_LENGTH strlen(META_SERVICES_LIST_INPUTS_FROM_INPUT)

#define META_SERVICES_LIST_OUTPUTS_FROM_PROCESS \
  "select id, identifier,title,abstract from CollectionDB.ows_Output where id in (SELECT output_id from CollectionDB.ProcessOutputAssignment where process_id=%s) order by id"
#define META_SERVICES_LIST_OUTPUTS_FROM_PROCESS_LENGTH strlen(META_SERVICES_LIST_OUTPUTS_FROM_PROCESS)

#define META_SERVICES_LIST_OUTPUTS_FROM_OUTPUT \
  "select id, identifier,title,abstract from CollectionDB.ows_Output where id in (SELECT child_output from CollectionDB.OutputOutputAssignment where parent_output=%s) order by id"
#define META_SERVICES_LIST_OUTPUTS_FROM_OUTPUT_LENGTH strlen(META_SERVICES_LIST_OUTPUTS_FROM_OUTPUT)

#define META_SERVICES_LIST_LITERAL_FROM_IO \
  "select (SELECT name as type FROM CollectionDB.PrimitiveDatatypes where CollectionDB.PrimitiveDatatypes.id=data_type_id),default_value,(SELECT uom from CollectionDB.PrimitiveUOM where id=CollectionDB.LiteralDataDomain.uom),translate(translate(ARRAY((SELECT allowed_Value from CollectionDB.AllowedValues where id in (SELECT allowed_value_id from CollectionDB.AllowedValuesAssignment where literal_data_domain_id=CollectionDB.LiteralDataDomain.id)))::varchar,'{',''),'}',''),def as allowedvalues from CollectionDB.LiteralDataDomain where id in (SELECT data_description_id from CollectionDB.%sDataDescriptionAssignment where %s_id = %s);"
#define META_SERVICES_LIST_LITERAL_FROM_IO_LENGTH strlen(META_SERVICES_LIST_LITERAL_FROM_IO)

#define META_SERVICES_LIST_FORMATS_FROM_IO \
  "select mime_type,encoding,schema,maximum_megabytes,CASE WHEN use_mapserver THEN 'true' ELSE 'false' END, ms_styles, def from CollectionDB.ows_Format,CollectionDB.PrimitiveFormats where CollectionDB.ows_Format.primitive_format_id=CollectionDB.PrimitiveFormats.id and CollectionDB.ows_Format.id in (SELECT format_id from collectiondb.ows_datadescription where id in ( SELECT data_description_id from CollectionDB.%sDataDescriptionAssignment where %s_id = %s))"
#define META_SERVICES_LIST_FORMATS_FROM_IO_LENGTH strlen(META_SERVICES_LIST_FORMATS_FROM_IO)

/**
 * Create a new iotype pointer using field names from an OGRFeature
 *
 * @param f the OGRFeature 
 * @param fields the fields names
 * @return the iotype
 */
iotype* getIoType(OGRFeature* f,const char** fields){
  iotype* io=(iotype*)malloc(IOTYPE_SIZE);
  io->content=NULL;
  io->next=NULL;
  for(int i=0;i<6;i++){
    if(fields[i]==NULL)
      return io;
    const char* tmpS=f->GetFieldAsString( i );
    if(strlen(tmpS)>0){
      if(io->content==NULL)
	io->content=createMap(fields[i],tmpS);
      else
	addToMap(io->content,fields[i],tmpS);
    }
  }
  return io;
}

/**
 * Fill the AdditionalParameters map with the data extracted from metadb
 *
 * @param conf the main configuration maps
 * @param ap the map to fill
 * @param dref the description identifier
 * @return the number of metadata field found
 */
int fillAdditionalParameters(int iDbId,maps* conf,map** ap,const char* dref){
  int res=0;
  char* ioQuery=(char*)malloc((META_SERVICES_AP_FROM_ANYTHING_LENGTH+strlen(dref)+1)*sizeof(char));
  sprintf(ioQuery,META_SERVICES_AP_FROM_ANYTHING,dref);
  OGRFeature  *meta = NULL;
  OGRLayer *metas=fetchSql(conf,iDbId-1,ioQuery);
  free(ioQuery);
  int cnt=0;
  char fields[3][6]={
    "title",
    "role",
    "href"
  };
  while( (meta = metas->GetNextFeature()) != NULL ){
    int i=0;
    for(i=0;i<3;i++){      
      const char *tmp=meta->GetFieldAsString(i+1);
      if(strlen(tmp)>0)
        if(*ap==NULL){
          (*ap)=createMap(fields[i],tmp);
          addToMap(*ap,"fromDb","true");
        }
        else
          setMapArray(*ap,fields[i],res,tmp);
    }
    char* apQuery=(char*)malloc((META_SERVICES_AP_FROM_AP_LENGTH+strlen(dref)+1)*sizeof(char));
    sprintf(apQuery,META_SERVICES_AP_FROM_AP,meta->GetFieldAsString(0));
    OGRFeature  *adp = NULL;
    OGRLayer *adps=fetchSql(conf,iDbId-1,apQuery);
    free(apQuery);
    while( (adp = adps->GetNextFeature()) != NULL ){
      addToMap(*ap,adp->GetFieldAsString(0),adp->GetFieldAsString(1));
      OGRFeature::DestroyFeature( adp );
    }
    cleanFetchSql(conf,iDbId-1,adps);
    res++;
    OGRFeature::DestroyFeature( meta );
  }
  cleanFetchSql(conf,iDbId-1,metas);
  return res;
}

/**
 * Fill the metadata map with the data extracted from metadb
 *
 * @param conf the main configuration maps
 * @param metadata the map to fill
 * @param dref the description identifier
 * @return the number of metadata field found
 */
int fillMetadata(int iDbId,maps* conf,map** metadata,const char* dref){
  int res=0;
  char* pcaIoQuery=(char*)malloc((META_SERVICES_META_FROM_ANYTHING_LENGTH+strlen(dref)+1)*sizeof(char));
  sprintf(pcaIoQuery,META_SERVICES_META_FROM_ANYTHING,dref);
  OGRFeature  *meta = NULL;
  OGRLayer *metas=fetchSql(conf,iDbId-1,pcaIoQuery);
  free(pcaIoQuery);
  int cnt=0;
  char fields[4][255]={
    "id",
    "title",
    "role",
    "href"
  };
  while( (meta = metas->GetNextFeature()) != NULL ){
    int i=0;
    char* pcaLastKey=NULL;
    for(i=1;i<4;i++){
      const char *tmp=meta->GetFieldAsString(i);
      if(strlen(tmp)>0){
        if(*metadata==NULL)
          *metadata=createMap(fields[i],tmp);
        else
          setMapArray(*metadata,fields[i],res,tmp);
        if(pcaLastKey!=NULL)
          free(pcaLastKey);
        pcaLastKey=getMapArrayKey(*metadata,fields[i],res);
      }else{
        // Parse nested metadata
        if(i==3){
          // Loop over the array
          char* pcaNbNestedMetaQuery=(char*)malloc((META_SERVICES_INNER_NB_META_LENGTH+5)*sizeof(char));
          sprintf(pcaNbNestedMetaQuery,META_SERVICES_INNER_NB_META,meta->GetFieldAsString(0));
          OGRFeature *nbMeta = NULL;
          OGRLayer *nbValues=fetchSql(conf,iDbId-1,pcaNbNestedMetaQuery);
          free(pcaNbNestedMetaQuery);
          int nbElements=1;
          while( (nbMeta = nbValues->GetNextFeature()) != NULL ){
            nbElements=nbMeta->GetFieldAsInteger(0);
            OGRFeature::DestroyFeature( nbMeta );
          }
          cleanFetchSql(conf,iDbId-1,nbValues);
          for(int iCounter=0;iCounter<nbElements;iCounter++){
            char* pcaNestedMetaQuery=(char*)malloc((META_SERVICES_INNER_META_FROM_ANYTHING_LENGTH+10)*sizeof(char));
            sprintf(pcaNestedMetaQuery,META_SERVICES_INNER_META_FROM_ANYTHING,meta->GetFieldAsString(0),iCounter);
            OGRFeature  *nmeta = NULL;
            OGRLayer *nmetas=fetchSql(conf,iDbId-1,pcaNestedMetaQuery);
            free(pcaNestedMetaQuery);
            int iCount=0;
            char* pcaPrefix=(char*) malloc(15*sizeof(char));
            if(iCounter>0)
              sprintf(pcaPrefix,"%s_%d_%d",fields[2],res,iCounter);
            else{
              sprintf(pcaPrefix,"%s_%d",fields[2],res);
              char* pcaName=(char*) malloc((strlen(pcaPrefix)+9)*sizeof(char));
              sprintf(pcaName,"%s_elength",pcaPrefix);
              addIntToMap(*metadata,pcaName,nbElements);
              free(pcaName);
            }
            while( (nmeta = nmetas->GetNextFeature()) != NULL ){
              for(int iCnt=1;iCnt<4;iCnt++){
                char* pcaName=(char*) malloc((strlen(pcaPrefix)+strlen(fields[iCnt])+16)*sizeof(char));
                if(iCount>0)
                  sprintf(pcaName,"%s_%s_%d",pcaPrefix,fields[iCnt],iCount);
                else
                  sprintf(pcaName,"%s_%s",pcaPrefix,fields[iCnt]);
                const char *tmp=nmeta->GetFieldAsString(iCnt);
                if(strlen(tmp)>0){
                  if(*metadata==NULL)
                    *metadata=createMap(pcaName,tmp);
                  else
                    addToMap(*metadata,pcaName,tmp);
                }
                free(pcaName);
              }
              OGRFeature::DestroyFeature( nmeta );
              iCount++;
            }
            char* pcaName=(char*) malloc((strlen(pcaPrefix)+18)*sizeof(char));
            sprintf(pcaName,"%s_length",pcaPrefix);
            addIntToMap(*metadata,pcaName,iCount);
            cleanFetchSql(conf,iDbId-1,nmetas);
            free(pcaPrefix);
            free(pcaName);
          }
        }
      }
    }
    res++;
    OGRFeature::DestroyFeature( meta );
  }
  cleanFetchSql(conf,iDbId-1,metas);
  return res;
}

/**
 * Fill the metadata map with the data extracted from metadb
 *
 * @param conf the main configuration maps
 * @param pmContent the map to fill
 * @param dref the description identifier
 * @return the number of metadata field found
 */
int fillKeywords(int iDbId,maps* conf,map** pmContent,const char* dref){
  int res=0;
  char* pcaIoQuery=(char*)malloc((META_SERVICES_META_FROM_ANYTHING_LENGTH+strlen(dref)+1)*sizeof(char));
  sprintf(pcaIoQuery,META_SERVICES_KEYWORDS_FROM_PROCESS,dref);
  OGRFeature  *poKeywordsFeature = NULL;
  OGRLayer *poKeywordsLayer=fetchSql(conf,iDbId-1,pcaIoQuery);
  free(pcaIoQuery);
  char* pcaKeywords=NULL;
  int cnt=0;
  while( (poKeywordsFeature = poKeywordsLayer->GetNextFeature()) != NULL ){
    const char *pccValue=poKeywordsFeature->GetFieldAsString(0);
    if(pcaKeywords==NULL){
      pcaKeywords=zStrdup(pccValue);
    }
    else{
      char* pcaTmp=zStrdup(pcaKeywords);
      pcaKeywords=(char*) realloc(pcaKeywords,(strlen(pcaTmp)+strlen(pccValue)+2)*sizeof(char));
      sprintf(pcaKeywords,"%s,%s",pcaTmp,pccValue);
      free(pcaTmp);
    }
    res++;
    OGRFeature::DestroyFeature( poKeywordsFeature );
  }
  if(pcaKeywords!=NULL){
    addToMap(*pmContent,"keywords",pcaKeywords);
    free(pcaKeywords);
  }
  cleanFetchSql(conf,iDbId-1,poKeywordsLayer);
  return res;
}

/**
 * Try to fill the default/supported map for the LiteralData with the data
 * extracted from metadb.
 *
 * @param conf the main configuration maps
 * @param in the element to fill default/supported
 * @param input the OGRFeature corresponding to the input/output
 * @param ltype the element type ("Input" or "Output")
 * @return the number of default/supported definition found
 */
int fillLiteralData(int iDbId,maps* conf,elements* in,OGRFeature  *input,const char* ltype){
  int res=0;
  char* ioQuery=(char*)malloc((META_SERVICES_LIST_LITERAL_FROM_IO_LENGTH+(strlen(ltype)*2)+strlen(input->GetFieldAsString( 0 ))+1)*sizeof(char));
  sprintf(ioQuery,META_SERVICES_LIST_LITERAL_FROM_IO,ltype,ltype,input->GetFieldAsString( 0 ));
  OGRFeature  *io = NULL;
  OGRLayer *ios=fetchSql(conf,iDbId-1,ioQuery);
  free(ioQuery);
  int ioCnt=0;
  const char* fields[5]={"dataType","value","uom","AllowedValues",NULL};
  while( (io = ios->GetNextFeature()) != NULL ){
    iotype* currentIoType;
    if(strncmp(io->GetFieldAsString( 4 ),"1",1)==0){
      in->defaults=getIoType(io,fields);
    }else{
      if(in->supported==NULL)
	in->supported=getIoType(io,fields);
      else{
	iotype* p=in->supported;
	while(p->next!=NULL){
	  p=p->next;
	}
	p->next=getIoType(io,fields);
      }
    }
    in->format=strdup("LiteralData");
    res++;
    OGRFeature::DestroyFeature( io );
  }
  cleanFetchSql(conf,iDbId-1,ios);
  return res;
}

/**
 * Try to fill the default/supported map for the ComplexData with the data
 * extracted from metadb.
 *
 * @param conf the main configuration maps
 * @param in the element to fill default/supported
 * @param input the OGRFeature corresponding to the input/output
 * @param ltype the element type ("Input" or "Output")
 * @return the number of default/supported definition found
 */
int fillComplexData(int iDbId,maps* conf,elements* in,OGRFeature  *input,const char* ltype){
  int res=0;
  char* ioQuery=(char*)malloc((META_SERVICES_LIST_FORMATS_FROM_IO_LENGTH+(strlen(ltype)*2)+strlen(input->GetFieldAsString( 0 ))+1)*sizeof(char));
  sprintf(ioQuery,META_SERVICES_LIST_FORMATS_FROM_IO,ltype,ltype,input->GetFieldAsString( 0 ));
  OGRFeature  *io = NULL;
  OGRLayer *ios=fetchSql(conf,iDbId-1,ioQuery);
  free(ioQuery);
  int ioCnt=0;
  const char* fields[6]={"mimeType","ecoding","schema","maximumMegabytes","useMapserver","msStyle"};
  while( (io = ios->GetNextFeature()) != NULL ){
    iotype* currentIoType;
    if(strncmp(io->GetFieldAsString( 6 ),"1",1)==0){
      in->defaults=getIoType(io,fields);
    }else{
      if(in->supported==NULL)
	in->supported=getIoType(io,fields);
      else{
	iotype* p=in->supported;
	while(p->next!=NULL){
	  p=p->next;
	}
	p->next=getIoType(io,fields);
      }
    }
    in->format=strdup("ComplexData");
    res++;
    OGRFeature::DestroyFeature( io );
  }
  cleanFetchSql(conf,iDbId-1,ios);
  return res;
}

/**
 * Extract input definition from metadb
 *
 * @param conf the main configuration maps
 * @param input the OGRFeature pointing to an input
 * @return a new elements* corresponding to the current input
 */
elements* extractInput(int iDbId,maps* conf,OGRFeature *input){
  elements* res=createElements(input->GetFieldAsString( 1 ));
  res->content=createMap("title",input->GetFieldAsString( 2 ));
  addToMap(res->content,"abstract",input->GetFieldAsString( 3 ));
  addToMap(res->content,"minOccurs",input->GetFieldAsString( 4 ));
  addToMap(res->content,"maxOccurs",input->GetFieldAsString( 5 ));
  // Extract metadata
  fillMetadata(iDbId,conf,&res->metadata,input->GetFieldAsString( 0 ));
  res->additional_parameters=NULL;
  fillAdditionalParameters(iDbId,conf,&res->additional_parameters,input->GetFieldAsString( 0 ));
  res->defaults=NULL;
  res->supported=NULL;
  res->child=NULL;
  res->next=NULL;
  // Extract iotypes
  int ioCnt=fillLiteralData(iDbId,conf,res,input,"Input");
  if(ioCnt==0){
    ioCnt=fillComplexData(iDbId,conf,res,input,"Input");
  }
  if(ioCnt==0){
    char* nestedInputsQuery=(char*)malloc((META_SERVICES_LIST_INPUTS_FROM_INPUT_LENGTH+strlen(input->GetFieldAsString( 0 ))+1)*sizeof(char));
    sprintf(nestedInputsQuery,META_SERVICES_LIST_INPUTS_FROM_INPUT,input->GetFieldAsString( 0 ));
    OGRFeature  *ninput = NULL;
    OGRLayer *ninputs=fetchSql(conf,iDbId-1,nestedInputsQuery);
    free(nestedInputsQuery);
    while( (ninput = ninputs->GetNextFeature()) != NULL ){
      elements* nin=extractInput(iDbId,conf,ninput);
      addToElements(&res->child,nin);
      freeElements(&nin);
      free(nin);
      OGRFeature::DestroyFeature( ninput );
    }
    cleanFetchSql(conf,iDbId-1,ninputs);
  }
  return res;
}

/**
 * Extract output definition from metadb
 *
 * @param conf the main configuration maps
 * @param output the OGRFeature pointing to an output
 * @return a new elements* corresponding to the current output
 */
elements* extractOutput(int iDbId,maps* conf,OGRFeature *output){
  elements* res=createElements(output->GetFieldAsString( 1 ));
  res->content=createMap("title",output->GetFieldAsString( 2 ));
  addToMap(res->content,"abstract",output->GetFieldAsString( 3 ));
  fillMetadata(iDbId,conf,&res->metadata,output->GetFieldAsString( 0 ));
  fillAdditionalParameters(iDbId,conf,&res->additional_parameters,output->GetFieldAsString( 0 ));
  int ioCnt=fillLiteralData(iDbId,conf,res,output,"Output");
  if(ioCnt==0)
    ioCnt=fillComplexData(iDbId,conf,res,output,"Output");
  char* nestedOutputsQuery=(char*)malloc((META_SERVICES_LIST_OUTPUTS_FROM_OUTPUT_LENGTH+strlen(output->GetFieldAsString( 0 ))+1)*sizeof(char));
  sprintf(nestedOutputsQuery,META_SERVICES_LIST_OUTPUTS_FROM_OUTPUT,output->GetFieldAsString( 0 ));
  OGRFeature  *noutput = NULL;
  OGRLayer *noutputs=fetchSql(conf,iDbId-1,nestedOutputsQuery);
  free(nestedOutputsQuery);
  while( (noutput = noutputs->GetNextFeature()) != NULL ){
    elements* nout=extractOutput(iDbId,conf,noutput);
    addToElements(&res->child,nout);
    freeElements(&nout);
    free(nout);
    OGRFeature::DestroyFeature( noutput );
  }
  cleanFetchSql(conf,iDbId-1,noutputs);
  return res;
}

/**
 * Extract service from metadb
 *
 * @param conf the main configuration maps
 * @param serviceName the service name
 * @param minimal 1 for minimal metadata extraction (GetCapabilities), 0 in other cases.
 * @return a new service* corresponding to the service
 */
service* extractServiceFromDb(maps* conf,const char* serviceName,int minimal){
  OGRFeature  *poFeature = NULL;
  int iDbId=getCurrentId(conf);
  char* pcaClause=(char*)malloc((strlen(serviceName)+14)*sizeof(char));
  char* pcEmpty=NULL;
  sprintf(pcaClause,"identifier='%s'",serviceName);
  filterJobByUser(conf,&pcaClause,pcEmpty);
  char* tmpQuery=(char*)malloc((META_SERVICES_LIST_ALL_LENGTH+strlen(pcaClause)+21)*sizeof(char));
  sprintf(tmpQuery,"%s WHERE %s",META_SERVICES_LIST_ALL,pcaClause);
  free(pcaClause);
  maps* pmsTmp=getMaps(conf,"sqlenv");
  int iLen=0;
  if(getMapFromMaps(conf,"sqlenv","lastQuery")!=NULL){
    map* pmTmp=getMapFromMaps(conf,"sqlenv","length");
    if(pmTmp!=NULL)
      iLen=atoi(pmTmp->value);
    setMapArray(pmsTmp->content,"lastQuery",iLen,tmpQuery);
  }
  else{
    setMapInMaps(conf,"sqlenv","lastQuery",tmpQuery);
    pmsTmp=getMaps(conf,"sqlenv");
  }
  OGRLayer *res=fetchSql(conf,iDbId-1,tmpQuery);
  free(tmpQuery);
  if(res!=NULL){
    while( (poFeature = res->GetNextFeature()) != NULL ){
      setMapArray(pmsTmp->content,"lastResult",iLen,"Succeeded");
      //setMapInMaps(conf,"lenv","lastResult","Succeeded");
      service* s = (service*) malloc(SERVICE_SIZE);
      s->name = strdup(poFeature->GetFieldAsString( 1 ));
      s->content = createMap("title",poFeature->GetFieldAsString( 2 ));
      addToMap(s->content,"abstract",poFeature->GetFieldAsString( 3 ));
      addToMap(s->content,"processVersion",poFeature->GetFieldAsString( 4 ));
      addToMap(s->content,"serviceType",poFeature->GetFieldAsString( 5 ));
      addToMap(s->content,"serviceProvider",poFeature->GetFieldAsString( 6 ));
      addToMap(s->content,"confId",poFeature->GetFieldAsString( 7 ));
      addToMap(s->content,"mutable",poFeature->GetFieldAsString( 8 ));
      addToMap(s->content,"user_id",poFeature->GetFieldAsString( 9 ));
      addToMap(s->content,"fromDb","true");
      // Fetch Keywords
      fillKeywords(iDbId,conf,&s->content,poFeature->GetFieldAsString( 0 ));
      s->metadata=NULL;
      fillMetadata(iDbId,conf,&s->metadata,poFeature->GetFieldAsString( 0 ));
      s->additional_parameters=NULL;
      fillAdditionalParameters(iDbId,conf,&s->additional_parameters,poFeature->GetFieldAsString( 0 ));
      s->inputs=NULL;
      s->outputs=NULL;
      if(minimal==1){
        OGRFeature::DestroyFeature( poFeature );
        cleanFetchSql(conf,iDbId-1,res);
        return s;
      }
      char* inputsQuery=(char*)malloc((META_SERVICES_LIST_INPUTS_FROM_PROCESS_LENGTH+strlen(poFeature->GetFieldAsString( 0 ))+1)*sizeof(char));
      sprintf(inputsQuery,META_SERVICES_LIST_INPUTS_FROM_PROCESS,poFeature->GetFieldAsString( 0 ));
      OGRFeature  *input = NULL;
      OGRLayer *inputs=fetchSql(conf,iDbId-1,inputsQuery);
      setMapArray(pmsTmp->content,"lastQuery",iLen+1,inputsQuery);
      free(inputsQuery);
      while( (input = inputs->GetNextFeature()) != NULL ){
        setMapArray(pmsTmp->content,"lastResult",iLen+1,"Succeeded");
        elements* in=extractInput(iDbId,conf,input);
        if(in!=NULL){
          addToElements(&s->inputs,in);
          freeElements(&in);
          free(in);
        }
        OGRFeature::DestroyFeature( input );
      }
      cleanFetchSql(conf,iDbId-1,inputs);
      char* outputsQuery=(char*)malloc((META_SERVICES_LIST_OUTPUTS_FROM_PROCESS_LENGTH+strlen(poFeature->GetFieldAsString( 0 ))+1)*sizeof(char));
      sprintf(outputsQuery,META_SERVICES_LIST_OUTPUTS_FROM_PROCESS,poFeature->GetFieldAsString( 0 ));
      OGRFeature  *output = NULL;
      OGRLayer *outputs=fetchSql(conf,iDbId-1,outputsQuery);
      setMapArray(pmsTmp->content,"lastQuery",iLen+2,outputsQuery);
      free(outputsQuery);
      s->outputs=NULL;
      while( (output = outputs->GetNextFeature()) != NULL ){
        setMapArray(pmsTmp->content,"lastResult",iLen+2,"Succeeded");
        elements* in=extractOutput(iDbId,conf,output);
        if(in!=NULL){
          addToElements(&s->outputs,in);
          freeElements(&in);
          free(in);
        }
        OGRFeature::DestroyFeature( output );
      }
      cleanFetchSql(conf,iDbId-1,outputs);
      OGRFeature::DestroyFeature( poFeature );
      cleanFetchSql(conf,iDbId-1,res);
      return s;
    }
  }
  setMapArray(pmsTmp->content,"lastResult",iLen,"Failed");
  cleanFetchSql(conf,iDbId-1,res);
  return NULL;
}

/**
 * Extract every service definitions from metadb
 *
 * @param reg the registry
 * @param conf the main configuration maps
 * @param n the node where to add the services found
 * @param func the C function to call for each service found
 * @param minimal 1 for minimal metadata extraction (GetCapabilities), 0 in other cases.
 * @return the number of services found, -1 in case of failure
 */
int fetchServicesFromDb(registry* reg,maps* conf, void* doc0, void* n0,
			void (func) (registry *, maps *, void*, void*,
				     service *), int minimal ){
  int result=0;
  xmlDocPtr doc=(xmlDocPtr) doc0;
  xmlNodePtr n=(xmlNodePtr) n0;
  int iDbId=_init_sql(conf,"metadb");
  if(getMapFromMaps(conf,"lenv","dbIssue")!=NULL || result < 0){
    map* pmTmp=getMapFromMaps(conf,"lenv","dbIssue");
    return -1;
  }
  // Fetch every services
  char* pcaClause=NULL;
  char* pcEmpty=NULL;
  filterJobByUser(conf,&pcaClause,pcEmpty);
  char* pcaQuery=(char*)malloc((META_SERVICES_LIST_ALL_LENGTH+strlen(pcaClause)+21)*sizeof(char));
  sprintf(pcaQuery,"%s WHERE %s",META_SERVICES_LIST_ALL,pcaClause);
  OGRLayer *res=fetchSql(conf,iDbId-1,pcaQuery);
  free(pcaQuery);
  if(res!=NULL){
    OGRFeature  *poFeature = NULL;
    const char *tmp1;
    poFeature = res->GetNextFeature();
    while( poFeature != NULL ){
      if(compareCnt(conf,"serviceCntSkip","eupper") && compareCnt(conf,"serviceCntLimit","lower")){
        service* s=extractServiceFromDb(conf,poFeature->GetFieldAsString( 1 ),minimal);
#ifdef USE_HPC_NESTEDOUTPUTS
        addNestedOutputs(&s);
#endif
        func(reg,conf,doc,n,s);
        freeService(&s);
        free(s);
      }
      OGRFeature::DestroyFeature( poFeature );
      poFeature = res->GetNextFeature();
      result++;
    }
    cleanFetchSql(conf,iDbId-1,res);
  }
  close_sql(conf,iDbId-1);
  return result;
}

#endif
