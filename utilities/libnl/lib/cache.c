/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2012 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup cache_mngt
 * @defgroup cache Cache
 *
 * @code
 *   Cache Management             |    | Type Specific Cache Operations
 *                                      
 *                                |    | +----------------+ +------------+
 *                                       | request update | | msg_parser |
 *                                |    | +----------------+ +------------+
 *                                     +- - - - -^- - - - - - - -^- -|- - - -
 *    nl_cache_update:            |              |               |   |
 *          1) --------- co_request_update ------+               |   |
 *                                |                              |   |
 *          2) destroy old cache     +----------- pp_cb ---------|---+
 *                                |  |                           |
 *          3) ---------- nl_recvmsgs ----------+   +- cb_valid -+
 *             +--------------+   |  |          |   |
 *             | nl_cache_add |<-----+   + - - -v- -|- - - - - - - - - - -
 *             +--------------+   |      | +-------------+
 *                                         | nl_recvmsgs |
 *                                |      | +-----|-^-----+
 *                                           +---v-|---+
 *                                |      |   | nl_recv |
 *                                           +---------+
 *                                |      |                 Core Netlink
 * @endcode
 * 
 * Related sections in the development guide:
 * - @core_doc{core_cache, Caching System}
 *
 * @{
 *
 * Header
 * ------
 * ~~~~{.c}
 * #include <netlink/cache.h>
 * ~~~~
 */

#include "nl-default.h"

#include <netlink/netlink.h>
#include <netlink/cache.h>
#include <netlink/object.h>
#include <netlink/hashtable.h>
#include <netlink/utils.h>

#include "nl-core.h"
#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-priv-dynamic-core/object-api.h"
#include "nl-priv-dynamic-core/cache-api.h"
#include "hashtable-api.h"
#include "nl-aux-core/nl-core.h"

/**
 * @name Access Functions
 * @{
 */

/**
 * Return the number of items in the cache
 * @arg cache		cache handle
 */
int nl_cache_nitems(struct nl_cache *cache)
{
	return cache->c_nitems;
}

/**
 * Return the number of items matching a filter in the cache
 * @arg cache		Cache object.
 * @arg filter		Filter object.
 */
int nl_cache_nitems_filter(struct nl_cache *cache, struct nl_object *filter)
{
	struct nl_object *obj;
	int nitems = 0;

	if (cache->c_ops == NULL)
		BUG();

	nl_list_for_each_entry(obj, &cache->c_items, ce_list) {
		if (filter && !nl_object_match_filter(obj, filter))
			continue;

		nitems++;
	}

	return nitems;
}

/**
 * Returns \b true if the cache is empty.
 * @arg cache		Cache to check
 * @return \a true if the cache is empty, otherwise \b false is returned.
 */
int nl_cache_is_empty(struct nl_cache *cache)
{
	return nl_list_empty(&cache->c_items);
}

/**
 * Return the operations set of the cache
 * @arg cache		cache handle
 */
struct nl_cache_ops *nl_cache_get_ops(struct nl_cache *cache)
{
	return cache->c_ops;
}

/**
 * Return the first element in the cache
 * @arg cache		cache handle
 */
struct nl_object *nl_cache_get_first(struct nl_cache *cache)
{
	if (nl_list_empty(&cache->c_items))
		return NULL;

	return nl_list_entry(cache->c_items.next,
			     struct nl_object, ce_list);
}

/**
 * Return the last element in the cache
 * @arg cache		cache handle
 */
struct nl_object *nl_cache_get_last(struct nl_cache *cache)
{
	if (nl_list_empty(&cache->c_items))
		return NULL;

	return nl_list_entry(cache->c_items.prev,
			     struct nl_object, ce_list);
}

/**
 * Return the next element in the cache
 * @arg obj		current object
 */
struct nl_object *nl_cache_get_next(struct nl_object *obj)
{
	if (nl_list_at_tail(obj, &obj->ce_cache->c_items, ce_list))
		return NULL;
	else
		return nl_list_entry(obj->ce_list.next,
				     struct nl_object, ce_list);
}

/**
 * Return the previous element in the cache
 * @arg obj		current object
 */
struct nl_object *nl_cache_get_prev(struct nl_object *obj)
{
	if (nl_list_at_head(obj, &obj->ce_cache->c_items, ce_list))
		return NULL;
	else
		return nl_list_entry(obj->ce_list.prev,
				     struct nl_object, ce_list);
}

/** @} */

/**
 * @name Cache Allocation/Deletion
 * @{
 */

/**
 * Allocate new cache
 * @arg ops		Cache operations
 *
 * Allocate and initialize a new cache based on the cache operations
 * provided.
 *
 * @return Allocated cache or NULL if allocation failed.
 */
struct nl_cache *nl_cache_alloc(struct nl_cache_ops *ops)
{
	struct nl_cache *cache;

	cache = calloc(1, sizeof(*cache));
	if (!cache)
		return NULL;

	nl_init_list_head(&cache->c_items);
	cache->c_ops = ops;
	cache->c_flags |= ops->co_flags;
	cache->c_refcnt = 1;

	/*
	 * If object type provides a hash keygen
	 * functions, allocate a hash table for the
	 * cache objects for faster lookups
	 */
	if (ops->co_obj_ops->oo_keygen) {
		cache->hashtable = nl_rhash_table_alloc();
	}

	NL_DBG(2, "Allocated cache %p <%s>.\n", cache, nl_cache_name(cache));

	return cache;
}

/**
 * Allocate new cache and fill it
 * @arg ops		Cache operations
 * @arg sock		Netlink socket
 * @arg result		Result pointer
 *
 * Allocate new cache and fill it. Equivalent to calling:
 * @code
 * cache = nl_cache_alloc(ops);
 * nl_cache_refill(sock, cache);
 * @endcode
 *
 * @see nl_cache_alloc
 *
 * @return 0 on success or a negative error code.
 */
int nl_cache_alloc_and_fill(struct nl_cache_ops *ops, struct nl_sock *sock,
			    struct nl_cache **result)
{
	struct nl_cache *cache;
	int err;

	if (!(cache = nl_cache_alloc(ops)))
		return -NLE_NOMEM;

	if (sock && (err = nl_cache_refill(sock, cache)) < 0) {
		nl_cache_free(cache);
		return err;
	}

	*result = cache;
	return 0;
}

/**
 * Allocate new cache based on type name
 * @arg kind		Name of cache type
 * @arg result		Result pointer
 *
 * Lookup cache ops via nl_cache_ops_lookup() and allocate the cache
 * by calling nl_cache_alloc(). Stores the allocated cache in the
 * result pointer provided.
 *
 * @see nl_cache_alloc
 *
 * @return 0 on success or a negative error code.
 */
int nl_cache_alloc_name(const char *kind, struct nl_cache **result)
{
	struct nl_cache_ops *ops;
	struct nl_cache *cache;

	ops = nl_cache_ops_lookup_safe(kind);
	if (!ops)
		return -NLE_NOCACHE;

	cache = nl_cache_alloc(ops);
	nl_cache_ops_put(ops);
	if (!cache)
		return -NLE_NOMEM;

	*result = cache;
	return 0;
}

/**
 * Allocate new cache containing a subset of an existing cache
 * @arg orig		Original cache to base new cache on
 * @arg filter		Filter defining the subset to be filled into the new cache
 *
 * Allocates a new cache matching the type of the cache specified by
 * \p orig. Iterates over the \p orig cache applying the specified
 * \p filter and copies all objects that match to the new cache.
 *
 * The copied objects are clones but do not contain a reference to each
 * other. Later modifications to objects in the original cache will
 * not affect objects in the new cache.
 *
 * @return A newly allocated cache or NULL.
 */
struct nl_cache *nl_cache_subset(struct nl_cache *orig,
				 struct nl_object *filter)
{
	struct nl_cache *cache;
	struct nl_object *obj;

	if (!filter)
		BUG();

	cache = nl_cache_alloc(orig->c_ops);
	if (!cache)
		return NULL;

	NL_DBG(2, "Filling subset of cache %p <%s> with filter %p into %p\n",
	       orig, nl_cache_name(orig), filter, cache);

	nl_list_for_each_entry(obj, &orig->c_items, ce_list) {
		if (!nl_object_match_filter(obj, filter))
			continue;

		nl_cache_add(cache, obj);
	}

	return cache;
}

/**
 * Allocate new cache and copy the contents of an existing cache
 * @arg cache		Original cache to base new cache on
 *
 * Allocates a new cache matching the type of the cache specified by
 * \p cache. Iterates over the \p cache cache and copies all objects
 * to the new cache.
 *
 * The copied objects are clones but do not contain a reference to each
 * other. Later modifications to objects in the original cache will
 * not affect objects in the new cache.
 *
 * @return A newly allocated cache or NULL.
 */
struct nl_cache *nl_cache_clone(struct nl_cache *cache)
{
	struct nl_cache_ops *ops = nl_cache_get_ops(cache);
	struct nl_cache *clone;
	struct nl_object *obj;

	clone = nl_cache_alloc(ops);
	if (!clone)
		return NULL;

	NL_DBG(2, "Cloning %p into %p\n", cache, clone);

	nl_list_for_each_entry(obj, &cache->c_items, ce_list)
		nl_cache_add(clone, obj);

	return clone;
}

/**
 * Remove all objects of a cache.
 * @arg cache		Cache to clear
 *
 * The objects are unliked/removed from the cache by calling
 * nl_cache_remove() on each object in the cache. If any of the objects
 * to not contain any further references to them, those objects will
 * be freed.
 *
 * Unlike with nl_cache_free(), the cache is not freed just emptied.
 */
void nl_cache_clear(struct nl_cache *cache)
{
	struct nl_object *obj, *tmp;

	NL_DBG(2, "Clearing cache %p <%s>...\n", cache, nl_cache_name(cache));

	nl_list_for_each_entry_safe(obj, tmp, &cache->c_items, ce_list)
		nl_cache_remove(obj);
}

static void __nl_cache_free(struct nl_cache *cache)
{
	nl_cache_clear(cache);

	if (cache->hashtable)
		nl_rhash_table_free(cache->hashtable);

	NL_DBG(2, "Freeing cache %p <%s>...\n", cache, nl_cache_name(cache));
	free(cache);
}

/**
 * Increase reference counter of cache
 * @arg cache		Cache
 */
void nl_cache_get(struct nl_cache *cache)
{
	cache->c_refcnt++;

	NL_DBG(3, "Incremented cache %p <%s> reference count to %d\n",
	       cache, nl_cache_name(cache), cache->c_refcnt);
}

/**
 * Free a cache.
 * @arg cache		Cache to free.
 *
 * Calls nl_cache_clear() to remove all objects associated with the
 * cache and frees the cache afterwards.
 *
 * @see nl_cache_clear()
 */
void nl_cache_free(struct nl_cache *cache)
{
	if (!cache)
		return;

	cache->c_refcnt--;

	NL_DBG(3, "Decremented cache %p <%s> reference count, %d remaining\n",
	       cache, nl_cache_name(cache), cache->c_refcnt);

	if (cache->c_refcnt <= 0)
		__nl_cache_free(cache);
}

void nl_cache_put(struct nl_cache *cache)
{
	nl_cache_free(cache);
}

/** @} */

/**
 * @name Cache Modifications
 * @{
 */

static int __cache_add(struct nl_cache *cache, struct nl_object *obj)
{
	int ret;

	obj->ce_cache = cache;

	if (cache->hashtable) {
		ret = nl_rhash_table_add(cache->hashtable, obj);
		if (ret < 0) {
			obj->ce_cache = NULL;
			return ret;
		}
	}

	nl_list_add_tail(&obj->ce_list, &cache->c_items);
	cache->c_nitems++;

	NL_DBG(3, "Added object %p to cache %p <%s>, nitems %d\n",
	       obj, cache, nl_cache_name(cache), cache->c_nitems);

	return 0;
}

/**
 * Add object to cache.
 * @arg cache		Cache
 * @arg obj		Object to be added to the cache
 *
 * Adds the object \p obj to the specified \p cache. In case the object
 * is already associated with another cache, the object is cloned before
 * adding it to the cache. In this case, the sole reference to the object
 * will be the one of the cache. Therefore clearing/freeing the cache
 * will result in the object being freed again.
 *
 * If the object has not been associated with a cache yet, the reference
 * counter of the object is incremented to account for the additional
 * reference.
 *
 * The type of the object and cache must match, otherwise an error is
 * returned (-NLE_OBJ_MISMATCH).
 *
 * @see nl_cache_move()
 *
 * @return 0 or a negative error code.
 */
int nl_cache_add(struct nl_cache *cache, struct nl_object *obj)
{
	struct nl_object *new;
	int ret = 0;

	if (cache->c_ops->co_obj_ops != obj->ce_ops)
		return -NLE_OBJ_MISMATCH;

	if (!nl_list_empty(&obj->ce_list)) {
		NL_DBG(3, "Object %p already in cache, cloning new object\n", obj);

		new = nl_object_clone(obj);
		if (!new)
			return -NLE_NOMEM;
	} else {
		nl_object_get(obj);
		new = obj;
	}

	ret = __cache_add(cache, new);
	if (ret < 0)
		nl_object_put(new);

	return ret;
}

/**
 * Move object from one cache to another
 * @arg cache		Cache to move object to.
 * @arg obj		Object subject to be moved
 *
 * Removes the the specified object \p obj from its associated cache
 * and moves it to another cache.
 *
 * If the object is not associated with a cache, the function behaves
 * just like nl_cache_add().
 *
 * The type of the object and cache must match, otherwise an error is
 * returned (-NLE_OBJ_MISMATCH).
 *
 * @see nl_cache_add()
 *
 * @return 0 on success or a negative error code.
 */
int nl_cache_move(struct nl_cache *cache, struct nl_object *obj)
{
	if (cache->c_ops->co_obj_ops != obj->ce_ops)
		return -NLE_OBJ_MISMATCH;

	NL_DBG(3, "Moving object %p from cache %p to cache %p\n",
	       obj, obj->ce_cache, cache);

	/* Acquire reference, if already in a cache this will be
	 * reverted during removal */
	nl_object_get(obj);

	if (!nl_list_empty(&obj->ce_list))
		nl_cache_remove(obj);

	return __cache_add(cache, obj);
}

/**
 * Remove object from cache.
 * @arg obj		Object to remove from cache
 *
 * Removes the object \c obj from the cache it is associated with. The
 * reference counter of the object will be decremented. If the reference
 * to the object was the only one remaining, the object will be freed.
 *
 * If no cache is associated with the object, this function is a NOP.
 */
void nl_cache_remove(struct nl_object *obj)
{
	int ret;
	struct nl_cache *cache = obj->ce_cache;

	if (cache == NULL)
		return;

	if (cache->hashtable) {
		ret = nl_rhash_table_del(cache->hashtable, obj);
		if (ret < 0)
			NL_DBG(2, "Failed to delete %p from cache %p <%s>.\n",
			       obj, cache, nl_cache_name(cache));
	}

	nl_list_del(&obj->ce_list);
	obj->ce_cache = NULL;
	nl_object_put(obj);
	cache->c_nitems--;

	NL_DBG(2, "Deleted object %p from cache %p <%s>.\n",
	       obj, cache, nl_cache_name(cache));
}

/** @} */

/**
 * @name Synchronization
 * @{
 */

/**
 * Set synchronization arg1 of cache
 * @arg cache		Cache
 * @arg arg		argument
 *
 * Synchronization arguments are used to specify filters when
 * requesting dumps from the kernel.
 */
void nl_cache_set_arg1(struct nl_cache *cache, int arg)
{
	cache->c_iarg1 = arg;
}

/**
 * Set synchronization arg2 of cache
 * @arg cache		Cache
 * @arg arg		argument
 *
 * Synchronization arguments are used to specify filters when
 * requesting dumps from the kernel.
 */
void nl_cache_set_arg2(struct nl_cache *cache, int arg)
{
	cache->c_iarg2 = arg;
}

/**
 * Set cache flags
 * @arg cache		Cache
 * @arg flags		Flags
 */
void nl_cache_set_flags(struct nl_cache *cache, unsigned int flags)
{
	cache->c_flags |= flags;
}

/**
 * Invoke the request-update operation
 * @arg sk		Netlink socket.
 * @arg cache		Cache
 *
 * This function causes the \e request-update function of the cache
 * operations to be invoked. This usually causes a dump request to
 * be sent over the netlink socket which triggers the kernel to dump
 * all objects of a specific type to be dumped onto the netlink
 * socket for pickup.
 *
 * The behaviour of this function depends on the implemenation of
 * the \e request_update function of each individual type of cache.
 *
 * This function will not have any effects on the cache (unless the
 * request_update implementation of the cache operations does so).
 *
 * Use nl_cache_pickup() to pick-up (read) the objects from the socket
 * and fill them into the cache.
 *
 * @see nl_cache_pickup(), nl_cache_resync()
 *
 * @return 0 on success or a negative error code. Some implementations
 * of co_request_update() return a positive number on success that is
 * the number of bytes sent. Treat any non-negative number as success too.
 */
static int nl_cache_request_full_dump(struct nl_sock *sk,
				      struct nl_cache *cache)
{
	if (sk->s_proto != cache->c_ops->co_protocol)
		return -NLE_PROTO_MISMATCH;

	if (cache->c_ops->co_request_update == NULL)
		return -NLE_OPNOTSUPP;

	NL_DBG(2, "Requesting update from kernel for cache %p <%s>\n",
	          cache, nl_cache_name(cache));

	return cache->c_ops->co_request_update(cache, sk);
}

/** @cond SKIP */
struct update_xdata {
	struct nl_cache_ops *ops;
	struct nl_parser_param *params;
};

static int update_msg_parser(struct nl_msg *msg, void *arg)
{
	struct update_xdata *x = arg;
	int ret = 0;

	ret = nl_cache_parse(x->ops, &msg->nm_src, msg->nm_nlh, x->params);
	if (ret == -NLE_EXIST)
		return NL_SKIP;
	else
		return ret;
}
/** @endcond */

/**
 * Pick-up a netlink request-update with your own parser
 * @arg sk		Netlink socket
 * @arg cache		Cache
 * @arg param		Parser parameters
 */
static int __cache_pickup(struct nl_sock *sk, struct nl_cache *cache,
			  struct nl_parser_param *param)
{
	int err;
	struct nl_cb *cb;
	struct update_xdata x = {
		.ops = cache->c_ops,
		.params = param,
	};

	NL_DBG(2, "Picking up answer for cache %p <%s>\n",
	       cache, nl_cache_name(cache));

	cb = nl_cb_clone(sk->s_cb);
	if (cb == NULL)
		return -NLE_NOMEM;

	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, update_msg_parser, &x);

	err = nl_recvmsgs(sk, cb);
	if (err < 0)
		NL_DBG(2, "While picking up for %p <%s>, recvmsgs() returned %d: %s\n",
		       cache, nl_cache_name(cache), err, nl_geterror(err));

	nl_cb_put(cb);

	return err;
}

