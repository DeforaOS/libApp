TARGETS	= appmessage appserver tests.log transport
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libSystem` -lApp
LDFLAGS	= -L../src -Wl,-rpath,../src
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

appmessage_OBJS = appmessage.o
appmessage_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
appmessage_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) ../src/libApp.a

appmessage: $(appmessage_OBJS)
	$(CC) -o appmessage $(appmessage_OBJS) $(appmessage_LDFLAGS)

appserver_OBJS = appserver.o
appserver_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
appserver_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) ../src/libApp.a

appserver: $(appserver_OBJS)
	$(CC) -o appserver $(appserver_OBJS) $(appserver_LDFLAGS)

tests.log: appmessage appserver tests.sh transport ../src/transport/tcp.c ../src/transport/udp.c
	./tests.sh -P "$(PREFIX)" -- "tests.log"

transport_OBJS = transport.o
transport_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
transport_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

transport: $(transport_OBJS)
	$(CC) -o transport $(transport_OBJS) $(transport_LDFLAGS)

appmessage.o: appmessage.c ../src/libApp.a
	$(CC) $(appmessage_CFLAGS) -c appmessage.c

appserver.o: appserver.c ../src/libApp.a
	$(CC) $(appserver_CFLAGS) -c appserver.c

transport.o: transport.c ../src/libApp.a
	$(CC) $(transport_CFLAGS) -c transport.c

clean:
	$(RM) -- $(appmessage_OBJS) $(appserver_OBJS) $(tests.log_OBJS) $(transport_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
