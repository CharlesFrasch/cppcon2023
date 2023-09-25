#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <new>


/// Threadsafe, efficient circular FIFO
template<typename T, typename Alloc = std::allocator<T>>
class Fifo3 : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo3(size_type capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , capacity_{capacity}
        , ring_{allocator_traits::allocate(*this, capacity)}
    {}

    ~Fifo3() {
        while(not empty()) {
            element(popCursor_)->~T();
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, capacity_);
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

    /// Returns whether the container has capacity_() elements
    auto full() const noexcept { return size() == capacity(); }

    /// Returns the number of elements that can be held in the fifo
    auto capacity() const noexcept { return capacity_; }


    /// Push one object onto the fifo.
    /// @return `true` if the operation is successful; `false` if fifo is full.
    auto push(T const& value) {
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        auto popCursor = popCursor_.load(std::memory_order_acquire);
        if (full(pushCursor, popCursor)) {
            return false;
        }
        new (element(pushCursor)) T(value);
        pushCursor_.store(pushCursor + 1, std::memory_order_release);
        return true;
    }

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    auto pop(T& value) {
        auto pushCursor = pushCursor_.load(std::memory_order_acquire);
        auto popCursor = popCursor_.load(std::memory_order_relaxed);
        if (empty(pushCursor, popCursor)) {
            return false;
        }
        value = *element(popCursor);
        element(popCursor)->~T();
        popCursor_.store(popCursor + 1, std::memory_order_release);
        return true;
    }

private:
    auto full(size_type pushCursor, size_type popCursor) const noexcept {
        return (pushCursor - popCursor) == capacity_;
    }
    static auto empty(size_type pushCursor, size_type popCursor) noexcept {
        return pushCursor == popCursor;
    }
    auto element(size_type cursor) noexcept {
        return &ring_[cursor % capacity_];
    }

private:
    size_type capacity_;
    T* ring_;

    using CursorType = std::atomic<size_type>;
    static_assert(CursorType::is_always_lock_free);

    // N.B. std::hardware_destructive_interference_size is not used directly
    // error: use of ‘std::hardware_destructive_interference_size’ [-Werror=interference-size]
    // note: its value can vary between compiler versions or with different ‘-mtune’ or ‘-mcpu’ flags
    // note: if this use is part of a public ABI, change it to instead use a constant variable you define
    // note: the default value for the current CPU tuning is 64 bytes
    // note: you can stabilize this value with ‘--param hardware_destructive_interference_size=64’, or disable this warning with ‘-Wno-interference-size’
    static constexpr auto hardware_destructive_interference_size = size_type{64};

    /// Loaded and stored by the push thread; loaded by the pop thread
    alignas(hardware_destructive_interference_size) CursorType pushCursor_;

    /// Loaded and stored by the pop thread; loaded by the push thread
    alignas(hardware_destructive_interference_size) CursorType popCursor_;

    // Padding to avoid false sharing with adjacent objects
    char padding_[hardware_destructive_interference_size - sizeof(size_type)];
};
