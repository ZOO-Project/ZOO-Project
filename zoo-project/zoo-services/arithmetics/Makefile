CFLAGS=-I../../zoo-kernel/ -I./ `xml2-config --cflags` `python-config --cflags`  `gdal-config --cflags`   -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

cgi-env/test_service.zo: test_service.c
	g++ ${CFLAGS} -shared -fpic -o cgi-env/test_service.zo ./test_service.c -lzoo_service

clean:
	rm -f cgi-env/test_service.zo
