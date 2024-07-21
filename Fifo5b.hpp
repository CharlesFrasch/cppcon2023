#pragma once

#include <atomic>
#include <cassert>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>

// For ValueSizeTraits
#include "Fifo5.hpp"


/// Like Fifo5a except uses atomic_ref
template<typename T, typename Alloc = std::allocator<T>>
    requires std::is_trivial_v<T>
class Fifo5b : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo5b(size_type capacity, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , mask_{capacity - 1}
        , ring_{allocator_traits::allocate(*this, capacity)} {
        assert((capacity & mask_) == 0);
    }

    ~Fifo5b() {
        allocator_traits::deallocate(*this, ring_, capacity());
    }


    /// Returns the number of elements in the fifo
    auto size() const noexcept {
        auto pushCursor = pushCursorRef_.load(std::memory_order_relaxed);
        auto popCursor = popCursorRef_.load(std::memory_order_relaxed);

        assert(popCursor <= pushCursor);
        return pushCursor - popCursor;
    }

    /// Returns whether the container has no elements
    auto empty() const noexcept { return size() == 0; }

    /// Returns whether the container has capacity_() elements
    auto full() const noexcept { return size() == capacity(); }

    /// Returns the number of elements that can be held in the fifo
    auto capacity() const noexcept { return mask_ + 1; }


    /// An RAII proxy object returned by push(). Allows the caller to
    /// manipulate value_type's members directly in the fifo's ring. The
    /// actual push happens when the pusher goes out of scope.
    class pusher_t
    {
    public:
        pusher_t() = default;
        explicit pusher_t(Fifo5b* fifo, size_type cursor) noexcept : fifo_{fifo}, cursor_{cursor} {}

        pusher_t(pusher_t const&) = delete;
        pusher_t& operator=(pusher_t const&) = delete;

        pusher_t(pusher_t&& other) noexcept
            : fifo_{std::move(other.fifo_)}
            , cursor_{std::move(other.cursor_)} {
            other.release();
        }
        pusher_t& operator=(pusher_t&& other) noexcept {
            fifo_ = std::move(other.fifo_);
            cursor_ = std::move(other.cursor_);
            other.release();
            return *this;
        }

        ~pusher_t() {
            if (fifo_) {
                fifo_->pushCursorRef_.store(cursor_ + 1, std::memory_order_release);
            }
        }

        /// If called the actual push operation will not be called when the
        /// pusher_t goes out of scope. Operations on the pusher_t instance
        /// after release has been called are undefined.
        void release() noexcept { fifo_ = {}; }

        /// Return whether or not the pusher_t is active.
        explicit operator bool() const noexcept { return fifo_; }

        /// @name Direct access to the fifo's ring
        ///@{
        value_type* get() noexcept { return fifo_->element(cursor_); }
        value_type const* get() const noexcept { return fifo_->element(cursor_); }

        value_type& operator*() noexcept { return *get(); }
        value_type const& operator*() const noexcept { return *get(); }

        value_type* operator->() noexcept { return get(); }
        value_type const* operator->() const noexcept { return get(); }
        ///@}

        /// Copy-assign a `value_type` to the pusher. Prefer to use this
        /// form rather than assigning directly to a value_type&. It takes
        /// advantage of ValueSizeTraits.
        pusher_t& operator=(value_type const& value) noexcept {
            std::memcpy(get(), std::addressof(value), ValueSizeTraits<value_type>::size(value));
            return *this;
        }

    private:
        Fifo5b* fifo_{};
        size_type cursor_;
    };
    friend class pusher_t;

    /// Optionally push one object onto a file via a pusher.
    /// @return a pointer to pusher_t.
    pusher_t push() noexcept {
        auto pushCursor = pushCursor_;
        if (full(pushCursor, popCursorCached_)) {
            // popCursorCached_ = popCursor_.load(std::memory_order_acquire);
            popCursorCached_ = popCursorRef_.load(std::memory_order_acquire);
            if (full(pushCursor, popCursorCached_)) {
                return pusher_t{};
            }
        }
        return pusher_t(this, pushCursor);
    }

    /// Push one object onto the fifo.
    /// @return `true` if the operation is successful; `false` if fifo is full.
    auto push(T const& value) noexcept {
        if (auto pusher = push(); pusher) {
            pusher = value;
            return true;
        }
        return false;
    }

    /// An RAII proxy object returned by pop(). Allows the caller to
    /// manipulate value_type members directly in the fifo's ring. The
    // /actual pop happens when the popper goes out of scope.
    class popper_t
    {
    public:
        popper_t() = default;
        explicit popper_t(Fifo5b* fifo, size_type cursor) noexcept : fifo_{fifo}, cursor_{cursor} {}

        popper_t(popper_t const&) = delete;
        popper_t& operator=(popper_t const&) = delete;

        popper_t(popper_t&& other) noexcept
            : fifo_{std::move(other.fifo_)}
            , cursor_{std::move(other.cursor_)} {
            other.release();
        }
        popper_t& operator=(popper_t&& other) noexcept {
            fifo_ = std::move(other.fifo_);
            cursor_ = std::move(other.cursor_);
            other.release();
            return *this;
        }

        ~popper_t() {
            if (fifo_) {
                fifo_->popCursorRef_.store(cursor_ + 1, std::memory_order_release);
            }
        }

        /// If called the actual pop operation will not be called when the
        /// popper_t goes out of scope. Operations on the popper_t instance
        /// after release has been called are undefined.
        void release() noexcept { fifo_ = {}; }

        /// Return whether or not the popper_t is active.
        explicit operator bool() const noexcept { return fifo_; }

        /// @name Direct access to the fifo's ring
        ///@{
        value_type* get() noexcept { return fifo_->element(cursor_); }
        value_type const* get() const noexcept { return fifo_->element(cursor_); }

        value_type& operator*() noexcept { return *get(); }
        value_type const& operator*() const noexcept { return *get(); }

        value_type* operator->() noexcept { return get(); }
        value_type const* operator->() const noexcept { return get(); }
        ///@}

    private:
        Fifo5b* fifo_{};
        size_type cursor_;
    };
    friend popper_t;

    auto pop() noexcept {
        auto popCursor = popCursor_;
        if (empty(pushCursorCached_, popCursor)) {
            pushCursorCached_ = pushCursorRef_.load(std::memory_order_acquire);
            if (empty(pushCursorCached_, popCursor)) {
                return popper_t{};
            }
        }
        return popper_t(this, popCursor);
    };

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    auto pop(T& value) noexcept {
        if (auto popper = pop(); popper) {
            value = *popper;
            return true;
        }
        return false;
    }

private:
    auto full(size_type pushCursor, size_type popCursor) const noexcept {
        assert(popCursor <= pushCursor);
        return (pushCursor - popCursor) == capacity();
    }
    static auto empty(size_type pushCursor, size_type popCursor) noexcept {
        return pushCursor == popCursor;
    }

    auto* element(size_type cursor) noexcept { return &ring_[cursor & mask_]; }
    auto const* element(size_type cursor) const noexcept { return &ring_[cursor & mask_]; }

private:
    size_type mask_;
    T* ring_;

    using CursorType = size_type;
    using CursorRefType = std::atomic_ref<CursorType>;

    // https://stackoverflow.com/questions/39680206/understanding-stdhardware-destructive-interference-size-and-stdhardware-cons
    // See Fifo3.hpp for reason why std::hardware_destructive_interference_size is not used directly
    static constexpr auto hardware_destructive_interference_size = size_type{64};

    /// Loaded and stored by the push thread; loaded by the pop thread
    alignas(hardware_destructive_interference_size) CursorType pushCursor_{};
    CursorRefType pushCursorRef_{pushCursor_};

    /// Exclusive to the push thread
    alignas(hardware_destructive_interference_size) size_type popCursorCached_{};

    /// Loaded and stored by the pop thread; loaded by the push thread
    alignas(hardware_destructive_interference_size) CursorType popCursor_{};
    CursorRefType popCursorRef_{popCursor_};

    /// Exclusive to the pop thread
    alignas(hardware_destructive_interference_size) size_type pushCursorCached_{};

    // Padding to avoid false sharing with adjacent objects
    char padding_[hardware_destructive_interference_size - sizeof(size_type)];
};