static int pickup_checkdup_cb(struct nl_object *c, struct nl_parser_param *p)
{
	struct nl_cache *cache = (struct nl_cache *)p->pp_arg;
	_nl_auto_nl_object struct nl_object *old = NULL;

	old = nl_cache_search(cache, c);
	if (old) {
		if (nl_object_update(old, c) == 0)
			return 0;

		nl_cache_remove(old);
	}

	return nl_cache_add(cache, c);
}

static int pickup_cb(struct nl_object *c, struct nl_parser_param *p)
{
	struct nl_cache *cache = p->pp_arg;

	return nl_cache_add(cache, c);
}

static int __nl_cache_pickup(struct nl_sock *sk, struct nl_cache *cache,
			     int checkdup)
{
	struct nl_parser_param p;

	p.pp_cb = checkdup ? pickup_checkdup_cb : pickup_cb;
	p.pp_arg = cache;

	if (sk->s_proto != cache->c_ops->co_protocol)
		return -NLE_PROTO_MISMATCH;

	return __cache_pickup(sk, cache, &p);
}

/**
 * Pickup a netlink dump response and put it into a cache.
 * @arg sk		Netlink socket.
 * @arg cache		Cache to put items into.
 *
 * Waits for netlink messages to arrive, parses them and puts them into
 * the specified cache. If an old object with same key attributes is
 * present in the cache, it is replaced with the new object.
 * If the old object type supports an update operation, an update is
 * attempted before a replace.
 *
 * @return 0 on success or a negative error code.
 */
