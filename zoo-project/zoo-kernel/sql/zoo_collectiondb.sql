--------------------------------------------------------------------------------
--
-- PostgreSQL definition of tables required byt the ZOO-Kernel version >= 1.8.0
-- if the the metadb is activated
--
-- Copyright (C) 2018-2022 GeoLabs SARL. All rights reserved.
-- Author: Gérald Fenoy <gerald.fenoy@geolabs.fr>
--
-- Permission is hereby granted, free of charge, to any person obtaining a copy
-- of this software and associated documentation files (the "Software"), to deal
-- in the Software without restriction, including without limitation the rights
-- to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
-- copies of the Software, and to permit persons to whom the Software is
-- furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
-- OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
-- THE SOFTWARE.
--
--------------------------------------------------------------------------------
-- If your database is not using UTF-8 per default then uncomment the following
-- SET client_encoding = 'UTF8';
--------------------------------------------------------------------------------

create schema CollectionDB;

set search_path = CollectionDB, pg_catalog;

CREATE OR REPLACE FUNCTION update_Description() RETURNS trigger AS
$$
DECLARE
	i integer;
BEGIN
	INSERT INTO CollectionDB.Descriptions (id) VALUES (NEW.id);
	RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE FUNCTION is_in_ows_DataDescription(i integer) RETURNS boolean
    LANGUAGE plpgsql
    AS $$
DECLARE
        res int;
        mymax int;
BEGIN
	SELECT id from CollectionDB.ows_DataDescription where id=i INTO res ;
	if res is NULL then
	   return false;
	else
	   return true;
	end if;
END;
$$;

create table CollectionDB.Descriptions (
       id serial primary key
);

create table CollectionDB.ows_Metadata (
       id serial primary key,
       title text,
       role text,
       href text
);

create table CollectionDB.DescriptionsMetadataAssignment(
       descriptions_id int references CollectionDB.Descriptions(id),
       metadata_id int references CollectionDB.ows_Metadata(id)
);

create table CollectionDB.ows_Keywords (
    id serial primary key,
    keyword varchar
);

create table CollectionDB.DescriptionsKeywordsAssignment(
       descriptions_id int references CollectionDB.Descriptions(id),
       keywords_id int references CollectionDB.ows_Keywords(id)
);

create table CollectionDB.ows_AdditionalParameters (
    id serial primary key,
    title varchar,
    role varchar,
    href varchar
);

create table CollectionDB.DescriptionsAdditionalParametersAssignment (
       descriptions_id int references CollectionDB.Descriptions(id),
       additional_parameters_id int references CollectionDB.ows_AdditionalParameters(id)
);

--
-- See reference for primitive datatypes
-- https://www.w3.org/TR/xmlschema-2/#built-in-primitive-datatypes
--
create table CollectionDB.PrimitiveDataTypes (
       id serial primary key,
       name varchar(255)
);
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('string');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('boolean');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('integer');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('float');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('double');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('duration');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('dateTime');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('time');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('date');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('gYearMonth');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('gYear');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('gMonthDay');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('gDay');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('gMonth');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('hexBinary');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('base64Binary');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('anyURI');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('QName');
INSERT INTO CollectionDB.PrimitiveDataTypes (name) VALUES ('NOTATION');

--
-- List all primitive formats
--
create table CollectionDB.PrimitiveFormats (
       id serial primary key,
       mime_type varchar(255),
       encoding varchar(15),
       schema varchar(255)
);

-- https://tools.ietf.org/html/rfc4180
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('text/csv','utf-8');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('text/css','utf-8');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('text/html','utf-8');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('text/javascript','utf-8');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('text/plain','utf-8');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding,schema)
       VALUES ('text/xml','utf-8','http://schema.opengis.net/gml/3.2.1/gml.xsd');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding,schema)
       VALUES ('text/xml','utf-8','http://schema.opengis.net/gml/3.1.0/gml.xsd');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('application/gml+xml','utf-8');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('application/json','utf-8');
