AC_INIT([ZOO Kernel], [1.7.0], [bugs@zoo-project.org])

AC_CONFIG_MACRO_DIR([macros])

# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
AC_PROG_CXX
AC_PROG_SED

# Checks for libraries.
AC_CHECK_LIB([curl], [curl_easy_init,curl_easy_setopt,curl_easy_cleanup,curl_easy_perform])
AC_CHECK_LIB([dl], [dlopen,dlsym,dlerror,dlclose])
AC_CHECK_LIB([crypto], [EVP_DigestInit,EVP_md5,EVP_DigestUpdate,BIO_f_base64,BIO_new])

DEFAULT_LIBS="$LIBS"
AC_SUBST([DEFAULT_LIBS])


# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h inttypes.h malloc.h stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 getcwd memset setenv strdup strstr])

#============================================================================
# Detect if uuid is available
#============================================================================

AC_ARG_WITH([uuid],
    [AS_HELP_STRING([--with-uuid=PATH], [Specifies an alternative location for the ossp-uuid library])],
    [UUID_DIR="$withval"], [UUID_DIR="/usr/"])
UUID_CFLAGS="-I$UUID_DIR/include"
UUID_LDFLAGS="-L$UUID_DIR/lib -luuid"
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$UUID_CFLAGS"
LIBS_SAVE="$LIBS"
LIBS="$UUID_LDFLAGS"
AC_CHECK_HEADERS([uuid/uuid.h],
			[], [AC_MSG_ERROR([could not find header file uuid.h])])
AC_CHECK_LIB([uuid], [uuid_generate_time],
			[], [AC_MSG_ERROR([could not find function in uuid library])])
CPPFLAGS="$CPPFLAGS_SAVE"
AC_SUBST([UUID_CFLAGS])
AC_SUBST([UUID_LDFLAGS])

#============================================================================
# Detect if json-c is available
#============================================================================

AC_ARG_WITH([callback],
    [AS_HELP_STRING([--with-callback=yes], [Activate callback invocation during HPC execution])],
    [CALLBACK_ACTIVATED="$withval"], [CALLBACK_ACTIVATED="no"])
    
AC_ARG_WITH([json],
    [AS_HELP_STRING([--with-json=PATH], [Specifies an alternative location for the json-c library])],
    [JSON_DIR="$withval"], [JSON_DIR="/usr/"])
if test "x$JSON_DIR" != "x"
then
	JSON_CPPFLAGS="-DUSE_JSON -I$JSON_DIR/include/json-c/"
	JSON_LDFLAGS="-L$JSON_DIR/lib -ljson-c"
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$JSON_CPPFLAGS"
	LIBS_SAVE="$LIBS"
	LIBS="$JSON_LDFLAGS"
	AC_CHECK_HEADERS([json_object.h],
		[], [AC_MSG_ERROR([could not find header file json_object.h])])
	AC_CHECK_LIB([json-c], [json_object_new_object],
		[], [AC_MSG_ERROR([could not find function in json-c library])])
	CPPFLAGS="$CPPFLAGS_SAVE"
	JSON_FILE="service_json.o"
	JSON_ENABLED="-DJSON"
	if test "x$CALLBACK_ACTIVATED" == "xyes"
	then
		CALLBACK_FILE="service_callback.o"
		CALLBACK_USE="-DUSE_CALLBACK"
	fi
fi
AC_SUBST([JSON_CPPFLAGS])
AC_SUBST([JSON_LDFLAGS])
AC_SUBST([JSON_FILE])
AC_SUBST([JSON_ENABLED])
AC_SUBST([CALLBACK_FILE])
AC_SUBST([CALLBACK_USE])

#============================================================================
# Detect if openssl is available
#============================================================================

AC_ARG_WITH([openssl],
    [AS_HELP_STRING([--with-openssl=PATH], [Specifies an alternative location for the openssl library])],
    [OPENSSL_DIR="$withval"], [OPENSSL_DIR="/usr/"])
OPENSSL_CFLAGS="-I$OPENSSL_DIR/include"
OPENSSL_LDFLAGS="-lcrypto -L$OPENSSL_DIR/lib -lssl"
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$OPENSSL_CFLAGS"
LIBS_SAVE="$LIBS"
LIBS="$OPENSSL_LDFLAGS"
AC_CHECK_HEADERS([openssl/md5.h openssl/hmac.h openssl/evp.h openssl/bio.h openssl/buffer.h],
			[], [AC_MSG_ERROR([could not find header file $i related to openssl])])
AC_CHECK_LIB(crypto, BIO_f_base64,
			[], [AC_MSG_ERROR([could not find $i function in openssl library])])
CPPFLAGS="$CPPFLAGS_SAVE"
AC_SUBST([OPENSSL_CFLAGS])
AC_SUBST([OPENSSL_LDFLAGS])

#============================================================================
# Detect if gettext is available
#============================================================================

#AC_ARG_WITH([gettext],
#    [AS_HELP_STRING([--with-gettext=PATH], [Specifies an alternative location for the openssl library])],
#    [GETTEXT_DIR="$withval"], [GETTEXT_DIR="/usr/"])

#GETTEXT_CFLAGS="-I$GETTEXT_DIR/include"
#GETTEXT_LDFLAGS="-L$GETTEXT_DIR/lib -lintl"
#CFLAGS_SAVE="$CFLAGS"
#CFLAGS="$GETTEXT_CFLAGS"
#LIBS_SAVE="$LIBS"
#LIBS="$GETTEXT_LDFLAGS"
#AM_GNU_GETTEXT([external], [], [])
#AC_CHECK_LIB(intl,
#			[dgettext], [] , [AC_MSG_ERROR([could not find $i function in gettext library])])
#AC_SUBST([GETTEXT_CFLAGS])
#AC_SUBST([GETTEXT_LDFLAGS])

#============================================================================
# Detect if run on debian / ubuntu
#============================================================================
if test -f "/usr/bin/dpkg"
then
	DEB_DEF=-DDEB
fi
AC_SUBST([DEB_DEF])

AC_ARG_WITH([etc-dir],
    [AS_HELP_STRING([--with-etc-dir=PATH], [Specifies an alternative path to store the main.cfg file ( default: ZOO-Kernel location) ])],
    [ETC_DEF="#define ETC_DIR \\\"${sysconfdir}\\\""], [ETC_DEF=""])
AC_SUBST([ETC_DEF])

