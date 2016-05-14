SUBDIRS = . ini testapp

noinst_LTLIBRARIES = libAdvCoapLib.la

libAdvCoapLib_la_CPPFLAGS = \
              -I$(top_builddir)/iniparser/src \
              -I$(top_builddir)/ipmg \
              -I$(top_builddir)/platform/linux \
              -I$(top_srcdir)/iniparser/src \
              -I$(top_srcdir)/ipmg \
              -I$(top_srcdir)/platform/linux

libAdvCoapLib_la_LIBADD = ../iniparser/libiniparser.la

libAdvCoapLib_la_SOURCES = adv_coap.c \
                           adv_payload_parser.c \
                           ipso_id.h

include_HEADERS = adv_coap.h \
                  adv_payload_parser.h \
                  uri_cmd.h
