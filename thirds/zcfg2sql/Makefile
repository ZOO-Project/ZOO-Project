ZRPATH=../../zoo-project
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
LCFLAGS=${CFLAGS} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

ifeq (${CALLBACK_FILE},service_callback.o)
CALLBACK_FILES=${ZRPATH}/zoo-kernel/${CALLBACK_FILE} ${ZRPATH}/zoo-kernel/ulinet.o ${ZRPATH}/zoo-kernel/caching.o
else
CALLBACK_FILES=
endif

ifeq (${YAML_FILE},service_yaml.o)
YAML_FILES=${ZRPATH}/zoo-kernel/${YAML_FILE}
else
YAML_FILES=
endif

ifeq (${JSON_FILE},service_json.o service_callback.o)
JSON_FILES=${ZRPATH}/zoo-kernel/service_json.o ${ZRPATH}/zoo-kernel/service_callback.o ${ZRPATH}/zoo-kernel/ulinet.o
else
JSON_FILES=
endif

ifeq (${MS_FILE},service_internal_ms.o)
MS_FILES=${ZRPATH}/zoo-kernel/${MS_FILE} ${ZRPATH}/zoo-kernel/caching.o
else
MS_FILES=
endif

zcfg2sql: zcfg2sql.c
	g++ -g -O2 ${JSCFLAGS} ${RUBYCFLAGS} ${XML2CFLAGS} ${LCFLAGS} -c zcfg2sql.c  -fno-common -DPIC -o zcfg2sql.o
	g++  ${XML2CFLAGS} ${LCFLAGS} zcfg2sql.o ${CALLBACK_FILES} ${ZRPATH}/zoo-kernel/server_internal.o ${ZRPATH}/zoo-kernel/service_internal.o ${MS_FILES} ${ZRPATH}/zoo-kernel/response_print.o ${ZRPATH}/zoo-kernel/lex.cr.o ${ZRPATH}/zoo-kernel/lex.sr.o ${ZRPATH}/zoo-kernel/service_conf.tab.o ${ZRPATH}/zoo-kernel/main_conf_read.tab.o ${YAML_FILES} ${JSON_FILES} -o zcfg2sql -L${ZRPATH}/zoo-kernel/ ${LDFLAGS}

clean:
	rm -f zcfg2sql
