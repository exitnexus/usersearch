###
# Configuration Options
###

###
# Local Settings
###
CC			= g++
CFLAGS		= -Wall -fno-strict-aliasing -I/usr/local/include -L/usr/local/lib -Ilibevent -Ilibevent/compat

DAEMON		= usersearch
DAEMON_O	= tqueue.o search.o
DAEMON_INC  = \
	libevent/.libs/buffer.o \
	libevent/.libs/epoll.o \
	libevent/.libs/evbuffer.o \
	libevent/.libs/event.o \
	libevent/.libs/event_tagging.o \
	libevent/.libs/http.o \
	libevent/.libs/log.o \
	libevent/.libs/poll.o \
	libevent/.libs/select.o \
	libevent/.libs/signal.o \
	libevent/.libs/strlcpy.o

DAEMON_L	= -lpthread -lrt

DATE		= `date +%Y-%m-%d-%H-%M`

ifdef DEBUG
	CFLAGS		+= -DUSE_DEBUG -g3 
else
	CFLAGS		+= -O3
endif

all : $(DAEMON)

libevent/% : 
	cd libevent; CFLAGS=$(CFLAGS) ./configure; make

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(DAEMON): $(DAEMON_O) $(DAEMON_INC) $(DAEMON).c
	$(CC) $(LDFLAGS) $(CFLAGS) $(DAEMON_L) $(DAEMON_O) $(DAEMON_INC) $(DAEMON).c -o $(DAEMON)

pristine: clean
	cd libevent; make clean

clean:
	rm -f $(DAEMON).tar.gz
	rm -f *.o $(DAEMON)
#	rm -f *~

tar: pristine
	tar zcf $(DAEMON).tar.gz --exclude=search.txt --exclude=.svn *
