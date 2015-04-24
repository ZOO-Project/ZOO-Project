ZRPATH=../../zoo-project
include ${ZRPATH}/zoo-kernel/ZOOMakefile.opts
CFLAGS=${ZOO_CFLAGS} ${XML2CFLAGS} ${GDAL_CFLAGS} ${PYTHONCFLAGS} -DLINUX_FREE_ISSUE #-DDEBUG
CC=gcc

WXCXX=`wx-config --cxx`
WXCFLAGS=`wx-config --cxxflags`
WXLIBS=`wx-config --libs`

saga2zcfg: saga2zcfg.c
	${WXCXX} ${SAGA_CFLAGS} ${WXCFLAGS} ${CFLAGS} -fpic -o saga2zcfg ./saga2zcfg.c ${GDAL_LIBS} ${MACOS_LD_FLAGS}  ${WXLIBS} ${SAGA_LDFLAGS}

clean:
	rm -f saga2zcfg
