iotgwdir = $(prefix)/lib/Advantech/iotgw
ACLOCAL_AMFLAGS = -I m4
AM_CFLAGS = -I../include -I../include/internal -I../../../inc -I../../../Tools/AdvLog/include
AM_CXXFLAGS = -I../include -I../include/internal  -I../../../inc -I../../../Tools/AdvLog/include

iotgw_LTLIBRARIES = libMoteSim.la libTopology.la

libMoteSim_la_LIBADD = ../../../Tools/AdvLog/src/libAdvLog.la
libMoteSim_la_LDFLAGS = -version-info $(LIB_VERSION)
libMoteSim_la_SOURCES = SensorNetwork_API.cpp SimuParser.cpp SimuTools.cpp ../../../inc/SensorNetwork_API.h ../../../inc/SensorNetwork_BaseDef.h ../include/SimuParser.h ../include/SimuTools.h ../include/WSNType.h
libTopology_la_LIBADD = ../../../Tools/AdvLog/src/libAdvLog.la
libTopology_la_LDFLAGS = -version-info $(LIB_VERSION)
libTopology_la_SOURCES = TopologyTranslate.cpp ../include/TopologyTranslate.h

