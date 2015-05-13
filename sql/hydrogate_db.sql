--
-- PostgreSQL database dump
--

SET statement_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SET check_function_bodies = false;
SET client_min_messages = warning;

--
-- Name: plpgsql; Type: EXTENSION; Schema: -; Owner: 
--

CREATE EXTENSION IF NOT EXISTS plpgsql WITH SCHEMA pg_catalog;


--
-- Name: EXTENSION plpgsql; Type: COMMENT; Schema: -; Owner: 
--

COMMENT ON EXTENSION plpgsql IS 'PL/pgSQL procedural language';


SET search_path = public, pg_catalog;

SET default_tablespace = '';

SET default_with_oids = false;

--
-- Name: authors; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE authors (
    author_id integer NOT NULL,
    name character varying(45) NOT NULL,
    surname character varying(45) NOT NULL,
    email character varying(256) NOT NULL,
    phone character varying(45) NOT NULL,
    affiliation character varying(256) NOT NULL,
    created timestamp without time zone NOT NULL
);


ALTER TABLE public.authors OWNER TO postgres;

--
-- Name: authors_author_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE authors_author_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.authors_author_id_seq OWNER TO postgres;

--
-- Name: authors_author_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE authors_author_id_seq OWNED BY authors.author_id;


--
-- Name: groups; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE groups (
    group_id integer NOT NULL,
    name character varying(45) NOT NULL,
    description character varying(512) NOT NULL,
    enabled boolean DEFAULT true NOT NULL,
    created timestamp without time zone NOT NULL
);


ALTER TABLE public.groups OWNER TO postgres;

--
-- Name: groups_group_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE groups_group_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.groups_group_id_seq OWNER TO postgres;

--
-- Name: groups_group_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE groups_group_id_seq OWNED BY groups.group_id;


--
-- Name: hpc_centers; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE hpc_centers (
    hpc_id integer NOT NULL,
    name character varying(45) NOT NULL,
    address character varying(255) NOT NULL,
    port integer NOT NULL,
    account_name character varying(512) NOT NULL,
    account_password character varying(512) NOT NULL,
    tsalt1 integer NOT NULL,
    tsalt2 integer NOT NULL,
    enabled boolean DEFAULT true NOT NULL,
    created timestamp without time zone NOT NULL,
    qtype integer DEFAULT 1 NOT NULL
);


ALTER TABLE public.hpc_centers OWNER TO postgres;

--
-- Name: hpc_centers_hpc_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE hpc_centers_hpc_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.hpc_centers_hpc_id_seq OWNER TO postgres;

--
-- Name: hpc_centers_hpc_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE hpc_centers_hpc_id_seq OWNED BY hpc_centers.hpc_id;


--
-- Name: hpc_membership; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE hpc_membership (
    hpc_id integer NOT NULL,
    group_id integer NOT NULL,
    enabled boolean DEFAULT true NOT NULL
);


ALTER TABLE public.hpc_membership OWNER TO postgres;

--
-- Name: hpc_programs; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE hpc_programs (
    hpc_id integer NOT NULL,
    program_id integer NOT NULL,
    enabled boolean DEFAULT true NOT NULL,
    install_path character varying(256) NOT NULL
);


ALTER TABLE public.hpc_programs OWNER TO postgres;

--
-- Name: hpc_templates; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE hpc_templates (
    hpc_id integer NOT NULL,
    pbs_script_header character varying,
    pbs_script_task character varying,
    pbs_script_tail character varying
);


ALTER TABLE public.hpc_templates OWNER TO postgres;

--
-- Name: jobs; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE jobs (
    job_id integer NOT NULL,
    user_id integer NOT NULL,
    hpc_id integer NOT NULL,
    package_id integer NOT NULL,
    state integer NOT NULL,
    hpc_job_id character varying(512) NOT NULL,
    hpc_job_desc character varying(512) NOT NULL,
    stdout character varying(1024) NOT NULL,
    stderr character varying(1024) NOT NULL,
    created timestamp without time zone NOT NULL,
    updated timestamp without time zone NOT NULL,
    description character varying(512) NOT NULL,
    stdin character varying(1024) NOT NULL,
    errcode integer NOT NULL,
    callbackurl character varying(1024) NOT NULL,
    definition character varying(2048) NOT NULL
);


ALTER TABLE public.jobs OWNER TO postgres;

--
-- Name: jobs_job_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE jobs_job_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.jobs_job_id_seq OWNER TO postgres;

