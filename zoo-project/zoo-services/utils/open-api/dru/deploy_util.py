#
# Author : Blasco Brauzzi, Fabrice Brito, Frank Löschau
#
# Copyright 2023-2024 Terradue. All rights reserved.
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
        cwl_obj = load_cwl(cwl)

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

        process.add_inputs_from_cwl(workflow.inputs, len(workflow.id))
        process.add_outputs_from_cwl(workflow.outputs, len(workflow.id))

        return process


    def add_inputs_from_cwl(self, inputs, trim_len):
        """
        Adds a process input from a CWL input representation.
        """

        for input in inputs:
            process_input = ProcessInput.create_from_cwl(input, trim_len)
            self.inputs.append(process_input)

    def add_outputs_from_cwl(self, outputs, trim_len):
        """
        Adds a process output from a CWL input representation.
        """
        for output in outputs:
            process_output = ProcessOutput.create_from_cwl(output, trim_len)
            self.outputs.append(process_output)

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

        # Inputs treatment
        for input in self.inputs:
            if input.is_complex:
                current_content_type = input.file_content_type if input.file_content_type else "text/plain"
                cur.execute("SELECT id from CollectionDB.PrimitiveFormats WHERE mime_type='{0}' LIMIT 1".
                            format(current_content_type))
                val=cur.fetchone()
                if val is None:
                    cur.execute("INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('{0}')".
                                format(current_content_type))
                cur.execute("INSERT INTO CollectionDB.ows_Format (def,primitive_format_id) VALUES "+
                            "(true,(SELECT id from CollectionDB.PrimitiveFormats WHERE mime_type='{0}' LIMIT 1));".
                            format(current_content_type))
                if input.namespace is not None:
                    cur.execute(f"INSERT INTO CollectionDB.ows_DataDescription (format_id,data_format_id) VALUES ((SELECT last_value FROM CollectionDB.ows_Format_id_seq),(SELECT id from CollectionDB.PrimitiveDataFormats where cwl_type=$q${input.type}$q$));")
                else:
                    cur.execute("INSERT INTO CollectionDB.ows_DataDescription (format_id) VALUES ((SELECT last_value FROM CollectionDB.ows_Format_id_seq));")
            else:
                if input.is_bbox:
                    cur.execute("INSERT INTO CollectionDB.BoundingBoxData (format_id) VALUES (NULL);")

                else:
                    cur.execute("INSERT INTO CollectionDB.LiteralDataDomain (def,data_type_id) VALUES "+
                            "(true,(SELECT id from CollectionDB.PrimitiveDatatypes where name = $q${0}$q$));".format(input.type))
                    if input.possible_values:
                        for i in range(len(input.possible_values)):
                            cur.execute("INSERT INTO CollectionDB.AllowedValues (allowed_value) VALUES ($q${0}$q$);".format(input.possible_values[i]))
                            cur.execute("INSERT INTO CollectionDB.AllowedValuesAssignment (literal_data_domain_id,allowed_value_id) VALUES ("+
                                            "(select last_value as id from CollectionDB.ows_DataDescription_id_seq),"+
                                            "(select last_value as id from CollectionDB.AllowedValues_id_seq)"
                                            ");")
                    if input.default_value:
                        cur.execute("UPDATE CollectionDB.LiteralDataDomain"+
                                        " set default_value = $q${0}$q$ ".format(input.default_value)+
                                        " WHERE id = "+
                                        "  ((SELECT last_value FROM CollectionDB.ows_DataDescription_id_seq));")

            cur.execute(("INSERT INTO CollectionDB.ows_Input (identifier,title,abstract,min_occurs,max_occurs) VALUES "+
                      "($q${0}$q$,"+
                      "$q${1}$q$,"+
                      "$q${2}$q$,"+
                      "{3},"+
                      "{4});").format(input.identifier,
                                         input.title,
                                         input.description,
                                         input.min_occurs,
                                         999 if input.max_occurs == 0 else input.max_occurs))
            cur.execute("INSERT INTO CollectionDB.InputDataDescriptionAssignment (input_id,data_description_id) VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),(select last_value from CollectionDB.ows_DataDescription_id_seq));");
            cur.execute("INSERT INTO CollectionDB.ProcessInputAssignment(process_id,input_id) VALUES((select id from pid),(select last_value as id from CollectionDB.Descriptions_id_seq));")


        # Output treatment
        for output in self.outputs:
            if output.is_complex:
                cur.execute("INSERT INTO CollectionDB.ows_Format (def,primitive_format_id) VALUES "+
                          "(true,(SELECT id from CollectionDB.PrimitiveFormats WHERE mime_type='{0}' LIMIT 1));".format(
                              output.file_content_type
                              if output.file_content_type
                              else "text/plain"
                              ))
            else:
                pass
            local_format=os.environ['ZOO_OUTPUT_FORMAT'] if 'ZOO_OUTPUT_FORMAT' in os.environ else 'geojson-feature-collection'
            cur.execute(f"INSERT INTO CollectionDB.ows_DataDescription (format_id,data_format_id) VALUES ((SELECT last_value FROM CollectionDB.ows_Format_id_seq),(SELECT id from CollectionDB.PrimitiveDataFormats where short_name='{local_format}'));")
            cur.execute("INSERT INTO CollectionDB.ows_Output"+
                      "(identifier,title,abstract)"+
                      " VALUES "+
                      "($q${0}$q$,$q${1}$q$,$q${2}$q$);".format(output.identifier,output.title,output.description))
            cur.execute("INSERT INTO CollectionDB.OutputDataDescriptionAssignment (output_id,data_description_id) VALUES ((select last_value as id from CollectionDB.Descriptions_id_seq),(select last_value from CollectionDB.ows_DataDescription_id_seq));")
            cur.execute("INSERT INTO CollectionDB.ProcessOutputAssignment(process_id,output_id) VALUES((select id from pid),(select last_value as id from CollectionDB.Descriptions_id_seq));")
        cur.execute("DROP TABLE pid;")
        conn.commit()
        conn.close()
        return True

    def write_ogc_api_json(self, stream):
        ogc = self.get_ogc_api_json()
        print(json.dumps(ogc, indent=2), file=stream)

    def write_ogc_api_yaml(self, stream):
        ogc = self.get_ogc_api_json()
        print(yaml.dump(ogc), file=stream)

    def get_ogc_api_json(self):
        ogc = {
            "id": self.identifier,
            "version": self.version,
            "title": self.title,
            "description": self.description,
            "jobControlOptions": [],
            "outputTransmission": [],
            "links": [],
            "inputs": {},
            "outputs": {},
        }

        for input in self.inputs:
            ogc_input_schema = {"type": input.type}
            if input.min_occurs == 0:
                ogc_input_schema["nullable"] = True
            elif input.max_occurs != 1:
                ogc_input_schema["minItems"] = input.min_occurs
            if input.max_occurs != 1:
                ogc_input_schema["type"] = "array"
                ogc_input_schema["maxItems"] = (
                    input.max_occurs if input.max_occurs > 1 else 100
                )
                ogc_input_schema["items"] = {"type": input.type}
            if input.possible_values:
                ogc_input_schema["enum"] = input.possible_values.copy()
            if input.default_value:
                ogc_input_schema["default"] = input.default_value
            if input.is_file:
                ogc_input_schema["contentMediaType"] = input.file_content_type()
            elif input.is_directory:
                ogc_input_schema["contentMediaType"] = input.file_content_type()

            ogc_input = {
                "title": input.title,
                "description": input.description,
                "schema": ogc_input_schema,
            }

            if input.is_complex:
                pass  # TODO
            else:
                ogc["inputs"][input.identifier] = ogc_input

        for output in self.outputs:
            ogc_output_schema = {"type": output.type}

            ogc_output = {
                "title": output.title,
                "description": output.description,
                "schema": ogc_output_schema,
            }

            if output.is_complex:
                pass  # TODO
            else:
                ogc["outputs"][output.identifier] = ogc_output
        return ogc


