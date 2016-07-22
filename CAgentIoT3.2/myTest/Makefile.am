
libIoTSensor_la_SOURCES = ArrayLib.c BaseLib.c AdvJSONWrap.cpp AdvAPIMuxServer.c BasicFun_Tool.c BasicSocketFunc.c GW_list.c HashTable.c IoTGWFunction.c IoTGWHandler.c LoadSNManagerAPI.c mqtt_client_shared.c MqttHal.c SnGwParser.c SnMJSONParser.c common.c

libIoTSensor_la_LDFLAGS = -lpthread -ldl -release $(PACKAGE_VERSION_MAJOR).$(PACKAGE_VERSION_MINOR).$(PACKAGE_VERSION_PATCH).$(PACKAGE_VERSION_REVISION) -module

include_HEADERS =

AM_CPPFLAGS = -Wall -fPIC -I$(top_srcdir)/Platform/Linux/ -I$(top_srcdir)/../inc -I$(top_srcdir)/Include -I$(top_srcdir)/Library/cJSON -I$(top_srcdir)/Library/Log -I$(top_srcdir)/Library/SUSIHelper -I$(top_srcdir)/Library/SQFlashHelper
lib_LTLIBRARIES = libIoTSensor.la
# LDADD = $(top_srcdir)/Library/SAConfig/libSAConfig.la $(INTLLIBS)