--
-- Name: jobs_job_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE jobs_job_id_seq OWNED BY jobs.job_id;


--
-- Name: packages; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE packages (
    package_id integer NOT NULL,
    hpc_id integer NOT NULL,
    user_id integer NOT NULL,
    state integer NOT NULL,
    folder character varying NOT NULL,
    stdout character varying NOT NULL,
    stderr character varying NOT NULL,
    created timestamp without time zone NOT NULL,
    updated timestamp without time zone NOT NULL,
    stdin character varying NOT NULL,
    callbackurl character varying(1024) NOT NULL
);


ALTER TABLE public.packages OWNER TO postgres;

--
-- Name: packages_package_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE packages_package_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.packages_package_id_seq OWNER TO postgres;

--
-- Name: packages_package_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE packages_package_id_seq OWNED BY packages.package_id;


--
-- Name: process_parameters; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE process_parameters (
    process_parameter_id integer NOT NULL,
    program_id integer NOT NULL,
    datatype integer NOT NULL,
    optional boolean NOT NULL,
    default_value character varying(255) NOT NULL,
    description character varying(255) NOT NULL,
    max double precision NOT NULL,
    min double precision NOT NULL,
    option character varying(25) NOT NULL,
    systemcontrolled boolean NOT NULL,
    name character varying(255) NOT NULL
);


ALTER TABLE public.process_parameters OWNER TO postgres;

--
-- Name: process_parameters_process_parameter_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE process_parameters_process_parameter_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.process_parameters_process_parameter_id_seq OWNER TO postgres;

--
-- Name: process_parameters_process_parameter_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE process_parameters_process_parameter_id_seq OWNED BY process_parameters.process_parameter_id;


--
-- Name: program_authors; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE program_authors (
    author_id integer NOT NULL,
    program_id integer NOT NULL
);


ALTER TABLE public.program_authors OWNER TO postgres;

--
-- Name: programs; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE programs (
    program_id integer NOT NULL,
    name character varying(50) NOT NULL,
    type integer NOT NULL,
    description character varying(512) NOT NULL,
    version character varying(10) NOT NULL,
    enabled boolean DEFAULT true NOT NULL,
    created timestamp without time zone NOT NULL
);


ALTER TABLE public.programs OWNER TO postgres;

--
-- Name: programs_program_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE programs_program_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.programs_program_id_seq OWNER TO postgres;

--
-- Name: programs_program_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE programs_program_id_seq OWNED BY programs.program_id;


--
-- Name: user_groups; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE user_groups (
    group_id integer NOT NULL,
    user_id integer NOT NULL
);


ALTER TABLE public.user_groups OWNER TO postgres;

--
-- Name: users; Type: TABLE; Schema: public; Owner: postgres; Tablespace: 
--

CREATE TABLE users (
    user_id integer NOT NULL,
    name character varying(25) NOT NULL,
    password character varying(512) NOT NULL,
    salt character varying(20) NOT NULL,
    enabled boolean DEFAULT true NOT NULL,
    created timestamp without time zone NOT NULL,
    description character varying(512) NOT NULL
);


ALTER TABLE public.users OWNER TO postgres;

--
-- Name: users_user_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE users_user_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER TABLE public.users_user_id_seq OWNER TO postgres;

--
-- Name: users_user_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE users_user_id_seq OWNED BY users.user_id;


--
-- Name: author_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY authors ALTER COLUMN author_id SET DEFAULT nextval('authors_author_id_seq'::regclass);


--
-- Name: group_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY groups ALTER COLUMN group_id SET DEFAULT nextval('groups_group_id_seq'::regclass);


--
-- Name: hpc_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY hpc_centers ALTER COLUMN hpc_id SET DEFAULT nextval('hpc_centers_hpc_id_seq'::regclass);


--
-- Name: job_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY jobs ALTER COLUMN job_id SET DEFAULT nextval('jobs_job_id_seq'::regclass);


--
-- Name: package_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY packages ALTER COLUMN package_id SET DEFAULT nextval('packages_package_id_seq'::regclass);


--
-- Name: process_parameter_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY process_parameters ALTER COLUMN process_parameter_id SET DEFAULT nextval('process_parameters_process_parameter_id_seq'::regclass);


--
-- Name: program_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY programs ALTER COLUMN program_id SET DEFAULT nextval('programs_program_id_seq'::regclass);


