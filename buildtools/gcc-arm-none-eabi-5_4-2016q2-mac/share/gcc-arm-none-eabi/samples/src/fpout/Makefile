include ../makefile.conf
NAME=fpout
STARTUP_DEFS=

LDSCRIPTS=-L. -L$(BASE)/ldscripts -T gcc.ld

# Must use -u _printf_float to use floating IO
LFLAGS=$(USE_NANO) $(USE_SEMIHOST) $(LDSCRIPTS) $(GC) $(MAP) -u _printf_float

$(NAME)-$(CORE).axf: $(NAME).c $(STARTUP)
	$(CC) $^ $(CFLAGS) $(LFLAGS) -o $@

clean: 
	rm -f $(NAME)*.axf $(NAME)*.map
