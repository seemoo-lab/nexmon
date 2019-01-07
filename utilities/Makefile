SUBDIRS := $(wildcard */.)

all: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

install:
	for dir in $(SUBDIRS); do \
		make -C $$dir install; \
	done

.PHONY: all $(SUBDIRS)
