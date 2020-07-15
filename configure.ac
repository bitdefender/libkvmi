AC_INIT([libkvmi], [0.1])

LT_INIT

AC_CONFIG_SRCDIR(src/kvmi.c)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS(config.h)

CFLAGS="$CFLAGS -W -Wall"

AC_ARG_ENABLE(debug,
    [  --enable-debug          compile with gdb debug information],
    CFLAGS="$CFLAGS -g")

AC_ARG_ENABLE(optimize,
    [  --enable-optimize       optimize compiled code (-O2)],
    CFLAGS="$CFLAGS -O2")

AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_CANONICAL_HOST
AS_CASE([$host_cpu],
  [x86_64], [ARCH=x86_64],
  [AC_MSG_ERROR([No support for CPU $host_cpu])]
)
AC_SUBST([ARCH])

AC_OUTPUT(Makefile src/Makefile include/Makefile examples/Makefile libkvmi.pc)
