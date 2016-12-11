// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#include "../inline_deque.h"

#include "util_test.h"

template<class T>
std::string tostr(const T& q) {
    std::string ret;
    for (auto v : q) {
        char buf[32];
        sprintf(buf, "%d ", v.value());
        ret.append(buf);
    }
    return ret;
}

inline_deque<Value, 8> make_test_queue() {
    inline_deque<Value, 8> q;
    for (int i = 0; i < 4; ++i) {
        q.push_back(Value(i * 2));
        q.push_back(Value(i * 2 + 1));
        q.pop_front();
    }
    return q;
}

bool test_insert_start() {
    inline_deque<Value, 8> q = make_test_queue();

    EXPECT_INTEQ(q.size(), 4);
    EXPECT_INTEQ(q[0], 4);
    EXPECT_INTEQ(q[3], 7);
    EXPECT_STREQ(tostr(q), "4 5 6 7 ");

    {
        inline_deque<Value, 8> q2 { q };
        Value v { 100 };
        q2.insert(q2.begin(), v);
        EXPECT_STREQ(tostr(q2), "100 4 5 6 7 ");
    }

    {
        inline_deque<Value, 8> q2 { q };
        q2.insert(q2.begin(), std::move(Value(100)));
        EXPECT_STREQ(tostr(q2), "100 4 5 6 7 ");
    }

    {
        inline_deque<Value, 8> q2 { q };
        q2.insert(q2.begin(), 2, Value(100));
        EXPECT_STREQ(tostr(q2), "100 100 4 5 6 7 ");
    }

    return true;
}

bool test_insert_middle() {
    inline_deque<Value, 8> q = make_test_queue();

    {
        inline_deque<Value, 8> q2 { q };
        Value v { 100 };
        q2.insert(q2.begin() + 2, v);
        EXPECT_STREQ(tostr(q2), "4 5 100 6 7 ");
    }

    {
        inline_deque<Value, 8> q2 { q };
        q2.emplace(q2.begin() + 2, 100);
        EXPECT_STREQ(tostr(q2), "4 5 100 6 7 ");
    }

    {
        inline_deque<Value, 8> q2 { q };
        q2.insert(q2.begin() + 1, 3, Value(100));
        EXPECT_STREQ(tostr(q2), "4 100 100 100 5 6 7 ");
    }

    return true;
}

bool test_insert_end() {
    inline_deque<Value, 8> q = make_test_queue();

    {
        inline_deque<Value, 8> q2 { q };
        Value v { 100 };
        q2.insert(q2.end(), v);
        EXPECT_STREQ(tostr(q2), "4 5 6 7 100 ");
    }

    return true;
}

bool test_insert_full() {
    inline_deque<Value, 4> q;
    for (int i = 0; i < 4; ++i) {
        q.emplace_back(i);
    }

    EXPECT_INTEQ(q.size(), q.capacity());
    EXPECT_STREQ(tostr(q), "0 1 2 3 ");

    {
        inline_deque<Value, 4> q2 { q };
        Value v { 100 };
        q2.insert(q2.begin() + 1, v);
        EXPECT_STREQ(tostr(q2), "0 100 1 2 3 ");
    }

    return true;
}

int main(void) {
    bool ok = true;

    TEST(test_insert_start);
    TEST(test_insert_end);
    TEST(test_insert_middle);
    TEST(test_insert_full);

    return !ok;
}
