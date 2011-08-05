GDAL_SRC=./gdal_src/

include $(GDAL_SRC)GDALmake.opt

CPPFLAGS        :=      $(GDAL_INCLUDE) -I$(GDAL_ROOT)/frmts $(CPPFLAGS)
DEP_LIBS        =       $(EXE_DEP_LIBS) $(XTRAOBJ)
BIN_LIST = cgi-env/ogr2ogr_service.zo

default:        $(BIN_LIST)

all: default

lib-depend:
	(cd $(GDAL_SRC)/gcore ; $(MAKE) )
	(cd $(GDAL_SRC)/port ; $(MAKE) )

CFLAGS=-DZOO_SERVICE -I../../../zoo-kernel/

#cgi-env/ogr2ogr_service.zo: service.c
#	g++  -DZOO_SERVICE ${CFLAGS} -shared -fpic -o cgi-env/ogr2ogr_service.zo ./service.c -lgdal

cgi-env/ogr2ogr_service.zo: service.c $(DEP_LIBS)
	g++ $(CFLAGS) $(CPPFLAGS) -shared -fpic $< `gdal-config --libs` -o $@

clean:
	rm -f cgi-env/*zo
