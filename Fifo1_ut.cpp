#include "Fifo1.hpp"

#include <gtest/gtest.h>

#include <type_traits>


using TestFifo = Fifo1<int>;

TEST(Fifo1, initialConditions) {
    auto fifo = TestFifo(1234);

    EXPECT_EQ(1234u, fifo.size());
    EXPECT_TRUE(fifo.empty());
    EXPECT_FALSE(fifo.full());
}

TEST(Fifo1, push) {
    auto fifo = TestFifo(4);
    ASSERT_EQ(4u, fifo.size());

    EXPECT_TRUE(fifo.push(42));
    EXPECT_FALSE(fifo.empty());
    EXPECT_FALSE(fifo.full());

    EXPECT_TRUE(fifo.push(42));
    EXPECT_FALSE(fifo.empty());
    EXPECT_FALSE(fifo.full());

    EXPECT_TRUE(fifo.push(42));
    EXPECT_FALSE(fifo.empty());
    EXPECT_FALSE(fifo.full());

    EXPECT_TRUE(fifo.push(42));
    EXPECT_FALSE(fifo.empty());
    EXPECT_TRUE(fifo.full());

    EXPECT_FALSE(fifo.push(42));
    EXPECT_FALSE(fifo.empty());
    EXPECT_TRUE(fifo.full());
}

TEST(Fifo1, pop) {
    auto fifo = TestFifo(4);

    auto value = TestFifo::ValueType{};
    EXPECT_FALSE(fifo.pop(value));

    for (auto i = 0u; i < fifo.size(); ++i) {
        fifo.push(42 + i);
    }

    for (auto i = 0u; i < fifo.size(); ++i) {
        auto value = TestFifo::ValueType{};
        EXPECT_TRUE(fifo.pop(value));
        EXPECT_EQ(42+i, value);
    }
    EXPECT_FALSE(fifo.pop(value));
}

TEST(Fifo1, popFullFifo) {
    auto fifo = TestFifo(4);

    auto value = TestFifo::ValueType{};
    EXPECT_FALSE(fifo.pop(value));

    for (auto i = 0u; i < fifo.size(); ++i) {
        fifo.push(42 + i);
    }
    EXPECT_TRUE(fifo.full());

    for (auto i = 0u; i < fifo.size()*4; ++i) {
        EXPECT_TRUE(fifo.pop(value));
        EXPECT_EQ(42+i, value);

        EXPECT_TRUE(fifo.push(42 + 4 + i));
        EXPECT_TRUE(fifo.full());
    }
}

TEST(Fifo1, popEmpty) {
    auto fifo = TestFifo(4);

    auto value = TestFifo::ValueType{};
    EXPECT_FALSE(fifo.pop(value));

    for (auto i = 0u; i < fifo.size()*4; ++i) {
        EXPECT_TRUE(fifo.empty());
        EXPECT_TRUE(fifo.push(42 + i));
        EXPECT_TRUE(fifo.pop(value));
        EXPECT_EQ(42+i, value);
    }
}
