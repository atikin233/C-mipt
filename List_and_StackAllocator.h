#include <iostream>

template <size_t N>
struct StackStorage {
  std::byte buffer[N];
  size_t top;

  StackStorage() : top(0) {}
  StackStorage(const StackStorage&) = delete;
  StackStorage& operator=(const StackStorage&) = delete;
};

template <typename T, size_t N>
class StackAllocator {
 public:
  using value_type = T;
  template <typename U>
  struct rebind {
    using other = StackAllocator<U, N>;
  };

  explicit StackAllocator(StackStorage<N>& s) noexcept : st(&s) {}
  template <typename U>
  StackAllocator(const StackAllocator<U, N>& o) noexcept : st(o.st) {}

  T* allocate(size_t count) {
    size_t alignment = alignof(T);
    size_t cur = st->top;
    if (size_t m = cur % alignment) cur += alignment - m;
    size_t need = sizeof(T) * count;
    if (cur + need > N) throw std::bad_alloc();
    T* ptr = reinterpret_cast<T*>(st->buffer + cur);
    st->top = cur + need;
    return ptr;
  }
  void deallocate(T*, size_t) noexcept {}

  template <typename U>
  bool operator==(const StackAllocator<U, N>& other) const noexcept {
    return st == other.st;
  }
  template <typename U>
  bool operator!=(const StackAllocator<U, N>& other) const noexcept {
    return st != other.st;
  }

  bool operator==(const StackAllocator& other) const noexcept {
    return st == other.st;
  }
  bool operator!=(const StackAllocator& other) const noexcept {
    return st != other.st;
  }

 private:
  StackStorage<N>* st;
  template <typename, size_t>
  friend class StackAllocator;
};

template <typename T, typename A = std::allocator<T>>
class List {
  struct BaseNode {
    BaseNode* prev;
    BaseNode* next;
    BaseNode() : prev(this), next(this) {}
  };
  struct Node : BaseNode {
    T value;
    template <typename... Args>
    Node(Args&&... args) : value(std::forward<Args>(args)...) {}
  };

  List(const List& o, const A& al) : head_(), size_(0), alloc_(al), na_(al) {
    try {
      for (auto it = o.begin(); it != o.end(); ++it) emplace(end(), *it);
    } catch (...) {
      clear();
      throw;
    }
  }

  using Traits = std::allocator_traits<A>;
  using NodeAlloc = typename Traits::template rebind_alloc<Node>;
  using NTraits = std::allocator_traits<NodeAlloc>;

 public:
  template <bool Const>
  class Iterator {
    BaseNode* n_;
    explicit Iterator(BaseNode* p) : n_(p) {}
    friend class List;

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using reference = std::conditional_t<Const, const T&, T&>;
    using pointer = std::conditional_t<Const, const T*, T*>;

    Iterator() = default;
    template <bool O, typename = std::enable_if_t<Const || !O>>
    Iterator(const Iterator<O>& o) noexcept : n_(o.n_) {}

    reference operator*() const { return static_cast<Node*>(n_)->value; }
    pointer operator->() const { return std::addressof(**this); }

    Iterator& operator++() {
      n_ = n_->next;
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    Iterator& operator--() {
      n_ = n_->prev;
      return *this;
    }
    Iterator operator--(int) {
      Iterator tmp = *this;
      --*this;
      return tmp;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) {
      return a.n_ == b.n_;
    }
    friend bool operator!=(const Iterator& a, const Iterator& b) {
      return !(a == b);
    }
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  List() noexcept(noexcept(A())) : head_(), size_(0), alloc_(), na_(alloc_) {}

  explicit List(const A& a) noexcept
      : head_(), size_(0), alloc_(a), na_(alloc_) {}

  template <typename U = T,
            typename = std::enable_if_t<std::is_default_constructible_v<U>>>
  List(size_t n, const A& a = A()) : List(a) {
    for (size_t i = 0; i < n; ++i) emplace(end());
  }

  List(size_t n, const T& v, const A& a = A()) : List(a) {
    for (size_t i = 0; i < n; ++i) emplace(end(), v);
  }

  List(const List& o)
      : List(o, Traits::select_on_container_copy_construction(o.alloc_)) {}

  List& operator=(const List& o) {
    if (this == &o) return *this;

    if constexpr (Traits::propagate_on_container_copy_assignment::value) {
      List tmp(o, o.alloc_);
      swap_nodes(tmp);
      alloc_ = o.alloc_;
      na_ = alloc_;
    } else {
      if (alloc_ == o.alloc_) {
        List tmp(o);
        swap_nodes(tmp);
      } else {
        List tmp(alloc_);
        for (const auto& item : o) {
          tmp.push_back(item);
        }
        swap_nodes(tmp);
      }
    }
    return *this;
  }

  ~List() { clear(); }

  A get_allocator() const noexcept { return alloc_; }

  bool empty() const noexcept { return size_ == 0; }
  size_t size() const noexcept { return size_; }

  iterator begin() noexcept { return iterator(head_.next); }
  const_iterator begin() const noexcept { return const_iterator(head_.next); }
  iterator end() noexcept { return iterator(&head_); }
  const_iterator end() const noexcept {
    return const_iterator(const_cast<BaseNode*>(&head_));
  }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  const_reverse_iterator crend() const noexcept { return rend(); }

  void push_back(const T& v) { emplace(end(), v); }
  void push_front(const T& v) { emplace(begin(), v); }
  void pop_back() { erase(--end()); }
  void pop_front() { erase(begin()); }

  iterator insert(const_iterator pos, const T& v) { return emplace(pos, v); }

  iterator erase(const_iterator pos) {
    BaseNode* n = pos.n_;
    BaseNode* p = n->prev;
    BaseNode* s = n->next;
    p->next = s;
    s->prev = p;
    Node* nd = static_cast<Node*>(n);
    NTraits::destroy(na_, nd);
    NTraits::deallocate(na_, nd, 1);
    --size_;
    return iterator(s);
  }

 private:
  BaseNode head_;
  size_t size_;
  A alloc_;
  NodeAlloc na_;

  template <typename... Args>
  iterator emplace(const_iterator pos, Args&&... args) {
    BaseNode* s = pos.n_;
    BaseNode* p = s->prev;
    Node* n = NTraits::allocate(na_, 1);
    try {
      NTraits::construct(na_, n, std::forward<Args>(args)...);
    } catch (...) {
      NTraits::deallocate(na_, n, 1);
      throw;
    }
    n->prev = p;
    n->next = s;
    p->next = n;
    s->prev = n;
    ++size_;
    return iterator(n);
  }

  void clear() {
    while (size_) pop_front();
  }

  void swap_nodes(List& other) noexcept {
    using std::swap;
    swap(head_.next, other.head_.next);
    head_.next->prev = &head_;
    other.head_.next->prev = &other.head_;
    swap(head_.prev, other.head_.prev);
    head_.prev->next = &head_;
    other.head_.prev->next = &other.head_;
    swap(size_, other.size_);
    swap(alloc_, other.alloc_);
    swap(na_, other.na_);
  }
};
