#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __linux__
//#include <linux/wireless.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/genetlink.h>
#include <linux/nl80211.h>

#else
 #warning NOT COMPILING FOR LINUX
#endif

#include "osdep.h"
#include "./attacks/deauth.h"

#define MAX_CHAN_COUNT 128

struct channel{
	int chan;
	int hop;
};

int lpos_x = 0;
volatile struct channel chans [MAX_CHAN_COUNT] = { {1, 1}, {7, 1}, {13, 1}, {2, 1}, {8, 1}, {3, 1}, {14, 1}, {9, 1}, {4, 1}, {10, 1}, {5, 1}, {11, 1}, {6, 1}, {12, 1}, {0, 0} };

int lpos_in = 0;
int lpos_out = 0;
int chans_in [MAX_CHAN_COUNT] = {0};
int chans_out [MAX_CHAN_COUNT] = {0};

pthread_t *hopper = NULL;
pthread_t chan_sniffer = (int)NULL;
int hopper_useconds = 0;
volatile int sniff = 0;
pthread_mutex_t chan_thread_mutex;

extern char *osdep_iface_in;
extern char *osdep_iface_out;

extern void *global_cur_options;
extern struct attacks *global_cur_attack;

// deauth
extern struct ether_addr mac_block;
extern unsigned char essid_block[33];
extern unsigned char essid_len;



/***********************************nl80211******************************************/

struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};

enum command_identify_by {
	CIB_NONE,
	CIB_PHY,
	CIB_NETDEV,
	CIB_WDEV,
};

enum id_input {
	II_NONE,
	II_NETDEV,
	II_PHY_NAME,
	II_PHY_IDX,
	II_WDEV,
};


struct channels_ctx {
	int last_band;
	bool width_40;
	bool width_80;
	bool width_160;
};

#define BIT(x) (1ULL<<(x))

/* libnl 1.x compatibility code */
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
static inline struct nl_handle *nl_socket_alloc(void)
{
	return nl_handle_alloc();
}

static inline void nl_socket_free(struct nl_sock *h)
{
	nl_handle_destroy(h);
}

static inline int nl_socket_set_buffer_size(struct nl_sock *sk,
					    int rxbuf, int txbuf)
{
	return nl_set_buffer_size(sk, rxbuf, txbuf);
}
#endif /* CONFIG_LIBNL20 && CONFIG_LIBNL30 */

static int nl80211_init(struct nl80211_state *state)
{
	int err;

	state->nl_sock = nl_socket_alloc();
	if (!state->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	if (genl_connect(state->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK;
		goto out_handle_destroy;
	}

	nl_socket_set_buffer_size(state->nl_sock, 8192, 8192);

	state->nl80211_id = genl_ctrl_resolve(state->nl_sock, "nl80211");
	if (state->nl80211_id < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

 out_handle_destroy:
	nl_socket_free(state->nl_sock);
	return err;
}

static void nl80211_cleanup(struct nl80211_state *state)
{
	nl_socket_free(state->nl_sock);
}


static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}

static int (*registered_handler)(struct nl_msg *, void *);
static void *registered_handler_data;

void register_handler(int (*handler)(struct nl_msg *, void *), void *data)
{
	registered_handler = handler;
	registered_handler_data = data;
}

int valid_handler(struct nl_msg *msg, void *arg)
{
	if (registered_handler)
		return registered_handler(msg, registered_handler_data);

	return NL_OK;
}

int ieee80211_channel_to_frequency(int chan, enum nl80211_band band)
{
	/* see 802.11 17.3.8.3.2 and Annex J
	 * there are overlapping channel numbers in 5GHz and 2GHz bands */
	if (chan <= 0)
		return 0; /* not supported */
	switch (band) {
	case NL80211_BAND_2GHZ:
		if (chan == 14)
			return 2484;
		else if (chan < 14)
			return 2407 + chan * 5;
		break;
	case NL80211_BAND_5GHZ:
		if (chan >= 182 && chan <= 196)
			return 4000 + chan * 5;
		else
			return 5000 + chan * 5;
		break;
	case NL80211_BAND_60GHZ:
		if (chan < 5)
			return 56160 + chan * 2160;
		break;
	default:
		;
	}
	return 0; /* not supported */
}

int ieee80211_frequency_to_channel(int freq)
{
	/* see 802.11-2007 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}

static char *dfs_state_name(enum nl80211_dfs_state state)
{
	switch (state) {
	case NL80211_DFS_USABLE:
		return "usable";
	case NL80211_DFS_AVAILABLE:
		return "available";
	case NL80211_DFS_UNAVAILABLE:
		return "unavailable";
	default:
		return "unknown";
	}
}

static int print_channels_handler(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct channels_ctx *ctx = arg;
	struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
	struct nlattr *tb_band[NL80211_BAND_ATTR_MAX + 1];
	struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
	struct nlattr *nl_band;
	struct nlattr *nl_freq;
	int rem_band, rem_freq;


	nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (tb_msg[NL80211_ATTR_WIPHY_BANDS]) {
		nla_for_each_nested(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS], rem_band) {
			if (ctx->last_band != nl_band->nla_type) {

				ctx->width_40 = false;
				ctx->width_80 = false;
				ctx->width_160 = false;
				ctx->last_band = nl_band->nla_type;
			}

			nla_parse(tb_band, NL80211_BAND_ATTR_MAX, nla_data(nl_band), nla_len(nl_band), NULL);

			if (tb_band[NL80211_BAND_ATTR_HT_CAPA]) {
				__u16 cap = nla_get_u16(tb_band[NL80211_BAND_ATTR_HT_CAPA]);

				if (cap & BIT(1))
					ctx->width_40 = true;
			}

			if (tb_band[NL80211_BAND_ATTR_VHT_CAPA]) {
				__u32 capa;

				ctx->width_80 = true;

				capa = nla_get_u32(tb_band[NL80211_BAND_ATTR_VHT_CAPA]);
				switch ((capa >> 2) & 3) {
				case 2:
					/* width_80p80 = true; */
					/* fall through */
				case 1:
					ctx->width_160 = true;
				break;
				}
			}

			if (tb_band[NL80211_BAND_ATTR_FREQS]) {
				nla_for_each_nested(nl_freq, tb_band[NL80211_BAND_ATTR_FREQS], rem_freq) {
					uint32_t freq;

					nla_parse(tb_freq, NL80211_FREQUENCY_ATTR_MAX, nla_data(nl_freq), nla_len(nl_freq), NULL);

					if (!tb_freq[NL80211_FREQUENCY_ATTR_FREQ])
						continue;
					freq = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);

					chans[lpos_x].chan = ieee80211_frequency_to_channel(freq);
					chans[lpos_x].hop = 0;
					lpos_x++;

					if (tb_freq[NL80211_FREQUENCY_ATTR_DISABLED]) {
						continue;
					}
				}
			}
		}
	}

	return NL_SKIP;
}

