ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-DZOO_SERVICE ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/ogrtindex_service.zo: service.c $(DEP_LIBS)
	g++ $(CFLAGS) $(CPPFLAGS) -shared -fpic $<  -o $@ ${GDAL_LIBS} -L${ZRPATH}/zoo-kernel/ -lzoo_service -lfcgi -L/usr/local/Cellar/gettext/0.19.8.1/lib/ -lintl

install:
	install -d ${CGI_DIR}/ogr/
	install cgi-env/* ${CGI_DIR}/ogr/

clean:
	rm -f cgi-env/*zo