int nl_cache_pickup_checkdup(struct nl_sock *sk, struct nl_cache *cache)
{
	return __nl_cache_pickup(sk, cache, 1);
}

/**
 * Pickup a netlink dump response and put it into a cache.
 * @arg sk		Netlink socket.
 * @arg cache		Cache to put items into.
 *
 * Waits for netlink messages to arrive, parses them and puts them into
 * the specified cache.
 *
 * @return 0 on success or a negative error code.
 */
int nl_cache_pickup(struct nl_sock *sk, struct nl_cache *cache)
{
	return __nl_cache_pickup(sk, cache, 0);
}

static int cache_include(struct nl_cache *cache, struct nl_object *obj,
			 struct nl_msgtype *type, change_func_t cb,
			 change_func_v2_t cb_v2, void *data)
{
	_nl_auto_nl_object struct nl_object *old = NULL;
	_nl_auto_nl_object struct nl_object *clone = NULL;
	uint64_t diff = 0;

	switch (type->mt_act) {
	case NL_ACT_NEW:
	case NL_ACT_DEL:
		old = nl_cache_search(cache, obj);
		if (old) {
			if (cb_v2 && old->ce_ops->oo_update) {
				clone = nl_object_clone(old);
				diff = nl_object_diff64(old, obj);
			}
			/*
			 * Some objects types might support merging the new
			 * object with the old existing cache object.
			 * Handle them first.
			 */
			if (nl_object_update(old, obj) == 0) {
				if (cb_v2) {
					cb_v2(cache, clone, old, diff,
					      NL_ACT_CHANGE, data);
				} else if (cb)
					cb(cache, old, NL_ACT_CHANGE, data);
				return 0;
			}
			nl_cache_remove(old);
			if (type->mt_act == NL_ACT_DEL) {
				if (cb_v2)
					cb_v2(cache, old, NULL, 0, NL_ACT_DEL,
					      data);
				else if (cb)
					cb(cache, old, NL_ACT_DEL, data);
			}
		}

		if (type->mt_act == NL_ACT_NEW) {
			nl_cache_move(cache, obj);
			if (old == NULL) {
				if (cb_v2) {
					cb_v2(cache, NULL, obj, 0, NL_ACT_NEW,
					      data);
				} else if (cb)
					cb(cache, obj, NL_ACT_NEW, data);
			} else if (old) {
				diff = 0;
				if (cb || cb_v2)
					diff = nl_object_diff64(old, obj);
				if (diff && cb_v2) {
					cb_v2(cache, old, obj, diff, NL_ACT_CHANGE,
					      data);
				} else if (diff && cb)
					cb(cache, obj, NL_ACT_CHANGE, data);

			}
		}
		break;
	default:
		NL_DBG(2, "Unknown action associated to object %p\n", obj);
		return 0;
	}

	return 0;
}

