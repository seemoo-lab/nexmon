SUBDIRS := $(wildcard */.)

all: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

.PHONY: all $(SUBDIRS)
