TARGETS	= tcp.so tcp4.so tcp6.so template.so udp.so udp4.so udp6.so
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W -fPIC `pkg-config --cflags libSystem`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --cflags libSystem`
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

tcp_OBJS = tcp.o
tcp_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
tcp_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

tcp.so: $(tcp_OBJS)
	$(CCSHARED) -o tcp.so $(tcp_OBJS) $(tcp_LDFLAGS)

tcp4_OBJS = tcp4.o
tcp4_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
tcp4_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

tcp4.so: $(tcp4_OBJS)
	$(CCSHARED) -o tcp4.so $(tcp4_OBJS) $(tcp4_LDFLAGS)

tcp6_OBJS = tcp6.o
tcp6_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
tcp6_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

tcp6.so: $(tcp6_OBJS)
	$(CCSHARED) -o tcp6.so $(tcp6_OBJS) $(tcp6_LDFLAGS)

template_OBJS = template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

template.so: $(template_OBJS)
	$(CCSHARED) -o template.so $(template_OBJS) $(template_LDFLAGS)

udp_OBJS = udp.o
udp_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
udp_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

udp.so: $(udp_OBJS)
	$(CCSHARED) -o udp.so $(udp_OBJS) $(udp_LDFLAGS)

udp4_OBJS = udp4.o
udp4_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
udp4_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

udp4.so: $(udp4_OBJS)
	$(CCSHARED) -o udp4.so $(udp4_OBJS) $(udp4_LDFLAGS)

udp6_OBJS = udp6.o
udp6_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
udp6_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

udp6.so: $(udp6_OBJS)
	$(CCSHARED) -o udp6.so $(udp6_OBJS) $(udp6_LDFLAGS)

tcp.o: tcp.c
	$(CC) $(tcp_CFLAGS) -c tcp.c

tcp4.o: tcp4.c tcp.c
	$(CC) $(tcp4_CFLAGS) -c tcp4.c

tcp6.o: tcp6.c tcp.c
	$(CC) $(tcp6_CFLAGS) -c tcp6.c

template.o: template.c
	$(CC) $(template_CFLAGS) -c template.c

udp.o: udp.c
	$(CC) $(udp_CFLAGS) -c udp.c

udp4.o: udp4.c udp.c
	$(CC) $(udp4_CFLAGS) -c udp4.c

udp6.o: udp6.c udp.c
	$(CC) $(udp6_CFLAGS) -c udp6.c

clean:
	$(RM) -- $(tcp_OBJS) $(tcp4_OBJS) $(tcp6_OBJS) $(template_OBJS) $(udp_OBJS) $(udp4_OBJS) $(udp6_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0644 -- tcp.so $(DESTDIR)$(LIBDIR)/App/transport/tcp.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0644 -- tcp4.so $(DESTDIR)$(LIBDIR)/App/transport/tcp4.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0644 -- tcp6.so $(DESTDIR)$(LIBDIR)/App/transport/tcp6.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0644 -- udp.so $(DESTDIR)$(LIBDIR)/App/transport/udp.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0644 -- udp4.so $(DESTDIR)$(LIBDIR)/App/transport/udp4.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0644 -- udp6.so $(DESTDIR)$(LIBDIR)/App/transport/udp6.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/tcp.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/tcp4.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/tcp6.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/udp.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/udp4.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/udp6.so

.PHONY: all clean distclean install uninstall
