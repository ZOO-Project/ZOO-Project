--------------------------------------------------------------------------------
--
-- PostgreSQL definition of tables required byt the ZOO-Kernel version >= 1.8.0
-- if the the rabbitmq option is activated
--
-- Copyright (C) 2020 GeoLabs SARL. All rights reserved.
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
-- Create a dedicated schema to store all tables
-- Uncomment the following 2 lines to activate the schema use
-- CREATE SCHEMA processdb;
-- SET search_path TO processdb;
--------------------------------------------------------------------------------
CREATE TABLE servers (
       id serial primary key,
       host varchar(17) UNIQUE,
       nb int
);

CREATE TABLE workers (
       id serial primary key,
       uuid text,
       host varchar(17),
       pid int,
       status int,
       creation_time timestamp with time zone default now(),
       UNIQUE(uuid)
);


CREATE OR REPLACE FUNCTION registerServer(schema text,host text, nb int) RETURNS boolean AS 
$BODY$
DECLARE
	res int;
        cnt int;
BEGIN
        EXECUTE 'SELECT count(*) from '||schema||'.servers where host = '''||host||'''' INTO cnt;
	IF cnt = 0  THEN
	   EXECUTE 'INSERT INTO '||schema||'.servers (host,nb) VALUES ('''||host||''','||nb||')';
	   RETURN true;
	ELSE
	   RETURN false;
	END IF;
END;
$BODY$
LANGUAGE 'plpgsql' COST 100.0 SECURITY INVOKER;



CREATE OR REPLACE FUNCTION checkAvailableExecutionSlot(schema text,uuid text, host text, pid int) RETURNS boolean AS 
$BODY$
DECLARE
	res int;
        cnt int;
BEGIN
        EXECUTE 'SELECT count(*) from '||schema||'.workers where uuid = '''||uuid||'''' INTO cnt;
	IF cnt = 0  THEN
	   EXECUTE 'INSERT INTO '||schema||'.workers (uuid,host,pid,status) VALUES ('''||uuid||''','''||host||''','||pid||',1)';
	   RETURN true;
	ELSE
	   RETURN false;
	END IF;
END;
$BODY$
LANGUAGE 'plpgsql' COST 100.0 SECURITY INVOKER;

