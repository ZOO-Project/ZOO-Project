ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=-I${INST_INCLUDE} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG

cgi-env/wps_hpc.zo: service.c service2.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/wps_hpc.zo service.c service2.c ${GDAL_LIBS} ${XML2LDFLAGS} ${MACOS_LD_FLAGS} ${ZOO_LDFLAGS} ${MACOS_LD_NET_FLAGS} ${XSLT_LDFLAGS} ${FCGI_LDFLAGS} -L${ZRPATH}/zoo-kernel/ -lzoo_service ${ZRPATH}/zoo-kernel/sshapi.o ${HPCLDFLAGS}

install:
	install -d ${CGI_DIR}
	install cgi-env/*.zcfg ${CGI_DIR}/
	install cgi-env/*.zo ${CGI_DIR}/

clean:
	rm -f cgi-env/wps_hpc.zo
