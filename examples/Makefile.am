AM_CFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/include/linux/$(ARCH)

bin_PROGRAMS = hookguest-libkvmi

hookguest_libkvmi_SOURCES = hookguest.c
hookguest_libkvmi_LDADD = $(top_builddir)/src/libkvmi.la -ldl