int nl_cache_include(struct nl_cache *cache, struct nl_object *obj,
		     change_func_t change_cb, void *data)
{
	struct nl_cache_ops *ops = cache->c_ops;
	int i;

	if (ops->co_obj_ops != obj->ce_ops)
		return -NLE_OBJ_MISMATCH;

	for (i = 0; ops->co_msgtypes[i].mt_id >= 0; i++)
		if (ops->co_msgtypes[i].mt_id == obj->ce_msgtype)
			return cache_include(cache, obj, &ops->co_msgtypes[i],
					     change_cb, NULL, data);

	NL_DBG(3, "Object %p does not seem to belong to cache %p <%s>\n",
	       obj, cache, nl_cache_name(cache));

	return -NLE_MSGTYPE_NOSUPPORT;
}

int nl_cache_include_v2(struct nl_cache *cache, struct nl_object *obj,
			change_func_v2_t change_cb, void *data)
{
	struct nl_cache_ops *ops = cache->c_ops;
	int i;

	if (ops->co_obj_ops != obj->ce_ops)
		return -NLE_OBJ_MISMATCH;

	for (i = 0; ops->co_msgtypes[i].mt_id >= 0; i++)
		if (ops->co_msgtypes[i].mt_id == obj->ce_msgtype)
			return cache_include(cache, obj, &ops->co_msgtypes[i],
						NULL, change_cb, data);

	NL_DBG(3, "Object %p does not seem to belong to cache %p <%s>\n",
	       obj, cache, nl_cache_name(cache));

	return -NLE_MSGTYPE_NOSUPPORT;
}

