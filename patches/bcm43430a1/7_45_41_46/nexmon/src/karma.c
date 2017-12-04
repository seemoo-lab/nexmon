#pragma NEXMON targetregion "patch"

#include <firmware_version.h>
#include <wrapper.h>    // wrapper definitions for functions that already exist in the firmware
#include <structs.h>    // structures that are used by the code in the firmware
#include <patcher.h>
#include <helper.h>

//#include "d11.h"
//#include "brcm.h"


#include "karma.h"

void print_mac(struct ether_addr addr)
{
	printf("%x:%x:%x:%x:%x:%x", 
		addr.octet[0],
		addr.octet[1],
		addr.octet[2],
		addr.octet[3],
		addr.octet[4],
		addr.octet[5]
	);
}


//Not really a hook, as the legacy method has no implementation (this is why we don't call back to the umodified method)
void wlc_recv_process_prbreq_hook(struct wlc_info *wlc, void *wrxh, uint8 *plcp, struct dot11_management_header *hdr, uint8 *body, int body_len)
{
	uint8 i;

	printf("Entered wlc_recv_process_prbreq_hook\n");
	
	for (i = 0; i < body_len; i++)
	{
		printf("0x%02x ", body[i]);
	}
}

void wlc_recv_mgmt_ctl_hook(struct wlc_info *wlc, void *osh, void *wrxh, void *p)
{
	struct ether_addr cur_addr = wlc->pub->cur_etheraddr;
	uint8 *plcp;
	struct dot11_management_header *hdr;
	uint16 fc, ft, fk;
	char eabuf[ETHER_ADDR_STR_LEN];
//	wlc_bsscfg *bsscfg = NULL;
	//uint8 *body;

	plcp = PKTDATA(osh, p);

	hdr = (struct dot11_management_header*)(plcp + D11_PHY_HDR_LEN);

        fc = ltoh16(hdr->fc); //Account for endianess of frames FC field
        ft = FC_TYPE(fc); //Frame Type (MGMT / CTL / DATA)
        fk = (fc & FC_KIND_MASK); //Frame Kind (ASSOC; PROBEREQUEST etc.)



	printf("wl%d ether %x:%x:%x:%x:%x:%x: wlc_recv_mgmt_ctl\n", 
		wlc->pub->unit,
		cur_addr.octet[0],
		cur_addr.octet[1],
		cur_addr.octet[2],
		cur_addr.octet[3],
		cur_addr.octet[4],
		cur_addr.octet[5]
	);


	printf("Frame data\n===================\n\n");
	printf("FC:\t%04x\n", fc);
	printf("Frame Type:\t%04x\n", ft);
	printf("Frame Kind:\t%04x\n", fk);
	printf("  da:\t\t"); print_mac(hdr->da); printf("\n");
	printf("  sa:\t\t"); print_mac(hdr->sa); printf("\n");
	printf("  bssid:\t"); print_mac(hdr->bssid); printf("\n");

	switch(fk)
	{
		case FC_PROBE_REQ:
			printf("Frame is PROBE REQUEST\n");
			bcm_ether_ntoa(&hdr->sa, eabuf); //Test Firmware String conversion of MAC-Address
			printf("SA %s\n", eabuf);
			break;
	}


	// call legacy method
	wlc_recv_mgmt_ctl(wlc, osh, wrxh, p);
}

__attribute__((at(0x012da2, "", CHIP_VER_BCM43430a1, FW_VER_7_45_41_46)))
BPatch(wlc_recv_mgmt_ctl_hook, wlc_recv_mgmt_ctl_hook);


__attribute__((at(0x00820b9a, "flashpatch", CHIP_VER_BCM43430a1, FW_VER_7_45_41_46)))
BLPatch(wlc_recv_process_prbreq_hook, wlc_recv_process_prbreq_hook);
