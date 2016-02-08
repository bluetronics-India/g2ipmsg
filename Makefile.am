## Process this file with automake to produce Makefile.in

SUBDIRS = src po

include aminclude.am

serverdir = $(libdir)/bonobo/servers
server_in_files = g2ipmsg.server.in
server_DATA = $(server_in_files:.server.in=.server)
$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@BINDIR\@|$(bindir)|" $< > $@

@INTLTOOL_SERVER_RULE@

Applicationdir = $(datadir)/applications
Application_in_files = g2ipmsg.desktop.in
Application_DATA = $(Application_in_files:.desktop.in=.desktop) 
@INTLTOOL_DESKTOP_RULE@

schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = g2ipmsg.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas) 	
@INTLTOOL_SCHEMAS_RULE@

uidir = $(datadir)/gnome-2.0/ui
ui_DATA = g2ipmsg.xml

MOSTLYCLEANFILES = $(DX_CLEANFILES)

EXTRA_DIST =          \
	$(DX_CONFIG)  \
	macros/*.m4   \
	aminclude.am  \
	autogen.sh    \
	g2ipmsg.glade \
	g2ipmsg.gladep \
	g2ipmsg.server.in.in \
	g2ipmsg.desktop.in \
	g2ipmsg.schemas.in  \
	g2ipmsg.xml    \
	g2ipmsg.spec   \
	README.jp      \
	intltool-extract.in \
	intltool-update.in  \
	intltool-merge.in

install-data-local: install-schemas
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	  for pixmap in $(srcdir)/pixmaps/*.xpm; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	    fi \
	  done \
	fi
	if test -d $(srcdir)/sounds; then \
	  $(mkinstalldirs) $(DESTDIR)$(datadir)/sounds/$(PACKAGE); \
	  for sound in $(srcdir)/sounds/*; do \
	    if test -f $$sound; then \
	      $(INSTALL_DATA) $$sound $(DESTDIR)$(datadir)/sounds/$(PACKAGE); \
	    fi \
	  done \
	fi 
	  for pngfile in $(srcdir)/pixmaps/*.png; do \
	    if test -f $$pngfile; then \
	      $(INSTALL_DATA) $$pngfile $(DESTDIR)$(datadir)/pixmaps/$(PACKAGE); \
	    fi \
	  done 
#for GNOME menu icons
	$(INSTALL_DATA) $(srcdir)/pixmaps/ipmsg.png $(DESTDIR)$(datadir)/pixmaps

# 	if test -f $(srcdir)/$(server_DATA); then \
# 	     $(INSTALL_DATA) $(srcdir)/$(server_DATA) $(serverdir); \
# 	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi
	if test -d sounds; then \
	  mkdir $(distdir)/sounds; \
	  for sound in sounds/*; do \
	    if test -f $$sound; then \
	      cp -p $$sound $(distdir)/sounds; \
	    fi \
	  done \
	fi 

install-schemas: g2ipmsg.schemas
if GCONF_SCHEMAS_INSTALL
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
	gconftool-2 --makefile-install-rule g2ipmsg.schemas
endif GCONF_SCHEMAS_INSTALL
