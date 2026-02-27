/*
 * Do not modify this file. Changes will be overwritten.
 *
 * Generated automatically from ../../tools/make-dissector-reg.py.
 */

#include "config.h"

#include <gmodule.h>

#include "moduleinfo.h"

/* plugins are DLLs */
#define WS_BUILD_DLL
#include "ws_symbol_export.h"

#ifndef ENABLE_STATIC
WS_DLL_PUBLIC_DEF void plugin_register (void);
WS_DLL_PUBLIC_DEF const gchar version[] = VERSION;

extern void proto_register_mac_header_generic(void);
extern void proto_register_mac_header_type_1(void);
extern void proto_register_mac_header_type_2(void);
extern void proto_register_mac_mgmt_msg(void);
extern void proto_register_mac_mgmt_msg_aas_beam(void);
extern void proto_register_mac_mgmt_msg_aas_fbck(void);
extern void proto_register_mac_mgmt_msg_arq_feedback(void);
extern void proto_register_mac_mgmt_msg_clk_cmp(void);
extern void proto_register_mac_mgmt_msg_dcd(void);
extern void proto_register_mac_mgmt_msg_dlmap(void);
extern void proto_register_mac_mgmt_msg_dreg_cmd(void);
extern void proto_register_mac_mgmt_msg_dreg_req(void);
extern void proto_register_mac_mgmt_msg_dsa(void);
extern void proto_register_mac_mgmt_msg_dsc(void);
extern void proto_register_mac_mgmt_msg_dsd(void);
extern void proto_register_mac_mgmt_msg_dsx_rvd(void);
extern void proto_register_mac_mgmt_msg_fpc(void);
extern void proto_register_mac_mgmt_msg_pkm(void);
extern void proto_register_mac_mgmt_msg_pmc_req(void);
extern void proto_register_mac_mgmt_msg_pmc_rsp(void);
extern void proto_register_mac_mgmt_msg_prc_lt_ctrl(void);
extern void proto_register_mac_mgmt_msg_reg_req(void);
extern void proto_register_mac_mgmt_msg_reg_rsp(void);
extern void proto_register_mac_mgmt_msg_rep(void);
extern void proto_register_mac_mgmt_msg_res_cmd(void);
extern void proto_register_mac_mgmt_msg_rng_req(void);
extern void proto_register_mac_mgmt_msg_rng_rsp(void);
extern void proto_register_mac_mgmt_msg_sbc(void);
extern void proto_register_mac_mgmt_msg_ucd(void);
extern void proto_register_mac_mgmt_msg_ulmap(void);
extern void proto_register_wimax(void);
extern void proto_register_wimax_cdma(void);
extern void proto_register_wimax_compact_dlmap_ie(void);
extern void proto_register_wimax_compact_ulmap_ie(void);
extern void proto_register_wimax_fch(void);
extern void proto_register_wimax_ffb(void);
extern void proto_register_wimax_hack(void);
extern void proto_register_wimax_harq_map(void);
extern void proto_register_wimax_pdu(void);
extern void proto_register_wimax_phy_attributes(void);
extern void proto_register_wimax_utility_decoders(void);

/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_DEF void
plugin_register (void)
{
    proto_register_mac_header_generic();
    proto_register_mac_header_type_1();
    proto_register_mac_header_type_2();
    proto_register_mac_mgmt_msg();
    proto_register_mac_mgmt_msg_aas_beam();
    proto_register_mac_mgmt_msg_aas_fbck();
    proto_register_mac_mgmt_msg_arq_feedback();
    proto_register_mac_mgmt_msg_clk_cmp();
    proto_register_mac_mgmt_msg_dcd();
    proto_register_mac_mgmt_msg_dlmap();
    proto_register_mac_mgmt_msg_dreg_cmd();
    proto_register_mac_mgmt_msg_dreg_req();
    proto_register_mac_mgmt_msg_dsa();
    proto_register_mac_mgmt_msg_dsc();
    proto_register_mac_mgmt_msg_dsd();
    proto_register_mac_mgmt_msg_dsx_rvd();
    proto_register_mac_mgmt_msg_fpc();
    proto_register_mac_mgmt_msg_pkm();
    proto_register_mac_mgmt_msg_pmc_req();
    proto_register_mac_mgmt_msg_pmc_rsp();
    proto_register_mac_mgmt_msg_prc_lt_ctrl();
    proto_register_mac_mgmt_msg_reg_req();
    proto_register_mac_mgmt_msg_reg_rsp();
    proto_register_mac_mgmt_msg_rep();
    proto_register_mac_mgmt_msg_res_cmd();
    proto_register_mac_mgmt_msg_rng_req();
    proto_register_mac_mgmt_msg_rng_rsp();
    proto_register_mac_mgmt_msg_sbc();
    proto_register_mac_mgmt_msg_ucd();
    proto_register_mac_mgmt_msg_ulmap();
    proto_register_wimax();
    proto_register_wimax_cdma();
    proto_register_wimax_compact_dlmap_ie();
    proto_register_wimax_compact_ulmap_ie();
    proto_register_wimax_fch();
    proto_register_wimax_ffb();
    proto_register_wimax_hack();
    proto_register_wimax_harq_map();
    proto_register_wimax_pdu();
    proto_register_wimax_phy_attributes();
    proto_register_wimax_utility_decoders();
}

