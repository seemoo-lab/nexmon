#ifndef NETLINK_NEXTHOP_ENCAP_H_
#define NETLINK_NEXTHOP_ENCAP_H_

struct rtnl_nh_encap;

struct nh_encap_ops {
	uint16_t encap_type;

	int (*build_msg)(struct nl_msg *msg, void *priv);
	int (*parse_msg)(struct nlattr *nla, struct rtnl_nh_encap **encap_out);

	int (*compare)(void *a, void *b);
	void *(*clone)(void *priv);

	void (*dump)(void *priv, struct nl_dump_params *dp);
	void (*destructor)(void *priv);
};

/*
 * generic nexthop encap
 */
int nh_encap_parse_msg(struct nlattr *encap, struct nlattr *encap_type,
		       struct rtnl_nh_encap **encap_out);
int nh_encap_build_msg(struct nl_msg *msg, struct rtnl_nh_encap *rtnh_encap);

void nh_encap_dump(struct rtnl_nh_encap *rtnh_encap, struct nl_dump_params *dp);

int nh_encap_compare(struct rtnl_nh_encap *a, struct rtnl_nh_encap *b);

void *nh_encap_check_and_get_priv(struct rtnl_nh_encap *nh_encap,
				  uint16_t encap_type);

/*
 * MPLS encap
 */
extern const struct nh_encap_ops mpls_encap_ops;

/*
 * IPv6 encap
 */
extern const struct nh_encap_ops ip6_encap_ops;

/*
 * IPv4 encap
 */
extern const struct nh_encap_ops ip_encap_ops;

/*
 * ILA encap
 */
extern const struct nh_encap_ops ila_encap_ops;
#endif