AC_ARG_WITH([cgi-dir],
    [AS_HELP_STRING([--with-cgi-dir=PATH], [Specifies an alternative cgi directory path ( default: /usr/lib/cgi-bin) ])],
    [CGI_DIR="$withval"], [CGI_DIR="/usr/lib/cgi-bin"])
AC_SUBST([CGI_DIR])

AC_ARG_WITH([db-backend],
    [AS_HELP_STRING([--with-db-backend], [Relies on a database for storing status messages and response files ])],
    [RELY_ON_DB="-DRELY_ON_DB"], [RELY_ON_DB=""])
AC_SUBST([RELY_ON_DB])


# ===========================================================================
# Detect if libyaml is available
# ===========================================================================

AC_ARG_WITH([yaml],
        [AS_HELP_STRING([--with-yaml=PATH], [Specifies an alternative location for the yaml library])],
        [YAMLPATH="$withval"], [YAMLPATH=""])

if test -z "$YAMLPATH"
then
	YAML_LDFLAGS=""
	YAML_CPPFLAGS=""
	YAML_FILE=""
	YAML_FILE1=""
else

	# Extract the linker and include flags
	YAML_LDFLAGS="-L$YAMLPATH/lib -lyaml"
	YAML_CPPFLAGS="-I$YAMLPATH/include -DYAML"
	YAML_FILE="service_yaml.o"
	YAML_FILE1="zcfg2yaml"
	
	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$YAML_CPPFLAGS"
	LIBS_SAVE="$LIBS"
	LIBS="$YAML_LDFLAGS"
	AC_CHECK_LIB([yaml], [yaml_parser_initialize,yaml_parser_set_input_file,yaml_parser_scan])
	AC_CHECK_HEADERS([yaml.h],
                 [], [AC_MSG_ERROR([could not find headers include related to YAML])])
	LIBS="$LIBS_SAVE"
fi
AC_SUBST([YAML_CPPFLAGS])
AC_SUBST([YAML_LDFLAGS])
AC_SUBST([YAML_FILE])
AC_SUBST([YAML_FILE1])

# ===========================================================================
# Detect if fastcgi is available
# ===========================================================================

AC_ARG_WITH([fastcgi],
        [AS_HELP_STRING([--with-fastcgi=PATH], [Specifies an alternative location for the fastcgi library])],
        [FCGIPATH="$withval"], [FCGIPATH="/usr"])

# Extract the linker and include flags
FCGI_LDFLAGS="-L$FCGIPATH/lib -lfcgi"
FCGI_CPPFLAGS="-I$FCGIPATH/include"

# Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$FCGI_CPPFLAGS"
LIBS_SAVE="$LIBS"
LIBS="$FCGI_LDFLAGS"
AC_CHECK_LIB([fcgi], [main])
AC_CHECK_HEADERS([fcgi_stdio.h],
                 [], [AC_MSG_ERROR([could not find headers include related to fastcgi])])
LIBS="$LIBS_SAVE"
AC_SUBST([FCGI_CPPFLAGS])
AC_SUBST([FCGI_LDFLAGS])

AC_ARG_WITH([metadb],
        [AS_HELP_STRING([--with-metadb=yes], [Activates the metadata database support])],
        [WITHMETADB="$withval"], [WITHMETADB=""])

if test "x$WITHMETADB" = "xyes"; then
   	METADB="-DMETA_DB"
	METADB_FILE="meta_sql.o sqlapi.o"
else
	METADB=""
	METADB_FILE=""
fi

AC_SUBST([METADB])
AC_SUBST([METADB_FILE])

AC_ARG_WITH([hpc],
        [AS_HELP_STRING([--with-hpc=yes], [Specifies if you need to activate HPC support])],
        [HPCWITH="$withval"], [HPCWITH="no"])


AC_ARG_WITH([ssh2],
        [AS_HELP_STRING([--with-ssh2=PATH], [Specifies an alternative location for the ssh2 library])],
        [SSH2PATH="$withval"], [SSH2PATH="/usr"])

if test "x$HPCWITH" = "xyes"; then
   	HPC_FILES="service_internal_hpc.o sshapi.o"
	HPC_ENABLED="-DUSE_HPC"
	HPC_CPPFLAGS=""
	HPC_LDFLAGS=""
	# Extract the linker and include flags
	SSH2_LDFLAGS="-L$SSH2PATH/lib -lssh2"
	SSH2_CPPFLAGS="-I$SSH2PATH/include"
	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$SSH2_CPPFLAGS"
	LIBS_SAVE="$LIBS"
	LIBS="$SSH2_LDFLAGS"
	
	AC_CHECK_HEADERS([libssh2.h],
                 [], [AC_MSG_ERROR([could not find headers related to libssh2])])
	AC_CHECK_LIB([ssh2], [libssh2_session_init])

	LIBS="$LIBS_SAVE"
fi

AC_SUBST([HPC_CPPFLAGS])
AC_SUBST([HPC_LDFLAGS])
AC_SUBST([HPC_ENABLED])
AC_SUBST([HPC_FILES])

AC_SUBST([SSH2_CPPFLAGS])
AC_SUBST([SSH2_LDFLAGS])

# ===========================================================================
# Detect if libxml2 is installed
# ===========================================================================

AC_ARG_WITH([xml2config], 
	[AS_HELP_STRING([--with-xml2config=FILE], [Specifies an alternative xml2-config file])], 
	[XML2CONFIG="$withval"], [XML2CONFIG=""])

if test "x$XML2CONFIG" = "x"; then
	# XML2CONFIG was not specified, so search within the current path
	AC_PATH_PROG([XML2CONFIG], [xml2-config])

	# If we couldn't find xml2-config, display a warning
	if test "x$XML2CONFIG" = "x"; then
		AC_MSG_ERROR([could not find xml2-config from libxml2 within the current path. You may need to try re-running configure with a --with-xml2config parameter.])
	fi
else
	# XML2CONFIG was specified; display a message to the user
	if test "x$XML2CONFIG" = "xyes"; then
		AC_MSG_ERROR([you must Specifies a parameter to --with-xml2config, e.g. --with-xml2config=/path/to/xml2-config])
	else
		if test -f $XML2CONFIG; then
			AC_MSG_RESULT([Using user-specified xml2-config file: $XML2CONFIG])
		else
			AC_MSG_ERROR([the user-specified xml2-config file $XML2CONFIG does not exist])
		fi	
	fi
fi

