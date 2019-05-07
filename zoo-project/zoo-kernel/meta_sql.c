/*
 * Author : Gérald Fenoy
 *
 * Copyright 2017 GeoLabs SARL. All rights reserved.
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
  "select id,identifier,title,abstract,service_type,service_provider,conf_id"\
  " from ows_process"
#define META_SERVICES_LIST_ALL_LENGTH strlen(META_SERVICES_LIST_ALL)

#define META_SERVICES_KEYWORDS_FROM_PROCESS \
  "SELECT keyword FROM CollectionDB.ows_Keywords where id in"\
  " (SELECT keywords_id FROM CollectionDB.DescriptionsKeywordsAssignment"\
  " where descriptions_id=%s) "
#define META_SERVICES_KEYWORDS_FROM_PROCESS_LENGTH strlen(META_SERVICES_KEYWORDS_FROM_PROCESS)

#define META_SERVICES_META_FROM_ANYTHING \
  "SELECT title,role,href FROM CollectionDB.ows_Metadata where id in"\
  " (SELECT metadata_id FROM CollectionDB.DescriptionsMetadataAssignment"\
  " where descriptions_id=%s) "
#define META_SERVICES_META_FROM_ANYTHING_LENGTH strlen(META_SERVICES_META_FROM_ANYTHING)

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
int fillAdditionalParameters(maps* conf,map** ap,const char* dref){
  int res=0;
  char* ioQuery=(char*)malloc((META_SERVICES_AP_FROM_ANYTHING_LENGTH+strlen(dref)+1)*sizeof(char));
  sprintf(ioQuery,META_SERVICES_AP_FROM_ANYTHING,dref);
  OGRFeature  *meta = NULL;
  OGRLayer *metas=fetchSql(conf,0,ioQuery);
  free(ioQuery);
  int cnt=0;
  char fields[3][255]={
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
    OGRLayer *adps=fetchSql(conf,0,apQuery);
    free(apQuery);
    while( (adp = adps->GetNextFeature()) != NULL ){
      addToMap(*ap,adp->GetFieldAsString(0),adp->GetFieldAsString(1));
      OGRFeature::DestroyFeature( adp );
    }
    cleanFetchSql(conf,0,adps);
    res++;
    OGRFeature::DestroyFeature( meta );
  }
  cleanFetchSql(conf,0,metas);
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
int fillMetadata(maps* conf,map** metadata,const char* dref){
  int res=0;
  char* ioQuery=(char*)malloc((META_SERVICES_META_FROM_ANYTHING_LENGTH+strlen(dref)+1)*sizeof(char));
  sprintf(ioQuery,META_SERVICES_META_FROM_ANYTHING,dref);
  OGRFeature  *meta = NULL;
  OGRLayer *metas=fetchSql(conf,0,ioQuery);
  free(ioQuery);
  int cnt=0;
  char fields[3][255]={
    "title",
    "role",
    "href"
  };
  while( (meta = metas->GetNextFeature()) != NULL ){
    int i=0;
    for(i=0;i<3;i++){
      const char *tmp=meta->GetFieldAsString(i);
      if(strlen(tmp)>0)
	if(*metadata==NULL)
	  *metadata=createMap(fields[i],tmp);
	else
	  addToMap(*metadata,fields[i],tmp);
    }
    res++;
    OGRFeature::DestroyFeature( meta );
  }
  cleanFetchSql(conf,0,metas);
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
int fillLiteralData(maps* conf,elements* in,OGRFeature  *input,const char* ltype){
  int res=0;
  char* ioQuery=(char*)malloc((META_SERVICES_LIST_LITERAL_FROM_IO_LENGTH+(strlen(ltype)*2)+strlen(input->GetFieldAsString( 0 ))+1)*sizeof(char));
  sprintf(ioQuery,META_SERVICES_LIST_LITERAL_FROM_IO,ltype,ltype,input->GetFieldAsString( 0 ));
  OGRFeature  *io = NULL;
  OGRLayer *ios=fetchSql(conf,0,ioQuery);
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
  cleanFetchSql(conf,0,ios);
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
int fillComplexData(maps* conf,elements* in,OGRFeature  *input,const char* ltype){
  int res=0;
  char* ioQuery=(char*)malloc((META_SERVICES_LIST_FORMATS_FROM_IO_LENGTH+(strlen(ltype)*2)+strlen(input->GetFieldAsString( 0 ))+1)*sizeof(char));
  sprintf(ioQuery,META_SERVICES_LIST_FORMATS_FROM_IO,ltype,ltype,input->GetFieldAsString( 0 ));
  OGRFeature  *io = NULL;
  OGRLayer *ios=fetchSql(conf,0,ioQuery);
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
  cleanFetchSql(conf,0,ios);
  return res;
}

/**
 * Extract input definition from metadb
 *
 * @param conf the main configuration maps
 * @param input the OGRFeature pointing to an input
 * @return a new elements* corresponding to the current input
 */
