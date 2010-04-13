CFLAGS=-I../../../zoo-kernel-svn/ -I./ `xml2-config --cflags` `python-config --cflags` -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/service.zo: service.c
	g++  -DZOO_SERVICE ${CFLAGS} -shared -fpic -o cgi-env/gdal_grid_service.zo ./service.c -lgdal

clean:
	rm -f cgi-env/*.zo
