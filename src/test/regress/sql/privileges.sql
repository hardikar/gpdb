--
-- Test access privileges
--
set optimizer=off;
-- Clean up in case a prior regression run failed

-- Suppress NOTICE messages when users/groups don't exist
SET client_min_messages TO 'error';

DROP ROLE IF EXISTS regressgroup1;
DROP ROLE IF EXISTS regressgroup2;

DROP ROLE IF EXISTS regressuser1;
DROP ROLE IF EXISTS regressuser2;
DROP ROLE IF EXISTS regressuser3;
DROP ROLE IF EXISTS regressuser4;
DROP ROLE IF EXISTS regressuser5;

RESET client_min_messages;

-- test proper begins here

CREATE USER regressuser1;
CREATE USER regressuser2;
CREATE USER regressuser3;
CREATE USER regressuser4;
CREATE USER regressuser5;
CREATE USER regressuser5;	-- duplicate

CREATE GROUP regressgroup1;
CREATE GROUP regressgroup2 WITH USER regressuser1, regressuser2;

ALTER GROUP regressgroup1 ADD USER regressuser4;

ALTER GROUP regressgroup2 ADD USER regressuser2;	-- duplicate
ALTER GROUP regressgroup2 DROP USER regressuser2;
GRANT regressgroup2 TO regressuser4 WITH ADMIN OPTION;


-- test owner privileges

SET SESSION AUTHORIZATION regressuser1;
SELECT session_user, current_user;

CREATE TABLE atest1 ( a int, b text );
SELECT * FROM atest1;
INSERT INTO atest1 VALUES (1, 'one');
DELETE FROM atest1;
UPDATE atest1 SET b = 'blech' WHERE a = 213;
TRUNCATE atest1;
LOCK atest1 IN ACCESS EXCLUSIVE MODE;

REVOKE ALL ON atest1 FROM PUBLIC;
SELECT * FROM atest1;

GRANT ALL ON atest1 TO regressuser2;
GRANT SELECT ON atest1 TO regressuser3, regressuser4;
SELECT * FROM atest1;

CREATE TABLE atest2 (col1 varchar(10), col2 boolean);
GRANT SELECT ON atest2 TO regressuser2;
GRANT UPDATE ON atest2 TO regressuser3;
GRANT INSERT ON atest2 TO regressuser4;
GRANT TRUNCATE ON atest2 TO regressuser5;


SET SESSION AUTHORIZATION regressuser2;
SELECT session_user, current_user;

-- try various combinations of queries on atest1 and atest2

SELECT * FROM atest1; -- ok
SELECT * FROM atest2; -- ok
INSERT INTO atest1 VALUES (2, 'two'); -- ok
INSERT INTO atest2 VALUES ('foo', true); -- fail
INSERT INTO atest1 SELECT 1, b FROM atest1; -- ok
UPDATE atest1 SET b = 'twotwo' WHERE a = 2; -- ok
UPDATE atest2 SET col2 = NOT col2; -- fail
SELECT * FROM atest1 FOR UPDATE; -- ok
SELECT * FROM atest2 FOR UPDATE; -- fail
DELETE FROM atest2; -- fail
TRUNCATE atest2; -- fail
LOCK atest2 IN ACCESS EXCLUSIVE MODE; -- fail
COPY atest2 FROM stdin; -- fail
GRANT ALL ON atest1 TO PUBLIC; -- fail

-- checks in subquery, both ok
SELECT * FROM atest1 WHERE ( b IN ( SELECT col1 FROM atest2 ) );
SELECT * FROM atest2 WHERE ( col1 IN ( SELECT b FROM atest1 ) );


SET SESSION AUTHORIZATION regressuser3;
SELECT session_user, current_user;

SELECT * FROM atest1; -- ok
SELECT * FROM atest2; -- fail
INSERT INTO atest1 VALUES (2, 'two'); -- fail
INSERT INTO atest2 VALUES ('foo', true); -- fail
INSERT INTO atest1 SELECT 1, b FROM atest1; -- fail
UPDATE atest1 SET b = 'twotwo' WHERE a = 2; -- fail
UPDATE atest2 SET col2 = NULL; -- ok
UPDATE atest2 SET col2 = NOT col2; -- fails; requires SELECT on atest2
UPDATE atest2 SET col2 = true FROM atest1 WHERE atest1.a = 5; -- ok
SELECT * FROM atest1 FOR UPDATE; -- fail
SELECT * FROM atest2 FOR UPDATE; -- fail
DELETE FROM atest2; -- fail
TRUNCATE atest2; -- fail
LOCK atest2 IN ACCESS EXCLUSIVE MODE; -- ok
COPY atest2 FROM stdin; -- fail

