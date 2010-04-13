AC_PREREQ([2.63])
AC_INIT([ZOO Kernel], [0.0.1], [bugs@zoo-project.org])
#AM_INIT_AUTOMAKE
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX
AC_PROG_CXX
AC_PROG_SED

# Checks for libraries.
AC_CHECK_LIB([cgic], [cgiMain])
AC_CHECK_LIB([curl], [curl_easy_init curl_easy_setopt curl_easy_cleanup curl_easy_perform])
AC_CHECK_LIB([dl], [dlopen dlsym dlerror dlclose])
AC_CHECK_LIB([fl], [main])
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([fcgi], [main])
AC_CHECK_LIB([ssl], [main])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h inttypes.h libintl.h malloc.h stddef.h stdlib.h string.h unistd.h])

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
# Detect if gdal is installed
#============================================================================

AC_ARG_WITH([gdal-config], 
	[AS_HELP_STRING([--with-gdal-config=FILE], [specify an alternative gdal-config file])], 
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
# Detect if libxml2 is installed
# ===========================================================================

AC_ARG_WITH([xml2config], 
	[AS_HELP_STRING([--with-xml2config=FILE], [specify an alternative xml2-config file])], 
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
		AC_MSG_ERROR([you must specify a parameter to --with-xml2config, e.g. --with-xml2config=/path/to/xml2-config])
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




# ===========================================================================
# Detect if python is installed
# ===========================================================================

AC_ARG_WITH([python], 
	[AS_HELP_STRING([--with-python=PATH], [specify an alternative directory for python installation])], 
	[PYTHONCONFIG="$withval/bin/python-config"], [PYTHONCONFIG=""])

if test "x$PYTHONCONFIG" = "x"; then
	# PYTHON was not specified, so search within the current path
	AC_PATH_PROG([PYTHONCONFIG], [python-config])

	# If we couldn't find python-config, display a warning
	if test "x$PYTHONCONFIG" = "x"; then
		AC_MSG_ERROR([could not find python-config from libpython within the current path. You may need to try re-running configure with a --with-python parameter.])
	fi
else
	# PYTHON was specified; display a message to the user
	if test "x$PYTHON" = "xyes"; then
		AC_MSG_ERROR([you must specify a parameter to --with-python, e.g. --with-python=/path/to/python])
	else
		if test -f $PYTHONCONFIG; then
			AC_MSG_RESULT([Using user-specified python-config file: $PYTHONCONFIG])
		else
			AC_MSG_ERROR([the user-specified python-config file $PYTHONCONFIG does not exist])
		fi	
	fi
fi


# Extract the linker and include flags 
PYTHON_LDFLAGS=`$PYTHONCONFIG --libs`
PYTHON_CPPFLAGS=`$PYTHONCONFIG --cflags`

# Check headers file
CPPFLAGS_SAVE="$CPPFLAGS"
CPPFLAGS="$PYTHON_CPPFLAGS"
AC_CHECK_HEADERS([Python.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libpython])])

# Ensure we can link against libpython
LIBS_SAVE="$LIBS"
LIBS="$PYTHON_LDFLAGS"
# Shouldn't we get python here rather than python2.6 :) ??
PY_LIB=`$PYTHONCONFIG --libs | sed -e 's/^.*\(python2\..\)$/\1/'`
echo $PY_LIB
AC_CHECK_LIB([$PY_LIB], [PyObject_CallObject], [], [AC_MSG_ERROR([could not find libpython])], [])

AC_SUBST([PYTHON_CPPFLAGS])
AC_SUBST([PYTHON_LDFLAGS])

# ===========================================================================
# Detect if php is installed
# ===========================================================================




AC_ARG_WITH([php], 
	[AS_HELP_STRING([--with-php=PATH], [specify an alternative directory for php installation or --with-php=no  to disable php support])], 
	[PHP_PATH="$withval"], [PHP_PATH=""])
if test "$PHP_PATH" != "no";
then 
	PHPCONFIG="$PHP_PATH/bin/php-config"
	PHP_ENABLED="-DUSE_PHP"
	PHP_FILE="service_internal_php.o"
	if test "x$PHPCONFIG" = "x"; 
	then
	# PHP was not specified, so search within the current path
	AC_PATH_PROG([PHPCONFIG], [php-config])

	# If we couldn't find php-config, display a warning
	if test "x$PHPCONFIG" = "x"; 
	then
		AC_MSG_ERROR([could not find php-config from libphp within the current path. You may need to try re-running configure with a --with-php parameter.])
	fi
	else
		# PHP was specified; display a message to the user
		if test "x$PHP" = "xyes"; 
		then
			AC_MSG_ERROR([you must specify a parameter to --with-php, e.g. --with-php=/path/to/php])
		else
			if test -f $PHPCONFIG; 
			then
				AC_MSG_RESULT([Using user-specified php-config file: $PHPCONFIG])
			else
				AC_MSG_ERROR([the user-specified php-config file $PHPCONFIG does not exist])
			fi	
		fi
	fi
	# Extract the linker and include flags 
	PHP_LDFLAGS="-L/`$PHPCONFIG --prefix`/lib -lphp5"
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
	AC_CHECK_LIB([php5], [call_user_function], [], [AC_MSG_ERROR([could not find libphp])], [])
	AC_SUBST([PHP_CPPFLAGS])
	AC_SUBST([PHP_LDFLAGS])
else
	PHP_ENABLED=""
	PHP_FILE=""
fi

