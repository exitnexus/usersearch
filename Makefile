###
# Configuration Options
###

###
# Local Settings
###
DATE		= `date +%Y-%m-%d-%H-%M`
UNAME       = $(shell uname)

CC			= g++
CFLAGS		= -Wall -fno-strict-aliasing -I/usr/local/include -L/usr/local/lib -Ilibevent -Ilibevent/compat -ansi

DAEMON		= usersearch

DAEMON_O	= search.o usersearch.o
DAEMON_INC  = \
	libevent/.libs/buffer.o \
	libevent/.libs/evbuffer.o \
	libevent/.libs/event.o \
	libevent/.libs/event_tagging.o \
	libevent/.libs/evutil.o \
	libevent/.libs/http.o \
	libevent/.libs/log.o \
	libevent/.libs/poll.o \
	libevent/.libs/select.o \
	libevent/.libs/signal.o \
	libevent/.libs/strlcpy.o
	
ifeq ($(UNAME),Darwin)
	DAEMON_INC += libevent/.libs/kqueue.o
else
	DAEMON_INC += libevent/.libs/epoll.o
endif

ifeq ($(UNAME),Darwin)
	DAEMON_L	= -lpthread
else
	DAEMON_L	= -lpthread -lrt
endif

ifdef DEBUG
	CFLAGS		+= -DUSE_DEBUG -O0 -g3 
else
	CFLAGS		+= -O3
endif

ifdef PROFILE
	CFLAGS		+= -pg
endif

all : $(DAEMON)

libevent/% : 
	echo "Compiling libevent ---------------------------------------------------"
	cd libevent; CFLAGS=$(CFLAGS) ./configure; make
	echo "Done libevent --------------------------------------------------------"

search.o : iterpair.h search.h usersetbase.h search.c
	$(CC) -c $(CFLAGS) search.c -o search.o

usersearch.o : iterpair.h search.h tqueue.h usersearch.c
	$(CC) -c $(CFLAGS) usersearch.c -o usersearch.o

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(DAEMON): $(DAEMON_INC) $(DAEMON_O)
	$(CC) $(LDFLAGS) $(CFLAGS) $(DAEMON_L) $(DAEMON_O) $(DAEMON_INC) -o $(DAEMON)

pristine: clean
	cd libevent; make clean
	
distclean: pristine

clean:
	rm -f $(DAEMON).tar.gz
	rm -f *.o $(DAEMON)
#	rm -f *~

tar: pristine
	tar zcf $(DAEMON).tar.gz --exclude=data* --exclude=.svn --exclude=callgrind.* *

ship: tar
	scp $(DAEMON).tar.gz master:/home/timo/

fresh: clean all