class ProcessInput:

    cwl_type_map = {
        "boolean": "boolean",
        "int": "integer",
        "long": "integer",
        "float": "float",
        "double": "double",
        "string": "string",
        "enum": None,
    }

    def __init__(self, identifier, title=None, description=None, input_type="string"):
        self.identifier = str(identifier)
        self.title = title or identifier
        if self.title:
            self.title = str(self.title)
        self.description = description or title
        if self.description:
            self.description = str(self.description)
        self.type = input_type
        self.min_occurs = 1
        self.max_occurs = 1
        self.default_value = None
        self.possible_values = None
        self.is_complex = False
        self.is_bbox = False
        self.is_file = False
        self.file_content_type = None
        self.is_directory = False
        self.namespace = None
        self.is_array = False

    @classmethod
    def create_from_cwl(cls, input, trim_len):

        process_input = cls(
            input.id[trim_len+1:],
            input.label,
            input.doc,
        )

        process_input.set_type_from_cwl(input, trim_len)

        if input.default:
            process_input.default_value = input.default

        return process_input

    def handle_custom_types(self, type_name):
        schema_parts=type_name.split("#")
        type_name = None
        if len(schema_parts)>1 and schema_parts[0].count("ogc.yaml")>0 and schema_parts[1]=="BBox":
            self.is_bbox = True
            type_name = None
        if len(schema_parts)>1 and schema_parts[0].count("stac.yaml")>0:
            self.is_complex = True
            type_name = schema_parts[1]
            self.namespace = schema_parts[0]
            self.file_content_type = "application/json"
        if len(schema_parts)>1 and schema_parts[0].count("geojson.yaml")>0:
            self.is_complex = True
            self.namespace = schema_parts[0]
            type_name = schema_parts[1]
            self.file_content_type = "application/json"
        return type_name

    def set_type_from_cwl(self, input, trim_len):
        type_name = None
        # if input.type is something like ['null', 'typename'],
        # it means the input is optional and of type typename
        current_type_is_array=False
        if isinstance(input.type, str) or (isinstance(input.type, list) and len(input.type) == 2 and input.type[0] == 'null'):
            type_name = input.type[1] if isinstance(input.type, list) else input.type
            self.is_array=current_type_is_array
            if isinstance(type_name, cwl_v1_0.InputEnumSchema):
                self.possible_values = [str(s)[trim_len+len(self.identifier)+2:] for s in type_name.symbols]
                type_name = "string"
            if type_name in self.__class__.cwl_type_map:
                type_name = self.__class__.cwl_type_map[type_name]
            elif type_name == "File":
                if input.format is not None:
                    self.is_complex = True
                    self.is_file = True
                    type_name="File"
                    self.file_content_type = input.format
                else:
                    type_name = "string"
                    self.file_content_type = "text/plain"
            elif type_name == "Directory":
                type_name = "string"
                self.file_content_type = "text/plain"
            elif isinstance(type_name, cwl_v1_0.InputArraySchema):
                current_type_is_array=True
                type_name = type_name.items
                if type_name in self.__class__.cwl_type_map:
                    type_name = self.__class__.cwl_type_map[type_name]
                elif type_name == "File":
                    if input.format is not None:
                        self.is_complex = True
                        self.is_file = True
                        type_name="File"
                        self.file_content_type = input.format
                    else:
                        type_name = "string"
                        self.file_content_type = "text/plain"
                elif type_name == "Directory":
                    type_name = "string"
                    self.file_content_type = "text/plain"
                elif type_name.count("schemas")>0:
                    type_name = self.handle_custom_types(type_name)
                else:
                    type_name = None
            elif type_name.count("schemas")>0:
                type_name = self.handle_custom_types(type_name)

            else:
                raise Exception(
                    "Unsupported 0 type for input '{0}': {1}".format(input.id, type_name)
                )

            self.type = type_name
            self.min_occurs = 0 if (input.default is not None or (isinstance(input.type, list) and input.type[0]=='null')) else 1
            # How should we set the maximum length of an array for instance?
            # We currently set the default maximum to 1024
            self.max_occurs = 1 if not(current_type_is_array) else 1024
            # 0 means unbounded, TODO: what should be the maxOcccurs value if unbounded is not available?

        elif isinstance(input.type, cwl_v1_0.InputArraySchema):
            type_name = input.type.items

            if type_name in self.__class__.cwl_type_map:
                type_name = self.__class__.cwl_type_map[type_name]
            elif type_name == "File":
                type_name = "string"
                self.file_content_type = "text/plain"
            elif type_name == "Directory":
                type_name = "string"
                self.file_content_type = "text/plain"
            elif type_name.count("schemas")>0:
                type_name = self.handle_custom_types(type_name)
            else:
                type_name = None
            self.min_occurs = 1
            self.max_occurs = 0

            if not type_name:
                raise Exception("Unsupported 1 type: '{0}'".format(type_name))

            self.type = type_name

        elif isinstance(input.type, cwl_v1_0.InputEnumSchema):
            type_name = "string"
            self.possible_values = [str(s)[trim_len+len(self.identifier)+2:] for s in input.type.symbols]



