#include <iostream>

#include "rcu.h"

auto test1() {
  rcu<int> x{std::make_unique<int>(10)};

  // Shows 2 references because `x` has one, then `p`.
  std::cout << "p = x.read()" << std::endl;
  rcu_ptr<int> p = x.read();
  std::cout << "Value p = " << *p.get() << "; References = " << p.use_count()
            << std::endl;

  // Shows 3 references because `x` has one, then `p`, `q`.
  std::cout << "q = x.read()" << std::endl;
  rcu_ptr<int> q = x.read();
  std::cout << "Value p = " << *p.get() << "; References = " << p.use_count()
            << std::endl;
  std::cout << "Value q = " << *q.get() << "; References = " << q.use_count()
            << std::endl;

  // Shows 2 references because `x` has one, then `q`. Variable `p` is
  // destroyed.
  std::cout << "p.~rcu_ptr()" << std::endl;
  p.~rcu_ptr();
  std::cout << "Value q = " << *q.get() << "; References = " << q.use_count()
            << std::endl;

  // Now the owner is destroyed.
  std::cout << "x.~rcu()" << std::endl;
  x.~rcu();
  std::cout << "Value q = " << *q.get() << "; References = " << q.use_count()
            << std::endl;

  std::cout << "q.~rcu_ptr()" << std::endl;
  q.~rcu_ptr();
}

auto test2() {
  rcu<int> x{std::make_unique<int>(10)};

  // Shows 2 references because `x` has one, then `p`.
  std::cout << "p = x.read()" << std::endl;
  rcu_ptr<int> p = x.read();
  std::cout << "Value p = " << *p.get() << "; References = " << p.use_count()
            << std::endl;

  // Now update with a new value
  std::cout << "x.update(20)" << std::endl;
  x.update(std::make_unique<int>(20));
  // Should still show 2.
  std::cout << "Value p = " << *p.get() << "; References = " << p.use_count()
            << std::endl;

  // Shows 3 references because `x` has one, then `p`, `q`.
  std::cout << "q = x.read()" << std::endl;
  rcu_ptr<int> q = x.read();
  std::cout << "Value p = " << *p.get() << "; References = " << p.use_count()
            << std::endl;
  std::cout << "Value q = " << *q.get() << "; References = " << q.use_count()
            << std::endl;

  // Shows 2 references because `x` has one, then `q`. Variable `p` is
  // destroyed.
  std::cout << "p.~rcu_ptr()" << std::endl;
  p.~rcu_ptr();
  std::cout << "Value q = " << *q.get() << "; References = " << q.use_count()
            << std::endl;
}

auto main() -> int {
  test1();
  test2();
  return 0;
}
