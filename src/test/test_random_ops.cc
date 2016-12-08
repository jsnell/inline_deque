// Copyright 2016 Juho Snellman

#include <chrono>
#include <cstring>
#include <deque>
#include <limits>
#include <map>
#include <random>
#include <vector>

#include "../inline_deque.h"

static std::uniform_int_distribution<uint64_t> rand_uint64 {
    0, std::numeric_limits<uint64_t>::max()
};

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
        --live_;
    }

    Value& operator=(const Value& other) {
        val_ = other.val_;
        return *this;
    }

    Value& operator=(Value&& other) {
        val_ = other.val_;
        other.val_ = 0x88888888;
        return *this;
    }

    uint32_t value() {
        return val_;
    }

    static uint64_t live_;

private:
    uint32_t val_;
};

uint64_t Value::live_ = 0;

template<class Q>
struct Worker {
    Worker() {
    }

    ~Worker() {
    }

    void setup(std::mt19937_64* rand) {
    }

    void step(std::mt19937_64* rand) {
        while (queue_.size() == target_) {
            target_ = rand_uint64(*rand) % 0xf;
        }

        auto val = rand_uint64(*rand) & 0xffff;
        if (queue_.size() < target_) {
            switch (val & 3) {
            case 0:
                queue_.push_back(Value(val));
                break;
            case 1:
                queue_.emplace_back(val);
                break;
            case 2:
                queue_.push_front(Value(val));
                break;
            case 3:
                queue_.emplace_front(val);
                break;
            }
        } else {
            if (val & 1) {
                mix(queue_.back().value());
                queue_.pop_back();
            } else {
                mix(queue_.front().value());
                queue_.pop_front();
            }
        }

        switch (val & 0xff) {
        case 0:
            queue_.shrink_to_fit();
            break;
        case 1:
            for (auto v : queue_) {
                mix(v.value());
            }
            break;
        // Various combinations of moves and copies.
        case 2:
            std::swap(queue_, other_queue_);
            break;
        case 3: {
            Q tmp(queue_);
            queue_ = other_queue_;
            other_queue_ = tmp;
            break;
        }
        case 4: {
            Q tmp(std::move(queue_));
            queue_ = std::move(other_queue_);
            other_queue_ = std::move(tmp);
            break;
        }
        case 5: {
            Q tmp(queue_);
            queue_ = std::move(other_queue_);
            other_queue_ = std::move(tmp);
            break;
        }
        case 6: {
            Q tmp(std::move(queue_));
            queue_ = other_queue_;
            other_queue_ = std::move(tmp);
            break;
        }
        default:
            break;
        }

        mix(Value::live_);
        mix(queue_.size());
        if (!queue_.empty()) {
            mix(queue_[rand_uint64(*rand) % queue_.size()].value());
        }
    }

    void mix(uint64_t val) {
        csum_ = ((csum_ << 5) + val) ^ csum_;
    }

    Worker(const Worker<Q>& w) = delete;
    void operator=(const Worker<Q>& w) = delete;

    Q queue_;
    Q other_queue_;
    uint64_t csum_ = 0;
    uint64_t target_ = 0;
};

template<class Q>
struct Master {
    Master(int n)
        : workers_(n) {
        rand_.seed(1234);
    }

    uint64_t run() {
        setup();
        uint64_t csum = 0;
        for (int i = 0; i < 1 << 13; ++i) {
            csum = 0;
            for (auto& w : workers_) {
                w.step(&rand_);
                csum ^= w.csum_;
            }
            // printf("%d: %lu\n", i, csum);
        }
        return csum;
    }

    void setup() {
        for (auto& w : workers_) {
            w.setup(&rand_);
        }
    }

    std::vector<Worker<Q>> workers_;
    std::mt19937_64 rand_;
};

template<typename Q>
uint64_t test_random(const char* label, int n,
                     std::map<uint64_t, std::vector<std::string>>* csums) {
    uint64_t csum;
    {
        Value::live_ = 0;
        Master<Q> master(n);
        master.setup();
        csum = master.run();
    }
    (*csums)[csum].push_back(label);

    return csum;
}

int main(int argc, char** argv) {
    int n = 1000;
    if (argc > 1) {
        sscanf(argv[1], "%d", &n);
    }
    std::map<uint64_t, std::vector<std::string>> csums;
    test_random<inline_deque<Value, 0, 0, uint16_t>>(
        "inline_deque<0>", n, &csums);
    test_random<inline_deque<Value, 1, 0, uint16_t>>(
        "inline_deque<1, 0>", n, &csums);
    test_random<inline_deque<Value, 1, 1, uint16_t>>(
        "inline_deque<1>", n, &csums);
    test_random<inline_deque<Value, 2, 2, uint16_t>>(
        "inline_deque<2>", n, &csums);
    test_random<inline_deque<Value, 4, 4, uint16_t>>(
        "inline_deque<4>", n, &csums);
    test_random<inline_deque<Value, 16>>(
        "inline_deque<16>", n, &csums);
    test_random<std::deque<Value>>("deque<>", n, &csums);

    if (csums.size() == 1) {
        printf("OK\n");
    } else {
        printf("FAIL: Checksum mismatch\n");
        for (auto v : csums) {
            printf("  %lu:\n", v.first);
            for (auto s : v.second) {
                printf("    %s\n", s.c_str());
            }
        }
        return 1;
    }

    return 0;
}
