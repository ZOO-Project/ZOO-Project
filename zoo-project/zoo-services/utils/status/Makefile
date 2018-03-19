ZRPATH=../../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS1=${CFLAGS} -I${INST_INCLUDE} ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG

cgi-env/wps_status.zo: service.c
	g++ ${CFLAGS1} -shared -fpic -o cgi-env/wps_status.zo ./service.c ${GDAL_LIBS} ${XML2LDFLAGS} ${MACOS_LD_FLAGS} ${ZOO_LDFLAGS} ${MACOS_LD_NET_FLAGS} ${XSLT_LDFLAGS} ${FCGI_LDFLAGS} -L${ZRPATH}/zoo-kernel/ -lzoo_service

install:
	install -d ${CGI_DIR}
	install cgi-env/*.zcfg ${CGI_DIR}/
	install cgi-env/*.zo ${CGI_DIR}/
	install cgi-env/updateStatus.xsl ${SHAREDSTATEDIR}/

clean:
	rm -f cgi-env/wps_status.zo