static int resync_cb(struct nl_object *c, struct nl_parser_param *p)
{
	struct nl_cache_assoc *ca = p->pp_arg;

	if (ca->ca_change_v2)
		return nl_cache_include_v2(ca->ca_cache, c, ca->ca_change_v2,
					   ca->ca_change_data);
	else
		return nl_cache_include(ca->ca_cache, c, ca->ca_change,
					ca->ca_change_data);
}

static int cache_resync(struct nl_sock *sk, struct nl_cache *cache,
			change_func_t change_cb, change_func_v2_t change_cb_v2,
			void *data)
{
	struct nl_object *obj, *next;
	struct nl_af_group *grp;
	struct nl_cache_assoc ca = {
		.ca_cache = cache,
		.ca_change = change_cb,
		.ca_change_v2 = change_cb_v2,
		.ca_change_data = data,
	};
	struct nl_parser_param p = {
		.pp_cb = resync_cb,
		.pp_arg = &ca,
	};
	int err;

	if (sk->s_proto != cache->c_ops->co_protocol)
		return -NLE_PROTO_MISMATCH;

	NL_DBG(1, "Resyncing cache %p <%s>...\n", cache, nl_cache_name(cache));

	/* Mark all objects so we can see if some of them are obsolete */
	nl_cache_mark_all(cache);

	grp = cache->c_ops->co_groups;
	do {
		if (grp && grp->ag_group &&
			(cache->c_flags & NL_CACHE_AF_ITER))
			nl_cache_set_arg1(cache, grp->ag_family);

restart:
		err = nl_cache_request_full_dump(sk, cache);
		if (err < 0)
			goto errout;

		err = __cache_pickup(sk, cache, &p);
		if (err == -NLE_DUMP_INTR)
			goto restart;
		else if (err < 0)
			goto errout;

		if (grp)
			grp++;
	} while (grp && grp->ag_group &&
		(cache->c_flags & NL_CACHE_AF_ITER));

	nl_list_for_each_entry_safe(obj, next, &cache->c_items, ce_list) {
		if (nl_object_is_marked(obj)) {
			nl_object_get(obj);
			nl_cache_remove(obj);
			if (change_cb)
				change_cb(cache, obj, NL_ACT_DEL, data);
			else if (change_cb_v2)
				change_cb_v2(cache, obj, NULL, 0, NL_ACT_DEL,
					     data);
			nl_object_put(obj);
		}
	}

	NL_DBG(1, "Finished resyncing %p <%s>\n", cache, nl_cache_name(cache));

	err = 0;
errout:
	return err;
}

