// SPDX-License-Identifier: ISC
/*
 * Copyright (c) 2010 Broadcom Corporation
 */

#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <net/cfg80211.h>
#include <net/rtnetlink.h>
#include <net/addrconf.h>
#include <net/ieee80211_radiotap.h>
#include <net/ipv6.h>
#include <brcmu_utils.h>
#include <brcmu_wifi.h>

#include "core.h"
#include "bus.h"
#include "debug.h"
#include "fwil_types.h"
#include "p2p.h"
#include "pno.h"
#include "cfg80211.h"
#include "fwil.h"
#include "feature.h"
#include "proto.h"
#include "pcie.h"
#include "common.h"

/* NEXMON */
#include <linux/if_arp.h>
#include <linux/netlink.h>
#include <net/genetlink.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include "nexmon_ioctls.h"
#include "defs.h"
#include "sdio.h"
#include <linux/mmc/sdio_func.h>
#include <brcm_hw_ids.h>

/* NEXMON */
#define NETLINK_USER                     31
#define NEXUDP_IOCTL                      0

/* NEXMON Generic Netlink CSI streaming definitions */
#define NEXMON_CSI_GENL_NAME "nexmon_csi"
#define NEXMON_CSI_GENL_VERSION 1
#define NEXMON_CSI_GENL_MCGRP_NAME "csi"

enum {
	NEXMON_CSI_CMD_UNSPEC,
	NEXMON_CSI_CMD_SAMPLE,
	__NEXMON_CSI_CMD_MAX,
};
#define NEXMON_CSI_CMD_MAX (__NEXMON_CSI_CMD_MAX - 1)

enum {
	NEXMON_CSI_ATTR_UNSPEC,
	NEXMON_CSI_ATTR_RAW,
	NEXMON_CSI_ATTR_TSFT,
	NEXMON_CSI_ATTR_IFINDEX,
	NEXMON_CSI_ATTR_PAD,
	__NEXMON_CSI_ATTR_MAX,
};
#define NEXMON_CSI_ATTR_MAX (__NEXMON_CSI_ATTR_MAX - 1)

enum {
	NEXMON_CSI_MCGRP_CSI,
};

static const struct genl_multicast_group nexmon_csi_mcgrps[] = {
	[NEXMON_CSI_MCGRP_CSI] = { .name = NEXMON_CSI_GENL_MCGRP_NAME },
};

static const struct nla_policy nexmon_csi_genl_policy[NEXMON_CSI_ATTR_MAX + 1] = {
	[NEXMON_CSI_ATTR_RAW] = { .type = NLA_BINARY, .len = 4096 },
	[NEXMON_CSI_ATTR_TSFT] = { .type = NLA_U64 },
	[NEXMON_CSI_ATTR_IFINDEX] = { .type = NLA_U32 },
};

static struct genl_family nexmon_csi_gnl_family = {
	.name = NEXMON_CSI_GENL_NAME,
	.version = NEXMON_CSI_GENL_VERSION,
	.maxattr = NEXMON_CSI_ATTR_MAX,
	.policy = nexmon_csi_genl_policy,
	.netnsok = true,
	.module = THIS_MODULE,
	.mcgrps = nexmon_csi_mcgrps,
	.n_mcgrps = ARRAY_SIZE(nexmon_csi_mcgrps),
	.resv_start_op = NEXMON_CSI_CMD_MAX + 1,
};
static bool nexmon_csi_gnl_registered;

#define MONITOR_DISABLED  0
#define MONITOR_IEEE80211 1
#define MONITOR_RADIOTAP  2
#define MONITOR_LOG_ONLY  3
#define MONITOR_DROP_FRM  4
#define MONITOR_IPV4_UDP  5

#define MAX_WAIT_FOR_8021X_TX			msecs_to_jiffies(950)

#define BRCMF_BSSIDX_INVALID			-1

#define	RXS_PBPRES				BIT(2)

#define	D11_PHY_HDR_LEN				6

struct d11rxhdr_le {
	__le16 RxFrameSize;
	u16 PAD;
	__le16 PhyRxStatus_0;
	__le16 PhyRxStatus_1;
	__le16 PhyRxStatus_2;
	__le16 PhyRxStatus_3;
	__le16 PhyRxStatus_4;
	__le16 PhyRxStatus_5;
	__le16 RxStatus1;
	__le16 RxStatus2;
	__le16 RxTSFTime;
	__le16 RxChan;
	u8 unknown[12];
} __packed;

struct wlc_d11rxhdr {
	struct d11rxhdr_le rxhdr;
	__le32 tsf_l;
	s8 rssi;
	s8 rxpwr0;
	s8 rxpwr1;
	s8 do_rssi_ma;
	s8 rxpwr[4];
} __packed;

struct brcmf_mon_radiotap {
	struct ieee80211_radiotap_header hdr;
	__le64 tsft;
	u8 flags;
	u8 pad;
	__le16 channel_freq;
	__le16 channel_flags;
	s8 dbm_antsignal;
	u8 pad2;
} __packed;

 /*NEXMON*/
static struct netlink_kernel_cfg cfg = {0};
static struct sock *nl_sock = NULL;
/* ifindex of the primary interface used by legacy requests.  Storing an
 * ifindex rather than a net_device pointer lets the receive path acquire a
 * real device reference while holding RTNL, and therefore prevents detach
 * from racing a firmware command. */
static int nexmon_default_ifindex;

/* Primary-interface radio settings captured when monitor mode is enabled so
 * brcmf_net_mon_stop() can put them back.  Monitor mode drives these
 * chip-globally on iflist[0], so a single saved snapshot matches the driver's
 * single-radio scope (the same scope as nexmon_default_ifindex above).
 *
 * The snapshot is per-field: a firmware that rejects one query must not stop
 * the others from being restored.  TX power is stored as the raw "qtxpower"
 * iovar value (quarter-dBm plus the WL_TXPWR_OVERRIDE bit), not a dBm
 * conversion, so restore is byte-exact and keeps the override state.  The
 * legacy BRCMF_C_{GET,SET}_TXPWR DCMDs (65/66) are not used here: current
 * brcmfmac firmware answers them with -EBADE (-52).
 * State is stored per-device in drvr->nexmon_saved_* to support multi-adapter setups. */

static const struct net_device_ops brcmf_netdev_ops_pri;
static const struct net_device_ops brcmf_netdev_ops_mon;

 struct nexudp_header {
    char nex[3];
    char type;
    int securitycookie;
} __attribute__((packed));

 struct nexudp_ioctl_header {
    struct nexudp_header nexudphdr;
    unsigned int cmd;
    unsigned int set;
    char payload[1];
} __attribute__((packed));

/*
 * NEXMON netlink command handler (protocol NETLINK_USER == 31).
 *
 * Trust model / known limitations:
 *  - Access is gated by a single blanket CAP_NET_ADMIN check
 *    (netlink_capable below); any CAP_NET_ADMIN process may forward *any*
 *    Broadcom DCMD to the firmware. There is intentionally no per-command
 *    allowlist here -- callers with CAP_NET_ADMIN are already trusted to
 *    reconfigure the device. If a tighter policy is ever needed, add a
 *    static is_allowed_dcmd(frame->cmd) check before the
 *    brcmf_fil_cmd_data_{set,get}() calls below.
 *  - A single module-global socket (nl_sock) and one primary ifindex
 *    (nexmon_default_ifindex) are used, so this interface is not
 *    multi-radio / network-namespace aware. Reworking that would mean
 *    moving to Generic Netlink or an nl80211 vendor command; out of scope
 *    for the current single-radio target.
 */
 static void
