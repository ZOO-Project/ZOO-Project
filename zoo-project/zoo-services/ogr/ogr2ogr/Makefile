ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-DZOO_SERVICE ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/ogr2ogr_service.zo: service.c $(DEP_LIBS)
	g++ $(CFLAGS) $(CPPFLAGS) -shared -fpic $<  -o $@ ${GDAL_LIBS} -lzoo_service

install:
	install -d ${CGI_DIR}/ogr/ogr2ogr
	install cgi-env/* ${CGI_DIR}/ogr/ogr2ogr

clean:
	rm -f cgi-env/*zo
