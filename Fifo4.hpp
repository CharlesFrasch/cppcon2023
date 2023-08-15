#pragma once

#include <atomic>
#include <memory>
#include <new>


/// Threadsafe, efficient circular FIFO with cached cursors
template<typename T, typename Alloc = std::allocator<T>>
class Fifo4 : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo4(size_type size, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , size_{size}
        , ring_{allocator_traits::allocate(*this, size_)}
    {}

    ~Fifo4() {
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
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        if (full(pushCursor, popCursorCached_)) {
            popCursorCached_ = popCursor_.load(std::memory_order_acquire);
        }
        if (full(pushCursor, popCursorCached_)) {
            return false;
        }

        new (&ring_[pushCursor % size_]) T(value);
        pushCursor_.store(pushCursor + 1, std::memory_order_release);
        return true;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    auto pop(T& value) {
        auto popCursor = popCursor_.load(std::memory_order_relaxed);
        if (empty(pushCursorCached_, popCursor)) {
            pushCursorCached_ = pushCursor_.load(std::memory_order_acquire);
        }
        if (empty(pushCursorCached_, popCursor)) {
            return false;
        }

        value = ring_[popCursor % size_];
        ring_[popCursor % size_].~T();
        popCursor_.store(popCursor + 1, std::memory_order_release);
        return true;
    }

private:
    auto full(size_type pushCursor, size_type popCursor) const {
        return (pushCursor - popCursor) == size_;
    }
    static auto empty(size_type pushCursor, size_type popCursor) {
        return pushCursor == popCursor;
    }

private:
    size_type size_;
    T* ring_;

    using CursorType = std::atomic<size_type>;
    static_assert(CursorType::is_always_lock_free);

    // https://stackoverflow.com/questions/39680206/understanding-stdhardware-destructive-interference-size-and-stdhardware-cons
    static constexpr auto hardware_destructive_interference_size = size_type{128};

    /// Loaded and stored by the push thread; loaded by the pop thread
    alignas(hardware_destructive_interference_size) CursorType pushCursor_;

    /// Exclusive to the push thread
    alignas(hardware_destructive_interference_size) size_type popCursorCached_{};

    /// Loaded and stored by the pop thread; loaded by the push thread
    alignas(hardware_destructive_interference_size) CursorType popCursor_;

    /// Exclusive to the pop thread
    alignas(hardware_destructive_interference_size) size_type pushCursorCached_{};
};
