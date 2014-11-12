SUBDIRS	= gtkdoc
TARGETS	= $(OBJDIR)AppBroker.1 $(OBJDIR)AppBroker.html $(OBJDIR)AppClient.1 $(OBJDIR)AppClient.html $(OBJDIR)gtkdoc/libApp.types $(OBJDIR)gtkdoc/tmpl.stamp $(OBJDIR)gtkdoc/sgml.stamp $(OBJDIR)gtkdoc/html.stamp
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: subdirs $(TARGETS)

subdirs:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE)) || exit; done

$(OBJDIR)AppBroker.1: AppBroker.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)AppBroker.1"

$(OBJDIR)AppBroker.html: AppBroker.css.xml AppBroker.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)AppBroker.html"

$(OBJDIR)AppClient.1: AppClient.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)AppClient.1"

$(OBJDIR)AppClient.html: AppClient.css.xml AppClient.xml manual.css.xml
	./docbook.sh -P "$(PREFIX)" -- "$(OBJDIR)AppClient.html"

$(OBJDIR)gtkdoc/libApp.types:
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/libApp.types"

$(OBJDIR)gtkdoc/tmpl.stamp: $(OBJDIR)gtkdoc/libApp.types
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/tmpl.stamp"

$(OBJDIR)gtkdoc/sgml.stamp: $(OBJDIR)gtkdoc/tmpl.stamp
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/sgml.stamp"

$(OBJDIR)gtkdoc/html.stamp: gtkdoc/libApp-docs.xml $(OBJDIR)gtkdoc/sgml.stamp $(OBJDIR)gtkdoc/tmpl.stamp
	./gtkdoc.sh -P "$(PREFIX)" -- "$(OBJDIR)gtkdoc/html.stamp"

clean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) clean) || exit; done
	$(RM) -- $(AppBroker.1_OBJS) $(AppBroker.html_OBJS) $(AppClient.1_OBJS) $(AppClient.html_OBJS) $(gtkdoc/libApp.types_OBJS) $(gtkdoc/tmpl.stamp_OBJS) $(gtkdoc/sgml.stamp_OBJS) $(gtkdoc/html.stamp_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "AppBroker.1"
	./docbook.sh -c -P "$(PREFIX)" -- "AppBroker.html"
	./docbook.sh -c -P "$(PREFIX)" -- "AppClient.1"
	./docbook.sh -c -P "$(PREFIX)" -- "AppClient.html"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/libApp.types"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/tmpl.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/sgml.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/html.stamp"

distclean:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) distclean) || exit; done
	$(RM) -- $(AppBroker.1_OBJS) $(AppBroker.html_OBJS) $(AppClient.1_OBJS) $(AppClient.html_OBJS) $(gtkdoc/libApp.types_OBJS) $(gtkdoc/tmpl.stamp_OBJS) $(gtkdoc/sgml.stamp_OBJS) $(gtkdoc/html.stamp_OBJS)
	./docbook.sh -c -P "$(PREFIX)" -- "AppBroker.1"
	./docbook.sh -c -P "$(PREFIX)" -- "AppBroker.html"
	./docbook.sh -c -P "$(PREFIX)" -- "AppClient.1"
	./docbook.sh -c -P "$(PREFIX)" -- "AppClient.html"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/libApp.types"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/tmpl.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/sgml.stamp"
	./gtkdoc.sh -c -P "$(PREFIX)" -- "gtkdoc/html.stamp"
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) install) || exit; done
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)AppBroker.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)AppBroker.html"
	./gtkdoc.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)gtkdoc/html.stamp"

uninstall:
	@for i in $(SUBDIRS); do (cd "$$i" && $(MAKE) uninstall) || exit; done
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "AppBroker.1"
	./docbook.sh -P "$(DESTDIR)$(PREFIX)" -u -- "AppBroker.html"
	./gtkdoc.sh -P "$(DESTDIR)$(PREFIX)" -u -- "gtkdoc/html.stamp"

.PHONY: all subdirs clean distclean install uninstall
