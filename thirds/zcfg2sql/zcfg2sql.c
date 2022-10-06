/*
 * Author : Gérald FENOY
 *
 *  Copyright (c) 2017-2019 GeoLabs SARL. All rights reserved.
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

#include "service.h"

extern int getServiceFromFile (maps *, const char *, service **);

/**
 * Dump an elements on stderr using the SQL syntaxe
 *
 * @param e the elements to dump
 */
void dumpElementsAsSQL(const char* type,elements* e,int index,int level,FILE* f){
  elements* tmp=e;
  int i;
  while(tmp!=NULL){
    fprintf(f,"--\n-- %s %s \n--\n",type,tmp->name);
    map* mcurs=tmp->content;
    iotype* tmpio=tmp->defaults;
    int ioc=0;
    if(tmp->format!=NULL){
      //fprintf(stderr,"%s:\n",tmp->format);
      if(strncmp(tmp->format,"Complex",7)==0){
	while(tmpio!=NULL){
	  map* values[3]={
	    getMap(tmpio->content,"mimeType"),
	    getMap(tmpio->content,"encoding"),
	    getMap(tmpio->content,"schema")
	  };
	  char cond[3][256]={
	    "mime_type is NULL",
	    "encoding is NULL",
	    "schema is NULL"
	  };
	  char *condNames[3]={
	    (char*)"mime_type",
	    (char*)"encoding",
	    (char*)"schema"
	  };
	  for(i=0;i<3;i++)
	    if(values[i]!=NULL){
	      sprintf(cond[i],"%s='%s'",condNames[i],values[i]->value);
	    }
	  fprintf(f,"INSERT INTO CollectionDB.ows_Format"
		  " (def,primitive_format_id) VALUES \n(true,");
	  fprintf(f,"(SELECT id from CollectionDB.PrimitiveFormats"
		  " WHERE %s AND %s AND %s));\n",cond[0],cond[1],cond[2]);
	  fprintf(f,"INSERT INTO CollectionDB.ows_DataDescription (format_id)"
		  " VALUES ((SELECT last_value FROM CollectionDB.ows_Format_id_seq));\n");
	  tmpio=tmpio->next;
	  ioc++;
	}
      }else{
	if(strncmp(tmp->format,"Literal",7)==0){
	  while(tmpio!=NULL){
	    map* values[4]={
	      getMap(tmpio->content,"dataType"),
	      getMap(tmpio->content,"value"),
	      getMap(tmpio->content,"uom"),
	      getMap(tmpio->content,"AllowedValues")
	    };
	    char *fields[20]={
	      (char*)"default_value",
	      (char*)"def"
	    };
	    fprintf(f,"INSERT INTO CollectionDB.LiteralDataDomain"
		    " (def,data_type_id) VALUES \n(true,");
	    fprintf(f,"(SELECT id from CollectionDB.PrimitiveDatatypes"
		    " where name = '%s'));\n",values[0]->value);
	    if(values[1]!=NULL)
	      fprintf(f,"UPDATE CollectionDB.LiteralDataDomain \n"
		      "set %s = $q$%s$q$ \n WHERE id = \n"
		      "  ((SELECT last_value FROM CollectionDB.ows_DataDescription_id_seq));\n ",
		      fields[0],values[1]->value);
	    if(values[2]!=NULL)
	      fprintf(f,"UPDATE CollectionDB.LiteralDataDomain \n"
		      "set uom = (SELECT id from CollectionDB.PrimitiveUOM WHERE uom=$q$%s$q$) \n WHERE id = \n"
		      "  (SELECT last_value FROM CollectionDB.ows_DataDescription_id_seq);\n ",
		      values[2]->value);
	    if(values[3]!=NULL){
	      char *tmp=strtok(values[3]->value,",");
	      while(tmp!=NULL){
		fprintf(f,"INSERT INTO CollectionDB.AllowedValues (allowed_value) VALUES ('%s');\n",tmp);
		fprintf(f,"INSERT INTO CollectionDB.AllowedValuesAssignment"
			" (allowed_value_id,literal_data_domain_id) VALUES "
			"((SELECT last_value FROM CollectionDB.AllowedValues_id_seq),"
			"(SELECT last_value FROM CollectionDB.ows_DataDescription_id_seq));\n");
		tmp=strtok(NULL,",");
	      }
	    }
	    tmpio=tmpio->next;
	    ioc++;
	  }	  
	}
      }

    }
    
    map* values[4]={
      getMap(tmp->content,"Title"),
      getMap(tmp->content,"Abstract"),
      getMap(tmp->content,"minOccurs"),
      getMap(tmp->content,"maxOccurs")
    };
    char *toto=(char*)",min_occurs,max_occurs";
    if(strcmp(type,"Input")==0){
      fprintf(f,"INSERT INTO CollectionDB.ows_Input "
	      "(identifier,title,abstract,min_occurs,max_occurs)"
	      " VALUES "
	      "($q$%s$q$,\n$q$%s$q$,\n$q$%s$q$,\n%s,\n%s);\n",
	      tmp->name,
	      values[0]->value,
	      values[1]->value,
	      values[2]->value,
	      values[3]->value
	      );
      if(tmp->format!=NULL)
	fprintf(f,"INSERT INTO CollectionDB.InputDataDescriptionAssignment"
		" (input_id,data_description_id) VALUES "
		"((select last_value as id "
	        "from CollectionDB.Descriptions_id_seq),"
		"(select last_value "
		"from CollectionDB.ows_DataDescription_id_seq));\n");
      if(level==0)
	fprintf(f,"INSERT INTO CollectionDB.ProcessInputAssignment"
		"(process_id,input_id)"
		" VALUES"
		"((select id from pid),"
		"(select last_value as id from CollectionDB.Descriptions_id_seq));\n");
      else
	fprintf(f,"INSERT INTO CollectionDB.InputInputAssignment"
		"(parent_input,child_input)"
		" VALUES"
		"((select id from cid_%d),"
		"(select last_value as id from CollectionDB.Descriptions_id_seq));\n",level-1);
    }
    else{
      fprintf(f,"INSERT INTO CollectionDB.ows_Output\n"
	      "\t(identifier,title,abstract)\n"
	      "\t VALUES \n"
	      "($q$%s$q$,$q$%s$q$,$q$%s$q$);\n",
	      tmp->name,
	      values[0]->value,
	      values[1]->value
	      );
      if(tmp->format!=NULL)
	fprintf(f,"INSERT INTO CollectionDB.OutputDataDescriptionAssignment"
		" (output_id,data_description_id) VALUES "
		"((select last_value as id "
	        "from CollectionDB.Descriptions_id_seq),"
		"(select last_value "
		"from CollectionDB.ows_DataDescription_id_seq));\n");
      if(level==0)
	fprintf(f,"INSERT INTO CollectionDB.ProcessOutputAssignment"
		"(process_id,output_id)"
		" VALUES"
		"((select id from pid),"
		"(select last_value as id from CollectionDB.Descriptions_id_seq));\n");
      else
	fprintf(f,"INSERT INTO CollectionDB.OutputOutputAssignment"
		"(parent_output,child_output)"
		" VALUES"
		"((select id from cid_%d),"
		"(select last_value as id from CollectionDB.Descriptions_id_seq));\n",level-1);
    }

    map* mcurs1=tmp->metadata;
    if(mcurs1!=NULL){
      while(mcurs1!=NULL){
	if(strncasecmp(mcurs1->name,"title",5)==0){
	  fprintf(stdout,"INSERT INTO CollectionDB.ows_Metadata (title) "
		  "VALUES ($q$%s$q$);\n",mcurs->value);
	tryagain0:
	  if(mcurs1->next!=NULL){
	    if(strncasecmp(mcurs->next->name,"role",4)==0){
	      fprintf(stdout,"UPDATE CollectionDB.ows_Metadata set role = \n"
		      "$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_Metadata_id_seq);\n",mcurs1->next->value);
	      mcurs1=mcurs1->next;
	      goto tryagain0;
	    }
	    else{
	      if(strncasecmp(mcurs1->next->name,"href",4)==0){
		fprintf(stdout,"UPDATE CollectionDB.ows_Metadata set href = \n"
			"$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_Metadata_id_seq);\n",mcurs1->next->value);
		mcurs1=mcurs1->next;
		goto tryagain0;
	      }
	    }
	  }
	  fprintf(stdout,"INSERT INTO CollectionDB.DescriptionsMetadataAssignment (descriptions_id,metadata_id) "
		  "VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),"
		  "(SELECT last_value FROM CollectionDB.ows_Metadata_id_seq));\n");	  
	}
	mcurs1=mcurs1->next;
      }
    }
    mcurs=tmp->additional_parameters;
    if(mcurs!=NULL){
      while(mcurs!=NULL){
	if(strncasecmp(mcurs->name,"title",5)==0){
	  fprintf(stdout,"INSERT INTO CollectionDB.ows_AdditionalParameters (title) "
		  "VALUES ($q$%s$q$);\n",mcurs->value);
	tryagain:
	  if(mcurs->next!=NULL){
	    if(strncasecmp(mcurs->next->name,"role",4)==0){
	      fprintf(stdout,"UPDATE CollectionDB.ows_AdditionalParameters set role = \n"
		      "$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq);\n",mcurs->next->value);
	      mcurs=mcurs->next;
	      goto tryagain;
	    }
	    else{
	      if(strncasecmp(mcurs->next->name,"href",4)==0){
		fprintf(stdout,"UPDATE CollectionDB.ows_AdditionalParameters set href = \n"
			"$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq);\n",mcurs->next->value);
		mcurs=mcurs->next;
		goto tryagain;
	      }
	    }
	  }
	  fprintf(stdout,"INSERT INTO CollectionDB.DescriptionsAdditionalParametersAssignment (descriptions_id,additional_parameters_id) "
		  "VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),"
		  "(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq));\n");	  
	}else{
	  fprintf(stdout,"INSERT INTO CollectionDB.ows_AdditionalParameter (key,value,additional_parameters_id) "
		  "VALUES ($q$%s$q$,$q$%s$q$,"
		  "(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq));\n",mcurs->name,mcurs->value);	  
	}
	mcurs=mcurs->next;
      }
    }

    if(tmp->format!=NULL){
      tmpio=tmp->supported;
      if(strncmp(tmp->format,"Complex",7)==0){
	while(tmpio!=NULL){
	  map* values[3]={
	    getMap(tmpio->content,"mimeType"),
	    getMap(tmpio->content,"encoding"),
	    getMap(tmpio->content,"schema")
	  };
	  char cond[3][256]={
	    "mime_type is NULL",	    
	    "encoding is NULL",
	    "schema is NULL",
	  };
	  char *condNames[3]={
	    (char*)"mime_type",
	    (char*)"encoding",
	    (char*)"schema"
	  };
	  for(i=0;i<3;i++)
	    if(values[i]!=NULL){
	      sprintf(cond[i],"%s='%s'",condNames[i],values[i]->value);
	    }
	  fprintf(f,"INSERT INTO CollectionDB.ows_Format"
		  " (def,primitive_format_id) VALUES \n(false,");
	  fprintf(f,"(SELECT id from CollectionDB.PrimitiveFormats"
		  " WHERE %s AND %s AND %s));\n",cond[0],cond[1],cond[2]);
	  fprintf(f,"INSERT INTO CollectionDB.ows_DataDescription (format_id)"
		  " VALUES ((SELECT last_value FROM CollectionDB.ows_Format_id_seq));\n");
	  fprintf(f,"INSERT INTO CollectionDB.%sDataDescriptionAssignment"
		  " (%s_id,data_description_id) VALUES "
		  "((select last_value as id "
		  "from CollectionDB.Descriptions_id_seq),"
		  "(select last_value "
		  "from CollectionDB.ows_DataDescription_id_seq));\n",
		  type,type);	  
	  tmpio=tmpio->next;
	  ioc++;
	}
      }else{
	if(strncmp(tmp->format,"Literal",7)==0){
	  while(tmpio!=NULL){
	    map* values[4]={
	      getMap(tmpio->content,"dataType"),
	      getMap(tmpio->content,"value"),
	      getMap(tmpio->content,"uom"),
	      getMap(tmpio->content,"AllowedValues")
	    };
	    map* values0[4]={
	      getMap(tmp->defaults->content,"dataType"),
	      getMap(tmp->defaults->content,"value"),
	      getMap(tmp->defaults->content,"uom"),
	      getMap(tmp->defaults->content,"AllowedValues")
	    };
	    char *fields[20]={
	      (char*)"default_value",
	      (char*)"def"
	    };
	    fprintf(f,"INSERT INTO CollectionDB.LiteralDataDomain"
		    " (def,data_type_id) VALUES \n(false,");
	    if(values[0]!=NULL)
	      fprintf(f,"(SELECT id from CollectionDB.PrimitiveDatatypes"
		      " where name = '%s'));\n",values[0]->value);
	    else
	      fprintf(f,"(SELECT id from CollectionDB.PrimitiveDatatypes"
		      " where name = '%s'));\n",values0[0]->value);
	    if(values[1]!=NULL)
	      fprintf(f,"UPDATE CollectionDB.LiteralDataDomain \n"
		      "set %s = $q$%s$q$ \n WHERE id = \n"
		      "  ((SELECT last_value FROM CollectionDB.ows_DataDescription_id_seq));\n ",
		      fields[0],values[1]->value);
	    if(values[2]!=NULL)
	      fprintf(f,"UPDATE CollectionDB.LiteralDataDomain \n"
		      "set uom = (SELECT id from CollectionDB.PrimitiveUOM WHERE uom=$q$%s$q$) \n WHERE id = \n"
		      "  (SELECT last_value FROM CollectionDB.ows_DataDescription_id_seq);\n ",
		      values[2]->value);
	    if(type!=NULL)
	    fprintf(f,"INSERT INTO CollectionDB.%sDataDescriptionAssignment"
		    " (%s_id,data_description_id) VALUES "
		    "((select last_value "
		    "from CollectionDB.Descriptions_id_seq),"
		    "(select last_value "
		    "from CollectionDB.ows_DataDescription_id_seq));\n",
		    type,type);	  
	    tmpio=tmpio->next;
	    ioc++;
	  }	  
	}
      }
    }

    if(tmp->format==NULL){
      fprintf(f,"--\n-- Child %d \n--\n",level);
      fprintf(f,"CREATE TEMPORARY TABLE cid_%d AS (select last_value as id from CollectionDB.Descriptions_id_seq) ;\n",level);
      if(tmp->child!=NULL)
	dumpElementsAsSQL(type,tmp->child,0,level+1,f);
      fprintf(f,"DROP TABLE cid_%d;\n",level);
      fprintf(f,"--\n-- End Child %d \n--\n",level);
    }
    tmp=tmp->next;
  }
}


