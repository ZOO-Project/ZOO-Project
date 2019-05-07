ZRPATH=../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS= ${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} ${CGAL_CFLAGS} -frounding-math -fPIC -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/cgal_service.zo: delaunay.c voronoi.c cgal_service.o
	g++ ${CFLAGS} -c ./delaunay.c
	g++ ${CFLAGS} -c ./voronoi.c
	g++ ${CFLAGS} -shared -o cgi-env/cgal_service.zo ./delaunay.o ./voronoi.o ./cgal_service.o ${GDAL_LIBS} ${CGAL_LDFLAGS} -lCGAL -lgmp ${GDAL_LIBS} -L${ZRPATH}/zoo-kernel/ -lzoo_service -lfcgi

cgal_service.o: cgal_service.c cgal_service.h
	g++ ${CFLAGS} -c ./cgal_service.c

install:
	install -d ${CGI_DIR}/cgal
	install cgi-env/* ${CGI_DIR}/cgal/

clean:
	rm -f cgi-env/*.zo *.o
