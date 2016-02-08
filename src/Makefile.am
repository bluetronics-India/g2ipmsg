## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = g2ipmsg
common_sources= \
	compat.h ipmsg_types.h      \
	ipmsg.c	copying.h g2ipmsg.h \
	support.c support.h \
	interface.c interface.h \
	callbacks.c callbacks.h \
        hostinfo.c hostinfo.h \
        msginfo.c msginfo.h  \
        udp.c udp.h      \
        ipmsg.h  private.h    \
        common.h msgout.h     \
        message.c message.h   \
        userdb.c userdb.h     \
        protocol.h protocol.c \
	codeset.h codeset.c   \
	recvmsg.c             \
	logfile.h logfile.c   \
	menu.c menu.h         \
	fileattach.h fileattach.c \
	tcp.c tcp.h               \
	sound.c sound.h           \
	netcommon.c  netcommon.h  \
	fuzai.c  fuzai.h          \
	uicommon.h uicommon.c     \
	systray.h systray.c       \
	downloads.h downloads.c   \
	dialog.c                  \
	cryptcommon.h             \
	util.h util.c             


if OPENSSL_ENABLED
common_sources += \
	base64.h base64.c      \
	pbkdf2.h pbkdf2.c      \
	symcrypt.h symcrypt.c  \
	rand.c  cryptif.c      \
	pubcrypt.h pubcrypt.c
endif

if DBUSGLIB_ENABLED
common_sources += \
	dbusif.c dbusif.h
endif 

if GNOME_SCREENSAVER_ENABLED
common_sources += \
	screensaver.c screensaver.h
endif

g2ipmsg_SOURCES =           \
	$(common_sources)   \
	main.c 

if ENABLE_APPLET
bin_PROGRAMS += g2ipmsg_applet

g2ipmsg_applet_SOURCES =    \
	$(common_sources)   \
	applet.c
endif

g2ipmsg_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)
g2ipmsg_applet_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)
