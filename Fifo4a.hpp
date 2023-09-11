#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <new>


/// Threadsafe, efficient circular FIFO with cached cursors; bitwise AND vs remainder
template<typename T, typename Alloc = std::allocator<T>>
class Fifo4a : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo4a(size_type capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , mask_{capacity - 1}
        , ring_{allocator_traits::allocate(*this, capacity)}
    {}

    ~Fifo4a() {
        while(not empty()) {
            ring_[popCursor_ & mask_].~T();
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, capacity());
    }

    /// Returns the number of elements in the fifo
    auto size() const noexcept {
        auto pushCursor = pushCursor_.load(std::memory_order_acquire);
        auto popCursor = popCursor_.load(std::memory_order_relaxed);

        assert(popCursor <= pushCursor);
        return pushCursor - popCursor;
    }

    /// Returns whether the container has no elements
    auto empty() const noexcept { return size() == 0; }

    /// Returns whether the container has capacity_() elements
    auto full() const noexcept { return size() == capacity(); }

    /// Returns the number of elements that can be held in the fifo
    auto capacity() const noexcept { return mask_ + 1; }


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

        new (&ring_[pushCursor & mask_]) T(value);
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

        value = ring_[popCursor & mask_];
        ring_[popCursor & mask_].~T();
        popCursor_.store(popCursor + 1, std::memory_order_release);
        return true;
    }

private:
    auto full(size_type pushCursor, size_type popCursor) const noexcept {
        return (pushCursor - popCursor) == capacity();
    }
    static auto empty(size_type pushCursor, size_type popCursor) noexcept {
        return pushCursor == popCursor;
    }

private:
    size_type mask_;
    T* ring_;

    using CursorType = std::atomic<size_type>;
    static_assert(CursorType::is_always_lock_free);

    // See Fifo3 for reason std::hardware_destructive_interference_size is not used directly
    static constexpr auto hardware_destructive_interference_size = size_type{64};

    /// Loaded and stored by the push thread; loaded by the pop thread
    alignas(hardware_destructive_interference_size) CursorType pushCursor_;

    /// Exclusive to the push thread
    alignas(hardware_destructive_interference_size) size_type popCursorCached_{};

    /// Loaded and stored by the pop thread; loaded by the push thread
    alignas(hardware_destructive_interference_size) CursorType popCursor_;

    /// Exclusive to the pop thread
    alignas(hardware_destructive_interference_size) size_type pushCursorCached_{};

    // Padding to avoid false sharing with adjacent objects
    char padding_[hardware_destructive_interference_size - sizeof(size_type)];
};
