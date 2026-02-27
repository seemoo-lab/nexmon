#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)
#define xLIST(EL) EL ## _list
#define LIST(EL) xLIST(EL)

struct LIST(EL) {
	int ref;
	isl_ctx *ctx;

	int n;

	size_t size;
	struct EL *p[1];
};

#define ISL_DECLARE_LIST_PRIVATE(EL)					\
__isl_give isl_##EL##_list *isl_##EL##_list_dup(			\
	__isl_keep isl_##EL##_list *list);

ISL_DECLARE_LIST_PRIVATE(basic_set)
ISL_DECLARE_LIST_PRIVATE(set)
ISL_DECLARE_LIST_PRIVATE(aff)
ISL_DECLARE_LIST_PRIVATE(pw_aff)
ISL_DECLARE_LIST_PRIVATE(band)
