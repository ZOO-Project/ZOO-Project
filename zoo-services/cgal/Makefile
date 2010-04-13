CFLAGS=-I../../zoo-kernel-svn/ -I./ `xml2-config --cflags` `python-config --cflags` -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/service.zo: service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/service.zo ./service.c -lgdal

clean:
	rm -f *.zo *.o *.tab.c *.tab.h *.sr.c* service_loader lex.* *.lreg *.sibling
	rm -rf service_loader.dSYM
