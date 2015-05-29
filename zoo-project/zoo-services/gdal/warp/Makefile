ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/warp_service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/warp_service.zo ./service.c ${GDAL_LIBS} ${MACOS_LD_FLAGS} -lzoo_service

install:
	install -d ${CGI_DIR}/gdal/warp
	install cgi-env/* ${CGI_DIR}/gdal/warp


clean:
	rm -f cgi-env/warp_service.*
