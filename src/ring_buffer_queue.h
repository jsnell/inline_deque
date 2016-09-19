// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#ifndef RING_BUFFER_QUEUE_H
#define RING_BUFFER_QUEUE_H

#include <stdexcept>
#include <cstddef>
#include <limits>

template<typename T,
         size_t InitialCapacity = 8,
         size_t MinimumCapacity = 8,
         class Allocator = std::allocator<T>>
class ring_buffer_queue {
public:
    ring_buffer_queue(size_t initial_capacity = InitialCapacity,
                      const Allocator& alloc = Allocator())
        : capacity_(initial_capacity),
          ptr_(alloc) {
        capacity_ = 1;
        while (capacity_ < initial_capacity) {
            capacity_ *= 2;
        }
        e_ = ptr_.allocate(capacity_);
    }

    ~ring_buffer_queue() {
        reset();
    }

    // Adding new elements at back of queue.

    void push_back(const T& e) {
        if (full()) {
            overflow();
        }
        ptr_.construct(&e_[ptr_write()], e);
        ptr_.write_++;
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (full()) {
            overflow();
        }
        ptr_.construct(&e_[ptr_write()],
                       std::forward<Args>(args)...);
        ptr_.write_++;
    }

    // Accessing items (front, back, random access, pop).

    const T& front() const {
        require_nonempty();
        return e_[ptr_read()];
    }

    const T& back() const {
        require_nonempty();
        return e_[ptr_write(-1)];
    }

    T& operator[] (size_t i) {
        return e_[ptr_read(i)];
    }

    const T& operator[] (size_t i) const {
        return e_[ptr_read(i)];
    }

    T& at(size_t i) {
        if (i >= size()) {
            throw std::out_of_range("index too large");
        }
        return e_[ptr_read(i)];
    }

    void pop_front() {
        require_nonempty();
        ptr_.destroy(&e_[ptr_read()]);
        ptr_.read_++;
        shrink();
    }

    void pop_back() {
        require_nonempty();
        ptr_.write_--;
        ptr_.destroy(&e_[ptr_write()]);
        shrink();
    }

    // Size of queue

    bool empty() const {
        return size() == 0;
    }

    size_t size() const {
        return ptr_.write_ - ptr_.read_;
    }

    size_t capacity() const {
        return capacity_;
    }

    size_t max_capacity() const {
        return std::numeric_limits<uint32_t>::max();
    }

    void clear() {
        while (!empty()) {
            pop_front();
        }
    }

    void shrink_to_fit() {
        size_t new_capacity = capacity_;
        while (new_capacity > size() * 2) {
            new_capacity /= 2;
        }
    }

    // Copying / assignment

    ring_buffer_queue(const ring_buffer_queue& other)
        : ptr_(other.ptr_) {
        clone_from(other);
    }

    ring_buffer_queue(ring_buffer_queue&& other)
        : ptr_(other.ptr_) {
        move_from(other);
    }

    ring_buffer_queue& operator=(const ring_buffer_queue& other) {
        if (&other == this) {
            return *this;
        }

        reset();
        clone_from(other);
        return *this;
    }

    ring_buffer_queue& operator=(ring_buffer_queue&& other) {
        if (&other == this) {
            return *this;
        }

        reset();
        move_from(other);

        return *this;
    }

    // Iterators

    template<typename RB, typename VT>
    struct iterator_base {
        typedef std::random_access_iterator_tag iterator_category;
        typedef iterator_base iterator;

        iterator_base(RB* q, size_t index)
            : q_(q), i_(index) {
        }

        iterator_base(const iterator& other)
            : q_(other.q_),
              i_(other.i_) {
        }

        bool operator==(const iterator& other) const {
            return q_ == other.q_ && i_ == other.i_;
        }
        bool operator!=(const iterator& other) const {
            return q_ != other.q_ || i_ != other.i_;
        }

        iterator operator+(size_t i) const {
            return iterator(q_, i_ + i);
        }
        iterator& operator+=(size_t i) {
            i_ += i;
            return *this;
        }
        iterator& operator++() {
            return *this += 1;
        }
        iterator operator++(int) {
            iterator ret = *this;
            ++*this;
            return ret;
        }

