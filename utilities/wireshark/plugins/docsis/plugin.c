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

extern void proto_register_cmctrl_tlv(void);
extern void proto_register_docsis(void);
extern void proto_register_docsis_bintrngreq(void);
extern void proto_register_docsis_bpkmattr(void);
extern void proto_register_docsis_bpkmreq(void);
extern void proto_register_docsis_bpkmrsp(void);
extern void proto_register_docsis_cmctrlreq(void);
extern void proto_register_docsis_cmctrlrsp(void);
extern void proto_register_docsis_cmstatus(void);
extern void proto_register_docsis_dbcack(void);
extern void proto_register_docsis_dbcreq(void);
extern void proto_register_docsis_dbcrsp(void);
extern void proto_register_docsis_dccack(void);
extern void proto_register_docsis_dccreq(void);
extern void proto_register_docsis_dccrsp(void);
extern void proto_register_docsis_dcd(void);
extern void proto_register_docsis_dpd(void);
extern void proto_register_docsis_dpvreq(void);
extern void proto_register_docsis_dpvrsp(void);
extern void proto_register_docsis_dsaack(void);
extern void proto_register_docsis_dsareq(void);
extern void proto_register_docsis_dsarsp(void);
extern void proto_register_docsis_dscack(void);
extern void proto_register_docsis_dscreq(void);
extern void proto_register_docsis_dscrsp(void);
extern void proto_register_docsis_dsdreq(void);
extern void proto_register_docsis_dsdrsp(void);
extern void proto_register_docsis_intrngreq(void);
extern void proto_register_docsis_map(void);
extern void proto_register_docsis_mdd(void);
extern void proto_register_docsis_mgmt(void);
extern void proto_register_docsis_ocd(void);
extern void proto_register_docsis_regack(void);
extern void proto_register_docsis_regreq(void);
extern void proto_register_docsis_regreqmp(void);
extern void proto_register_docsis_regrsp(void);
extern void proto_register_docsis_regrspmp(void);
extern void proto_register_docsis_rngreq(void);
extern void proto_register_docsis_rngrsp(void);
extern void proto_register_docsis_sync(void);
extern void proto_register_docsis_tlv(void);
extern void proto_register_docsis_type29ucd(void);
extern void proto_register_docsis_type35ucd(void);
extern void proto_register_docsis_uccreq(void);
extern void proto_register_docsis_uccrsp(void);
extern void proto_register_docsis_ucd(void);
extern void proto_register_docsis_vsif(void);

/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_DEF void
plugin_register (void)
{
    proto_register_cmctrl_tlv();
    proto_register_docsis();
    proto_register_docsis_bintrngreq();
    proto_register_docsis_bpkmattr();
    proto_register_docsis_bpkmreq();
    proto_register_docsis_bpkmrsp();
    proto_register_docsis_cmctrlreq();
    proto_register_docsis_cmctrlrsp();
    proto_register_docsis_cmstatus();
    proto_register_docsis_dbcack();
    proto_register_docsis_dbcreq();
    proto_register_docsis_dbcrsp();
    proto_register_docsis_dccack();
    proto_register_docsis_dccreq();
    proto_register_docsis_dccrsp();
    proto_register_docsis_dcd();
    proto_register_docsis_dpd();
    proto_register_docsis_dpvreq();
    proto_register_docsis_dpvrsp();
    proto_register_docsis_dsaack();
    proto_register_docsis_dsareq();
    proto_register_docsis_dsarsp();
    proto_register_docsis_dscack();
    proto_register_docsis_dscreq();
    proto_register_docsis_dscrsp();
    proto_register_docsis_dsdreq();
    proto_register_docsis_dsdrsp();
    proto_register_docsis_intrngreq();
    proto_register_docsis_map();
    proto_register_docsis_mdd();
    proto_register_docsis_mgmt();
    proto_register_docsis_ocd();
    proto_register_docsis_regack();
    proto_register_docsis_regreq();
    proto_register_docsis_regreqmp();
    proto_register_docsis_regrsp();
    proto_register_docsis_regrspmp();
    proto_register_docsis_rngreq();
    proto_register_docsis_rngrsp();
    proto_register_docsis_sync();
    proto_register_docsis_tlv();
    proto_register_docsis_type29ucd();
    proto_register_docsis_type35ucd();
    proto_register_docsis_uccreq();
    proto_register_docsis_uccrsp();
    proto_register_docsis_ucd();
    proto_register_docsis_vsif();
}

