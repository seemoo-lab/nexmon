include ../makefile.conf
NAME=multiram
STARTUP_DEFS=-D__NO_SYSTEM_INIT

CXXFLAGS+=-D__STARTUP_COPY_MULTIPLE -D__STARTUP_CLEAR_BSS_MULTIPLE
LDSCRIPTS=-L. -L$(BASE)/ldscripts -T multi-ram.ld
LFLAGS=$(USE_NANO) $(USE_SEMIHOST) $(LDSCRIPTS) $(GC) $(MAP)

$(NAME)-$(CORE).axf: $(NAME).cc $(STARTUP)
	$(CXX) $^ $(CXXFLAGS) -fno-exceptions $(LFLAGS) -o $@

clean: 
	rm -f $(NAME)*.axf *.map
