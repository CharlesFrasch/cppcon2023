#include "Fifo1.hpp"
#include "Fifo2.hpp"
#include "Fifo3.hpp"
#include "Fifo4.hpp"
#include "Fifo7.hpp"

#include <gtest/gtest.h>

#include <type_traits>


template<typename FifoT>
class FifoTest : public testing::Test {
public:
    using FifoType = FifoT;
    using ValueType = typename FifoType::ValueType;

    FifoType fifo{4};
};

using ValueType = int;
using FifoTypes = ::testing::Types<
    Fifo1<ValueType>,
    Fifo2<ValueType>,
    Fifo3<ValueType>,
    Fifo4<ValueType>,
    Fifo7<ValueType>
    >;
TYPED_TEST_SUITE(FifoTest, FifoTypes);


TYPED_TEST(FifoTest, properties) {
    // TODO
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
    auto value = typename TestFixture::ValueType{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.size(); ++i) {
        this->fifo.push(42 + i);
    }

    for (auto i = 0u; i < this->fifo.size(); ++i) {
        auto value = typename TestFixture::ValueType{};
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42+i, value);
    }
    EXPECT_FALSE(this->fifo.pop(value));
}

TYPED_TEST(FifoTest, popFullFifo) {
    auto value = typename TestFixture::ValueType{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.size(); ++i) {
        this->fifo.push(42 + i);
    }
    EXPECT_TRUE(this->fifo.full());

    for (auto i = 0u; i < this->fifo.size()*4; ++i) {
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42+i, value);

        EXPECT_TRUE(this->fifo.push(42 + 4 + i));
        EXPECT_TRUE(this->fifo.full());
    }
}

TYPED_TEST(FifoTest, popEmpty) {
    auto value = typename TestFixture::ValueType{};
    EXPECT_FALSE(this->fifo.pop(value));

    for (auto i = 0u; i < this->fifo.size()*4; ++i) {
        EXPECT_TRUE(this->fifo.empty());
        EXPECT_TRUE(this->fifo.push(42 + i));
        EXPECT_TRUE(this->fifo.pop(value));
        EXPECT_EQ(42+i, value);
    }
}
