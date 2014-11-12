TARGETS	= $(OBJDIR)AppBroker $(OBJDIR)AppClient
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libSystem`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= -L../src `pkg-config --libs libSystem` -lApp
LDFLAGS	= -L$(PREFIX)/lib -Wl,-rpath,$(PREFIX)/lib
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

AppBroker_OBJS = $(OBJDIR)appbroker.o
AppBroker_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
AppBroker_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)AppBroker: $(AppBroker_OBJS)
	$(CC) -o $(OBJDIR)AppBroker $(AppBroker_OBJS) $(AppBroker_LDFLAGS)

AppClient_OBJS = $(OBJDIR)appclient.o
AppClient_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
AppClient_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)AppClient: $(AppClient_OBJS)
	$(CC) -o $(OBJDIR)AppClient $(AppClient_OBJS) $(AppClient_LDFLAGS)

$(OBJDIR)appbroker.o: appbroker.c
	$(CC) $(AppBroker_CFLAGS) -o $(OBJDIR)appbroker.o -c appbroker.c

$(OBJDIR)appclient.o: appclient.c
	$(CC) $(AppClient_CFLAGS) -o $(OBJDIR)appclient.o -c appclient.c

clean:
	$(RM) -- $(AppBroker_OBJS) $(AppClient_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)AppBroker $(DESTDIR)$(BINDIR)/AppBroker

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/AppBroker

.PHONY: all clean distclean install uninstall
