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

extern void proto_register_ams(void);
extern void proto_register_ecat(void);
extern void proto_register_ecat_mailbox(void);
extern void proto_register_esl(void);
extern void proto_register_ethercat_frame(void);
extern void proto_register_ioraw(void);
extern void proto_register_nv(void);

/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_DEF void
plugin_register (void)
{
    proto_register_ams();
    proto_register_ecat();
    proto_register_ecat_mailbox();
    proto_register_esl();
    proto_register_ethercat_frame();
    proto_register_ioraw();
    proto_register_nv();
}

extern void proto_reg_handoff_ams(void);
extern void proto_reg_handoff_ecat(void);
extern void proto_reg_handoff_ecat_mailbox(void);
extern void proto_reg_handoff_esl(void);
extern void proto_reg_handoff_ethercat_frame(void);
extern void proto_reg_handoff_ioraw(void);
extern void proto_reg_handoff_nv(void);

WS_DLL_PUBLIC_DEF void plugin_reg_handoff(void);

WS_DLL_PUBLIC_DEF void
plugin_reg_handoff(void)
{
    proto_reg_handoff_ams();
    proto_reg_handoff_ecat();
    proto_reg_handoff_ecat_mailbox();
    proto_reg_handoff_esl();
    proto_reg_handoff_ethercat_frame();
    proto_reg_handoff_ioraw();
    proto_reg_handoff_nv();
}
#endif
