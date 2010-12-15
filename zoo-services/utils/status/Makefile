CFLAGS=-I../../../zoo-kernel/ -I./ `xslt-config --cflags` `xml2-config --cflags` -DLINUX_FREE_ISSUE #-DDEBUG
# if JS_ENABLED flag is set to true in your ZOO-Kernel Makefile then you'll have
# uncomment the following line
# JS_LDFLAGS=../../../zoo-kernel/ulinet.o ../../../zoo-kernel/service_internal_js.o

cgi-env/wps_status.zo: service.c
	g++ ${CFLAGS} -o cgi-env/wps_status.zo ./service.c ../../../zoo-kernel/service_internal.o ${JS_LDFLAGS} `xml2-config --libs` `xslt-config --libs`

clean:
	rm -f cgi-env/wps_status.zo