--
-- Name: user_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY users ALTER COLUMN user_id SET DEFAULT nextval('users_user_id_seq'::regclass);


--
-- Data for Name: authors; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY authors (author_id, name, surname, email, phone, affiliation, created) FROM stdin;
1	David	Tarboton	dtarb@usu.edu	NA	Utah State University	2014-03-24 02:00:00
\.


--
-- Name: authors_author_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('authors_author_id_seq', 1, true);


--
-- Data for Name: groups; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY groups (group_id, name, description, enabled, created) FROM stdin;
\.


--
-- Name: groups_group_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('groups_group_id_seq', 1, false);


--
-- Data for Name: hpc_centers; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY hpc_centers (hpc_id, name, address, port, account_name, account_password, tsalt1, tsalt2, enabled, created, qtype) FROM stdin;
1	Dummy	Dummy	0	 	 	0	0	f	2014-02-04 16:24:17	1
\.


--
-- Name: hpc_centers_hpc_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('hpc_centers_hpc_id_seq', 6, true);


--
-- Data for Name: hpc_membership; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY hpc_membership (hpc_id, group_id, enabled) FROM stdin;
\.


--
-- Data for Name: hpc_programs; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY hpc_programs (hpc_id, program_id, enabled, install_path) FROM stdin;
2	1	t	/home/hpcuser/.hydrogate/bin/taudem/
2	2	t	/home/hpcuser/.hydrogate/bin/ueb/
2	4	t	/home/hpcuser/.hydrogate/bin/taudem/
2	5	t	/home/hpcuser/.hydrogate/bin/taudem/
2	6	t	/home/hpcuser/.hydrogate/bin/scripts/
5	1	t	/home/hpcuser/.hydrogate/bin/taudem/
6	1	t	/home/hpcuser/.hydrogate/bin/taudem/
\.


--
-- Data for Name: hpc_templates; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY hpc_templates (hpc_id, pbs_script_header, pbs_script_task, pbs_script_tail) FROM stdin;
2	#!/bin/sh\n\n#PBS -N $HGW_JOB_NAME\n#PBS -A CI-WATER\n#PBS -e $HGW_ERROR_LOG_FILE\n#PBS -o $HGW_OUTPUT_LOG_FILE\n#PBS -l nodes=$HGW_NODES:ppn=$HGW_PPN\n#PBS -l walltime=$HGW_WALLTIME\n\ncd $PBS_O_WORKDIR\n\n#HGW_MODULE_LIST	$HGW_RUNNER $HGW_RUNNER_PARAM $HGW_PROGRAM $HGW_PROGRAM_PARAM\n\nrc=$?\n\n[ $rc -ne 0 ] && echo $rc > $HGW_ERRORCODE_FILE && zip -q stderr.log stdout.log && exit $rc	echo 0 > $HGW_ERRORCODE_FILE\nzip -q $HGW_RESULT_ZIP_FILE_NAME $HGW_RESULT_FILE_LIST
5	#!/bin/sh\n\n#PBS -N $HGW_JOB_NAME\n#PBS -e $HGW_ERROR_LOG_FILE\n#PBS -o $HGW_OUTPUT_LOG_FILE\n#PBS -l nodes=$HGW_NODES:ppn=$HGW_PPN\n#PBS -l walltime=$HGW_WALLTIME\n\ncd $PBS_O_WORKDIR\n\n#HGW_MODULE_LIST	$HGW_RUNNER $HGW_RUNNER_PARAM $HGW_PROGRAM $HGW_PROGRAM_PARAM\n\nrc=$?\n\n[ $rc -ne 0 ] && echo $rc > $HGW_ERRORCODE_FILE && zip -q stderr.log stdout.log && exit $rc	echo 0 > $HGW_ERRORCODE_FILE\nzip -q $HGW_RESULT_ZIP_FILE_NAME $HGW_RESULT_FILE_LIST
6	#!/bin/bash \n#SBATCH --job-name=$HGW_JOB_NAME \n#SBATCH --output=$HGW_OUTPUT_LOG_FILE \n#SBATCH --error=$HGW_ERROR_LOG_FILE \n#SBATCH -N $HGW_NODES \n#SBATCH --ntasks-per-node $HGW_PPN \n#SBATCH --time=$HGW_WALLTIME \n\n# Initiate dotkits \n. /rc/tools/utils/dkinit \nuse -q OpenMPI\n	$HGW_RUNNER $HGW_RUNNER_PARAM $HGW_PROGRAM $HGW_PROGRAM_PARAM \nrc=$? \n[ $rc -ne 0 ] && echo $rc > $HGW_ERRORCODE_FILE && zip -q stderr.log stdout.log && exit $rc\n	echo 0 > $HGW_ERRORCODE_FILE \nzip -q $HGW_RESULT_ZIP_FILE_NAME $HGW_RESULT_FILE_LIST \n
\.

