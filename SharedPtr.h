#include <iostream>

namespace Utilities {

template <typename T>
struct AlignedBuffer {
  alignas(T) std::byte storage[sizeof(T)];
  T* pointer() { return reinterpret_cast<T*>(storage); }
  const T* pointer() const { return reinterpret_cast<const T*>(storage); }

  template <typename... Args>
  void construct(Args&&... args) {
    ::new (pointer()) T(std::forward<Args>(args)...);
  }
  void destroy() { pointer()->~T(); }
};

}  // namespace Utilities

struct BaseControlBlock {
  std::size_t use_count{1}, weak_count{1};
  virtual ~BaseControlBlock() = default;
  virtual void dispose() = 0;
  virtual void destroy() = 0;
};

template <typename T, typename Deleter, typename Allocator>
struct ControlBlockRegular : BaseControlBlock, Deleter, Allocator {
  T* ptr;

  ControlBlockRegular(T* p, Deleter d, Allocator a)
      : Deleter(std::move(d)), Allocator(std::move(a)), ptr(p) {}

  void dispose() override {
    Deleter& del = *this;
    del(ptr);
  }
  void destroy() override {
    using Rebind = typename std::allocator_traits<
        Allocator>::template rebind_alloc<ControlBlockRegular>;
    Rebind alloc(*this);
    this->~ControlBlockRegular();
    std::allocator_traits<Rebind>::deallocate(alloc, this, 1);
  }
};

template <typename T, typename Allocator>
struct CountedStorage : BaseControlBlock, Allocator {
  Utilities::AlignedBuffer<T> buffer;

  explicit CountedStorage(Allocator a) : Allocator(std::move(a)) {}

  void dispose() override {
    Allocator& a = *this;
    std::allocator_traits<Allocator>::destroy(a, buffer.pointer());
  }
  void destroy() override {
    using Rebind = typename std::allocator_traits<
        Allocator>::template rebind_alloc<CountedStorage>;
    Rebind alloc(*this);
    this->~CountedStorage();
    std::allocator_traits<Rebind>::deallocate(alloc, this, 1);
  }
};

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
 public:
  SharedPtr() noexcept = default;
  SharedPtr(std::nullptr_t) noexcept : ptr_(nullptr), cb_(nullptr) {}

  explicit SharedPtr(T* p, BaseControlBlock* cb) noexcept : ptr_(p), cb_(cb) {}

  template <typename U>
  explicit SharedPtr(U* p) : SharedPtr(p, std::default_delete<U>()) {
    initEnableShared(*this, p);
  }

  template <typename U, typename D>
  SharedPtr(U* p, D d) : SharedPtr(p, std::move(d), std::allocator<U>()) {
    initEnableShared(*this, p);
  }

  template <typename U, typename D, typename A>
  SharedPtr(U* p, D d, A a) : ptr_(p) {
    using CB = ControlBlockRegular<U, D, A>;
    using Re = typename std::allocator_traits<A>::template rebind_alloc<CB>;
    Re alloc(a);
    CB* c = std::allocator_traits<Re>::allocate(alloc, 1);
    try {
      ::new (static_cast<void*>(c)) CB(p, std::move(d), std::move(a));
    } catch (...) {
      std::allocator_traits<Re>::deallocate(alloc, c, 1);
      d(p);
      throw;
    }
    cb_ = c;
    initEnableShared(*this, p);
  }

  ~SharedPtr() {
    if (!cb_) return;
    if (--cb_->use_count == 0) {
      cb_->dispose();
      if (--cb_->weak_count == 0) {
        cb_->destroy();
      }
    }
  }

  SharedPtr(SharedPtr&& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
    o.ptr_ = nullptr;
    o.cb_ = nullptr;
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
    o.ptr_ = nullptr;
    o.cb_ = nullptr;
  }

  SharedPtr& operator=(SharedPtr&& o) noexcept {
    SharedPtr(std::move(o)).swap(*this);
    return *this;
  }

  template <typename U>
  SharedPtr& operator=(SharedPtr<U>&& o) noexcept {
    SharedPtr(std::move(o)).swap(*this);
    return *this;
  }

  SharedPtr(const SharedPtr& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
    if (cb_) ++cb_->use_count;
  }

  template <typename U>
  SharedPtr(const SharedPtr<U>& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
    if (cb_) ++cb_->use_count;
  }

  template <typename U>
  explicit SharedPtr(const WeakPtr<U>& w) : ptr_(w.ptr_), cb_(w.cb_) {
    if (cb_ && cb_->use_count > 0) {
      ++cb_->use_count;
    } else {
      ptr_ = nullptr;
      cb_ = nullptr;
    }
  }

  SharedPtr& operator=(const SharedPtr& o) noexcept {
    SharedPtr(o).swap(*this);
    return *this;
  }

  template <typename U>
  SharedPtr& operator=(const SharedPtr<U>& o) noexcept {
    SharedPtr(o).swap(*this);
    return *this;
  }

  T* get() const noexcept { return ptr_; }
  std::size_t use_count() const noexcept { return cb_ ? cb_->use_count : 0; }
  T& operator*() const { return *ptr_; }
  T* operator->() const noexcept { return ptr_; }

  void swap(SharedPtr& o) noexcept {
    std::swap(ptr_, o.ptr_);
    std::swap(cb_, o.cb_);
  }
  void reset() noexcept { SharedPtr().swap(*this); }
  template <typename U>
  void reset(U* p) {
    SharedPtr(p).swap(*this);
  }

 private:
  template <typename U>
  friend void initEnableShared(SharedPtr<U>&, U*);
  template <typename U>
  friend class SharedPtr;
  template <typename U>
  friend class WeakPtr;

  T* ptr_{nullptr};
  BaseControlBlock* cb_{nullptr};
};

