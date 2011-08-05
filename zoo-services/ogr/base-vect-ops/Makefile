ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=${ZOO_CFLAGS} ${JSCFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} `geos-config --cflags` -DLINUX_FREE_ISSUE #-DDEBUG

cgi-env/ogr_service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/ogr_service.zo ./service.c ../../../zoo-kernel/service_internal.o ${JS_LDFLAGS} ${JSLDFLAGS} ${GDAL_LIBS} ${XML2LDFLAGS} ${MACOS_LD_FLAGS} ${ZOO_LDFLAGS} ${MACOS_LD_NET_FLAGS} `geos-config --libs`

clean:
	rm -f cgi-env/ogr_service.zo
