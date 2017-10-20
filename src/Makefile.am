bin_PROGRAMS = xmixer
xmixer_SOURCES = xmixer.c alsa.c config.c keybinder.c gsd-media-keys-window.c msd-osd-window.c
xmixer_CFLAGS = $(MIXER_CFLAGS)  -DXMIXER_DATA=\"$(pkgdatadir)\"
xmixer_LDFLAGS = -module -avoid-version ${MIXER_LIBS} -lX11 -lm

noinst_HEADERS = alsa.h config.h gsd-media-keys-window.h keybinder.h msd-osd-window.h
