#define xCAT(A,B) A ## B
#define CAT(A,B) xCAT(A,B)
#undef EL
#define EL CAT(isl_,BASE)
#define xMULTI(BASE) isl_multi_ ## BASE
#define MULTI(BASE) xMULTI(BASE)

struct MULTI(BASE) {
	int ref;
	isl_space *space;

	int n;
	EL *p[1];
};

#define ISL_DECLARE_MULTI_PRIVATE(BASE)					\
__isl_give isl_multi_##BASE *isl_multi_##BASE##_alloc(			\
	__isl_take isl_space *space);

ISL_DECLARE_MULTI_PRIVATE(aff)
