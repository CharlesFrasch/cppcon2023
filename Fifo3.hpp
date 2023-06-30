#pragma once

#include <atomic>
#include <cstdlib>
#include <memory>
#include <new>


/// Threadsafe, efficient circular FIFO
template<typename T>
class Fifo3
{
public:
    using ValueType = T;

    explicit Fifo3(std::size_t size)
        : size_{size}
        , ring_{
              static_cast<ValueType*>(std::aligned_alloc(alignof(T), size * sizeof(T))),
              &std::free}
    {}


    std::size_t size() const { return size_; }
    bool empty() const { return popCursor_ == pushCursor_; }
    bool full() const { return (pushCursor_ - popCursor_) == size_; }

    bool push(T const& value) {
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        auto popCursor = popCursor_.load(std::memory_order_acquire);
        if (full(pushCursor, popCursor)) {
            return false;
        }
        new (&ring_[pushCursor % size_]) T(value);
        pushCursor_.store(pushCursor + 1, std::memory_order_release);
        return true;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo was empty.
    bool pop(T& value) {
        auto pushCursor = pushCursor_.load(std::memory_order_acquire);
        auto popCursor = popCursor_.load(std::memory_order_relaxed);
        if (empty(pushCursor, popCursor)) {
            return false;
        }
        value = ring_[popCursor % size_];
        ring_[popCursor % size_].~T();
        popCursor_.store(popCursor + 1, std::memory_order_release);
        return true;
    }

private:
    bool full(std::size_t pushCursor, std::size_t popCursor) const {
        return (pushCursor - popCursor) == size_;
    }
    static bool empty(std::size_t pushCursor, std::size_t popCursor) {
        return pushCursor == popCursor;
    }

private:
    std::size_t size_;

    using RingType = std::unique_ptr<ValueType[], decltype(&std::free)>;
    RingType ring_;

    // https://stackoverflow.com/questions/39680206/understanding-stdhardware-destructive-interference-size-and-stdhardware-cons
    static constexpr auto hardware_destructive_interference_size = std::size_t{128};

    /// Loaded and stored by the push thread; loaded by the pop thread
    alignas(hardware_destructive_interference_size) std::atomic<std::size_t> pushCursor_{};

    /// Loaded and stored by the pop thread; loaded by the push thread
    alignas(hardware_destructive_interference_size) std::atomic<std::size_t> popCursor_{};
};
