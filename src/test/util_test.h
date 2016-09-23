// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#ifndef UTIL_TEST_H
#define UTIL_TEST_H

#define TEST(fun) \
    do {                                              \
        if (fun()) {                                  \
            printf("[OK] %s\n", #fun);                \
        } else {                                      \
            ok = false;                               \
            printf("[FAILED] %s\n", #fun);            \
        }                                             \
    } while (0)


#define EXPECT(expr)                                    \
    do {                                                \
        if (!(expr))  {                                 \
            printf("%s:%d: Expect failed: %s\n",        \
                   __FILE__, __LINE__, #expr);          \
            return false;                               \
        }                                               \
    } while (0)

#define EXPECT_INTEQ(actual, expect)                    \
    do {                                                \
        if (expect != actual)  {                        \
            printf("%s:%d: Expect failed, wanted %ld"   \
                   " got %ld\n",                        \
                   __FILE__, __LINE__,                  \
                   (long) expect, (long) actual);       \
            return false;                               \
        }                                               \
    } while (0)

#endif // UTIL_TEST_H