class ProcessOutput:
    def __init__(self, identifier, title=None, description=None, input_type="string"):
        self.identifier = str(identifier)
        self.title = title or identifier
        if self.title:
            self.title = str(self.title)
        self.description = description or title
        if self.description:
            self.description = str(self.description)
        self.type = input_type
        self.min_occurs = 1
        self.max_occurs = 1
        self.default_value = None
        self.possible_values = None
        self.is_complex = False
        self.is_file = False
        self.file_content_type = None
        self.is_directory = False

    @classmethod
    def create_from_cwl(cls, output, trim_len):

        process_output = cls(
            output.id[trim_len+1:],
            output.label,
            output.doc,
        )

        process_output.set_type_from_cwl(output)

        return process_output

    def set_type_from_cwl(self, output):
        if isinstance(output.type, str):
            type_name = output.type
            if type_name == "string":
                pass
            elif type_name == "File":
                type_name = "string"
                self.file_content_type = "text/plain"
            elif type_name == "Directory":
                self.is_complex = True
                self.file_content_type = "application/json"
            else:
                raise Exception(
                    "Unsupported type for output '{0}': {1}".format(
                        output.id, type_name
                    )
                )
            self.type = type_name

        elif isinstance(output.type, cwl_v1_0.OutputArraySchema):
            type_name = output.type.items

            if type_name == "string":
                pass
            elif type_name == "File":
                type_name = "string"
                self.file_content_type = "text/plain"
            elif type_name == "Directory":
                self.is_complex = True
                self.file_content_type = "application/json"
            else:
                raise Exception(
                    "Unsupported type for output '{0}': {1}".format(
                        output.id, type_name
                    )
                )

            self.min_occurs = 1
            self.max_occurs = 0

            if not type_name:
                raise Exception("Unsupported type: '{0}'".format(type_name))

            self.type = type_name

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