# Extract the linker and include flags 
XML2_LDFLAGS=`$XML2CONFIG --libs`
XML2_CPPFLAGS=`$XML2CONFIG --cflags`

# Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$XML2_CPPFLAGS"
AC_CHECK_HEADERS([libxml/tree.h libxml/parser.h libxml/xpath.h libxml/xpathInternals.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libxml2])])

# Ensure we can link against libxml2
LIBS_SAVE="$LIBS"
LIBS="$XML2_LDFLAGS"
AC_CHECK_LIB([xml2], [xmlInitParser], [], [AC_MSG_ERROR([could not find libxml2])], [])

AC_SUBST([XML2_CPPFLAGS])
AC_SUBST([XML2_LDFLAGS])
LIBS="$LIBS_SAVE"


# ===========================================================================
# Detect if libxslt is installed
# ===========================================================================

AC_ARG_WITH([xsltconfig], 
	[AS_HELP_STRING([--with-xsltconfig=FILE], [Specifies an alternative xslt-config file])], 
	[XSLTCONFIG="$withval"], [XSLTCONFIG=""])

if test "x$XSLTCONFIG" = "x"; then
	# XSLTCONFIG was not specified, so search within the current path
	AC_PATH_PROG([XSLTCONFIG], [xslt-config])

	# If we couldn't find xslt-config, display a warning
	if test "x$XSLTCONFIG" = "x"; then
		AC_MSG_ERROR([could not find xslt-config from libxslt within the current path. You may need to try re-running configure with a --with-xtltconfig parameter.])
	fi
else
	# XSLTCONFIG was specified; display a message to the user
	if test "x$XSLTCONFIG" = "xyes"; then
		AC_MSG_ERROR([you must Specifies a parameter to --with-xsltconfig, e.g. --with-xsltconfig=/path/to/xslt-config])
	else
		if test -f $XSLTCONFIG; then
			AC_MSG_RESULT([Using user-specified xslt-config file: $XSLTCONFIG])
		else
			AC_MSG_ERROR([the user-specified xslt-config file $XSLTCONFIG does not exist])
		fi	
	fi
fi

# Extract the linker and include flags 
XSLT_LDFLAGS=`$XSLTCONFIG --libs`
XSLT_CPPFLAGS=`$XSLTCONFIG --cflags`

# Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$XSLT_CPPFLAGS"
AC_CHECK_HEADERS([libxslt/xslt.h libxslt/xsltInternals.h libxslt/transform.h libxslt/xsltutils.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libxlst])])

AC_SUBST([XSLT_CPPFLAGS])
AC_SUBST([XSLT_LDFLAGS])

#============================================================================
# Detect if gdal is installed
#============================================================================

AC_ARG_WITH([gdal-config], 
	[AS_HELP_STRING([--with-gdal-config=FILE], [Specifies an alternative gdal-config file])], 
	[GDAL_CONFIG="$withval"], [GDAL_CONFIG=""])
if test -z $GDAL_CONFIG;
then
	AC_PATH_PROG([GDAL_CONFIG], [gdal-config])
	if test -z $GDAL_CONFIG; 
	then
		AC_MSG_ERROR([could not find gdal-config from libgdal within the current path. You may need to try re-running configure with a --with-gdal-config parameter.])
	fi
	
else
	if test -f $GDAL_CONFIG; then
		AC_MSG_RESULT([Using user-specified gdal-config file: $GDAL_CONFIG])
	else
		AC_MSG_ERROR([the user-specified gdal-config file $GDAL_CONFIG does not exist])
	fi
fi

GDAL_CFLAGS="`$GDAL_CONFIG --cflags`"
GDAL_LIBS="`$GDAL_CONFIG --libs`"

AC_SUBST([GDAL_CFLAGS])
AC_SUBST([GDAL_LIBS])

# ===========================================================================
# Detect if proj is installed
# ===========================================================================

AC_ARG_WITH([proj],
        [AS_HELP_STRING([--with-proj=PATH], [Specifies an alternative location for PROJ4 setup])],
        [PROJPATH="$withval"], [PROJPATH=""])

# Extract the linker and include flags
PROJ_LDFLAGS="-L$PROJPATH/lib"
PROJ_CPPFLAGS="-I$PROJPATH/include"

# Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$PROJ_CPPFLAGS"
AC_CHECK_HEADERS([proj_api.h],
                 [], [AC_MSG_ERROR([could not find headers include related to PROJ4])])

AC_SUBST([PROJ_CPPFLAGS])
AC_SUBST([PROJ_LDFLAGS])

# ===========================================================================
# Detect if libgeos is installed
# ===========================================================================

AC_ARG_WITH([geosconfig], 
	[AS_HELP_STRING([--with-geosconfig=FILE], [Specifies an alternative geos-config file])], 
	[GEOSCONFIG="$withval"], [GEOSCONFIG=""])

if test "x$GEOSCONFIG" = "x"; then
	# GEOSCONFIG was not specified, so search within the current path
	AC_PATH_PROG([GEOSCONFIG], [geos-config])

	# If we couldn't find geos-config, display a warning
	if test "x$GEOSCONFIG" = "x"; then
		AC_MSG_WARN([could not find geos-config from libgeos within the current path. You may need to try re-running configure with a --with-geosconfig parameter.])
	fi
else
	# GEOSCONFIG was specified; display a message to the user
	if test "x$GEOSCONFIG" = "xyes"; then
		AC_MSG_WARN([you must Specifies a parameter to --with-geosconfig, e.g. --with-geosconfig=/path/to/geos-config])
	else
		if test -f $GEOSCONFIG; then
			AC_MSG_RESULT([Using user-specified geos-config file: $GEOSCONFIG])
		else
			AC_MSG_ERROR([the user-specified geos-config file $GEOSCONFIG does not exist])
		fi	
	fi
fi

GEOS_LDFLAGS=`$GEOSCONFIG --libs`
GEOS_CPPFLAGS=`$GEOSCONFIG --cflags`

# Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$GEOS_CPPFLAGS"
AC_CHECK_HEADERS([geos_c.h],
		 [], [AC_MSG_WARN([could not find headers include related to libgeos])])

AC_SUBST([GEOS_CPPFLAGS])
AC_SUBST([GEOS_LDFLAGS])


# ===========================================================================
# Detect if cgal is installed
# ===========================================================================

AC_ARG_WITH([cgal],
        [AS_HELP_STRING([--with-cgal=PATH], [Specifies an alternative location for CGAL setup])],
        [CGALPATH="$withval"], [CGALPATH="/usr"])


# Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CGAL_CPPFLAGS="-I$CGALPATH/include"
CPPFLAGS="$CGAL_CPPFLAGS"
AC_CHECK_HEADERS([CGAL/Delaunay_triangulation_2.h],
         [], [AC_MSG_WARN([could not find headers include related to libCGAL])])

# Extract the linker and include flags
CGAL_LDFLAGS="-L$CGALPATH/lib"
CGAL_CPPFLAGS="-I$CGALPATH/include"


AC_SUBST([CGAL_CPPFLAGS])
AC_SUBST([CGAL_LDFLAGS])
#============================================================================
# Detect if mapserver is installed
#============================================================================

AC_ARG_WITH([mapserver], 
       [AS_HELP_STRING([--with-mapserver=PATH], [Specifies the path for MapServer compiled source tree])], 
       [MS_SRC_PATH="$withval"], [MS_SRC_PATH=""])

AC_ARG_WITH([ms-version], 
       [AS_HELP_STRING([--with-ms-version=VERSION], [Specifies the MapServer version to build against])], 
       [MS_VERSION="$withval"], [MS_VERSION=""])

if test -z $MS_SRC_PATH;
then
	MS_CPPFLAGS=""
	MS_LDFLAGS=""
else
       if test "x$MS_SRC_PATH" = "xmacos";
       then
               MS_LDFLAGS="/Library/Frameworks/MapServer.framework//Versions/6.0/MapServer -lintl"
               MS_CPPFLAGS="-DUSE_MS `/Library/Frameworks/MapServer.framework/Programs/mapserver-config --includes` -I/Library/Frameworks/MapServer.framework/Versions/Current/Headers/ -I../mapserver "
               AC_MSG_WARN([Please make sure that ../mapserver exists and contains MapServer source tree])
               AC_MSG_RESULT([Using MacOS X Framework for MapServer])
       else
	if test "x$MS_VERSION" = "x7";
	then
		MS_LDFLAGS="-L$MS_SRC_PATH/lib -lmapserver"
		MS_CPPFLAGS="-DUSE_MS -I$MS_SRC_PATH/include/mapserver "
               	AC_MSG_RESULT([Using user-specified MapServer src path: $MS_SRC_PATH])
        else
               if test -d $MS_SRC_PATH; then
                       MS_LDFLAGS="-L$MS_SRC_PATH -lmapserver `$MS_SRC_PATH/mapserver-config --libs`"
                       MS_CPPFLAGS="-DUSE_MS `$MS_SRC_PATH/mapserver-config --includes` `$MS_SRC_PATH/mapserver-config --cflags` -I$MS_SRC_PATH "
               
                       AC_MSG_RESULT([Using user-specified MapServer src path: $MS_SRC_PATH])
               else
                       AC_MSG_ERROR([the user-specified mapserver-config file $MS_SRC_PATH does not exist])
               fi
        fi
       fi
       MS_FILE="service_internal_ms.o"
fi

MS_CFLAGS="$MS_CPPFLAGS"
MS_LIBS="$MS_LDFLAGS"

AC_SUBST([MS_CFLAGS])
AC_SUBST([MS_LIBS])
AC_SUBST([MS_FILE])
AC_SUBST([MS_VERSION])

# ===========================================================================
# Detect if R is installed
# ===========================================================================

AC_ARG_WITH([r], 
	[AS_HELP_STRING([--with-r=PATH], [To enable python support or Specifies an alternative directory for R installation,  disabled by default])], 
	[R_PATH="$withval"; R_ENABLED="-DUSE_R"], [R_ENABLED=""])

if test -z "$R_ENABLED"
then
	R_FILE=""
else
	R_FILE="service_internal_r.o"
	# Extract the linker and include flags 
	R_LDFLAGS="-L$R_PATH/lib/ -lR"
	R_CPPFLAGS="-I$R_PATH/include/ -I$R_PATH/share/R/include/"

	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$R_CPPFLAGS"
	AC_CHECK_HEADERS([Rinternals.h],
		 [], [AC_MSG_ERROR([could not find headers include related to R])])

	# Ensure we can link against libphp
	#LIBS_SAVE="$LIBS"
	#LIBS="$R_LDFLAGS"
	#AC_CHECK_LIB([$LIBS], [R_tryEval], [], [AC_MSG_ERROR([could not find libR])], [])
	#LIBS="$LIBS_SAVE"
fi

AC_SUBST([R_CPPFLAGS])
AC_SUBST([R_LDFLAGS])
AC_SUBST([R_ENABLED])
AC_SUBST([R_FILE])


# ===========================================================================
# Detect if python is installed
# ===========================================================================

AC_ARG_WITH([python], 
	[AS_HELP_STRING([--with-python=PATH], [To enable python support or Specifies an alternative directory for python installation,  disabled by default])], 
	[PYTHON_PATH="$withval"; PYTHON_ENABLED="-DUSE_PYTHON"], [PYTHON_ENABLED=""])

AC_ARG_WITH([pyvers], 
	[AS_HELP_STRING([--with-pyvers=NUM], [To use a specific python version])], 
	[PYTHON_VERS="$withval"], [PYTHON_VERS=""])


if test -z "$PYTHON_ENABLED"
then
	PYTHON_FILE=""
else
	PYTHONCONFIG="$PYTHON_PATH/bin/python${PYTHON_VERS}-config"
	PYTHON_FILE="service_internal_python.o"
	if test  "$PYTHON_PATH" = "yes"
	then
		# PYTHON was not specified, so search within the current path
		PYTHONCFG_PATH=`which python${PYTHON_VERS}-config`
		if test -z "${PYTHONCFG_PATH}" ; then
		AC_PATH_PROG([PYTHONCONFIG], [python-config-${PYTHON_VERS}])
		else
		AC_PATH_PROG([PYTHONCONFIG], [python${PYTHON_VERS}-config])
		fi
	else
		PYTHONCONFIG="$PYTHON_PATH/bin/python${PYTHON_VERS}-config"
	fi

	# Extract the linker and include flags 
	PYTHON_LDFLAGS=`$PYTHONCONFIG --ldflags`
	PYTHON_CPPFLAGS=`$PYTHONCONFIG --includes`

	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$PYTHON_CPPFLAGS"
	AC_CHECK_HEADERS([Python.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libpython])])

	# Ensure we can link against libphp
	LIBS_SAVE="$LIBS"
	LIBS="$PYTHON_LDFLAGS"
	PY_LIB=`$PYTHONCONFIG --libs | sed \
			      -e 's/.*\(python[[0-9]]\.[[0-9]]\).*/\1/'`
	AC_CHECK_LIB([$PY_LIB], [PyObject_CallObject], [], [AC_MSG_ERROR([could not find libpython])], [])
	LIBS="$LIBS_SAVE"
