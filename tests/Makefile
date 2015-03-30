TARGETS	= $(OBJDIR)Dummy.h $(OBJDIR)appclient $(OBJDIR)appmessage $(OBJDIR)appserver $(OBJDIR)includes $(OBJDIR)lookup $(OBJDIR)tests.log $(OBJDIR)transport
OBJDIR	=
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
LDFLAGS	= -L../src -Wl,-rpath,../src -rdynamic
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

$(OBJDIR)Dummy.h: ../data/Dummy.interface ../tools/appbroker.sh
	../tools/appbroker.sh -O APPBROKER=../tools/AppBroker -P "$(PREFIX)" -- "$(OBJDIR)Dummy.h"

appclient_OBJS = $(OBJDIR)appclient.o
appclient_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
appclient_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) ../src/libApp.a

$(OBJDIR)appclient: $(appclient_OBJS)
	$(CC) -o $(OBJDIR)appclient $(appclient_OBJS) $(appclient_LDFLAGS)

appmessage_OBJS = $(OBJDIR)appmessage.o
appmessage_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
appmessage_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) ../src/libApp.a

$(OBJDIR)appmessage: $(appmessage_OBJS)
	$(CC) -o $(OBJDIR)appmessage $(appmessage_OBJS) $(appmessage_LDFLAGS)

appserver_OBJS = $(OBJDIR)appserver.o
appserver_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
appserver_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) ../src/libApp.a

$(OBJDIR)appserver: $(appserver_OBJS)
	$(CC) -o $(OBJDIR)appserver $(appserver_OBJS) $(appserver_LDFLAGS)

includes_OBJS = $(OBJDIR)includes.o
includes_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
includes_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)includes: $(includes_OBJS)
	$(CC) -o $(OBJDIR)includes $(includes_OBJS) $(includes_LDFLAGS)

lookup_OBJS = $(OBJDIR)lookup.o
lookup_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
lookup_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)lookup: $(lookup_OBJS)
	$(CC) -o $(OBJDIR)lookup $(lookup_OBJS) $(lookup_LDFLAGS)

$(OBJDIR)tests.log: appclient appmessage appserver includes lookup tests.sh transport ../src/transport/tcp.c ../src/transport/udp.c
	./tests.sh -P "$(PREFIX)" -- "$(OBJDIR)tests.log"

transport_OBJS = $(OBJDIR)transport.o
transport_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
transport_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)transport: $(transport_OBJS)
	$(CC) -o $(OBJDIR)transport $(transport_OBJS) $(transport_LDFLAGS)

$(OBJDIR)appclient.o: appclient.c ../src/libApp.a
	$(CC) $(appclient_CFLAGS) -o $(OBJDIR)appclient.o -c appclient.c

$(OBJDIR)appmessage.o: appmessage.c ../src/libApp.a
	$(CC) $(appmessage_CFLAGS) -o $(OBJDIR)appmessage.o -c appmessage.c

$(OBJDIR)appserver.o: appserver.c ../src/libApp.a $(OBJDIR)Dummy.h
	$(CC) $(appserver_CFLAGS) -o $(OBJDIR)appserver.o -c appserver.c

$(OBJDIR)includes.o: includes.c
	$(CC) $(includes_CFLAGS) -o $(OBJDIR)includes.o -c includes.c

$(OBJDIR)lookup.o: lookup.c ../src/apptransport.h
	$(CC) $(lookup_CFLAGS) -o $(OBJDIR)lookup.o -c lookup.c

$(OBJDIR)transport.o: transport.c ../src/libApp.a
	$(CC) $(transport_CFLAGS) -o $(OBJDIR)transport.o -c transport.c

clean:
	$(RM) -- $(Dummy.h_OBJS) $(appclient_OBJS) $(appmessage_OBJS) $(appserver_OBJS) $(includes_OBJS) $(lookup_OBJS) $(tests.log_OBJS) $(transport_OBJS)
	../tools/appbroker.sh -O APPBROKER=../tools/AppBroker -c -P "$(PREFIX)" -- "Dummy.h"
	./tests.sh -c -P "$(PREFIX)" -- "tests.log"

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
