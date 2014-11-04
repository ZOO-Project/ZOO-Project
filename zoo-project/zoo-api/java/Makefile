include ../../zoo-kernel/ZOOMakefile.opts

OS:=$(shell uname -s)
ext=so
ifeq ($(OS),Darwin)
	ext=dylib
endif

JAVA_FILES=../../zoo-kernel/${JAVA_FILE}

ifeq ($(JS_ENABLED),-DUSE_JS)
	JS_FILES=../../zoo-kernel/${JS_FILE}
endif
ifeq ($(MS_FILE),service_internal_ms.o)
	MS_FILES=../../zoo-kernel/${MS_FILE}
endif

all: libZOO.${ext}

ZOO.class: ZOO.java
	javac ZOO.java

ZOO.h: ZOO.java ZOO.class
	javah ZOO

zoo-api.o: zoo-api.c ZOO.h
	gcc -fPIC -c zoo-api.c ${JAVACFLAGS} ${XML2CFLAGS} -I../../zoo-kernel/ -I../../../thirds/cgic206/

libZOO.${ext}: zoo-api.o
	gcc zoo-api.o -shared -o libZOO.${ext} -Wl ../../zoo-kernel/service_internal.o ${JAVA_FILES} ${MS_FILES} ${JS_FILES} ../../zoo-kernel/ulinet.o ${GDAL_LIBS} ${XML2LDFLAGS} ${PYTHONLDFLAGS} ${PERLLDFLAGS}  ${PHPLDFLAGS} ${JAVALDFLAGS} ${JSLDFLAGS} -lcrypto ${MS_LDFLAGS} ${MACOS_LD_FLAGS} -lcurl -lfcgi ${MACOS_LD_NET_FLAGS}

clean:
	rm -f *${ext} *o *.h *class
