#pragma once

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

    explicit Fifo1(size_type size, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , size_{size}
        , ring_{allocator_traits::allocate(*this, size)}
    {}

    // For consistency with other fifos
    Fifo1(Fifo1 const&) = delete;
    Fifo1& operator=(Fifo1 const&) = delete;
    Fifo1(Fifo1&&) = delete;
    Fifo1& operator=(Fifo1&&) = delete;

    ~Fifo1() {
        while(not empty()) {
            ring_[popCursor_ % size_].~T();
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, size_);
    }

    auto size() const { return size_; }
    auto empty() const { return popCursor_ == pushCursor_; }
    auto full() const { return (pushCursor_ - popCursor_) == size_; }

    /// Push one object onto the fifo.
    /// @return `true` if the operation is successful; `false` if fifo is full.
    auto push(T const& value) {
        if (full()) {
            return false;
        }
        new (&ring_[pushCursor_ % size_]) T(value);
        ++pushCursor_;
        return true;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    auto pop(T& value) {
        if (empty()) {
            return false;
        }
        value = ring_[popCursor_ % size_];
        ring_[popCursor_ % size_].~T();
        ++popCursor_;
        return true;
    }

private:
    size_type size_;
    T* ring_;
    size_type pushCursor_;
    size_type popCursor_;
};
