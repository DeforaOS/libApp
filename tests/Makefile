TARGETS	= AppBroker tests.log transport
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem`
CFLAGS	= -Wall -g -O2
LDFLAGSF= `pkg-config --libs libSystem`
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -p
INSTALL	= install


all: $(TARGETS)

AppBroker_OBJS = ../tools/appbroker.o
AppBroker_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
AppBroker_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) ../src/libApp.a

AppBroker: $(AppBroker_OBJS)
	$(CC) -o AppBroker $(AppBroker_OBJS) $(AppBroker_LDFLAGS)

tests.log: transport ../src/transport/tcp.c ../src/transport/udp.c
	./tests.sh -P "$(PREFIX)" -- "tests.log"

transport_OBJS = transport.o
transport_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
transport_LDFLAGS = $(LDFLAGSF) $(LDFLAGS) -lApp -L../src -Wl,-rpath,../src

transport: $(transport_OBJS)
	$(CC) -o transport $(transport_OBJS) $(transport_LDFLAGS)

../tools/appbroker.o: ../tools/appbroker.c ../src/libApp.a
	$(CC) $(AppBroker_CFLAGS) -o ../tools/appbroker.o -c ../tools/appbroker.c

transport.o: transport.c ../src/libApp.a
	$(CC) $(transport_CFLAGS) -c transport.c

clean:
	$(RM) -- $(AppBroker_OBJS) $(tests.log_OBJS) $(transport_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)

uninstall:

.PHONY: all clean distclean install uninstall