--
-- Name: jobs_job_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('jobs_job_id_seq', 76, true);

--
-- Name: packages_package_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('packages_package_id_seq', 107, true);


--
-- Data for Name: process_parameters; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY process_parameters (process_parameter_id, program_id, datatype, optional, default_value, description, max, min, option, systemcontrolled, name) FROM stdin;
8	2	1	f	 	control file	-1	-1	 	f	control
6	2	1	f	./	working directory of input files	-1	-1	-wdir	t	wdir
10	4	1	f	.	D8 flow direction output file	-1	-1	-p	t	p
11	4	1	f	.	D8 slope file	-1	-1	-sd8	t	sd8
14	4	1	f	.	carved or pit filled input elevation file	-1	-1	-fel	t	fel
1	1	1	f	.	path of the input dem directory	-1	-1	-z	t	z
2	1	1	f	.	output directory path containing output dems with pits filled	-1	-1	-fel	t	fel
18	5	1	f	.	D-infinity slope file	-1	-1	-slp	t	slp
17	5	1	f	.	D-infinity flow direction output file	-1	-1	-ang	t	ang
19	5	1	f	.	carved or pit filled input elevation file	-1	-1	-fel	t	fel
15	4	2	t	1 1	it specifies the number of rows and columns in the domain tiling to be used per processor	-1	-1	-mf	f	mf
20	5	2	t	1 1	it specifies the number of rows and columns in the domain tiling to be used per processor	-1	-1	-mf	f	mf
21	6	1	f	 	ad file	-1	-1	-ad	f	ad
22	6	1	f	 	fel file	-1	-1	-fel	f	fel
\.


--
-- Name: process_parameters_process_parameter_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('process_parameters_process_parameter_id_seq', 22, true);


--
-- Data for Name: program_authors; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY program_authors (author_id, program_id) FROM stdin;
1	1
\.


--
-- Data for Name: programs; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY programs (program_id, name, type, description, version, enabled, created) FROM stdin;
1	pitremove	2	Pit Remove creates a hydrologically correct DEM by raising the elevation of pits to the point where they overflow their confining pour point and can drain to the edge of the domain.	5.2	t	2014-02-13 15:41:00
2	ueb	2	UEB	1.0	t	2014-03-08 17:07:00
4	d8flowdir	2	D8 Flow Directions	5.2	t	2014-05-04 16:58:00
5	dinfflowdir	2	Assigns a flow direction based on the D-infinity flow method using the steepest slope of a triangular facet	5.2	t	2014-05-04 22:12:00
6	gdalslop.sh	3	Compute slope and aspect using GDAL	1.0	t	2014-09-03 01:31:00
\.


--
-- Name: programs_program_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('programs_program_id_seq', 6, true);


--
-- Data for Name: user_groups; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY user_groups (group_id, user_id) FROM stdin;
\.


--
-- Data for Name: users; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY users (user_id, name, password, salt, enabled, created, description) FROM stdin;
1	root	encryptedpass	vvY6bMoTScBlTLZe	t	2014-02-04 16:30:47	root user
\.


--
-- Name: users_user_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('users_user_id_seq', 1, true);


--
-- Name: author_id; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY authors
    ADD CONSTRAINT author_id PRIMARY KEY (author_id);


--
-- Name: group_id; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY groups
    ADD CONSTRAINT group_id PRIMARY KEY (group_id);


--
-- Name: hpc_id; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY hpc_centers
    ADD CONSTRAINT hpc_id PRIMARY KEY (hpc_id);


--
-- Name: hpc_membership_pk; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY hpc_membership
    ADD CONSTRAINT hpc_membership_pk PRIMARY KEY (hpc_id, group_id);


--
-- Name: hpc_programs_pk; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY hpc_programs
    ADD CONSTRAINT hpc_programs_pk PRIMARY KEY (hpc_id, program_id);


--
-- Name: hpc_templates_pk; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY hpc_templates
    ADD CONSTRAINT hpc_templates_pk PRIMARY KEY (hpc_id);