nexmon_nl_ioctl_handler(struct sk_buff *skb)
{
    const size_t header_len = offsetof(struct nexudp_ioctl_header, payload);
    struct nlmsghdr *nlh;
    struct nexudp_ioctl_header *frame = NULL;
    struct net_device *ndev = NULL;
    struct net_device *target_ndev = NULL;
    struct brcmf_if *ifp;
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh_tx;
    size_t frame_len;
    size_t payload_len;
    size_t reply_len;
    u32 portid;
    u32 seq;
    int ifindex;
    int err;

    if (!netlink_capable(skb, CAP_NET_ADMIN)) {
        pr_err_ratelimited("brcmfmac: NEXMON: netlink request without CAP_NET_ADMIN\n");
        return;
    }

    if (skb->len < NLMSG_HDRLEN)
        return;

    if (skb_is_nonlinear(skb) && skb_linearize(skb))
        return;

    nlh = nlmsg_hdr(skb);
    if (!nlmsg_ok(nlh, skb->len)) {
        pr_err_ratelimited("brcmfmac: NEXMON: malformed netlink header\n");
        return;
    }

    frame_len = nlmsg_len(nlh);
    if (frame_len < header_len ||
        frame_len > header_len + BRCMF_DCMD_MAXLEN) {
        pr_err_ratelimited("brcmfmac: NEXMON: invalid request length %zu\n",
                           frame_len);
        netlink_ack(skb, nlh, -EINVAL, NULL);
        return;
    }

    frame = kmalloc(frame_len, GFP_KERNEL);
    if (!frame) {
        netlink_ack(skb, nlh, -ENOMEM, NULL);
        return;
    }
    memcpy(frame, nlmsg_data(nlh), frame_len);

    payload_len = frame_len - header_len;
    portid = NETLINK_CB(skb).portid;
    seq = nlh->nlmsg_seq;

    if (memcmp(frame->nexudphdr.nex, "NEX", 3)) {
        pr_err_ratelimited("brcmfmac: NEXMON: invalid nexudp_ioctl_header\n");
        netlink_ack(skb, nlh, -EINVAL, NULL);
        goto out_free;
    }

    if (frame->nexudphdr.type != NEXUDP_IOCTL || frame->set > 1) {
        pr_err_ratelimited("brcmfmac: NEXMON: invalid frame type\n");
        netlink_ack(skb, nlh, -EINVAL, NULL);
        goto out_free;
    }

    /* Netlink does not use the UDP security cookie.  New clients put the
     * target ifindex in that field; zero retains compatibility with older
     * clients by selecting the primary interface. */
    ifindex = frame->nexudphdr.securitycookie;
    if (ifindex <= 0)
        ifindex = READ_ONCE(nexmon_default_ifindex);
    if (ifindex <= 0) {
        pr_err_ratelimited("brcmfmac: NEXMON: no default interface, bus not ready\n");
        netlink_ack(skb, nlh, -ENODEV, NULL);
        goto out_free;
    }

    rtnl_lock();
    ndev = dev_get_by_index(&init_net, ifindex);
    if (!ndev) {
        rtnl_unlock();
        pr_err_ratelimited("brcmfmac: NEXMON: interface index %d not found\n",
                           ifindex);
        netlink_ack(skb, nlh, -ENODEV, NULL);
        goto out_free;
    }

    if (ndev->netdev_ops != &brcmf_netdev_ops_pri &&
        ndev->netdev_ops != &brcmf_netdev_ops_mon) {
        rtnl_unlock();
        pr_err_ratelimited("brcmfmac: NEXMON: interface index %d is not a brcmfmac device\n",
                           ifindex);
        netlink_ack(skb, nlh, -ENODEV, NULL);
        goto out_put;
    }

    ifp = netdev_priv(ndev);

    if (ifp == NULL || ifp->drvr == NULL || ifp->drvr->bus_if == NULL) {
        rtnl_unlock();
        pr_err_ratelimited("brcmfmac: NEXMON: interface torn down\n");
        netlink_ack(skb, nlh, -ENODEV, NULL);
        goto out_put;
    }

    if (ifp->drvr->bus_if->state != BRCMF_BUS_UP) {
        rtnl_unlock();
        pr_err_ratelimited("brcmfmac: NEXMON: bus not UP (state=%d), ignoring ioctl\n",
                           ifp->drvr->bus_if->state);
        netlink_ack(skb, nlh, -ENETDOWN, NULL);
        goto out_put;
    }

    struct brcmf_if *target_ifp = ifp;

    /* Monitor and promiscuous mode commands and chip-global variables are directed to primary BSS */
    if (frame->cmd == 108 /* WLC_SET_MONITOR */ || frame->cmd == 107 /* WLC_GET_MONITOR */ ||
        frame->cmd == 10  /* WLC_SET_PROMISC */ || frame->cmd == 9   /* WLC_GET_PROMISC */ ||
        frame->cmd == 30  /* WLC_SET_CHANNEL */ || frame->cmd == 29  /* WLC_GET_CHANNEL */ ||
        frame->cmd == 262 /* WLC_GET_VAR */     || frame->cmd == 263 /* WLC_SET_VAR */ ||
        frame->cmd == NEX_GET_CHANSPEC_OVERRIDE || frame->cmd == NEX_SET_CHANSPEC_OVERRIDE ||
        frame->cmd == NEX_READ_D11_SHM || frame->cmd == NEX_WRITE_D11_SHM ||
        frame->cmd == NEX_READ_TEMPLATE_RAM || frame->cmd == NEX_WRITE_TEMPLATE_RAM) {
        struct brcmf_if *pri_ifp = ifp->drvr ? ifp->drvr->iflist[0] : NULL;

        /*
         * The redirect targets a different ifp than the one our ndev
         * reference pins, so take a reference on its netdev as well. rtnl is
         * still held here, which makes reading iflist[0] and the dev_hold()
         * atomic with respect to brcmf_detach()/brcmf_remove_interface().
         * Without that reference the primary ifp can be freed underneath the
         * blocking DCMD below (up to DCMD_RESP_TIMEOUT), which is a
         * use-after-free when the wedge watchdog resets the bus while a
         * capture tool keeps issuing WLC_GET_VAR/WLC_SET_VAR.
         *
         * If the primary ifp has no netdev to pin it is mid-teardown; leave
         * target_ifp pointing at the pinned ifp so the DCMD fails cleanly
         * rather than running on an unpinnable interface.
         */
        if (pri_ifp && pri_ifp->ndev) {
            target_ifp = pri_ifp;
            target_ndev = pri_ifp->ndev;
            dev_hold(target_ndev);
        }
    }
    rtnl_unlock();

    if (frame->cmd == NEX_SET_CHANSPEC_OVERRIDE) {
        u16 cs = 0;
        if (payload_len >= sizeof(u16))
            cs = le16_to_cpu(*(u16 *)frame->payload);
        if (cs == 0 && payload_len >= sizeof(u32))
            cs = (u16)le32_to_cpu(*(u32 *)frame->payload);

        if (cs) {
            /* Fast-path channel hopping (sub-2ms): bypass cfg80211 locks,
             * association checks, and regulatory scans. Directly set the
             * chanspec on the radio hardware and update monitor state. */
            err = brcmf_fil_iovar_int_set(target_ifp, "chanspec", cs);
            if (!err)
                nexmon_mon_restore_set_chanspec(cs);
        } else {
            err = -EINVAL;
        }

        if (err) {
            if (err == -EBADE)
                bphy_err_ratelimited(ifp->drvr,
                                     "NEXMON: Fast channel hop to 0x%04x failed (-EBADE: disallowed by regulatory CLM)\n",
                                     cs);
            else
                bphy_err_ratelimited(ifp->drvr,
                                     "NEXMON: Fast channel hop to 0x%04x failed: %d\n",
                                     cs, err);
            netlink_ack(skb, nlh, err < 0 ? err : -err, NULL);
            goto out_put;
        }

        skb_out = nlmsg_new(4, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, 4, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), "ACK", 4);
        nlmsg_unicast(nl_sock, skb_out, portid);
        goto out_put;
    }

    if (frame->cmd == NEX_GET_CHANSPEC_OVERRIDE) {
        u32 cs_val = (u32)nexmon_mon_restore_get_chanspec();
        if (!cs_val) {
            s32 fw_cs = 0;
            if (!brcmf_fil_iovar_int_get(target_ifp, "chanspec", &fw_cs))
                cs_val = (u32)(u16)fw_cs;
        }

        if (payload_len >= sizeof(u32))
            *(__le32 *)frame->payload = cpu_to_le32(cs_val);
        else if (payload_len >= sizeof(u16))
            *(__le16 *)frame->payload = cpu_to_le16((u16)cs_val);

        skb_out = nlmsg_new(frame_len, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, frame_len, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), frame, frame_len);
        nlmsg_unicast(nl_sock, skb_out, portid);
        goto out_put;
    }

    if (frame->cmd == NEX_READ_D11_SHM) {
        struct nex_d11_shm_req req;
        struct brcmf_bus *bus_if = target_ifp->drvr ? target_ifp->drvr->bus_if : NULL;
        struct brcmf_sdio_dev *sdiodev = NULL;
        u32 target_addr;
        int ret = 0;

        if (payload_len < sizeof(struct nex_d11_shm_req)) {
            netlink_ack(skb, nlh, -EINVAL, NULL);
            goto out_put;
        }

        memcpy(&req, frame->payload, sizeof(req));
        req.offset = le32_to_cpu(req.offset);
        req.width = le16_to_cpu(req.width);
        req.is_reg = le16_to_cpu(req.is_reg);
        if (req.width != 4)
            req.width = 2;

        if (req.offset >= 0xE8004000 && req.offset < 0xE8008000) {
            req.offset -= 0xE8004000;
            req.is_reg = 0;
        } else if (req.offset >= 0x18001800 && req.offset < 0x18002000) {
            req.offset -= 0x18001800;
            req.is_reg = 3;
        } else if (req.offset >= 0x18001000 && req.offset < 0x18001800) {
            req.offset -= 0x18001000;
            req.is_reg = 1;
        } else if (req.offset >= 0x18000000) {
            req.is_reg = 2;
        }

        if (bus_if && bus_if->bus_priv.sdio)
            sdiodev = bus_if->bus_priv.sdio;

        if (!sdiodev || !sdiodev->func1) {
            netlink_ack(skb, nlh, -EOPNOTSUPP, NULL);
            goto out_put;
        }

        if (sdiodev->bus)
            brcmf_sdio_sleep(sdiodev->bus, false);

        sdio_claim_host(sdiodev->func1);
        if (req.is_reg == 3) {
            /* Template RAM read via hardware indirect registers (0x130/0x134) */
            brcmf_sdiod_writel(sdiodev, D11_TPLATEWRPTR_REG, req.offset & 0x7FC, &ret);
            if (!ret) {
                u32 raw = brcmf_sdiod_readl(sdiodev, D11_TPLATEWRDATA_REG, &ret);
                if (!ret) {
                    if (req.width == 4)
                        req.val = raw;
                    else
                        req.val = (req.offset & 2) ? ((raw >> 16) & 0xFFFF) : (raw & 0xFFFF);
                }
            }
        } else if (req.is_reg == 1 || req.is_reg == 2) {
            target_addr = (req.is_reg == 1) ? D11REG_ADDR(req.offset) : req.offset;
            if (req.width == 4) {
                req.val = brcmf_sdiod_readl(sdiodev, target_addr & ~3, &ret);
            } else {
                u32 raw = brcmf_sdiod_readl(sdiodev, target_addr & ~3, &ret);
                req.val = (req.offset & 2) ? ((raw >> 16) & 0xFFFF) : (raw & 0xFFFF);
            }
        } else {
            /* Broadcom SiliconBackplane D11 objaddr (0x160) / objdata (0x164) */
            brcmf_sdiod_writel(sdiodev, D11_OBJADDR_REG, OBJADDR_SHM_SEL | ((req.offset & 0xFFFC) >> 2), &ret);
            if (!ret) {
                (void)brcmf_sdiod_readl(sdiodev, D11_OBJADDR_REG, &ret);
                if (!ret) {
                    u32 raw = brcmf_sdiod_readl(sdiodev, D11_OBJDATA_REG, &ret);
                    if (!ret) {
                        if (req.width == 4)
                            req.val = raw;
                        else
                            req.val = (req.offset & 2) ? ((raw >> 16) & 0xFFFF) : (raw & 0xFFFF);
                    }
                }
            }
            if (ret) {
                /* Cypress AXI memory window fallback */
                ret = 0;
                target_addr = D11SHM_ADDR(req.offset);
                u32 raw = brcmf_sdiod_readl(sdiodev, target_addr & ~3, &ret);
                if (!ret) {
                    if (req.width == 4)
                        req.val = raw;
                    else
                        req.val = (req.offset & 2) ? ((raw >> 16) & 0xFFFF) : (raw & 0xFFFF);
                }
            }
        }
        sdio_release_host(sdiodev->func1);

        if (ret) {
            netlink_ack(skb, nlh, ret < 0 ? ret : -ret, NULL);
            goto out_put;
        }

        req.offset = cpu_to_le32(req.offset);
        req.val = cpu_to_le32(req.val);
        req.width = cpu_to_le16(req.width);
        req.is_reg = cpu_to_le16(req.is_reg);
        memcpy(frame->payload, &req, sizeof(req));

        skb_out = nlmsg_new(frame_len, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, frame_len, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), frame, frame_len);
        nlmsg_unicast(nl_sock, skb_out, portid);
        goto out_put;
    }

    if (frame->cmd == NEX_WRITE_D11_SHM) {
        struct nex_d11_shm_req req;
        struct brcmf_bus *bus_if = target_ifp->drvr ? target_ifp->drvr->bus_if : NULL;
        struct brcmf_sdio_dev *sdiodev = NULL;
        u32 target_addr;
        int ret = 0;

        if (payload_len < sizeof(struct nex_d11_shm_req)) {
            netlink_ack(skb, nlh, -EINVAL, NULL);
            goto out_put;
        }

        memcpy(&req, frame->payload, sizeof(req));
        req.offset = le32_to_cpu(req.offset);
        req.val = le32_to_cpu(req.val);
        req.width = le16_to_cpu(req.width);
        req.is_reg = le16_to_cpu(req.is_reg);
        if (req.width != 4)
            req.width = 2;

        if (req.offset >= 0xE8004000 && req.offset < 0xE8008000) {
            req.offset -= 0xE8004000;
            req.is_reg = 0;
        } else if (req.offset >= 0x18001800 && req.offset < 0x18002000) {
            req.offset -= 0x18001800;
            req.is_reg = 3;
        } else if (req.offset >= 0x18001000 && req.offset < 0x18001800) {
            req.offset -= 0x18001000;
            req.is_reg = 1;
        } else if (req.offset >= 0x18000000) {
            req.is_reg = 2;
        }

        if (bus_if && bus_if->bus_priv.sdio)
            sdiodev = bus_if->bus_priv.sdio;

        if (!sdiodev || !sdiodev->func1) {
            netlink_ack(skb, nlh, -EOPNOTSUPP, NULL);
            goto out_put;
        }

        if (sdiodev->bus)
            brcmf_sdio_sleep(sdiodev->bus, false);

        sdio_claim_host(sdiodev->func1);
        if (req.is_reg == 3) {
            /* Template RAM write via hardware indirect registers (0x130/0x134) */
            brcmf_sdiod_writel(sdiodev, D11_TPLATEWRPTR_REG, req.offset & 0x7FC, &ret);
            if (!ret)
                brcmf_sdiod_writel(sdiodev, D11_TPLATEWRDATA_REG, req.val, &ret);
        } else if (req.is_reg == 1 || req.is_reg == 2) {
            target_addr = (req.is_reg == 1) ? D11REG_ADDR(req.offset) : req.offset;
            if (req.width == 4) {
                brcmf_sdiod_writel(sdiodev, target_addr & ~3, req.val, &ret);
            } else {
                u32 raw = brcmf_sdiod_readl(sdiodev, target_addr & ~3, &ret);
                if (!ret) {
                    if (req.offset & 2)
                        raw = (raw & 0x0000FFFF) | ((req.val & 0xFFFF) << 16);
                    else
                        raw = (raw & 0xFFFF0000) | (req.val & 0xFFFF);
                    brcmf_sdiod_writel(sdiodev, target_addr & ~3, raw, &ret);
                }
            }
        } else {
            /* SHM write: SiliconBackplane objaddr (0x160) / objdata (0x164) */
            brcmf_sdiod_writel(sdiodev, D11_OBJADDR_REG, OBJADDR_SHM_SEL | ((req.offset & 0xFFFC) >> 2), &ret);
            if (!ret) {
                (void)brcmf_sdiod_readl(sdiodev, D11_OBJADDR_REG, &ret);
                if (!ret) {
                    if (req.width == 4) {
                        brcmf_sdiod_writel(sdiodev, D11_OBJDATA_REG, req.val, &ret);
                    } else {
                        u32 raw = brcmf_sdiod_readl(sdiodev, D11_OBJDATA_REG, &ret);
                        if (!ret) {
                            if (req.offset & 2)
                                raw = (raw & 0x0000FFFF) | ((req.val & 0xFFFF) << 16);
                            else
                                raw = (raw & 0xFFFF0000) | (req.val & 0xFFFF);
                            brcmf_sdiod_writel(sdiodev, D11_OBJADDR_REG, OBJADDR_SHM_SEL | ((req.offset & 0xFFFC) >> 2), &ret);
                            (void)brcmf_sdiod_readl(sdiodev, D11_OBJADDR_REG, &ret);
                            brcmf_sdiod_writel(sdiodev, D11_OBJDATA_REG, raw, &ret);
                        }
                    }
                }
            }
            if (ret) {
                /* Cypress AXI memory window fallback */
                ret = 0;
                target_addr = D11SHM_ADDR(req.offset);
                if (req.width == 4) {
                    brcmf_sdiod_writel(sdiodev, target_addr & ~3, req.val, &ret);
                } else {
                    u32 raw = brcmf_sdiod_readl(sdiodev, target_addr & ~3, &ret);
                    if (!ret) {
                        if (req.offset & 2)
                            raw = (raw & 0x0000FFFF) | ((req.val & 0xFFFF) << 16);
                        else
                            raw = (raw & 0xFFFF0000) | (req.val & 0xFFFF);
                        brcmf_sdiod_writel(sdiodev, target_addr & ~3, raw, &ret);
                    }
                }
            }
        }
        sdio_release_host(sdiodev->func1);

        if (ret) {
            netlink_ack(skb, nlh, ret < 0 ? ret : -ret, NULL);
            goto out_put;
        }

        skb_out = nlmsg_new(4, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, 4, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), "ACK", 4);
        nlmsg_unicast(nl_sock, skb_out, portid);
        goto out_put;
    }

    if (frame->cmd == NEX_READ_TEMPLATE_RAM) {
        struct nex_d11_templ_req req;
        struct brcmf_bus *bus_if = target_ifp->drvr ? target_ifp->drvr->bus_if : NULL;
        struct brcmf_sdio_dev *sdiodev = NULL;
        int ret = 0, i;
        u32 base_addr;
        size_t hdr_sz = sizeof(req.offset) + sizeof(req.len) + sizeof(req.method) + sizeof(req.pad);
        size_t resp_len;

        if (payload_len < sizeof(req.offset) + sizeof(req.len)) {
            netlink_ack(skb, nlh, -EINVAL, NULL);
            goto out_put;
        }

        memset(&req, 0, sizeof(req));
        memcpy(&req, frame->payload, min_t(size_t, payload_len, sizeof(req)));
        req.offset = le32_to_cpu(req.offset);
        req.len = le32_to_cpu(req.len);
        req.method = le16_to_cpu(req.method);
        if (req.len > sizeof(req.data))
            req.len = sizeof(req.data);
        if (req.offset + req.len > 0x800) {
            netlink_ack(skb, nlh, -EINVAL, NULL);
            goto out_put;
        }

        if (bus_if && bus_if->bus_priv.sdio)
            sdiodev = bus_if->bus_priv.sdio;

        if (!sdiodev || !sdiodev->func1) {
            netlink_ack(skb, nlh, -EOPNOTSUPP, NULL);
            goto out_put;
        }

        if (sdiodev->bus)
            brcmf_sdio_sleep(sdiodev->bus, false);

        sdio_claim_host(sdiodev->func1);
        if (req.method == 1) {
            base_addr = D11_TEMPLATE_RAM_BASE + (req.offset & 0x7FC);
            for (i = 0; i < req.len && !ret; i += 4) {
                u32 dword = brcmf_sdiod_readl(sdiodev, (base_addr + i) & ~3, &ret);
                int chunk = min_t(int, req.len - i, 4);
                memcpy(req.data + i, &dword, chunk);
            }
        }
        if (req.method == 0 || ret) {
            ret = 0;
            for (i = 0; i < req.len && !ret; i += 4) {
                u32 dword;
                int chunk = min_t(int, req.len - i, 4);
                brcmf_sdiod_writel(sdiodev, D11_TPLATEWRPTR_REG, (req.offset + i) & 0x7FC, &ret);
                if (ret)
                    break;
                dword = brcmf_sdiod_readl(sdiodev, D11_TPLATEWRDATA_REG, &ret);
                if (ret)
                    break;
                memcpy(req.data + i, &dword, chunk);
            }
        }
        sdio_release_host(sdiodev->func1);

        if (ret) {
            netlink_ack(skb, nlh, ret < 0 ? ret : -ret, NULL);
            goto out_put;
        }

        resp_len = hdr_sz + req.len;
        req.offset = cpu_to_le32(req.offset);
        req.len = cpu_to_le32(req.len);
        req.method = cpu_to_le16(req.method);
        req.pad = 0;
        memcpy(frame->payload, &req, resp_len);

        size_t resp_frame_len = header_len + resp_len;
        skb_out = nlmsg_new(resp_frame_len, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, resp_frame_len, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), frame, resp_frame_len);
        nlmsg_unicast(nl_sock, skb_out, portid);
        goto out_put;
    }

    if (frame->cmd == NEX_WRITE_TEMPLATE_RAM) {
        struct nex_d11_templ_req req;
        struct brcmf_bus *bus_if = target_ifp->drvr ? target_ifp->drvr->bus_if : NULL;
        struct brcmf_sdio_dev *sdiodev = NULL;
        int ret = 0, i;

        if (payload_len < sizeof(req.offset) + sizeof(req.len)) {
            netlink_ack(skb, nlh, -EINVAL, NULL);
            goto out_put;
        }

        memset(&req, 0, sizeof(req));
        memcpy(&req, frame->payload, min_t(size_t, payload_len, sizeof(req)));
        req.offset = le32_to_cpu(req.offset);
        req.len = le32_to_cpu(req.len);
        req.method = le16_to_cpu(req.method);
        if (req.len > sizeof(req.data) || req.offset + req.len > 0x800) {
            netlink_ack(skb, nlh, -EINVAL, NULL);
            goto out_put;
        }

        if (bus_if && bus_if->bus_priv.sdio)
            sdiodev = bus_if->bus_priv.sdio;

        if (!sdiodev || !sdiodev->func1) {
            netlink_ack(skb, nlh, -EOPNOTSUPP, NULL);
            goto out_put;
        }

        if (sdiodev->bus)
            brcmf_sdio_sleep(sdiodev->bus, false);

        sdio_claim_host(sdiodev->func1);
        if (req.method == 1) {
            /* Direct backplane write */
            u32 base_addr = D11_TEMPLATE_RAM_BASE + (req.offset & 0x7FC);
            for (i = 0; i < req.len && !ret; i += 4) {
                u32 dword = 0;
                int chunk = min_t(int, req.len - i, 4);
                memcpy(&dword, req.data + i, chunk);
                brcmf_sdiod_writel(sdiodev, (base_addr + i) & ~3, dword, &ret);
            }
        } else {
            /* Register-indirect write via tplatewrptr (0x130) and tplatewrdata (0x134) */
            for (i = 0; i < req.len && !ret; i += 4) {
                u32 dword = 0;
                int chunk = min_t(int, req.len - i, 4);
                memcpy(&dword, req.data + i, chunk);
                brcmf_sdiod_writel(sdiodev, D11_TPLATEWRPTR_REG, (req.offset + i) & 0x7FC, &ret);
                if (!ret)
                    brcmf_sdiod_writel(sdiodev, D11_TPLATEWRDATA_REG, dword, &ret);
            }
        }
        sdio_release_host(sdiodev->func1);

        if (ret) {
            netlink_ack(skb, nlh, ret < 0 ? ret : -ret, NULL);
            goto out_put;
        }

        skb_out = nlmsg_new(4, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, 4, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), "ACK", 4);
        nlmsg_unicast(nl_sock, skb_out, portid);
        goto out_put;
    }

    if (frame->set) {
        err = brcmf_fil_cmd_data_set(target_ifp, frame->cmd, frame->payload,
                                     payload_len);
        if (err) {
            /*
             * Ratelimited on purpose. A channel hopper sweeping channels the
             * regdomain rejects hits this once per hop (WLC_SET_VAR -> -EBADE),
             * and once the bus is down a dcmd fails instantly instead of taking
             * the 2.5s timeout - so an unsuppressed airodump-ng emits tens of
             * identical lines per millisecond and buries the watchdog's
             * "bus wedged, forcing reset" message, which is the one that
             * matters. ifp->drvr is validated non-NULL above.
             */
            if (err == -EBADE)
                bphy_err_ratelimited(ifp->drvr,
                                     "NEXMON: DCMD set %u failed: %d (-EBADE: disallowed by firmware regulatory CLM; check 'iw reg set <country>')\n",
                                     frame->cmd, err);
            else
                bphy_err_ratelimited(ifp->drvr,
                                     "NEXMON: DCMD set %u failed: %d\n",
                                     frame->cmd, err);
            netlink_ack(skb, nlh, err < 0 ? err : -err, NULL);
            goto out_put;
        }

        if (frame->cmd == 263 && payload_len >= 13 &&
            !strncmp(frame->payload, "chanspec", 8)) {
            u16 cs = *(u16 *)(frame->payload + 9);
            if (cs)
                nexmon_mon_restore_set_chanspec(cs);
        }

        skb_out = nlmsg_new(4, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, 4, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), "ACK", 4);
        nlmsg_unicast(nl_sock, skb_out, portid);
    } else {
        err = brcmf_fil_cmd_data_get(target_ifp, frame->cmd, frame->payload,
                                     payload_len);
        if (err) {
            /* Ratelimited - see the matching set path above for why. */
            bphy_err_ratelimited(ifp->drvr,
                                 "NEXMON: DCMD get %u failed: %d\n",
                                 frame->cmd, err);
            netlink_ack(skb, nlh, err < 0 ? err : -err, NULL);
            goto out_put;
        }

        reply_len = frame_len;
        skb_out = nlmsg_new(reply_len, GFP_KERNEL);
        if (!skb_out) {
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        nlh_tx = nlmsg_put(skb_out, 0, seq, NLMSG_DONE, reply_len, 0);
        if (!nlh_tx) {
            kfree_skb(skb_out);
            netlink_ack(skb, nlh, -ENOMEM, NULL);
            goto out_put;
        }
        memcpy(nlmsg_data(nlh_tx), frame, reply_len);
        nlmsg_unicast(nl_sock, skb_out, portid);
    }

out_put:
    if (target_ndev)
        dev_put(target_ndev);
    dev_put(ndev);
out_free:
    kfree(frame);
}

