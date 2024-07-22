#include <atomic>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>

template <typename T>
class rcu_ptr {
 public:
  rcu_ptr() : ptr_(nullptr), refs_(nullptr) {}
  rcu_ptr(const T *ptr) : ptr_(ptr), refs_(new std::atomic<std::uint32_t>(1)) {}

  rcu_ptr(const rcu_ptr &obj) {
    this->ptr_ = obj.ptr_;

    auto refs = obj.refs_.load();
    this->refs_.store(refs);
    (*refs)++;
  }

  rcu_ptr &operator=(const rcu_ptr &obj) {
    free();
    this->ptr_ = obj.ptr_;

    auto refs = obj.refs_.load();
    this->refs_.store(refs);
    (*refs_)++;
    return *this;
  }

  rcu_ptr(rcu_ptr &&rval) {
    this->ptr_ = rval.ptr_;
    rval.ptr_ = nullptr;

    auto refs = rval.refs_.load();
    rval.refs_.store(nullptr);
    this->refs_.store(refs);
  }

  rcu_ptr &operator=(rcu_ptr &&rval) {
    free();
    this->ptr_ = rval.ptr_;
    rval.ptr_ = nullptr;

    auto refs = rval.refs_.load();
    rval.refs_.store(nullptr);
    this->refs_.store(refs);
    return *this;
  }

  std::uint32_t use_count() const {
    auto refs = this->refs_.load();
    if (refs == nullptr) return 0;
    return *refs;
  }

  const T *get() const { return this->ptr_; }

  const T *operator->() const { return this->ptr_; }

  const T &operator*() const { return *this->ptr_; }

  explicit operator bool() const { return this->ptr_ != nullptr; }

  ~rcu_ptr() { free(); }

 private:
  const T *ptr_ = nullptr;
  std::atomic<std::atomic<std::uint32_t> *> refs_{nullptr};

  void free() {
    auto refs = this->refs_.load();
    if (refs == nullptr) return;

    std::uint32_t ref = refs->fetch_sub(1);
    if (ref == 1) {
      if (this->ptr_ != nullptr) {
        delete this->ptr_;
      }
      delete refs;
    }

    this->ptr_ = nullptr;
    this->refs_.store(nullptr);
  }
};

template <typename T>
class rcu {
 public:
  rcu() = default;

  rcu(const rcu &rhs) = delete;
  rcu &operator=(const rcu &rhs) = delete;
  rcu(rcu &&) = delete;
  rcu &operator=(rcu &&) = delete;

  rcu(std::unique_ptr<const T> &&ptr) : node_(new rcu_ptr<T>(ptr.release())) {}

  rcu_ptr<T> read() const {
    while (true) {
      auto node = this->node_.load();
      if (node == nullptr) return rcu_ptr<T>();

      // In case the `update` swapped the pointer, we need to do it again,
      // because it will be (if not already) freed and result in an invalid
      // memory access.
      auto new_node = rcu_ptr<T>(*node);
      if (this->node_.load() == node)
        return new_node;
    }
  }

  void update(std::unique_ptr<const T> &&update) {
    //std::lock_guard<std::mutex> lock(this->write_mutex_);

    rcu_ptr<T> *rptr = new rcu_ptr<T>(update.release());
    rcu_ptr<T> *old = this->node_.exchange(rptr);
    delete old;
  }

  ~rcu() {
    rcu_ptr<T> *old = this->node_.exchange(nullptr);
    if (old != nullptr) {
      delete old;
    }
  }

 private:
  //std::mutex write_mutex_;
  std::atomic<rcu_ptr<T> *> node_{nullptr};
};
