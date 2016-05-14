bin_PROGRAMS = 3303parser test_coap test_observe

AM_CPPFLAGS = \
              -I$(top_builddir)/adv_coaplib \
              -I$(top_builddir)/iniparser/src \
              -I$(top_builddir)/ipmg \
              -I$(top_builddir)/platform/linux \
              -I$(top_srcdir)/adv_coaplib \
              -I$(top_srcdir)/iniparser/src \
              -I$(top_srcdir)/ipmg \
              -I$(top_srcdir)/platform/linux

LDADD = ../libAdvCoapLib.la \
        ../../iniparser/libiniparser.la

3303parser_SOURCES = 3303parser.c

test_coap_SOURCES = test_coap.c

test_observe_SOURCES = test_observe.c
