MDK_ROOT = ..
include $(MDK_ROOT)/common.mak

# TODO: Remove '-fcommon' from CFLAGS and address issues, reference:
# https://gcc.gnu.org/gcc-10/porting_to.html#common
CFLAGS		?= -g -O3 -Wall -Wextra -fcommon
LINKFLAGS	= -lpthread -lpcap $(LDFLAGS)

SBINDIR		?= $(PREFIX)/sbin
MANDIR		= $(PREFIX)/share/man

OSD		= osdep
LIBS		+= -lm -L$(OSD) -l$(OSD)
LIBOSD		= $(OSD)/lib$(OSD).so

OBJS		= debug.o helpers.o mac_addr.o linkedlist.o greylist.o dumpfile.o packet.o brute.o
OBJS_OSD	= osdep.o channelhopper.o ghosting.o fragmenting.o

ATTACKS		= attacks
OBJ_ATT		= $(shell ls attacks/*.h | sed s/"\.h"/"\.o"/g)

all: osd mdk4 $(OBJ_ATT) att

att:
	$(MAKE) -C $(ATTACKS)

osd:
	$(MAKE) -C $(OSD)

$(LIBOSD) $(OSD)/libosdep.a: osd

$(OBJ_ATT):
	$(MAKE) -C $(ATTACKS)

mdk4: mdk4.c $(OSD)/libosdep.a $(OBJS) $(OBJS_OSD) $(OBJ_ATT)
	$(MAKE) -C $(ATTACKS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(^) -o $(@) $(LIBS) $(LINKFLAGS)

test: test.c $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(^) -o $(@) $(LINKFLAGS)
	mv $(@) ..

install: mdk4
	install -D -m 0755 $^ $(DESTDIR)$(SBINDIR)/$^
	$(MAKE) -C $(ATTACKS) install
	$(MAKE) -C $(OSD) install
	mkdir -p $(DESTDIR)$(PREFIX)/share/mdk4
	cp -r pocs $(DESTDIR)$(PREFIX)/share/mdk4

.PHONY : clean
clean:
	rm -f mdk4
	rm -f $(OBJS) $(OBJS_OSD)
	rm -f ../test *.o
	$(MAKE) -C $(OSD) clean
	$(MAKE) -C $(ATTACKS) clean

distclean: clean
