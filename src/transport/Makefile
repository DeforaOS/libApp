TARGETS	= $(OBJDIR)self.so $(OBJDIR)tcp.so $(OBJDIR)tcp4.so $(OBJDIR)tcp6.so $(OBJDIR)template.so $(OBJDIR)udp.so $(OBJDIR)udp4.so $(OBJDIR)udp6.so
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../../include
CPPFLAGS=
CFLAGSF	= -W -fPIC `pkg-config --cflags libSystem`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs libSystem` -L.. -lApp
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

self_OBJS = $(OBJDIR)self.o
self_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
self_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) 

$(OBJDIR)self.so: $(self_OBJS)
	$(CCSHARED) -o $(OBJDIR)self.so $(self_OBJS) $(self_LDFLAGS)

tcp_OBJS = $(OBJDIR)tcp.o
tcp_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
tcp_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) 

$(OBJDIR)tcp.so: $(tcp_OBJS)
	$(CCSHARED) -o $(OBJDIR)tcp.so $(tcp_OBJS) $(tcp_LDFLAGS)

tcp4_OBJS = $(OBJDIR)tcp4.o
tcp4_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
tcp4_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) 

$(OBJDIR)tcp4.so: $(tcp4_OBJS)
	$(CCSHARED) -o $(OBJDIR)tcp4.so $(tcp4_OBJS) $(tcp4_LDFLAGS)

tcp6_OBJS = $(OBJDIR)tcp6.o
tcp6_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
tcp6_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) 

$(OBJDIR)tcp6.so: $(tcp6_OBJS)
	$(CCSHARED) -o $(OBJDIR)tcp6.so $(tcp6_OBJS) $(tcp6_LDFLAGS)

template_OBJS = $(OBJDIR)template.o
template_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
template_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)template.so: $(template_OBJS)
	$(CCSHARED) -o $(OBJDIR)template.so $(template_OBJS) $(template_LDFLAGS)

udp_OBJS = $(OBJDIR)udp.o
udp_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
udp_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) 

$(OBJDIR)udp.so: $(udp_OBJS)
	$(CCSHARED) -o $(OBJDIR)udp.so $(udp_OBJS) $(udp_LDFLAGS)

udp4_OBJS = $(OBJDIR)udp4.o
udp4_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
udp4_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) 

$(OBJDIR)udp4.so: $(udp4_OBJS)
	$(CCSHARED) -o $(OBJDIR)udp4.so $(udp4_OBJS) $(udp4_LDFLAGS)

udp6_OBJS = $(OBJDIR)udp6.o
udp6_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
udp6_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) 

$(OBJDIR)udp6.so: $(udp6_OBJS)
	$(CCSHARED) -o $(OBJDIR)udp6.so $(udp6_OBJS) $(udp6_LDFLAGS)

$(OBJDIR)self.o: self.c
	$(CC) $(self_CFLAGS) -o $(OBJDIR)self.o -c self.c

$(OBJDIR)tcp.o: tcp.c common.h common.c
	$(CC) $(tcp_CFLAGS) -o $(OBJDIR)tcp.o -c tcp.c

$(OBJDIR)tcp4.o: tcp4.c tcp.c common.h common.c
	$(CC) $(tcp4_CFLAGS) -o $(OBJDIR)tcp4.o -c tcp4.c

$(OBJDIR)tcp6.o: tcp6.c tcp.c common.h common.c
	$(CC) $(tcp6_CFLAGS) -o $(OBJDIR)tcp6.o -c tcp6.c

$(OBJDIR)template.o: template.c
	$(CC) $(template_CFLAGS) -o $(OBJDIR)template.o -c template.c

$(OBJDIR)udp.o: udp.c common.h common.c
	$(CC) $(udp_CFLAGS) -o $(OBJDIR)udp.o -c udp.c

$(OBJDIR)udp4.o: udp4.c udp.c common.h common.c
	$(CC) $(udp4_CFLAGS) -o $(OBJDIR)udp4.o -c udp4.c

$(OBJDIR)udp6.o: udp6.c udp.c common.h common.c
	$(CC) $(udp6_CFLAGS) -o $(OBJDIR)udp6.o -c udp6.c

clean:
	$(RM) -- $(self_OBJS) $(tcp_OBJS) $(tcp4_OBJS) $(tcp6_OBJS) $(template_OBJS) $(udp_OBJS) $(udp4_OBJS) $(udp6_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0755 $(OBJDIR)tcp.so $(DESTDIR)$(LIBDIR)/App/transport/tcp.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0755 $(OBJDIR)tcp4.so $(DESTDIR)$(LIBDIR)/App/transport/tcp4.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0755 $(OBJDIR)tcp6.so $(DESTDIR)$(LIBDIR)/App/transport/tcp6.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0755 $(OBJDIR)udp.so $(DESTDIR)$(LIBDIR)/App/transport/udp.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0755 $(OBJDIR)udp4.so $(DESTDIR)$(LIBDIR)/App/transport/udp4.so
	$(MKDIR) $(DESTDIR)$(LIBDIR)/App/transport
	$(INSTALL) -m 0755 $(OBJDIR)udp6.so $(DESTDIR)$(LIBDIR)/App/transport/udp6.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/tcp.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/tcp4.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/tcp6.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/udp.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/udp4.so
	$(RM) -- $(DESTDIR)$(LIBDIR)/App/transport/udp6.so

.PHONY: all clean distclean install uninstall
