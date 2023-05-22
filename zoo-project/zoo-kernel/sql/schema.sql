--------------------------------------------------------------------------------
--
-- PostgreSQL definition of tables required byt the ZOO-Kernel version >= 1.5.0
-- if the the db-backend option is activated
--
-- Copyright (C) 2015 GeoLabs SARL. All rights reserved.
-- Author: David Saggiorato <david.saggiorato@geolabs.fr>
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
-- Users table 
-- Used to store the user name provided in case of authentication
create table users (
       id serial PRIMARY KEY,
       name varchar(255),
       provider varchar(255),
       creation_time timestamp with time zone default now(),
       access_time timestamp with time zone default now()
);
INSERT INTO users (id,name,provider) VALUES (0,'anonymous','unknown');
--------------------------------------------------------------------------------
-- Services table
-- Used to store information about services running asynchronously
create table services (
       osid TEXT,
       sid TEXT,
       uuid TEXT unique,
       processid TEXT,
       fstate varchar(25),
       status TEXT,
       response TEXT,
       creation_time timestamp with time zone default now(),
       start_time timestamp with time zone default now(),
       updated_time timestamp with time zone default now(),
       finished_time timestamp with time zone default NULL,
       end_time timestamp with time zone default NULL,
       progress int,
       itype varchar(10),
       message TEXT,
       user_id int REFERENCES users(id) ON DELETE CASCADE
);
--------------------------------------------------------------------------------
-- Responses table 
-- Used to store the response provided by a services running asynchronously
create table responses (
       uuid text references services(uuid) ON DELETE CASCADE,
       content text,
       creation_time timestamp with time zone default now()
);
--------------------------------------------------------------------------------
-- Files table
-- Used to store the files generated during the service execution
create table files (
       uuid TEXT references services(uuid) ON DELETE CASCADE,
       filename text,
       nature varchar(10),
       name varchar(255),
       creation_time timestamp with time zone default now(),
       expiration_time timestamp with time zone default now() + interval '48 hours'
);
--------------------------------------------------------------------------------
--------------------------------------------------------------------------------
-- Function to display date respecting the RFC 3339
CREATE OR REPLACE FUNCTION display_date_rfc3339(param_in timestamp with time zone)
RETURNS text AS
$$
DECLARE var_out varchar;
BEGIN
	PERFORM set_config('timezone', 'UTC', true);
	var_out := to_char(param_in , 'YYYY-MM-DD"T"HH24:MI:SS.MS"Z"');
	RETURN var_out;
END;
$$ language plpgsql VOLATILE;
--------------------------------------------------------------------------------