-- checks in subquery, both fail
SELECT * FROM atest1 WHERE ( b IN ( SELECT col1 FROM atest2 ) );
SELECT * FROM atest2 WHERE ( col1 IN ( SELECT b FROM atest1 ) );

SET SESSION AUTHORIZATION regressuser4;
COPY atest2 FROM stdin; -- ok
bar	true
\.
SELECT * FROM atest1; -- ok


-- groups

SET SESSION AUTHORIZATION regressuser3;
CREATE TABLE atest3 (one int, two int, three int);
GRANT DELETE ON atest3 TO GROUP regressgroup2;

SET SESSION AUTHORIZATION regressuser1;

SELECT * FROM atest3; -- fail
DELETE FROM atest3; -- ok


-- views

SET SESSION AUTHORIZATION regressuser3;

CREATE VIEW atestv1 AS SELECT * FROM atest1; -- ok
/* The next *should* fail, but it's not implemented that way yet. */
CREATE VIEW atestv2 AS SELECT * FROM atest2;
CREATE VIEW atestv3 AS SELECT * FROM atest3; -- ok

SELECT * FROM atestv1; -- ok
SELECT * FROM atestv2; -- fail
GRANT SELECT ON atestv1, atestv3 TO regressuser4;
GRANT SELECT ON atestv2 TO regressuser2;

SET SESSION AUTHORIZATION regressuser4;

SELECT * FROM atestv1; -- ok
SELECT * FROM atestv2; -- fail
SELECT * FROM atestv3; -- ok

CREATE VIEW atestv4 AS SELECT * FROM atestv3; -- nested view
SELECT * FROM atestv4; -- ok
GRANT SELECT ON atestv4 TO regressuser2;

SET SESSION AUTHORIZATION regressuser2;

-- Two complex cases:

SELECT * FROM atestv3; -- fail
SELECT * FROM atestv4; -- ok (even though regressuser2 cannot access underlying atestv3)

SELECT * FROM atest2; -- ok
SELECT * FROM atestv2; -- fail (even though regressuser2 can access underlying atest2)


-- privileges on functions, languages

-- switch to superuser
\c -

REVOKE ALL PRIVILEGES ON LANGUAGE sql FROM PUBLIC;
GRANT USAGE ON LANGUAGE sql TO regressuser1; -- ok
GRANT USAGE ON LANGUAGE c TO PUBLIC; -- fail

SET SESSION AUTHORIZATION regressuser1;
GRANT USAGE ON LANGUAGE sql TO regressuser2; -- fail
CREATE FUNCTION testfunc1(int) RETURNS int AS 'select 2 * $1;' LANGUAGE sql CONTAINS SQL;
CREATE FUNCTION testfunc2(int) RETURNS int AS 'select 3 * $1;' LANGUAGE sql CONTAINS SQL;

REVOKE ALL ON FUNCTION testfunc1(int), testfunc2(int) FROM PUBLIC;
GRANT EXECUTE ON FUNCTION testfunc1(int), testfunc2(int) TO regressuser2;
GRANT USAGE ON FUNCTION testfunc1(int) TO regressuser3; -- semantic error
GRANT ALL PRIVILEGES ON FUNCTION testfunc1(int) TO regressuser4;
GRANT ALL PRIVILEGES ON FUNCTION testfunc_nosuch(int) TO regressuser4;

CREATE FUNCTION testfunc4(boolean) RETURNS text
  AS 'select col1 from atest2 where col2 = $1;'
  LANGUAGE sql SECURITY DEFINER READS SQL DATA;
GRANT EXECUTE ON FUNCTION testfunc4(boolean) TO regressuser3;

SET SESSION AUTHORIZATION regressuser2;
SELECT testfunc1(5), testfunc2(5); -- ok
CREATE FUNCTION testfunc3(int) RETURNS int AS 'select 2 * $1;' LANGUAGE sql CONTAINS SQL; -- fail

SET SESSION AUTHORIZATION regressuser3;
SELECT testfunc1(5); -- fail
SELECT col1 FROM atest2 WHERE col2 = true; -- fail
SELECT testfunc4(true); -- ok

SET SESSION AUTHORIZATION regressuser4;
SELECT testfunc1(5); -- ok

DROP FUNCTION testfunc1(int); -- fail

\c -

DROP FUNCTION testfunc1(int); -- ok
-- restore to sanity
GRANT ALL PRIVILEGES ON LANGUAGE sql TO PUBLIC;

