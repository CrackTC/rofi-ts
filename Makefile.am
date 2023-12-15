ACLOCAL_AMFLAGS=-I m4
plugindir=${PWD}/lib/rofi

plugin_LTLIBRARIES = ts.la

ts_la_SOURCES=\
		 src/ts.c

ts_la_CFLAGS= @glib_CFLAGS@ @rofi_CFLAGS@ @cairo_CFLAGS@
ts_la_LIBADD= @glib_LIBS@ @rofi_LIBS@ @cairo_LIBS@
ts_la_LDFLAGS= -module -avoid-version
