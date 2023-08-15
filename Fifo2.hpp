#pragma once

#include <atomic>
#include <memory>


/// Threadsafe but flawed circular FIFO
template<typename T, typename Alloc = std::allocator<T>>
class Fifo2 : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo2(size_type size, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , size_{size}
        , ring_{allocator_traits::allocate(*this, size)}
    {}

    ~Fifo2() {
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

    using CursorType = std::atomic<size_type>;
    static_assert(CursorType::is_always_lock_free);

    /// Loaded and stored by the push thread; loaded by the pop thread
    CursorType pushCursor_;

    /// Loaded and stored by the pop thread; loaded by the push thread
    CursorType popCursor_;
};
