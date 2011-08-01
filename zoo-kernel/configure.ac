AC_INIT([ZOO Kernel], [1.2.0], [bugs@zoo-project.org])

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
	[AS_HELP_STRING([--with-python=PATH], [To enable python support or specify an alternative directory for python installation,  disabled by default])], 
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
		# PHP was not specified, so search within the current path
		AC_PATH_PROG([PYTHONCONFIG], [python${PYTHON_VERS}-config])
	else
		PYTHONCONFIG="$PYTHON_PATH/bin/python${PYTHON_VERS}-config"
	fi

	# Extract the linker and include flags 
	PYTHON_LDFLAGS=`$PYTHONCONFIG --ldflags`
	PYTHON_CPPFLAGS=`$PYTHONCONFIG --cflags`

	# Check headers file
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$PYTHON_CPPFLAGS"
	AC_CHECK_HEADERS([Python.h],
		 [], [AC_MSG_ERROR([could not find headers include related to libpython])])

	# Ensure we can link against libphp
	LIBS_SAVE="$LIBS"
	LIBS="$PYTHON_LDFLAGS"
	PY_LIB=`$PYTHONCONFIG --libs | sed -e 's/^.*\(python2\..\)$/\1/'`
	AC_CHECK_LIB([$PY_LIB], [PyObject_CallObject], [], [AC_MSG_ERROR([could not find libpython])], [])
	AC_SUBST([PYTHON_CPPFLAGS])
	AC_SUBST([PYTHON_LDFLAGS])
fi

AC_SUBST([PYTHON_ENABLED])
AC_SUBST([PYTHON_FILE])

# ===========================================================================
# Detect if php is installed
# ===========================================================================

AC_ARG_WITH([php], 
	[AS_HELP_STRING([--with-php=PATH], [To enable php support or specify an alternative directory for php installation,  disabled by default])], 
	[PHP_PATH="$withval"; PHP_ENABLED="-DUSE_PHP"], [PHP_ENABLED=""])


if test -z "$PHP_ENABLED"
then
	PHP_FILE=""
else
	PHPCONFIG="$PHP_PATH/bin/php-config"
	PHP_FILE="service_internal_php.o"
	if test  "$PHP_PATH" = "yes"
	then
		# PHP was not specified, so search within the current path
		AC_PATH_PROG([PHPCONFIG], [php-config])
	else
		PHPCONFIG="$PHP_PATH/bin/php-config"
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
fi

AC_SUBST([PHP_ENABLED])
AC_SUBST([PHP_FILE])

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
# Detect if java is installed
# ===========================================================================

AC_ARG_WITH([java], 
	[AS_HELP_STRING([--with-java=PATH], [To enable java support, specify a JDK_HOME,  disabled by default])], 
	[JDKHOME="$withval"; JAVA_ENABLED="-DUSE_JAVA"], [JAVA_ENABLED=""])

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
		JAVA_CPPFLAGS="-I/Developer//SDKs/MacOSX10.6.sdk/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers/"
	else
		if test -d "$JDKHOME/jre/lib/i386";
		then
			JAVA_LDFLAGS="-L$JDKHOME/jre/lib/i386/client/ -ljvm -lpthread"
			JAVA_CPPFLAGS="-I$JDKHOME/include -I$JDKHOME/include/linux"
		else
			JAVA_LDFLAGS="-L$JDKHOME/jre/lib/amd64/client/ -ljvm -lpthread"
			JAVA_CPPFLAGS="-I$JDKHOME/include -I$JDKHOME/include/linux"
		fi
	fi

	# Check headers file (second time we check that in fact)
	CPPFLAGS_SAVE="$CPPFLAGS"
	CPPFLAGS="$JAVA_CPPFLAGS"
	AC_CHECK_HEADERS([jni.h],
			 [], [AC_MSG_ERROR([could not find headers include related to libjava])])

	# Ensure we can link against libjava
	LIBS_SAVE="$LIBS"
	LIBS="$JAVA_LDFLAGS"
	if test "x$JDKHOME" != "xmacos";
	then
		AC_CHECK_LIB([jvm], [JNI_CreateJavaVM], [], [AC_MSG_ERROR([could not find libjava])], [])
	fi

	AC_SUBST([JAVA_CPPFLAGS])
	AC_SUBST([JAVA_LDFLAGS])
fi 

AC_SUBST([JAVA_ENABLED])
AC_SUBST([JAVA_FILE])

# ===========================================================================
# Detect if spidermonkey is installed
# ===========================================================================

AC_ARG_WITH([js], 
	[AS_HELP_STRING([--with-js=PATH], [specify --with-js=path-to-js to enable js support, specify --with-js on linux debian like, js support is disabled by default ])], 
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
				XUL_VERSION="`dpkg -l | grep xulrunner | grep dev | awk '{print $3;}' | cut -d'+' -f1`"
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

	#AC_CHECK_HEADERS([jsapi.h],
        #                [], [AC_MSG_ERROR([could not find headers include related to libjs])])

	
	LIBS_SAVE="$LIBS"
        LIBS="$JS_LDFLAGS"

        AC_CHECK_LIB([$JS_LIB], [JS_NewCompartmentAndGlobalObject,JS_CompileFile,JS_CallFunctionName], [], [AC_MSG_ERROR([could not find $JS_LIB])], [])
			
        AC_SUBST([JS_CPPFLAGS])
        AC_SUBST([JS_LDFLAGS])
fi

AC_SUBST([JS_ENABLED])
AC_SUBST([JS_FILE])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([ZOOMakefile.opts])
AC_OUTPUT