int nl_cache_resync(struct nl_sock *sk, struct nl_cache *cache,
		    change_func_t change_cb, void *data)
{
	return cache_resync(sk, cache, change_cb, NULL, data);
}

int nl_cache_resync_v2(struct nl_sock *sk, struct nl_cache *cache,
		    change_func_v2_t change_cb_v2, void *data)
{
	return cache_resync(sk, cache, NULL, change_cb_v2, data);
}

/** @} */

/**
 * @name Parsing
 * @{
 */

/** @cond SKIP */
int nl_cache_parse(struct nl_cache_ops *ops, struct sockaddr_nl *who,
		   struct nlmsghdr *nlh, struct nl_parser_param *params)
{
	int i, err;

	if (!nlmsg_valid_hdr(nlh, ops->co_hdrsize))
		return -NLE_MSG_TOOSHORT;

	for (i = 0; ops->co_msgtypes[i].mt_id >= 0; i++) {
		if (ops->co_msgtypes[i].mt_id == nlh->nlmsg_type) {
			err = ops->co_msg_parser(ops, who, nlh, params);
			if (err != -NLE_OPNOTSUPP)
				goto errout;
		}
	}


	err = -NLE_MSGTYPE_NOSUPPORT;
errout:
	return err;
}
/** @endcond */

