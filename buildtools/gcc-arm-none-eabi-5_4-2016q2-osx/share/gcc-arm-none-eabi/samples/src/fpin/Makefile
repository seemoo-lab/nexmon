include ../makefile.conf
NAME=fpin
STARTUP_DEFS=

LDSCRIPTS=-L. -L$(BASE)/ldscripts -T gcc.ld

# Must use -u _printf_float -u _scanf_float to use floating IO
LFLAGS=$(USE_NANO) $(USE_SEMIHOST) $(LDSCRIPTS) $(GC) $(MAP) \
  -u _printf_float -u _scanf_float

$(NAME)-$(CORE).axf: $(NAME).c $(STARTUP)
	$(CC) $^ $(CFLAGS) $(LFLAGS) -o $@

clean: 
	rm -f $(NAME)*.axf *.map
