from ctypes import *

isl = cdll.LoadLibrary("libisl.so")
libc = cdll.LoadLibrary("libc.so.6")

class Error(Exception):
    pass

class Context:
    defaultInstance = None

    def __init__(self):
        ptr = isl.isl_ctx_alloc()
        self.ptr = ptr

    def __del__(self):
        isl.isl_ctx_free(self)

    def from_param(self):
        return self.ptr

    @staticmethod
    def getDefaultInstance():
        if Context.defaultInstance == None:
            Context.defaultInstance = Context()
        return Context.defaultInstance

isl.isl_ctx_alloc.restype = c_void_p
isl.isl_ctx_free.argtypes = [Context]

class union_map:
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        if len(args) == 1 and args[0].__class__ is map:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_union_map_from_map(isl.isl_map_copy(args[0].ptr))
            return
        if len(args) == 1 and type(args[0]) == str:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_union_map_read_from_str(self.ctx, args[0])
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_union_map_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_union_map_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.union_map("%s")' % str(self)
    def affine_hull(self):
        res = isl.isl_union_map_affine_hull(isl.isl_union_map_copy(self.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def polyhedral_hull(self):
        res = isl.isl_union_map_polyhedral_hull(isl.isl_union_map_copy(self.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def coalesce(self):
        res = isl.isl_union_map_coalesce(isl.isl_union_map_copy(self.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def lexmin(self):
        res = isl.isl_union_map_lexmin(isl.isl_union_map_copy(self.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def lexmax(self):
        res = isl.isl_union_map_lexmax(isl.isl_union_map_copy(self.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def union(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_union(isl.isl_union_map_copy(self.ptr), isl.isl_union_map_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def subtract(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_subtract(isl.isl_union_map_copy(self.ptr), isl.isl_union_map_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def intersect(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_intersect(isl.isl_union_map_copy(self.ptr), isl.isl_union_map_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def intersect_params(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            raise
        res = isl.isl_union_map_intersect_params(isl.isl_union_map_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def gist(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_gist(isl.isl_union_map_copy(self.ptr), isl.isl_union_map_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def gist_params(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            raise
        res = isl.isl_union_map_gist_params(isl.isl_union_map_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def gist_domain(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_map_gist_domain(isl.isl_union_map_copy(self.ptr), isl.isl_union_set_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def gist_range(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_map_gist_range(isl.isl_union_map_copy(self.ptr), isl.isl_union_set_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def intersect_domain(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_map_intersect_domain(isl.isl_union_map_copy(self.ptr), isl.isl_union_set_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def intersect_range(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_map_intersect_range(isl.isl_union_map_copy(self.ptr), isl.isl_union_set_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def apply_domain(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_apply_domain(isl.isl_union_map_copy(self.ptr), isl.isl_union_map_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def apply_range(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_apply_range(isl.isl_union_map_copy(self.ptr), isl.isl_union_map_copy(arg1.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def reverse(self):
        res = isl.isl_union_map_reverse(isl.isl_union_map_copy(self.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def detect_equalities(self):
        res = isl.isl_union_map_detect_equalities(self.ptr)
        return union_map(ctx=self.ctx, ptr=res)
    def deltas(self):
        res = isl.isl_union_map_deltas(isl.isl_union_map_copy(self.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def is_empty(self):
        res = isl.isl_union_map_is_empty(self.ptr)
        return res
    def is_single_valued(self):
        res = isl.isl_union_map_is_single_valued(self.ptr)
        return res
    def is_injective(self):
        res = isl.isl_union_map_is_injective(self.ptr)
        return res
    def is_bijective(self):
        res = isl.isl_union_map_is_bijective(self.ptr)
        return res
    def is_subset(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_is_subset(self.ptr, arg1.ptr)
        return res
    def is_equal(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_is_equal(self.ptr, arg1.ptr)
        return res
    def is_strict_subset(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_map_is_strict_subset(self.ptr, arg1.ptr)
        return res
    def foreach_map(self, arg1):
        exc_info = [None]
        fn = CFUNCTYPE(c_int, c_void_p, c_void_p)
        def cb_func(cb_arg0, cb_arg1):
            cb_arg0 = map(ctx=self.ctx, ptr=cb_arg0)
            try:
                arg1(cb_arg0)
            except:
                import sys
                exc_info[0] = sys.exc_info()
                return -1
            return 0
        cb = fn(cb_func)
        res = isl.isl_union_map_foreach_map(self.ptr, cb, None)
        if exc_info[0] != None:
            raise exc_info[0][0], exc_info[0][1], exc_info[0][2]
        return res

isl.isl_union_map_from_map.restype = c_void_p
isl.isl_union_map_from_map.argtypes = [c_void_p]
isl.isl_union_map_read_from_str.restype = c_void_p
isl.isl_union_map_read_from_str.argtypes = [Context, c_char_p]
isl.isl_union_map_affine_hull.restype = c_void_p
isl.isl_union_map_polyhedral_hull.restype = c_void_p
isl.isl_union_map_coalesce.restype = c_void_p
isl.isl_union_map_lexmin.restype = c_void_p
isl.isl_union_map_lexmax.restype = c_void_p
isl.isl_union_map_union.restype = c_void_p
isl.isl_union_map_subtract.restype = c_void_p
isl.isl_union_map_intersect.restype = c_void_p
isl.isl_union_map_intersect_params.restype = c_void_p
isl.isl_union_map_gist.restype = c_void_p
isl.isl_union_map_gist_params.restype = c_void_p
isl.isl_union_map_gist_domain.restype = c_void_p
isl.isl_union_map_gist_range.restype = c_void_p
isl.isl_union_map_intersect_domain.restype = c_void_p
isl.isl_union_map_intersect_range.restype = c_void_p
isl.isl_union_map_apply_domain.restype = c_void_p
isl.isl_union_map_apply_range.restype = c_void_p
isl.isl_union_map_reverse.restype = c_void_p
isl.isl_union_map_detect_equalities.restype = c_void_p
isl.isl_union_map_deltas.restype = c_void_p
isl.isl_union_map_free.argtypes = [c_void_p]
isl.isl_union_map_to_str.argtypes = [c_void_p]
isl.isl_union_map_to_str.restype = POINTER(c_char)

class map(union_map):
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        if len(args) == 1 and type(args[0]) == str:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_map_read_from_str(self.ctx, args[0])
            return
        if len(args) == 1 and args[0].__class__ is basic_map:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_map_from_basic_map(isl.isl_basic_map_copy(args[0].ptr))
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_map_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_map_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.map("%s")' % str(self)
    def lexmin(self):
        res = isl.isl_map_lexmin(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def lexmax(self):
        res = isl.isl_map_lexmax(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def reverse(self):
        res = isl.isl_map_reverse(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def union(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).union(arg1)
        res = isl.isl_map_union(isl.isl_map_copy(self.ptr), isl.isl_map_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def intersect_domain(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_map(self).intersect_domain(arg1)
        res = isl.isl_map_intersect_domain(isl.isl_map_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def intersect_range(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_map(self).intersect_range(arg1)
        res = isl.isl_map_intersect_range(isl.isl_map_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def apply_domain(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).apply_domain(arg1)
        res = isl.isl_map_apply_domain(isl.isl_map_copy(self.ptr), isl.isl_map_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def apply_range(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).apply_range(arg1)
        res = isl.isl_map_apply_range(isl.isl_map_copy(self.ptr), isl.isl_map_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def intersect(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).intersect(arg1)
        res = isl.isl_map_intersect(isl.isl_map_copy(self.ptr), isl.isl_map_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def intersect_params(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_map(self).intersect_params(arg1)
        res = isl.isl_map_intersect_params(isl.isl_map_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def subtract(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).subtract(arg1)
        res = isl.isl_map_subtract(isl.isl_map_copy(self.ptr), isl.isl_map_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def complement(self):
        res = isl.isl_map_complement(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def deltas(self):
        res = isl.isl_map_deltas(isl.isl_map_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def detect_equalities(self):
        res = isl.isl_map_detect_equalities(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def affine_hull(self):
        res = isl.isl_map_affine_hull(isl.isl_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def polyhedral_hull(self):
        res = isl.isl_map_polyhedral_hull(isl.isl_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def flatten(self):
        res = isl.isl_map_flatten(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def flatten_domain(self):
        res = isl.isl_map_flatten_domain(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def flatten_range(self):
        res = isl.isl_map_flatten_range(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def sample(self):
        res = isl.isl_map_sample(isl.isl_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def is_empty(self):
        res = isl.isl_map_is_empty(self.ptr)
        return res
    def is_subset(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).is_subset(arg1)
        res = isl.isl_map_is_subset(self.ptr, arg1.ptr)
        return res
    def is_strict_subset(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).is_strict_subset(arg1)
        res = isl.isl_map_is_strict_subset(self.ptr, arg1.ptr)
        return res
    def is_equal(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).is_equal(arg1)
        res = isl.isl_map_is_equal(self.ptr, arg1.ptr)
        return res
    def is_single_valued(self):
        res = isl.isl_map_is_single_valued(self.ptr)
        return res
    def is_injective(self):
        res = isl.isl_map_is_injective(self.ptr)
        return res
    def is_bijective(self):
        res = isl.isl_map_is_bijective(self.ptr)
        return res
    def gist(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_map(self).gist(arg1)
        res = isl.isl_map_gist(isl.isl_map_copy(self.ptr), isl.isl_map_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def gist_domain(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_map(self).gist_domain(arg1)
        res = isl.isl_map_gist_domain(isl.isl_map_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def coalesce(self):
        res = isl.isl_map_coalesce(isl.isl_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def foreach_basic_map(self, arg1):
        exc_info = [None]
        fn = CFUNCTYPE(c_int, c_void_p, c_void_p)
        def cb_func(cb_arg0, cb_arg1):
            cb_arg0 = basic_map(ctx=self.ctx, ptr=cb_arg0)
            try:
                arg1(cb_arg0)
            except:
                import sys
                exc_info[0] = sys.exc_info()
                return -1
            return 0
        cb = fn(cb_func)
        res = isl.isl_map_foreach_basic_map(self.ptr, cb, None)
        if exc_info[0] != None:
            raise exc_info[0][0], exc_info[0][1], exc_info[0][2]
        return res

isl.isl_map_read_from_str.restype = c_void_p
isl.isl_map_read_from_str.argtypes = [Context, c_char_p]
isl.isl_map_from_basic_map.restype = c_void_p
isl.isl_map_from_basic_map.argtypes = [c_void_p]
isl.isl_map_lexmin.restype = c_void_p
isl.isl_map_lexmax.restype = c_void_p
isl.isl_map_reverse.restype = c_void_p
isl.isl_map_union.restype = c_void_p
isl.isl_map_intersect_domain.restype = c_void_p
isl.isl_map_intersect_range.restype = c_void_p
isl.isl_map_apply_domain.restype = c_void_p
isl.isl_map_apply_range.restype = c_void_p
isl.isl_map_intersect.restype = c_void_p
isl.isl_map_intersect_params.restype = c_void_p
isl.isl_map_subtract.restype = c_void_p
isl.isl_map_complement.restype = c_void_p
isl.isl_map_deltas.restype = c_void_p
isl.isl_map_detect_equalities.restype = c_void_p
isl.isl_map_affine_hull.restype = c_void_p
isl.isl_map_polyhedral_hull.restype = c_void_p
isl.isl_map_flatten.restype = c_void_p
isl.isl_map_flatten_domain.restype = c_void_p
isl.isl_map_flatten_range.restype = c_void_p
isl.isl_map_sample.restype = c_void_p
isl.isl_map_gist.restype = c_void_p
isl.isl_map_gist_domain.restype = c_void_p
isl.isl_map_coalesce.restype = c_void_p
isl.isl_map_free.argtypes = [c_void_p]
isl.isl_map_to_str.argtypes = [c_void_p]
isl.isl_map_to_str.restype = POINTER(c_char)

class basic_map(map):
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        if len(args) == 1 and type(args[0]) == str:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_basic_map_read_from_str(self.ctx, args[0])
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_basic_map_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_basic_map_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.basic_map("%s")' % str(self)
    def intersect_domain(self, arg1):
        try:
            if not arg1.__class__ is basic_set:
                arg1 = basic_set(arg1)
        except:
            return map(self).intersect_domain(arg1)
        res = isl.isl_basic_map_intersect_domain(isl.isl_basic_map_copy(self.ptr), isl.isl_basic_set_copy(arg1.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def intersect_range(self, arg1):
        try:
            if not arg1.__class__ is basic_set:
                arg1 = basic_set(arg1)
        except:
            return map(self).intersect_range(arg1)
        res = isl.isl_basic_map_intersect_range(isl.isl_basic_map_copy(self.ptr), isl.isl_basic_set_copy(arg1.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def intersect(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return map(self).intersect(arg1)
        res = isl.isl_basic_map_intersect(isl.isl_basic_map_copy(self.ptr), isl.isl_basic_map_copy(arg1.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def union(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return map(self).union(arg1)
        res = isl.isl_basic_map_union(isl.isl_basic_map_copy(self.ptr), isl.isl_basic_map_copy(arg1.ptr))
        return map(ctx=self.ctx, ptr=res)
    def apply_domain(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return map(self).apply_domain(arg1)
        res = isl.isl_basic_map_apply_domain(isl.isl_basic_map_copy(self.ptr), isl.isl_basic_map_copy(arg1.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def apply_range(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return map(self).apply_range(arg1)
        res = isl.isl_basic_map_apply_range(isl.isl_basic_map_copy(self.ptr), isl.isl_basic_map_copy(arg1.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def affine_hull(self):
        res = isl.isl_basic_map_affine_hull(isl.isl_basic_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def reverse(self):
        res = isl.isl_basic_map_reverse(isl.isl_basic_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def sample(self):
        res = isl.isl_basic_map_sample(isl.isl_basic_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def detect_equalities(self):
        res = isl.isl_basic_map_detect_equalities(isl.isl_basic_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def is_equal(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return map(self).is_equal(arg1)
        res = isl.isl_basic_map_is_equal(self.ptr, arg1.ptr)
        return res
    def lexmin(self):
        res = isl.isl_basic_map_lexmin(isl.isl_basic_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def lexmax(self):
        res = isl.isl_basic_map_lexmax(isl.isl_basic_map_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def is_empty(self):
        res = isl.isl_basic_map_is_empty(self.ptr)
        return res
    def is_subset(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return map(self).is_subset(arg1)
        res = isl.isl_basic_map_is_subset(self.ptr, arg1.ptr)
        return res
    def deltas(self):
        res = isl.isl_basic_map_deltas(isl.isl_basic_map_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def flatten(self):
        res = isl.isl_basic_map_flatten(isl.isl_basic_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def flatten_domain(self):
        res = isl.isl_basic_map_flatten_domain(isl.isl_basic_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def flatten_range(self):
        res = isl.isl_basic_map_flatten_range(isl.isl_basic_map_copy(self.ptr))
        return basic_map(ctx=self.ctx, ptr=res)
    def gist(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return map(self).gist(arg1)
        res = isl.isl_basic_map_gist(isl.isl_basic_map_copy(self.ptr), isl.isl_basic_map_copy(arg1.ptr))
        return basic_map(ctx=self.ctx, ptr=res)

isl.isl_basic_map_read_from_str.restype = c_void_p
isl.isl_basic_map_read_from_str.argtypes = [Context, c_char_p]
isl.isl_basic_map_intersect_domain.restype = c_void_p
isl.isl_basic_map_intersect_range.restype = c_void_p
isl.isl_basic_map_intersect.restype = c_void_p
isl.isl_basic_map_union.restype = c_void_p
isl.isl_basic_map_apply_domain.restype = c_void_p
isl.isl_basic_map_apply_range.restype = c_void_p
isl.isl_basic_map_affine_hull.restype = c_void_p
isl.isl_basic_map_reverse.restype = c_void_p
isl.isl_basic_map_sample.restype = c_void_p
isl.isl_basic_map_detect_equalities.restype = c_void_p
isl.isl_basic_map_lexmin.restype = c_void_p
isl.isl_basic_map_lexmax.restype = c_void_p
isl.isl_basic_map_deltas.restype = c_void_p
isl.isl_basic_map_flatten.restype = c_void_p
isl.isl_basic_map_flatten_domain.restype = c_void_p
isl.isl_basic_map_flatten_range.restype = c_void_p
isl.isl_basic_map_gist.restype = c_void_p
isl.isl_basic_map_free.argtypes = [c_void_p]
isl.isl_basic_map_to_str.argtypes = [c_void_p]
isl.isl_basic_map_to_str.restype = POINTER(c_char)

class union_set:
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        if len(args) == 1 and args[0].__class__ is set:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_union_set_from_set(isl.isl_set_copy(args[0].ptr))
            return
        if len(args) == 1 and type(args[0]) == str:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_union_set_read_from_str(self.ctx, args[0])
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_union_set_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_union_set_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.union_set("%s")' % str(self)
    def identity(self):
        res = isl.isl_union_set_identity(isl.isl_union_set_copy(self.ptr))
        return union_map(ctx=self.ctx, ptr=res)
    def detect_equalities(self):
        res = isl.isl_union_set_detect_equalities(isl.isl_union_set_copy(self.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def affine_hull(self):
        res = isl.isl_union_set_affine_hull(isl.isl_union_set_copy(self.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def polyhedral_hull(self):
        res = isl.isl_union_set_polyhedral_hull(isl.isl_union_set_copy(self.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def coalesce(self):
        res = isl.isl_union_set_coalesce(isl.isl_union_set_copy(self.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def lexmin(self):
        res = isl.isl_union_set_lexmin(isl.isl_union_set_copy(self.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def lexmax(self):
        res = isl.isl_union_set_lexmax(isl.isl_union_set_copy(self.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def subtract(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_set_subtract(isl.isl_union_set_copy(self.ptr), isl.isl_union_set_copy(arg1.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def intersect(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_set_intersect(isl.isl_union_set_copy(self.ptr), isl.isl_union_set_copy(arg1.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def intersect_params(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            raise
        res = isl.isl_union_set_intersect_params(isl.isl_union_set_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def gist(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_set_gist(isl.isl_union_set_copy(self.ptr), isl.isl_union_set_copy(arg1.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def gist_params(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            raise
        res = isl.isl_union_set_gist_params(isl.isl_union_set_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def apply(self, arg1):
        try:
            if not arg1.__class__ is union_map:
                arg1 = union_map(arg1)
        except:
            raise
        res = isl.isl_union_set_apply(isl.isl_union_set_copy(self.ptr), isl.isl_union_map_copy(arg1.ptr))
        return union_set(ctx=self.ctx, ptr=res)
    def is_empty(self):
        res = isl.isl_union_set_is_empty(self.ptr)
        return res
    def is_subset(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_set_is_subset(self.ptr, arg1.ptr)
        return res
    def is_equal(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_set_is_equal(self.ptr, arg1.ptr)
        return res
    def is_strict_subset(self, arg1):
        try:
            if not arg1.__class__ is union_set:
                arg1 = union_set(arg1)
        except:
            raise
        res = isl.isl_union_set_is_strict_subset(self.ptr, arg1.ptr)
        return res
    def foreach_set(self, arg1):
        exc_info = [None]
        fn = CFUNCTYPE(c_int, c_void_p, c_void_p)
        def cb_func(cb_arg0, cb_arg1):
            cb_arg0 = set(ctx=self.ctx, ptr=cb_arg0)
            try:
                arg1(cb_arg0)
            except:
                import sys
                exc_info[0] = sys.exc_info()
                return -1
            return 0
        cb = fn(cb_func)
        res = isl.isl_union_set_foreach_set(self.ptr, cb, None)
        if exc_info[0] != None:
            raise exc_info[0][0], exc_info[0][1], exc_info[0][2]
        return res

isl.isl_union_set_from_set.restype = c_void_p
isl.isl_union_set_from_set.argtypes = [c_void_p]
isl.isl_union_set_read_from_str.restype = c_void_p
isl.isl_union_set_read_from_str.argtypes = [Context, c_char_p]
isl.isl_union_set_identity.restype = c_void_p
isl.isl_union_set_detect_equalities.restype = c_void_p
isl.isl_union_set_affine_hull.restype = c_void_p
isl.isl_union_set_polyhedral_hull.restype = c_void_p
isl.isl_union_set_coalesce.restype = c_void_p
isl.isl_union_set_lexmin.restype = c_void_p
isl.isl_union_set_lexmax.restype = c_void_p
isl.isl_union_set_subtract.restype = c_void_p
isl.isl_union_set_intersect.restype = c_void_p
isl.isl_union_set_intersect_params.restype = c_void_p
isl.isl_union_set_gist.restype = c_void_p
isl.isl_union_set_gist_params.restype = c_void_p
isl.isl_union_set_apply.restype = c_void_p
isl.isl_union_set_free.argtypes = [c_void_p]
isl.isl_union_set_to_str.argtypes = [c_void_p]
isl.isl_union_set_to_str.restype = POINTER(c_char)

class set(union_set):
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        if len(args) == 1 and args[0].__class__ is basic_set:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_set_from_basic_set(isl.isl_basic_set_copy(args[0].ptr))
            return
        if len(args) == 1 and type(args[0]) == str:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_set_read_from_str(self.ctx, args[0])
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_set_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_set_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.set("%s")' % str(self)
    def sample(self):
        res = isl.isl_set_sample(isl.isl_set_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def detect_equalities(self):
        res = isl.isl_set_detect_equalities(isl.isl_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def affine_hull(self):
        res = isl.isl_set_affine_hull(isl.isl_set_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def polyhedral_hull(self):
        res = isl.isl_set_polyhedral_hull(isl.isl_set_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def union(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).union(arg1)
        res = isl.isl_set_union(isl.isl_set_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return set(ctx=self.ctx, ptr=res)
    def intersect(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).intersect(arg1)
        res = isl.isl_set_intersect(isl.isl_set_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return set(ctx=self.ctx, ptr=res)
    def intersect_params(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).intersect_params(arg1)
        res = isl.isl_set_intersect_params(isl.isl_set_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return set(ctx=self.ctx, ptr=res)
    def subtract(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).subtract(arg1)
        res = isl.isl_set_subtract(isl.isl_set_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return set(ctx=self.ctx, ptr=res)
    def complement(self):
        res = isl.isl_set_complement(isl.isl_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def apply(self, arg1):
        try:
            if not arg1.__class__ is map:
                arg1 = map(arg1)
        except:
            return union_set(self).apply(arg1)
        res = isl.isl_set_apply(isl.isl_set_copy(self.ptr), isl.isl_map_copy(arg1.ptr))
        return set(ctx=self.ctx, ptr=res)
    def is_empty(self):
        res = isl.isl_set_is_empty(self.ptr)
        return res
    def is_subset(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).is_subset(arg1)
        res = isl.isl_set_is_subset(self.ptr, arg1.ptr)
        return res
    def is_strict_subset(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).is_strict_subset(arg1)
        res = isl.isl_set_is_strict_subset(self.ptr, arg1.ptr)
        return res
    def is_equal(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).is_equal(arg1)
        res = isl.isl_set_is_equal(self.ptr, arg1.ptr)
        return res
    def lexmin(self):
        res = isl.isl_set_lexmin(isl.isl_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def lexmax(self):
        res = isl.isl_set_lexmax(isl.isl_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def gist(self, arg1):
        try:
            if not arg1.__class__ is set:
                arg1 = set(arg1)
        except:
            return union_set(self).gist(arg1)
        res = isl.isl_set_gist(isl.isl_set_copy(self.ptr), isl.isl_set_copy(arg1.ptr))
        return set(ctx=self.ctx, ptr=res)
    def coalesce(self):
        res = isl.isl_set_coalesce(isl.isl_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def foreach_basic_set(self, arg1):
        exc_info = [None]
        fn = CFUNCTYPE(c_int, c_void_p, c_void_p)
        def cb_func(cb_arg0, cb_arg1):
            cb_arg0 = basic_set(ctx=self.ctx, ptr=cb_arg0)
            try:
                arg1(cb_arg0)
            except:
                import sys
                exc_info[0] = sys.exc_info()
                return -1
            return 0
        cb = fn(cb_func)
        res = isl.isl_set_foreach_basic_set(self.ptr, cb, None)
        if exc_info[0] != None:
            raise exc_info[0][0], exc_info[0][1], exc_info[0][2]
        return res
    def identity(self):
        res = isl.isl_set_identity(isl.isl_set_copy(self.ptr))
        return map(ctx=self.ctx, ptr=res)
    def is_wrapping(self):
        res = isl.isl_set_is_wrapping(self.ptr)
        return res
    def flatten(self):
        res = isl.isl_set_flatten(isl.isl_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)

isl.isl_set_from_basic_set.restype = c_void_p
isl.isl_set_from_basic_set.argtypes = [c_void_p]
isl.isl_set_read_from_str.restype = c_void_p
isl.isl_set_read_from_str.argtypes = [Context, c_char_p]
isl.isl_set_sample.restype = c_void_p
isl.isl_set_detect_equalities.restype = c_void_p
isl.isl_set_affine_hull.restype = c_void_p
isl.isl_set_polyhedral_hull.restype = c_void_p
isl.isl_set_union.restype = c_void_p
isl.isl_set_intersect.restype = c_void_p
isl.isl_set_intersect_params.restype = c_void_p
isl.isl_set_subtract.restype = c_void_p
isl.isl_set_complement.restype = c_void_p
isl.isl_set_apply.restype = c_void_p
isl.isl_set_lexmin.restype = c_void_p
isl.isl_set_lexmax.restype = c_void_p
isl.isl_set_gist.restype = c_void_p
isl.isl_set_coalesce.restype = c_void_p
isl.isl_set_identity.restype = c_void_p
isl.isl_set_flatten.restype = c_void_p
isl.isl_set_free.argtypes = [c_void_p]
isl.isl_set_to_str.argtypes = [c_void_p]
isl.isl_set_to_str.restype = POINTER(c_char)

class basic_set(set):
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        if len(args) == 1 and type(args[0]) == str:
            self.ctx = Context.getDefaultInstance()
            self.ptr = isl.isl_basic_set_read_from_str(self.ctx, args[0])
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_basic_set_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_basic_set_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.basic_set("%s")' % str(self)
    def intersect(self, arg1):
        try:
            if not arg1.__class__ is basic_set:
                arg1 = basic_set(arg1)
        except:
            return set(self).intersect(arg1)
        res = isl.isl_basic_set_intersect(isl.isl_basic_set_copy(self.ptr), isl.isl_basic_set_copy(arg1.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def intersect_params(self, arg1):
        try:
            if not arg1.__class__ is basic_set:
                arg1 = basic_set(arg1)
        except:
            return set(self).intersect_params(arg1)
        res = isl.isl_basic_set_intersect_params(isl.isl_basic_set_copy(self.ptr), isl.isl_basic_set_copy(arg1.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def apply(self, arg1):
        try:
            if not arg1.__class__ is basic_map:
                arg1 = basic_map(arg1)
        except:
            return set(self).apply(arg1)
        res = isl.isl_basic_set_apply(isl.isl_basic_set_copy(self.ptr), isl.isl_basic_map_copy(arg1.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def affine_hull(self):
        res = isl.isl_basic_set_affine_hull(isl.isl_basic_set_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def sample(self):
        res = isl.isl_basic_set_sample(isl.isl_basic_set_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def detect_equalities(self):
        res = isl.isl_basic_set_detect_equalities(isl.isl_basic_set_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def is_equal(self, arg1):
        res = isl.isl_basic_set_is_equal(self.ptr, arg1.ptr)
        return res
    def lexmin(self):
        res = isl.isl_basic_set_lexmin(isl.isl_basic_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def lexmax(self):
        res = isl.isl_basic_set_lexmax(isl.isl_basic_set_copy(self.ptr))
        return set(ctx=self.ctx, ptr=res)
    def union(self, arg1):
        try:
            if not arg1.__class__ is basic_set:
                arg1 = basic_set(arg1)
        except:
            return set(self).union(arg1)
        res = isl.isl_basic_set_union(isl.isl_basic_set_copy(self.ptr), isl.isl_basic_set_copy(arg1.ptr))
        return set(ctx=self.ctx, ptr=res)
    def is_empty(self):
        res = isl.isl_basic_set_is_empty(self.ptr)
        return res
    def is_subset(self, arg1):
        try:
            if not arg1.__class__ is basic_set:
                arg1 = basic_set(arg1)
        except:
            return set(self).is_subset(arg1)
        res = isl.isl_basic_set_is_subset(self.ptr, arg1.ptr)
        return res
    def gist(self, arg1):
        try:
            if not arg1.__class__ is basic_set:
                arg1 = basic_set(arg1)
        except:
            return set(self).gist(arg1)
        res = isl.isl_basic_set_gist(isl.isl_basic_set_copy(self.ptr), isl.isl_basic_set_copy(arg1.ptr))
        return basic_set(ctx=self.ctx, ptr=res)
    def is_wrapping(self):
        res = isl.isl_basic_set_is_wrapping(self.ptr)
        return res
    def flatten(self):
        res = isl.isl_basic_set_flatten(isl.isl_basic_set_copy(self.ptr))
        return basic_set(ctx=self.ctx, ptr=res)

isl.isl_basic_set_read_from_str.restype = c_void_p
isl.isl_basic_set_read_from_str.argtypes = [Context, c_char_p]
isl.isl_basic_set_intersect.restype = c_void_p
isl.isl_basic_set_intersect_params.restype = c_void_p
isl.isl_basic_set_apply.restype = c_void_p
isl.isl_basic_set_affine_hull.restype = c_void_p
isl.isl_basic_set_sample.restype = c_void_p
isl.isl_basic_set_detect_equalities.restype = c_void_p
isl.isl_basic_set_lexmin.restype = c_void_p
isl.isl_basic_set_lexmax.restype = c_void_p
isl.isl_basic_set_union.restype = c_void_p
isl.isl_basic_set_gist.restype = c_void_p
isl.isl_basic_set_flatten.restype = c_void_p
isl.isl_basic_set_free.argtypes = [c_void_p]
isl.isl_basic_set_to_str.argtypes = [c_void_p]
isl.isl_basic_set_to_str.restype = POINTER(c_char)

class pw_qpolynomial:
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_pw_qpolynomial_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_pw_qpolynomial_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.pw_qpolynomial("%s")' % str(self)

isl.isl_pw_qpolynomial_free.argtypes = [c_void_p]
isl.isl_pw_qpolynomial_to_str.argtypes = [c_void_p]
isl.isl_pw_qpolynomial_to_str.restype = POINTER(c_char)

class union_pw_qpolynomial:
    def __init__(self, *args, **keywords):
        if "ptr" in keywords:
            self.ctx = keywords["ctx"]
            self.ptr = keywords["ptr"]
            return
        raise Error
    def __del__(self):
        if hasattr(self, 'ptr'):
            isl.isl_union_pw_qpolynomial_free(self.ptr)
    def __str__(self):
        ptr = isl.isl_union_pw_qpolynomial_to_str(self.ptr)
        res = str(cast(ptr, c_char_p).value)
        libc.free(ptr)
        return res
    def __repr__(self):
        return 'isl.union_pw_qpolynomial("%s")' % str(self)

isl.isl_union_pw_qpolynomial_free.argtypes = [c_void_p]
isl.isl_union_pw_qpolynomial_to_str.argtypes = [c_void_p]
isl.isl_union_pw_qpolynomial_to_str.restype = POINTER(c_char)
