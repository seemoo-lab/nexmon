include ../makefile.conf
NAME=hello
STARTUP_DEFS=

LFLAGS=$(USE_NANO) $(USE_SEMIHOST) $(GC) $(MAP)

$(NAME)-$(CORE).axf: $(NAME).c
	$(CC) $^ $(CFLAGS) $(LFLAGS) -o $@

clean: 
	rm -f $(NAME)*.axf *.map
