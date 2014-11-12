MKDIR	= mkdir -m 0755 -p
INSTALL	= install
RM	= rm -f
TARGETS	= $(OBJDIR)libApp.pc
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)libApp.pc: libApp.pc.in ../config.sh
	./pkgconfig.sh -P "$(PREFIX)" -- "$(OBJDIR)libApp.pc"

clean:
	$(RM) -- $(libApp.pc_OBJS)
	./pkgconfig.sh -c -P "$(PREFIX)" -- "libApp.pc"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	./pkgconfig.sh -P "$(DESTDIR)$(PREFIX)" -i -- "$(OBJDIR)libApp.pc"
	$(MKDIR) $(DESTDIR)$(PREFIX)/etc/AppInterface
	$(INSTALL) -m 0644 Dummy.interface $(DESTDIR)$(PREFIX)/etc/AppInterface/Dummy.interface

uninstall:
	./pkgconfig.sh -P "$(DESTDIR)$(PREFIX)" -u -- "libApp.pc"
	$(RM) -- $(DESTDIR)$(PREFIX)/etc/AppInterface/Dummy.interface

.PHONY: all clean distclean install uninstall
