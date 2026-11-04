#define xFN(TYPE,NAME) TYPE ## _ ## NAME
#define FN(TYPE,NAME) xFN(TYPE,NAME)

/* Set the PROPERTY of "obj" to "field",
 * where the PROPERTY of "obj" may be missing
 * due to a preceding call to TYPE_take_PROPERTY.
 * However, in this case, "obj" only has a single reference and
 * then the call to TYPE_cow has no effect.
 */
__isl_give TYPE *FN(FN(TYPE,restore),PROPERTY)(__isl_keep TYPE *obj,
	__isl_take FIELD_TYPE *field)
{
	if (!obj || !field)
		goto error;

	if (obj->FIELD_NAME == field) {
		FN(FIELD_TYPE,free)(field);
		return obj;
	}

	obj = FN(TYPE,cow)(obj);
	if (!obj)
		goto error;
	FN(FIELD_TYPE,free)(obj->FIELD_NAME);
	obj->FIELD_NAME = field;

	return obj;
error:
	FN(TYPE,free)(obj);
	FN(FIELD_TYPE,free)(field);
	return NULL;
}
