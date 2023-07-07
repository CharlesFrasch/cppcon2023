#pragma once

#include <cstdlib>
#include <memory>


/// Non-threadsafe circular FIFO
template<typename T>
class Fifo1
{
public:
    using ValueType = T;

    explicit Fifo1(std::size_t size)
        : size_{size}
        , ring_{static_cast<ValueType*>(std::aligned_alloc(alignof(T), size * sizeof(T))), &std::free}
    {}

    ~Fifo1() {
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

    using RingType = std::unique_ptr<ValueType[], decltype(&std::free)>;
    RingType ring_;

    std::size_t pushCursor_;
    std::size_t popCursor_;
};
