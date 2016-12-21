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

extern void proto_register_ircomm(void);
extern void proto_register_irda(void);
extern void proto_register_irsir(void);

/* Start the functions we need for the plugin stuff */

WS_DLL_PUBLIC_DEF void
plugin_register (void)
{
    proto_register_ircomm();
    proto_register_irda();
    proto_register_irsir();
}

extern void proto_reg_handoff_ircomm(void);
extern void proto_reg_handoff_irda(void);
extern void proto_reg_handoff_irsir(void);

WS_DLL_PUBLIC_DEF void plugin_reg_handoff(void);

WS_DLL_PUBLIC_DEF void
plugin_reg_handoff(void)
{
    proto_reg_handoff_ircomm();
    proto_reg_handoff_irda();
    proto_reg_handoff_irsir();
}
#endif
