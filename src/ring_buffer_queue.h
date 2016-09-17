// -*- mode: c++; c-basic-offset: 4 indent-tabs-mode: nil -*- */
//
// Copyright 2016 Juho Snellman, released under a MIT license (see
// LICENSE).

#ifndef RING_BUFFER_QUEUE_H
#define RING_BUFFER_QUEUE_H

#include <stdexcept>

template<typename T,
         size_t InitialCapacity = 8,
         size_t MinimumCapacity = 8,
         class Allocator = std::allocator<T>>
class ring_buffer_queue {
public:
    ring_buffer_queue(size_t initial_capacity = InitialCapacity,
                      const Allocator& alloc = Allocator())
        : capacity_(initial_capacity),
          alloc_(alloc) {
        capacity_ = 1;
        while (capacity_ < initial_capacity) {
            capacity_ *= 2;
        }
        e_ = alloc_.allocate(capacity_);
    }

    ~ring_buffer_queue() {
        reset();
    }

    // Adding new elements at back of queue.

    void push_back(const T& e) {
        if (read_ptr_ == index(write_ptr_ + 1)) {
            overflow();
        }
        alloc_.construct(&e_[write_ptr_], e);
        write_ptr_ = index(write_ptr_ + 1);
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        if (read_ptr_ == index(write_ptr_ + 1)) {
            overflow();
        }
        alloc_.construct(&e_[write_ptr_], std::forward<Args>(args)...);
        write_ptr_ = index(write_ptr_ + 1);
    }

    // Accessing items (front, back, random access, pop).

    const T& front() const {
        require_nonempty();
        return e_[read_ptr_];
    }

    const T& back() const {
        require_nonempty();
        return e_[index(write_ptr_ - 1)];
    }

    T& operator[] (size_t i) {
        return e_[index(read_ptr_ + i)];
    }

    const T& operator[] (size_t i) const {
        return e_[index(read_ptr_ + i)];
    }

    T& at(size_t i) {
        if (i >= size()) {
            throw std::out_of_range("index too large");
        }
        return e_[index(read_ptr_ + i)];
    }

    void pop_front() {
        require_nonempty();
        alloc_.destroy(&e_[read_ptr_]);
        read_ptr_ = index(read_ptr_ + 1);
        shrink();
    }

    void pop_back() {
        require_nonempty();
        write_ptr_ = index(write_ptr_ - 1);
        alloc_.destroy(&e_[write_ptr_]);
        shrink();
    }

    // Size of queue

    bool empty() const {
        return read_ptr_ == write_ptr_;
    }

    size_t size() const {
        if (write_ptr_ < read_ptr_) {
            return capacity_ - (read_ptr_ - write_ptr_);
        } else {
            return write_ptr_ - read_ptr_;
        }
    }

    size_t capacity() const {
        return capacity_;
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
        : alloc_(other.alloc_) {
        clone_from(other);
    }

    ring_buffer_queue(ring_buffer_queue&& other)
        : alloc_(other.alloc_) {
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
    size_t index(size_t ptr) const {
        return ptr & (capacity_ - 1);
    }

    void overflow() {
        resize(capacity_ * 2);
    }

    void shrink() {
        if (read_ptr_ == 0 && capacity_ > size() * 2) {
            shrink_to_fit();
        }
    }

    void resize(size_t new_capacity) {
        new_capacity = std::max(new_capacity, MinimumCapacity);
        if (new_capacity == capacity_) {
            return;
        }
        T* new_e = alloc_.allocate(new_capacity);
        size_t current_size = size();
        for (int i = 0; i < current_size; ++i) {
            alloc_.construct(&new_e[i],
                                  std::move(e_[index(read_ptr_ + i)]));
        }
        alloc_.deallocate(e_, capacity_);
        e_ = new_e;
        capacity_ = new_capacity;
        read_ptr_ = 0;
        write_ptr_ = current_size;
    }

    void move_from(ring_buffer_queue& other) {
        read_ptr_ = other.read_ptr_;
        write_ptr_ = other.write_ptr_;
        capacity_ = other.capacity_;
        e_ = other.e_;
        other.e_ = NULL;
    }

    void clone_from(const ring_buffer_queue& other) {
        read_ptr_ = other.read_ptr_;
        write_ptr_ = other.write_ptr_;
        capacity_ = other.capacity_;
        e_ = alloc_.allocate(capacity_);
        for (size_t i = 0; i < size(); ++i) {
            alloc_.construct(&e_[index(read_ptr_ + i)],
                                  other.e_[index(read_ptr_ + i)]);
        }
    }

    void reset() {
        // e_ will be NULL if this object has been the source of a move.
        if (e_ != NULL) {
            clear();
            alloc_.deallocate(e_, capacity_);
        }
    }

    void require_nonempty() const {
        if (read_ptr_ == write_ptr_) {
            throw std::out_of_range("empty queue");
        }
    }

    T* e_;
    size_t capacity_;
    uint32_t read_ptr_ = 0;
    uint32_t write_ptr_ = 0;
    Allocator alloc_;
};

#endif // RING_BUFFER_QUEUE_H