extern void proto_reg_handoff_mac_header_generic(void);
extern void proto_reg_handoff_mac_mgmt_msg(void);
extern void proto_reg_handoff_mac_mgmt_msg_aas(void);
extern void proto_reg_handoff_mac_mgmt_msg_aas_beam(void);
extern void proto_reg_handoff_mac_mgmt_msg_arq(void);
extern void proto_reg_handoff_mac_mgmt_msg_clk_cmp(void);
extern void proto_reg_handoff_mac_mgmt_msg_dcd(void);
extern void proto_reg_handoff_mac_mgmt_msg_dlmap(void);
extern void proto_reg_handoff_mac_mgmt_msg_dreg(void);
extern void proto_reg_handoff_mac_mgmt_msg_dsa(void);
extern void proto_reg_handoff_mac_mgmt_msg_dsc(void);
extern void proto_reg_handoff_mac_mgmt_msg_dsd(void);
extern void proto_reg_handoff_mac_mgmt_msg_dsx_rvd(void);
extern void proto_reg_handoff_mac_mgmt_msg_fpc(void);
extern void proto_reg_handoff_mac_mgmt_msg_pkm(void);
extern void proto_reg_handoff_mac_mgmt_msg_pmc(void);
extern void proto_reg_handoff_mac_mgmt_msg_prc_lt_ctrl(void);
extern void proto_reg_handoff_mac_mgmt_msg_reg_req(void);
extern void proto_reg_handoff_mac_mgmt_msg_reg_rsp(void);
extern void proto_reg_handoff_mac_mgmt_msg_rep(void);
extern void proto_reg_handoff_mac_mgmt_msg_res_cmd(void);
extern void proto_reg_handoff_mac_mgmt_msg_rng_req(void);
extern void proto_reg_handoff_mac_mgmt_msg_rng_rsp(void);
extern void proto_reg_handoff_mac_mgmt_msg_sbc(void);
extern void proto_reg_handoff_mac_mgmt_msg_ucd(void);
extern void proto_reg_handoff_mac_mgmt_msg_ulmap(void);
extern void proto_reg_handoff_wimax_pdu(void);

WS_DLL_PUBLIC_DEF void plugin_reg_handoff(void);

WS_DLL_PUBLIC_DEF void
plugin_reg_handoff(void)
{
    proto_reg_handoff_mac_header_generic();
    proto_reg_handoff_mac_mgmt_msg();
    proto_reg_handoff_mac_mgmt_msg_aas();
    proto_reg_handoff_mac_mgmt_msg_aas_beam();
    proto_reg_handoff_mac_mgmt_msg_arq();
    proto_reg_handoff_mac_mgmt_msg_clk_cmp();
    proto_reg_handoff_mac_mgmt_msg_dcd();
    proto_reg_handoff_mac_mgmt_msg_dlmap();
    proto_reg_handoff_mac_mgmt_msg_dreg();
    proto_reg_handoff_mac_mgmt_msg_dsa();
    proto_reg_handoff_mac_mgmt_msg_dsc();
    proto_reg_handoff_mac_mgmt_msg_dsd();
    proto_reg_handoff_mac_mgmt_msg_dsx_rvd();
    proto_reg_handoff_mac_mgmt_msg_fpc();
    proto_reg_handoff_mac_mgmt_msg_pkm();
    proto_reg_handoff_mac_mgmt_msg_pmc();
    proto_reg_handoff_mac_mgmt_msg_prc_lt_ctrl();
    proto_reg_handoff_mac_mgmt_msg_reg_req();
    proto_reg_handoff_mac_mgmt_msg_reg_rsp();
    proto_reg_handoff_mac_mgmt_msg_rep();
    proto_reg_handoff_mac_mgmt_msg_res_cmd();
    proto_reg_handoff_mac_mgmt_msg_rng_req();
    proto_reg_handoff_mac_mgmt_msg_rng_rsp();
    proto_reg_handoff_mac_mgmt_msg_sbc();
    proto_reg_handoff_mac_mgmt_msg_ucd();
    proto_reg_handoff_mac_mgmt_msg_ulmap();
    proto_reg_handoff_wimax_pdu();
}
#endif