/**
 * Parse a netlink message and add it to the cache.
 * @arg cache		cache to add element to
 * @arg msg		netlink message
 *
 * Parses a netlink message by calling the cache specific message parser
 * and adds the new element to the cache. If an old object with same key
 * attributes is present in the cache, it is replaced with the new object.
 * If the old object type supports an update operation, an update is
 * attempted before a replace.
 *
 * @return 0 or a negative error code.
 */
int nl_cache_parse_and_add(struct nl_cache *cache, struct nl_msg *msg)
{
	struct nl_parser_param p = {
		.pp_cb = pickup_cb,
		.pp_arg = cache,
	};

	return nl_cache_parse(cache->c_ops, NULL, nlmsg_hdr(msg), &p);
}

/**
 * (Re)fill a cache with the contents in the kernel.
 * @arg sk		Netlink socket.
 * @arg cache		cache to update
 *
 * Clears the specified cache and fills it with the current state in
 * the kernel.
 *
 * @return 0 or a negative error code.
 */
int nl_cache_refill(struct nl_sock *sk, struct nl_cache *cache)
{
	struct nl_af_group *grp;
	int err;

	if (sk->s_proto != cache->c_ops->co_protocol)
		return -NLE_PROTO_MISMATCH;

	nl_cache_clear(cache);
	grp = cache->c_ops->co_groups;
	do {
		if (grp && grp->ag_group &&
			(cache->c_flags & NL_CACHE_AF_ITER))
			nl_cache_set_arg1(cache, grp->ag_family);

restart:
		err = nl_cache_request_full_dump(sk, cache);
		if (err < 0)
			return err;

		NL_DBG(2, "Updating cache %p <%s> for family %u, request sent, waiting for reply\n",
		       cache, nl_cache_name(cache), grp ? grp->ag_family : AF_UNSPEC);

		err = nl_cache_pickup(sk, cache);
		if (err == -NLE_DUMP_INTR) {
			NL_DBG(2, "Dump interrupted, restarting!\n");
			goto restart;
		} else if (err < 0)
			break;

		if (grp)
			grp++;
	} while (grp && grp->ag_group &&
			(cache->c_flags & NL_CACHE_AF_ITER));

	return err;
}

/** @} */

/**
 * @name Utillities
 * @{
 */
static struct nl_object *__cache_fast_lookup(struct nl_cache *cache,
					     struct nl_object *needle)
{
	struct nl_object *obj;

	obj = nl_rhash_table_lookup(cache->hashtable, needle);
	if (obj) {
		nl_object_get(obj);
		return obj;
	}

	return NULL;
}

/**
 * Search object in cache
 * @arg cache		Cache
 * @arg needle		Object to look for.
 *
 * Searches the cache for an object which matches the object \p needle.
 * The function nl_object_identical() is used to determine if the
 * objects match. If a matching object is found, the reference counter
 * is incremented and the object is returned.
 * 
 * Therefore, if an object is returned, the reference to the object
 * must be returned by calling nl_object_put() after usage.
 *
 * @return Reference to object or NULL if not found.
 */
struct nl_object *nl_cache_search(struct nl_cache *cache,
				  struct nl_object *needle)
{
	struct nl_object *obj;

	if (cache->hashtable)
		return __cache_fast_lookup(cache, needle);

	nl_list_for_each_entry(obj, &cache->c_items, ce_list) {
		if (nl_object_identical(obj, needle)) {
			nl_object_get(obj);
			return obj;
		}
	}

	return NULL;
}

/**
 * Find object in cache
 * @arg cache		Cache
 * @arg filter		object acting as a filter
 *
 * Searches the cache for an object which matches the object filter.
 * If the filter attributes matches the object type id attributes,
 * and the cache supports hash lookups, a faster hashtable lookup
 * is used to return the object. Else, function nl_object_match_filter() is
 * used to determine if the objects match. If a matching object is
 * found, the reference counter is incremented and the object is returned.
 *
 * Therefore, if an object is returned, the reference to the object
 * must be returned by calling nl_object_put() after usage.
 *
 * @return Reference to object or NULL if not found.
 */
struct nl_object *nl_cache_find(struct nl_cache *cache,
				struct nl_object *filter)
{
	struct nl_object *obj;

