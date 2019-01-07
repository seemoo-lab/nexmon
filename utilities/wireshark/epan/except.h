/*
 * Portable Exception Handling for ANSI C.
 * Copyright (C) 1999 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 */

/**
 * @file
 * Portable Exception Handling for ANSI C.<BR>
 * Modified to support throwing an exception with a null message pointer,
 * and to have the message not be const (as we generate messages with
 * "g_strdup_sprintf()", which means they need to be freed; using
 * a null message means that we don't have to use a special string
 * for exceptions with no message, and don't have to worry about
 * not freeing that).
 */

#ifndef XCEPT_H
#define XCEPT_H

#include <glib.h>
#include <setjmp.h>
#include <stdlib.h>
#include <assert.h>
#include "ws_symbol_export.h"

#define XCEPT_GROUP_ANY 0
#define XCEPT_CODE_ANY  0
#define XCEPT_BAD_ALLOC 1

#ifdef __cplusplus
extern "C" {
#endif

enum { except_no_call, except_call };

typedef struct {
    unsigned long except_group;
    unsigned long except_code;
} except_id_t;

typedef struct {
    except_id_t volatile except_id;
    const char *volatile except_message;
    void *volatile except_dyndata;
} except_t;

struct except_cleanup {
    void (*except_func)(void *);
    void *except_context;
};

struct except_catch {
    const except_id_t *except_id;
    size_t except_size;
    except_t except_obj;
    jmp_buf except_jmp;
};

enum except_stacktype {
    XCEPT_CLEANUP, XCEPT_CATCHER
};

struct except_stacknode {
    struct except_stacknode *except_down;
    enum except_stacktype except_type;
    union {
        struct except_catch *except_catcher;
        struct except_cleanup *except_cleanup;
    } except_info;
};

/* private functions made external so they can be used in macros */
extern void except_setup_clean(struct except_stacknode *,
        struct except_cleanup *, void (*)(void *), void *);
WS_DLL_PUBLIC void except_setup_try(struct except_stacknode *,
        struct except_catch *, const except_id_t [], size_t);
WS_DLL_PUBLIC struct except_stacknode *except_pop(void);

/* public interface functions */
WS_DLL_PUBLIC int except_init(void);
WS_DLL_PUBLIC void except_deinit(void);
WS_DLL_PUBLIC WS_NORETURN void except_rethrow(except_t *);
WS_DLL_PUBLIC WS_NORETURN void except_throw(long, long, const char *);
WS_DLL_PUBLIC WS_NORETURN void except_throwd(long, long, const char *, void *);
WS_DLL_PUBLIC WS_NORETURN void except_throwf(long, long, const char *, ...)
    G_GNUC_PRINTF(3, 4);
WS_DLL_PUBLIC void (*except_unhandled_catcher(void (*)(except_t *)))(except_t *);
extern unsigned long except_code(except_t *);
extern unsigned long except_group(except_t *);
extern const char *except_message(except_t *);
extern void *except_data(except_t *);
WS_DLL_PUBLIC void *except_take_data(except_t *);
WS_DLL_PUBLIC void except_set_allocator(void *(*)(size_t), void (*)(void *));
WS_DLL_PUBLIC void *except_alloc(size_t);
WS_DLL_PUBLIC void except_free(void *);

#define except_code(E) ((E)->except_id.except_code)
#define except_group(E) ((E)->except_id.except_group)
#define except_message(E) ((E)->except_message)
#define except_data(E) ((E)->except_dyndata)

#ifdef __cplusplus
}
#endif

/*
 * void except_cleanup_push(void (*)(void *), void *);
 * void except_cleanup_pop(int);
 * void except_checked_cleanup_pop(void (*)(void *), int);
 * void except_try_push(const except_id_t [], size_t, except_t **);
 * void except_try_pop(void);
 */

#define except_cleanup_push(F, C)                               \
    {                                                           \
        struct except_stacknode except_sn;                      \
        struct except_cleanup except_cl;                        \
        except_setup_clean(&except_sn, &except_cl, F, C)

#define except_cleanup_pop(E)                                   \
        except_pop();                                           \
        if (E)                                                  \
            except_cl.except_func(except_cl.except_context);    \
    }

#define except_checked_cleanup_pop(F, E)                        \
            except_pop();                                       \
        assert (except_cl.except_func == (F));                  \
        if (E)                                                  \
            except_cl.except_func(except_cl.except_context);    \
    }


/* --- Variants to allow nesting of except_cleanup_push w/o "shadowing" variables */
#define except_cleanup_push_pfx(pfx, F, C)                      \
    {                                                           \
        struct except_stacknode pfx##_except_sn;                \
        struct except_cleanup pfx##_except_cl;                  \
        except_setup_clean(&pfx##_except_sn, &pfx##_except_cl, F, C)

#define except_cleanup_pop_pfx(pfx, E)                          \
        except_pop();                                           \
        if (E)                                                  \
            pfx##_except_cl.except_func(pfx##_except_cl.except_context);\
    }

#define except_checked_cleanup_pop_pfx(pfx, F, E)               \
            except_pop();                                       \
        assert (pfx##_except_cl.except_func == (F));            \
        if (E)                                                  \
            pfx##_except_cl.except_func(pfx##_except_cl.except_context);\
    }
/* ---------- */


#define except_try_push(ID, NUM, PPE)                           \
     {                                                          \
        struct except_stacknode except_sn;                      \
        struct except_catch except_ch;                          \
        except_setup_try(&except_sn, &except_ch, ID, NUM);      \
        if (setjmp(except_ch.except_jmp))                       \
            *(PPE) = &except_ch.except_obj;                     \
        else                                                    \
            *(PPE) = 0

#define except_try_pop()                                        \
        except_free(except_ch.except_obj.except_dyndata);       \
        except_pop();                                           \
    }

#endif /* XCEPT_H */

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
