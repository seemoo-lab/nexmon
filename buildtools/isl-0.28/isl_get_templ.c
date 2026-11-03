#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)

/* Return a copy of the PROPERTY of "obj".
 */
__isl_give FIELD_TYPE *FN(FN(TYPE,get),PROPERTY)(__isl_keep TYPE *obj)
{
	return FN(FIELD_TYPE,copy)(FN(FN(TYPE,peek),PROPERTY)(obj));
}
