#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)

/* Return the PROPERTY of "obj".
 */
__isl_keep FIELD_TYPE *FN(FN(TYPE,peek),PROPERTY)(__isl_keep TYPE *obj)
{
	if (!obj)
		return NULL;
	return obj->FIELD_NAME;
}