fi

AC_SUBST([PYTHON_CPPFLAGS])
AC_SUBST([PYTHON_LDFLAGS])
AC_SUBST([PYTHON_ENABLED])
AC_SUBST([PYTHON_FILE])

# ===========================================================================
# Detect if spidermonkey is installed
# ===========================================================================

AC_ARG_WITH([js], 
	[AS_HELP_STRING([--with-js=PATH], [Specifies --with-js=path-to-js to enable js support, specify --with-js on linux debian like, js support is disabled by default ])], 
	[JSHOME="$withval";JS_ENABLED="-DUSE_JS"], [JS_ENABLED=""])

if test -z "$JS_ENABLED"
then
	JS_FILE=""
else
	JS_FILE="service_internal_js.o"
	if test "$JSHOME" = "yes"
	then

		#on teste si on est sous debian like 
		if test -f "/usr/bin/dpkg"
		then
			if test -n "`dpkg -l | grep libmozjs185-dev`"
			then
				JS_CPPFLAGS="-I/usr/include/js/"
                        	JS_LDFLAGS="-L/usr/lib -lmozjs185 -lm"
                        	JS_LIB="mozjs185"
			else 
				XUL_VERSION="`dpkg -l | grep xulrunner | grep dev | head -1| awk '{print $3;}' | cut -d'+' -f1`"
				if test -n "$XUL_VERSION"
				then
					JS_CPPFLAGS="-I/usr/include/xulrunner-$XUL_VERSION"
					JS_LDFLAGS="-L/usr/lib/xulrunner-$XUL_VERSION -lmozjs -lm"
					JS_LIB="mozjs"
				else
					AC_MSG_ERROR([You must install libmozjs185-dev or xulrunner-dev ])
				fi
			fi
		else
			AC_MSG_ERROR([You must  specify your custom install of libmozjs185])
		fi
	else
		JS_CPPFLAGS="-I$JSHOME/include/js/"
                JS_LDFLAGS="-L$JSHOME/lib -lmozjs185 -lm"
                JS_LIB="mozjs185"

	fi 
	CPPFLAGS_SAVE="$CPPFLAGS"
        CPPFLAGS="$JS_CPPFLAGS"
	AC_LANG_PUSH([C++])
	AC_CHECK_HEADERS([jsapi.h],
                        [], [AC_MSG_ERROR([could not find headers include related to libjs])])

	AC_LANG_POP([C++])
	LIBS_SAVE="$LIBS"
        LIBS="$JS_LDFLAGS"

        AC_CHECK_LIB([$JS_LIB], [JS_CompileFile,JS_CallFunctionName], [], [AC_MSG_ERROR([could not find $JS_LIB])], [])
	LIBS="$LIBS_SAVE"
	
        AC_SUBST([JS_CPPFLAGS])
        AC_SUBST([JS_LDFLAGS])
fi

AC_SUBST([JS_ENABLED])
AC_SUBST([JS_FILE])

# ===========================================================================
# Detect if php is installed
# ===========================================================================

AC_ARG_WITH([php], 
	[AS_HELP_STRING([--with-php=PATH], [To enable php support or specify an alternative directory for php installation,  disabled by default])], 
	[PHP_PATH="$withval"; PHP_ENABLED="-DUSE_PHP"], [PHP_ENABLED=""])

AC_ARG_WITH([php-version], 
	[AS_HELP_STRING([--with-phpvers=NUM], [To use a specific php version])], 
	[PHP_VERS="$withval"], [PHP_VERS=""])


if test -z "$PHP_ENABLED"
then
	PHP_FILE=""
else
	PHPCONFIG="$PHP_PATH/bin/php-config"
	if test "x$PHP_VERS" = "x7"
	then
		PHP_FILE="service_internal_php7.o"
	else
		PHP_FILE="service_internal_php.o"
	fi
	if test  "$PHP_PATH" = "yes"
	then
		# PHP was not specified, so search within the current path
		AC_PATH_PROG([PHPCONFIG], [php-config])
	else
		PHPCONFIG="$PHP_PATH/bin/php-config"
	fi

	# Extract the linker and include flags 
	if test "x$PHP_VERS" = "x7"
	then
		PHP_LDFLAGS="-L/`$PHPCONFIG --prefix`/lib64 -lphp7"
	else
		PHP_LDFLAGS="-L/`$PHPCONFIG --prefix`/lib -lphp5"
	fi
	PHP_CPPFLAGS=`$PHPCONFIG --includes`

	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$PHP_CPPFLAGS"
	AC_CHECK_HEADERS([sapi/embed/php_embed.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libphp])])

	# Ensure we can link against libphp
	LIBS_SAVE="$LIBS"
	LIBS="$PHP_LDFLAGS"
	# Shouldn't we get php here rather than php5 :) ??
	if test "x$PHP_VERS" = "x7"
	then
		echo $LIBS
		#AC_CHECK_LIB([php7], [call_user_function], [], [AC_MSG_ERROR([could not find libphp])], [])
	else
		AC_CHECK_LIB([php5], [call_user_function], [], [AC_MSG_ERROR([could not find libphp])], [])
	fi
	LIBS="$LIBS_SAVE"
	AC_SUBST([PHP_CPPFLAGS])
	AC_SUBST([PHP_LDFLAGS])
fi

AC_SUBST([PHP_ENABLED])
AC_SUBST([PHP_FILE])

# ===========================================================================
# Detect if java is installed
# ===========================================================================

AC_ARG_WITH([java], 
	[AS_HELP_STRING([--with-java=PATH], [To enable java support, specify a JDK_HOME,  disabled by default])], 
	[JDKHOME="$withval"; JAVA_ENABLED="-DUSE_JAVA"], [JAVA_ENABLED=""])

AC_ARG_WITH([java-rpath], 
	[AS_HELP_STRING([--with-java-rpath=yes], [To set rpath for java support, disabled by default])], 
	[JAVA_RPATH="$withval"], [JAVA_RPATH=""])

