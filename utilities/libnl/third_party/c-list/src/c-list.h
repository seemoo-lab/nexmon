#pragma once

/*
 * Circular Intrusive Double Linked List Collection in ISO-C11
 *
 * This implements a generic circular double linked list. List entries must
 * embed the CList object, which provides pointers to the next and previous
 * element. Insertion and removal can be done in O(1) due to the double links.
 * Furthermore, the list is circular, thus allows access to front/tail in O(1)
 * as well, even if you only have a single head pointer (which is not how the
 * list is usually operated, though).
 *
 * Note that you are free to use the list implementation without a head
 * pointer. However, usual operation uses a single CList object as head, which
 * is itself linked in the list and as such must be identified as list head.
 * This allows very simply list operations and avoids a lot of special cases.
 * Most importantly, you can unlink entries without requiring a head pointer.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct CList CList;

/**
 * struct CList - Entry of a circular double linked list
 * @next:               next entry
 * @prev:               previous entry
 *
 * Each entry in a list must embed a CList object. This object contains
 * pointers to its next and previous elements, which can be freely accessed by
 * the API user at any time. Note that the list is circular, and the list head
 * is linked in the list as well.
 *
 * The list head must be initialized via C_LIST_INIT before use. There is no
 * reason to initialize entry objects before linking them. However, if you need
 * a boolean state that tells you whether the entry is linked or not, you should
 * initialize the entry via C_LIST_INIT as well.
 */
struct CList {
        CList *next;
        CList *prev;
};

#define C_LIST_INIT(_var) { .next = &(_var), .prev = &(_var) }

/**
 * c_list_init() - initialize list entry
 * @what:               list entry to initialize
 *
 * Return: @what is returned.
 */
static inline CList *c_list_init(CList *what) {
        *what = (CList)C_LIST_INIT(*what);
        return what;
}

/**
 * c_list_entry_offset() - get parent container of list entry
 * @what:               list entry, or NULL
 * @offset:             offset of the list member in its surrounding type
 *
 * If the list entry @what is embedded into a surrounding structure, this will
 * turn the list entry pointer @what into a pointer to the parent container
 * (sometimes called container_of(3)). Use the `c_list_entry()` macro for an
 * easier API.
 *
 * If @what is NULL, this will also return NULL.
 *
 * Return: Pointer to parent container, or NULL.
 */
static inline void *c_list_entry_offset(const CList *what, size_t offset) {
        if (what) {
            /*
             * We allow calling "c_list_entry()" on the list head, which is
             * commonly a plain CList struct. The returned entry pointer is
             * thus invalid. For instance, this is used by the
             * c_list_for_each_entry*() macros. Gcc correctly warns about that
             * with "-Warray-bounds". However, as long as the value is never
             * dereferenced, this is fine. We explicitly use integer arithmetic
             * to circumvent the Gcc warning.
             */
            return (void *)(((uintptr_t)(void *)what) - offset);
        }
        return NULL;
}

/**
 * c_list_entry() - get parent container of list entry
 * @_what:              list entry, or NULL
 * @_t:                 type of parent container
 * @_m:                 member name of list entry in @_t
 *
 * If the list entry @_what is embedded into a surrounding structure, this will
 * turn the list entry pointer @_what into a pointer to the parent container
 * (using offsetof(3), or sometimes called container_of(3)).
 *
 * If @_what is NULL, this will also return NULL.
 *
 * Return: Pointer to parent container, or NULL.
 */
#define c_list_entry(_what, _t, _m) \
        ((_t *)c_list_entry_offset((_what), offsetof(_t, _m)))

/**
 * c_list_is_linked() - check whether an entry is linked
 * @what:               entry to check, or NULL
 *
 * Return: True if @what is linked in a list, false if not.
 */
static inline _Bool c_list_is_linked(const CList *what) {
        return what && what->next != what;
}

/**
 * c_list_is_empty() - check whether a list is empty
 * @list:               list to check, or NULL
 *
 * This is the same as !c_list_is_linked().
 *
 * Return: True if @list is empty, false if not.
 */
static inline _Bool c_list_is_empty(const CList *list) {
        return !c_list_is_linked(list);
}

