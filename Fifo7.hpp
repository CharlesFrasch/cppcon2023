#pragma once

#include <cstdlib>
#include <functional>
#include <memory>


/// adds allocator
// TODO make same as others esp thread safety
template<typename T, typename Allocator = std::allocator<T>>
class Fifo7 : private Allocator
{
public:
    using ValueType = T;
    using allocator_traits = std::allocator_traits<Allocator>;

    using RingType = std::unique_ptr<ValueType[], std::function<void(ValueType*)>>;

    explicit Fifo7(std::size_t size, const Allocator& alloc = Allocator())
        : Allocator(alloc)
        , size_{size}
        , ring_{
              allocator_traits::allocate(*this, size_),
              [this](ValueType* ring) {
                  allocator_traits::deallocate(*this, ring, size_);
              }}
    {}

    ~Fifo7() {
        while(not empty()) {
            ring_[popCursor_ % size_].~T();
            ++popCursor_;
        }
    }

    std::size_t size() const { return size_; }
    bool empty() const { return popCursor_ == pushCursor_; }
    bool full() const { return (pushCursor_ - popCursor_) == size_; }

    bool push(T const& value) {
        if (full()) {
            return false;
        }
        new (&ring_[pushCursor_ % size_]) T(value);
        ++pushCursor_;
        return true;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo was empty.
    bool pop(T& value) {
        if (empty()) {
            return false;
        }
        value = ring_[popCursor_ % size_];
        ring_[popCursor_ % size_].~T();
        ++popCursor_;
        return true;
    }

private:
    std::size_t size_;
    RingType ring_;
    std::size_t pushCursor_;
    std::size_t popCursor_;
};
