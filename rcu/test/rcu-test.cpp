#include "rcu.h"

#include <gtest/gtest.h>

#include <memory>
#include <thread>

TEST(RcuInitTest, NullPointer) {
  EXPECT_EXIT(
      {
        // Initialise direct with `nullptr`.
        rcu<int> x{nullptr};
      },
      testing::KilledBySignal(SIGABRT), "");
}

TEST(RcuInitTest, NullPointerFromReset) {
  std::unique_ptr<int> v = std::make_unique<int>(5);
  v.reset(nullptr);

  EXPECT_EXIT(
      {
        // Move an empty unique_ptr.
        rcu<int> x{std::move(v)};
      },
      testing::KilledBySignal(SIGABRT), "");
}

TEST(RcuInitTest, DestructorWhenNotFree) {
  // No default constructor, so we make use of another. From an API, not ideal,
  // we'd want a default constructor and only have `rcu` be able to create
  // `rcu_ptr` with a reference.
  rcu_ptr<int> p;

  EXPECT_EXIT(
      {
        rcu<int> x{std::make_unique<int>(5)};
        p = x.read();

        // x would be destroyed here, while p outlives x.
      },
      testing::KilledBySignal(SIGABRT), "");
}

TEST(RcuRead, InitAndRead) {
  std::unique_ptr<int> v = std::make_unique<int>(10);
  int *rv = v.get();
  rcu<int> x{std::move(v)};

  rcu_ptr<int> p = x.read();

  // 2 references. The first in `rcu` because it's active, and `p`.
  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rv);
  EXPECT_EQ(*p, 10);
}

TEST(RcuRead, ReadTwice) {
  std::unique_ptr<int> v = std::make_unique<int>(10);
  int *rv = v.get();
  rcu<int> x{std::move(v)};

  rcu_ptr<int> p = x.read();
  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rv);

  rcu_ptr<int> q = x.read();
  EXPECT_EQ(p.use_count(), 3);
  EXPECT_EQ(p.get(), rv);
  EXPECT_EQ(q.use_count(), 3);
  EXPECT_EQ(q.get(), rv);
}

TEST(RcuRead, ReadTwiceFree) {
  std::unique_ptr<int> v = std::make_unique<int>(10);
  int *rv = v.get();
  rcu<int> x{std::move(v)};

  rcu_ptr<int> p = x.read();
  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rv);

  {
    rcu_ptr<int> q = x.read();
    EXPECT_EQ(p.use_count(), 3);
    EXPECT_EQ(p.get(), rv);
    EXPECT_EQ(q.use_count(), 3);
    EXPECT_EQ(q.get(), rv);

    // q is dropped. Dereference by 1.
  }

  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rv);
}

TEST(RcuUpdate, UpdateFirst) {
  std::unique_ptr<int> v = std::make_unique<int>(10);
  rcu<int> x{std::move(v)};

  std::unique_ptr<int> w = std::make_unique<int>(20);
  int *rw = w.get();
  EXPECT_TRUE(x.update(std::move(w)));

  rcu_ptr<int> p = x.read();
  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rw);
  EXPECT_EQ(*p, 20);
}

TEST(RcuUpdate, ReadUpdate) {
  std::unique_ptr<int> v = std::make_unique<int>(10);
  int *rv = v.get();
  rcu<int> x{std::move(v)};

  rcu_ptr<int> p = x.read();
  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rv);

  std::unique_ptr<int> w = std::make_unique<int>(20);
  int *rw = w.get();
  EXPECT_TRUE(x.update(std::move(w)));

  rcu_ptr<int> q = x.read();
  EXPECT_EQ(q.use_count(), 2);
  EXPECT_EQ(q.get(), rw);
  EXPECT_EQ(*q, 20);

  // p has a reference, `rcu` has decremented the reference showing that the
  // update has allocated new space and indicated it no longer holds the old
  // reference active.
  EXPECT_EQ(p.use_count(), 1);
  EXPECT_EQ(p.get(), rv);
  EXPECT_EQ(*p, 10);
}

TEST(RcuUpdate, UpdateTwice) {
  std::unique_ptr<int> v = std::make_unique<int>(10);
  int *rv = v.get();
  rcu<int> x{std::move(v)};

  rcu_ptr<int> p = x.read();
  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rv);

  std::unique_ptr<int> w = std::make_unique<int>(20);
  EXPECT_TRUE(x.update(std::move(w)));

  std::unique_ptr<int> t = std::make_unique<int>(30);
#ifndef __clang_analyzer__
  // Ignore the "use after free" diagnostic. Not relevant here.
  int *rt = t.get();
#else
  int *rt = nullptr;