-- https://tools.ietf.org/html/rfc3302
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('image/tiff');
-- https://www.ietf.org/rfc/rfc4047.txt
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('image/fits');
-- https://tools.ietf.org/html/rfc3745
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('image/jp2');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('image/png');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('image/jpeg');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('image/gif');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('application/octet-stream');
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('application/vnd.google-earth.kml+xml');
-- https://www.iana.org/assignments/media-types/application/zip
INSERT INTO CollectionDB.PrimitiveFormats (mime_type) VALUES ('application/zip');
-- https://www.iana.org/assignments/media-types/application/xml
INSERT INTO CollectionDB.PrimitiveFormats (mime_type,encoding) VALUES ('application/xml','utf-8');

create table CollectionDB.ows_Format (
    id serial primary key,
    primitive_format_id int references CollectionDB.PrimitiveFormats(id),
    maximum_megabytes int,
    def boolean,
	use_mapserver bool,
	ms_styles text
);

create table CollectionDB.ows_DataDescription (
    id serial primary key,
    format_id int references CollectionDB.ows_Format(id)
);

create table CollectionDB.PrimitiveUom (
	id serial primary key,
	uom varchar
);
-- source : Open Geospatial Consortium - URNs of definitions in ogc namespace
insert into CollectionDB.PrimitiveUom (uom) values ('degree');
insert into CollectionDB.PrimitiveUom (uom) values ('radian');
insert into CollectionDB.PrimitiveUom (uom) values ('metre');
insert into CollectionDB.PrimitiveUom (uom) values ('unity');

create table CollectionDB.LiteralDataDomain (
    possible_literal_values varchar,
    default_value varchar,
    data_type_id int references CollectionDB.PrimitiveDataTypes(id),
    uom int references CollectionDB.PrimitiveUom(id),
    def boolean
) inherits (CollectionDB.ows_DataDescription);
alter table CollectionDB.LiteralDataDomain add constraint literal_data_domain_id unique (id);

create table CollectionDB.BoundingBoxData (
    epsg int
) inherits (CollectionDB.ows_DataDescription);
alter table CollectionDB.BoundingBoxData add constraint bounding_box_data_id unique (id);

create table CollectionDB.ComplexData (
) inherits (CollectionDB.ows_DataDescription);
alter table CollectionDB.ComplexData add constraint complex_data_id unique (id);

create table CollectionDB.AllowedValues (
    id serial primary key,
    allowed_value varchar(255)
);

create table CollectionDB.AllowedValuesAssignment (
    id serial primary key,
    literal_data_domain_id int references CollectionDB.LiteralDataDomain (id),
    allowed_value_id int references CollectionDB.AllowedValues (id)
);

create table CollectionDB.ows_AdditionalParameter (
    id serial primary key,
    key varchar,
    value varchar,
    additional_parameters_id int references CollectionDB.ows_AdditionalParameters(id)
);

create table CollectionDB.ows_Input (
    id int primary key default nextval('collectiondb.descriptions_id_seq'::regclass),
    title text,
    abstract text,
    identifier varchar(255),
    min_occurs int,
    max_occurs int
); -- inherits (CollectionDB.Descriptions);
alter table CollectionDB.ows_Input add constraint codb_input_id unique (id);
CREATE TRIGGER ows_Input_proc AFTER INSERT ON CollectionDB.ows_Input FOR EACH ROW EXECUTE PROCEDURE update_Description();

create table CollectionDB.ows_Output (
    id int primary key default nextval('collectiondb.descriptions_id_seq'::regclass),
    title text,
    abstract text,
    identifier varchar(255)
); --inherits (CollectionDB.Descriptions);
alter table CollectionDB.ows_Output add constraint codb_output_id unique (id);
CREATE TRIGGER ows_Output_proc AFTER INSERT ON CollectionDB.ows_Output FOR EACH ROW EXECUTE PROCEDURE update_Description();

create table CollectionDB.zoo_PrivateMetadata (
    id serial primary key,
    identifier varchar,
    metadata_date timestamp
);

create table CollectionDB.ows_Process (
    id int primary key default nextval('collectiondb.descriptions_id_seq'::regclass),
    title text,
    abstract text,
    identifier varchar(255),
    availability boolean,
    process_description_xml text,
    private_metadata_id int references CollectionDB.zoo_PrivateMetadata(id)
); -- inherits (CollectionDB.Descriptions);
alter table CollectionDB.ows_Process add constraint codb_process_id unique (id);
alter table CollectionDB.ows_Process add constraint codb_process_identifier unique (identifier);
CREATE TRIGGER ows_Process_proc AFTER INSERT ON CollectionDB.ows_Process FOR EACH ROW EXECUTE PROCEDURE update_Description();

