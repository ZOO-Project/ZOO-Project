ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-DZOO_SERVICE -I${INST_INCLUDE} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/dem_service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/dem_service.zo ./service.c ${GDAL_LIBS} ${MACOS_LD_FLAGS} -L${INST_LIB} -lzoo_service

install:
	install -d ${CGI_DIR}/gdal/dem
	install cgi-env/* ${CGI_DIR}/gdal/dem


clean:
	rm -f cgi-env/*.zo