if test -z "$JAVA_ENABLED"
then
	JAVA_FILE=""
else
	JAVA_FILE="service_internal_java.o"
	if test "x$JDKHOME" = "x"; 
	then
		AC_MSG_ERROR([could not find java installation path within the current path. You may need to try re-running configure with a --with-java parameter.])
	fi	# JAVA was specified; display a message to the user
	if test "x$JDKHOME" = "xyes"; 
	then
		AC_MSG_ERROR([you must specify a parameter to --with-java, e.g. --with-java=/path/to/java])
	fi

	# Extract the linker and include flags
	if test "x$JDKHOME" = "xmacos";
	then
		JAVA_LDFLAGS="-framework JavaVM"
		for i in `ls /Applications/Xcode.app/Contents/Developer//Platforms/MacOSX.platform/Developer/SDKs/`; do
		    JAVA_CPPFLAGS="-I/Applications/Xcode.app/Contents/Developer//Platforms/MacOSX.platform/Developer/SDKs/$i/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers/"
		done
	else
		if test -d "$JDKHOME/jre/lib/i386";
		then
			JAVA_LDFLAGS="-L$JDKHOME/jre/lib/i386/server/ -ljvm -lpthread"
			JAVA_CPPFLAGS="-I$JDKHOME/include -I$JDKHOME/include/linux"
			if test x$JAVA_RPATH = "xyes"; then
			   JAVA_LDFLAGS="$JAVA_LDFLAGS -Wl,-rpath,$JDKHOME/jre/lib/i386/server/"
			fi
		else
			if test -d "$JDKHOME/jre/lib/amd64"; then
			   JAVA_LDFLAGS="-L$JDKHOME/jre/lib/amd64/server/ -ljvm -lpthread"
			   JAVA_CPPFLAGS="-I$JDKHOME/include -I$JDKHOME/include/linux"
			   if test x$JAVA_RPATH = "xyes"; then
			      JAVA_LDFLAGS="$JAVA_LDFLAGS -Wl,-rpath,$JDKHOME/jre/lib/amd64/server/"
			   fi
			else
			   JAVA_LDFLAGS="-L$JDKHOME/jre/lib/server/ -ljvm -lpthread"
			   JAVA_CPPFLAGS="-I$JDKHOME/include/ -I$JDKHOME/include/darwin"
			   if test x$JAVA_RPATH = "xyes"; then
			      JAVA_LDFLAGS="$JAVA_LDFLAGS -Wl,-rpath,$JDKHOME/jre/lib/server/"
			   fi
			fi
		fi
	fi

	AC_LANG([C++])
	#echo $JAVA_CPPFLAGS
	# Check headers file (second time we check that in fact)
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$JAVA_CPPFLAGS"
	AC_CHECK_HEADERS([jni.h],
			 [], [AC_MSG_ERROR([could not find jni.h file])])
	CPPFLAGS="$CPPFLAGS_SAVE"
	# Ensure we can link against libjava
	LIBS_SAVE="$LIBS"
	LIBS="$JAVA_LDFLAGS"
	if test "x$JDKHOME" != "xmacos";
	then
		AC_CHECK_LIB([jvm], [JNI_CreateJavaVM], [], [AC_MSG_ERROR([could not find libjvm])], [])
	fi
	LIBS="$LIBS_SAVE"

	AC_SUBST([JAVA_CPPFLAGS])
	AC_SUBST([JAVA_LDFLAGS])
fi 

AC_SUBST([JAVA_ENABLED])
AC_SUBST([JAVA_FILE])

# ===========================================================================
# Detect if mono is installed
# ===========================================================================

AC_ARG_WITH([mono], 
	[AS_HELP_STRING([--with-mono=PATH], [To enable mono support, specify the path to find pkg-config,  disabled by default])], 
	[MONOHOME="$withval"; MONO_ENABLED="-DUSE_MONO"], [MONO_ENABLED=""])

if test -z "$MONO_ENABLED"
then
	MONO_FILE=""
else
	MONO_FILE="service_internal_mono.o"
	if test "x$MONOHOME" = "x"; 
	then
		MONOHOME="/usr"
	fi
	if test "x$MONOHOME" = "xyes"; 
	then
		MONOHOME="/usr"
	fi

	# Extract the linker and include flags
	MONO_CFLAGS=`$MONOHOME/bin/pkg-config --cflags mono-2`
	MONO_LDFLAGS=`$MONOHOME/bin/pkg-config --libs mono-2`

	AC_LANG([C++])
	#echo $JAVA_CPPFLAGS
	# Check headers file (second time we check that in fact)
	CPPFLAGS_SAVE="$CFLAGS"
	CPPFLAGS="$MONO_CFLAGS"
	AC_CHECK_HEADERS([mono/jit/jit.h],
			 [], [AC_MSG_ERROR([could not find jit.h file])])
	CPPFLAGS="$CPPFLAGS_SAVE"
	# Ensure we can link against libmono-2.0
	LIBS_SAVE="$LIBS"
	LIBS="$MONO_LDFLAGS"
	AC_CHECK_LIB([mono-2.0], [mono_runtime_invoke], [], [AC_MSG_ERROR([could not find libmono])], [])
	LIBS="$LIBS_SAVE"

	AC_SUBST([MONO_CFLAGS])
	AC_SUBST([MONO_LDFLAGS])
fi 

AC_SUBST([MONO_ENABLED])
AC_SUBST([MONO_FILE])

# ===========================================================================
# Detect if ruby is installed
# ===========================================================================
AC_ARG_WITH([ruby], 
	[AS_HELP_STRING([--with-ruby=PATH], [To enable ruby support or specify an alternative directory for ruby installation,  disabled by default])], 
	[RUBY_PATH="$withval"; RUBY_ENABLED="-DUSE_RUBY"], [RUBY_ENABLED=""])

AC_ARG_WITH([rvers], 
	[AS_HELP_STRING([--with-rvers=NUM], [To use a specific ruby version])], 
	[RUBY_VERS="$withval"], [RUBY_VERS=""])


if test -z "$RUBY_ENABLED"
then
	RUBY_FILE=""
