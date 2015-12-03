ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-I${INST_INCLUDE} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG

cgi-env/wps_status.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/wps_status.zo ./service.c ${GDAL_LIBS} ${XML2LDFLAGS} ${MACOS_LD_FLAGS} ${ZOO_LDFLAGS} ${MACOS_LD_NET_FLAGS} ${XSLT_LDFLAGS} ${FCGI_LDFLAGS} -L${INST_LIB} -lzoo_service

install:
	install -d ${CGI_DIR}
	install cgi-env/* ${CGI_DIR}/
clean:
	rm -f cgi-env/wps_status.zo
