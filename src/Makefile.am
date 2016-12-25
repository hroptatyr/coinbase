### Makefile.am
include $(top_builddir)/version.mk

AM_CFLAGS = $(EXTRA_CFLAGS)
AM_CPPFLAGS = -D_POSIX_C_SOURCE=201001L -D_XOPEN_SOURCE=700 -D_BSD_SOURCE
AM_CPPFLAGS += -DHAVE_VERSION_H
AM_LDFLAGS = $(XCCLDFLAGS)

bin_PROGRAMS =
noinst_PROGRAMS =
noinst_LTLIBRARIES =
BUILT_SOURCES =
EXTRA_DIST = $(BUILT_SOURCES)
CLEANFILES =
SUFFIXES =

bin_PROGRAMS += coin
coin_SOURCES = coin.c coin.yuck
coin_SOURCES += tls.c tls.h
coin_SOURCES += version.c version.h
coin_SOURCES += boobs.h
coin_CPPFLAGS = $(AM_CPPFLAGS) $(LIBEV_CPPFLAGS)
coin_LDFLAGS = $(AM_LDFLAGS)  $(LIBEV_LDFLAGS)
coin_LDADD = $(LIBEV_LIBS) -lssl -lcrypto
BUILT_SOURCES += coin.yucc

bin_PROGRAMS += coin2b
coin2b_SOURCES = coin2b.c coin2b.yuck
coin2b_SOURCES += jsmn.c jsmn.h
coin2b_SOURCES += hash.c hash.h
coin2b_SOURCES += dfp754_d32.c dfp754_d32.h
coin2b_SOURCES += dfp754_d64.c dfp754_d64.h
coin2b_SOURCES += version.c version.h
coin2b_CPPFLAGS = $(AM_CPPFLAGS)
coin2b_CPPFLAGS += $(dfp754_CFLAGS)
coin2b_LDFLAGS = $(AM_LDFLAGS)
coin2b_LDFLAGS += $(dfp754_LIBS)
BUILT_SOURCES += coin2b.yucc


CLEANFILES += version.c
EXTRA_DIST += version.c.in


## version rules
version.c: version.c.in $(top_builddir)/.version
	$(AM_V_GEN) PATH="$(top_builddir)/build-aux:$${PATH}" \
		yuck$(EXEEXT) scmver --ignore-noscm --force -o $@ \
			--use-reference --reference $(top_builddir)/.version $<

## yuck rule
SUFFIXES += .yuck
SUFFIXES += .yucc
.yuck.yucc:
	$(AM_V_GEN) PATH=$(top_builddir)/build-aux:"$${PATH}" \
		yuck$(EXEEXT) gen -o $@ $<

## Create preprocessor output (debugging purposes only)
.c.i:
	$(COMPILE) -E -o $@ $<

## Create assembler output (debugging purposes only)
.c.s:
	$(COMPILE) -S -c $(AM_CFLAGS) $<

## Makefile.am ends here