/**
 * c_list_link_before() - link entry into list
 * @where:              linked list entry used as anchor
 * @what:               entry to link
 *
 * This links @what directly in front of @where. @where can either be a list
 * head or any entry in the list.
 *
 * If @where points to the list head, this effectively links @what as new tail
 * element. Hence, the macro c_list_link_tail() is an alias to this.
 *
 * @what is not inspected prior to being linked. Hence, it better not be linked
 * into another list, or the other list will be corrupted.
 */
static inline void c_list_link_before(CList *where, CList *what) {
        CList *prev = where->prev, *next = where;

        next->prev = what;
        what->next = next;
        what->prev = prev;
        prev->next = what;
}
#define c_list_link_tail(_list, _what) c_list_link_before((_list), (_what))

/**
 * c_list_link_after() - link entry into list
 * @where:              linked list entry used as anchor
 * @what:               entry to link
 *
 * This links @what directly after @where. @where can either be a list head or
 * any entry in the list.
 *
 * If @where points to the list head, this effectively links @what as new front
 * element. Hence, the macro c_list_link_front() is an alias to this.
 *
 * @what is not inspected prior to being linked. Hence, it better not be linked
 * into another list, or the other list will be corrupted.
 */
static inline void c_list_link_after(CList *where, CList *what) {
        CList *prev = where, *next = where->next;

        next->prev = what;
        what->next = next;
        what->prev = prev;
        prev->next = what;
}
#define c_list_link_front(_list, _what) c_list_link_after((_list), (_what))

/**
 * c_list_unlink_stale() - unlink element from list
 * @what:               element to unlink
 *
 * This unlinks @what. If @what was initialized via C_LIST_INIT(), it has no
 * effect. If @what was never linked, nor initialized, behavior is undefined.
 *
 * Note that this does not modify @what. It just modifies the previous and next
 * elements in the list to no longer reference @what. If you want to make sure
 * @what is re-initialized after removal, use c_list_unlink().
 */
static inline void c_list_unlink_stale(CList *what) {
        CList *prev = what->prev, *next = what->next;

        next->prev = prev;
        prev->next = next;
}

/**
 * c_list_unlink() - unlink element from list and re-initialize
 * @what:               element to unlink
 *
 * This is like c_list_unlink_stale() but re-initializes @what after removal.
 */
static inline void c_list_unlink(CList *what) {
        /* condition is not needed, but avoids STOREs in fast-path */
        if (c_list_is_linked(what)) {
                c_list_unlink_stale(what);
                *what = (CList)C_LIST_INIT(*what);
        }
}

/**
 * c_list_swap() - exchange the contents of two lists
 * @list1:      the list to operate on
 * @list2:      the list to operate on
 *
 * This replaces the contents of the list @list1 with the contents
 * of @list2, and vice versa.
 */
static inline void c_list_swap(CList *list1, CList *list2) {
        CList t;

        /* make neighbors of list1 point to list2, and vice versa */
        t = *list1;
        t.next->prev = list2;
        t.prev->next = list2;
        t = *list2;
        t.next->prev = list1;
        t.prev->next = list1;

        /* swap list1 and list2 now that their neighbors were fixed up */
        t = *list1;
        *list1 = *list2;
        *list2 = t;
}

/**
 * c_list_splice() - splice one list into another
 * @target:     the list to splice into
 * @source:     the list to splice
 *
 * This removes all the entries from @source and splice them into @target.
 * The order of the two lists is preserved and the source is appended
 * to the end of target.
 *
 * On return, the source list will be empty.
 */
static inline void c_list_splice(CList *target, CList *source) {
        if (!c_list_is_empty(source)) {
                /* attach the front of @source to the tail of @target */
                source->next->prev = target->prev;
                target->prev->next = source->next;

                /* attach the tail of @source to the front of @target */
                source->prev->next = target;
                target->prev = source->prev;

                /* clear source */
                *source = (CList)C_LIST_INIT(*source);
        }
}

/**
 * c_list_split() - split one list in two
 * @source:     the list to split
 * @where:      new starting element of newlist
 * @target:     new list
 *
 * This splits @source in two. All elements following @where (including @where)
 * are moved to @target, replacing any old list. If @where points to @source
 * (i.e., the end of the list), @target will be empty.
 */
