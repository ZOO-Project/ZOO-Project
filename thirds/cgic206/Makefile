OS:=$(shell uname -s)
ifeq ($(OS),Darwin)
	MACOS_CFLAGS=-arch x86_64
	LIBS= -L./ -lcgic /usr/lib/libfcgi.dylib
else
	LIBS= -L./ -lcgic /usr/lib/libfcgi.a
endif
CFLAGS=-g -Wall ${MACOS_CFLAGS}
CC=gcc
AR=ar
RANLIB=ranlib

all: libcgic.a cgictest.cgi capture

install: libcgic.a
	cp libcgic.a  ../../dist/lib
	cp cgic.h  ../../dist//include
	@echo libcgic.a is in  ../../dist/lib and cgic.h is in  ../../dist//include.

libcgic.a: cgic.o cgic.h
	rm -f libcgic.a
	$(AR) rc libcgic.a cgic.o
	$(RANLIB) libcgic.a

#mingw32 and cygwin users: replace .cgi with .exe

cgictest.cgi: cgictest.o libcgic.a
	gcc cgictest.o -o cgictest.cgi ${LIBS}

capture: capture.o libcgic.a
	gcc capture.o -o capture ${LIBS}

clean:
	rm -f *.o *.a cgictest.cgi capture

