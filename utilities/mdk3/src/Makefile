MDK_ROOT = ..
include $(MDK_ROOT)/common.mak

CFLAGS		+= -g -O3 -Wall -Wextra
LINKFLAGS	= -lpthread -lpcap $(LDFLAGS)

SBINDIR		= $(PREFIX)/sbin
MANDIR		= $(PREFIX)/share/man

OSD		= osdep
LIBS		+= -lm -L$(OSD) -l$(OSD)
LIBOSD		= $(OSD)/lib$(OSD).so

OBJS		= debug.o helpers.o mac_addr.o linkedlist.o greylist.o dumpfile.o packet.o brute.o
OBJS_OSD	= osdep.o channelhopper.o ghosting.o fragmenting.o

ATTACKS		= attacks
OBJ_ATT		= $(shell ls attacks/*.h | sed s/"\.h"/"\.o"/g)

all: osd mdk3 $(OBJ_ATT) att

att:
	$(MAKE) -C $(ATTACKS)

osd:
	$(MAKE) -C $(OSD)

$(LIBOSD):
	$(MAKE) -C $(OSD)

$(OBJ_ATT):
	$(MAKE) -C $(ATTACKS)

mdk3: mdk3.c $(OSD)/libosdep.a $(OBJS) $(OBJS_OSD) $(OBJ_ATT)
	$(MAKE) -C $(ATTACKS)
	$(CC) $(CFLAGS) $(^) -o $(@) $(LIBS) $(LINKFLAGS)

test: test.c $(OBJS)
	$(CC) $(CFLAGS) $(^) -o $(@) $(LINKFLAGS)
	mv $(@) ..

install: mdk3
	install -D -m 0755 $^ $(SBINDIR)/$^
	$(MAKE) -C $(ATTACKS) install
	$(MAKE) -C $(OSD) install

.PHONY : clean
clean:
	rm -f mdk3
	rm -f $(OBJS) $(OBJS_OSD)
	rm -f ../test *.o
	$(MAKE) -C $(OSD) clean
	$(MAKE) -C $(ATTACKS) clean

distclean: clean
