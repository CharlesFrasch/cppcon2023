#include "Fifo1.hpp"
#include "Fifo2.hpp"
#include "Fifo3.hpp"
#include "Fifo4.hpp"
#include "Fifo5.hpp"

#include <gtest/gtest.h>

#include <type_traits>


extern "C" {
    void __ubsan_on_report() {
          FAIL() << "Encountered an undefined behavior sanitizer error";
    }
    void __asan_on_error() {
          FAIL() << "Encountered an address sanitizer error";
    }
    void __tsan_on_report() {
          FAIL() << "Encountered a thread sanitizer error";
    }
}  // extern "C"


template<typename FifoT>
class FifoTestBase : public testing::Test {
public:
    using FifoType = FifoT;
    using value_type = typename FifoType::value_type;

    FifoType fifo{4};
};

using test_type = unsigned int;


template<typename FifoT> using FifoTest = FifoTestBase<FifoT>;
using FifoTypes = ::testing::Types<
    Fifo1<test_type>,
    Fifo2<test_type>,
    Fifo3<test_type>,
    Fifo4<test_type>,
    Fifo5<test_type>
    >;
TYPED_TEST_SUITE(FifoTest, FifoTypes);

TYPED_TEST(FifoTest, properties) {
    EXPECT_FALSE(std::is_default_constructible_v<typename TestFixture::FifoType>);
    EXPECT_TRUE((std::is_constructible_v<typename TestFixture::FifoType, unsigned long>));
    EXPECT_TRUE((std::is_constructible_v<typename TestFixture::FifoType, unsigned long, std::allocator<typename TestFixture::value_type>>));
    EXPECT_FALSE(std::is_copy_constructible_v<typename TestFixture::FifoType>);
    EXPECT_FALSE(std::is_move_constructible_v<typename TestFixture::FifoType>);
    EXPECT_FALSE(std::is_copy_assignable_v<typename TestFixture::FifoType>);
    EXPECT_FALSE(std::is_move_assignable_v<typename TestFixture::FifoType>);
    EXPECT_TRUE(std::is_destructible_v<typename TestFixture::FifoType>);
}

TYPED_TEST(FifoTest, initialConditions) {
    EXPECT_EQ(4u, this->fifo.capacity());
    EXPECT_EQ(0, this->fifo.size());
    EXPECT_TRUE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());
}

TYPED_TEST(FifoTest, push) {
    ASSERT_EQ(4u, this->fifo.capacity());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_EQ(1u, this->fifo.size());
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_EQ(2u, this->fifo.size());
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_EQ(3u, this->fifo.size());
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_EQ(4u, this->fifo.size());
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_TRUE(this->fifo.full());

    EXPECT_FALSE(this->fifo.push(42));
    EXPECT_EQ(4u, this->fifo.size());
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_TRUE(this->fifo.full());
}

TYPED_TEST(FifoTest, pop) {
    auto value = typename TestFixture::value_type{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.capacity(); ++i) {
        this->fifo.push(42 + i);
    }

    for (auto i = 0u; i < this->fifo.capacity(); ++i) {
        EXPECT_EQ(this->fifo.capacity() - i, this->fifo.size());
        auto value = typename TestFixture::value_type{};
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42 + i, value);
    }
    EXPECT_EQ(0, this->fifo.size());
    EXPECT_TRUE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.pop(value));
}

TYPED_TEST(FifoTest, popFullFifo) {
    auto value = typename TestFixture::value_type{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.capacity(); ++i) {
        this->fifo.push(42 + i);
    }
    EXPECT_TRUE(this->fifo.full());

    for (auto i = 0u; i < this->fifo.capacity()*4; ++i) {
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42 + i, value);
    EXPECT_FALSE(this->fifo.full());

        EXPECT_TRUE(this->fifo.push(42 + 4 + i));
        EXPECT_TRUE(this->fifo.full());
    }
}

TYPED_TEST(FifoTest, popEmpty) {
    auto value = typename TestFixture::value_type{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.capacity()*4; ++i) {
        EXPECT_TRUE(this->fifo.empty());
        EXPECT_TRUE(this->fifo.push(42 + i));
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42 + i, value);
    }

    EXPECT_TRUE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.pop(value));
}

TYPED_TEST(FifoTest, wrap) {
    auto value = typename TestFixture::value_type{};
    for (auto i = 0u; i < this->fifo.capacity() * 2 + 1; ++i) {
        this->fifo.push(42 + i);
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42 + i, value);
    }

    for (auto i = 0u; i < 8u; ++i) {
        this->fifo.push(42 + i);
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42 + i, value);
    }
}


template<typename FifoT> using ProxyTest = FifoTestBase<FifoT>;
using ProxyFifoTypes = ::testing::Types<
    Fifo5<test_type>
    >;
TYPED_TEST_SUITE(ProxyTest, ProxyFifoTypes);

TYPED_TEST(ProxyTest, properties) {
        EXPECT_FALSE(std::is_copy_constructible_v<typename TestFixture::FifoType::pusher_t>);
        EXPECT_TRUE(std::is_move_constructible_v<typename TestFixture::FifoType::pusher_t>);
        EXPECT_FALSE(std::is_copy_assignable_v<typename TestFixture::FifoType::pusher_t>);
        EXPECT_TRUE(std::is_move_assignable_v<typename TestFixture::FifoType::pusher_t>);

        EXPECT_FALSE(std::is_copy_constructible_v<typename TestFixture::FifoType::popper_t>);
        EXPECT_TRUE(std::is_move_constructible_v<typename TestFixture::FifoType::popper_t>);
        EXPECT_FALSE(std::is_copy_assignable_v<typename TestFixture::FifoType::popper_t>);
        EXPECT_TRUE(std::is_move_assignable_v<typename TestFixture::FifoType::popper_t>);
}

