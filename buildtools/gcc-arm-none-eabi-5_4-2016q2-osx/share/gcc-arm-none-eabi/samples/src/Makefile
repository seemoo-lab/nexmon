subdirs=$(shell find . -mindepth 1 -maxdepth 1 -type d)
all:
	for d in $(subdirs); do make -C $$d || exit 1; done

clean:
	for d in $(subdirs); do make -C $$d $@; done