#endif
  EXPECT_TRUE(x.update(std::move(t)));

  rcu_ptr<int> q = x.read();
  EXPECT_EQ(q.use_count(), 2);
  EXPECT_EQ(q.get(), rt);
  EXPECT_EQ(*q, 30);

  // p has a reference, `rcu` has decremented the reference showing that the
  // update has allocated new space and indicated it no longer holds the old
  // reference active.
  EXPECT_EQ(p.use_count(), 1);
  EXPECT_EQ(p.get(), rv);
  EXPECT_EQ(*p, 10);
}

TEST(RcuUpdate, UpdateMany) {
  std::unique_ptr<int> v = std::make_unique<int>(10);
  int *rv = v.get();
  rcu<int, 5> x{std::move(v)};

  rcu_ptr<int> p = x.read();
  EXPECT_EQ(p.use_count(), 2);
  EXPECT_EQ(p.get(), rv);

  // We want to call `update()` just a bit more than the number of elements in
  // the array.
  int *rt = nullptr;
  for (int i = 0; i < 10; i++) {
    std::unique_ptr<int> t = std::make_unique<int>(20);
    rt = t.get();
    EXPECT_TRUE(x.update(std::move(t)));
  }

  rcu_ptr<int> q = x.read();
  EXPECT_EQ(q.use_count(), 2);
  EXPECT_EQ(q.get(), rt);
  EXPECT_EQ(*q, 20);

  // p has a reference, `rcu` has decremented the reference showing that the
  // update has allocated new space and indicated it no longer holds the old
  // reference active.
  EXPECT_EQ(p.use_count(), 1);
  EXPECT_EQ(p.get(), rv);
  EXPECT_EQ(*p, 10);
}

TEST(RcuUpdate, UpdateFull) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  // Now we want to use all 5 slots
  rcu_ptr<int> p0 = x.read();
  EXPECT_EQ(p0.use_count(), 2);

  EXPECT_TRUE(x.update(std::make_unique<int>(1)));
  rcu_ptr<int> p1 = x.read();
  EXPECT_EQ(p1.use_count(), 2);

  EXPECT_TRUE(x.update(std::make_unique<int>(2)));
  rcu_ptr<int> p2 = x.read();
  EXPECT_EQ(p2.use_count(), 2);

  EXPECT_TRUE(x.update(std::make_unique<int>(3)));
  rcu_ptr<int> p3 = x.read();
  EXPECT_EQ(p3.use_count(), 2);

  EXPECT_TRUE(x.update(std::make_unique<int>(4)));
  rcu_ptr<int> p4 = x.read();
  EXPECT_EQ(p4.use_count(), 2);

  // Too many different `rcu_ptr` instances with different values.
  EXPECT_FALSE(x.update(std::make_unique<int>(5)));
  rcu_ptr<int> p5 = x.read();
  EXPECT_EQ(p4.use_count(), 3);
  EXPECT_EQ(p5.use_count(), 3);
  EXPECT_EQ(*p5, 4);
}

TEST(RcuUpdate, UpdateFullWithArray) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  // NOLINTBEGIN(cppcoreguidelines-pro-bounds-constant-array-index)
  std::array<rcu_ptr<int>, 5> p;
  for (int i = 0; i < 4; i++) {
    p[i] = x.read();
    EXPECT_EQ(p[i].use_count(), 2);
    EXPECT_EQ(*p[i], i);
    EXPECT_TRUE(x.update(std::make_unique<int>(i + 1)));
  }
  // NOLINTEND(cppcoreguidelines-pro-bounds-constant-array-index)

  // Too many different `rcu_ptr` instances with different values.
  EXPECT_FALSE(x.update(std::make_unique<int>(5)));
  p[4] = x.read();
  EXPECT_EQ(p[3].use_count(), 1); // Decremented because the update succeeded
  EXPECT_EQ(p[4].use_count(), 2); // Remains at 2 because the update failed
  EXPECT_EQ(*p[4], 4);

  rcu_ptr<int> p5 = x.read();
  EXPECT_EQ(p[4].use_count(), 3);
  EXPECT_EQ(p5.use_count(), 3);
  EXPECT_EQ(*p5, 4);
}

struct MyData {
  MyData(int value) : value_{value} { }

  int value_;
};

TEST(RcuPtr, CopyConstructor) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  rcu_ptr<int> p1 = x.read();
  EXPECT_EQ(p1.use_count(), 2);
  EXPECT_TRUE(p1);

  rcu_ptr<int> p1c(p1);
  EXPECT_EQ(p1.use_count(), 3);
  EXPECT_TRUE(p1);
  EXPECT_EQ(p1c.use_count(), 3);
  EXPECT_TRUE(p1c);
}

TEST(RcuPtr, CopyAssignment) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  rcu_ptr<int> p1 = x.read();
  EXPECT_EQ(p1.use_count(), 2);
  EXPECT_TRUE(p1);

  rcu_ptr<int> p1c = p1;
  EXPECT_EQ(p1.use_count(), 3);
  EXPECT_TRUE(p1);
  EXPECT_EQ(p1c.use_count(), 3);
  EXPECT_TRUE(p1c);
}

