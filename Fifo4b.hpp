#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <new>


/// Threadsafe, efficient circular FIFO with cached cursors; constrained cursors
template<typename T, typename Alloc = std::allocator<T>>
class Fifo4b : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo4b(size_type capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , capacity_{capacity + 1}
        , ring_{allocator_traits::allocate(*this, capacity)}
    {}

    ~Fifo4b() {
        // TODO fix shouldn't matter for benchmark since it waits until
        // the fifo is empty
        // while(not empty()) {
        //     ring_[popCursor_ & mask_].~T();
        //     ++popCursor_;
        // }
        allocator_traits::deallocate(*this, ring_, capacity());
    }

    /// Returns the number of elements in the fifo
    auto size() const noexcept {
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        auto popCursor = popCursor_.load(std::memory_order_relaxed);

        assert(popCursor <= pushCursor);
        return pushCursor - popCursor;
    }

    /// Returns whether the container has no elements
    auto empty() const noexcept { return size() == 0; }

    /// Returns whether the container has capacity() elements
    auto full() const noexcept { return size() == capacity(); }

    /// Returns the number of elements that can be held in the fifo
    auto capacity() const noexcept { return capacity_ - 1; }


    /// Push one object onto the fifo.
    /// @return `true` if the operation is successful; `false` if fifo is full.
    auto push(T const& value) {
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        auto nextPushCursor = pushCursor + 1;
        if (nextPushCursor == capacity_) {
          nextPushCursor = 0;
        }
        if (nextPushCursor == popCursorCached_) {
            popCursorCached_ = popCursor_.load(std::memory_order_acquire);
            if (nextPushCursor == popCursorCached_) {
                return false;
            }
        }

        new (&ring_[pushCursor]) T(value);
        pushCursor_.store(nextPushCursor, std::memory_order_release);
        return true;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    auto pop(T& value) {
        auto popCursor = popCursor_.load(std::memory_order_relaxed);
        if (popCursor  == pushCursorCached_) {
          pushCursorCached_ = pushCursor_.load(std::memory_order_acquire);
          if (pushCursorCached_ == popCursor) {
            return false;
          }
        }

        value = ring_[popCursor];
        ring_[popCursor].~T();
        auto nextPopCursor = popCursor + 1;
        if (nextPopCursor == capacity_) {
            nextPopCursor = 0;
        }
        popCursor_.store(nextPopCursor, std::memory_order_release);
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
    size_type capacity_;
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
