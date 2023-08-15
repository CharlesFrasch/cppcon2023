#pragma once

#include <atomic>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>

/// std::is_implicit_lifetime not implemented in g++-12
/// Almost correct. See [P2674R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2674r0.pdf).
template<typename T>
struct is_implicit_lifetime : std::disjunction<
                              std::is_scalar<T>,
                              std::is_array<T>,
                              std::is_aggregate<T>,
                              std::conjunction<
                              std::is_trivially_destructible<T>,
                              std::disjunction<
                              std::is_trivially_default_constructible<T>,
                              std::is_trivially_copy_constructible<T>,
                              std::is_trivially_move_constructible<T>>>> {};
template<typename T>
inline constexpr bool is_implicit_lifetime_v = is_implicit_lifetime<T>::value;


/// A trait used to optimize the number of bytes copied. Specialize this
/// on the type used to parameterize the Fifo5 to implement the
/// optimization. The general template returns `sizeof(T)`.
template<typename T>
struct ValueSizeTraits
{
    using value_type = T;
    static std::size_t size(value_type const& value) { return sizeof(value_type); }
};


/// Require implicit lifetime, add ValueSizeTraits, pusher and popper to Fifo4
template<typename T, typename Alloc = std::allocator<T>>
requires is_implicit_lifetime_v<T>
class Fifo5 : private Alloc
{
public:
    using value_type = T;
    using allocator_traits = std::allocator_traits<Alloc>;
    using size_type = typename allocator_traits::size_type;

    explicit Fifo5(size_type size, Alloc const& alloc = Alloc{})
        : Alloc{alloc}
        , size_{size}

        // allocate allocates n * sizeof(T) bytes of properly aligned but uninitialized
        // storage. Then it creates an array of type T[n] in the storage and starts its
        // lifetime, but ***does not start lifetime of any of its elements***.
        // Nowhere is "an array of type unsigned char or std::byte"
        // mentioned in the description of std::allocator<T>::allocate.
        , ring_{allocator_traits::allocate(*this, size)}
    {}

    ~Fifo5() {
        allocator_traits::deallocate(*this, ring_, size_);
    }

    auto size() const { return size_; }
    auto empty() const { return popCursor_ == pushCursor_; }
    auto full() const { return (pushCursor_ - popCursor_) == size_; }

    /// An RAII proxy object returned by push(). Allows the caller to
    /// manipulate value_type's members directly in the fifo's ring. The
    /// actual push happens when the pusher goes out of scope.
    class pusher_t
    {
    public:
        pusher_t() = default;
        explicit pusher_t(Fifo5* fifo, size_type cursor) noexcept : fifo_{fifo}, cursor_{cursor} {}

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
                fifo_->pushCursor_.store(cursor_ + 1, std::memory_order_release);
            }
        }

        /// If called the actual push operation will not be called when the
        /// pusher_t goes out of scope. Operations on the pusher_t instance
        /// after release has been called are undefined.
        void release() noexcept { fifo_ = {}; }

        /// Return whether or not the pusher_t is active.
        explicit operator bool() const noexcept { return fifo_; }

        /// In-place construct `value_type` with @a args through std::forward.
        template< typename... Args >
        void emplace(Args &&... args) noexcept;

        /// @name Direct access to the fifo's ring
        ///@{

        // QUESTION
        // These get() operations return a reference to one of the
        // elements in the T[n] array created by the allocate call
        // above. But the T's lifetime has not be started. Do I need to
        // call std::start_lifetime_as<T>(fifo->element(cursor_))?
        // If so, is it OK if get() happens to be called several times
        // on the same storage?
        auto& get() noexcept { return *fifo_->element(cursor_); }
        auto const& get() const noexcept { return *fifo_->element(cursor_); }

        value_type& operator*() noexcept { return get(); }
        value_type const& operator*() const noexcept { return get(); }

        value_type* operator->() noexcept { return &get(); }
        value_type const* operator->() const noexcept { return &get(); }
        ///@}

        /// Copy-assign a `value_type` to the pusher. Prefer to use this
        /// form rather than assigning directly to a value_type&. It takes
        /// advantage of ValueSizeTraits.
        pusher_t& operator=(value_type const& value) noexcept {
            std::memcpy(&get(), std::addressof(value), ValueSizeTraits<value_type>::size(value));
            return *this;
        }

    private:
        Fifo5* fifo_{};
        size_type cursor_;
    };
    friend class pusher_t;

    /// Optionally push one object onto a file via a pusher.
    /// @return a pointer to pusher_t.
    pusher_t push() noexcept {
        auto pushCursor = pushCursor_.load(std::memory_order_relaxed);
        if (full(pushCursor, popCursorCached_)) {
            popCursorCached_ = popCursor_.load(std::memory_order_acquire);
        }
        if (full(pushCursor, popCursorCached_)) {
            return pusher_t{};
        }
        return pusher_t(this, pushCursor);
    }

    /// Push one object onto the fifo.
    /// @return `true` if the operation is successful; `false` if fifo is full.
    auto push(T const& value) noexcept {
        auto pusher = push();
        if (pusher) {
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
        explicit popper_t(Fifo5* fifo, size_type cursor) noexcept : fifo_{fifo}, cursor_{cursor} {}

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
                fifo_->popCursor_.store(cursor_ + 1, std::memory_order_release);
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

        // QUESTION
        // If std::start_lifetime_as<T> must be called in the pusher_t
        // get() calls must it also be applied here? Or, are the
        // pusher_t calls to it and the memcpy sufficient to have an
        // actual object in the referenced t[] element?
        auto& get() noexcept { return *fifo_->element(cursor_); }
        auto const& get() const noexcept { return *fifo_->element(cursor_); }

        value_type& operator*() noexcept { return get(); }
        value_type const& operator*() const noexcept { return get(); }

        value_type* operator->() noexcept { return &get(); }
        value_type const* operator->() const noexcept { return &get(); }
        ///@}

    private:
        Fifo5* fifo_{};
        size_type cursor_;
    };
    friend popper_t;

    auto pop() noexcept {
        auto popCursor = popCursor_.load(std::memory_order_relaxed);
        if (empty(pushCursorCached_, popCursor)) {
            pushCursorCached_ = pushCursor_.load(std::memory_order_acquire);
        }
        if (empty(pushCursorCached_, popCursor)) {
            return popper_t{};
        }
        return popper_t(this, popCursor);
    };

    /// Pop one object from the fifo.
    /// @return `true` if the pop operation is successful; `false` if fifo is empty.
    auto pop(T& value) noexcept {
        auto popper = pop();
        if (popper) {
            value = *popper;
            return true;
        }
        return false;
    }

private:
    auto full(size_type pushCursor, size_type popCursor) noexcept {
        return (pushCursor - popCursor) == size_;
    }
    static auto empty(size_type pushCursor, size_type popCursor) noexcept {
        return pushCursor == popCursor;
    }

    auto* element(size_type cursor) noexcept { return &ring_[cursor % size_]; }
    auto const* element(size_type cursor) const noexcept { return &ring_[cursor % size_]; }

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
