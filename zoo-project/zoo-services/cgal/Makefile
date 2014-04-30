ZRPATH=../..
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/cgal_service.zo: delaunay.c voronoi.c cgal_service.o
	g++ ${CFLAGS} -c ./delaunay.c
	g++ ${CFLAGS} -c ./voronoi.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/cgal_service.zo ./delaunay.o ./voronoi.o ./cgal_service.o ${GDAL_LIBS} ${MACOS_LD_FLAGS} -lCGAL -lgmp

cgal_service.o: cgal_service.c cgal_service.h
	g++ ${CFLAGS} -c ./cgal_service.c


clean:
	rm -f cgi-env/*.zo *.o
