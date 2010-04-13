GDAL_SRC=/var/tmp/portage/sci-libs/gdal-1.6.1/work/gdal-1.6.1/ogr/
CFLAGS=-I$(GDAL_SRC) -I../../../zoo-kernel-svn/ -I./ `xml2-config --cflags` `python-config --cflags` -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/ogr2ogr_service.zo: service.c
	g++  -DZOO_SERVICE ${CFLAGS} -shared -fpic -o cgi-env/ogr2ogr_service.zo ./service.c -lgdal

clean:
	rm -f cgi-env/*.zo
