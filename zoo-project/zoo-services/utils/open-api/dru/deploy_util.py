#
# Author : Blasco Brauzzi, Fabrice Brito, Frank Löschau
#
# Copyright 2023-2025 Terradue. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including with
# out limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import sys
import os
import json
import yaml
import re
from cwl_utils.parser import load_document as load_cwl
from cwl_utils.parser import *
import cwl_utils.__meta__ as cwl_meta
from cwl2ogc import BaseCWLtypes2OGCConverter

try:
    import zoo
except ImportError:
    print("Not running in zoo instance, using ZooStub object for testing")
    from ZooStub import ZooStub
    conf = {}
    conf["lenv"] = {"message": ""}
    zoo = ZooStub()
    pass

class Process:
    def __init__(
        self,
        identifier,
        version,
        title=None,
        description=None,
        store_supported=True,
        status_supported=True,
        service_type=None,
        service_provider=None,
    ):
        self.identifier = identifier
        self.version = version
        self.title = title or identifier
        if self.title:
            self.title = str(self.title)
        self.description = description or title
        if self.description:
            self.description = str(self.description)
        self.store_supported = store_supported
        self.status_supported = status_supported
        self.service_type = service_type
        self.service_provider = service_provider
        self.version = version
        self.metadata = None
        self.inputs = []
        self.outputs = []

    
    @classmethod
    def create_from_cwl(cls, cwl, workflow_id=None):
        """
        Creates a Process object from a dictionary representing the CWL YAML file.
        """
        if cwl_meta.__version__ < "0.16":
            cwl_obj = load_cwl(cwl)
        else:
            cwl_obj = load_cwl(cwl, id_=(workflow_id if workflow_id else "main"), load_all=True)

        workflows = [
            item for item in cwl_obj if item.class_ == 'Workflow'
        ]
        if len(workflows) == 0:
            raise Exception("No workflow found")
        if workflow_id is None:
            workflow = workflows[0]
        else:
            workflow = next(
                (wf for wf in workflows if wf.id.split("#")[1] == workflow_id), None
            )
            if workflow is None:
                raise Exception("Workflow '{0}' not found".format(workflow_id))

        version = 'unknown'
        root_metadata=None
        if workflow.extension_fields and 'https://schema.org/softwareVersion' in workflow.extension_fields:
            version = workflow.extension_fields['https://schema.org/softwareVersion']
        elif workflow.extension_fields and 'https://schema.org/version' in workflow.extension_fields:
            version = workflow.extension_fields['https://schema.org/version']
        else:
            # If software version is not specified inside the workflow,
            # get it from the top level (using legacy code)
            software_namespace_prefix = None
            if "$namespaces" in cwl and isinstance(cwl["$namespaces"], dict):
                for (prefix, uri) in cwl["$namespaces"].items():
                    if uri in [
                        "https://schema.org/SoftwareApplication",
                        "https://schema.org/",
                    ]:
                        software_namespace_prefix = prefix
            for key in ["softwareVersion", "version"]:
                key = "{0}:{1}".format(software_namespace_prefix, key)
                if key in cwl:
                    version = str(cwl[key])
                    break
            for item in cwl:
                search_str="{0}:".format(software_namespace_prefix)
                if item.count(search_str)>0:
                    if root_metadata is None:
                        root_metadata={}
                    root_metadata[item.replace(search_str,"https://schema.org/")]=cwl[item]

        id = re.sub(".*#", '', workflow.id)

        process = Process(
            identifier=id,
            version=version,
            title=workflow.label,
            description=workflow.doc,
        )

        if root_metadata is not None:
            process.metadata=root_metadata
        elif workflow.extension_fields:
            process.metadata=workflow.extension_fields

        process.cwl2ogc_converter = BaseCWLtypes2OGCConverter(workflow)

        return process


    def write_zcfg(self, stream):
        """
        Writes the configuration file for the Zoo process (.zfcg) to a stream.
        """

        print("[{0}]".format(self.identifier), file=stream)
        if self.title:
            print("  Title = {0}".format(self.title), file=stream)
        if self.description:
            print("  Abstract = {0}".format(self.description), file=stream)
        if self.service_provider:
            print("  serviceType = {0}".format(self.service_type), file=stream)
            print("  serviceProvider = {0}".format(self.service_provider), file=stream)
        if self.version:
            print("  processVersion = {0}".format(self.version), file=stream)
        print(
            "  storeSupported = {0}".format(
                "true" if self.store_supported else "false"
            ),
            file=stream,
        )
        print(
            "  statusSupported = {0}".format(
                "true" if self.status_supported else "false"
            ),
            file=stream,
        )
        print(
            "  mutable = {0}".format("true"),
            file=stream,
        )

        if self.metadata is not None:
            print("  <MetaData>", file=stream)
            cnt=0
            for item in self.metadata:
                rname="role"
                hname="href"
                if cnt > 0:
                    rname+="_"+str(cnt)
                    hname+="_"+str(cnt)
                if isinstance(self.metadata[item],str):
                    print(rname+" = "+item)
                    print(hname+" = "+self.metadata[item])
                    cnt+=1
                elif isinstance(self.metadata[item],list):
                    if len(self.metadata[item])>0 and isinstance(self.metadata[item][0],dict):
                        dcnt=0
                        print(rname+" = "+item)
                        cnt+=1
                        for icnt in len(self.metadata[item]):
                            srname=rname+"_role"
                            shname=hname+"_href"
                            srname1=None
                            shname1=None
                            if dcnt>0:
                                srname1=srname+"_"+str(dcnt+1)
                                shname1=shname+"_"+str(dcnt+1)
                                srname+="_"+str(dcnt)
                                shname+="_"+str(dcnt)
                            for subitem in self.metadata[item][icnt]:
                                if subitem=="class":
                                    print("    "+srname+" = @context")
                                    print("    "+shname+" = https://schema.org")
                                    print("    "+srname1+" = @type")
                                    print("    "+shname1+" = "+self.metadata[item][icnt][subitem].split(":")[1])
                                    dcnt+=1
                                else:
                                    if subitem.count(":")>0:
                                        print("    "+srname+" = "+subitem.split(":")[1])
                                    else:
                                        print("    "+srname+" = "+subitem)
                                    print("    "+shname+" = "+self.metadata[item][icnt][subitem])
                                dcnt+=1
                        print("    "+rname+"_length = "+str(dcnt))
            print("    length = "+str(cnt))
            print("  </MetaData>", file=stream)

        print("  <DataInputs>", file=stream)
        for input in self.inputs:
            print("    [{0}]".format(input.identifier), file=stream)
            print("      Title = {0}".format(input.title), file=stream)
            print("      Abstract = {0}".format(input.description), file=stream)
            print("      minOccurs = {0}".format(input.min_occurs), file=stream)
            print(
                "      maxOccurs = {0}".format(
                    999 if input.max_occurs == 0 else input.max_occurs
                ),
                file=stream,
            )
            if input.is_complex:
                pass
            else:
                print("      <LiteralData>", file=stream)
                print("        dataType = {0}".format(input.type), file=stream)
                if input.possible_values:
                    print(
                        "        AllowedValues = {0}".format(
                            ",".join(input.possible_values)
                        ),
                        file=stream,
                    )
                if input.default_value:
                    print("        <Default>", file=stream)
                    print(
                        "          value = {0}".format(input.default_value), file=stream
                    )
                    print("        </Default>", file=stream)
                else:
                    print("        <Default/>", file=stream)
                print("      </LiteralData>", file=stream)
        print("  </DataInputs>", file=stream)

        print("  <DataOutputs>", file=stream)
        for output in self.outputs:
            print("    [{0}]".format(output.identifier), file=stream)
            print("      Title = {0}".format(output.title), file=stream)
            print("      Abstract = {0}".format(output.description), file=stream)
            if output.is_complex:
                print("      <ComplexData>", file=stream)
                print("        <Default>", file=stream)
                print(
                    "          mimeType = {0}".format(
                        output.file_content_type
                        if output.file_content_type
                        else "text/plain"
                    ),
                    file=stream,
                )
                print("        </Default>", file=stream)
                print("      </ComplexData>", file=stream)
            else:
                print("      <LiteralData>", file=stream)
                print("        dataType = {0}".format(input.type), file=stream)
                print("        <Default/>", file=stream)
                print("      </LiteralData>", file=stream)
        print("  </DataOutputs>", file=stream)

    def run_sql(self, conf):
        """
        Store the metadata informations in the ZOO-Project database
        """
        import psycopg2
        import psycopg2.extensions
        psycopg2.extensions.register_type(psycopg2.extensions.UNICODE)
        if "auth_env" in conf:
            self.user=conf["auth_env"]["user"]
        else:
            self.user="anonymous"
        conn = psycopg2.connect("host=%s port=%s dbname=%s user=%s password=%s" % (conf["metadb"]["host"], conf["metadb"]["port"], conf["metadb"]["dbname"], conf["metadb"]["user"], conf["metadb"]["password"]))
        cur = conn.cursor()
        if "orequest_method" in conf["lenv"]:
            cur.execute("DELETE FROM collectiondb.ows_process WHERE identifier=$q$%s$q$ and user_id=(select id from public.users where name=$q$%s$q$)" % (self.identifier,self.user))
        conn.commit()
        cur.execute("SELECT id FROM collectiondb.ows_process WHERE identifier=$q$%s$q$ and user_id=(select id from public.users where name=$q$%s$q$)" % (self.identifier,self.user))
        vals = cur.fetchone()
        if vals is not None:
            conn.close()
            return False
        conn.commit()
        cur.execute(("INSERT INTO CollectionDB.zoo_DeploymentMetadata"+
                  "(executable_name,service_type_id)"+
                  " VALUES "+
                  " ($q${0}$q$,"+
                  "(SELECT id from CollectionDB.zoo_ServiceTypes WHERE service_type=$q${1}$q$));")
                  .format(self.service_provider,self.service_type))
        cur.execute("INSERT INTO CollectionDB.zoo_PrivateMetadata(id) VALUES (default);")
        cur.execute("INSERT INTO CollectionDB.PrivateMetadataDeploymentMetadataAssignment(private_metadata_id,deployment_metadata_id) VALUES"+
                  "((SELECT last_value FROM CollectionDB.zoo_PrivateMetadata_id_seq),"+
                  "(SELECT last_value FROM CollectionDB.zoo_DeploymentMetadata_id_seq));")
        try:
            cur.execute("SELECT id from public.users WHERE name = $q${0}$q$".format(self.user))
            if cur.fetchone() is None:
                cur.execute("INSERT INTO public.users (name) VALUES ($q${0}$q$)".format(self.user));
        except Exception as e:
            print(e,file=sys.stderr)
            cur.commit()
        cur.execute(("INSERT INTO CollectionDB.ows_Process"+
                  "(identifier,title,abstract,version,user_id,private_metadata_id,mutable,availability)"+
                  "VALUES"+
                  "($q${0}$q$,"+
                  "$q${1}$q$,"+
                  "$q${2}$q$,"+
                  "$q${3}$q$,"+
                  "(select id from public.users where name=$q${4}$q$),"+
                  "(SELECT last_value FROM CollectionDB.PrivateMetadataDeploymentMetadataAssignment_id_seq),"+
                  "true,true);").format(self.identifier,self.title,self.description,self.version,self.user))
        cur.execute("CREATE TEMPORARY TABLE pid AS (select last_value as id from CollectionDB.Descriptions_id_seq);")

        # Main Metadata extraction
        if self.metadata is not None:
            for item in self.metadata:
                if isinstance(self.metadata[item],dict):
                    self.metadata[item]=[self.metadata[item]]
                if isinstance(self.metadata[item],str):
                    cur.execute("INSERT INTO CollectionDB.ows_Metadata (role,href) "+
                                "VALUES ($q${0}$q$,$q${1}$q$)".format(item,self.metadata[item]))
                    cur.execute("INSERT INTO CollectionDB.DescriptionsMetadataAssignment (descriptions_id,metadata_id) "+
                                "VALUES ((select id from pid),(select last_value from CollectionDB.ows_Metadata_id_seq))")
                elif isinstance(self.metadata[item],list):
                    # Parse Keywords if any
                    if len(self.metadata[item])>0 and isinstance(self.metadata[item][0],str):
                        for icnt in range(len(self.metadata[item])):
                            res=cur.execute("INSERT INTO CollectionDB.ows_Keywords (keyword) "+
                                            "VALUES ($q${0}$q$)".format(self.metadata[item][icnt]));
                            cur.execute("INSERT INTO CollectionDB.DescriptionsKeywordsAssignment (descriptions_id,keywords_id) "+
                                        "VALUES ((select id from pid),(select last_value from CollectionDB.ows_Keywords_id_seq))")
                    elif len(self.metadata[item])>0 and isinstance(self.metadata[item][0],dict):
                        res=cur.execute("INSERT INTO CollectionDB.ows_Metadata (role) "+
                                        "VALUES ($q${0}$q$) RETURNING id".format(item))
                        metadata_id=cur.fetchone()
                        cur.execute("INSERT INTO CollectionDB.DescriptionsMetadataAssignment (descriptions_id,metadata_id) "+
                                    "VALUES ((select id from pid),(select last_value from CollectionDB.ows_Metadata_id_seq))")
                        for icnt in range(len(self.metadata[item])):
                            for subitem in self.metadata[item][icnt]:
                                if subitem=="class":
                                    cur.execute("INSERT INTO CollectionDB.ows_Metadata (role,href,pid,index) "+
                                                "VALUES ($q$@context$q$,$q$https://schema.org$q$,{0},{1})".format(
                                                    str(metadata_id[0]),
                                                    str(icnt)
                                                ))
                                    cur.execute("INSERT INTO CollectionDB.ows_Metadata (role,href,pid,index) "+
                                                "VALUES ($q$@type$q$,$q${0}$q$,{1},{2})".format(
                                                    self.metadata[item][icnt][subitem].split(":")[1]
                                                    if self.metadata[item][icnt][subitem].count(":")>0
                                                    else self.metadata[item][icnt][subitem],
                                                    str(metadata_id[0]),
                                                    str(icnt)
                                                ))
                                else:
                                    if subitem.count(":")>0:
                                        cur.execute("INSERT INTO CollectionDB.ows_Metadata (role,href,pid,index) "+
                                                    "VALUES ($q${0}$q$,$q${1}$q$,{2},{3})".format(
                                                        subitem.split(":")[1],
                                                        self.metadata[item][icnt][subitem],
                                                        str(metadata_id[0]),
                                                        str(icnt)
                                                    ))
                                    else:
                                        cur.execute("INSERT INTO CollectionDB.ows_Metadata (role,href,pid,index) "+
                                                    "VALUES ($q${0}$q$,$q${1}$q$,{2},{3})".format(
                                                        subitem,
                                                        self.metadata[item][icnt][subitem],
                                                        str(metadata_id[0]),
                                                        str(icnt)
                                                    ))

        # Inputs handling
        myInputs = self.cwl2ogc_converter.get_inputs()
        for input in myInputs:
            # zoo.debug(str(myInputs[input]))
            self.sql_handle_io(cur,myInputs[input],input,"Input")
            cur.execute(("INSERT INTO CollectionDB.ows_Input (identifier,title,abstract,min_occurs,max_occurs,raw_schema) VALUES "+
                      "($q${0}$q$,"+
                      "$q${1}$q$,"+
                      "$q${2}$q$,"+
                      "{3},"+
                      "{4},"+
                      "$q${5}$q$);").format(input,
                                myInputs[input].get("title",None),
                                myInputs[input].get("description",None),
                                myInputs[input].get("minOccurs",None),
                                999 if myInputs[input]["maxOccurs"] == 0 else myInputs[input]["maxOccurs"],
                                json.dumps(myInputs[input].get("schema",None))
                      ))
            cur.execute("INSERT INTO CollectionDB.InputDataDescriptionAssignment (input_id,data_description_id)"+
                        "VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),"+
                        "(select last_value from CollectionDB.ows_DataDescription_id_seq));");
            cur.execute("INSERT INTO CollectionDB.ProcessInputAssignment(process_id,input_id)"+
                        "VALUES((select id from pid),(select last_value as id from CollectionDB.Descriptions_id_seq));")
            for i in range(len(myInputs[input]["metadata"])):
                self.sql_add_metadata(cur, myInputs[input]["metadata"][i])

        # Output handling
        myOutputs = self.cwl2ogc_converter.get_outputs()
        for output in myOutputs:
            # zoo.debug(str(myOutputs[output]))
            self.sql_handle_io(cur,myOutputs[output],output,"Output")
            cur.execute("INSERT INTO CollectionDB.ows_Output"+
                      "(identifier,title,abstract,raw_schema)"+
                      " VALUES "+
                      "($q${0}$q$,$q${1}$q$,$q${2}$q$,$q${3}$q$);".format(output,
                                                                myOutputs[output].get("title",None),
                                                                myOutputs[output].get("description",None),
                                                                json.dumps(myOutputs[output].get("schema",None))
                                                            )
                      )
            cur.execute("INSERT INTO CollectionDB.OutputDataDescriptionAssignment (output_id,data_description_id) "+
                        "VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),"+
                        "(select last_value from CollectionDB.ows_DataDescription_id_seq));")
            # zoo.info(f"Using {local_format} as output format")
            cur.execute("INSERT INTO CollectionDB.ProcessOutputAssignment(process_id,output_id) "+
                        "VALUES((select id from pid),(select last_value as id from CollectionDB.Descriptions_id_seq));")
            for i in range(len(myOutputs[output]["metadata"])):
                self.sql_add_metadata(cur, myOutputs[output]["metadata"][i])

        cur.execute("DROP TABLE pid;")
        conn.commit()
        conn.close()
        return True

    def handle_oneof(self,cur,io_entity):
        for i in range(len(io_entity["oneOf"])):
            if "$id" in io_entity["oneOf"][i]:
                schema=io_entity["oneOf"][i]["$id"]
                # This is a workaround for schema with anchors
                if schema.count("#")>0:
                    schema=schema.split("#")[0]
                cur.execute("SELECT namespace|| '#' || cwl_type from CollectionDB.PrimitiveDataFormats "+
                            "where schemaUrl=$q${0}$q$ LIMIT 1".format(schema))
                val= cur.fetchone()
                if val is not None:
                    self.sql_add_complex_data(cur, val[0])
                else:
                    zoo.error("No format found for schema "+schema)
                # do not break to use the last option as complex data type

    def sql_handle_io(self,cur,io_entity,name,str_type):
        #zoo.debug(f"Handling {str_type} {name} with schema {io_entity['schema']}")
        if io_entity["metadata"][0]["value"].count("#BBox") > 0:
            cur.execute("INSERT INTO CollectionDB.BoundingBoxData (format_id) VALUES (NULL);")
        else:
            if "oneOf" in io_entity["schema"]:
                self.handle_oneof(cur,io_entity["schema"])
            elif io_entity["schema"]["type"] != "array":
                if io_entity["schema"]["type"] != "object":
                    self.sql_add_literal_data_domain(cur, io_entity["schema"])
                else:
                    if io_entity["metadata"][0]["value"].count("#")>0:
                        self.sql_add_complex_data(cur, io_entity["metadata"][0]["value"])
            else:
                io_entity["maxOccurs"] = 0
                if "oneOf" in io_entity["schema"]["items"]:
                    self.handle_oneof(cur,io_entity["schema"]["items"])
                elif io_entity["schema"]["items"]["type"] != "object":
                    self.sql_add_literal_data_domain(cur, io_entity["schema"]["items"])
                else:
                    if io_entity["metadata"][0]["value"].count("#")>0:
                        self.sql_add_complex_data(cur, io_entity["metadata"][0]["value"])

            if "enum" in io_entity["schema"]:
                for i in range(len(io_entity["schema"]["enum"])):
                    cur.execute("INSERT INTO CollectionDB.AllowedValues (allowed_value) VALUES ($q${0}$q$);".format(io_entity["schema"]["enum"][i]))
                    cur.execute("INSERT INTO CollectionDB.AllowedValuesAssignment (literal_data_domain_id,allowed_value_id) VALUES ("+
                                    "(select last_value as id from CollectionDB.ows_DataDescription_id_seq),"+
                                    "(select last_value as id from CollectionDB.AllowedValues_id_seq)"
                                    ");")

            if "default" in io_entity["schema"]:
                cur.execute("UPDATE CollectionDB.LiteralDataDomain"+
                                " set default_value = $q${0}$q$ ".format(io_entity["schema"]["default"])+
                                " WHERE id = "+
                                "  ((SELECT last_value FROM CollectionDB.ows_DataDescription_id_seq));")

    def sql_add_metadata(self, cur, metadata_object):
        cur.execute("INSERT INTO CollectionDB.ows_Metadata (title,href) VALUES "+
                    "($q${0}$q$,$q${1}$q$);".format(
                        metadata_object["title"],
                        metadata_object["value"]
                    )
        )
        cur.execute("INSERT INTO CollectionDB.DescriptionsMetadataAssignment (descriptions_id,metadata_id) "+
                "VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),"+
                "(select last_value from CollectionDB.ows_Metadata_id_seq));")

    def sql_add_literal_data_domain(self, cur, primitive_data):
        if "format" in primitive_data:
            try:
                cur.execute("SELECT id from CollectionDB.PrimitiveDatatypes "+
                            "where name = $q${0}$q$;".format(primitive_data["format"]))
                val=cur.fetchone()
                if val is None:
                    cur.execute("SELECT id from CollectionDB.PrimitiveDatatypes "+
                                "where name = $q${0}$q$;".format(primitive_data["type"]))
                    val=cur.fetchone()
            except Exception as e:
                zoo.error(f"Error fetching primitive datatype: {e}")
            if val is not None:
                cur.execute("INSERT INTO CollectionDB.LiteralDataDomain (def,data_type_id) VALUES "+
                            "(true,{0});".format(val[0]))
        else:
            cur.execute("INSERT INTO CollectionDB.LiteralDataDomain (def,data_type_id) VALUES "+
                        "(true,(SELECT id from CollectionDB.PrimitiveDatatypes where name = $q${0}$q$));".format(primitive_data["type"]))

    def sql_add_complex_data(self, cur, complex_data):
        # myInputs[input]["metadata"][0]["value"] = <complex_data>
        parsed_str=complex_data.split("#")[0].split("/")
        usable_str=parsed_str[len(parsed_str)-1]
        cur.execute("SELECT id FROM CollectionDB.PrimitiveDataFormats "+
                    "where namespace=$q${0}$q$ and cwl_type=$q${1}$q$ LIMIT 1".format(usable_str,
                                                                                    complex_data.split("#")[1]))
        val=cur.fetchone()
        # zoo.debug(f"Found format: {val}")
        # Use a static mimeType for now
        current_content_type="application/json"
        cur.execute("SELECT id from CollectionDB.PrimitiveFormats WHERE mime_type='{0}' LIMIT 1".
                            format(current_content_type))
        val1=cur.fetchone()
        cur.execute("INSERT INTO CollectionDB.ows_Format (def,primitive_format_id) VALUES "+
                    "(true,{0});".format(val1[0]))
        cur.execute(f"INSERT INTO CollectionDB.ows_DataDescription (format_id,data_format_id) VALUES ((SELECT last_value FROM CollectionDB.ows_Format_id_seq),{val[0]});")


