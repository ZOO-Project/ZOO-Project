ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-I${INST_INCLUDE} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/profile_service.zo: service.c
	g++  -DZOO_SERVICE ${CFLAGS} -shared -fpic -o cgi-env/profile_service.zo ./service.c ${GDAL_LIBS} ${MACOS_LD_FLAGS} -L${ZRPATH}/zoo-kernel/ -lzoo_service

install:
	install -d ${CGI_DIR}/gdal/profile
	install cgi-env/* ${CGI_DIR}/gdal/profile


clean:
	rm -f cgi-env/*.zo
