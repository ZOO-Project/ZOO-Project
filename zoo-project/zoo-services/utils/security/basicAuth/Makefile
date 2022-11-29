ZRPATH=../../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-I${INST_INCLUDE} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/security_service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/security_service.zo ./service.c ${ZRPATH}/zoo-kernel/server_internal.o -L${ZRPATH}/zoo-kernel/ -lzoo_service -laprutil-1

install:
	install -d ${CGI_DIR}/
	install cgi-env/* ${CGI_DIR}/



clean:
	rm -f cgi-env/*.zo