else
	RUBY_FILE="service_internal_ruby.o"

	# Extract the linker and include flags 
	RUBY_LDFLAGS="-lruby"
	RUBY_CPPFLAGS="-I$RUBY_PATH -I$RUBY_PATH/x86_64-darwin13.0/ -DZRUBY_VERSION=$RUBY_VERS"

	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$RUBY_CPPFLAGS"
	AC_CHECK_HEADERS([ruby.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libruby])])

	# Ensure we can link against libphp
	LIBS_SAVE="$LIBS"
	LIBS="$RUBY_LDFLAGS"
	# AC_CHECK_LIB([lruby], [PyObject_CallObject], [], [AC_MSG_ERROR([could not find libpython])], [])
	LIBS="$LIBS_SAVE"
	AC_SUBST([RUBY_CPPFLAGS])
	AC_SUBST([RUBY_LDFLAGS])
fi

AC_SUBST([RUBY_ENABLED])
AC_SUBST([RUBY_FILE])

# ===========================================================================
# Detect if perl is installed
# ===========================================================================

AC_ARG_WITH([perl], 
	[AS_HELP_STRING([--with-perl=PATH], [To enable perl support or specify an alternative directory for perl installation,  disabled by default])], 
	[PERL_PATH="$withval"; PERL_ENABLED="-DUSE_PERL"], [PERL_ENABLED=""])


if test -z "$PERL_ENABLED"
then
	PERL_FILE=""
else
	PERL_FILE="service_internal_perl.o"
	if test  "$PERL_PATH" = "yes"
	then
		# Perl was not specified, so search within the current path
		AC_PATH_PROG([PERLCONFIG], [perl])
	else
		PERLCONFIG="$PERL_PATH/bin/perl"
	fi

	# Extract the linker and include flags 
	PERL_LDFLAGS=`$PERLCONFIG -MExtUtils::Embed -e ldopts`
	PERL_CPPFLAGS=`$PERLCONFIG -MExtUtils::Embed -e ccopts`

	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$PERL_CPPFLAGS"
	AC_CHECK_HEADERS([EXTERN.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libperl])])

	AC_SUBST([PERL_CPPFLAGS])
	AC_SUBST([PERL_LDFLAGS])
fi

AC_SUBST([PERL_ENABLED])
AC_SUBST([PERL_FILE])

# ===========================================================================
# Detect if otb is available
# ===========================================================================

AC_ARG_WITH([itk],
        [AS_HELP_STRING([--with-itk=PATH], [Specifies an alternative location for the itk library])],
        [ITKPATH="$withval"], [ITKPATH=""])

AC_ARG_WITH([itk-version],
        [AS_HELP_STRING([--with-itk-version=VERSION], [Specifies an alternative version for the itk library])],
        [ITKVERS="$withval"], [ITKVERS=""])

AC_ARG_WITH([otb],
        [AS_HELP_STRING([--with-otb=PATH], [Specifies an alternative location for the otb library])],
        [OTBPATH="$withval"], [OTBPATH=""])

AC_ARG_WITH([otb-version],
        [AS_HELP_STRING([--with-otb-version=PATH], [Specifies an alternative location for the otb library])],
        [OTBVERS="$withval"], [OTBVERS=""])

if test -z "$OTBPATH"
then
	OTB_LDFLAGS=""
	OTB_CPPFLAGS=""
	OTB_FILE=""
	OTB_ENABLED=""
else
	if test -z "$ITKVERS" 
    	then
		ITKVERS="4.5"
    	fi
	OTB_ENABLED="-DUSE_OTB"
	IVERS="$(echo -e '4.10\n$ITKVERS' | sort -r | head -n1)"
	if test "$IVERS" == "$ITKVERS"; then
	   ITK_LDFLAGS="-lITKBiasCorrection-$ITKVERS -lITKCommon-$ITKVERS -lITKIOImageBase-$ITKVERS -lITKKLMRegionGrowing-$ITKVERS -lITKLabelMap-$ITKVERS -lITKMesh-$ITKVERS -lITKMetaIO-$ITKVERS -lITKOptimizers-$ITKVERS -lITKPath-$ITKVERS -lITKPolynomials-$ITKVERS -lITKQuadEdgeMesh-$ITKVERS -lITKSpatialObjects-$ITKVERS -lITKStatistics-$ITKVERS -lITKVNLInstantiation-$ITKVERS -lITKWatersheds-$ITKVERS -litkNetlibSlatec-$ITKVERS -litksys-$ITKVERS -litkv3p_netlib-$ITKVERS -litkvcl-$ITKVERS -litkvnl-$ITKVERS"
	else
		ITK_LDFLAGS="-lITKBiasCorrection-$ITKVERS -lITKCommon-$ITKVERS -lITKIOImageBase-$ITKVERS -lITKKLMRegionGrowing-$ITKVERS -lITKLabelMap-$ITKVERS -lITKMesh-$ITKVERS -lITKMetaIO-$ITKVERS -lITKOptimizers-$ITKVERS -lITKPath-$ITKVERS -lITKPolynomials-$ITKVERS -lITKQuadEdgeMesh-$ITKVERS -lITKSpatialObjects-$ITKVERS -lITKStatistics-$ITKVERS -lITKVNLInstantiation-$ITKVERS -lITKWatersheds-$ITKVERS -litkNetlibSlatec-$ITKVERS -litksys-$ITKVERS -litkv3p_netlib-$ITKVERS -litkvcl-$ITKVERS -litkvnl-$ITKVERS -litkvnl_algo-$ITKVERS"
	fi

	if test -a "${OTBPATH}/include/OTB-${OTBVERS}" ; then 
		OTB_RPATH="$OTBPATH/include/OTB-${OTBVERS}/"
		OTB_CPPFLAGS="-I${OTB_RPATH} -I${OTB_RPATH}ApplicationEngine -I$OTB_RPATH/Common -I$ITKPATH/include/ITK-$ITKVERS -I$OTB_RPATH/Utilities/ITK -I$OTB_RPATH/ -I$OTB_RPATH/IO -I$OTB_RPATH/UtilitiesAdapters/OssimAdapters -I$OTB_RPATH/UtilitiesAdapters/CurlAdapters -I$OTB_RPATH/Utilities/BGL -I$OTB_RPATH/UtilitiesAdapters/ITKPendingPatches -I$OTB_RPATH/Utilities/otbconfigfile $GDAL_CFLAGS"
		OTB_LDFLAGS="-L/usr/lib/x86_64-linux-gnu/ -lOTBImageIO-$OTBVERS -lOTBCommon-$OTBVERS -lOTBApplicationEngine-$OTBVERS -L$ITKPATH/lib $ITK_LDFLAGS"
	else
		OTB_CPPFLAGS="-I${OTB_RPATH} -I$OTBPATH/include/otb/ApplicationEngine -I$OTBPATH/include/otb/Common -I$ITKPATH/include/ITK-$ITKVERS -I$OTBPATH/include/otb/Utilities/ITK -I$OTBPATH/include/otb/ -I$OTBPATH/include/otb/IO -I$OTBPATH/include/otb/UtilitiesAdapters/OssimAdapters -I$OTBPATH/include/otb/UtilitiesAdapters/CurlAdapters -I$OTBPATH/include/otb/Utilities/BGL -I$OTBPATH/include/otb/UtilitiesAdapters/ITKPendingPatches -I$OTBPATH/include/otb/Utilities/otbconfigfile $GDAL_CFLAGS"
		OTB_LDFLAGS="-L$OTBPATH/lib/otb -lOTBIO -lOTBCommon -lOTBApplicationEngine -L$ITKPATH/lib $ITK_LDFLAGS"

	fi
	OTB_FILE="otbZooWatcher.o service_internal_otb.o"
	
	AC_LANG_PUSH([C++])
	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$OTB_CPPFLAGS"
	LDFLAGS_SAVE="$LDFLAGS"
	LIBS="$LIBS_SAVE $OTB_LDFLAGS"
	#echo $OTB_CPPFLAGS
	AC_CHECK_HEADERS([otbWrapperApplication.h otbWrapperInputImageListParameter.h otbWrapperApplicationRegistry.h],
			[], [AC_MSG_ERROR([could not find header file $i related to OTB])])
	LDFLAGS_SAVE="$LDFLAGS"
	LDFLAGS="$OTB_LDFLAGS"
	#echo $OTB_LDFLAGS
	#UVERS="$(echo -e '5.8\n$OTBVERS' | sort -r | head -n1)"
	#if test "$OTBVERS" ==  "$UVERS" ; then 
	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "otbWrapperApplicationRegistry.h"]],[[std::vector<std::string> list = otb::Wrapper::ApplicationRegistry::GetAvailableApplications();]])],
			[AC_MSG_RESULT([checking for GetAvailableApplications... yes])],[AC_MSG_ERROR([checking for GetAvailableApplications... failed])])
	#else
	#	AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "otbWrapperApplication.h"]],[[std::vector<std::string> list = otb::Wrapper::ApplicationRegistry::GetAvailableApplication();]])],
	#						   [AC_MSG_RESULT([checking for GetAvailableApplication... yes])],[AC_MSG_ERROR([checking for GetAvailableApplication... failed])])
	#fi
							   			
	LDFLAGS="$LDFLAGS_SAVE"
	AC_LANG_POP([C++])
	AC_LANG(C++)
			