AC_SUBST([PHP_ENABLED])
AC_SUBST([PHP_FILE])


# ===========================================================================
# Detect if java is installed
# ===========================================================================



AC_ARG_WITH([java], 
	[AS_HELP_STRING([--with-java=PATH], [specify an alternative directory for java installation or --with-java=no to disabled java support])], 
	[JDKHOME="$withval"], [JDKHOME=""])


if test "$JDKHOME" != "no";
then
	JAVA_ENABLED="-DUSE_JAVA"
	JAVA_FILE="service_internal_java.o"
	if test "x$JDKHOME" = "x"; 
	then
		# JAVA was not specified, so search within the current path
		#AC_PATH_PROG([JAVACONFIG], [java])

		# If we couldn't find java-config, display a warning
		if test "x$JDKHOME" = "x"; 
		then
			AC_MSG_ERROR([could not find java installation path within the current path. You may need to try re-running configure with a --with-java parameter.])
		fi
	else
		# JAVA was specified; display a message to the user
		if test "x$JDKHOME" = "xyes"; 
		then
			AC_MSG_ERROR([you must specify a parameter to --with-java, e.g. --with-java=/path/to/java])
		else
			if test -f $JDKHOME/include/jni.h; 
			then
				AC_MSG_RESULT([Using specific java install dir : $JDKHOME])
			else
				AC_MSG_ERROR([the specific java install dir $DKHOME does not exist])
			fi	
		fi
	fi
	# Extract the linker and include flags
	if test -d "$JDKHOME/jre/lib/i386";
	then
		JAVA_LDFLAGS="-L$JDKHOME/jre/lib/i386/ -ljavaplugin_jni -L$JDKHOME/jre/lib/i386/server/ -ljvm -lpthread"
		JAVA_CPPFLAGS="-I$JDKHOME/include -I$JDKHOME/include/linux"
	else
		JAVA_LDFLAGS="-L$JDKHOME/jre/lib/amd64/ -ljavaplugin_jni -L$JDKHOME/jre/lib/amd64/server/ -ljvm -lpthread"
                JAVA_CPPFLAGS="-I$JDKHOME/include -I$JDKHOME/include/linux"
	fi

	# Check headers file (second time we check that in fact)
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$JAVA_CPPFLAGS"
	AC_CHECK_HEADERS([jni.h],
			 [], [AC_MSG_ERROR([could not find headers include related to libjava])])

	# Ensure we can link against libjava
	LIBS_SAVE="$LIBS"
	LIBS="$JAVA_LDFLAGS"
	# Shouldn't we get java here rather than java5 :) ??
	AC_CHECK_LIB([javaplugin_jni], [JNI_CreateJavaVM], [], [AC_MSG_ERROR([could not find libjava])], [])
	AC_CHECK_LIB([jvm], [main], [], [AC_MSG_ERROR([could not find libjava])], [])

	AC_SUBST([JAVA_CPPFLAGS])
	AC_SUBST([JAVA_LDFLAGS])
else
	JAVA_ENABLED=""
	JAVA_FILE=""
fi 

AC_SUBST([JAVA_ENABLED])
AC_SUBST([JAVA_FILE])





# ===========================================================================
# Detect if spidermonkey is installed
# ===========================================================================

AC_ARG_WITH([js], 
	[AS_HELP_STRING([--with-js=PATH], [specify an alternative directory for spidermonkey installation or --with-js=no to disabled js support])], 
	[JSHOME="$withval"], [JSHOME=""])
if test "$JSHOME" != "no";
then
        JS_ENABLED="-DUSE_JS"
        JS_FILE="service_internal_js.o"

	#if test "x$JSHOME" = "x"; then
	#	# JS was not specified, so search within the current path
#		# AC_PATH_PROG([JSCONFIG], [js-config])
#	
#		# If we couldn't find js-config, display a warning
#		if test "x$JSHOME" = "x"; then
#			AC_MSG_ERROR([could not find js-config from libpython within the current path. You may need to try re-running configure with a --with-js parameter.])
#		fi
#	else
#		# JS was specified; display a message to the user
#		if test "x$JSHOME" = "xyes"; then
#			AC_MSG_ERROR([you must specify a parameter to --with-js, e.g. --with-js=/path/to/js])
#		else
#			if test -f $JSHOME; then
#			AC_MSG_RESULT([Using user-specified js-config file: $JSCONFIG])
#			else
#				AC_MSG_ERROR([the user-specified js-config file $JSCONFIG does not exist])
#			fi	
#		fi
#	fi


	# Extract the linker and include flags 
	JS_CPPFLAGS="-I$JSHOME/include"
	JS_LDFLAGS="-L$JSHOME/lib -ljs -lm"
	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$JS_CPPFLAGS"
	#AC_CHECK_HEADERS([jsapi.h],
	#		 [], [AC_MSG_ERROR([could not find headers include related to libjs])])

	# Ensure we can link against libjs
	LIBS_SAVE="$LIBS"
	LIBS="$JS_LDFLAGS"
	
	AC_CHECK_LIB([js], [JS_CallFunctionName], [], [AC_MSG_ERROR([could not find libjs])], [])

	AC_SUBST([JS_CPPFLAGS])
	AC_SUBST([JS_LDFLAGS])

else
        JS_ENABLED=""
        JS_FILE=""
fi

AC_SUBST([JS_ENABLED])
AC_SUBST([JS_FILE])






AC_CONFIG_FILES([Makefile])
AC_OUTPUT
