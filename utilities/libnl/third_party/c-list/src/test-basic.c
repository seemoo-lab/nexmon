/*
 * Tests for basic functionality
 * This contains basic, deterministic tests for list behavior, API
 * functionality, and usage.
 */

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c-list.h"

static void assert_list_integrity(CList *list) {
        CList *iter;

        iter = list;
        do {
                assert(iter->next->prev == iter);
                assert(iter->prev->next == iter);

                iter = iter->next;
        } while (iter != list);
}

static void test_iterators(void) {
        CList *iter, *safe, a, b, list = C_LIST_INIT(list);
        unsigned int i;

        assert(!c_list_first(&list));
        assert(!c_list_last(&list));

        /* link @a and verify iterators see just it */

        c_list_link_tail(&list, &a);
        assert(c_list_is_linked(&a));
        assert(c_list_first(&list) == &a);
        assert(c_list_last(&list) == &a);

        i = 0;
        c_list_for_each(iter, &list) {
                assert(iter == &a);
                ++i;
        }
        assert(i == 1);

        i = 0;
        iter = NULL;
        c_list_for_each_continue(iter, &list) {
                assert(iter == &a);
                ++i;
        }
        assert(i == 1);

        i = 0;
        iter = &a;
        c_list_for_each_continue(iter, &list)
                ++i;
        assert(i == 0);

        /* link @b as well and verify iterators again */

        c_list_link_tail(&list, &b);
        assert(c_list_is_linked(&a));
        assert(c_list_is_linked(&b));

        i = 0;
        c_list_for_each(iter, &list) {
                assert((i == 0 && iter == &a) ||
                       (i == 1 && iter == &b));
                ++i;
        }
        assert(i == 2);

        i = 0;
        iter = NULL;
        c_list_for_each_continue(iter, &list) {
                assert((i == 0 && iter == &a) ||
                       (i == 1 && iter == &b));
                ++i;
        }
        assert(i == 2);

        i = 0;
        iter = &a;
        c_list_for_each_continue(iter, &list) {
                assert(iter == &b);
                ++i;
        }
        assert(i == 1);

        i = 0;
        iter = &b;
        c_list_for_each_continue(iter, &list)
                ++i;
        assert(i == 0);

        /* verify safe-iterator while removing elements */

        i = 0;
        c_list_for_each_safe(iter, safe, &list) {
                assert(iter == &a || iter == &b);
                c_list_unlink_stale(iter);
                ++i;
        }
        assert(i == 2);

        assert(c_list_is_empty(&list));

        /* link both and verify *_unlink() iterators */

        c_list_link_tail(&list, &a);
        c_list_link_tail(&list, &b);

        i = 0;
        c_list_for_each_safe_unlink(iter, safe, &list) {
                assert(iter == &a || iter == &b);
                assert(!c_list_is_linked(iter));
                ++i;
        }
        assert(i == 2);

        assert(c_list_is_empty(&list));
}

static void test_swap(void) {
        CList list1 = (CList)C_LIST_INIT(list1);
        CList list2 = (CList)C_LIST_INIT(list2);
        CList list;

        c_list_swap(&list1, &list2);

        assert(list1.prev == list1.next && list1.prev == &list1);
        assert(list2.prev == list2.next && list2.prev == &list2);

        c_list_link_tail(&list1, &list);

        assert(c_list_first(&list1) == &list);
        assert(c_list_last(&list1) == &list);
        assert(list.next == &list1);
        assert(list.prev == &list1);

        c_list_swap(&list1, &list2);

        assert(c_list_first(&list2) == &list);
        assert(c_list_last(&list2) == &list);
        assert(list.next == &list2);
        assert(list.prev == &list2);

        assert(list1.prev == list1.next && list1.prev == &list1);
}

static void test_splice(void) {
        CList target = (CList)C_LIST_INIT(target);
        CList source = (CList)C_LIST_INIT(source);
        CList e1, e2;

        c_list_link_tail(&source, &e1);

        c_list_splice(&target, &source);
        assert(c_list_first(&target) == &e1);
        assert(c_list_last(&target) == &e1);

        source = (CList)C_LIST_INIT(source);

        c_list_link_tail(&source, &e2);

        c_list_splice(&target, &source);
        assert(c_list_first(&target) == &e1);
        assert(c_list_last(&target) == &e2);
}

