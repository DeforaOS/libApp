PACKAGE	= libApp
VERSION	= 0.1.5
SUBDIRS	= data doc include src src/transport tests tools
RM	= rm -f
LN	= ln -f
TAR	= tar
MKDIR	= mkdir -m 0755 -p


all: subdirs

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done

dist:
	$(RM) -r -- $(OBJDIR)$(PACKAGE)-$(VERSION)
	$(LN) -s -- "$$PWD" $(OBJDIR)$(PACKAGE)-$(VERSION)
	@cd $(OBJDIR). && $(TAR) -czvf $(OBJDIR)$(PACKAGE)-$(VERSION).tar.gz -- \
		$(PACKAGE)-$(VERSION)/data/Makefile \
		$(PACKAGE)-$(VERSION)/data/Dummy.interface \
		$(PACKAGE)-$(VERSION)/data/appbroker.sh \
		$(PACKAGE)-$(VERSION)/data/libApp.pc.in \
		$(PACKAGE)-$(VERSION)/data/pkgconfig.sh \
		$(PACKAGE)-$(VERSION)/data/project.conf \
		$(PACKAGE)-$(VERSION)/doc/Makefile \
		$(PACKAGE)-$(VERSION)/doc/AppBroker.css.xml \
		$(PACKAGE)-$(VERSION)/doc/AppBroker.xml \
		$(PACKAGE)-$(VERSION)/doc/AppClient.css.xml \
		$(PACKAGE)-$(VERSION)/doc/AppClient.xml \
		$(PACKAGE)-$(VERSION)/doc/docbook.sh \
		$(PACKAGE)-$(VERSION)/doc/gtkdoc.sh \
		$(PACKAGE)-$(VERSION)/doc/manual.css.xml \
		$(PACKAGE)-$(VERSION)/doc/project.conf \
		$(PACKAGE)-$(VERSION)/doc/gtkdoc/Makefile \
		$(PACKAGE)-$(VERSION)/doc/gtkdoc/libApp-docs.xml \
		$(PACKAGE)-$(VERSION)/doc/gtkdoc/project.conf \
		$(PACKAGE)-$(VERSION)/include/App.h \
		$(PACKAGE)-$(VERSION)/include/Makefile \
		$(PACKAGE)-$(VERSION)/include/project.conf \
		$(PACKAGE)-$(VERSION)/include/App/app.h \
		$(PACKAGE)-$(VERSION)/include/App/appclient.h \
		$(PACKAGE)-$(VERSION)/include/App/appmessage.h \
		$(PACKAGE)-$(VERSION)/include/App/appserver.h \
		$(PACKAGE)-$(VERSION)/include/App/apptransport.h \
		$(PACKAGE)-$(VERSION)/include/App/Makefile \
		$(PACKAGE)-$(VERSION)/include/App/project.conf \
		$(PACKAGE)-$(VERSION)/src/appclient.c \
		$(PACKAGE)-$(VERSION)/src/appinterface.c \
		$(PACKAGE)-$(VERSION)/src/appmessage.c \
		$(PACKAGE)-$(VERSION)/src/appserver.c \
		$(PACKAGE)-$(VERSION)/src/apptransport.c \
		$(PACKAGE)-$(VERSION)/src/marshall.c \
		$(PACKAGE)-$(VERSION)/src/Makefile \
		$(PACKAGE)-$(VERSION)/src/appinterface.h \
		$(PACKAGE)-$(VERSION)/src/appmessage.h \
		$(PACKAGE)-$(VERSION)/src/apptransport.h \
		$(PACKAGE)-$(VERSION)/src/marshall.h \
		$(PACKAGE)-$(VERSION)/src/project.conf \
		$(PACKAGE)-$(VERSION)/src/transport/self.c \
		$(PACKAGE)-$(VERSION)/src/transport/tcp.c \
		$(PACKAGE)-$(VERSION)/src/transport/tcp4.c \
		$(PACKAGE)-$(VERSION)/src/transport/tcp6.c \
		$(PACKAGE)-$(VERSION)/src/transport/template.c \
		$(PACKAGE)-$(VERSION)/src/transport/udp.c \
		$(PACKAGE)-$(VERSION)/src/transport/udp4.c \
		$(PACKAGE)-$(VERSION)/src/transport/udp6.c \
		$(PACKAGE)-$(VERSION)/src/transport/Makefile \
		$(PACKAGE)-$(VERSION)/src/transport/common.h \
		$(PACKAGE)-$(VERSION)/src/transport/common.c \
		$(PACKAGE)-$(VERSION)/src/transport/project.conf \
		$(PACKAGE)-$(VERSION)/tests/appclient.c \
		$(PACKAGE)-$(VERSION)/tests/appmessage.c \
		$(PACKAGE)-$(VERSION)/tests/appserver.c \
		$(PACKAGE)-$(VERSION)/tests/includes.c \
		$(PACKAGE)-$(VERSION)/tests/lookup.c \
		$(PACKAGE)-$(VERSION)/tests/transport.c \
		$(PACKAGE)-$(VERSION)/tests/Makefile \
		$(PACKAGE)-$(VERSION)/tests/tests.sh \
		$(PACKAGE)-$(VERSION)/tests/project.conf \
		$(PACKAGE)-$(VERSION)/tools/appbroker.c \
		$(PACKAGE)-$(VERSION)/tools/appclient.c \
		$(PACKAGE)-$(VERSION)/tools/Makefile \
		$(PACKAGE)-$(VERSION)/tools/project.conf \
		$(PACKAGE)-$(VERSION)/Makefile \
		$(PACKAGE)-$(VERSION)/COPYING \
		$(PACKAGE)-$(VERSION)/config.h \
		$(PACKAGE)-$(VERSION)/config.sh \
		$(PACKAGE)-$(VERSION)/project.conf
	$(RM) -- $(OBJDIR)$(PACKAGE)-$(VERSION)

distcheck: dist
	$(TAR) -xzvf $(OBJDIR)$(PACKAGE)-$(VERSION).tar.gz
	$(MKDIR) -- $(PACKAGE)-$(VERSION)/objdir
	$(MKDIR) -- $(PACKAGE)-$(VERSION)/destdir
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/")
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" DESTDIR="$$PWD/destdir" install)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" DESTDIR="$$PWD/destdir" uninstall)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) OBJDIR="$$PWD/objdir/" distclean)
	(cd "$(PACKAGE)-$(VERSION)" && $(MAKE) dist)
	$(RM) -r -- $(PACKAGE)-$(VERSION)

install:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done

.PHONY: all subdirs clean distclean dist distcheck install uninstall
