/*
 * Tests for embedded CList members
 */

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "c-list.h"

typedef struct Entry Entry;

struct Entry {
        short foo;
        CList link;
        short bar;
};

static void test_entry(void) {
        CList list = C_LIST_INIT(list);
        Entry e1 = { .foo = 1 * 7, .bar = 1 * 11 };
        Entry e2 = { .foo = 2 * 7, .bar = 2 * 11 };
        Entry e3 = { .foo = 3 * 7, .bar = 3 * 11 };
        Entry e4 = { .foo = 4 * 7, .bar = 4 * 11 };
        Entry *e;
        CList *iter, *safe;
        size_t i;

        /* verify c_list_entry() works as expected (even with NULL) */

        assert(!c_list_entry(NULL, Entry, link));
        assert(&e1 == c_list_entry(&e1.link, Entry, link));

        /* verify @list is empty */

        assert(!c_list_first_entry(&list, Entry, link));
        assert(!c_list_last_entry(&list, Entry, link));

        /* link 2 entries and verify list state */

        c_list_link_tail(&list, &e1.link);
        c_list_link_tail(&list, &e2.link);

        assert(c_list_first_entry(&list, Entry, link)->foo == 1 * 7);
        assert(c_list_first_entry(&list, Entry, link)->bar == 1 * 11);
        assert(c_list_last_entry(&list, Entry, link)->foo == 2 * 7);
        assert(c_list_last_entry(&list, Entry, link)->bar == 2 * 11);

        i = 0;
        c_list_for_each(iter, &list) {
                e = c_list_entry(iter, Entry, link);
                assert(i != 0 || e == &e1);
                assert(i != 1 || e == &e2);
                assert(i < 2);
                ++i;
        }
        assert(i == 2);

        /* link 2 more entries */

        c_list_link_tail(&list, &e3.link);
        c_list_link_tail(&list, &e4.link);

        assert(c_list_first_entry(&list, Entry, link)->foo == 1 * 7);
        assert(c_list_first_entry(&list, Entry, link)->bar == 1 * 11);
        assert(c_list_last_entry(&list, Entry, link)->foo == 4 * 7);
        assert(c_list_last_entry(&list, Entry, link)->bar == 4 * 11);

        i = 0;
        c_list_for_each(iter, &list) {
                e = c_list_entry(iter, Entry, link);
                assert(i != 0 || e == &e1);
                assert(i != 1 || e == &e2);
                assert(i != 2 || e == &e3);
                assert(i != 3 || e == &e4);
                assert(i < 4);
                ++i;
        }
        assert(i == 4);

        assert(!c_list_is_empty(&list));
        assert(c_list_is_linked(&e1.link));
        assert(c_list_is_linked(&e2.link));
        assert(c_list_is_linked(&e3.link));
        assert(c_list_is_linked(&e4.link));

        /* remove via safe iterator */

        i = 0;
        c_list_for_each_safe(iter, safe, &list) {
                e = c_list_entry(iter, Entry, link);
                assert(i != 0 || e == &e1);
                assert(i != 1 || e == &e2);
                assert(i != 2 || e == &e3);
                assert(i != 3 || e == &e4);
                assert(i < 4);
                ++i;
                c_list_unlink(&e->link);
        }
        assert(i == 4);

        assert(c_list_is_empty(&list));
        assert(!c_list_is_linked(&e1.link));
        assert(!c_list_is_linked(&e2.link));
        assert(!c_list_is_linked(&e3.link));
        assert(!c_list_is_linked(&e4.link));
}

#if defined(__GNUC__) || defined(__clang__)
static void test_entry_gnu(void) {
        CList list = C_LIST_INIT(list);
        Entry e1 = { .foo = 1 * 7, .bar = 1 * 11 };
        Entry e2 = { .foo = 2 * 7, .bar = 2 * 11 };
        Entry e3 = { .foo = 3 * 7, .bar = 3 * 11 };
        Entry e4 = { .foo = 4 * 7, .bar = 4 * 11 };
        Entry *e, *safe;
        size_t i;

        /* link entries and verify list state */

        c_list_link_tail(&list, &e1.link);
        c_list_link_tail(&list, &e2.link);
        c_list_link_tail(&list, &e3.link);
        c_list_link_tail(&list, &e4.link);

        i = 0;
        c_list_for_each_entry(e, &list, link) {
                assert(i != 0 || e == &e1);
                assert(i != 1 || e == &e2);
                assert(i != 2 || e == &e3);
                assert(i != 3 || e == &e4);
                assert(i < 4);
                ++i;
        }
        assert(i == 4);

        assert(!c_list_is_empty(&list));
        assert(c_list_is_linked(&e1.link));
        assert(c_list_is_linked(&e2.link));
        assert(c_list_is_linked(&e3.link));
        assert(c_list_is_linked(&e4.link));

        /* remove via safe iterator */

        i = 0;
        c_list_for_each_entry_safe(e, safe, &list, link) {
                assert(i != 0 || e == &e1);
                assert(i != 1 || e == &e2);
                assert(i != 2 || e == &e3);
                assert(i != 3 || e == &e4);
                assert(i < 4);
                ++i;
                c_list_unlink(&e->link);
        }
        assert(i == 4);

        assert(c_list_is_empty(&list));
        assert(!c_list_is_linked(&e1.link));
        assert(!c_list_is_linked(&e2.link));
        assert(!c_list_is_linked(&e3.link));
        assert(!c_list_is_linked(&e4.link));
}
#else
static void test_entry_gnu(void) {
}
#endif

int main(void) {
        test_entry();
        test_entry_gnu();
        return 0;
}
