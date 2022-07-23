include ../../zoo-kernel/ZOOMakefile.opts

OS:=$(shell uname -s)
ext=so
ifeq ($(OS),Darwin)
	ext=dylib
endif

JAVA_FILES=../../zoo-kernel/${JAVA_FILE}

ifeq ($(JS_ENABLED),-DUSE_JS)
	JS_FILES=../../zoo-kernel/${JS_FILE},
endif
ifeq ($(NODEJS_ENABLED),-DUSE_NODEJS)
	NODEJS_FILES=../../zoo-kernel/${NODEJS_FILE},
endif
ifeq ($(MS_FILE),service_internal_ms.o)
	MS_FILES=../../zoo-kernel/${MS_FILE},
endif

ifeq ($(YAML_FILE),service_yaml.o)
	YAML_FILES=../../zoo-kernel/${YAML_FILE},
endif



all: libZOO.${ext}

org/zoo_project/ZOO.class: org/zoo_project/ZOO.java
	javac org/zoo_project/ZOO.java

org_zoo_project_ZOO.h: org/zoo_project/ZOO.java org/zoo_project/ZOO.class
	javah org.zoo_project.ZOO

zoo-api.o: zoo-api.c org_zoo_project_ZOO.h
	gcc -fPIC -c zoo-api.c ${JAVACFLAGS} ${XML2CFLAGS} -I../../zoo-kernel/ -I../../../thirds/cgic206/

libZOO.${ext}: zoo-api.o
	gcc zoo-api.o -shared -o libZOO.${ext} -Wl,${YAML_FILES}../../zoo-kernel/lex.cr.o,../../zoo-kernel/main_conf_read.tab.o,../../zoo-kernel/lex.sr.o,../../zoo-kernel/service_conf.tab.o,../../zoo-kernel/service_internal.o,../../zoo-kernel/server_internal.o,../../zoo-kernel/response_print.o,${JAVA_FILES},${MS_FILES}${JS_FILES}${NODEJS_FILES}../../zoo-kernel/ulinet.o ${GDAL_LIBS} ${XML2LDFLAGS} ${PYTHONLDFLAGS} ${PERLLDFLAGS}  ${PHPLDFLAGS} ${JAVALDFLAGS} ${JSLDFLAGS} ${NODEJSLDFLAGS} -lcrypto ${MS_LDFLAGS} ${MACOS_LD_FLAGS} -lcurl -lfcgi ${MACOS_LD_NET_FLAGS} -lzoo_service ${YAML_LDFLAGS}

clean:
	rm -f *${ext} *o *.h *class org/zoo_project/*.class
