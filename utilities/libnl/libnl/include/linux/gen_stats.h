#ifndef __LINUX_GEN_STATS_H
#define __LINUX_GEN_STATS_H

enum {
	TCA_STATS_UNSPEC,
	TCA_STATS_BASIC,
	TCA_STATS_RATE_EST,
	TCA_STATS_QUEUE,
	TCA_STATS_APP,
	__TCA_STATS_MAX,
};
#define TCA_STATS_MAX (__TCA_STATS_MAX - 1)

/**
 * @bytes: number of seen bytes
 * @packets: number of seen packets
 */
struct gnet_stats_basic
{
	__u64	bytes;
	__u32	packets;
};

/**
 * @bps: current byte rate
 * @pps: current packet rate
 */
struct gnet_stats_rate_est
{
	__u32	bps;
	__u32	pps;
};

/**
 * @qlen: queue length
 * @backlog: backlog size of queue
 * @drops: number of dropped packets
 * @requeues: number of requeues
 */
struct gnet_stats_queue
{
	__u32	qlen;
	__u32	backlog;
	__u32	drops;
	__u32	requeues;
	__u32	overlimits;
};

/**
 * @interval: sampling period
 * @ewma_log: the log of measurement window weight
 */
struct gnet_estimator
{
	signed char	interval;
	unsigned char	ewma_log;
};


#endif /* __LINUX_GEN_STATS_H */