template <typename T, typename A, typename... Args>
SharedPtr<T> allocateShared(const A& a, Args&&... args) {
  using S = CountedStorage<T, A>;
  using ReS = typename std::allocator_traits<A>::template rebind_alloc<S>;
  ReS alloc_s(a);
  S* s = std::allocator_traits<ReS>::allocate(alloc_s, 1);
  try {
    ::new (static_cast<void*>(s)) S(a);
  } catch (...) {
    std::allocator_traits<ReS>::deallocate(alloc_s, s, 1);
    throw;
  }
  using ReT = typename std::allocator_traits<A>::template rebind_alloc<T>;
  ReT alloc_t(a);
  std::allocator_traits<ReT>::construct(alloc_t, s->buffer.pointer(),
                                        std::forward<Args>(args)...);
  return SharedPtr<T>(s->buffer.pointer(), static_cast<BaseControlBlock*>(s));
}

template <typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
  return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

template <typename T>
class WeakPtr {
 public:
  WeakPtr() noexcept = default;

  WeakPtr(const WeakPtr& o) noexcept { copy_from(o); }
  template <typename U>
  WeakPtr(const WeakPtr<U>& o) noexcept {
    copy_from(o);
  }
  template <typename U>
  WeakPtr(const SharedPtr<U>& o) noexcept {
    copy_from(o);
  }

  WeakPtr(WeakPtr&& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
    o.ptr_ = nullptr;
    o.cb_ = nullptr;
  }
  template <typename U>
  WeakPtr(WeakPtr<U>&& o) noexcept : ptr_(o.ptr_), cb_(o.cb_) {
    o.ptr_ = nullptr;
    o.cb_ = nullptr;
  }

  ~WeakPtr() {
    if (cb_) {
      if (--cb_->weak_count == 0 && cb_->use_count == 0) {
        cb_->destroy();
      }
    }
  }

  WeakPtr& operator=(const WeakPtr& o) noexcept {
    WeakPtr(o).swap(*this);
    return *this;
  }
  template <typename U>
  WeakPtr& operator=(const WeakPtr<U>& o) noexcept {
    WeakPtr(o).swap(*this);
    return *this;
  }
  template <typename U>
  WeakPtr& operator=(const SharedPtr<U>& o) noexcept {
    WeakPtr(o).swap(*this);
    return *this;
  }
  WeakPtr& operator=(WeakPtr&& o) noexcept {
    if (this != &o) {
      reset();
      ptr_ = o.ptr_;
      cb_ = o.cb_;
      o.ptr_ = nullptr;
      o.cb_ = nullptr;
    }
    return *this;
  }
  template <typename U>
  WeakPtr& operator=(WeakPtr<U>&& o) noexcept {
    reset();
    ptr_ = o.ptr_;
    cb_ = o.cb_;
    o.ptr_ = nullptr;
    o.cb_ = nullptr;
    return *this;
  }

  bool expired() const noexcept { return use_count() == 0; }
  std::size_t use_count() const noexcept { return cb_ ? cb_->use_count : 0; }
  SharedPtr<T> lock() const noexcept {
    return expired() ? SharedPtr<T>() : SharedPtr<T>(*this);
  }

  void reset() noexcept {
    if (cb_) {
      if (--cb_->weak_count == 0 && cb_->use_count == 0) {
        cb_->destroy();
      }
      ptr_ = nullptr;
      cb_ = nullptr;
    }
  }
  void swap(WeakPtr& o) noexcept {
    std::swap(ptr_, o.ptr_);
    std::swap(cb_, o.cb_);
  }

 private:
  template <typename U>
  void copy_from(const SharedPtr<U>& u) noexcept {
    ptr_ = u.ptr_;
    cb_ = u.cb_;
    if (cb_) ++cb_->weak_count;
  }
  template <typename U>
  void copy_from(const WeakPtr<U>& u) noexcept {
    ptr_ = u.ptr_;
    cb_ = u.cb_;
    if (cb_) ++cb_->weak_count;
  }

  template <typename U>
  friend class SharedPtr;
  template <typename U>
  friend class WeakPtr;

  T* ptr_{nullptr};
  BaseControlBlock* cb_{nullptr};
};

template <typename T>
class EnableSharedFromThis {
 protected:
  EnableSharedFromThis() noexcept = default;
  ~EnableSharedFromThis() = default;
  EnableSharedFromThis(const EnableSharedFromThis&) noexcept = default;
  EnableSharedFromThis& operator=(const EnableSharedFromThis&) noexcept =
      default;

 public:
  SharedPtr<T> shared_from_this() { return SharedPtr<T>(weak_this_); }
  SharedPtr<const T> shared_from_this() const {
    return SharedPtr<const T>(weak_this_);
  }

 private:
  template <typename U>
  friend void initEnableShared(SharedPtr<U>&, U*);
  WeakPtr<T> weak_this_;
};

template <typename T>
void initEnableShared(SharedPtr<T>& sp, T* p) {
  if constexpr (std::is_base_of_v<EnableSharedFromThis<T>, T>) {
    p->weak_this_ = sp;
  }
}
