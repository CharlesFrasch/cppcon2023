#include "Fifo1.hpp"
#include "Fifo2.hpp"
#include "Fifo3.hpp"
#include "Fifo4.hpp"
#include "Fifo5.hpp"
// TODO #include "Fifo7.hpp"

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

using test_type = int;


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
    EXPECT_EQ(4u, this->fifo.size());
    EXPECT_TRUE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());
}

TYPED_TEST(FifoTest, push) {
    ASSERT_EQ(4u, this->fifo.size());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.full());

    EXPECT_TRUE(this->fifo.push(42));
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_TRUE(this->fifo.full());

    EXPECT_FALSE(this->fifo.push(42));
    EXPECT_FALSE(this->fifo.empty());
    EXPECT_TRUE(this->fifo.full());
}

TYPED_TEST(FifoTest, pop) {
    auto value = typename TestFixture::value_type{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.size(); ++i) {
        this->fifo.push(42 + i);
    }

    for (auto i = 0u; i < this->fifo.size(); ++i) {
        auto value = typename TestFixture::value_type{};
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42+i, value);
    }
    EXPECT_FALSE(this->fifo.pop(value));
}

TYPED_TEST(FifoTest, popFullFifo) {
    auto value = typename TestFixture::value_type{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.size(); ++i) {
        this->fifo.push(42 + i);
    }
    EXPECT_TRUE(this->fifo.full());

    for (auto i = 0u; i < this->fifo.size()*4; ++i) {
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42+i, value);
    EXPECT_FALSE(this->fifo.full());

        EXPECT_TRUE(this->fifo.push(42 + 4 + i));
        EXPECT_TRUE(this->fifo.full());
    }
}

TYPED_TEST(FifoTest, popEmpty) {
    auto value = typename TestFixture::value_type{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.size()*4; ++i) {
        EXPECT_TRUE(this->fifo.empty());
        EXPECT_TRUE(this->fifo.push(42 + i));
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42+i, value);
    }

    EXPECT_TRUE(this->fifo.empty());
    EXPECT_FALSE(this->fifo.pop(value));
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
    for (auto i = 0u; i < this->fifo.size(); ++i) {
        EXPECT_FALSE(this->fifo.full());
        {
            auto pusher = this->fifo.push();
            EXPECT_TRUE(!!pusher);
            pusher = i;

            EXPECT_EQ(i, pusher.get());
            EXPECT_EQ(i, *pusher);
            EXPECT_EQ(i, *pusher.operator->());
            // TODO EXPECT_EQ(i, static_cast<typename TestFixture::FifoType::ValueType>(pusher));

            EXPECT_EQ(i, std::as_const(pusher).get());
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
        pusher = 24;
        pusher.release();
    }
    EXPECT_EQ(42, *this->fifo.pop());
    EXPECT_TRUE(this->fifo.empty());
}



struct ABC
{
    int a;
    int b;
    int c;
};

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
    EXPECT_EQ(200, pusher->b);
    EXPECT_EQ(300, pusher->c);

    // move ctor
    auto pusher2 = std::move(pusher);
    EXPECT_FALSE(!!pusher);
    EXPECT_TRUE(!!pusher2);
    EXPECT_EQ(100, pusher2->a);
    EXPECT_EQ(200, pusher2->b);
    EXPECT_EQ(300, pusher2->c);

    // move assignment
    pusher = std::move(pusher2);
    EXPECT_TRUE(!!pusher);
    EXPECT_FALSE(!!pusher2);
    EXPECT_EQ(100, pusher->a);
    EXPECT_EQ(200, pusher->b);
    EXPECT_EQ(300, pusher->c);
}