class Services(object):

    def __init__(self, conf, inputs, outputs):

        self.conf = conf
        self.inputs = inputs
        self.outputs = outputs
        self.zooservices_folder = self.get_zoo_services_folder()


    def get_zoo_services_folder(self):

        # checking for namespace
        if (
            "zooServicesNamespace" in self.conf
            and "namespace" in self.conf["zooServicesNamespace"]
            and "servicesNamespace" in self.conf
            and "path" in self.conf["servicesNamespace"]
        ):
            zoo.info(
                f"Using namespace {self.conf['zooServicesNamespace']['namespace']}"
            )
            zooservices_folder = os.path.join(
                self.conf["servicesNamespace"]["path"],
                self.conf["zooServicesNamespace"]["namespace"],
            )
        else:
            # if no namespace is used, we will use the default services path
            zooservices_folder = self._get_conf_value(
                key="CONTEXT_DOCUMENT_ROOT", section="renv"
            )
            zoo.info(f"Using default namespace {zooservices_folder}")

        # Checking if zoo can write in the servicePath
        self.check_write_permissions(zooservices_folder)

        return zooservices_folder

    def _get_conf_value(self, key, section="main"):

        if key in self.conf[section].keys():
            return self.conf[section][key]
        else:
            zoo.error(f"{key} not set, check configuration")
            raise ValueError(f"{key} not set, check configuration")

    def _get_conf_value_if_exists(self, key, section="main"):

        if key in self.conf[section].keys():
            return self.conf[section][key]
        else:
            return None

    def get_process_identifier(self):
        process_identifier = self.conf["lenv"]["deployedServiceId"]
        return process_identifier

    @staticmethod
    def check_write_permissions(folder):

        if not os.access(folder, os.W_OK):
            errorMsg = f"Cannot write to {folder}. Please check folder"
            zoo.error(errorMsg)
            raise Exception(errorMsg)