static void test_split(void) {
        CList e1, e2;

        /* split empty list */
        {
                CList source = C_LIST_INIT(source), target;

                c_list_split(&source, &source, &target);
                assert(c_list_is_empty(&source));
                assert(c_list_is_empty(&target));
                assert_list_integrity(&source);
                assert_list_integrity(&target);
        }

        /* split 1-element list excluding the element */
        {
                CList source = C_LIST_INIT(source), target;

                c_list_link_tail(&source, &e1);
                c_list_split(&source, &source, &target);
                assert(!c_list_is_empty(&source));
                assert(c_list_is_empty(&target));
                assert_list_integrity(&source);
                assert_list_integrity(&target);
        }

        /* split 1-element list including the element */
        {
                CList source = C_LIST_INIT(source), target;

                c_list_link_tail(&source, &e1);
                c_list_split(&source, &e1, &target);
                assert(c_list_is_empty(&source));
                assert(!c_list_is_empty(&target));
                assert_list_integrity(&source);
                assert_list_integrity(&target);
        }

        /* split 2-element list excluding the elements */
        {
                CList source = C_LIST_INIT(source), target;

                c_list_link_tail(&source, &e1);
                c_list_link_tail(&source, &e2);
                c_list_split(&source, &source, &target);
                assert(!c_list_is_empty(&source));
                assert(c_list_is_empty(&target));
                assert_list_integrity(&source);
                assert_list_integrity(&target);
        }

        /* split 2-element list including one element */
        {
                CList source = C_LIST_INIT(source), target;

                c_list_link_tail(&source, &e1);
                c_list_link_tail(&source, &e2);
                c_list_split(&source, &e2, &target);
                assert(!c_list_is_empty(&source));
                assert(!c_list_is_empty(&target));
                assert_list_integrity(&source);
                assert_list_integrity(&target);
        }

        /* split 2-element list including both elements */
        {
                CList source = C_LIST_INIT(source), target;

                c_list_link_tail(&source, &e1);
                c_list_link_tail(&source, &e2);
                c_list_split(&source, &e1, &target);
                assert(c_list_is_empty(&source));
                assert(!c_list_is_empty(&target));
                assert_list_integrity(&source);
                assert_list_integrity(&target);
        }
}


static void test_flush(void) {
        CList e1 = C_LIST_INIT(e1), e2 = C_LIST_INIT(e2);
        CList list1 = C_LIST_INIT(list1), list2 = C_LIST_INIT(list2);

        c_list_link_tail(&list2, &e1);
        c_list_link_tail(&list2, &e2);

        assert(c_list_is_linked(&e1));
        assert(c_list_is_linked(&e2));

        c_list_flush(&list1);
        c_list_flush(&list2);

        assert(!c_list_is_linked(&e1));
        assert(!c_list_is_linked(&e2));
}

static void test_macros(void) {
        /* Verify `c_list_entry()` evaluates arguments only once. */
        {
                struct TestList {
                        int a;
                        CList link;
                        int b;
                } list = { .link = C_LIST_INIT(list.link) };
                CList *p[2] = { &list.link, NULL };
                unsigned int i = 0;

                assert(i == 0);
                assert(c_list_entry(p[i++], struct TestList, link) == &list);
                assert(i == 1);
        }
}

#if defined(__GNUC__) || defined(__clang__)
static void test_gnu(void) {
        CList e1 = C_LIST_INIT(e1), e2 = C_LIST_INIT(e2);

        /* Test `c_list_flush()` in combination with cleanup attributes. */
        {
                __attribute((cleanup(c_list_flush))) CList list1 = C_LIST_INIT(list1);
                __attribute((cleanup(c_list_flush))) CList list2 = C_LIST_INIT(list2);

                c_list_link_tail(&list2, &e1);
                c_list_link_tail(&list2, &e2);

                assert(c_list_is_linked(&e1));
                assert(c_list_is_linked(&e2));
        }

        assert(!c_list_is_linked(&e1));
        assert(!c_list_is_linked(&e2));
}
#else
static void test_gnu(void) {
}
#endif

int main(void) {
        test_iterators();
        test_swap();
        test_splice();
        test_split();
        test_flush();
        test_macros();
        test_gnu();
        return 0;
}