fi
AC_SUBST([OTB_CPPFLAGS])
AC_SUBST([OTB_LDFLAGS])
AC_SUBST([OTB_FILE])
AC_SUBST([OTB_ENABLED])

# ===========================================================================
# Detect if saga-gis is available
# ===========================================================================

AC_ARG_WITH([wx-config],
        [AS_HELP_STRING([--with-wx-config=PATH], [Specifies an alternative path for the wx-config tool])],
        [WXCFG="$withval"], [WXCFG=""])

AC_ARG_WITH([saga],
        [AS_HELP_STRING([--with-saga=PATH], [Specifies an alternative location for the SAGA-GIS library])],
        [SAGAPATH="$withval"], [SAGAPATH=""])

AC_ARG_WITH([saga-version],
        [AS_HELP_STRING([--with-saga-version=VERSION], [Specifies the SAGA-GIS version number])],
        [SAGAVERS="$withval"], [SAGAVERS="2"])

if test -z "$SAGAPATH"
then
	SAGA_LDFLAGS=""
	SAGA_CPPFLAGS=""
	SAGA_FILE=""
	SAGA_ENABLED=""
else
	if test -z "$WXCFG" ; then
	   WXCFG="$(which wx-config)"
	fi
	if test "`$WXCFG --list | grep unicode`" == "" ; then
	   AC_MSG_ERROR(SAGA requires a unicode build of wxGTK)
	fi
	WX_ISSUE="-D_WX_WXCRTVARARG_H_"
	SAGA_DEFS="-D_SAGA_LINUX -D_TYPEDEF_BYTE -D_TYPEDEF_WORD -DMODULE_LIBRARY_PATH=\\\"$SAGAPATH/lib/saga\\\""
	SAGA_CPPFLAGS="-DSAGA_VERSION=${SAGAVERS} -fPIC -I$SAGAPATH/include/saga/saga_core/saga_api/ `$WXCFG --unicode=yes --static=no --cxxflags` -D_SAGA_UNICODE $SAGA_DEFS $WX_ISSUE"
	SAGA_LDFLAGS="-fPIC `$WXCFG --unicode=yes --static=no --libs` -lsaga_api"
	SAGA_ENABLED="-DUSE_SAGA"
	SAGA_FILE="service_internal_saga.o"
	
	AC_LANG_PUSH([C++])
	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$SAGA_CPPFLAGS"
	LIBS_SAVE="$LIBS"
	LIBS="$SAGA_LDFLAGS"
        if test "$SAGAVERS" == "2"; then
        AC_CHECK_HEADERS([module_library.h],
                        [], [AC_MSG_ERROR([could not find header file $i related to SAGA-GIS])])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "module_library.h"],[SG_Get_Module_Library_Manager();]])],
                [AC_MSG_RESULT([checking for SG_Get_Module_Library_Manager... yes])],[AC_MSG_ERROR([checking for SG_Get_Module_Library_Manager... failed])])
        else
        AC_CHECK_HEADERS([tool_library.h],
                        [], [AC_MSG_ERROR([could not find header file $i related to SAGA-GIS])])
        AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "tool_library.h"],[SG_Get_Tool_Library_Manager();]])],
                [AC_MSG_RESULT([checking for SG_Get_Tool_Library_Manager... yes])],[AC_MSG_ERROR([checking for SG_Get_Tool_Library_Manager... failed])])
        fi
	
	LIBS="$LIBS_SAVE"
	AC_LANG_POP([C++])
fi
AC_SUBST([SAGA_CPPFLAGS])
AC_SUBST([SAGA_LDFLAGS])
AC_SUBST([SAGA_FILE])
AC_SUBST([SAGA_ENABLED])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([ZOOMakefile.opts])
AC_CONFIG_FILES([main.cfg])
AC_OUTPUT
