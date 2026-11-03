#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)

/* Return the PROPERTY of "obj".
 * This may be either a copy or the PROPERTY itself
 * if there is only one reference to "obj".
 * This allows the PROPERTY to be modified inplace
 * if both the TYPE and its PROPERTY have only a single reference.
 * The caller is not allowed to modify "obj" between this call and
 * a subsequent call to TYPE_restore_PROPERTY.
 * The only exception is that TYPE_free can be called instead.
 */
__isl_give FIELD_TYPE *FN(FN(TYPE,take),PROPERTY)(__isl_keep TYPE *obj)
{
	FIELD_TYPE *field;

	if (!obj)
		return NULL;
	if (obj->ref != 1)
		return FN(FN(TYPE,get),PROPERTY)(obj);
	field = obj->FIELD_NAME;
	obj->FIELD_NAME = NULL;
	return field;
}
