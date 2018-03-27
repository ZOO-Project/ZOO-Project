ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
ZCFLAGS=-I${ZRPATH}/zoo-kernel/ ${CFLAGS} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG

YAML_FILES=""
ifeq ("${YAML_FILE}","")
else
	YAML_FILES="${ZRPATH}/zoo-kernel/${YAML_FILE}"
endif

MS_FILES=""
ifeq ("${MS_FILE}","")
else
	MS_FILES="${ZRPATH}/zoo-kernel/${MS_FILE}"
endif

cgi-env/wps_registry.zo: service.c
	g++ ${ZCFLAGS} -shared -fpic -o cgi-env/wps_registry.zo ./service.c ${GDAL_LIBS} ${XML2LDFLAGS} ${MACOS_LD_FLAGS} ${ZOO_LDFLAGS} ${MACOS_LD_NET_FLAGS} ${XSLT_LDFLAGS} -lfcgi -L${ZRPATH}/zoo-kernel/ -lzoo_service ${ZRPATH}/zoo-kernel/server_internal.o ${ZRPATH}/zoo-kernel/lex.cr.o ${ZRPATH}/zoo-kernel/lex.sr.o ${ZRPATH}/zoo-kernel/service_conf.tab.o ${ZRPATH}/zoo-kernel/main_conf_read.tab.o ${ZRPATH}/zoo-kernel/response_print.o ${YAML_FILES} ${MS_FILES} ${MS_LDFLAGS} ${YAML_LDFLAGS}

install:
	install -d ${CGI_DIR}
	install cgi-env/* ${CGI_DIR}/
clean:
	rm -f cgi-env/wps_registry.zo