-- truncate
SET SESSION AUTHORIZATION regressuser5;
TRUNCATE atest2; -- ok
TRUNCATE atest3; -- fail

-- has_table_privilege function

-- bad-input checks
select has_table_privilege(NULL,'pg_authid','select');
select has_table_privilege('pg_shad','select');
select has_table_privilege('nosuchuser','pg_authid','select');
select has_table_privilege('pg_authid','sel');
select has_table_privilege(-999999,'pg_authid','update');
select has_table_privilege(1,'select');

-- superuser
\c -

select has_table_privilege(current_user,'pg_authid','select');
select has_table_privilege(current_user,'pg_authid','insert');

select has_table_privilege(t2.oid,'pg_authid','update')
from (select oid from pg_roles where rolname = current_user) as t2;
select has_table_privilege(t2.oid,'pg_authid','delete')
from (select oid from pg_roles where rolname = current_user) as t2;

-- 'rule' privilege no longer exists, but for backwards compatibility
-- has_table_privilege still recognizes the keyword and says FALSE
select has_table_privilege(current_user,t1.oid,'rule')
from (select oid from pg_class where relname = 'pg_authid') as t1;
select has_table_privilege(current_user,t1.oid,'references')
from (select oid from pg_class where relname = 'pg_authid') as t1;

select has_table_privilege(t2.oid,t1.oid,'select')
from (select oid from pg_class where relname = 'pg_authid') as t1,
  (select oid from pg_roles where rolname = current_user) as t2;
select has_table_privilege(t2.oid,t1.oid,'insert')
from (select oid from pg_class where relname = 'pg_authid') as t1,
  (select oid from pg_roles where rolname = current_user) as t2;

select has_table_privilege('pg_authid','update');
select has_table_privilege('pg_authid','delete');
select has_table_privilege('pg_authid','truncate');

select has_table_privilege(t1.oid,'select')
from (select oid from pg_class where relname = 'pg_authid') as t1;
select has_table_privilege(t1.oid,'trigger')
from (select oid from pg_class where relname = 'pg_authid') as t1;

-- non-superuser
SET SESSION AUTHORIZATION regressuser3;

select has_table_privilege(current_user,'pg_class','select');
select has_table_privilege(current_user,'pg_class','insert');

select has_table_privilege(t2.oid,'pg_class','update')
from (select oid from pg_roles where rolname = current_user) as t2;
select has_table_privilege(t2.oid,'pg_class','delete')
from (select oid from pg_roles where rolname = current_user) as t2;

select has_table_privilege(current_user,t1.oid,'references')
from (select oid from pg_class where relname = 'pg_class') as t1;

select has_table_privilege(t2.oid,t1.oid,'select')
from (select oid from pg_class where relname = 'pg_class') as t1,
  (select oid from pg_roles where rolname = current_user) as t2;
select has_table_privilege(t2.oid,t1.oid,'insert')
from (select oid from pg_class where relname = 'pg_class') as t1,
  (select oid from pg_roles where rolname = current_user) as t2;

select has_table_privilege('pg_class','update');
select has_table_privilege('pg_class','delete');
select has_table_privilege('pg_class','truncate');

select has_table_privilege(t1.oid,'select')
from (select oid from pg_class where relname = 'pg_class') as t1;
select has_table_privilege(t1.oid,'trigger')
from (select oid from pg_class where relname = 'pg_class') as t1;

select has_table_privilege(current_user,'atest1','select');
select has_table_privilege(current_user,'atest1','insert');

select has_table_privilege(t2.oid,'atest1','update')
from (select oid from pg_roles where rolname = current_user) as t2;
select has_table_privilege(t2.oid,'atest1','delete')
from (select oid from pg_roles where rolname = current_user) as t2;

select has_table_privilege(current_user,t1.oid,'references')
from (select oid from pg_class where relname = 'atest1') as t1;

select has_table_privilege(t2.oid,t1.oid,'select')
from (select oid from pg_class where relname = 'atest1') as t1,
  (select oid from pg_roles where rolname = current_user) as t2;
select has_table_privilege(t2.oid,t1.oid,'insert')
from (select oid from pg_class where relname = 'atest1') as t1,
  (select oid from pg_roles where rolname = current_user) as t2;

select has_table_privilege('atest1','update');
select has_table_privilege('atest1','delete');
select has_table_privilege('atest1','truncate');

select has_table_privilege(t1.oid,'select')
from (select oid from pg_class where relname = 'atest1') as t1;
select has_table_privilege(t1.oid,'trigger')
from (select oid from pg_class where relname = 'atest1') as t1;


