SUBDIRS := $(wildcard */.)
SUBCLEAN = $(addsuffix .clean,$(SUBDIRS))

all: $(SUBDIRS)

$(SUBDIRS):
	make -C $@

clean: $(SUBCLEAN)

$(SUBCLEAN): %.clean:
	make -C $* clean
	rm -f ../../libs/armeabi/*.so
	rm -f ../../libs/armeabi/*.a


.PHONY: all $(SUBDIRS) clean $(SUBCLEAN)

