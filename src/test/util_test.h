// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#ifndef UTIL_TEST_H
#define UTIL_TEST_H

#include <cassert>

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

#define EXPECT_STREQ(actual, expect)                    \
    do {                                                \
        if (expect != actual)  {                        \
            printf("%s:%d: Expect failed, wanted '%s'"  \
                   " got '%s'\n",                       \
                   __FILE__, __LINE__,                  \
                   std::string(expect).c_str(),         \
                   std::string(actual).c_str());        \
            return false;                               \
        }                                               \
    } while (0)

#define EXPECT_THROW(expr, except)                      \
    do {                                                \
        bool ok = false;                                \
        try { expr; } catch (except& e) { ok = true; }  \
        if (!ok) {                                      \
            printf("%s:%d: Expected %s: %s\n",          \
                   __FILE__, __LINE__,                  \
                   #except, #expr);                     \
            return false;                               \
        }                                               \
    } while (0)

// A class with some move semantics
class Value {
public:
    explicit Value(uint32_t val) : val_(val) {
        ++live_;
    }

    Value(const Value& other) : Value(other.val_) {
    }

    Value(Value&& other) : Value(other.val_) {
        other.val_ = 0x88888888;
    }

    ~Value() {
        val_ = 0xffffffff;
        deleted_ = true;
        --live_;
    }

    Value& operator=(const Value& other) {
        assert(!deleted_);
        val_ = other.val_;
        return *this;
    }

    Value& operator=(Value&& other) {
        assert(!deleted_);
        val_ = other.val_;
        other.val_ = 0x88888888;
        return *this;
    }

    uint32_t value() {
        return val_;
    }

    operator uint32_t() const {
        assert(!deleted_);
        return val_;
    }


    static uint64_t live_;

private:
    uint32_t val_;
    bool deleted_ = false;
};

uint64_t Value::live_ = 0;

#endif // UTIL_TEST_H
