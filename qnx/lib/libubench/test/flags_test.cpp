#include "ubench/flags.h"

#include <gtest/gtest.h>

enum class netflags {
  IFF_UP = 1,
  IFF_DOWN = 2,
  IFF_RUNNING = 4,
  IFF_LOOPBACK = 8,
  IFF_RUNLOOP = IFF_RUNNING | IFF_LOOPBACK,
};
DEFINE_GLOBAL_FLAG_OPERATORS(netflags)

enum cnetflags {
  IFF_UP = 1,
  IFF_DOWN = 2,
  IFF_RUNNING = 4,
  IFF_LOOPBACK = 8,
  IFF_RUNLOOP = IFF_RUNNING | IFF_LOOPBACK,
};
DEFINE_GLOBAL_FLAG_OPERATORS(cnetflags)

TEST(flags, default) {
  ubench::flags<netflags> x{};
  EXPECT_EQ(static_cast<int>(x), 0);
  EXPECT_FALSE(x);
  EXPECT_TRUE(!x);

  ubench::flags<cnetflags> y{};
  EXPECT_EQ(static_cast<unsigned int>(y), 0);
  EXPECT_FALSE(y);
  EXPECT_TRUE(!y);
}

TEST(flags, ctor_set_underlying_mask_type) {
  ubench::flags<netflags> x0{0};
  EXPECT_FALSE(x0 & netflags::IFF_UP);
  EXPECT_FALSE(x0 & netflags::IFF_DOWN);
  EXPECT_FALSE(x0 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x1{1};
  EXPECT_TRUE(x1 & netflags::IFF_UP);
  EXPECT_FALSE(x1 & netflags::IFF_DOWN);
  EXPECT_FALSE(x1 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x2{2};
  EXPECT_FALSE(x2 & netflags::IFF_UP);
  EXPECT_TRUE(x2 & netflags::IFF_DOWN);
  EXPECT_FALSE(x2 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x3{3};
  EXPECT_TRUE(x3 & netflags::IFF_UP);
  EXPECT_TRUE(x3 & netflags::IFF_DOWN);
  EXPECT_FALSE(x3 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x4{-1};
  EXPECT_TRUE(x4 & netflags::IFF_UP);
  EXPECT_TRUE(x4 & netflags::IFF_DOWN);
  EXPECT_TRUE(x4 & netflags::IFF_LOOPBACK);

  ubench::flags<cnetflags> y0{0};
  EXPECT_FALSE(y0 & IFF_UP);
  EXPECT_FALSE(y0 & IFF_DOWN);
  EXPECT_FALSE(y0 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y1{1};
  EXPECT_TRUE(y1 & IFF_UP);
  EXPECT_FALSE(y1 & IFF_DOWN);
  EXPECT_FALSE(y1 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y2{2};
  EXPECT_FALSE(y2 & IFF_UP);
  EXPECT_TRUE(y2 & IFF_DOWN);
  EXPECT_FALSE(y2 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y3{3};
  EXPECT_TRUE(y3 & IFF_UP);
  EXPECT_TRUE(y3 & IFF_DOWN);
  EXPECT_FALSE(y3 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y4{~0u};
  EXPECT_TRUE(y4 & IFF_UP);
  EXPECT_TRUE(y4 & IFF_DOWN);
  EXPECT_TRUE(y4 & IFF_LOOPBACK);
}

TEST(flags, ctor_set_bit_type) {
  ubench::flags<netflags> x{netflags::IFF_LOOPBACK};
  EXPECT_TRUE(x & netflags::IFF_LOOPBACK);
  EXPECT_FALSE(x & netflags::IFF_RUNNING);
  EXPECT_EQ(static_cast<int>(x), IFF_LOOPBACK);

  ubench::flags<cnetflags> y{IFF_LOOPBACK};
  EXPECT_TRUE(y & IFF_LOOPBACK);
  EXPECT_FALSE(y & IFF_RUNNING);
  EXPECT_EQ(static_cast<unsigned int>(y), IFF_LOOPBACK);
}

TEST(flags, ctor_set_bit_type_two_bits) {
  ubench::flags<netflags> x{netflags::IFF_RUNLOOP};
  EXPECT_TRUE(x & netflags::IFF_LOOPBACK);
  EXPECT_TRUE(x & netflags::IFF_RUNNING);
  EXPECT_EQ(static_cast<int>(x), IFF_RUNLOOP);

  ubench::flags<cnetflags> y{IFF_RUNLOOP};
  EXPECT_TRUE(y & IFF_LOOPBACK);
  EXPECT_TRUE(y & IFF_RUNNING);
  EXPECT_EQ(static_cast<unsigned int>(y), IFF_RUNLOOP);
}

TEST(flags, ctor_set_flag_type) {
  ubench::flags<netflags> x0{netflags::IFF_RUNLOOP};
  ubench::flags<netflags> x{x0};
  EXPECT_TRUE(x & netflags::IFF_LOOPBACK);
  EXPECT_TRUE(x & netflags::IFF_RUNNING);
  EXPECT_EQ(static_cast<int>(x), IFF_RUNLOOP);

  ubench::flags<cnetflags> y0{IFF_RUNLOOP};
  ubench::flags<cnetflags> y{y0};
  EXPECT_TRUE(y & IFF_LOOPBACK);
  EXPECT_TRUE(y & IFF_RUNNING);
  EXPECT_EQ(static_cast<unsigned int>(y), IFF_RUNLOOP);
}

TEST(flags, is_zero) {
  ubench::flags<netflags> x0{0};
  ubench::flags<netflags> x1{netflags::IFF_UP};
  EXPECT_TRUE(!x0);
  EXPECT_FALSE(!x1);

  ubench::flags<cnetflags> y0{0};
  ubench::flags<cnetflags> y1{IFF_UP};
  EXPECT_TRUE(!y0);
  EXPECT_FALSE(!y1);
}

TEST(flags, mask) {
  ubench::flags<netflags> x{netflags::IFF_RUNLOOP};
  ubench::flags<netflags> xm1{netflags::IFF_RUNNING};
  ubench::flags<netflags> xm2(netflags::IFF_UP);
  EXPECT_EQ(static_cast<int>(x & xm1), IFF_RUNNING);
  EXPECT_EQ(static_cast<int>(x & xm2), 0);

  ubench::flags<cnetflags> y{IFF_RUNLOOP};
  ubench::flags<cnetflags> ym1{IFF_RUNNING};
  ubench::flags<cnetflags> ym2(IFF_UP);
  EXPECT_EQ(static_cast<unsigned int>(y & ym1), IFF_RUNNING);
  EXPECT_EQ(static_cast<unsigned int>(y & ym2), 0);
}

TEST(flags, or_add) {
  ubench::flags<netflags> x{netflags::IFF_RUNNING};
  ubench::flags<netflags> xm{netflags::IFF_LOOPBACK};
  EXPECT_EQ(static_cast<int>(x | xm), IFF_RUNLOOP);

  ubench::flags<cnetflags> y{IFF_RUNNING};
  ubench::flags<cnetflags> ym{IFF_LOOPBACK};
  EXPECT_EQ(static_cast<unsigned int>(y | ym), IFF_RUNLOOP);
}

TEST(flags, or_nothing) {
  ubench::flags<netflags> x{netflags::IFF_RUNNING};
  ubench::flags<netflags> xm{netflags::IFF_RUNNING};
  EXPECT_EQ(static_cast<int>(x | xm), IFF_RUNNING);

  ubench::flags<cnetflags> y{IFF_RUNNING};
  ubench::flags<cnetflags> ym{IFF_RUNNING};
  EXPECT_EQ(static_cast<unsigned int>(y | ym), IFF_RUNNING);
}

TEST(flags, invert_add) {
  ubench::flags<netflags> x{netflags::IFF_RUNNING};
  ubench::flags<netflags> xm{netflags::IFF_LOOPBACK};
  EXPECT_EQ(static_cast<int>(x ^ xm), IFF_RUNLOOP);

  ubench::flags<cnetflags> y{IFF_RUNNING};
  ubench::flags<cnetflags> ym{IFF_LOOPBACK};
  EXPECT_EQ(static_cast<unsigned int>(y ^ ym), IFF_RUNLOOP);
}

TEST(flags, invert_clar) {
  ubench::flags<netflags> x{netflags::IFF_RUNNING};
  ubench::flags<netflags> xm{netflags::IFF_RUNNING};
  EXPECT_EQ(static_cast<int>(x ^ xm), 0);

  ubench::flags<cnetflags> y{IFF_RUNNING};
  ubench::flags<cnetflags> ym{IFF_RUNNING};
  EXPECT_EQ(static_cast<unsigned int>(y ^ ym), 0);
}

TEST(flags, invert) {
  ubench::flags<netflags> x{netflags::IFF_RUNNING};
  EXPECT_TRUE(~x & netflags::IFF_UP);
  EXPECT_TRUE(~x & netflags::IFF_DOWN);
  EXPECT_FALSE(~x & netflags::IFF_RUNNING);
  EXPECT_TRUE(~x & netflags::IFF_LOOPBACK);

  ubench::flags<cnetflags> y{IFF_RUNNING};
  EXPECT_TRUE(~y & IFF_UP);
  EXPECT_TRUE(~y & IFF_DOWN);
  EXPECT_FALSE(~y & IFF_RUNNING);
  EXPECT_TRUE(~y & IFF_LOOPBACK);
}
TEST(flags, assign_set_underlying_mask_type) {
  // the constructor(int) is explicit and we don't have an assignment operator.
  ubench::flags<netflags> x0{};
  x0 = ubench::flags<netflags>{0};
  EXPECT_FALSE(x0 & netflags::IFF_UP);
  EXPECT_FALSE(x0 & netflags::IFF_DOWN);
  EXPECT_FALSE(x0 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x1;
  x1 = ubench::flags<netflags>{1};
  EXPECT_TRUE(x1 & netflags::IFF_UP);
  EXPECT_FALSE(x1 & netflags::IFF_DOWN);
  EXPECT_FALSE(x1 & netflags::IFF_LOOPBACK);

  auto x2 = ubench::flags<netflags>{2};
  EXPECT_FALSE(x2 & netflags::IFF_UP);
  EXPECT_TRUE(x2 & netflags::IFF_DOWN);
  EXPECT_FALSE(x2 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x3;
  x3 = ubench::flags<netflags>{3};
  EXPECT_TRUE(x3 & netflags::IFF_UP);
  EXPECT_TRUE(x3 & netflags::IFF_DOWN);
  EXPECT_FALSE(x3 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x4{};
  x4 = ubench::flags<netflags>{-1};
  EXPECT_TRUE(x4 & netflags::IFF_UP);
  EXPECT_TRUE(x4 & netflags::IFF_DOWN);
  EXPECT_TRUE(x4 & netflags::IFF_LOOPBACK);

  auto y0 = ubench::flags<cnetflags>{0};
  EXPECT_FALSE(y0 & IFF_UP);
  EXPECT_FALSE(y0 & IFF_DOWN);
  EXPECT_FALSE(y0 & IFF_LOOPBACK);

  auto y1 = ubench::flags<cnetflags>{1};
  EXPECT_TRUE(y1 & IFF_UP);
  EXPECT_FALSE(y1 & IFF_DOWN);
  EXPECT_FALSE(y1 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y2{};
  y2 = ubench::flags<cnetflags>{2};
  EXPECT_FALSE(y2 & IFF_UP);
  EXPECT_TRUE(y2 & IFF_DOWN);
  EXPECT_FALSE(y2 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y3;
  y3 = ubench::flags<cnetflags>{3};
  EXPECT_TRUE(y3 & IFF_UP);
  EXPECT_TRUE(y3 & IFF_DOWN);
  EXPECT_FALSE(y3 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y4;
  y4 = ubench::flags<cnetflags>{~0u};
  EXPECT_TRUE(y4 & IFF_UP);
  EXPECT_TRUE(y4 & IFF_DOWN);
  EXPECT_TRUE(y4 & IFF_LOOPBACK);
}

TEST(flags, assign_set_bit_type) {
  ubench::flags<netflags> x0{};
  x0 = ubench::flags<netflags>{0};
  EXPECT_FALSE(x0 & netflags::IFF_UP);
  EXPECT_FALSE(x0 & netflags::IFF_DOWN);
  EXPECT_FALSE(x0 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x1;
  x1 = netflags::IFF_UP;
  EXPECT_TRUE(x1 & netflags::IFF_UP);
  EXPECT_FALSE(x1 & netflags::IFF_DOWN);
  EXPECT_FALSE(x1 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x2 = netflags::IFF_DOWN;
  EXPECT_FALSE(x2 & netflags::IFF_UP);
  EXPECT_TRUE(x2 & netflags::IFF_DOWN);
  EXPECT_FALSE(x2 & netflags::IFF_LOOPBACK);

  // Can't have an overload for `operator(bit_type, bit_type) ->
  // flags<bit_type>` due to ambiguity with any enum, and this has precedence
  // over the `|=` operator. If the user wants that, they have to implement the
  // specific type without a template.
  ubench::flags<netflags> x3;
  x3 = netflags::IFF_UP | netflags::IFF_DOWN;
  x3 |= netflags::IFF_UP | netflags::IFF_DOWN;
  EXPECT_TRUE(x3 & netflags::IFF_UP);
  EXPECT_TRUE(x3 & netflags::IFF_DOWN);
  EXPECT_FALSE(x3 & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x3b;
  x3b = ubench::flags<netflags>(netflags::IFF_UP) | netflags::IFF_DOWN;
  EXPECT_TRUE(x3b & netflags::IFF_UP);
  EXPECT_TRUE(x3b & netflags::IFF_DOWN);
  EXPECT_FALSE(x3b & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x3c;
  x3c = netflags::IFF_UP | ubench::flags<netflags>(netflags::IFF_DOWN);
  EXPECT_TRUE(x3c & netflags::IFF_UP);
  EXPECT_TRUE(x3c & netflags::IFF_DOWN);
  EXPECT_FALSE(x3c & netflags::IFF_LOOPBACK);

  ubench::flags<netflags> x4{};
  x4 = ubench::flags<netflags>{-1};
  EXPECT_TRUE(x4 & netflags::IFF_UP);
  EXPECT_TRUE(x4 & netflags::IFF_DOWN);
  EXPECT_TRUE(x4 & netflags::IFF_LOOPBACK);

  auto y0 = ubench::flags<cnetflags>{0};
  EXPECT_FALSE(y0 & IFF_UP);
  EXPECT_FALSE(y0 & IFF_DOWN);
  EXPECT_FALSE(y0 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y1 = IFF_UP;
  EXPECT_TRUE(y1 & IFF_UP);
  EXPECT_FALSE(y1 & IFF_DOWN);
  EXPECT_FALSE(y1 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y2{};
  y2 = IFF_DOWN;
  EXPECT_FALSE(y2 & IFF_UP);
  EXPECT_TRUE(y2 & IFF_DOWN);
  EXPECT_FALSE(y2 & IFF_LOOPBACK);

  // Can't have an overload for `operator(bit_type, bit_type) ->
  // flags<bit_type>` due to ambiguity with any enum, and this has precedence
  // over the `|=` operator. If the user wants that, they have to implement the
  // specific type without a template.
  //
  // These are defined with the macro DEFINE_GLOBAL_FLAG_OPERATORS(E) earlier to
  // define the operators *only* for the specific type.
  ubench::flags<cnetflags> y3;
  y3 = IFF_UP | IFF_DOWN;
  y3 |= IFF_UP | IFF_DOWN;
  EXPECT_TRUE(y3 & IFF_UP);
  EXPECT_TRUE(y3 & IFF_DOWN);
  EXPECT_FALSE(y3 & IFF_LOOPBACK);

  ubench::flags<cnetflags> y3b;
  y3b = ubench::flags<cnetflags>(IFF_UP) | IFF_DOWN;
  EXPECT_TRUE(y3b & IFF_UP);
  EXPECT_TRUE(y3b & IFF_DOWN);
  EXPECT_FALSE(y3b & IFF_LOOPBACK);

  ubench::flags<cnetflags> y3c;
  y3c = IFF_UP | ubench::flags<cnetflags>(IFF_DOWN);
  EXPECT_TRUE(y3c & IFF_UP);
  EXPECT_TRUE(y3c & IFF_DOWN);
  EXPECT_FALSE(y3c & IFF_LOOPBACK);

  ubench::flags<cnetflags> y4;
  y4 = ubench::flags<cnetflags>{~0u};
  EXPECT_TRUE(y4 & IFF_UP);
  EXPECT_TRUE(y4 & IFF_DOWN);
  EXPECT_TRUE(y4 & IFF_LOOPBACK);
}

TEST(flags, equality) {
  ubench::flags<netflags> x0{0};
  ubench::flags<netflags> x1{1};
  ubench::flags<netflags> x2{2};
  ubench::flags<netflags> x3{3};
  ubench::flags<netflags> x4{-1};
  EXPECT_FALSE(x2 == x0);
  EXPECT_FALSE(x2 == x1);
  EXPECT_TRUE(x2 == x2);
  EXPECT_FALSE(x2 == x3);
  EXPECT_FALSE(x2 == x4);

  ubench::flags<cnetflags> y0{0};
  ubench::flags<cnetflags> y1{1};
  ubench::flags<cnetflags> y2{2};
  ubench::flags<cnetflags> y3{3};
  ubench::flags<cnetflags> y4{~0u};
  EXPECT_FALSE(y2 == y0);
  EXPECT_FALSE(y2 == y1);
  EXPECT_TRUE(y2 == y2);
  EXPECT_FALSE(y2 == y3);
  EXPECT_FALSE(y2 == y4);
}