elements* extractInput(maps* conf,OGRFeature *input){
  elements* res=createElements(input->GetFieldAsString( 1 ));
  res->content=createMap("title",input->GetFieldAsString( 2 ));
  addToMap(res->content,"abstract",input->GetFieldAsString( 3 ));
  addToMap(res->content,"minOccurs",input->GetFieldAsString( 4 ));
  addToMap(res->content,"maxOccurs",input->GetFieldAsString( 5 ));
  // Extract metadata
  fillMetadata(conf,&res->metadata,input->GetFieldAsString( 0 ));
  res->additional_parameters=NULL;
  fillAdditionalParameters(conf,&res->additional_parameters,input->GetFieldAsString( 0 ));
  res->defaults=NULL;
  res->supported=NULL;
  res->child=NULL;
  res->next=NULL;
  // Extract iotypes
  int ioCnt=fillLiteralData(conf,res,input,"Input");
  if(ioCnt==0){
    ioCnt=fillComplexData(conf,res,input,"Input");
  }
  if(ioCnt==0){
    char* nestedInputsQuery=(char*)malloc((META_SERVICES_LIST_INPUTS_FROM_INPUT_LENGTH+strlen(input->GetFieldAsString( 0 ))+1)*sizeof(char));
    sprintf(nestedInputsQuery,META_SERVICES_LIST_INPUTS_FROM_INPUT,input->GetFieldAsString( 0 ));
    OGRFeature  *ninput = NULL;
    OGRLayer *ninputs=fetchSql(conf,0,nestedInputsQuery);
    free(nestedInputsQuery);
    while( (ninput = ninputs->GetNextFeature()) != NULL ){
      elements* nin=extractInput(conf,ninput);
      addToElements(&res->child,nin);
      freeElements(&nin);
      free(nin);
      OGRFeature::DestroyFeature( ninput );
    }
    cleanFetchSql(conf,0,ninputs);
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
elements* extractOutput(maps* conf,OGRFeature *output){
  elements* res=createElements(output->GetFieldAsString( 1 ));
  res->content=createMap("title",output->GetFieldAsString( 2 ));
  addToMap(res->content,"abstract",output->GetFieldAsString( 3 ));
  fillMetadata(conf,&res->metadata,output->GetFieldAsString( 0 ));
  fillAdditionalParameters(conf,&res->additional_parameters,output->GetFieldAsString( 0 ));
  int ioCnt=fillLiteralData(conf,res,output,"Output");
  if(ioCnt==0)
    ioCnt=fillComplexData(conf,res,output,"Output");
  char* nestedOutputsQuery=(char*)malloc((META_SERVICES_LIST_OUTPUTS_FROM_OUTPUT_LENGTH+strlen(output->GetFieldAsString( 0 ))+1)*sizeof(char));
  sprintf(nestedOutputsQuery,META_SERVICES_LIST_OUTPUTS_FROM_OUTPUT,output->GetFieldAsString( 0 ));
  OGRFeature  *noutput = NULL;
  OGRLayer *noutputs=fetchSql(conf,0,nestedOutputsQuery);
  free(nestedOutputsQuery);
  while( (noutput = noutputs->GetNextFeature()) != NULL ){
    elements* nout=extractOutput(conf,noutput);
    addToElements(&res->child,nout);
    freeElements(&nout);
    free(nout);
    OGRFeature::DestroyFeature( noutput );
  }
  cleanFetchSql(conf,0,noutputs);
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
  char* tmpQuery=(char*)malloc((META_SERVICES_LIST_ALL_LENGTH+strlen(serviceName)+21)*sizeof(char));
  sprintf(tmpQuery,"%s WHERE identifier='%s'",META_SERVICES_LIST_ALL,serviceName);  
  OGRLayer *res=fetchSql(conf,0,tmpQuery);
  free(tmpQuery);
  if(res!=NULL){
    while( (poFeature = res->GetNextFeature()) != NULL ){
      service* s = (service*) malloc(SERVICE_SIZE);
      s->name = strdup(poFeature->GetFieldAsString( 1 ));
      s->content = createMap("title",poFeature->GetFieldAsString( 2 ));
      addToMap(s->content,"abstract",poFeature->GetFieldAsString( 3 ));
      addToMap(s->content,"serviceType",poFeature->GetFieldAsString( 4 ));
      addToMap(s->content,"serviceProvider",poFeature->GetFieldAsString( 5 ));
      addToMap(s->content,"confId",poFeature->GetFieldAsString( 6 ));
      addToMap(s->content,"fromDb","true");
      s->metadata=NULL;
      fillMetadata(conf,&s->metadata,poFeature->GetFieldAsString( 0 ));
      s->additional_parameters=NULL;
      fillAdditionalParameters(conf,&s->additional_parameters,poFeature->GetFieldAsString( 0 ));
      s->inputs=NULL;
      s->outputs=NULL;
      if(minimal==1){
	OGRFeature::DestroyFeature( poFeature );
	cleanFetchSql(conf,0,res);
	return s;
      }
      char* inputsQuery=(char*)malloc((META_SERVICES_LIST_INPUTS_FROM_PROCESS_LENGTH+strlen(poFeature->GetFieldAsString( 0 ))+1)*sizeof(char));
      sprintf(inputsQuery,META_SERVICES_LIST_INPUTS_FROM_PROCESS,poFeature->GetFieldAsString( 0 ));
      OGRFeature  *input = NULL;
      OGRLayer *inputs=fetchSql(conf,0,inputsQuery);
      free(inputsQuery);
      while( (input = inputs->GetNextFeature()) != NULL ){
	elements* in=extractInput(conf,input);
	if(in!=NULL){
	  addToElements(&s->inputs,in);
	  freeElements(&in);
	  free(in);
	}
	OGRFeature::DestroyFeature( input );
      }
      cleanFetchSql(conf,0,inputs);
      char* outputsQuery=(char*)malloc((META_SERVICES_LIST_OUTPUTS_FROM_PROCESS_LENGTH+strlen(poFeature->GetFieldAsString( 0 ))+1)*sizeof(char));
      sprintf(outputsQuery,META_SERVICES_LIST_OUTPUTS_FROM_PROCESS,poFeature->GetFieldAsString( 0 ));
      OGRFeature  *output = NULL;
      OGRLayer *outputs=fetchSql(conf,0,outputsQuery);
      free(outputsQuery);
      s->outputs=NULL;
      while( (output = outputs->GetNextFeature()) != NULL ){
	elements* in=extractOutput(conf,output);
	if(in!=NULL){
	  addToElements(&s->outputs,in);
	  freeElements(&in);
	  free(in);
	}
	OGRFeature::DestroyFeature( output );
      }
      cleanFetchSql(conf,0,outputs);
      OGRFeature::DestroyFeature( poFeature );
      cleanFetchSql(conf,0,res);
      return s;
    }
  }
  cleanFetchSql(conf,0,res);
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
int fetchServicesFromDb(registry* reg,maps* conf, xmlDocPtr doc, xmlNodePtr n,
			void (func) (registry *, maps *, xmlDocPtr, xmlNodePtr,
				     service *), int minimal ){
  int result=0;
  result=_init_sql(conf,"metadb");
  if(getMapFromMaps(conf,"lenv","dbIssue")!=NULL || result < 0)
    return -1;
  // Fetch every services
  OGRLayer *res=fetchSql(conf,0,META_SERVICES_LIST_ALL);
  if(res!=NULL){
    OGRFeature  *poFeature = NULL;
    const char *tmp1;
    poFeature = res->GetNextFeature();
    while( poFeature != NULL ){
      service* s=extractServiceFromDb(conf,poFeature->GetFieldAsString( 1 ),minimal);
#ifdef USE_HPC
      addNestedOutputs(&s);
#endif
      func(reg,conf,doc,n,s);
      freeService(&s);
      free(s);
      OGRFeature::DestroyFeature( poFeature );
      poFeature = res->GetNextFeature();
      result++;
    }
    cleanFetchSql(conf,0,res);
  }
  return result;
}

#endif