static int handle_channels(struct nl80211_state *state, struct nl_msg *msg)
{
	static struct channels_ctx ctx = {
		.last_band = -1,
	};

	nla_put_flag(msg, NL80211_ATTR_SPLIT_WIPHY_DUMP);
	nlmsg_hdr(msg)->nlmsg_flags |= NLM_F_DUMP;

	register_handler(print_channels_handler, &ctx);

	return 0;
}

/*************************************************************************/

unsigned char get_channel_from_beacon(struct packet *pkt)
{
	int ie_type;
	int ie_len;
	unsigned char *pie_data;
	int ie_data_len;
	unsigned char channel = 0;

	if(pkt == NULL || pkt->len <= sizeof(struct ieee_hdr) + sizeof(struct beacon_fixed) )
		return 0;

	pie_data = pkt->data + sizeof(struct ieee_hdr) + sizeof(struct beacon_fixed);
	ie_data_len = pkt->len - (sizeof(struct ieee_hdr) + sizeof(struct beacon_fixed));

	while(ie_data_len > 0){

		ie_type = pie_data[0];
		ie_len = pie_data[1];
		if(ie_type == 0x03){ // Tag Number: DS Parameter Set, (channel), b/g

			channel = pie_data[2];

			break;
		}else if(ie_type == 0x3D){ // Tag Number: HT Information , (channel), 802.11n

			channel = pie_data[2];

			break;
		}

		pie_data += (1+1+ie_len);
		ie_data_len -=(1+1+ie_len);
	}

	return channel;
}

void channel_sniff()
{
	struct packet sniffed;
	struct ieee_hdr *hdr;
	struct ether_addr bssid;
	char ssid[32];
	int ie_len;
	unsigned char *pie_data;
	unsigned char channel;
	int i;

	while(sniff) {

		sniffed = osdep_read_packet();
		if (sniffed.len == 0){
			usleep(10);
			continue;
		}

		hdr = (struct ieee_hdr *) sniffed.data;
		if (hdr->type == IEEE80211_TYPE_BEACON){

			// channel
			channel = get_channel_from_beacon(&sniffed);

			// BSSID
			memcpy(bssid.ether_addr_octet, sniffed.data + 16, ETHER_ADDR_LEN);

			pie_data = sniffed.data + sizeof(struct ieee_hdr) + sizeof(struct beacon_fixed);
			// ssid
			ie_len = pie_data[1];
			pie_data += 2;
			memcpy(ssid, pie_data, ie_len);

			if(global_cur_attack->mode_identifier == DEAUTH_MODE){

				if(BLACKLIST_FROM_ESSID == ((struct deauth_options *)global_cur_options)->isblacklist){

					if(ie_len == essid_len){
						if(!memcmp(essid_block, pie_data, essid_len)){

							for(i=0; i<lpos_x; i++){
								if(chans[i].chan == channel){
									chans[i].hop = 1;
								}
							}
						}
					}

					continue;

				}else if(BLACKLIST_FROM_BSSID == ((struct deauth_options *)global_cur_options)->isblacklist){
					if(!memcmp(&bssid, &mac_block, sizeof(struct ether_addr))){

						for(i=0; i<lpos_x; i++){
							if(chans[i].chan == channel){
								chans[i].hop = 1;
							}
						}

						break;
					}
				}

			}
			for(i=0; i<lpos_x; i++){
				if(chans[i].chan == channel){
					chans[i].hop = 1;
				}
			}
		}
	}
}

