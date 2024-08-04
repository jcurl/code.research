#include <array>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

#ifndef RCU_H
#define RCU_H

template <typename T, std::size_t N>
class rcu;

template <typename T>
class rcu_ptr {
 public:
  rcu_ptr() {}

  rcu_ptr(const rcu_ptr &obj) : refcount_{obj.refcount_} {
    if (this->refcount_) {
      (*this->refcount_)++;
      this->ptr_ = obj.ptr_;
    }
  }

  auto operator=(const rcu_ptr &obj) -> rcu_ptr & {
    free();

    this->refcount_ = obj.refcount_;
    if (this->refcount_) {
      (*this->refcount_)++;
    }
    this->ptr_ = this->refcount_ ? obj.ptr_ : nullptr;
    return *this;
  }

  rcu_ptr(rcu_ptr &&rval) : refcount_{rval.refcount_} {
    rval.refcount_ = nullptr;
    this->ptr_ = this->refcount_ ? rval.ptr_ : nullptr;
    rval.ptr_ = nullptr;
  }

  auto operator=(rcu_ptr &&rval) -> rcu_ptr & {
    auto refcount = rval.refcount_;
    rval.refcount_ = nullptr;
    free();

    this->refcount_ = refcount;
    this->ptr_ = refcount ? rval.ptr_ : nullptr;
    rval.ptr_ = nullptr;
    return *this;
  }

  auto use_count() const -> std::uint32_t {
    return this->refcount_ ? this->refcount_->load() : 0;
  }

  auto get() const -> const T * { return this->ptr_; }

  auto operator->() const -> const T * { return this->ptr_; }

  auto operator*() const -> const T & { return *this->ptr_; }

  explicit operator bool() const { return this->ptr_ != nullptr; }

  ~rcu_ptr() { free(); }

 private:
  // Only the `rcu` class may instantiate a `rcu_ptr` with internal details.
  template <typename U, std::size_t N>
  friend class rcu;

  const T *ptr_{nullptr};
  std::atomic<std::uint32_t> *refcount_{nullptr};

  rcu_ptr(const T *ptr, std::atomic<std::uint32_t> *ref)
      : ptr_{ptr}, refcount_{ptr ? ref : nullptr} {}

  auto free() -> void {
    auto refcount = this->refcount_;
    if (!refcount) return;

    auto prev_count = refcount->fetch_sub(1);
    if (prev_count == 1) {
      // No more references. As this was the last reference, it can not be
      // incremented.
      delete this->ptr_;
    }
#if !defined(NDEBUG)
    else if (prev_count == 0) {
      // Freed a reference counter that was already free
      std::cout << "rcu_ptr.free() free when ref=0" << std::endl;
      std::abort();
    }
#endif

    // Only needed if we expect someone might use the object after it's been
    // destructed.
    this->refcount_ = nullptr;
    this->ptr_ = nullptr;
  }
};

template <typename T, std::size_t N = 10>
class rcu {
 public:
  rcu() = delete;

  rcu(const rcu &rhs) = delete;
  auto operator=(const rcu &rhs) -> rcu & = delete;
  rcu(rcu &&) = delete;
  auto operator=(rcu &&) -> rcu & = delete;

  rcu(std::unique_ptr<const T> &&ptr) {
    if (!ptr) std::abort();

    this->index_.store(0);
    this->slot_[0].refcount_.store(1);
    this->slot_[0].ptr_ = ptr.release();
  }

