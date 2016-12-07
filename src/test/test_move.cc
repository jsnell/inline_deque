// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#include "../inline_deque.h"

#include "util_test.h"

bool test_move_inline() {
    inline_deque<std::string, 4> q1, q2;
    q1.push_front("str1");
    q1.push_back("str2");

    std::swap(q1, q2);

    EXPECT(q2.front() == "str1");
    EXPECT(q2.back() == "str2");

    return true;
}

bool test_move_heap() {
    inline_deque<std::string, 1> q1, q2;
    q1.push_front("str1");
    q1.push_back("str2");

    std::swap(q1, q2);

    EXPECT(q2.front() == "str1");
    EXPECT(q2.back() == "str2");

    return true;
}

int main(void) {
    bool ok = true;

    TEST(test_move_inline);
    TEST(test_move_heap);

    return !ok;
}