void nl80211_get_channel_list(char *iface)
{
	struct nl80211_state nlstate, *state;
	int err;
	struct nl_cb *cb;
	struct nl_cb *s_cb;
	struct nl_msg *msg;

	lpos_x = 0;

	err = nl80211_init(&nlstate);
	if (err){
		fprintf(stderr, "failed to initialize nl80211.\n");
		return;
	}

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return;
	}

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	s_cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb || !s_cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		goto out;
	}

	state = &nlstate;

	genlmsg_put(msg, 0, 0, state->nl80211_id, 0, 0, NL80211_CMD_GET_WIPHY, 0);

	NLA_PUT_U32(msg, NL80211_ATTR_IFINDEX, if_nametoindex(iface));

	err = handle_channels(state, msg);
	if (err){
		goto out;
	};

	nl_socket_set_cb(state->nl_sock, s_cb);
	err = nl_send_auto_complete(state->nl_sock, msg);
	if (err < 0){
		goto out;
	};

	err = 1;
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, valid_handler, NULL);

	while (err > 0)
		nl_recvmsgs(state->nl_sock, cb);


nla_put_failure:
out:
	nl_cb_put(cb);
	nl_cb_put(s_cb);
	nlmsg_free(msg);

	nl80211_cleanup(&nlstate);
}

void nl80211_init_channel_list()
{
	int i=0, j=0, p=0;

	nl80211_get_channel_list(osdep_iface_out);
	chans[lpos_x].chan = 0;
	chans[lpos_x].hop = 0;


	if(0!=strcmp(osdep_iface_out, osdep_iface_in)){
		for(i = 0; i<= lpos_x; i++){
			chans_out[i] = chans[i].chan;
		}

		lpos_out = lpos_x;

		nl80211_get_channel_list(osdep_iface_in);
		for(i = 0; i<= lpos_x; i++){
			chans_in[i] = chans[i].chan;
		}

		lpos_in = lpos_x;
		p = 0;

		for(i = 0; i< lpos_out; i++){
			for(j = 0; j< lpos_in; j++){
				if(chans_out[i] == chans_in[j]){
					chans[p].chan = chans_out[i];
					chans[p].hop = 0;
					p++;
				}
			}
		}

		chans[p].chan = 0;
		chans[p].hop = 0;
		lpos_x = p;
	}
}

void channel_hopper()
{
    // A simple thread to hop channels
    int cclp = 0, i;

	if(sniff){
		for(i=0; i<lpos_x; i++){
			osdep_set_channel(chans[i].chan);
			usleep(3000*1000);
		}
		sniff = 0;
	}

    while (1) {
		if(chans[cclp].hop != 0){
			osdep_set_channel(chans[cclp].chan);
		}
		cclp++;
		if (chans[cclp].chan == 0){
			cclp = 0;

		}
		usleep(hopper_useconds);
    }
}

void init_channel_hopper(char *chanlist, int useconds)
{
    // Channel list chans[MAX_CHAN_COUNT] has been initialized with declaration for all b/g channels
    char *token = NULL;
    int chan_cur = EOF;
    int lpos = 0;

    if (hopper) {
      printf("There is already a channel hopper running, skipping this one!\n");
    }

    if (chanlist == NULL) {    // No channel list given - using defaults
#ifdef __linux__
		printf("\nUsing sniffed channels for hopping every %d milliseconds.\n", useconds/1000);
		nl80211_init_channel_list();
		sniff = 1;
		pthread_create(&chan_sniffer, NULL, (void*) channel_sniff, NULL);

#else
		printf("\nUsing default channels for hopping every %d milliseconds.\n", useconds/1000);
#endif
    } else {

	while((token = strsep(&chanlist, ",")) != NULL) {
	    if(sscanf(token, "%d", &chan_cur) != EOF) {
		chans[lpos].chan = chan_cur;
		chans[lpos].hop = 1;
		lpos++;
		if (lpos == MAX_CHAN_COUNT) {
		    fprintf(stderr, "Exceeded max channel list entries, list truncated.\n");
		    break;
		}
	    }
	}

	chans[lpos].chan = 0;
	chans[lpos].hop = 0;
    }

    hopper_useconds = useconds;
    hopper = malloc(sizeof(pthread_t));
    pthread_create(hopper, NULL, (void *) channel_hopper, NULL);

}
