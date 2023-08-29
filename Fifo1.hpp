#pragma once

#include <cassert>
#include <cstdlib>
#include <memory>


/// Non-threadsafe circular FIFO; has data races
template<typename T, typename Alloc = std::allocator<T>>
class Fifo1 : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo1(size_type capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , capacity_{capacity}
        , ring_{allocator_traits::allocate(*this, capacity)}
    {}

    // For consistency with other fifos
    Fifo1(Fifo1 const&) = delete;
    Fifo1& operator=(Fifo1 const&) = delete;
    Fifo1(Fifo1&&) = delete;
    Fifo1& operator=(Fifo1&&) = delete;

    ~Fifo1() {
        while(not empty()) {
            ring_[popCursor_ % capacity_].~T();
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, capacity_);
    }


    /// Returns the number of elements in the fifo
    auto size() const noexcept {
        assert(popCursor_ <= pushCursor_);
        return pushCursor_ - popCursor_;
    }

    /// Returns whether the container has no elements
    auto empty() const noexcept { return size() == 0; }

    /// Returns whether the container has capacity_() elements
    auto full() const noexcept { return size() == capacity(); }

    /// Returns the number of elements that can be held in the fifo
    auto capacity() const noexcept { return capacity_; }


    /// Push one object onto the fifo.
    /// @return `true` if the operation is successful; `false` if fifo is full.
    auto push(T const& value) {
        if (full()) {
            return false;
        }
        new (&ring_[pushCursor_ % capacity_]) T(value);
        ++pushCursor_;
        return true;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    auto pop(T& value) {
        if (empty()) {
            return false;
        }
        value = ring_[popCursor_ % capacity_];
        ring_[popCursor_ % capacity_].~T();
        ++popCursor_;
        return true;
    }

private:
    size_type capacity_;
    T* ring_;
    size_type pushCursor_;
    size_type popCursor_;
};