char *brcmf_ifname(struct brcmf_if *ifp)
{
	if (!ifp)
		return "<if_null>";

	if (ifp->ndev)
		return ifp->ndev->name;

	return "<if_none>";
}

struct brcmf_if *brcmf_get_ifp(struct brcmf_pub *drvr, int ifidx)
{
	struct brcmf_if *ifp;
	s32 bsscfgidx;

	if (ifidx < 0 || ifidx >= BRCMF_MAX_IFS) {
		bphy_err(drvr, "ifidx %d out of range\n", ifidx);
		return NULL;
	}

	ifp = NULL;
	bsscfgidx = drvr->if2bss[ifidx];
	if (bsscfgidx >= 0)
		ifp = drvr->iflist[bsscfgidx];

	return ifp;
}

void brcmf_configure_arp_nd_offload(struct brcmf_if *ifp, bool enable)
{
	s32 err;
	u32 mode;

	if (enable && ifp->drvr && ifp->drvr->wiphy &&
	    brcmf_is_apmode_operating(ifp->drvr->wiphy)) {
		brcmf_dbg(TRACE, "Skip ARP/ND offload enable when soft AP is running\n");
		return;
	}

	if (enable)
		mode = BRCMF_ARP_OL_AGENT | BRCMF_ARP_OL_PEER_AUTO_REPLY;
	else
		mode = 0;

	/* Try to set and enable ARP offload feature, this may fail, then it  */
	/* is simply not supported and err 0 will be returned                 */
	err = brcmf_fil_iovar_int_set(ifp, "arp_ol", mode);
	if (err) {
		brcmf_dbg(TRACE, "failed to set ARP offload mode to 0x%x, err = %d\n",
			  mode, err);
	} else {
		err = brcmf_fil_iovar_int_set(ifp, "arpoe", enable);
		if (err) {
			brcmf_dbg(TRACE, "failed to configure (%d) ARP offload err = %d\n",
				  enable, err);
		} else {
			brcmf_dbg(TRACE, "successfully configured (%d) ARP offload to 0x%x\n",
				  enable, mode);
		}
	}

	err = brcmf_fil_iovar_int_set(ifp, "ndoe", enable);
	if (err) {
		brcmf_dbg(TRACE, "failed to configure (%d) ND offload err = %d\n",
			  enable, err);
	} else {
		brcmf_dbg(TRACE, "successfully configured (%d) ND offload to 0x%x\n",
			  enable, mode);
	}
}

static void _brcmf_set_multicast_list(struct work_struct *work)
{
	struct brcmf_if *ifp = container_of(work, struct brcmf_if,
					    multicast_work);
	struct brcmf_pub *drvr = ifp->drvr;
	struct net_device *ndev;
	struct netdev_hw_addr *ha;
	u32 cmd_value, cnt;
	__le32 cnt_le;
	char *buf, *bufp;
	u32 buflen;
	s32 err;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	ndev = ifp->ndev;

	/* Determine initial value of allmulti flag */
	cmd_value = (ndev->flags & IFF_ALLMULTI) ? true : false;

	/* Send down the multicast list first. */
	cnt = netdev_mc_count(ndev);
	buflen = sizeof(cnt) + (cnt * ETH_ALEN);
	buf = kmalloc(buflen, GFP_KERNEL);
	if (!buf)
		return;
	bufp = buf;

	cnt_le = cpu_to_le32(cnt);
	memcpy(bufp, &cnt_le, sizeof(cnt_le));
	bufp += sizeof(cnt_le);

	netdev_for_each_mc_addr(ha, ndev) {
		if (!cnt)
			break;
		memcpy(bufp, ha->addr, ETH_ALEN);
		bufp += ETH_ALEN;
		cnt--;
	}

	err = brcmf_fil_iovar_data_set(ifp, "mcast_list", buf, buflen);
	if (err < 0) {
		bphy_err(drvr, "Setting mcast_list failed, %d\n", err);
		cmd_value = cnt ? true : cmd_value;
	}

	kfree(buf);

	/*
	 * Now send the allmulti setting.  This is based on the setting in the
	 * net_device flags, but might be modified above to be turned on if we
	 * were trying to set some addresses and dongle rejected it...
	 */
	err = brcmf_fil_iovar_int_set(ifp, "allmulti", cmd_value);
	if (err < 0)
		bphy_err(drvr, "Setting allmulti failed, %d\n", err);

	/*Finally, pick up the PROMISC flag */
	cmd_value = (ndev->flags & IFF_PROMISC) ? true : false;
	err = brcmf_fil_cmd_int_set(ifp, BRCMF_C_SET_PROMISC, cmd_value);
	if (err < 0) {
		/* PROMISC unsupported by firmware of older chips */
		if (err == -EBADE)
			bphy_info_once(drvr, "BRCMF_C_SET_PROMISC unsupported\n");
		else
			bphy_err(drvr, "Setting BRCMF_C_SET_PROMISC failed, err=%d\n",
				 err);
	}
	brcmf_configure_arp_nd_offload(ifp, !cmd_value);
}

#if IS_ENABLED(CONFIG_IPV6)
static void _brcmf_update_ndtable(struct work_struct *work)
{
	struct brcmf_if *ifp = container_of(work, struct brcmf_if,
					    ndoffload_work);
	struct brcmf_pub *drvr = ifp->drvr;
	int i, ret;

	/* clear the table in firmware */
	ret = brcmf_fil_iovar_data_set(ifp, "nd_hostip_clear", NULL, 0);
	if (ret) {
		brcmf_dbg(TRACE, "fail to clear nd ip table err:%d\n", ret);
		return;
	}

	for (i = 0; i < ifp->ipv6addr_idx; i++) {
		ret = brcmf_fil_iovar_data_set(ifp, "nd_hostip",
					       &ifp->ipv6_addr_tbl[i],
					       sizeof(struct in6_addr));
		if (ret)
			bphy_err(drvr, "add nd ip err %d\n", ret);
	}
}
#else
static void _brcmf_update_ndtable(struct work_struct *work)
{
}
#endif

static int brcmf_netdev_set_mac_address(struct net_device *ndev, void *addr)
{
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct sockaddr *sa = (struct sockaddr *)addr;
	int err;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	err = brcmf_c_set_cur_etheraddr(ifp, sa->sa_data);
	if (err >= 0) {
		brcmf_dbg(TRACE, "updated to %pM\n", sa->sa_data);
		memcpy(ifp->mac_addr, sa->sa_data, ETH_ALEN);
		eth_hw_addr_set(ifp->ndev, ifp->mac_addr);
	}
	return err;
}

static void brcmf_netdev_set_multicast_list(struct net_device *ndev)
{
	struct brcmf_if *ifp = netdev_priv(ndev);

	schedule_work(&ifp->multicast_work);
}

/**
 * brcmf_skb_is_iapp - checks if skb is an IAPP packet
 *
 * @skb: skb to check
 */
