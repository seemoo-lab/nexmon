provider gobject {
	probe type__new(char *, uintmax_t, uintmax_t);
	probe object__new(void*, uintmax_t);
	probe object__ref(void*, uintmax_t, int);
	probe object__unref(void*, uintmax_t, int);
	probe object__dispose(void*, uintmax_t, unsigned int);
	probe object__dispose__end(void*, uintmax_t, unsigned int);
	probe object__finalize(void*, uintmax_t);
	probe object__finalize__end(void*, uintmax_t);
	probe signal__new(unsigned int, char *, uintmax_t);
	probe signal__emit(unsigned int, unsigned int, void *, uintmax_t);
	probe signal__emit__end(unsigned int, unsigned int, void *, uintmax_t);
};
