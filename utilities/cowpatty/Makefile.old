##################################
# <jwright> Well, I may be doing stupid things with make
# <jwright> OK, it was Makefile stupid'ness
# <jwright> I don't really understand what the hell I am doing with Make, I'm
#           just copying other files and seeing what works.
# <dragorn> heh
# <dragorn> i think thats all anyone does
# <dragorn> make is a twisted beast
##################################
LDLIBS		= -lpcap
CFLAGS		= -pipe -Wall -DOPENSSL 
CFLAGS		+= -O2
LDLIBS		+= -lcrypto
CFLAGS		+= -g3 -ggdb
#CFLAGS		+= -static
PROGOBJ		= md5.o sha1.o utils.o cowpatty.o genpmk.o
PROG		= cowpatty genpmk
BINDIR		= /usr/local/bin

all: $(PROGOBJ) $(PROG)

cowpatty: common.h md5.c md5.h sha1.h cowpatty.c cowpatty.h sha1.c \
            sha1.h utils.c utils.h
	$(CC) $(CFLAGS) cowpatty.c -o cowpatty utils.o md5.o sha1.o $(LDLIBS)

genpmk: genpmk.c cowpatty.h utils.h sha1.h common.h
	$(CC) $(CFLAGS) genpmk.c -o genpmk utils.o sha1.o $(LDLIBS)

utils: utils.c utils.h
	$(CC) $(CFLAGS) utils.c -c

md5: md5.c md5.h
	$(CC) $(CFLAGS) md5.c -c

sha1: sha1.c sha1.h
	$(CC) $(CFLAGS) sha1.c -c

clean:
	@rm $(PROGOBJ) $(PROG)

strip:
	@ls -l $(PROG)
	@strip $(PROG)
	@ls -l $(PROG)

install: all
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(PROG) $(DESTDIR)$(BINDIR)