static bool brcmf_skb_is_iapp(struct sk_buff *skb)
{
	static const u8 iapp_l2_update_packet[6] __aligned(2) = {
		0x00, 0x01, 0xaf, 0x81, 0x01, 0x00,
	};
	unsigned char *eth_data;
#if !defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
	const u16 *a, *b;
#endif

	if (skb->len - skb->mac_len != 6 ||
	    !is_multicast_ether_addr(eth_hdr(skb)->h_dest))
		return false;

	eth_data = skb_mac_header(skb) + ETH_HLEN;
#if defined(CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS)
	return !(((*(const u32 *)eth_data) ^ (*(const u32 *)iapp_l2_update_packet)) |
		 ((*(const u16 *)(eth_data + 4)) ^ (*(const u16 *)(iapp_l2_update_packet + 4))));
#else
	a = (const u16 *)eth_data;
	b = (const u16 *)iapp_l2_update_packet;

	return !((a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2]));
#endif
}

static netdev_tx_t brcmf_netdev_start_xmit(struct sk_buff *skb,
					   struct net_device *ndev)
{
	int ret;
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_pub *drvr = ifp->drvr;
	struct ethhdr *eh;
	int head_delta;

	brcmf_dbg(DATA, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	/* Can the device send data? */
	if (drvr->bus_if->state != BRCMF_BUS_UP) {
		bphy_err(drvr, "xmit rejected state=%d\n", drvr->bus_if->state);
		netif_stop_queue(ndev);
		dev_kfree_skb(skb);
		ret = -ENODEV;
		goto done;
	}

	/* Some recent Broadcom's firmwares disassociate STA when they receive
	 * an 802.11f ADD frame. This behavior can lead to a local DoS security
	 * issue. Attacker may trigger disassociation of any STA by sending a
	 * proper Ethernet frame to the wireless interface.
	 *
	 * Moreover this feature may break AP interfaces in some specific
	 * setups. This applies e.g. to the bridge with hairpin mode enabled and
	 * IFLA_BRPORT_MCAST_TO_UCAST set. IAPP packet generated by a firmware
	 * will get passed back to the wireless interface and cause immediate
	 * disassociation of a just-connected STA.
	 */
	if (!drvr->settings->iapp && brcmf_skb_is_iapp(skb)) {
		dev_kfree_skb(skb);
		ret = -EINVAL;
		goto done;
	}

	/* Make sure there's enough writeable headroom */
	if (skb_headroom(skb) < drvr->hdrlen || skb_header_cloned(skb)) {
		head_delta = max_t(int, drvr->hdrlen - skb_headroom(skb), 0);

		brcmf_dbg(INFO, "%s: insufficient headroom (%d)\n",
			  brcmf_ifname(ifp), head_delta);
		atomic_inc(&drvr->bus_if->stats.pktcowed);
		ret = pskb_expand_head(skb, ALIGN(head_delta, NET_SKB_PAD), 0,
				       GFP_ATOMIC);
		if (ret < 0) {
			bphy_err(drvr, "%s: failed to expand headroom\n",
				 brcmf_ifname(ifp));
			atomic_inc(&drvr->bus_if->stats.pktcow_failed);
			goto done;
		}
	}

	/* validate length for ether packet */
	if (skb->len < sizeof(*eh)) {
		ret = -EINVAL;
		dev_kfree_skb(skb);
		goto done;
	}

	eh = (struct ethhdr *)(skb->data);

	if (eh->h_proto == htons(ETH_P_PAE))
		atomic_inc(&ifp->pend_8021x_cnt);

	/* determine the priority */
	if ((skb->priority == 0) || (skb->priority > 7))
		skb->priority = cfg80211_classify8021d(skb, NULL);

	/* set pacing shift for packet aggregation */
	sk_pacing_shift_update(skb->sk, 8);

	ret = brcmf_proto_tx_queue_data(drvr, ifp->ifidx, skb);
	if (ret < 0)
		brcmf_txfinalize(ifp, skb, false);

done:
	if (ret) {
		ndev->stats.tx_dropped++;
	} else {
		ndev->stats.tx_packets++;
		ndev->stats.tx_bytes += skb->len;
	}

	/* Return ok: we always eat the packet */
	return NETDEV_TX_OK;
}

void brcmf_txflowblock_if(struct brcmf_if *ifp,
			  enum brcmf_netif_stop_reason reason, bool state)
{
	unsigned long flags;

	if (!ifp || !ifp->ndev)
		return;

	brcmf_dbg(TRACE, "enter: bsscfgidx=%d stop=0x%X reason=%d state=%d\n",
		  ifp->bsscfgidx, ifp->netif_stop, reason, state);

	spin_lock_irqsave(&ifp->netif_stop_lock, flags);
	if (state) {
		if (!ifp->netif_stop)
			netif_stop_queue(ifp->ndev);
		ifp->netif_stop |= reason;
	} else {
		ifp->netif_stop &= ~reason;
		if (!ifp->netif_stop)
			netif_wake_queue(ifp->ndev);
	}
	spin_unlock_irqrestore(&ifp->netif_stop_lock, flags);
}

void brcmf_netif_rx(struct brcmf_if *ifp, struct sk_buff *skb)
{
	if (ifp->ndev->type == ARPHRD_IEEE80211_RADIOTAP) {
		if (!(ifp->ndev->flags & IFF_UP)) {
			brcmu_pkt_buf_free_skb(skb);
			return;
		}
		ifp->ndev->stats.rx_bytes += skb->len;
		ifp->ndev->stats.rx_packets++;
		netif_rx(skb);
		return;
	}

	/* Most of Broadcom's firmwares send 802.11f ADD frame every time a new
	 * STA connects to the AP interface. This is an obsoleted standard most
	 * users don't use, so don't pass these frames up unless requested.
	 */
	if (!ifp->drvr->settings->iapp && brcmf_skb_is_iapp(skb)) {
		brcmu_pkt_buf_free_skb(skb);
		return;
	}

	if (skb->pkt_type == PACKET_MULTICAST)
		ifp->ndev->stats.multicast++;

	if (!(ifp->ndev->flags & IFF_UP)) {
		brcmu_pkt_buf_free_skb(skb);
		return;
	}

	ifp->ndev->stats.rx_bytes += skb->len;
	ifp->ndev->stats.rx_packets++;

	brcmf_dbg(DATA, "rx proto=0x%X\n", ntohs(skb->protocol));
	netif_rx(skb);
}

/*
 * Recover 64-bit continuous microsecond hardware TSF timestamp from
 * the 32-bit wlc_d11rxhdr tsf_l or radiotap low word.
 * If tsf_low is 0 and uninitialized, falls back to ktime_get_boottime_ns() / 1000.
 */
static u64 brcmf_mon_recover_tsf64(struct brcmf_pub *drvr, u32 tsf_low)
{
	if (unlikely(!tsf_low && !drvr->nexmon_tsf_initialized))
		return div_u64(ktime_get_boottime_ns(), 1000);

	if (unlikely(!drvr->nexmon_tsf_initialized)) {
		drvr->nexmon_tsf_initialized = true;
		drvr->nexmon_last_tsf_low = tsf_low;
		drvr->nexmon_tsf_high = 0;
	} else {
		/* Rollover check: detect wrap from upper quadrant to lower quadrant */
		if (unlikely(drvr->nexmon_last_tsf_low >= 0x80000000U &&
			     tsf_low < 0x80000000U))
			drvr->nexmon_tsf_high++;
		drvr->nexmon_last_tsf_low = tsf_low;
	}

	return ((u64)drvr->nexmon_tsf_high << 32) | (u64)tsf_low;
}

void brcmf_netif_mon_rx(struct brcmf_if *ifp, struct sk_buff *skb)
{
	if (brcmf_feat_is_enabled(ifp, BRCMF_FEAT_MONITOR_FMT_RADIOTAP)) {
		/* Do nothing */
	} else if (brcmf_feat_is_enabled(ifp, BRCMF_FEAT_MONITOR_FMT_HW_RX_HDR)) {
		struct wlc_d11rxhdr *wlc_rxhdr;
		struct brcmf_mon_radiotap *rtap;
		unsigned int offset;
		u16 RxStatus1, rx_chan;
		s8 rx_rssi;
		u32 rx_tsf_l;

		/* The firmware controls the length of this buffer, so validate
		 * that the receive header is fully present before dereferencing
		 * it.
		 */
		if (!pskb_may_pull(skb, sizeof(struct wlc_d11rxhdr))) {
			brcmf_dbg(DATA, "mon rx: skb too short for rxhdr\n");
			dev_kfree_skb_any(skb);
			return;
		}
		wlc_rxhdr = (struct wlc_d11rxhdr *)skb->data;

		RxStatus1 = le16_to_cpu(wlc_rxhdr->rxhdr.RxStatus1);
		rx_chan = le16_to_cpu(wlc_rxhdr->rxhdr.RxChan);
		rx_rssi = wlc_rxhdr->rssi;
		rx_tsf_l = le32_to_cpu(wlc_rxhdr->tsf_l);

		offset = sizeof(struct wlc_d11rxhdr);
		/* MAC inserts 2 pad bytes for a4 headers or QoS or A-MSDU
		 * subframes
		 */
		if (RxStatus1 & RXS_PBPRES)
			offset += 2;
		offset += D11_PHY_HDR_LEN;

		/* Ensure the whole header we are about to strip is present. */
		if (!pskb_may_pull(skb, offset)) {
			brcmf_dbg(DATA, "mon rx: skb too short for offset %u\n", offset);
			dev_kfree_skb_any(skb);
			return;
		}
		skb_pull(skb, offset);

		/* Use RX header to fill radiotap channel, band and signal metadata */
		if (skb_headroom(skb) < sizeof(*rtap)) {
			dev_kfree_skb_any(skb);
			return;
		}
		{
			u16 freq = 2412;
			u16 ch_flags = IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM;
			u8 ch_num = rx_chan & 0xff;

			if (ch_num >= 1 && ch_num <= 13) {
				freq = 2407 + ch_num * 5;
				ch_flags = IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_OFDM;
			} else if (ch_num == 14) {
				freq = 2484;
				ch_flags = IEEE80211_CHAN_2GHZ | IEEE80211_CHAN_CCK;
			} else if (ch_num >= 36) {
				freq = 5000 + ch_num * 5;
				ch_flags = IEEE80211_CHAN_5GHZ | IEEE80211_CHAN_OFDM;
			}

			rtap = skb_push(skb, sizeof(*rtap));
			memset(rtap, 0, sizeof(*rtap));
			rtap->hdr.it_version = PKTHDR_RADIOTAP_VERSION;
			rtap->hdr.it_len = cpu_to_le16(sizeof(*rtap));
			rtap->hdr.it_present = cpu_to_le32(BIT(IEEE80211_RADIOTAP_TSFT) |
							   BIT(IEEE80211_RADIOTAP_FLAGS) |
							   BIT(IEEE80211_RADIOTAP_CHANNEL) |
							   BIT(IEEE80211_RADIOTAP_DBM_ANTSIGNAL));
			rtap->tsft = cpu_to_le64(brcmf_mon_recover_tsf64(ifp->drvr, rx_tsf_l));
			rtap->flags = 0;
			rtap->channel_freq = cpu_to_le16(freq);
			rtap->channel_flags = cpu_to_le16(ch_flags);
			rtap->dbm_antsignal = rx_rssi ? rx_rssi : -50;
		}

		/* Trim 4 bytes of hardware receive status if present */
		if (skb->len >= 4)
			skb_trim(skb, skb->len - 4);
	} else {
		struct brcmf_mon_radiotap *rtap;

		/* NOTE: RX status used when available; minimal radiotap otherwise */
		if (skb_headroom(skb) < sizeof(*rtap)) {
			dev_kfree_skb_any(skb);
			return;
		}
		rtap = skb_push(skb, sizeof(*rtap));
		memset(rtap, 0, sizeof(*rtap));
		rtap->hdr.it_version = PKTHDR_RADIOTAP_VERSION;
		rtap->hdr.it_len = cpu_to_le16(sizeof(*rtap));
		rtap->hdr.it_present = cpu_to_le32(BIT(IEEE80211_RADIOTAP_TSFT) |
						   BIT(IEEE80211_RADIOTAP_FLAGS));
		rtap->tsft = cpu_to_le64(div_u64(ktime_get_boottime_ns(), 1000));
		rtap->flags = 0;

		/* NOTE: 4-byte HW status trimmed safely if present */
		if (skb->len >= 4)
			skb_trim(skb, skb->len - 4);
	}

	skb->dev = ifp->ndev;
	skb_reset_mac_header(skb);
	skb->pkt_type = PACKET_OTHERHOST;
	skb->protocol = htons(ETH_P_802_2);

	brcmf_netif_rx(ifp, skb);
}

static int brcmf_rx_hdrpull(struct brcmf_pub *drvr, struct sk_buff *skb,
			    struct brcmf_if **ifp)
{
	int ret;
	struct brcmf_if *mon_if;

	/* process and remove protocol-specific header */
	ret = brcmf_proto_hdrpull(drvr, true, skb, ifp);

	if (ret || !(*ifp) || !(*ifp)->ndev) {
		if (ret != -ENODATA && *ifp && (*ifp)->ndev)
			(*ifp)->ndev->stats.rx_errors++;
		brcmu_pkt_buf_free_skb(skb);
		return -ENODATA;
	}

	/* If a monitor interface is active or frame carries radiotap header, route to mon_if */
	mon_if = READ_ONCE(drvr->mon_if);
	if (mon_if && mon_if->ndev && netif_running(mon_if->ndev)) {
		struct ieee80211_radiotap_header *rth = (void *)skb->data;
		if (skb->len >= sizeof(*rth) && rth->it_version == 0 &&
		    le16_to_cpu(rth->it_len) >= sizeof(*rth) &&
		    le16_to_cpu(rth->it_len) <= skb->len) {
			*ifp = mon_if;
		}
	} else if (!mon_if) {
		int i;
		for (i = 0; i < BRCMF_MAX_IFS; i++) {
			struct brcmf_if *tmp = READ_ONCE(drvr->iflist[i]);
			if (tmp && tmp->ndev && netif_running(tmp->ndev) &&
			    (tmp->ndev->type == ARPHRD_IEEE80211_RADIOTAP ||
			     (tmp->vif && tmp->vif->wdev.iftype == NL80211_IFTYPE_MONITOR))) {
				struct ieee80211_radiotap_header *rth = (void *)skb->data;
				if (skb->len >= sizeof(*rth) && rth->it_version == 0 &&
				    le16_to_cpu(rth->it_len) >= sizeof(*rth) &&
				    le16_to_cpu(rth->it_len) <= skb->len) {
					*ifp = tmp;
					WRITE_ONCE(drvr->mon_if, tmp);
					break;
				}
			}
		}
	}