TEST(RcuPtr, MoveConstructor) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  rcu_ptr<int> p1 = x.read();
  EXPECT_EQ(p1.use_count(), 2);
  EXPECT_TRUE(p1);

  rcu_ptr<int> p1c(std::move(p1));
  // While a user shouldn't, the test shows the move worked.
  EXPECT_EQ(p1.use_count(), 0); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(p1);
  EXPECT_EQ(p1c.use_count(), 2);
  EXPECT_TRUE(p1c);
}

TEST(RcuPtr, MoveAssignment) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  rcu_ptr<int> p1 = x.read();
  EXPECT_EQ(p1.use_count(), 2);
  EXPECT_TRUE(p1);

  rcu_ptr<int> p1c = std::move(p1);
  // While a user shouldn't, the test shows the move worked.
  EXPECT_EQ(p1.use_count(), 0); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(p1);
  EXPECT_EQ(p1c.use_count(), 2);
  EXPECT_TRUE(p1c);
}

TEST(RcuPtr, CopyAssignmentToEmpty) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  rcu_ptr<int> p1 = x.read();
  EXPECT_EQ(p1.use_count(), 2);
  EXPECT_TRUE(p1);

  rcu_ptr<int> p1c;
  p1c = p1;
  EXPECT_EQ(p1.use_count(), 3);
  EXPECT_TRUE(p1);
  EXPECT_EQ(p1c.use_count(), 3);
  EXPECT_TRUE(p1c);
}

TEST(RcuPtr, MoveAssignmentToEmpty) {
  std::unique_ptr<int> v = std::make_unique<int>(0);
  rcu<int, 5> x{std::move(v)};

  rcu_ptr<int> p1 = x.read();
  EXPECT_EQ(p1.use_count(), 2);
  EXPECT_TRUE(p1);

  rcu_ptr<int> p1c;
  p1c = std::move(p1);
  // While a user shouldn't, the test shows the move worked.
  EXPECT_EQ(p1.use_count(), 0); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(p1);
  EXPECT_EQ(p1c.use_count(), 2);
  EXPECT_TRUE(p1c);
}

// Run with disabled tests. Disabled as it's a long running test.
//
//  rcutest-test --gtest_also_run_disabled_tests
//    --gtest_filter=RcuStress.DISABLED_ThreadedUpdate
TEST(RcuStress, DISABLED_ThreadedUpdate) {
  std::unique_ptr<MyData> data(std::make_unique<MyData>(42));
  rcu<MyData> rcu(std::move(data));

  std::atomic<bool> terminate{false};

  // Thread X: Read the data
  std::thread thread_x([&]() {
    while (!terminate.load()) {
      auto ptr = rcu.read();
    }
  });

  // Thread Y: Update the data
  std::thread thread_y([&]() {
    while (!terminate.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::unique_ptr<MyData> new_data(new MyData(24));
      bool r = rcu.update(std::move(new_data));
      if (!r) {
        std::cout << "Error updating" << std::endl;
        std::abort();
      }
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(15));
  terminate.store(true);
  thread_x.join();
  thread_y.join();
}

// Run with disabled tests. Disabled as it's a long running test.
//
//  rcutest-test --gtest_also_run_disabled_tests
//    --gtest_filter=RcuStress.DISABLED_ReadOps
TEST(RcuStress, DISABLED_ReadOps) {
  const unsigned int processor_count_ = std::thread::hardware_concurrency();
  const unsigned int processor_count = processor_count_ < 2 ? 2 : processor_count_;
  std::cout << "Running with " << processor_count << " threads" << std::endl;

  std::unique_ptr<MyData> data(std::make_unique<MyData>(42));
  rcu<MyData> rcu(std::move(data));

  std::atomic<bool> terminate{false};
  std::atomic<std::uint32_t> counter{0};
  std::atomic<std::uint32_t> updates{0};

  std::thread thread_update([&]() {
    while (!terminate.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::unique_ptr<MyData> new_data(new MyData(24));
      bool r = rcu.update(std::move(new_data));
      if (!r) {
        std::cout << "Error updating" << std::endl;
        std::abort();
      }
      updates++;
    }
  });

  std::vector<std::thread> reads;
  for (unsigned int t = 0; t < processor_count - 1; t++) {
    reads.emplace_back(std::thread([&]() {
      while (!terminate.load()) {
        auto ptr = rcu.read();
        counter++;
      }
    }));
  }

  std::this_thread::sleep_for(std::chrono::seconds(30));
  terminate.store(true);
  thread_update.join();
  for (std::thread& r : reads) {
    r.join();
  }

  std::cout << " Updates: " << updates << std::endl;
  std::cout << " Reads: " << counter << std::endl;
  std::cout << " Reads/core/sec: " << counter / (processor_count - 1) / 30 << std::endl;
}