	if (cache->c_ops == NULL)
		BUG();

	if ((nl_object_get_id_attrs(filter) == filter->ce_mask)
		&& cache->hashtable)
		return __cache_fast_lookup(cache, filter);

	nl_list_for_each_entry(obj, &cache->c_items, ce_list) {
		if (nl_object_match_filter(obj, filter)) {
			nl_object_get(obj);
			return obj;
		}
	}

	return NULL;
}

/**
 * Mark all objects of a cache
 * @arg cache		Cache
 *
 * Marks all objects of a cache by calling nl_object_mark() on each
 * object associated with the cache.
 */
void nl_cache_mark_all(struct nl_cache *cache)
{
	struct nl_object *obj;

	NL_DBG(2, "Marking all objects in cache %p <%s>\n",
	       cache, nl_cache_name(cache));

	nl_list_for_each_entry(obj, &cache->c_items, ce_list)
		nl_object_mark(obj);
}

/** @} */

/**
 * @name Dumping
 * @{
 */

/**
 * Dump all elements of a cache.
 * @arg cache		cache to dump
 * @arg params		dumping parameters
 *
 * Dumps all elements of the \a cache to the file descriptor \a fd.
 */
void nl_cache_dump(struct nl_cache *cache, struct nl_dump_params *params)
{
	nl_cache_dump_filter(cache, params, NULL);
}

/**
 * Dump all elements of a cache (filtered).
 * @arg cache		cache to dump
 * @arg params		dumping parameters
 * @arg filter		filter object
 *
 * Dumps all elements of the \a cache to the file descriptor \a fd
 * given they match the given filter \a filter.
 */
void nl_cache_dump_filter(struct nl_cache *cache,
			  struct nl_dump_params *params,
			  struct nl_object *filter)
{
	struct nl_dump_params params_copy;
	struct nl_object_ops *ops;
	struct nl_object *obj;
	int type;

	NL_DBG(2, "Dumping cache %p <%s> with filter %p\n",
	       cache, nl_cache_name(cache), filter);

	if (!params) {
		/* It doesn't really make sense that @params is an optional parameter. In the
		 * past, nl_cache_dump() was documented that the @params would be optional, so
		 * try to save it.
		 *
		 * Note that this still isn't useful, because we don't set any dump option.
		 * It only exists not to crash applications that wrongly pass %NULL here. */
		_nl_assert_not_reached ();
		params_copy = (struct nl_dump_params) {
			.dp_type = NL_DUMP_DETAILS,
		};
		params = &params_copy;
	}

	type = params->dp_type;

	if (type > NL_DUMP_MAX || type < 0)
		BUG();

	if (cache->c_ops == NULL)
		BUG();

	ops = cache->c_ops->co_obj_ops;
	if (!ops->oo_dump[type])
		return;

	if (params->dp_buf)
		memset(params->dp_buf, 0, params->dp_buflen);

	nl_list_for_each_entry(obj, &cache->c_items, ce_list) {
		if (filter && !nl_object_match_filter(obj, filter))
			continue;

		NL_DBG(4, "Dumping object %p...\n", obj);
		dump_from_ops(obj, params);
	}
}

/** @} */

/**
 * @name Iterators
 * @{
 */

/**
 * Call a callback on each element of the cache.
 * @arg cache		cache to iterate on
 * @arg cb		callback function
 * @arg arg		argument passed to callback function
 *
 * Calls a callback function \a cb on each element of the \a cache.
 * The argument \a arg is passed on the callback function.
 */
void nl_cache_foreach(struct nl_cache *cache,
		      void (*cb)(struct nl_object *, void *), void *arg)
{
	nl_cache_foreach_filter(cache, NULL, cb, arg);
}

/**
 * Call a callback on each element of the cache (filtered).
 * @arg cache		cache to iterate on
 * @arg filter		filter object
 * @arg cb		callback function
 * @arg arg		argument passed to callback function
 *
 * Calls a callback function \a cb on each element of the \a cache
 * that matches the \a filter. The argument \a arg is passed on
 * to the callback function.
 */
void nl_cache_foreach_filter(struct nl_cache *cache, struct nl_object *filter,
			     void (*cb)(struct nl_object *, void *), void *arg)
{
	struct nl_object *obj, *tmp;

	if (cache->c_ops == NULL)
		BUG();

	nl_list_for_each_entry_safe(obj, tmp, &cache->c_items, ce_list) {
		if (filter) {
			int diff = nl_object_match_filter(obj, filter);

			NL_DBG(3, "%p<->%p object difference: %x\n",
				obj, filter, diff);

			if (!diff)
				continue;
		}

		/* Caller may hold obj for a long time */
		nl_object_get(obj);

		cb(obj, arg);

		nl_object_put(obj);
	}
}

/** @} */

/** @} */