	if (unlikely(!(*ifp) || !(*ifp)->ndev)) {
		brcmu_pkt_buf_free_skb(skb);
		return -ENODATA;
	}

	if ((*ifp)->ndev->type == ARPHRD_IEEE80211_RADIOTAP) {
		struct ieee80211_radiotap_header *rth = (void *)skb->data;

		if (skb->len >= sizeof(*rth) + sizeof(u64) &&
		    rth->it_version == PKTHDR_RADIOTAP_VERSION &&
		    (le32_to_cpu(rth->it_present) & BIT(IEEE80211_RADIOTAP_TSFT))) {
			__le32 *tsf = (__le32 *)(skb->data + sizeof(*rth));
			u32 tsf_l = le32_to_cpu(tsf[0]);
			u32 tsf_h = le32_to_cpu(tsf[1]);

			if (tsf_h == 0 && tsf_l != 0 && !skb_shared(skb)) {
				u64 tsf64 = brcmf_mon_recover_tsf64(drvr, tsf_l);
				tsf[1] = cpu_to_le32((u32)(tsf64 >> 32));
			}
		}

		skb->dev = (*ifp)->ndev;
		skb_reset_mac_header(skb);
		skb->ip_summed = CHECKSUM_UNNECESSARY;
		skb->pkt_type = PACKET_OTHERHOST;
		skb->protocol = htons(ETH_P_802_2);
		return 0;
	}

	skb->protocol = eth_type_trans(skb, (*ifp)->ndev);
	return 0;
}

/*
 * Zero-copy Netlink CSI streaming:
 * Detects Nexmon CSI frames emitted by firmware UDP tunneling (port 5500, magic 0x11111111).
 * If userspace Netlink listeners are active, multicasts the payload directly via
 * Generic Netlink (family "nexmon_csi", group "csi") and frees the incoming skb,
 * eliminating host broadcast flooding and socket buffer overflow at 80/160 MHz.
 * Returns true if consumed, false if unhandled (passes through to standard rx).
 */
static bool nexmon_csi_handle_rx(struct brcmf_if *ifp, struct sk_buff *skb)
{
	struct iphdr *iph;
	struct udphdr *uh;
	u8 *payload;
	u32 payload_len;
	u32 ip_hlen;
	struct sk_buff *nl_skb;
	void *msg_hdr;
	bool has_genl_listeners;
	bool has_nl_listeners;

	if (skb->protocol != htons(ETH_P_IP))
		return false;

	if (!pskb_may_pull(skb, sizeof(struct iphdr) + sizeof(struct udphdr) + 18))
		return false;

	iph = (struct iphdr *)skb->data;
	if (iph->version != 4 || iph->protocol != IPPROTO_UDP)
		return false;

	ip_hlen = iph->ihl * 4;
	if (ip_hlen < sizeof(struct iphdr) || skb->len < ip_hlen + sizeof(struct udphdr) + 18)
		return false;

	uh = (struct udphdr *)(skb->data + ip_hlen);
	if (uh->dest != htons(5500))
		return false;

	payload = (u8 *)uh + sizeof(struct udphdr);
	payload_len = ntohs(uh->len);
	if (payload_len < sizeof(struct udphdr) + 18)
		return false;
	payload_len -= sizeof(struct udphdr);

	if (payload + payload_len > skb->data + skb->len)
		payload_len = (skb->data + skb->len) - payload;

	/* Validate Nexmon CSI Magic: 0x11111111 */
	if (*(__le32 *)payload != cpu_to_le32(0x11111111))
		return false;

	has_genl_listeners = genl_has_listeners(&nexmon_csi_gnl_family, &init_net, NEXMON_CSI_MCGRP_CSI);
	has_nl_listeners = nl_sock && netlink_has_listeners(nl_sock, 1);

	if (has_genl_listeners || has_nl_listeners) {
		if (has_genl_listeners) {
			nl_skb = genlmsg_new(nla_total_size(payload_len) +
					     nla_total_size_64bit(sizeof(u64)) +
					     nla_total_size(sizeof(u32)), GFP_ATOMIC);
			if (nl_skb) {
				msg_hdr = genlmsg_put(nl_skb, 0, 0, &nexmon_csi_gnl_family,
						      0, NEXMON_CSI_CMD_SAMPLE);
				if (msg_hdr) {
					nla_put(nl_skb, NEXMON_CSI_ATTR_RAW, payload_len, payload);
					nla_put_u64_64bit(nl_skb, NEXMON_CSI_ATTR_TSFT,
							  div_u64(ktime_get_boottime_ns(), 1000),
							  NEXMON_CSI_ATTR_PAD);
					if (ifp && ifp->ndev)
						nla_put_u32(nl_skb, NEXMON_CSI_ATTR_IFINDEX, ifp->ndev->ifindex);
					genlmsg_end(nl_skb, msg_hdr);
					genlmsg_multicast_allns(&nexmon_csi_gnl_family, nl_skb, 0,
								NEXMON_CSI_MCGRP_CSI);
				} else {
					nlmsg_free(nl_skb);
				}
			}
		}

		if (has_nl_listeners) {
			nl_skb = nlmsg_new(payload_len, GFP_ATOMIC);
			if (nl_skb) {
				struct nlmsghdr *nlh = nlmsg_put(nl_skb, 0, 0, NLMSG_DONE, payload_len, 0);
				if (nlh) {
					memcpy(nlmsg_data(nlh), payload, payload_len);
					nlmsg_multicast(nl_sock, nl_skb, 0, 1, GFP_ATOMIC);
				} else {
					nlmsg_free(nl_skb);
				}
			}
		}

		if (ifp && ifp->ndev) {
			ifp->ndev->stats.rx_bytes += skb->len;
			ifp->ndev->stats.rx_packets++;
		}

		brcmu_pkt_buf_free_skb(skb);
		return true;
	}

	return false;
}

void brcmf_rx_frame(struct device *dev, struct sk_buff *skb, bool handle_event,
		    bool inirq)
{
	struct brcmf_if *ifp;
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	brcmf_dbg(DATA, "Enter: %s: rxp=%p\n", dev_name(dev), skb);

	if (brcmf_rx_hdrpull(drvr, skb, &ifp))
		return;

	if (nexmon_csi_handle_rx(ifp, skb))
		return;

	if (brcmf_proto_is_reorder_skb(skb)) {
		brcmf_proto_rxreorder(ifp, skb);
	} else {
		/* Process special event packets */
		if (handle_event) {
			gfp_t gfp = inirq ? GFP_ATOMIC : GFP_KERNEL;

			brcmf_fweh_process_skb(ifp->drvr, skb,
					       BCMILCP_SUBTYPE_VENDOR_LONG, gfp);
		}
		brcmf_netif_rx(ifp, skb);
	}
}

void brcmf_rx_event(struct device *dev, struct sk_buff *skb)
{
	struct brcmf_if *ifp;
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	brcmf_dbg(EVENT, "Enter: %s: rxp=%p\n", dev_name(dev), skb);

	if (brcmf_rx_hdrpull(drvr, skb, &ifp))
		return;

	brcmf_fweh_process_skb(ifp->drvr, skb, 0, GFP_KERNEL);
	brcmu_pkt_buf_free_skb(skb);
}

void brcmf_txfinalize(struct brcmf_if *ifp, struct sk_buff *txp, bool success)
{
	struct ethhdr *eh;
	u16 type;

	if (!ifp) {
		brcmu_pkt_buf_free_skb(txp);
		return;
	}

	eh = (struct ethhdr *)(txp->data);
	type = ntohs(eh->h_proto);

	if (type == ETH_P_PAE) {
		atomic_dec(&ifp->pend_8021x_cnt);
		/* Order the decrement before waitqueue_active() */
		smp_mb__after_atomic();
		if (waitqueue_active(&ifp->pend_8021x_wait))
			wake_up(&ifp->pend_8021x_wait);
	}

	if (!success)
		ifp->ndev->stats.tx_errors++;

	brcmu_pkt_buf_free_skb(txp);
}

static void brcmf_ethtool_get_drvinfo(struct net_device *ndev,
				    struct ethtool_drvinfo *info)
{
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_pub *drvr = ifp->drvr;
	char drev[BRCMU_DOTREV_LEN] = "n/a";

	if (drvr->revinfo.result == 0)
		brcmu_dotrev_str(drvr->revinfo.driverrev, drev);
	strscpy(info->driver, KBUILD_MODNAME, sizeof(info->driver));
	strscpy(info->version, drev, sizeof(info->version));
	strscpy(info->fw_version, drvr->fwver, sizeof(info->fw_version));
	strscpy(info->bus_info, dev_name(drvr->bus_if->dev),
		sizeof(info->bus_info));
}

static const struct ethtool_ops brcmf_ethtool_ops = {
	.get_drvinfo = brcmf_ethtool_get_drvinfo,
};

/* True if a monitor vif is IFF_UP. Used to keep the primary BSS alive so
 * cfg80211 can treat the phy as monitor-only (channel hops) without the
 * firmware tearing down hwrs_scb / D11 clocks that injection needs.
 */
bool brcmf_nexmon_monitor_running(struct brcmf_pub *drvr)
{
	int i;

	for (i = 0; i < BRCMF_MAX_IFS; i++) {
		struct brcmf_if *tmp = drvr->iflist[i];

		if (tmp && tmp->ndev && netif_running(tmp->ndev) &&
		    tmp->vif &&
		    tmp->vif->wdev.iftype == NL80211_IFTYPE_MONITOR)
			return true;
	}
	return false;
}

static int brcmf_netdev_stop(struct net_device *ndev)
{
	struct brcmf_if *ifp = netdev_priv(ndev);

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	/* Taking the STA netdev down is what lets cfg80211 allow monitor
	 * channel hops (has_monitors_only). Do not DISASSOC / clear READY
	 * in firmware while a monitor vif is still running. */
	if (!brcmf_nexmon_monitor_running(ifp->drvr)) {
		brcmf_cfg80211_down(ndev);
	} else {
		/* Monitor fast-path: we skip brcmf_cfg80211_down() above to keep
		 * the primary BSS up in firmware for injection. But that path is
		 * also what aborts an in-flight scan (brcmf_abort_scanning()).
		 * Without it, once this netdev goes down cfg80211's NETDEV_DOWN
		 * notifier completes and frees the pending scan request, and the
		 * later firmware escan-complete event then calls
		 * cfg80211_scan_done() on the freed request -> BUG_ON(!wiphy) in
		 * wiphy_to_rdev(). So abort a scan owned by this interface here,
		 * exactly once, under the same lock brcmf_cfg80211_down() uses. */
		struct brcmf_cfg80211_info *cfg = ifp->drvr->config;

		if (cfg &&
		    test_bit(BRCMF_SCAN_STATUS_BUSY, &cfg->scan_status) &&
		    cfg->escan_info.ifp == ifp) {
			mutex_lock(&cfg->usr_sync);
			brcmf_abort_scanning(cfg);
			mutex_unlock(&cfg->usr_sync);
		}
	}

	brcmf_net_setcarrier(ifp, false);

	return 0;
}

static int brcmf_netdev_open(struct net_device *ndev)
{
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_pub *drvr = ifp->drvr;
	struct brcmf_bus *bus_if = drvr->bus_if;
	u32 toe_ol;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d\n", ifp->bsscfgidx);

	/* If bus is not ready, can't continue */
	if (bus_if->state != BRCMF_BUS_UP) {
		bphy_err(drvr, "failed bus is not ready\n");
		return -EAGAIN;
	}

	atomic_set(&ifp->pend_8021x_cnt, 0);

	/* Get current TOE mode from dongle */
	if (brcmf_fil_iovar_int_get(ifp, "toe_ol", &toe_ol) >= 0
	    && (toe_ol & TOE_TX_CSUM_OL) != 0)
		ndev->features |= NETIF_F_IP_CSUM;
	else
		ndev->features &= ~NETIF_F_IP_CSUM;

	if (brcmf_cfg80211_up(ndev)) {
		bphy_err(drvr, "failed to bring up cfg80211\n");
		return -EIO;
	}

	/* Clear, carrier, set when connected or AP mode. */
	netif_carrier_off(ndev);
	return 0;
}

static const struct net_device_ops brcmf_netdev_ops_pri = {
	.ndo_open = brcmf_netdev_open,
	.ndo_stop = brcmf_netdev_stop,
	.ndo_start_xmit = brcmf_netdev_start_xmit,
	.ndo_set_mac_address = brcmf_netdev_set_mac_address,
	.ndo_set_rx_mode = brcmf_netdev_set_multicast_list
};

int brcmf_net_attach(struct brcmf_if *ifp, bool locked)
{
	struct brcmf_pub *drvr = ifp->drvr;
	struct net_device *ndev;
	s32 err;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d mac=%pM\n", ifp->bsscfgidx,
		  ifp->mac_addr);
	ndev = ifp->ndev;

	/* set appropriate operations */
	ndev->netdev_ops = &brcmf_netdev_ops_pri;

	ndev->needed_headroom += drvr->hdrlen;
	ndev->ethtool_ops = &brcmf_ethtool_ops;

	/* set the mac address & netns */
	eth_hw_addr_set(ndev, ifp->mac_addr);
	dev_net_set(ndev, wiphy_net(cfg_to_wiphy(drvr->config)));

	INIT_WORK(&ifp->multicast_work, _brcmf_set_multicast_list);
	INIT_WORK(&ifp->ndoffload_work, _brcmf_update_ndtable);

	if (locked)
		err = cfg80211_register_netdevice(ndev);
	else
		err = register_netdev(ndev);
	if (err != 0) {
		bphy_err(drvr, "couldn't register the net device\n");
		goto fail;
	}

	/* NEXMON: registration assigned a stable ifindex. */
	if (!READ_ONCE(nexmon_default_ifindex))
		WRITE_ONCE(nexmon_default_ifindex, ndev->ifindex);

	netif_carrier_off(ndev);

