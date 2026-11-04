#ifndef ISL_AFF_PRIVATE_H
#define ISL_AFF_PRIVATE_H

#include <isl/aff.h>
#include <isl/vec.h>
#include <isl/mat.h>
#include <isl/local_space.h>
#include <isl_reordering.h>

/* ls represents the domain space.
 */
struct isl_aff {
	int ref;

	isl_local_space	*ls;
	isl_vec		*v;
};

struct isl_pw_aff_piece {
	struct isl_set *set;
	struct isl_aff *aff;
};

struct isl_pw_aff {
	int ref;

	isl_space *dim;

	int n;

	size_t size;
	struct isl_pw_aff_piece p[1];
};

struct isl_pw_multi_aff_piece {
	isl_set *set;
	isl_multi_aff *maff;
};

struct isl_pw_multi_aff {
	int ref;

	isl_space *dim;

	int n;

	size_t size;
	struct isl_pw_multi_aff_piece p[1];
};

__isl_give isl_aff *isl_aff_alloc(__isl_take isl_local_space *ls);

__isl_give isl_aff *isl_aff_reset_space_and_domain(__isl_take isl_aff *aff,
	__isl_take isl_space *space, __isl_take isl_space *domain);
__isl_give isl_aff *isl_aff_reset_domain_space(__isl_take isl_aff *aff,
	__isl_take isl_space *dim);
__isl_give isl_aff *isl_aff_realign_domain(__isl_take isl_aff *aff,
	__isl_take isl_reordering *r);

__isl_give isl_aff *isl_aff_normalize(__isl_take isl_aff *aff);

__isl_give isl_aff *isl_aff_expand_divs( __isl_take isl_aff *aff,
	__isl_take isl_mat *div, int *exp);

__isl_give isl_pw_aff *isl_pw_aff_alloc_size(__isl_take isl_space *space,
	int n);
__isl_give isl_pw_aff *isl_pw_aff_reset_space(__isl_take isl_pw_aff *pwaff,
	__isl_take isl_space *dim);
__isl_give isl_pw_aff *isl_pw_aff_reset_domain_space(
	__isl_take isl_pw_aff *pwaff, __isl_take isl_space *space);
__isl_give isl_pw_aff *isl_pw_aff_add_disjoint(
	__isl_take isl_pw_aff *pwaff1, __isl_take isl_pw_aff *pwaff2);

__isl_give isl_pw_aff *isl_pw_aff_union_opt(__isl_take isl_pw_aff *pwaff1,
	__isl_take isl_pw_aff *pwaff2, int max);

#undef BASE
#define BASE aff

#include <isl_multi_templ.h>

__isl_give isl_multi_aff *isl_multi_aff_dup(__isl_keep isl_multi_aff *multi);
__isl_give isl_multi_aff *isl_multi_aff_align_params(
	__isl_take isl_multi_aff *multi, __isl_take isl_space *model);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_reset_domain_space(
	__isl_take isl_pw_multi_aff *pwmaff, __isl_take isl_space *space);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_reset_space(
	__isl_take isl_pw_multi_aff *pwmaff, __isl_take isl_space *space);
__isl_give isl_pw_multi_aff *isl_pw_multi_aff_add_disjoint(
	__isl_take isl_pw_multi_aff *pma1, __isl_take isl_pw_multi_aff *pma2);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_project_out(
	__isl_take isl_pw_multi_aff *pma,
	enum isl_dim_type type, unsigned first, unsigned n);

__isl_give isl_pw_multi_aff *isl_pw_multi_aff_substitute(
	__isl_take isl_pw_multi_aff *pma, enum isl_dim_type type, unsigned pos,
	__isl_keep isl_pw_aff *subs);

#endif