-- Grant options

SET SESSION AUTHORIZATION regressuser1;

CREATE TABLE atest4 (a int);

GRANT SELECT ON atest4 TO regressuser2 WITH GRANT OPTION;
GRANT UPDATE ON atest4 TO regressuser2;
GRANT SELECT ON atest4 TO GROUP regressgroup1 WITH GRANT OPTION;

SET SESSION AUTHORIZATION regressuser2;

GRANT SELECT ON atest4 TO regressuser3;
GRANT UPDATE ON atest4 TO regressuser3; -- fail

SET SESSION AUTHORIZATION regressuser1;

REVOKE SELECT ON atest4 FROM regressuser3; -- does nothing
SELECT has_table_privilege('regressuser3', 'atest4', 'SELECT'); -- true
REVOKE SELECT ON atest4 FROM regressuser2; -- fail
REVOKE GRANT OPTION FOR SELECT ON atest4 FROM regressuser2 CASCADE; -- ok
SELECT has_table_privilege('regressuser2', 'atest4', 'SELECT'); -- true
SELECT has_table_privilege('regressuser3', 'atest4', 'SELECT'); -- false

SELECT has_table_privilege('regressuser1', 'atest4', 'SELECT WITH GRANT OPTION'); -- true


-- Admin options

SET SESSION AUTHORIZATION regressuser4;
CREATE FUNCTION dogrant_ok() RETURNS void LANGUAGE sql SECURITY DEFINER AS
	'GRANT regressgroup2 TO regressuser5';
GRANT regressgroup2 TO regressuser5; -- ok: had ADMIN OPTION
SET ROLE regressgroup2;
GRANT regressgroup2 TO regressuser5; -- fails: SET ROLE suspended privilege

SET SESSION AUTHORIZATION regressuser1;
GRANT regressgroup2 TO regressuser5; -- fails: no ADMIN OPTION
SELECT dogrant_ok();			-- ok: SECURITY DEFINER conveys ADMIN
SET ROLE regressgroup2;
GRANT regressgroup2 TO regressuser5; -- fails: SET ROLE did not help

SET SESSION AUTHORIZATION regressgroup2;
GRANT regressgroup2 TO regressuser5; -- ok: a role can self-admin
CREATE FUNCTION dogrant_fails() RETURNS void LANGUAGE sql SECURITY DEFINER AS
	'GRANT regressgroup2 TO regressuser5';
SELECT dogrant_fails();			-- fails: no self-admin in SECURITY DEFINER
DROP FUNCTION dogrant_fails();

SET SESSION AUTHORIZATION regressuser4;
DROP FUNCTION dogrant_ok();
REVOKE regressgroup2 FROM regressuser5;


-- test that dependent privileges are revoked (or not) properly
\c -

set session role regressuser1;
create table dep_priv_test (a int);
grant select on dep_priv_test to regressuser2 with grant option;
grant select on dep_priv_test to regressuser3 with grant option;
set session role regressuser2;
grant select on dep_priv_test to regressuser4 with grant option;
set session role regressuser3;
grant select on dep_priv_test to regressuser4 with grant option;
set session role regressuser4;
grant select on dep_priv_test to regressuser5;
\dp dep_priv_test
set session role regressuser2;
revoke select on dep_priv_test from regressuser4 cascade;
\dp dep_priv_test
set session role regressuser3;
revoke select on dep_priv_test from regressuser4 cascade;
\dp dep_priv_test
set session role regressuser1;
drop table dep_priv_test;


-- clean up

\c regression

DROP FUNCTION testfunc2(int);
DROP FUNCTION testfunc4(boolean);

DROP VIEW atestv1;
DROP VIEW atestv2;
-- this should cascade to drop atestv4
DROP VIEW atestv3 CASCADE;
-- this should complain "does not exist"
DROP VIEW atestv4;

DROP TABLE atest1;
DROP TABLE atest2;
DROP TABLE atest3;
DROP TABLE atest4;

DROP GROUP regressgroup1;
DROP GROUP regressgroup2;

REVOKE USAGE ON LANGUAGE sql FROM regressuser1;

-- regression test: superuser create a schema and authorize it to a non-superuser
CREATE ROLE "non_superuser_schema";
CREATE SCHEMA test_non_superuser_schema AUTHORIZATION "non_superuser_schema";

DROP USER regressuser1;
DROP USER regressuser2;
DROP USER regressuser3;
DROP USER regressuser4;
DROP USER regressuser5;
reset optimizer;