	ndev->priv_destructor = brcmf_cfg80211_free_netdev;
	brcmf_dbg(INFO, "%s: Broadcom Dongle Host Driver\n", ndev->name);
	return 0;

fail:
	drvr->iflist[ifp->bsscfgidx] = NULL;
	ndev->netdev_ops = NULL;
	return -EBADE;
}

void brcmf_net_detach(struct net_device *ndev, bool locked)
{
	struct brcmf_if *ifp = netdev_priv(ndev);

	if (ifp && ifp->drvr && ifp->drvr->mon_if == ifp)
		WRITE_ONCE(ifp->drvr->mon_if, NULL);

	if (ndev->reg_state == NETREG_REGISTERED) {
		if (locked)
			cfg80211_unregister_netdevice(ndev);
		else
			unregister_netdev(ndev);
	} else {
		brcmf_cfg80211_free_netdev(ndev);
		free_netdev(ndev);
	}
}

/* Monitor-restore intent, preserved across a control-DCMD wedge reset.
 *
 * A wedge (brcmf_sdio_ctl_watchdog -> bus_reset) tears the whole stack down:
 * brcmf_detach() frees drvr, every netdev and the monitor vif, then the card
 * re-enumerates and re-probes from scratch. Nothing in drvr survives, so the
 * fact that a monitor session was live has to live in a module-global - the
 * same single-radio scope as nexmon_default_ifindex. On the next successful
 * brcmf_bus_started() we replay it (brcmf_mon_restore_vif) under the old name,
 * so a name-based channel hopper keeps working across recovery.
 *
 * Set on the last successful step of brcmf_net_mon_open(); cleared only by a
 * genuine user-initiated brcmf_net_mon_stop() (bus up). A stop with the bus
 * already down is the reset teardown itself and deliberately leaves it set.
 */
static struct {
	spinlock_t lock;
	bool active;
	char name[IFNAMSIZ];
	u16 chanspec;
} nexmon_mon_restore = {
	.lock = __SPIN_LOCK_UNLOCKED(nexmon_mon_restore.lock),
};

void nexmon_mon_restore_set_chanspec(u16 chanspec)
{
	unsigned long flags;

	spin_lock_irqsave(&nexmon_mon_restore.lock, flags);
	nexmon_mon_restore.chanspec = chanspec;
	spin_unlock_irqrestore(&nexmon_mon_restore.lock, flags);
}

u16 nexmon_mon_restore_get_chanspec(void)
{
	unsigned long flags;
	u16 chspec;

	spin_lock_irqsave(&nexmon_mon_restore.lock, flags);
	chspec = nexmon_mon_restore.chanspec;
	spin_unlock_irqrestore(&nexmon_mon_restore.lock, flags);
	return chspec;
}

static void nexmon_mon_restore_set(struct brcmf_if *ifp)
{
	unsigned long flags;

	spin_lock_irqsave(&nexmon_mon_restore.lock, flags);
	nexmon_mon_restore.active = true;
	strscpy(nexmon_mon_restore.name, ifp->ndev->name,
		sizeof(nexmon_mon_restore.name));
	spin_unlock_irqrestore(&nexmon_mon_restore.lock, flags);
}

static void nexmon_mon_restore_clear(void)
{
	unsigned long flags;

	spin_lock_irqsave(&nexmon_mon_restore.lock, flags);
	nexmon_mon_restore.active = false;
	nexmon_mon_restore.chanspec = 0;
	spin_unlock_irqrestore(&nexmon_mon_restore.lock, flags);
}

static bool nexmon_mon_restore_pending(void)
{
	unsigned long flags;
	bool active;

	spin_lock_irqsave(&nexmon_mon_restore.lock, flags);
	active = nexmon_mon_restore.active;
	spin_unlock_irqrestore(&nexmon_mon_restore.lock, flags);
	return active;
}

static bool nexmon_mon_restore_take(char *name, size_t name_sz)
{
	unsigned long flags;
	bool active;

	spin_lock_irqsave(&nexmon_mon_restore.lock, flags);
	active = nexmon_mon_restore.active;
	if (active)
		strscpy(name, nexmon_mon_restore.name, name_sz);
	spin_unlock_irqrestore(&nexmon_mon_restore.lock, flags);
	return active;
}

static int brcmf_net_mon_open(struct net_device *ndev)
{
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_pub *drvr = ifp->drvr;
	struct brcmf_if *prim = drvr->iflist[0] ? drvr->iflist[0] : ifp;
	u32 monitor;
	int err;

	brcmf_dbg(TRACE, "Enter\n");

	/* NEXMON */
	switch (ifp->ndev->type) {
		case ARPHRD_IEEE80211_RADIOTAP:
			monitor = 2; // RADIOTAP ENABLED MONITOR MODE
			break;
		case ARPHRD_IEEE80211:
			monitor = 1; // MONITOR MODE WITHOUT RADIOTAP
			break;
		default:
			monitor = 0;
	}
	
	err = brcmf_fil_cmd_int_set(prim, BRCMF_C_SET_MONITOR, monitor);
	if (err) {
		bphy_err(drvr, "BRCMF_C_SET_MONITOR error (%d)\n", err);
		return err;
	}

	/* NEXMON: monitor mode alone doesn't open the hardware RX filter to
	 * unassociated/foreign traffic; without promiscuous mode the D11 MAC
	 * silently drops everything that isn't for our own (nonexistent) BSS. */
	err = brcmf_fil_cmd_int_set(prim, BRCMF_C_SET_PROMISC, 1);
	if (err)
		bphy_err(drvr, "BRCMF_C_SET_PROMISC error (%d)\n", err);

	/* Chip-global: leave the PHY clocks on so injected frames actually
	 * key the radio (mpc/PM otherwise echo TX into monitor RX only).
	 * Snapshot the previous values first, per field, so brcmf_net_mon_stop()
	 * restores whatever it managed to read instead of leaving the STA on
	 * full TX power with PM and mpc disabled after monitor teardown. */
	{
		u32 qtxpwr_max = 127 | WL_TXPWR_OVERRIDE;

		drvr->nexmon_saved_pm_valid = false;
		drvr->nexmon_saved_txpwr_valid = false;
		drvr->nexmon_saved_mpc_valid = false;

		if (!brcmf_fil_cmd_int_get(prim, BRCMF_C_GET_PM,
					   &drvr->nexmon_saved_pm))
			drvr->nexmon_saved_pm_valid = true;
		else
			brcmf_dbg(INFO, "NEXMON: PM snapshot unsupported\n");

		/* Modern brcmfmac firmware exposes TX power through the
		 * "qtxpower" iovar; the legacy BRCMF_C_{GET,SET}_TXPWR DCMDs
		 * (65/66) return -EBADE (-52) on it. */
		if (!brcmf_fil_iovar_int_get(prim, "qtxpower",
					     &drvr->nexmon_saved_txpwr))
			drvr->nexmon_saved_txpwr_valid = true;
		else
			brcmf_dbg(INFO, "NEXMON: qtxpower snapshot unsupported\n");

		if (!brcmf_fil_iovar_int_get(prim, "mpc", &drvr->nexmon_saved_mpc))
			drvr->nexmon_saved_mpc_valid = true;
		else
			brcmf_dbg(INFO, "NEXMON: mpc snapshot unsupported\n");

		int tmp_err;

		tmp_err = brcmf_fil_cmd_int_set(prim, BRCMF_C_SET_PM, 0);
		if (tmp_err)
			bphy_err(drvr, "BRCMF_C_SET_PM error (%d)\n", tmp_err);
		tmp_err = brcmf_fil_iovar_int_set(prim, "qtxpower", qtxpwr_max);
		if (tmp_err)
			bphy_err(drvr, "qtxpower set error (%d)\n", tmp_err);
		tmp_err = brcmf_fil_iovar_int_set(prim, "mpc", 0);
		if (tmp_err)
			bphy_err(drvr, "mpc set error (%d)\n", tmp_err);
		err = 0;
	}

	/* Monitor is now live: remember it so a wedge-reset can replay the vif
	 * after the card re-probes (see nexmon_mon_restore, brcmf_bus_started).
	 * A user-initiated stop clears this again in brcmf_net_mon_stop(). */
	nexmon_mon_restore_set(ifp);
	{
		u32 chspec = 0;
		if (brcmf_fil_iovar_int_get(prim, "chanspec", &chspec) == 0 && chspec)
			nexmon_mon_restore_set_chanspec((u16)chspec);
	}

	return err;
}

static int brcmf_net_mon_stop(struct net_device *ndev)
{
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_pub *drvr = ifp->drvr;
	struct brcmf_if *prim = drvr->iflist[0] ? drvr->iflist[0] : ifp;
	u32 monitor;
	int err;

	brcmf_dbg(TRACE, "Enter\n");

	/* A stop with the bus already down is the wedge-reset teardown path
	 * (brcmf_detach -> unregister_netdevice -> ndo_stop). The dongle is
	 * gone, so the DCMDs below would only emit "bus is down" errors and
	 * fail; skip them and leave the monitor-restore intent set, so
	 * brcmf_bus_started() replays the session after the card re-probes.
	 * Only a genuine user-initiated stop (bus up) clears that intent. */
	if (drvr->bus_if->state != BRCMF_BUS_UP) {
		brcmf_dbg(SDIO, "monitor stop on down bus: deferring restore\n");
		return 0;
	}

	nexmon_mon_restore_clear();

	brcmf_fil_cmd_int_set(prim, BRCMF_C_SET_PROMISC, 0);

	monitor = 0;
	err = brcmf_fil_cmd_int_set(prim, BRCMF_C_SET_MONITOR, monitor);
	if (err)
		bphy_err(drvr, "BRCMF_C_SET_MONITOR error (%d)\n", err);

	/* Restore the chip-global radio settings that brcmf_net_mon_open()
	 * changed, so PM/TXPWR/mpc do not stay overridden on the STA after
	 * monitor mode is torn down.  Each field is put back only if its
	 * snapshot succeeded. */
	{

		if (drvr->nexmon_saved_pm_valid) {
			if (brcmf_fil_cmd_int_set(prim, BRCMF_C_SET_PM,
						  drvr->nexmon_saved_pm))
				bphy_err(drvr, "failed restoring PM\n");
			drvr->nexmon_saved_pm_valid = false;
		}
		if (drvr->nexmon_saved_txpwr_valid) {
			if (brcmf_fil_iovar_int_set(prim, "qtxpower",
						    drvr->nexmon_saved_txpwr))
				bphy_err(drvr, "failed restoring qtxpower\n");
			drvr->nexmon_saved_txpwr_valid = false;
		}
		if (drvr->nexmon_saved_mpc_valid) {
			if (brcmf_fil_iovar_int_set(prim, "mpc",
						    drvr->nexmon_saved_mpc))
				bphy_err(drvr, "failed restoring mpc\n");
			drvr->nexmon_saved_mpc_valid = false;
		}
	}

	return err;
}

static const struct net_device_ops brcmf_netdev_ops_mon = {
	.ndo_open = brcmf_net_mon_open,
	.ndo_stop = brcmf_net_mon_stop,
	.ndo_start_xmit = brcmf_netdev_start_xmit,
	.ndo_set_mac_address = brcmf_netdev_set_mac_address,
};

int brcmf_net_mon_attach(struct brcmf_if *ifp)
{
	struct brcmf_pub *drvr = ifp->drvr;
	struct net_device *ndev;
	int err;

	brcmf_dbg(TRACE, "Enter\n");

	ndev = ifp->ndev;
	ndev->netdev_ops = &brcmf_netdev_ops_mon;

	if (is_zero_ether_addr(ifp->mac_addr) && drvr->iflist[0])
		memcpy(ifp->mac_addr, drvr->iflist[0]->mac_addr, ETH_ALEN);
	eth_hw_addr_set(ndev, ifp->mac_addr);

	err = cfg80211_register_netdevice(ndev);
	if (err) {
		bphy_err(drvr, "Failed to register %s device\n", ndev->name);
		drvr->iflist[ifp->bsscfgidx] = NULL;
		ndev->netdev_ops = NULL;
		return err;
	}

	ndev->priv_destructor = brcmf_cfg80211_free_netdev;

	return 0;
}

void brcmf_net_setcarrier(struct brcmf_if *ifp, bool on)
{
	struct net_device *ndev;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d carrier=%d\n", ifp->bsscfgidx,
		  on);

	ndev = ifp->ndev;
	brcmf_txflowblock_if(ifp, BRCMF_NETIF_STOP_REASON_DISCONNECTED, !on);
	if (on) {
		if (!netif_carrier_ok(ndev))
			netif_carrier_on(ndev);

	} else {
		if (netif_carrier_ok(ndev))
			netif_carrier_off(ndev);
	}
}

static int brcmf_net_p2p_open(struct net_device *ndev)
{
	brcmf_dbg(TRACE, "Enter\n");

	return brcmf_cfg80211_up(ndev);
}

static int brcmf_net_p2p_stop(struct net_device *ndev)
{
	brcmf_dbg(TRACE, "Enter\n");

	return brcmf_cfg80211_down(ndev);
}

static netdev_tx_t brcmf_net_p2p_start_xmit(struct sk_buff *skb,
					    struct net_device *ndev)
{
	if (skb)
		dev_kfree_skb_any(skb);

	return NETDEV_TX_OK;
}

static const struct net_device_ops brcmf_netdev_ops_p2p = {
	.ndo_open = brcmf_net_p2p_open,
	.ndo_stop = brcmf_net_p2p_stop,
	.ndo_start_xmit = brcmf_net_p2p_start_xmit
};

static int brcmf_net_p2p_attach(struct brcmf_if *ifp)
{
	struct brcmf_pub *drvr = ifp->drvr;
	struct net_device *ndev;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d mac=%pM\n", ifp->bsscfgidx,
		  ifp->mac_addr);
	ndev = ifp->ndev;

	ndev->netdev_ops = &brcmf_netdev_ops_p2p;

	/* set the mac address */
	eth_hw_addr_set(ndev, ifp->mac_addr);

	if (register_netdev(ndev) != 0) {
		bphy_err(drvr, "couldn't register the p2p net device\n");
		goto fail;
	}

	brcmf_dbg(INFO, "%s: Broadcom Dongle Host Driver\n", ndev->name);

	return 0;

