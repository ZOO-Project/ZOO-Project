ZRPATH=../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/qrencode.zo: qrenc-service.c
	g++ -DZOO_SERVICE_PROVIDER ${CFLAGS} -shared -fpic -o cgi-env/qrencode.zo ./qrenc-service.c -lqrencode ${GDAL_LIBS} -lpng ${MACOS_LD_FLAGS}

clean:
	rm -f cgi-env/*.zo
