AC_INIT([xmixer],[0.5],[cjacker@foxmail.com])

AM_INIT_AUTOMAKE([foreign dist-xz no-dist-gzip tar-pax])

AC_CONFIG_MACRO_DIRS([m4])

AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LIBTOOL

PKG_CHECK_MODULES(MIXER, gtk+-3.0 >= 3.0.0 alsa glib-2.0 gthread-2.0 libcanberra)
AC_SUBST(MIXER_CFLAGS)
AC_SUBST(MIXER_LIBS)

AC_OUTPUT([ 
Makefile 
src/Makefile
data/Makefile
data/icons/Makefile
data/icons/22x22/Makefile
data/icons/22x22/apps/Makefile
])