create table CollectionDB.InputInputAssignment (
    id serial primary key,
    parent_input int references CollectionDB.ows_Input(id),
    child_input int references CollectionDB.ows_Input(id)
);

create table CollectionDB.InputDataDescriptionAssignment (
    id serial primary key,
    input_id int references CollectionDB.ows_Input(id),
    data_description_id int check (CollectionDB.is_in_ows_DataDescription(data_description_id))
);

create table CollectionDB.OutputOutputAssignment (
    id serial primary key,
    parent_output int references CollectionDB.ows_Output(id),
    child_output int references CollectionDB.ows_Output(id)
);

create table CollectionDB.OutputDataDescriptionAssignment (
    id serial primary key,
    output_id int references CollectionDB.ows_Output(id),
    data_description_id int check (CollectionDB.is_in_ows_DataDescription(data_description_id))
);

create table CollectionDB.zoo_ServiceTypes (
	id serial primary key,
	service_type varchar
);
insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('HPC');
insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('C');
insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('Java');
insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('Mono');
insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('JS');
insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('PHP');
insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('Python');

insert into CollectionDB.zoo_ServiceTypes (service_type) VALUES ('OTB');

create table CollectionDB.zoo_DeploymentMetadata (
    id serial primary key,
    executable_name varchar,
    configuration_identifier varchar,
        service_type_id int references CollectionDB.zoo_ServiceTypes(id));

create table CollectionDB.zoo_PrivateProcessInfo (
    id serial primary key
);

create table CollectionDB.PrivateMetadataDeploymentMetadataAssignment (
    id serial primary key,
    private_metadata_id int references CollectionDB.zoo_PrivateMetadata(id),
    deployment_metadata_id int references CollectionDB.zoo_DeploymentMetadata(id)
);

create table CollectionDB.PrivateMetadataPrivateProcessInfoAssignment (
    id serial primary key,
    private_metadata_id int references CollectionDB.zoo_PrivateMetadata(id),
    private_process_info_id int references CollectionDB.zoo_PrivateProcessInfo(id)
);

create table CollectionDB.ProcessInputAssignment (
    id serial primary key,
    process_id int references CollectionDB.ows_Process(id),
    input_id int references CollectionDB.ows_Input(id),
    index int
);

create table CollectionDB.ProcessOutputAssignment (
    id serial primary key,
    process_id int references CollectionDB.ows_Process(id),
    output_id int references CollectionDB.ows_Output(id),
    index int
);

CREATE OR REPLACE VIEW public.ows_process AS
       (SELECT
	id,
	identifier,
	title,
	abstract,
	(SELECT service_type FROM CollectionDB.zoo_ServiceTypes WHERE id = (SELECT service_type_id FROM CollectionDB.zoo_DeploymentMetadata WHERE id = (SELECT deployment_metadata_id FROM CollectionDB.PrivateMetadataDeploymentmetadataAssignment WHERE private_metadata_id=(SELECT id FROM CollectionDB.zoo_PrivateMetadata WHERE id = CollectionDB.ows_Process.private_metadata_id)))) as service_type,
	(SELECT executable_name FROM CollectionDB.zoo_DeploymentMetadata WHERE id = (SELECT deployment_metadata_id FROM CollectionDB.PrivateMetadataDeploymentmetadataAssignment WHERE private_metadata_id=(SELECT id FROM CollectionDB.zoo_PrivateMetadata WHERE id = CollectionDB.ows_Process.private_metadata_id))) as service_provider,
	(SELECT configuration_identifier FROM CollectionDB.zoo_DeploymentMetadata WHERE id = (SELECT deployment_metadata_id FROM CollectionDB.PrivateMetadataDeploymentmetadataAssignment WHERE private_metadata_id=(SELECT id FROM CollectionDB.zoo_PrivateMetadata WHERE id = CollectionDB.ows_Process.private_metadata_id))) as conf_id,
	availability
	FROM CollectionDB.ows_Process
	WHERE
	 availability
	);