static inline void c_list_split(CList *source, CList *where, CList *target) {
        if (where == source) {
                *target = (CList)C_LIST_INIT(*target);
        } else {
                target->next = where;
                target->prev = source->prev;

                where->prev->next = source;
                source->prev = where->prev;

                where->prev = target;
                target->prev->next = target;
        }
}

/**
 * c_list_first() - return pointer to first element, or NULL if empty
 * @list:               list to operate on, or NULL
 *
 * This returns a pointer to the first element, or NULL if empty. This never
 * returns a pointer to the list head.
 *
 * Return: Pointer to first list element, or NULL if empty.
 */
static inline CList *c_list_first(CList *list) {
        return c_list_is_empty(list) ? NULL : list->next;
}

/**
 * c_list_last() - return pointer to last element, or NULL if empty
 * @list:               list to operate on, or NULL
 *
 * This returns a pointer to the last element, or NULL if empty. This never
 * returns a pointer to the list head.
 *
 * Return: Pointer to last list element, or NULL if empty.
 */
static inline CList *c_list_last(CList *list) {
        return c_list_is_empty(list) ? NULL : list->prev;
}

/**
 * c_list_first_entry() - return pointer to first entry, or NULL if empty
 * @_list:              list to operate on, or NULL
 * @_t:                 type of list entries
 * @_m:                 name of CList member in @_t
 *
 * This is like c_list_first(), but also applies c_list_entry() on the result.
 *
 * Return: Pointer to first list entry, or NULL if empty.
 */
#define c_list_first_entry(_list, _t, _m) \
        c_list_entry(c_list_first(_list), _t, _m)

/**
 * c_list_last_entry() - return pointer to last entry, or NULL if empty
 * @_list:              list to operate on, or NULL
 * @_t:                 type of list entries
 * @_m:                 name of CList member in @_t
 *
 * This is like c_list_last(), but also applies c_list_entry() on the result.
 *
 * Return: Pointer to last list entry, or NULL if empty.
 */
#define c_list_last_entry(_list, _t, _m) \
        c_list_entry(c_list_last(_list), _t, _m)

/**
 * c_list_for_each*() - iterators
 *
 * The c_list_for_each*() macros provide simple for-loop wrappers to iterate
 * a linked list. They come in a set of flavours:
 *
 *   - "entry": This combines c_list_entry() with the loop iterator, so the
 *              iterator always has the type of the surrounding object, rather
 *              than CList.
 *
 *   - "safe": The loop iterator always keeps track of the next element to
 *             visit. This means, you can safely modify the current element,
 *             while retaining loop-integrity.
 *             You still must not touch any other entry of the list. Otherwise,
 *             the loop-iterator will be corrupted.
 *
 *   - "continue": Rather than starting the iteration at the front of the list,
 *                 use the current value of the iterator as starting position.
 *                 Note that the first loop iteration will be the following
 *                 element, not the given element.
 *
 *   - "unlink": This unlinks the current element from the list before the loop
 *               code is run. Note that this only does a partial unlink, since
 *               it assumes the entire list will be unlinked. You must not
 *               break out of the loop, or the list will be in an inconsistent
 *               state.
 */

/* direct/raw iterators */

#define c_list_for_each(_iter, _list)                                           \
        for (_iter = (_list)->next;                                             \
             (_iter) != (_list);                                                \
             _iter = (_iter)->next)

#define c_list_for_each_safe(_iter, _safe, _list)                               \
        for (_iter = (_list)->next, _safe = (_iter)->next;                      \
             (_iter) != (_list);                                                \
             _iter = (_safe), _safe = (_safe)->next)

#define c_list_for_each_continue(_iter, _list)                                  \
        for (_iter = (_iter) ? (_iter)->next : (_list)->next;                   \
             (_iter) != (_list);                                                \
             _iter = (_iter)->next)

#define c_list_for_each_safe_continue(_iter, _safe, _list)                      \
        for (_iter = (_iter) ? (_iter)->next : (_list)->next,                   \
             _safe = (_iter)->next;                                             \
             (_iter) != (_list);                                                \
             _iter = (_safe), _safe = (_safe)->next)

#define c_list_for_each_safe_unlink(_iter, _safe, _list)                        \
        for (_iter = (_list)->next, _safe = (_iter)->next;                      \
             c_list_init(_iter) != (_list);                                     \
             _iter = (_safe), _safe = (_safe)->next)

/* c_list_entry() based iterators */

