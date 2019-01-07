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

extern void proto_register_dcom_cba(void);
extern void proto_register_dcom_cba_acco(void);
extern void proto_register_pn_dcp(void);
extern void proto_register_pn_io(void);
extern void proto_register_pn_mrp(void);
extern void proto_register_pn_mrrt(void);
extern void proto_register_pn_ptcp(void);
extern void proto_register_pn_rt(void);

/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_DEF void
plugin_register (void)
{
    proto_register_dcom_cba();
    proto_register_dcom_cba_acco();
    proto_register_pn_dcp();
    proto_register_pn_io();
    proto_register_pn_mrp();
    proto_register_pn_mrrt();
    proto_register_pn_ptcp();
    proto_register_pn_rt();
}

extern void proto_reg_handoff_dcom_cba(void);
extern void proto_reg_handoff_dcom_cba_acco(void);
extern void proto_reg_handoff_pn_dcp(void);
extern void proto_reg_handoff_pn_io(void);
extern void proto_reg_handoff_pn_mrp(void);
extern void proto_reg_handoff_pn_mrrt(void);
extern void proto_reg_handoff_pn_ptcp(void);
extern void proto_reg_handoff_pn_rt(void);

WS_DLL_PUBLIC_DEF void plugin_reg_handoff(void);

WS_DLL_PUBLIC_DEF void
plugin_reg_handoff(void)
{
    proto_reg_handoff_dcom_cba();
    proto_reg_handoff_dcom_cba_acco();
    proto_reg_handoff_pn_dcp();
    proto_reg_handoff_pn_io();
    proto_reg_handoff_pn_mrp();
    proto_reg_handoff_pn_mrrt();
    proto_reg_handoff_pn_ptcp();
    proto_reg_handoff_pn_rt();
}
#endif
