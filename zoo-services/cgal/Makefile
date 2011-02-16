CFLAGS=-I../../zoo-kernel/ -I./ `xml2-config --cflags` `python-config --cflags` -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/cgal_service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/cgal_service.zo ./service.c -lgdal -lCGAL

clean:
	rm -f *.zo *.o *.tab.c *.tab.h *.sr.c* service_loader lex.* *.lreg *.sibling
	rm -rf service_loader.dSYM
