CFLAGS=-I../../zoo-kernel/ -I./ `xml2-config --cflags` `python-config --cflags`  `gdal-config --cflags`   -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/ogr_service.zo ./service.c `gdal-config --libs`

clean:
	rm -f cgi-env/ogr_service.zo
