###
# Configuration Options
###

###
# Local Settings
###
CC			= gcc
CFLAGS		= -Wall -fno-strict-aliasing -I/usr/local/include -L/usr/local/lib

DAEMON		= usersearch
DAEMON_O	= tqueue.o search.o
DAEMON_L	= -lpthread # -levent

DATE		= `date +%Y-%m-%d-%H-%M`

ifdef DEBUG
	CFLAGS		+= -DDEBUG -g3 
else
	CFLAGS		+= -O2
endif

all : $(DAEMON)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(DAEMON): $(DAEMON_O)
	$(CC) $(LDFLAGS) $(CFLAGS) $(DAEMON_L) $(DAEMON_O) $(DAEMON).c -o $(DAEMON)

clean:
	rm -f *.o $(DAEMON)
#	rm -f *~
