iotgwdir = $(prefix)/lib/Advantech/iotgw
iotgw_LTLIBRARIES = libDustWsnDrv.la libDustWsnDrv2.la

libDustWsnDrv_la_CPPFLAGS = \
                  -I$(top_builddir)/adv_coaplib \
                  -I$(top_builddir)/include \
                  -I$(top_srcdir)/adv_coaplib \
                  -I$(top_srcdir)/include \
		  -I../../../inc \
		  -I../../../Tools/AdvLog/include
if ENABLE_SERIALMUX
libDustWsnDrv_la_CPPFLAGS += \
                  -I$(top_builddir)/serialmuxclient \
                  -I$(top_srcdir)/serialmuxclient
else
libDustWsnDrv_la_CPPFLAGS += \
                  -I$(top_builddir)/sm_clib \
                  -I$(top_srcdir)/sm_clib
endif
if ENABLE_PARKING
libDustWsnDrv_la_CPPFLAGS += \
                  -I$(top_builddir)/alg_parking \
                  -I$(top_srcdir)/alg_parking
endif


libDustWsnDrv_la_CFLAGS = #-DDEBUG_PRINT
if ENABLE_SERIALMUX
libDustWsnDrv_la_CFLAGS += -DUSE_SERIALMUX
endif
if ENABLE_PARKING
libDustWsnDrv_la_CFLAGS += -DENABLE_PARKING
endif

libDustWsnDrv_la_LIBADD = ../adv_coaplib/libAdvCoapLib.la ../../../Tools/AdvLog/src/libAdvLog.la
if ENABLE_SERIALMUX
libDustWsnDrv_la_LIBADD += ../serialmuxclient/libserialmuxclient.la
else
libDustWsnDrv_la_LIBADD += ../sm_clib/libsm_clib.la
endif
if ENABLE_PARKING
libDustWsnDrv_la_LIBADD += ../alg_parking/libalgparking.la
endif

libDustWsnDrv_la_SOURCES = rs232.c \
                           IpMgWrapper.c \
                           DustWsnDrv.c \
                           MoteListAccess.c \
                           rs232.h \
                           IpMgWrapper.h \
                           DustWsnDrv.h \
                           MoteListAccess.h

libDustWsnDrv_la_LDFLAGS = -version-info $(LIB_VERSION) \
                           -shared


libDustWsnDrv2_la_CPPFLAGS = \
                  -I$(top_builddir)/adv_coaplib \
                  -I$(top_builddir)/include \
                  -I$(top_srcdir)/adv_coaplib \
                  -I$(top_srcdir)/include \
		  -I../../../inc \
		  -I../../../Tools/AdvLog/include
if ENABLE_SERIALMUX
libDustWsnDrv2_la_CPPFLAGS += \
                  -I$(top_builddir)/serialmuxclient \
                  -I$(top_srcdir)/serialmuxclient
else
libDustWsnDrv2_la_CPPFLAGS += \
                  -I$(top_builddir)/sm_clib \
                  -I$(top_srcdir)/sm_clib
endif
if ENABLE_PARKING
libDustWsnDrv2_la_CPPFLAGS += \
                  -I$(top_builddir)/alg_parking \
                  -I$(top_srcdir)/alg_parking
endif


libDustWsnDrv2_la_CFLAGS = #-DDEBUG_PRINT
if ENABLE_SERIALMUX
libDustWsnDrv2_la_CFLAGS += -DUSE_SERIALMUX
endif

libDustWsnDrv2_la_LIBADD = ../adv_coaplib/libAdvCoapLib.la ../../../Tools/AdvLog/src/libAdvLog.la
if ENABLE_SERIALMUX
libDustWsnDrv2_la_LIBADD += ../serialmuxclient/libserialmuxclient.la
else
libDustWsnDrv2_la_LIBADD += ../sm_clib/libsm_clib.la
endif

libDustWsnDrv2_la_CFLAGS += -DUSE_WSN_DEV2

libDustWsnDrv2_la_SOURCES = rs232.c \
                           IpMgWrapper.c \
                           DustWsnDrv.c \
                           MoteListAccess.c \
                           rs232.h \
                           IpMgWrapper.h \
                           DustWsnDrv.h \
                           MoteListAccess.h

libDustWsnDrv2_la_LDFLAGS = -version-info $(LIB_VERSION) \
                           -shared


