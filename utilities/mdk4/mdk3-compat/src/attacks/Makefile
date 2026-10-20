CFLAGS		= -g -O3 -Wall -Wextra
LINKFLAGS	= -lpthread

OBJS		= $(shell ls *.h | sed s/"\.h"/"\.o"/g)

all: $(OBJS)

install: $(OBJS)

.PHONY: clean
clean:
	rm -f $(OBJS) *.o

distclean: clean