fail:
	ifp->drvr->iflist[ifp->bsscfgidx] = NULL;
	ndev->netdev_ops = NULL;
	return -EBADE;
}

struct brcmf_if *brcmf_add_if(struct brcmf_pub *drvr, s32 bsscfgidx, s32 ifidx,
			      bool is_p2pdev, const char *name, u8 *mac_addr)
{
	struct brcmf_if *ifp;
	struct net_device *ndev;

	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d, ifidx=%d\n", bsscfgidx, ifidx);

	ifp = drvr->iflist[bsscfgidx];
	/*
	 * Delete the existing interface before overwriting it
	 * in case we missed the BRCMF_E_IF_DEL event.
	 */
	if (ifp) {
		if (ifidx) {
			bphy_err(drvr, "ERROR: netdev:%s already exists\n",
				 ifp->ndev->name);
			netif_stop_queue(ifp->ndev);
			brcmf_net_detach(ifp->ndev, false);
			drvr->iflist[bsscfgidx] = NULL;
		} else {
			brcmf_dbg(INFO, "netdev:%s ignore IF event\n",
				  ifp->ndev->name);
			return ERR_PTR(-EINVAL);
		}
	}

	if (!drvr->settings->p2p_enable && is_p2pdev) {
		/* this is P2P_DEVICE interface */
		brcmf_dbg(INFO, "allocate non-netdev interface\n");
		ifp = kzalloc(sizeof(*ifp), GFP_KERNEL);
		if (!ifp)
			return ERR_PTR(-ENOMEM);
	} else {
		brcmf_dbg(INFO, "allocate netdev interface\n");
		/* Allocate netdev, including space for private structure */
		ndev = alloc_netdev(sizeof(*ifp), is_p2pdev ? "p2p%d" : name,
				    NET_NAME_UNKNOWN, ether_setup);
		if (!ndev)
			return ERR_PTR(-ENOMEM);

		ndev->needs_free_netdev = true;
		ifp = netdev_priv(ndev);
		ifp->ndev = ndev;
		/* store mapping ifidx to bsscfgidx */
		if (drvr->if2bss[ifidx] == BRCMF_BSSIDX_INVALID)
			drvr->if2bss[ifidx] = bsscfgidx;
	}

	ifp->drvr = drvr;
	drvr->iflist[bsscfgidx] = ifp;
	ifp->ifidx = ifidx;
	ifp->bsscfgidx = bsscfgidx;

	init_waitqueue_head(&ifp->pend_8021x_wait);
	spin_lock_init(&ifp->netif_stop_lock);

	if (mac_addr != NULL)
		memcpy(ifp->mac_addr, mac_addr, ETH_ALEN);

	brcmf_dbg(TRACE, " ==== pid:%x, if:%s (%pM) created ===\n",
		  current->pid, name, ifp->mac_addr);

	return ifp;
}

static void brcmf_del_if(struct brcmf_pub *drvr, s32 bsscfgidx,
			 bool locked)
{
	struct brcmf_if *ifp;
	int ifidx;

	ifp = drvr->iflist[bsscfgidx];
	if (!ifp) {
		bphy_err(drvr, "Null interface, bsscfgidx=%d\n", bsscfgidx);
		return;
	}
	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d, ifidx=%d\n", bsscfgidx,
		  ifp->ifidx);
	ifidx = ifp->ifidx;

	if (ifp->ndev) {
		if (bsscfgidx == 0) {
			if (ifp->ndev->netdev_ops == &brcmf_netdev_ops_pri) {
				rtnl_lock();
				brcmf_netdev_stop(ifp->ndev);
				rtnl_unlock();
			}
		} else {
			netif_stop_queue(ifp->ndev);
		}

		if (ifp->ndev->netdev_ops == &brcmf_netdev_ops_pri) {
			cancel_work_sync(&ifp->multicast_work);
			cancel_work_sync(&ifp->ndoffload_work);
		}

		if (ifp->ndev->ifindex == READ_ONCE(nexmon_default_ifindex))
			WRITE_ONCE(nexmon_default_ifindex, 0);

		brcmf_net_detach(ifp->ndev, locked);
	} else {
		/* Only p2p device interfaces which get dynamically created
		 * end up here. In this case the p2p module should be informed
		 * about the removal of the interface within the firmware. If
		 * not then p2p commands towards the firmware will cause some
		 * serious troublesome side effects. The p2p module will clean
		 * up the ifp if needed.
		 */
		brcmf_p2p_ifp_removed(ifp, locked);
		kfree(ifp);
	}

	drvr->iflist[bsscfgidx] = NULL;
	if (drvr->if2bss[ifidx] == bsscfgidx)
		drvr->if2bss[ifidx] = BRCMF_BSSIDX_INVALID;
}

void brcmf_remove_interface(struct brcmf_if *ifp, bool locked)
{
	if (!ifp || WARN_ON(ifp->drvr->iflist[ifp->bsscfgidx] != ifp))
		return;
	brcmf_dbg(TRACE, "Enter, bsscfgidx=%d, ifidx=%d\n", ifp->bsscfgidx,
		  ifp->ifidx);
	brcmf_proto_del_if(ifp->drvr, ifp);
	brcmf_del_if(ifp->drvr, ifp->bsscfgidx, locked);
}

static int brcmf_psm_watchdog_notify(struct brcmf_if *ifp,
				     const struct brcmf_event_msg *evtmsg,
				     void *data)
{
	struct brcmf_pub *drvr = ifp->drvr;
	int err;

	brcmf_dbg(TRACE, "enter: bsscfgidx=%d\n", ifp->bsscfgidx);

	bphy_err(drvr, "PSM's watchdog has fired!\n");

	err = brcmf_debug_create_memdump(ifp->drvr->bus_if, data,
					 evtmsg->datalen);
	if (err)
		bphy_err(drvr, "Failed to get memory dump, %d\n", err);

	return err;
}

#ifdef CONFIG_INET
#define ARPOL_MAX_ENTRIES	8
static int brcmf_inetaddr_changed(struct notifier_block *nb,
				  unsigned long action, void *data)
{
	struct brcmf_pub *drvr = container_of(nb, struct brcmf_pub,
					      inetaddr_notifier);
	struct in_ifaddr *ifa = data;
	struct net_device *ndev = ifa->ifa_dev->dev;
	struct brcmf_if *ifp;
	int idx, i, ret;
	u32 val;
	__be32 addr_table[ARPOL_MAX_ENTRIES] = {0};

	/* Find out if the notification is meant for us */
	for (idx = 0; idx < BRCMF_MAX_IFS; idx++) {
		ifp = drvr->iflist[idx];
		if (ifp && ifp->ndev == ndev)
			break;
		if (idx == BRCMF_MAX_IFS - 1)
			return NOTIFY_DONE;
	}

	/* check if arp offload is supported */
	ret = brcmf_fil_iovar_int_get(ifp, "arpoe", &val);
	if (ret)
		return NOTIFY_OK;

	/* old version only support primary index */
	ret = brcmf_fil_iovar_int_get(ifp, "arp_version", &val);
	if (ret)
		val = 1;
	if (val == 1)
		ifp = drvr->iflist[0];

	/* retrieve the table from firmware */
	ret = brcmf_fil_iovar_data_get(ifp, "arp_hostip", addr_table,
				       sizeof(addr_table));
	if (ret) {
		bphy_err(drvr, "fail to get arp ip table err:%d\n", ret);
		return NOTIFY_OK;
	}

	for (i = 0; i < ARPOL_MAX_ENTRIES; i++)
		if (ifa->ifa_address == addr_table[i])
			break;

	switch (action) {
	case NETDEV_UP:
		if (i == ARPOL_MAX_ENTRIES) {
			brcmf_dbg(TRACE, "add %pI4 to arp table\n",
				  &ifa->ifa_address);
			/* set it directly */
			ret = brcmf_fil_iovar_data_set(ifp, "arp_hostip",
				&ifa->ifa_address, sizeof(ifa->ifa_address));
			if (ret)
				bphy_err(drvr, "add arp ip err %d\n", ret);
		}
		break;
	case NETDEV_DOWN:
		if (i < ARPOL_MAX_ENTRIES) {
			addr_table[i] = 0;
			brcmf_dbg(TRACE, "remove %pI4 from arp table\n",
				  &ifa->ifa_address);
			/* clear the table in firmware */
			ret = brcmf_fil_iovar_data_set(ifp, "arp_hostip_clear",
						       NULL, 0);
			if (ret) {
				bphy_err(drvr, "fail to clear arp ip table err:%d\n",
					 ret);
				return NOTIFY_OK;
			}
			for (i = 0; i < ARPOL_MAX_ENTRIES; i++) {
				if (addr_table[i] == 0)
					continue;
				ret = brcmf_fil_iovar_data_set(ifp, "arp_hostip",
							       &addr_table[i],
							       sizeof(addr_table[i]));
				if (ret)
					bphy_err(drvr, "add arp ip err %d\n",
						 ret);
			}
		}
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}
#endif

#if IS_ENABLED(CONFIG_IPV6)
static int brcmf_inet6addr_changed(struct notifier_block *nb,
				   unsigned long action, void *data)
{
	struct brcmf_pub *drvr = container_of(nb, struct brcmf_pub,
					      inet6addr_notifier);
	struct inet6_ifaddr *ifa = data;
	struct brcmf_if *ifp;
	int i;
	struct in6_addr *table;

	/* Only handle primary interface */
	ifp = drvr->iflist[0];
	if (!ifp)
		return NOTIFY_DONE;
	if (ifp->ndev != ifa->idev->dev)
		return NOTIFY_DONE;

	table = ifp->ipv6_addr_tbl;
	for (i = 0; i < NDOL_MAX_ENTRIES; i++)
		if (ipv6_addr_equal(&ifa->addr, &table[i]))
			break;

	switch (action) {
	case NETDEV_UP:
		if (i == NDOL_MAX_ENTRIES) {
			if (ifp->ipv6addr_idx < NDOL_MAX_ENTRIES) {
				table[ifp->ipv6addr_idx++] = ifa->addr;
			} else {
				for (i = 0; i < NDOL_MAX_ENTRIES - 1; i++)
					table[i] = table[i + 1];
				table[NDOL_MAX_ENTRIES - 1] = ifa->addr;
			}
		}
		break;
	case NETDEV_DOWN:
		if (i < NDOL_MAX_ENTRIES) {
			for (; i < ifp->ipv6addr_idx - 1; i++)
				table[i] = table[i + 1];
			memset(&table[i], 0, sizeof(table[i]));
			ifp->ipv6addr_idx--;
		}
		break;
	default:
		break;
	}

	schedule_work(&ifp->ndoffload_work);

	return NOTIFY_OK;
}
#endif

static int brcmf_revinfo_read(struct seq_file *s, void *data)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(s->private);
	struct brcmf_rev_info *ri = &bus_if->drvr->revinfo;
	char drev[BRCMU_DOTREV_LEN];
	char brev[BRCMU_BOARDREV_LEN];

	seq_printf(s, "vendorid: 0x%04x\n", ri->vendorid);
	seq_printf(s, "deviceid: 0x%04x\n", ri->deviceid);
	seq_printf(s, "radiorev: %s\n", brcmu_dotrev_str(ri->radiorev, drev));
	seq_printf(s, "chip: %s\n", ri->chipname);
	seq_printf(s, "chippkg: %u\n", ri->chippkg);
	seq_printf(s, "corerev: %u\n", ri->corerev);
	seq_printf(s, "boardid: 0x%04x\n", ri->boardid);
	seq_printf(s, "boardvendor: 0x%04x\n", ri->boardvendor);
	seq_printf(s, "boardrev: %s\n", brcmu_boardrev_str(ri->boardrev, brev));
	seq_printf(s, "driverrev: %s\n", brcmu_dotrev_str(ri->driverrev, drev));
	seq_printf(s, "ucoderev: %u\n", ri->ucoderev);
	seq_printf(s, "bus: %u\n", ri->bus);
	seq_printf(s, "phytype: %u\n", ri->phytype);
	seq_printf(s, "phyrev: %u\n", ri->phyrev);
	seq_printf(s, "anarev: %u\n", ri->anarev);
	seq_printf(s, "nvramrev: %08x\n", ri->nvramrev);

	seq_printf(s, "clmver: %s\n", bus_if->drvr->clmver);

	return 0;
}

/*
 * Serialize arming from debugfs reset and brcmf_fw_crashed() against
 * teardown.  The remove path sets ->removing and drains the work while
 * holding bus_reset_lock, so a racing armer is either drained or skips it.
 */
static void brcmf_bus_schedule_reset(struct brcmf_bus *bus_if)
{
	mutex_lock(&bus_if->bus_reset_lock);
	if (bus_if->drvr && bus_if->drvr->bus_reset.func && !bus_if->removing)
		schedule_work(&bus_if->drvr->bus_reset);
	mutex_unlock(&bus_if->bus_reset_lock);
}

void brcmf_bus_cancel_reset_work(struct brcmf_bus *bus_if)
{
	mutex_lock(&bus_if->bus_reset_lock);
	bus_if->removing = true;
	if (bus_if->drvr)
		cancel_work_sync(&bus_if->drvr->bus_reset);
	mutex_unlock(&bus_if->bus_reset_lock);
}

void brcmf_bus_allow_reset_work(struct brcmf_bus *bus_if)
{
	mutex_lock(&bus_if->bus_reset_lock);
	bus_if->removing = false;
	mutex_unlock(&bus_if->bus_reset_lock);
}

static void brcmf_core_bus_reset(struct work_struct *work)
{
	struct brcmf_pub *drvr = container_of(work, struct brcmf_pub,
					      bus_reset);

	brcmf_bus_reset(drvr->bus_if);
}

static ssize_t bus_reset_write(struct file *file, const char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct brcmf_pub *drvr = file->private_data;
	u8 value;

	if (kstrtou8_from_user(user_buf, count, 0, &value))
		return -EINVAL;

	if (value != 1)
		return -EINVAL;

	brcmf_bus_schedule_reset(drvr->bus_if);

	return count;
}

