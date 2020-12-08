AM_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/linux/$(ARCH)

lib_LTLIBRARIES = libkvmi.la

libkvmi_la_SOURCES = kvmi.c
libkvmi_la_LDFLAGS = -pthread -version-number 1:0 \
	-Wl,--version-script,$(srcdir)/version.ld