--
-- Name: job_id; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY jobs
    ADD CONSTRAINT job_id PRIMARY KEY (job_id, user_id, hpc_id, package_id);


--
-- Name: packages_pk; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY packages
    ADD CONSTRAINT packages_pk PRIMARY KEY (package_id, hpc_id, user_id);


--
-- Name: process_parameter_id; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY process_parameters
    ADD CONSTRAINT process_parameter_id PRIMARY KEY (process_parameter_id, program_id);


--
-- Name: program_authors_pk; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY program_authors
    ADD CONSTRAINT program_authors_pk PRIMARY KEY (author_id, program_id);


--
-- Name: programs_pk; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY programs
    ADD CONSTRAINT programs_pk PRIMARY KEY (program_id);


--
-- Name: user_groups_pk; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY user_groups
    ADD CONSTRAINT user_groups_pk PRIMARY KEY (group_id, user_id);


--
-- Name: user_id; Type: CONSTRAINT; Schema: public; Owner: postgres; Tablespace: 
--

ALTER TABLE ONLY users
    ADD CONSTRAINT user_id PRIMARY KEY (user_id);


--
-- Name: authors_program_authors_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY program_authors
    ADD CONSTRAINT authors_program_authors_fk FOREIGN KEY (author_id) REFERENCES authors(author_id);


--
-- Name: groups_hpc_membership_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY hpc_membership
    ADD CONSTRAINT groups_hpc_membership_fk FOREIGN KEY (group_id) REFERENCES groups(group_id);


--
-- Name: groups_user_groups_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY user_groups
    ADD CONSTRAINT groups_user_groups_fk FOREIGN KEY (group_id) REFERENCES groups(group_id);


--
-- Name: hpc_centers_hpc_membership_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY hpc_membership
    ADD CONSTRAINT hpc_centers_hpc_membership_fk FOREIGN KEY (hpc_id) REFERENCES hpc_centers(hpc_id);


--
-- Name: hpc_centers_hpc_programs_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY hpc_programs
    ADD CONSTRAINT hpc_centers_hpc_programs_fk FOREIGN KEY (hpc_id) REFERENCES hpc_centers(hpc_id);


--
-- Name: hpc_centers_hpc_templates_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY hpc_templates
    ADD CONSTRAINT hpc_centers_hpc_templates_fk FOREIGN KEY (hpc_id) REFERENCES hpc_centers(hpc_id);


--
-- Name: hpc_centers_jobs_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY jobs
    ADD CONSTRAINT hpc_centers_jobs_fk FOREIGN KEY (hpc_id) REFERENCES hpc_centers(hpc_id);


--
-- Name: hpc_centers_packages_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY packages
    ADD CONSTRAINT hpc_centers_packages_fk FOREIGN KEY (hpc_id) REFERENCES hpc_centers(hpc_id);


--
-- Name: packages_jobs_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY jobs
    ADD CONSTRAINT packages_jobs_fk FOREIGN KEY (package_id, hpc_id, user_id) REFERENCES packages(package_id, hpc_id, user_id);


--
-- Name: programs_hpc_programs_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY hpc_programs
    ADD CONSTRAINT programs_hpc_programs_fk FOREIGN KEY (program_id) REFERENCES programs(program_id);


--
-- Name: programs_process_parameters_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY process_parameters
    ADD CONSTRAINT programs_process_parameters_fk FOREIGN KEY (program_id) REFERENCES programs(program_id);


--
-- Name: programs_program_authors_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY program_authors
    ADD CONSTRAINT programs_program_authors_fk FOREIGN KEY (program_id) REFERENCES programs(program_id);


--
-- Name: users_jobs_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY jobs
    ADD CONSTRAINT users_jobs_fk FOREIGN KEY (user_id) REFERENCES users(user_id);


--
-- Name: users_packages_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY packages
    ADD CONSTRAINT users_packages_fk FOREIGN KEY (user_id) REFERENCES users(user_id);


--
-- Name: users_user_groups_fk; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY user_groups
    ADD CONSTRAINT users_user_groups_fk FOREIGN KEY (user_id) REFERENCES users(user_id);


--
-- Name: public; Type: ACL; Schema: -; Owner: postgres
--

REVOKE ALL ON SCHEMA public FROM PUBLIC;
REVOKE ALL ON SCHEMA public FROM postgres;
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO PUBLIC;


--
-- PostgreSQL database dump complete
--