static const struct file_operations bus_reset_fops = {
	.open	= simple_open,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,2)
	.llseek	= no_llseek,
#else
	.llseek	= noop_llseek,
#endif
	.write	= bus_reset_write,
};

/* Replay a monitor session that a wedge-reset tore down. Scheduled from
 * brcmf_bus_started() once the fresh stack is up; runs in workqueue context so
 * brcmf_mon_restore_vif() can take rtnl and recreate + open the vif. The intent
 * stays set on failure so a subsequent re-probe retries, and is re-affirmed by
 * brcmf_net_mon_open() when the recreated interface comes up. */
static void brcmf_core_mon_restore(struct work_struct *work)
{
	struct brcmf_pub *drvr = container_of(work, struct brcmf_pub,
					      mon_restore_work);
	char name[IFNAMSIZ];
	int err;

	if (!nexmon_mon_restore_take(name, sizeof(name)))
		return;

	if (drvr->bus_if->state != BRCMF_BUS_UP) {
		brcmf_err("mon restore: bus not up, skipping \"%s\"\n", name);
		return;
	}

	brcmf_dbg(TRACE, "restoring monitor iface \"%s\" after reset\n", name);

	err = brcmf_mon_restore_vif(drvr, name);
	if (err)
		brcmf_err("mon restore: failed to recreate \"%s\" (%d)\n",
			  name, err);
}

static int brcmf_bus_started(struct brcmf_pub *drvr, struct cfg80211_ops *ops)
{
	int ret = -1;
	struct brcmf_bus *bus_if = drvr->bus_if;
	struct brcmf_if *ifp;
	struct brcmf_if *p2p_ifp;

	brcmf_dbg(TRACE, "\n");

	/* add primary networking interface */
	ifp = brcmf_add_if(drvr, 0, 0, false, "wlan%d",
			   is_valid_ether_addr(drvr->settings->mac) ? drvr->settings->mac : NULL);
	if (IS_ERR(ifp))
		return PTR_ERR(ifp);

	p2p_ifp = NULL;

	/* signal bus ready */
	brcmf_bus_change_state(bus_if, BRCMF_BUS_UP);

	/* do bus specific preinit here */
	ret = brcmf_bus_preinit(bus_if);
	if (ret < 0)
		goto fail;

	/* Bus is ready, do any initialization */
	ret = brcmf_c_preinit_dcmds(ifp);
	if (ret < 0)
		goto fail;

	brcmf_feat_attach(drvr);

	ret = brcmf_proto_init_done(drvr);
	if (ret < 0)
		goto fail;

	brcmf_proto_add_if(drvr, ifp);

	drvr->config = brcmf_cfg80211_attach(drvr, ops,
					     drvr->settings->p2p_enable);
	if (drvr->config == NULL) {
		ret = -ENOMEM;
		goto fail;
	}

	ret = brcmf_net_attach(ifp, false);

	if ((!ret) && (drvr->settings->p2p_enable)) {
		p2p_ifp = drvr->iflist[1];
		if (p2p_ifp)
			ret = brcmf_net_p2p_attach(p2p_ifp);
	}

	if (ret)
		goto fail;

#ifdef CONFIG_INET
	drvr->inetaddr_notifier.notifier_call = brcmf_inetaddr_changed;
	ret = register_inetaddr_notifier(&drvr->inetaddr_notifier);
	if (ret)
		goto fail;

#if IS_ENABLED(CONFIG_IPV6)
	drvr->inet6addr_notifier.notifier_call = brcmf_inet6addr_changed;
	ret = register_inet6addr_notifier(&drvr->inet6addr_notifier);
	if (ret) {
		unregister_inetaddr_notifier(&drvr->inetaddr_notifier);
		goto fail;
	}
#endif
#endif /* CONFIG_INET */

	INIT_WORK(&drvr->bus_reset, brcmf_core_bus_reset);
	INIT_WORK(&drvr->mon_restore_work, brcmf_core_mon_restore);

	/* If a monitor session was live when a wedge-reset tore the previous
	 * incarnation down, replay it now that the stack is back up. Deferred to
	 * a workqueue because the recreate takes rtnl and opens the vif. */
	if (nexmon_mon_restore_pending())
		schedule_work(&drvr->mon_restore_work);

	/* populate debugfs */
	brcmf_debugfs_add_entry(drvr, "revinfo", brcmf_revinfo_read);
	debugfs_create_file("reset", 0600, brcmf_debugfs_get_devdir(drvr), drvr,
			    &bus_reset_fops);
	brcmf_feat_debugfs_create(drvr);
	brcmf_proto_debugfs_create(drvr);
	brcmf_bus_debugfs_create(bus_if);

	return 0;

fail:
	bphy_err(drvr, "failed: %d\n", ret);
	if (drvr->config) {
		brcmf_cfg80211_detach(drvr->config);
		drvr->config = NULL;
	}
	brcmf_net_detach(ifp->ndev, false);
	if (p2p_ifp)
		brcmf_net_detach(p2p_ifp->ndev, false);
	drvr->iflist[0] = NULL;
	drvr->iflist[1] = NULL;
	if (drvr->settings->ignore_probe_fail)
		ret = 0;

	return ret;
}

int brcmf_alloc(struct device *dev, struct brcmf_mp_device *settings)
{
	struct wiphy *wiphy;
	struct cfg80211_ops *ops;
	struct brcmf_pub *drvr = NULL;

	brcmf_dbg(TRACE, "Enter\n");

	ops = brcmf_cfg80211_get_ops(settings);
	if (!ops)
		return -ENOMEM;

	wiphy = wiphy_new(ops, sizeof(*drvr));
	if (!wiphy) {
		kfree(ops);
		return -ENOMEM;
	}

	set_wiphy_dev(wiphy, dev);
	drvr = wiphy_priv(wiphy);
	drvr->wiphy = wiphy;
	drvr->ops = ops;
	drvr->bus_if = dev_get_drvdata(dev);
	drvr->bus_if->drvr = drvr;
	drvr->settings = settings;

	return 0;
}

int brcmf_attach(struct device *dev)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;
	int ret = 0;
	int i;

	brcmf_dbg(TRACE, "Enter\n");

	for (i = 0; i < ARRAY_SIZE(drvr->if2bss); i++)
		drvr->if2bss[i] = BRCMF_BSSIDX_INVALID;

	mutex_init(&drvr->proto_block);

	/* Link to bus module */
	drvr->hdrlen = 0;

	/* Attach and link in the protocol */
	ret = brcmf_proto_attach(drvr);
	if (ret != 0) {
		bphy_err(drvr, "brcmf_prot_attach failed\n");
		goto fail;
	}

	/* Attach to events important for core code */
	brcmf_fweh_register(drvr, BRCMF_E_PSM_WATCHDOG,
			    brcmf_psm_watchdog_notify);

	/* attach firmware event handler */
	brcmf_fweh_attach(drvr);

	ret = brcmf_bus_started(drvr, drvr->ops);
	if (ret != 0) {
		bphy_err(drvr, "dongle is not responding: err=%d\n", ret);
		goto fail;
	}

	return 0;

fail:
	brcmf_detach(dev);

	return ret;
}

void brcmf_bus_add_txhdrlen(struct device *dev, uint len)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	if (drvr) {
		drvr->hdrlen += len;
	}
}

void brcmf_dev_reset(struct device *dev)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	if (drvr == NULL)
		return;

	if (drvr->iflist[0])
		brcmf_fil_cmd_int_set(drvr->iflist[0], BRCMF_C_TERMINATED, 1);
}

void brcmf_dev_coredump(struct device *dev)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);

	if (brcmf_debug_create_memdump(bus_if, NULL, 0) < 0)
		brcmf_dbg(TRACE, "failed to create coredump\n");
}

void brcmf_fw_crashed(struct device *dev)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr;

	/* May fire before brcmf_attach() wires up drvr, or after removal
	 * has cleared it; guard the derefs below (and the arming gate in
	 * brcmf_bus_schedule_reset() already checks drvr/->removing).
	 */
	if (!bus_if)
		return;
	drvr = bus_if->drvr;
	if (!drvr)
		return;

	bphy_err(drvr, "Firmware has halted or crashed\n");

	brcmf_dev_coredump(dev);

	brcmf_bus_schedule_reset(bus_if);
}

void brcmf_detach(struct device *dev)
{
	s32 i;
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	brcmf_dbg(TRACE, "Enter\n");

	if (drvr == NULL)
		return;

#ifdef CONFIG_INET
	unregister_inetaddr_notifier(&drvr->inetaddr_notifier);
#endif

#if IS_ENABLED(CONFIG_IPV6)
	unregister_inet6addr_notifier(&drvr->inet6addr_notifier);
#endif

	brcmf_bus_change_state(bus_if, BRCMF_BUS_DOWN);

	/* Make sure no monitor recreate is queued or mid-flight while we tear
	 * the stack down. The intent flag is untouched, so a following re-probe
	 * re-arms the replay if the session was still meant to be live. */
	cancel_work_sync(&drvr->mon_restore_work);

	WRITE_ONCE(nexmon_default_ifindex, 0);

	/* make sure primary interface removed last */
	for (i = BRCMF_MAX_IFS - 1; i > -1; i--) {
		struct brcmf_if *ifp = drvr->iflist[i];

		if (!ifp)
			continue;

		if (ifp->ndev) {
			brcmf_remove_interface(ifp, false);
			continue;
		}

		/* The P2P device interface has no netdev. Its removal can
		 * race with NL80211_CMD_DEL_INTERFACE issued by a user space
		 * process that is exiting (e.g. wpa_supplicant), which ends
		 * in brcmf_p2p_del_vif() under RTNL and the wiphy mutex. If
		 * both paths remove the interface, the wdev is unregistered
		 * twice and the vif is used after being freed. Take the same
		 * locks and re-read iflist, so only one path removes it.
		 */
		rtnl_lock();
		wiphy_lock(drvr->wiphy);
		ifp = drvr->iflist[i];
		if (ifp)
			brcmf_remove_interface(ifp, true);
		wiphy_unlock(drvr->wiphy);
		rtnl_unlock();
	}
	brcmf_bus_stop(drvr->bus_if);

	brcmf_fweh_detach(drvr);
	brcmf_proto_detach(drvr);

	if (drvr->mon_if) {
		struct brcmf_if *mon = drvr->mon_if;
		WRITE_ONCE(drvr->mon_if, NULL);
		brcmf_net_detach(mon->ndev, false);
	}

	if (drvr->config) {
		brcmf_p2p_detach(&drvr->config->p2p);
		brcmf_cfg80211_detach(drvr->config);
		drvr->config = NULL;
	}
}

void brcmf_free(struct device *dev)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_pub *drvr = bus_if->drvr;

	if (!drvr)
		return;

	bus_if->drvr = NULL;

	kfree(drvr->ops);

	wiphy_free(drvr->wiphy);
}

s32 brcmf_iovar_data_set(struct device *dev, char *name, void *data, u32 len)
{
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_if *ifp = bus_if->drvr->iflist[0];

	return brcmf_fil_iovar_data_set(ifp, name, data, len);
}

static int brcmf_get_pend_8021x_cnt(struct brcmf_if *ifp)
{
	return atomic_read(&ifp->pend_8021x_cnt);
}

int brcmf_netdev_wait_pend8021x(struct brcmf_if *ifp)
{
	struct brcmf_pub *drvr = ifp->drvr;
	int err;

	err = wait_event_timeout(ifp->pend_8021x_wait,
				 !brcmf_get_pend_8021x_cnt(ifp),
				 MAX_WAIT_FOR_8021X_TX);

	if (!err) {
		bphy_err(drvr, "Timed out waiting for no pending 802.1x packets\n");
		atomic_set(&ifp->pend_8021x_cnt, 0);
	}

	return !err;
}

void brcmf_bus_change_state(struct brcmf_bus *bus, enum brcmf_bus_state state)
{
	struct brcmf_pub *drvr = bus->drvr;
	struct net_device *ndev;
	int ifidx;

	brcmf_dbg(TRACE, "%d -> %d\n", bus->state, state);

	if (!drvr) {
		brcmf_dbg(INFO, "ignoring transition, bus not attached yet\n");
		return;
	}

	bus->state = state;

	if (state == BRCMF_BUS_UP) {
		for (ifidx = 0; ifidx < BRCMF_MAX_IFS; ifidx++) {
			if ((drvr->iflist[ifidx]) &&
			    (drvr->iflist[ifidx]->ndev)) {
				ndev = drvr->iflist[ifidx]->ndev;
				if (netif_queue_stopped(ndev))
					netif_wake_queue(ndev);
			}
		}
	}
}

int __init brcmf_core_init(void)
{
	int err;

	err = brcmf_sdio_register();
	if (err)
		return err;

	err = brcmf_usb_register();
	if (err)
		goto error_usb_register;

	err = brcmf_pcie_register();
	if (err)
		goto error_pcie_register;

	/* NEXMON netlink init */
	cfg.input = nexmon_nl_ioctl_handler;
	nl_sock = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
	if (!nl_sock) {
		brcmf_err("NEXMON: %s: Error creating netlink socket\n", __FUNCTION__);
		err = -ENOMEM;
		goto error_netlink_create;
	}

	/* NEXMON Generic Netlink CSI streaming init */
	err = genl_register_family(&nexmon_csi_gnl_family);
	if (err) {
		pr_warn("brcmfmac: NEXMON: failed to register genl family nexmon_csi: %d\n", err);
	} else {
		nexmon_csi_gnl_registered = true;
	}

	return 0;

error_netlink_create:
	brcmf_pcie_exit();
error_pcie_register:
	brcmf_usb_exit();
error_usb_register:
	brcmf_sdio_exit();
	return err;
}

void __exit brcmf_core_exit(void)
{
	/* NEXMON */
	if (nexmon_csi_gnl_registered) {
		genl_unregister_family(&nexmon_csi_gnl_family);
		nexmon_csi_gnl_registered = false;
	}

	if (nl_sock) {
		netlink_kernel_release(nl_sock);
		nl_sock = NULL;
	}

	brcmf_sdio_exit();
	brcmf_usb_exit();
	brcmf_pcie_exit();
}
