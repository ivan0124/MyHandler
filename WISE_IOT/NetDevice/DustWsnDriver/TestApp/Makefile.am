bin_PROGRAMS = testDustWsnDrv

testDustWsnDrv_CPPFLAGS = \
                          -I$(top_builddir)/include \
                          -I$(top_builddir)/platform/linux \
                          -I$(top_srcdir)/include \
                          -I$(top_srcdir)/platform/linux \
                          -I../../../inc \
						  -I../../../Tools/AdvLog/include

testDustWsnDrv_LDADD = ../ipmg/libDustWsnDrv.la ../../../Tools/AdvLog/src/libAdvLog.la

testDustWsnDrv_SOURCES = DustWsnDrv_test.c \
                         DustWsnDrv_test.h

EXTRA_DIST = $(top_srcdir)/adv_coaplib/ini