TYPED_TEST(ProxyTest, pusher) {
    for (auto i = 0u; i < this->fifo.capacity(); ++i) {
        EXPECT_FALSE(this->fifo.full());
        {
            auto pusher = this->fifo.push();
            EXPECT_TRUE(!!pusher);
            pusher = i;

            EXPECT_EQ(i, *pusher.get());
            EXPECT_EQ(i, *pusher);
            EXPECT_EQ(i, *pusher.operator->());
            // TODO EXPECT_EQ(i, static_cast<typename TestFixture::FifoType::ValueType>(pusher));

            EXPECT_EQ(i, *std::as_const(pusher).get());
            EXPECT_EQ(i, *std::as_const(pusher));
            EXPECT_EQ(i, *std::as_const(pusher).operator->());
            // TODO EXPECT_EQ(i, static_cast<typename TestFixture::FifoType::ValueType>(std::as_const(pusher)));
        }
        EXPECT_FALSE(this->fifo.empty());
    }
    EXPECT_TRUE(this->fifo.full());
    EXPECT_FALSE(!!this->fifo.push());
}

TYPED_TEST(ProxyTest, pusherRelease) {
    this->fifo.push() = 42;
    EXPECT_FALSE(this->fifo.empty());

    {
        auto pusher = this->fifo.push();
        ASSERT_TRUE(!!pusher);
        pusher = 24;
        pusher.release();
        EXPECT_FALSE(!!pusher);
    }
    EXPECT_EQ(42, *this->fifo.pop());
    EXPECT_TRUE(this->fifo.empty());
}

TYPED_TEST(ProxyTest, popperRelease) {
    this->fifo.push() = 42;
    EXPECT_FALSE(this->fifo.empty());

    {
        auto popper = this->fifo.pop();
        ASSERT_TRUE(!!popper);
        EXPECT_EQ(42, *popper);
        popper.release();
        EXPECT_FALSE(!!popper);
    }
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_EQ(42, *this->fifo.pop());
}


struct ABC
{
    int a;
    int b;
    int c;
};

// Specialize ValueSizeTraits not to copy ABC::c.
template<>
inline std::size_t ValueSizeTraits<ABC>::size(value_type const&) {
    return sizeof(ABC::a) + sizeof(ABC::b);
}


template<typename FifoT> using ProxyMoveTest = FifoTestBase<FifoT>;
using ProxyMoveFifoTypes = ::testing::Types<
    Fifo5<ABC>
    >;
TYPED_TEST_SUITE(ProxyMoveTest, ProxyMoveFifoTypes);

TYPED_TEST(ProxyMoveTest, pusherMove) {
    auto pusher = this->fifo.push();

    pusher = ABC{100, 200, 300};
    EXPECT_TRUE(!!pusher);
    EXPECT_EQ(100, pusher->a);

    // move ctor
    auto pusher2 = std::move(pusher);
    EXPECT_FALSE(!!pusher);
    EXPECT_TRUE(!!pusher2);
    EXPECT_EQ(100, pusher2->a);

    // move assignment
    pusher = std::move(pusher2);
    EXPECT_TRUE(!!pusher);
    EXPECT_FALSE(!!pusher2);
    EXPECT_EQ(100, pusher->a);
}

TYPED_TEST(ProxyMoveTest, popperMove) {
    for (auto i = 0; i < static_cast<int>(this->fifo.capacity()); ++i) {
        this->fifo.push(ABC{42 + i, 43, 44});
    }

    for (auto i = 0u; i < this->fifo.capacity(); ++i) {
        auto popper = this->fifo.pop();
        ASSERT_TRUE(!!popper);
        EXPECT_EQ(42 + i, popper->a);

        // move ctor
        auto popper2 = std::move(popper);
        EXPECT_FALSE(!!popper);
        ASSERT_TRUE(!!popper2);
        EXPECT_EQ(42 + i, popper2->a);

        // move assignment
        popper = std::move(popper2);
        ASSERT_TRUE(!!popper);
        EXPECT_FALSE(!!popper2);
        EXPECT_EQ(42 + i, popper->a);
    }
    EXPECT_TRUE(this->fifo.empty());
}

TYPED_TEST(ProxyMoveTest, pusherUsesValueSizeTraits) {
    // Push and pop a value into every slot of the fifo using assign
    // directly to value_type&. This bypasses use of ValueSizeTraits.
    for (auto i = 0; i < static_cast<int>(this->fifo.capacity()); ++i) {
        {
            auto pusher = this->fifo.push();
            *pusher = ABC{1, 2, 3};
        }
        auto popper = this->fifo.pop();
        EXPECT_EQ(1, popper->a);
        EXPECT_EQ(2, popper->b);
        EXPECT_EQ(3, popper->c);
    }

    // Now push using operator= and a different value
    {
        auto pusher = this->fifo.push();
        pusher = ABC{100, 200, 300};
    }

    auto popper = this->fifo.pop();
    EXPECT_EQ(100, popper->a);
    EXPECT_EQ(200, popper->b);
    EXPECT_EQ(3, popper->c);

}
