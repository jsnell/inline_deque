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
         size_t InlineCapacity = 1,
         size_t InitialCapacity = InlineCapacity,
         typename CapacityType = uint32_t,
         class Allocator = std::allocator<T>>
class ring_buffer_queue {
public:
    ring_buffer_queue(size_t initial_capacity = InitialCapacity,
                      const Allocator& alloc = Allocator())
        : capacity_(initial_capacity),
          ptr_(alloc) {
        if (initial_capacity > InlineCapacity) {
            capacity_ = 1;
            while (capacity_ < initial_capacity) {
                capacity_ *= 2;
            }
            e_.e_ = ptr_.allocate(capacity_);
        } else {
            capacity_ = InlineCapacity;
        }
    }

    ~ring_buffer_queue() {
        reset();
    }

    // Adding new elements at back of queue.

    void push_back(const T& e) {
        if (full()) {
            overflow();
        }
        ptr_.construct(&slot(ptr_write()), e);
        ptr_.write_++;
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (full()) {
            overflow();
        }
        ptr_.construct(&slot(ptr_write()),
                       std::forward<Args>(args)...);
        ptr_.write_++;
    }

    // Accessing items (front, back, random access, pop).

    const T& front() const {
        require_nonempty();
        return slot(ptr_read());
    }

    const T& back() const {
        require_nonempty();
        return slot(ptr_write(-1));
    }

    T& front() {
        require_nonempty();
        return slot(ptr_read());
    }

    T& back() {
        require_nonempty();
        return slot(ptr_write(-1));
    }

    T& operator[] (size_t i) {
        return slot(ptr_read(i));
    }

    const T& operator[] (size_t i) const {
        return slow(ptr_read(i));
    }

    T& at(size_t i) {
        if (i >= size()) {
            throw std::out_of_range("index too large");
        }
        return slot(ptr_read(i));
    }

    void pop_front() {
        require_nonempty();
        ptr_.destroy(&slot(ptr_read()));
        ptr_.read_++;
        shrink();
    }

    void pop_back() {
        require_nonempty();
        ptr_.write_--;
        ptr_.destroy(&slot(ptr_write()));
        shrink();
    }

    // Size of queue

    bool empty() const {
        return size() == 0;
    }

    CapacityType size() const {
        return ptr_.write_ - ptr_.read_;
    }

    CapacityType capacity() const {
        return capacity_;
    }

    CapacityType max_capacity() const {
        return std::numeric_limits<CapacityType>::max() >> 1;
    }

    void clear() {
        while (!empty()) {
            pop_front();
        }
    }

    void shrink_to_fit() {
        CapacityType new_capacity = capacity_;
        while (new_capacity &&
               new_capacity > size() * 2) {
            new_capacity /= 2;
        }
        if (new_capacity < capacity_) {
            resize(new_capacity);
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

    // TODO: swap? assign?

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

    // Misc

    Allocator get_allocator() const {
        return ptr_;
    }

protected:
    bool full() {
        return size() == capacity();
    }

    void overflow() {
        resize(capacity_ * 2);
    }

    void shrink() {
        if (ptr_read() == 0 && capacity_ > size() * 2) {
            shrink_to_fit();
        }
    }

    bool use_inline() const {
        return capacity_ == InlineCapacity;
    }

    void resize(CapacityType new_capacity) {
        new_capacity = std::max(new_capacity,
                                static_cast<CapacityType>(InlineCapacity));

        if (new_capacity == capacity_) {
            return;
        }

        T* old_e = (use_inline() ? (T*) &e_.inline_e_ : e_.e_);
        T* new_e;

        if (new_capacity == InlineCapacity) {
            new_e = (T*) &e_.inline_e_;
        } else {
            new_e = ptr_.allocate(new_capacity);
        }

        size_t current_size = size();
        for (int i = 0; i < current_size; ++i) {
            // Note: we have to use slot_impl() with a precomputed array
            // pointer instead of slot() here. The reason is that if the
            // new array is inline-allocated, writes to it will clobber
            // clobber e_.e_.
            ptr_.construct(&new_e[i],
                           std::move(slot_impl(ptr_read(i), old_e)));
        }

        if (!use_inline()) {
            ptr_.deallocate(old_e, capacity_);
        }

        capacity_ = new_capacity;

        if (!use_inline()) {
            e_.e_ = new_e;
        }

        ptr_.read_ = 0;
        ptr_.write_ = current_size;
    }

    void move_from(ring_buffer_queue& other) {
        /// XXX move
        ptr_ = other.ptr_;
        capacity_ = other.capacity_;
        if (use_inline()) {
            ptr_.construct((T*) &e_.inline_e_,
                           std::move((T&) other.e_.inline_e_));
        } else {
            e_.e_ = other.e_.e_;
        }
        other.e_.e_ = NULL;
        other.capacity_ = 0;
    }

    void clone_from(const ring_buffer_queue& other) {
        ptr_ = other.ptr_;
        capacity_ = other.capacity_;
        if (!use_inline()) {
            e_.e_ = ptr_.allocate(capacity_);
        }
        for (size_t i = 0; i < size(); ++i) {
            ptr_.construct(&slot(ptr_read(i)),
                           other.slot(ptr_read(i)));
        }
    }

    void reset() {
        // capacity_ will be 0 iff this was the source of a move
        if (capacity_ != 0) {
            clear();
            if (!use_inline()) {
                ptr_.deallocate(e_.e_, capacity_);
            }
        }
    }

    void require_nonempty() const {
        if (empty()) {
            throw std::out_of_range("empty queue");
        }
    }

    CapacityType ptr_read(CapacityType offset = 0) const {
        return (ptr_.read_ + offset);
    }

    CapacityType ptr_write(CapacityType offset = 0) const {
        return (ptr_.write_ + offset);
    }

    T& slot(CapacityType index) {
        if (use_inline()) {
            return slot_impl(index, (T*) e_.inline_e_);
        } else {
            return slot_impl(index, e_.e_);
        }
    }

    const T& slot(CapacityType index) const {
        if (use_inline()) {
            return slot_impl(index, (T*) e_.inline_e_);
        } else {
            return slot_impl(index, e_.e_);
        }
    }

    T& slot_impl(CapacityType index, T* array) {
        CapacityType actual_index = index & (capacity_ - 1);
        return array[actual_index];
    }

    const T& slot_impl(CapacityType index, T* array) const {
        CapacityType actual_index = index & (capacity_ - 1);
        return array[actual_index];
    }

    union {
        T* e_;
        uint8_t inline_e_[sizeof(T) * InlineCapacity];
    } e_;
    CapacityType capacity_;

    // A dummy struct just used for empty base class optimization.
    struct ptrs : Allocator {
        ptrs(const Allocator& alloc) : Allocator(alloc) {
        }

        CapacityType read_ = 0;
        CapacityType write_ = 0;
    } ptr_;
};

#endif // RING_BUFFER_QUEUE_H
