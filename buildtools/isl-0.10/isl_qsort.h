#ifndef ISL_QSORT_H
#define ISL_QSORT_H

void isl_quicksort(void *const pbase, size_t total_elems, size_t size,
	int (*cmp)(const void *, const void *, void *arg), void *arg);

#endif
