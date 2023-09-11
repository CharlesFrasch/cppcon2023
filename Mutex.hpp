#pragma once

#include <cstdlib>
#include <memory>
#include <mutex>


/// Thread-safe, mutex-based FIFO
template<typename T, typename Alloc = std::allocator<T>>
class Mutex : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Mutex(size_type capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , capacity_{capacity}
        , ring_{allocator_traits::allocate(*this, capacity)}
    {}

    ~Mutex() {
        while(not empty()) {
            ring_[popCursor_ % capacity_].~T();
            ++popCursor_;
        }
        allocator_traits::deallocate(*this, ring_, capacity_);
    }

    auto capacity() const noexcept { return capacity_; }
    auto size() const noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        return size(pushCursor_, popCursor_);
    }
    auto empty() const noexcept { return size() == 0; }
    auto full() const noexcept { return size() == capacity(); }

    auto push(T const& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (full(pushCursor_, popCursor_)) {
            return false;
        }
        new (&ring_[pushCursor_ % capacity_]) T(value);
        ++pushCursor_;
        return true;
    }

    auto pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (empty(pushCursor_, popCursor_)) {
            return false;
        }
        value = ring_[popCursor_ % capacity_];
        ring_[popCursor_ % capacity_].~T();
        ++popCursor_;
        return true;
    }

private:
    static auto size (size_type pushCursor, size_type popCursor) noexcept {
        return pushCursor - popCursor;
    }

    auto full(size_type pushCursor, size_type popCursor) const noexcept {
        return (pushCursor - popCursor) == capacity_;
    }
    static auto empty(size_type pushCursor, size_type popCursor) noexcept {
        return pushCursor == popCursor;
    }

private:
    size_type capacity_;
    T* ring_;
    mutable std::mutex mutex_;
    size_type pushCursor_{};
    size_type popCursor_{};
};