        iterator operator-(size_t i) const {
            return iterator(q_, i_ - i);
        }
        iterator& operator-=(size_t i) {
            i_ -= i;
            return *this;
        }
        iterator& operator--() {
            return *this -= 1;
        }
        iterator operator--(int) {
            iterator ret = *this;
            --*this;
            return ret;
        }

        bool operator<(const iterator& other) const {
            if (q_ == other.q_) {
                return i_ < other.i_;
            }
            return q_ < other.q_;
        }
        bool operator>(const iterator& other) const {
            if (q_ == other.q_) {
                return i_ > other.i_;
            }
            return q_ > other.q_;
        }
        bool operator<=(const iterator& other) const {
            if (q_ == other.q_) {
                return i_ <= other.i_;
            }
            return q_ <= other.q_;
        }
        bool operator>=(const iterator& other) const {
            if (q_ == other.q_) {
                return i_ >= other.i_;
            }
            return q_ >= other.q_;
        }

        VT& operator*() {
            return (*q_)[i_];
        }

        VT* operator->() {
            return &(*q_)[i_];
        }

    private:
        RB* q_;
        ptrdiff_t i_;
    };

    typedef iterator_base<ring_buffer_queue, T> iterator;
    typedef iterator_base<const ring_buffer_queue, const T> const_iterator;

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, size());
    }

    const_iterator begin() const {
        return const_iterator(this, 0);
    }

    const_iterator end() const {
        return const_iterator(this, size());
    }

protected:
    bool full() {
        return (ptr_.read_ + capacity_ - 1 == ptr_.write_);
    }

    void overflow() {
        resize(capacity_ * 2);
    }

    void shrink() {
        if (ptr_read() == 0 && capacity_ > size() * 2) {
            shrink_to_fit();
        }
    }

    void resize(size_t new_capacity) {
        new_capacity = std::max(new_capacity, MinimumCapacity);
        if (new_capacity == capacity_) {
            return;
        }
        T* new_e = ptr_.allocate(new_capacity);
        size_t current_size = size();
        for (int i = 0; i < current_size; ++i) {
            ptr_.construct(&new_e[i],
                           std::move(e_[ptr_read(i)]));
        }
        ptr_.deallocate(e_, capacity_);
        e_ = new_e;
        capacity_ = new_capacity;
        ptr_.read_ = 0;
        ptr_.write_ = current_size;
    }

    void move_from(ring_buffer_queue& other) {
        /// XXX move
        ptr_ = other.ptr_;
        capacity_ = other.capacity_;
        e_ = other.e_;
        other.e_ = NULL;
    }

    void clone_from(const ring_buffer_queue& other) {
        ptr_ = other.ptr_;
        capacity_ = other.capacity_;
        e_ = ptr_.allocate(capacity_);
        for (size_t i = 0; i < size(); ++i) {
            ptr_.construct(&e_[ptr_read(i)],
                           other.e_[ptr_read(i)]);
        }
    }

    void reset() {
        // e_ will be NULL if this object has been the source of a move.
        if (e_ != NULL) {
            clear();
            ptr_.deallocate(e_, capacity_);
        }
    }

    void require_nonempty() const {
        if (empty()) {
            throw std::out_of_range("empty queue");
        }
    }

    uint32_t ptr_read(uint32_t offset = 0) const {
        return (ptr_.read_ + offset) & (capacity_ - 1);
    }

    uint32_t ptr_write(uint32_t offset = 0) const {
        return (ptr_.write_ + offset) & (capacity_ - 1);
    }

    T* e_;
    uint32_t capacity_;

    // A dummy struct just used for empty base class optimization.
    struct ptrs : Allocator {
        ptrs(const Allocator& alloc) : Allocator(alloc) {
        }

        uint32_t read_ = 0;
        uint32_t write_ = 0;
    } ptr_;
};

#endif // RING_BUFFER_QUEUE_H
