CFLAGS=-I../../../zoo-kernel/ -I./ `geos-config --cflags` `xml2-config --cflags`  `gdal-config --cflags`  -DLINUX_FREE_ISSUE #-DDEBUG
# if JS_ENABLED flag is set to true in your ZOO-Kernel Makefile then you'll have
# uncomment the following line
# JS_LDFLAGS=../../../zoo-kernel/ulinet.o ../../../zoo-kernel/service_internal_js.o

cgi-env/ogr_service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/ogr_service.zo ./service.c ../../../zoo-kernel/service_internal.o ${JS_LDFLAGS} `xml2-config --libs` `gdal-config --libs` `geos-config --libs`

clean:
	rm -f cgi-env/ogr_service.zo
