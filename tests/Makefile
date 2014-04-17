TARGETS	= appclient appmessage appserver lookup tests.log transport
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

appclient_OBJS = appclient.o
appclient_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
appclient_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) ../src/libApp.a

appclient: $(appclient_OBJS)
	$(CC) -o appclient $(appclient_OBJS) $(appclient_LDFLAGS)

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

lookup_OBJS = lookup.o
lookup_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
lookup_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

lookup: $(lookup_OBJS)
	$(CC) -o lookup $(lookup_OBJS) $(lookup_LDFLAGS)

tests.log: appclient appmessage appserver lookup tests.sh transport ../src/transport/tcp.c ../src/transport/udp.c
	./tests.sh -P "$(PREFIX)" -- "tests.log"

transport_OBJS = transport.o
transport_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
transport_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

transport: $(transport_OBJS)
	$(CC) -o transport $(transport_OBJS) $(transport_LDFLAGS)

appclient.o: appclient.c ../src/libApp.a
	$(CC) $(appclient_CFLAGS) -c appclient.c

appmessage.o: appmessage.c ../src/libApp.a
	$(CC) $(appmessage_CFLAGS) -c appmessage.c

appserver.o: appserver.c ../src/libApp.a
	$(CC) $(appserver_CFLAGS) -c appserver.c

lookup.o: lookup.c ../src/apptransport.h
	$(CC) $(lookup_CFLAGS) -c lookup.c

transport.o: transport.c ../src/libApp.a
	$(CC) $(transport_CFLAGS) -c transport.c

clean:
	$(RM) -- $(appclient_OBJS) $(appmessage_OBJS) $(appserver_OBJS) $(lookup_OBJS) $(tests.log_OBJS) $(transport_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
