// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#include "../inline_deque.h"

#include "util_test.h"

std::string tostr(const inline_deque<Value, 8>& q) {
    std::string ret;
    for (auto v : q) {
        char buf[32];
        sprintf(buf, "%d ", v.value());
        ret.append(buf);
    }
    return ret;
}

bool test_erase_range() {
    inline_deque<Value, 8> q;
    for (int i = 0; i < 4; ++i) {
        q.push_back(Value(i * 2));
        q.push_back(Value(i * 2 + 1));
        q.pop_front();
    }
    EXPECT_INTEQ(q.size(), 4);
    EXPECT_INTEQ(q[0], 4);
    EXPECT_INTEQ(q[3], 7);
    EXPECT_STREQ(tostr(q), "4 5 6 7 ");

    // Empty range, delete nothing.
    {
        inline_deque<Value, 8> q2(q);
        q2.erase(q2.begin() + 1, q2.begin() + 1);
        EXPECT_STREQ(tostr(q2), "4 5 6 7 ");
    }

    // Erase first element
    {
        inline_deque<Value, 8> q2(q);
        q2.erase(q2.begin(), q2.begin() + 1);
        EXPECT_INTEQ(q2.size(), 3);
        EXPECT_INTEQ(q2[0], 5);
        EXPECT_INTEQ(q2[2], 7);
        EXPECT_STREQ(tostr(q2), "5 6 7 ");
    }

    // Erase first two elements
    {
        inline_deque<Value, 8> q2(q);
        q2.erase(q2.begin(), q2.begin() + 2);
        EXPECT_INTEQ(q2.size(), 2);
        EXPECT_INTEQ(q2[0], 6);
        EXPECT_INTEQ(q2[1], 7);
        EXPECT_STREQ(tostr(q2), "6 7 ");
    }

    // Erase last element
    {
        inline_deque<Value, 8> q2(q);
        q2.erase(q2.end() - 1, q2.end());
        EXPECT_INTEQ(q2.size(), 3);
        EXPECT_INTEQ(q2[0], 4);
        EXPECT_INTEQ(q2[2], 6);
        EXPECT_STREQ(tostr(q2), "4 5 6 ");
    }

    // Erase two from the middle
    {
        inline_deque<Value, 8> q2(q);
        q2.erase(q2.begin() + 1, q2.begin() + 3);
        EXPECT_INTEQ(q2.size(), 2);
        EXPECT_INTEQ(q2[0], 4);
        EXPECT_INTEQ(q2[1], 7);
        EXPECT_STREQ(tostr(q2), "4 7 ");
    }

    return true;
}

int main(void) {
    bool ok = true;

    TEST(test_erase_range);

    return !ok;
}
