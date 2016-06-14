ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=${ZOO_CFLAGS} ${JSCFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${GEOS_CFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG

ifneq ($(MS_FILE),)
	MS_FILES=${ZRPATH}/zoo-kernel/${MS_FILE} -lmapserver
else
	MS_FILES=
endif

cgi-env/ogr_service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/ogr_service.zo ./service.c ${GDAL_LIBS} ${XML2LDFLAGS} ${MACOS_LD_FLAGS} ${ZOO_LDFLAGS} ${MACOS_LD_NET_FLAGS} ${GEOS_LDFLAGS} -lfcgi  -lpthread -L${ZRPATH}/zoo-kernel/ -lzoo_service

install:
	install -d ${CGI_DIR}/ogr/base-vect-ops
	install cgi-env/* ${CGI_DIR}/ogr/base-vect-ops

clean:
	rm -f cgi-env/ogr_service.zo
