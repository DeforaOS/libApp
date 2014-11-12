TARGETS	= $(OBJDIR)libApp.a $(OBJDIR)libApp.so.0.0 $(OBJDIR)libApp.so.0 $(OBJDIR)libApp.so
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
LIBDIR	= $(PREFIX)/lib
CC	= cc
CPPFLAGSF= -I ../include
CPPFLAGS= -I $(PREFIX)/include
CFLAGSF	= -W -fPIC `pkg-config --cflags libSystem`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs libSystem`
LDFLAGS	= 
AR	= ar
RANLIB	= ranlib
CCSHARED= $(CC) -shared
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

libApp_OBJS = $(OBJDIR)appclient.o $(OBJDIR)appinterface.o $(OBJDIR)appmessage.o $(OBJDIR)appserver.o $(OBJDIR)apptransport.o $(OBJDIR)marshall.o
libApp_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
libApp_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)  

$(OBJDIR)libApp.a: $(libApp_OBJS)
	$(AR) -rc $(OBJDIR)libApp.a $(libApp_OBJS)
	$(RANLIB) $(OBJDIR)libApp.a

$(OBJDIR)libApp.so.0.0 $(OBJDIR)libApp.so.0 $(OBJDIR)libApp.so: $(libApp_OBJS)
	$(CCSHARED) -o $(OBJDIR)libApp.so.0.0 -Wl,-soname,libApp.so.0 $(libApp_OBJS) $(libApp_LDFLAGS)
	$(LN) -s -- libApp.so.0.0 $(OBJDIR)libApp.so.0
	$(LN) -s -- libApp.so.0.0 $(OBJDIR)libApp.so

$(OBJDIR)appclient.o: appclient.c appinterface.h ../include/App/appclient.h
	$(CC) $(libApp_CFLAGS) -o $(OBJDIR)appclient.o -c appclient.c

$(OBJDIR)appinterface.o: appinterface.c ../include/App/appserver.h marshall.h ../config.h
	$(CC) $(libApp_CFLAGS) -o $(OBJDIR)appinterface.o -c appinterface.c

$(OBJDIR)appmessage.o: appmessage.c ../include/App/appmessage.h appmessage.h
	$(CC) $(libApp_CFLAGS) -o $(OBJDIR)appmessage.o -c appmessage.c

$(OBJDIR)appserver.o: appserver.c appinterface.h ../include/App/appserver.h ../config.h
	$(CC) $(libApp_CFLAGS) -o $(OBJDIR)appserver.o -c appserver.c

$(OBJDIR)apptransport.o: apptransport.c apptransport.h ../include/App/apptransport.h ../config.h
	$(CC) $(libApp_CFLAGS) -o $(OBJDIR)apptransport.o -c apptransport.c

$(OBJDIR)marshall.o: marshall.c marshall.h
	$(CC) $(libApp_CFLAGS) -o $(OBJDIR)marshall.o -c marshall.c

clean:
	$(RM) -- $(libApp_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(LIBDIR)
	$(INSTALL) -m 0644 $(OBJDIR)libApp.a $(DESTDIR)$(LIBDIR)/libApp.a
	$(INSTALL) -m 0755 $(OBJDIR)libApp.so.0.0 $(DESTDIR)$(LIBDIR)/libApp.so.0.0
	$(LN) -s -- libApp.so.0.0 $(DESTDIR)$(LIBDIR)/libApp.so.0
	$(LN) -s -- libApp.so.0.0 $(DESTDIR)$(LIBDIR)/libApp.so

uninstall:
	$(RM) -- $(DESTDIR)$(LIBDIR)/libApp.a
	$(RM) -- $(DESTDIR)$(LIBDIR)/libApp.so.0.0
	$(RM) -- $(DESTDIR)$(LIBDIR)/libApp.so.0
	$(RM) -- $(DESTDIR)$(LIBDIR)/libApp.so

.PHONY: all clean distclean install uninstall
