// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#include "../ring_buffer_queue.h"

#include "util_test.h"

bool test_constructor_default_settings() {
    ring_buffer_queue<uint32_t> q;

    EXPECT_INTEQ(q.size(), 0);
    EXPECT_INTEQ(q.capacity(), 1);
    q.push_back(1);
    EXPECT_INTEQ(q.capacity(), 1);
    q.push_back(2);
    EXPECT_INTEQ(q.capacity(), 8);

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
    // Make sure ring_buffer_queue doesn't require a default constructor.
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

    ring_buffer_queue<CopyCounter> q1;

    q1.push_back(CopyCounter(3));
    EXPECT_INTEQ(normal_constructor_count, 1);
    EXPECT_INTEQ(copy_constructor_count, 1);
    EXPECT_INTEQ(move_constructor_count, 0);
    EXPECT_INTEQ(destructor_count, 1);

    ring_buffer_queue<CopyCounter> q2(q1);
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
        ring_buffer_queue<CopyCounter> q1;

        q1.emplace_back(3);
        EXPECT_INTEQ(normal_constructor_count, 1);
        EXPECT_INTEQ(copy_constructor_count, 0);
        EXPECT_INTEQ(move_constructor_count, 0);
        EXPECT_INTEQ(destructor_count, 0);
        CopyCounter* p1 = &q1.front();

        ring_buffer_queue<CopyCounter> q2(std::move(q1));
        EXPECT_INTEQ(normal_constructor_count, 1);
        EXPECT_INTEQ(copy_constructor_count, 0);
        // Just one element, so allocated inline. Call the move constructor.
        EXPECT_INTEQ(move_constructor_count, 1);
        EXPECT_INTEQ(destructor_count, 0);
        EXPECT(&q2.front() != p1);

        // Add a second element.
        q2.emplace_back(4);
        // +1 for constructing "4"
        EXPECT_INTEQ(normal_constructor_count, 2);
        EXPECT_INTEQ(copy_constructor_count, 0);
        // +1 for moving "3" from inline to the vector
        EXPECT_INTEQ(move_constructor_count, 2);
        EXPECT_INTEQ(destructor_count, 0);
        CopyCounter* p2 = &q2.front();

        ring_buffer_queue<CopyCounter> q3(std::move(q2));
        // The internal array was moved as a unit. Nothing copied,
        // pointer equality to original array.
        EXPECT_INTEQ(normal_constructor_count, 2);
        EXPECT_INTEQ(copy_constructor_count, 0);
        EXPECT_INTEQ(move_constructor_count, 2);
        EXPECT_INTEQ(destructor_count, 0);
        EXPECT(&q3.front() == p2);
        // Not a guarantee of the API, but a good indication of q2 having
        // been invalidated properly.
        EXPECT_INTEQ(q2.capacity(), 0);

        // Assignment must still be valid for objects that have been
        // moved from.
        q1 = ring_buffer_queue<CopyCounter>(16);
        EXPECT_INTEQ(q1.capacity(), 16);
        q2 = ring_buffer_queue<CopyCounter>(16);
        EXPECT_INTEQ(q2.capacity(), 16);
    }
    EXPECT_INTEQ(destructor_count, 2);

    return true;
}

bool test_initial_capacity() {
    // Round initial capacity to power of 2.
    {
        ring_buffer_queue<uint32_t> q(11);
        EXPECT_INTEQ(q.capacity(), 16);
    }

    // Initial capacity should not be below minimum capacity (except for
    // special case of inline allocation.
    {
        ring_buffer_queue<uint32_t, 1, 1, 32> q(11);
        EXPECT_INTEQ(q.capacity(), 32);

        ring_buffer_queue<uint32_t, 1, 1, 32> q2(1);
        EXPECT_INTEQ(q2.capacity(), 1);

        ring_buffer_queue<uint32_t, 16, 16, 32> q3(11);
        EXPECT_INTEQ(q3.capacity(), 16);
    }

    return true;
}

int main(void) {
    bool ok = true;

    TEST(test_constructor_default_settings);
    TEST(test_copy_constructor);
    TEST(test_move_constructor);
    TEST(test_initial_capacity);

    return !ok;
}
