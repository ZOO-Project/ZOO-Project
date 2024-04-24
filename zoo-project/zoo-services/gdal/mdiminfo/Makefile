ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-I${INST_INCLUDE} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/mdiminfo_service.zo: service.cpp
	g++  -I.. -DZOO_SERVICE ${CFLAGS} -shared -fpic -o cgi-env/mdiminfo_service.zo ./service.cpp ${GDAL_LIBS} ${MACOS_LD_FLAGS} -L${ZRPATH}/zoo-kernel/ -lzoo_service

install:
	install -d ${CGI_DIR}/gdal/mdiminfo
	install cgi-env/* ${CGI_DIR}/gdal/mdiminfo


clean:
	rm -f cgi-env/*.zo
