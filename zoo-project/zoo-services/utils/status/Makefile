ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG

cgi-env/wps_status.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/wps_status.zo ./service.c ../../../zoo-kernel/service_internal.o ${ZRPATH}/zoo-kernel/${MS_FILE} ${MS_LDFLAGS} ${JS_LDFLAGS} ${JSLDFLAGS} ${GDAL_LIBS} ${XML2LDFLAGS} ${MACOS_LD_FLAGS} ${ZOO_LDFLAGS} ${MACOS_LD_NET_FLAGS} `xslt-config --libs` -lfcgi

clean:
	rm -f cgi-env/wps_status.zo