int main(int argc, char *argv[]) {
  service* s=NULL;
  maps *m=NULL;
  char conf_file[1024];
  snprintf(conf_file,1024,"%s",argv[1]);
  s=(service*)malloc(SERVICE_SIZE);
  if(s == NULL){
    fprintf(stderr,"Unable to allocate memory");
    return -1;
  }
  int t=getServiceFromFile(m,conf_file,&s);

  fprintf(stdout,"--\n-- Load from %s %d \n--",__FILE__,__LINE__);
  int i;
  fprintf(stdout,"--\n-- Service %s\n--\n",s->name);
  if(s->content!=NULL){
    map* values[4]={
      getMap(s->content,"Title"),
      getMap(s->content,"Abstract"),
      getMap(s->content,"serviceProvider"),
      getMap(s->content,"serviceType")
    };
    fprintf(stdout,"INSERT INTO CollectionDB.zoo_DeploymentMetadata\n"
	    "(executable_name,service_type_id)\n"
	    "\nVALUES\n"
	    "($q$%s$q$,\n(SELECT id from CollectionDB.zoo_ServiceTypes WHERE service_type=$q$%s$q$));\n\n",
	    values[2]->value,
	    values[3]->value);
    fprintf(stdout,"INSERT INTO CollectionDB.zoo_PrivateMetadata"
	    "(id) VALUES (default);\n"); 
    fprintf(stdout,"INSERT INTO CollectionDB.PrivateMetadataDeploymentMetadataAssignment"
	    "(private_metadata_id,deployment_metadata_id) VALUES \n"
	    "((SELECT last_value FROM CollectionDB.zoo_PrivateMetadata_id_seq),\n"
	    "(SELECT last_value FROM CollectionDB.zoo_DeploymentMetadata_id_seq));\n");
    fprintf(stdout,"INSERT INTO CollectionDB.ows_Process\n"
	    "(identifier,title,abstract,private_metadata_id,"
	    "availability)\n"
	    "\nVALUES\n"
	    "($q$%s$q$,\n$q$%s$q$,\n$q$%s$q$,(SELECT last_value FROM CollectionDB.PrivateMetadataDeploymentMetadataAssignment_id_seq),\ntrue);\n\n",
	    s->name,
	    values[0]->value,
	    values[1]->value);
    fprintf(stdout,"CREATE TEMPORARY TABLE pid AS "
	    "(select last_value as id from CollectionDB.Descriptions_id_seq);\n");
    map* mcurs=s->content;
    //dumpMap(mcurs);
    mcurs=s->metadata;
    if(mcurs!=NULL){
      while(mcurs!=NULL){
	if(strncasecmp(mcurs->name,"title",5)==0){
	  fprintf(stdout,"INSERT INTO CollectionDB.ows_Metadata (title) "
		  "VALUES ($q$%s$q$);\n",mcurs->value);
	tryagain0:
	  if(mcurs->next!=NULL){
	    if(strncasecmp(mcurs->next->name,"role",4)==0){
	      fprintf(stdout,"UPDATE CollectionDB.ows_Metadata set role = \n"
		      "$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_Metadata_id_seq);\n",mcurs->next->value);
	      mcurs=mcurs->next;
	      goto tryagain0;
	    }
	    else{
	      if(strncasecmp(mcurs->next->name,"href",4)==0){
		fprintf(stdout,"UPDATE CollectionDB.ows_Metadata set href = \n"
			"$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_Metadata_id_seq);\n",mcurs->next->value);
		mcurs=mcurs->next;
		goto tryagain0;
	      }
	    }
	  }
	  fprintf(stdout,"INSERT INTO CollectionDB.DescriptionsMetadataAssignment (descriptions_id,metadata_id) "
		  "VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),"
		  "(SELECT last_value FROM CollectionDB.ows_Metadata_id_seq));\n");	  
	}
	mcurs=mcurs->next;
      }
    }
    mcurs=s->additional_parameters;
    if(mcurs!=NULL){
      while(mcurs!=NULL){
	if(strncasecmp(mcurs->name,"title",5)==0){
	  fprintf(stdout,"INSERT INTO CollectionDB.ows_AdditionalParameters (title) "
		  "VALUES ($q$%s$q$);\n",mcurs->value);
	tryagain:
	  if(mcurs->next!=NULL){
	    if(strncasecmp(mcurs->next->name,"role",4)==0){
	      fprintf(stdout,"UPDATE CollectionDB.ows_AdditionalParameters set role = \n"
		      "$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq);\n",mcurs->next->value);
	      mcurs=mcurs->next;
	      goto tryagain;
	    }
	    else{
	      if(strncasecmp(mcurs->next->name,"href",4)==0){
		fprintf(stdout,"UPDATE CollectionDB.ows_AdditionalParameters set href = \n"
			"$q$%s$q$ where id=(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq);\n",mcurs->next->value);
		mcurs=mcurs->next;
		goto tryagain;
	      }
	    }
	  }
	  fprintf(stdout,"INSERT INTO CollectionDB.DescriptionsAdditionalParametersAssignment (descriptions_id,additional_parameters_id) "
		  "VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),"
		  "(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq));\n");	  
	}else{
	  fprintf(stdout,"INSERT INTO CollectionDB.ows_AdditionalParameter (key,value,additional_parameters_id) "
		  "VALUES ($q$%s$q$,$q$%s$q$,"
		  "(SELECT last_value FROM CollectionDB.ows_AdditionalParameters_id_seq));\n",mcurs->name,mcurs->value);	  
	}
	mcurs=mcurs->next;
      }
    }
    
  }
  if(s->inputs!=NULL){
    //fprintf(stderr,"\ninputs:\n");
    dumpElementsAsSQL("Input",s->inputs,0,0,stdout);
  }
  if(s->outputs!=NULL){
    //fprintf(stderr,"\noutputs:\n");
    dumpElementsAsSQL("Output",s->outputs,0,0,stdout);
  }
  fprintf(stdout,"--\n-- Load from %s %d \n--",__FILE__,__LINE__);

  
  
  /*fprintf(stderr,"--\n-- Load from %s %d \n--",__FILE__,__LINE__);
  fflush(stderr);
  printf("--\n-- Load from %s %d \n--",__FILE__,__LINE__);
  if(t>=0){
    fprintf(stderr,"--\n-- Service %s\n--",s->name);
    fflush(stderr);
    dumpServiceAsSQL(s); 
    fprintf(stderr,"--\n-- Service %s\n--",s->name);
    fflush(stderr);
  }
  */
  fflush(stderr);
  return 0;
}