extern void proto_reg_handoff_cmctrl_tlv(void);
extern void proto_reg_handoff_docsis(void);
extern void proto_reg_handoff_docsis_bintrngreq(void);
extern void proto_reg_handoff_docsis_bpkmattr(void);
extern void proto_reg_handoff_docsis_bpkmreq(void);
extern void proto_reg_handoff_docsis_bpkmrsp(void);
extern void proto_reg_handoff_docsis_cmctrlreq(void);
extern void proto_reg_handoff_docsis_cmctrlrsp(void);
extern void proto_reg_handoff_docsis_cmstatus(void);
extern void proto_reg_handoff_docsis_dbcack(void);
extern void proto_reg_handoff_docsis_dbcreq(void);
extern void proto_reg_handoff_docsis_dbcrsp(void);
extern void proto_reg_handoff_docsis_dccack(void);
extern void proto_reg_handoff_docsis_dccreq(void);
extern void proto_reg_handoff_docsis_dccrsp(void);
extern void proto_reg_handoff_docsis_dcd(void);
extern void proto_reg_handoff_docsis_dpd(void);
extern void proto_reg_handoff_docsis_dpvreq(void);
extern void proto_reg_handoff_docsis_dpvrsp(void);
extern void proto_reg_handoff_docsis_dsaack(void);
extern void proto_reg_handoff_docsis_dsareq(void);
extern void proto_reg_handoff_docsis_dsarsp(void);
extern void proto_reg_handoff_docsis_dscack(void);
extern void proto_reg_handoff_docsis_dscreq(void);
extern void proto_reg_handoff_docsis_dscrsp(void);
extern void proto_reg_handoff_docsis_dsdreq(void);
extern void proto_reg_handoff_docsis_dsdrsp(void);
extern void proto_reg_handoff_docsis_intrngreq(void);
extern void proto_reg_handoff_docsis_map(void);
extern void proto_reg_handoff_docsis_mdd(void);
extern void proto_reg_handoff_docsis_mgmt(void);
extern void proto_reg_handoff_docsis_ocd(void);
extern void proto_reg_handoff_docsis_regack(void);
extern void proto_reg_handoff_docsis_regreq(void);
extern void proto_reg_handoff_docsis_regreqmp(void);
extern void proto_reg_handoff_docsis_regrsp(void);
extern void proto_reg_handoff_docsis_regrspmp(void);
extern void proto_reg_handoff_docsis_rngreq(void);
extern void proto_reg_handoff_docsis_rngrsp(void);
extern void proto_reg_handoff_docsis_sync(void);
extern void proto_reg_handoff_docsis_tlv(void);
extern void proto_reg_handoff_docsis_type29ucd(void);
extern void proto_reg_handoff_docsis_type35ucd(void);
extern void proto_reg_handoff_docsis_uccreq(void);
extern void proto_reg_handoff_docsis_uccrsp(void);
extern void proto_reg_handoff_docsis_ucd(void);
extern void proto_reg_handoff_docsis_vsif(void);

WS_DLL_PUBLIC_DEF void plugin_reg_handoff(void);

WS_DLL_PUBLIC_DEF void
plugin_reg_handoff(void)
{
    proto_reg_handoff_cmctrl_tlv();
    proto_reg_handoff_docsis();
    proto_reg_handoff_docsis_bintrngreq();
    proto_reg_handoff_docsis_bpkmattr();
    proto_reg_handoff_docsis_bpkmreq();
    proto_reg_handoff_docsis_bpkmrsp();
    proto_reg_handoff_docsis_cmctrlreq();
    proto_reg_handoff_docsis_cmctrlrsp();
    proto_reg_handoff_docsis_cmstatus();
    proto_reg_handoff_docsis_dbcack();
    proto_reg_handoff_docsis_dbcreq();
    proto_reg_handoff_docsis_dbcrsp();
    proto_reg_handoff_docsis_dccack();
    proto_reg_handoff_docsis_dccreq();
    proto_reg_handoff_docsis_dccrsp();
    proto_reg_handoff_docsis_dcd();
    proto_reg_handoff_docsis_dpd();
    proto_reg_handoff_docsis_dpvreq();
    proto_reg_handoff_docsis_dpvrsp();
    proto_reg_handoff_docsis_dsaack();
    proto_reg_handoff_docsis_dsareq();
    proto_reg_handoff_docsis_dsarsp();
    proto_reg_handoff_docsis_dscack();
    proto_reg_handoff_docsis_dscreq();
    proto_reg_handoff_docsis_dscrsp();
    proto_reg_handoff_docsis_dsdreq();
    proto_reg_handoff_docsis_dsdrsp();
    proto_reg_handoff_docsis_intrngreq();
    proto_reg_handoff_docsis_map();
    proto_reg_handoff_docsis_mdd();
    proto_reg_handoff_docsis_mgmt();
    proto_reg_handoff_docsis_ocd();
    proto_reg_handoff_docsis_regack();
    proto_reg_handoff_docsis_regreq();
    proto_reg_handoff_docsis_regreqmp();
    proto_reg_handoff_docsis_regrsp();
    proto_reg_handoff_docsis_regrspmp();
    proto_reg_handoff_docsis_rngreq();
    proto_reg_handoff_docsis_rngrsp();
    proto_reg_handoff_docsis_sync();
    proto_reg_handoff_docsis_tlv();
    proto_reg_handoff_docsis_type29ucd();
    proto_reg_handoff_docsis_type35ucd();
    proto_reg_handoff_docsis_uccreq();
    proto_reg_handoff_docsis_uccrsp();
    proto_reg_handoff_docsis_ucd();
    proto_reg_handoff_docsis_vsif();
}
#endif