  auto read() -> rcu_ptr<T> {
    // Find the index, and atomically increment the reference for the active
    // index.
    std::uint32_t i;
    std::uint32_t rc_start;
    std::uint32_t rc;
    do {
      i = this->index_.load();
      rc_start = this->slot_[i].refcount_.load();

      // If `update()` is called so that it has freed the slot, then it has
      // already updated the index since we've read it. Need to get the index
      // again and repeat. We never want to return a 'freed' slot.
      //
      // We can't just "increment" the reference because there are two load
      // operations needed: the index; and then the reference count.

      rc = rc_start + 1;

      // Check if the current reference ever made it to zero. Possible if the
      // `update()` thread freed the slot (in which case, it updated the index
      // before decrementing the reference counter). This is also why we can
      // never allow a slot to be initialised with a `nullptr` (why update and
      // the constructors never allow a `nullptr` and hence ensure we always
      // construct with a `refcount_` of 1). Else, we could not tell the
      // difference between an intended `nullptr` with a reference count of 0,
      // and one that was just freed during the `update()`. On the next
      // iteration we'll see that `index_` is now new, and get the different
      // reference counter value.

      // The slot could still be released, going from 1 to 0. So increment it
      // only if it hasn't changed. If the reference changed, get the current
      // index again and try the increment. Note, we'll repeat if the reference
      // was freed or incremented even if not freed, we can't tell.
    } while (rc_start == 0 ||
             !this->slot_[i].refcount_.compare_exchange_strong(rc_start, rc));

    // This `rcu` class own the reference counter, so this class must always
    // live longer than all existing `rcu_ptr` objects. If this class dies, then
    // the `rcu_ptr` will point to a `refcount_` that no longer exists in
    // memory, leading to undefined behaviour.
    return rcu_ptr<T>(this->slot_[i].ptr_, &this->slot_[i].refcount_);
  }

  auto update(std::unique_ptr<const T> &&update) -> bool {
    // Don't allow updates to `nullptr`.
    if (!update) return false;

    std::lock_guard<std::mutex> lock(this->write_mutex_);

    // Allocate a new position for the pointer and reference counter.
    std::uint32_t i_start = this->index_.load();
    std::uint32_t i = i_start;
    std::uint32_t rc;
    do {
      i = (i + 1) % N;
      if (i == i_start) return false;

      rc = this->slot_[i].refcount_.load();
    } while (rc);

    // Index 'i' is an unallocated slot, it can't increment and no one should
    // have a reference (if they do, then they shouldn't do anything with it
    // because they already freed it). This is the only place we update the
    // slot, and it can't happen in parallel with this function due to the lock.
    this->slot_[i].refcount_.store(1);
    this->slot_[i].ptr_ = update.release();
    index_.store(i);

    // Decrement the previous reference. If it reaches zero, then no-one else
    // has a copy and we can free the memory.
    free(i_start);
    return true;
  }

  ~rcu() {
    // Debug checks to ensure that the user doesn't leave any `rcu_ptr`
    // references alive.
#if !defined(NDEBUG)
    bool success = true;
#endif

    if (!free(this->index_.load())) {
#if !defined(NDEBUG)
      std::cout
          << "Abort due to not final free, dangling reference of an `rcu_ptr`"
          << std::endl;
      success = false;
#else
      std::abort();
#endif
    } else {
      for (std::uint32_t i = 0; i < N; i++) {
        if (this->slot_[i].refcount_.load() != 0) {
#if !defined(NDEBUG)
          std::cout << "Abort due to not final free, dangling reference of an "
                       "`rcu_ptr`"
                    << std::endl;
          success = false;
          break;
#else
          std::abort();
#endif
        }
      }
    }

#if !defined(NDEBUG)
    if (!success) {
      std::cout << " Owning: " << this->index_ << std::endl;
      for (std::uint32_t x = 0; x < N; x++) {
        std::cout << " slot[" << x
                  << "].refcount_ = " << this->slot_[x].refcount_.load()
                  << std::endl;
      }
      std::abort();
    }
#endif
  }

 private:
  std::mutex write_mutex_;

  auto free(std::uint32_t index) -> bool {
    std::uint32_t old_count = this->slot_[index].refcount_.fetch_sub(1);
    if (old_count == 1) {
      // No more references. As this was the last reference (it has reached
      // zero), it can not be incremented.
      auto ptr = this->slot_[index].ptr_;
      delete ptr;
      return true;
    }
#if !defined(NDEBUG)
    else if (old_count == 0) {
      // Double-free internally. Should never occur.
      std::cout << "Abort due to double free" << std::endl;
      std::abort();
    }
#endif
    return false;
  }

  class ref {
   public:
    std::atomic<std::uint32_t> refcount_{0};
    const T *ptr_{nullptr};
  };

  std::atomic<std::uint32_t> index_{0};
  std::array<ref, N> slot_{};
};

#endif  // RCU_H