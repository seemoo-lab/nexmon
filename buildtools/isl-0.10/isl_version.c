#include "gitversion.h"

const char *isl_version(void)
{
	return GIT_HEAD_ID"\n";
}
