// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#include "../inline_deque.h"

#include "util_test.h"

bool test_constructor_default_settings() {
    inline_deque<uint32_t> q;

    EXPECT_INTEQ(q.size(), 0);
    EXPECT_INTEQ(q.capacity(), 1);
    q.push_back(1);
    EXPECT_INTEQ(q.capacity(), 1);
    q.push_back(2);
    EXPECT_INTEQ(q.capacity(), 2);

    return true;
}

static int normal_constructor_count = 0;
static int copy_constructor_count = 0;
static int move_constructor_count = 0;
static int destructor_count = 0;

void reset_counters() {
    normal_constructor_count = 0;
    copy_constructor_count = 0;
    move_constructor_count = 0;
    destructor_count = 0;
}

struct CopyCounter {
    // Make sure inline_deque doesn't require a default constructor.
    CopyCounter() = delete;

    explicit CopyCounter(int n) : n_(n) {
        ++normal_constructor_count;
    }
    CopyCounter(const CopyCounter& other) {
        ++copy_constructor_count;
    }
    CopyCounter(CopyCounter&& other) {
        ++move_constructor_count;
    }
    ~CopyCounter() {
        ++destructor_count;
    }

    int n_;
};

bool test_copy_constructor() {
    reset_counters();

    inline_deque<CopyCounter> q1;

    q1.push_back(CopyCounter(3));
    EXPECT_INTEQ(normal_constructor_count, 1);
    EXPECT_INTEQ(copy_constructor_count, 1);
    EXPECT_INTEQ(move_constructor_count, 0);
    EXPECT_INTEQ(destructor_count, 1);

    inline_deque<CopyCounter> q2(q1);
    EXPECT_INTEQ(normal_constructor_count, 1);
    EXPECT_INTEQ(copy_constructor_count, 2);
    EXPECT_INTEQ(move_constructor_count, 0);
    EXPECT_INTEQ(destructor_count, 1);
    EXPECT_INTEQ(q2.size(), q1.size());
    EXPECT(&q2.front() != &q1.front());

    return true;
}

bool test_move_constructor() {
    reset_counters();

    {
        inline_deque<CopyCounter> q1;

        q1.emplace_back(3);
        EXPECT_INTEQ(normal_constructor_count, 1);
        EXPECT_INTEQ(copy_constructor_count, 0);
        EXPECT_INTEQ(move_constructor_count, 0);
        EXPECT_INTEQ(destructor_count, 0);
        CopyCounter* p1 = &q1.front();

        inline_deque<CopyCounter> q2(std::move(q1));
        EXPECT_INTEQ(normal_constructor_count, 1);
        EXPECT_INTEQ(copy_constructor_count, 0);
        // Just one element, so allocated inline. Call the move constructor.
        EXPECT_INTEQ(move_constructor_count, 1);
        // +1 for destroying the moved inline object.
        EXPECT_INTEQ(destructor_count, 1);
        EXPECT(&q2.front() != p1);

        // Add a second element.
        q2.emplace_back(4);
        // +1 for constructing "4"
        EXPECT_INTEQ(normal_constructor_count, 2);
        EXPECT_INTEQ(copy_constructor_count, 0);
        // +1 for moving "3" from inline to the vector.
        EXPECT_INTEQ(move_constructor_count, 2);
        // +1 for destroying the moved inline object.
        EXPECT_INTEQ(destructor_count, 2);
        CopyCounter* p2 = &q2.front();

        inline_deque<CopyCounter> q3(std::move(q2));
        // The internal array was moved as a unit. Nothing copied,
        // pointer equality to original array.
        EXPECT_INTEQ(normal_constructor_count, 2);
        EXPECT_INTEQ(copy_constructor_count, 0);
        EXPECT_INTEQ(move_constructor_count, 2);
        EXPECT_INTEQ(destructor_count, 2);
        EXPECT(&q3.front() == p2);
        // Neither of the following is a guarantee of the API, but they
        // are good indications of q2 having been invalidated properly.
        EXPECT_INTEQ(q2.capacity(), 1);
        EXPECT_INTEQ(q2.size(), 0);

        // Assignment must still be valid for objects that have been
        // moved from.
        q1 = inline_deque<CopyCounter>(16);
        EXPECT_INTEQ(q1.capacity(), 16);
        q2 = inline_deque<CopyCounter>(16);
        EXPECT_INTEQ(q2.capacity(), 16);
    }
    EXPECT_INTEQ(destructor_count, 4);

    return true;
}

bool test_initial_capacity() {
    // Round initial capacity to power of 2.
    {
        inline_deque<uint32_t> q(11);
        EXPECT_INTEQ(q.capacity(), 16);
    }

    // Initial capacity should not be below inline capacity.
    inline_deque<uint32_t, 32> q(11);
    EXPECT_INTEQ(q.capacity(), 32);

    return true;
}

bool test_no_inline() {
    inline_deque<std::string, 0, uint16_t> q;
    EXPECT_INTEQ(q.size(), 0);
    q.push_front(std::string("a"));
    EXPECT_INTEQ(q.size(), 1);
    EXPECT(q.front() == "a");

    return true;
}

bool test_initializer_list() {
    inline_deque<std::string, 0, uint16_t> q { "a", "b" };
    EXPECT_INTEQ(q.size(), 2);
    EXPECT(q.front() == "a");
    EXPECT(q.back() == "b");

    return true;
}

int main(void) {
    bool ok = true;

    TEST(test_constructor_default_settings);
    TEST(test_copy_constructor);
    TEST(test_move_constructor);
    TEST(test_initial_capacity);
    TEST(test_no_inline);
    TEST(test_initializer_list);

    return !ok;
}