#define c_list_for_each_entry(_iter, _list, _m)                                 \
        for (_iter = c_list_entry((_list)->next, __typeof__(*_iter), _m);       \
             &(_iter)->_m != (_list);                                           \
             _iter = c_list_entry((_iter)->_m.next, __typeof__(*_iter), _m))

#define c_list_for_each_entry_safe(_iter, _safe, _list, _m)                     \
        for (_iter = c_list_entry((_list)->next, __typeof__(*_iter), _m),       \
             _safe = c_list_entry((_iter)->_m.next, __typeof__(*_iter), _m);    \
             &(_iter)->_m != (_list);                                           \
             _iter = (_safe),                                                   \
             _safe = c_list_entry((_safe)->_m.next, __typeof__(*_iter), _m))

#define c_list_for_each_entry_continue(_iter, _list, _m)                        \
        for (_iter = c_list_entry((_iter) ? (_iter)->_m.next : (_list)->next,   \
                                  __typeof__(*_iter),                           \
                                  _m);                                          \
             &(_iter)->_m != (_list);                                           \
             _iter = c_list_entry((_iter)->_m.next, __typeof__(*_iter), _m))

#define c_list_for_each_entry_safe_continue(_iter, _safe, _list, _m)            \
        for (_iter = c_list_entry((_iter) ? (_iter)->_m.next : (_list)->next,   \
                                  __typeof__(*_iter),                           \
                                  _m),                                          \
             _safe = c_list_entry((_iter)->_m.next, __typeof__(*_iter), _m);    \
             &(_iter)->_m != (_list);                                           \
             _iter = (_safe),                                                   \
             _safe = c_list_entry((_safe)->_m.next, __typeof__(*_iter), _m))

#define c_list_for_each_entry_safe_unlink(_iter, _safe, _list, _m)              \
        for (_iter = c_list_entry((_list)->next, __typeof__(*_iter), _m),       \
             _safe = c_list_entry((_iter)->_m.next, __typeof__(*_iter), _m);    \
             c_list_init(&(_iter)->_m) != (_list);                              \
             _iter = (_safe),                                                   \
             _safe = c_list_entry((_safe)->_m.next, __typeof__(*_iter), _m))

/**
 * c_list_flush() - flush all entries from a list
 * @list:               list to flush
 *
 * This unlinks all entries from the given list @list and reinitializes their
 * link-nodes via C_LIST_INIT().
 *
 * Note that the entries are not modified in any other way, nor is their memory
 * released. This function just unlinks them and resets all the list nodes. It
 * is particularly useful with temporary lists on the stack in combination with
 * the GCC-extension __attribute__((__cleanup__(arg))).
 */
static inline void c_list_flush(CList *list) {
        CList *iter, *safe;

        c_list_for_each_safe_unlink(iter, safe, list)
                /* empty */ ;
}

/**
 * c_list_length() - return number of linked entries, excluding the head
 * @list:               list to operate on
 *
 * Returns the number of entries in the list, excluding the list head @list.
 * That is, for a list that is empty according to c_list_is_empty(), the
 * returned length is 0. This requires to iterate the list and has thus O(n)
 * runtime.
 *
 * Note that this function is meant for debugging purposes only. If you need
 * the list size during normal operation, you should maintain a counter
 * separately.
 *
 * Return: Number of items in @list.
 */
static inline size_t c_list_length(const CList *list) {
        size_t n = 0;
        const CList *iter;

        c_list_for_each(iter, list)
                ++n;

        return n;
}

/**
 * c_list_contains() - check whether an entry is linked in a certain list
 * @list:               list to operate on
 * @what:               entry to look for
 *
 * This checks whether @what is linked into @list. This requires a linear
 * search through the list, as such runs in O(n). Note that the list-head is
 * considered part of the list, and hence this returns true if @what equals
 * @list.
 *
 * Note that this function is meant for debugging purposes, and consistency
 * checks. You should always be aware whether your objects are linked in a
 * specific list.
 *
 * Return: True if @what is in @list, false otherwise.
 */
static inline _Bool c_list_contains(const CList *list, const CList *what) {
        const CList *iter;

        c_list_for_each(iter, list)
                if (what == iter)
                        return 1;

        return what == list;
}

#ifdef __cplusplus
}
#endif
