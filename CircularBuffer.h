#include <algorithm>
#include <cassert>
#include <limits>
#include <ranges>
#include <vector>

static constexpr std::size_t DYNAMIC_CAPACITY =
    std::numeric_limits<std::size_t>::max();

template <typename U, std::size_t StaticCap>
class BufferStorage {
 public:
  BufferStorage() = default;
  explicit BufferStorage(std::size_t cap) {
    if (cap != StaticCap) throw std::invalid_argument("capacity mismatch");
  }
  U* raw_data() noexcept { return reinterpret_cast<U*>(storage_.data()); }

  const U* raw_data() const noexcept { return reinterpret_cast<const U*>(storage_.data()); }

  std::size_t capacity() const noexcept { return StaticCap; }

  void swap(BufferStorage& other) noexcept { storage_.swap(other.storage_); }

 protected:
  class IterBase {
   protected:
    IterBase() = default;
    explicit IterBase(std::size_t) {}
    std::size_t cap() const noexcept { return StaticCap; }
  };

 private:
  alignas(U) std::array<std::byte, (StaticCap + 1) * sizeof(U)> storage_{};
};

template <typename U>
class BufferStorage<U, DYNAMIC_CAPACITY> {
 public:
  BufferStorage() = delete;
  explicit BufferStorage(std::size_t cap)
      : data_ptr_(static_cast<U*>(operator new[]((cap + 1) * sizeof(U)))),
        dyn_cap_(cap) {
    if (cap == 0) throw std::invalid_argument("capacity must be positive");
  }
  BufferStorage(const BufferStorage& other) : BufferStorage(other.dyn_cap_) {}
  BufferStorage(BufferStorage&& other) noexcept
      : data_ptr_(other.data_ptr_), dyn_cap_(other.dyn_cap_) {
    other.data_ptr_ = nullptr;
    other.dyn_cap_ = 0;
  }
  ~BufferStorage() {
    if (data_ptr_) operator delete[](data_ptr_);
  }

  U* raw_data() noexcept { return data_ptr_; }
  const U* raw_data() const noexcept { return data_ptr_; }
  std::size_t capacity() const noexcept { return dyn_cap_; }
  void swap(BufferStorage& other) noexcept {
    std::swap(data_ptr_, other.data_ptr_);
    std::swap(dyn_cap_, other.dyn_cap_);
  }

 protected:
  class IterBase {
   protected:
    IterBase() = default;

    explicit IterBase(std::size_t cap) : cap_(cap) {}

    std::size_t cap() const noexcept { return cap_; }

   private:
    std::size_t cap_;
  };

 private:
  U* data_ptr_ = nullptr;
  std::size_t dyn_cap_ = 0;
};

template <typename T, std::size_t Capacity = DYNAMIC_CAPACITY>
class CircularBuffer : public BufferStorage<T, Capacity> {
 public:
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  template <bool IsConst>
  class Iterator;
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  using Base = BufferStorage<T, Capacity>;
  using Base::capacity;
  using Base::raw_data;

  CircularBuffer() = default;

  explicit CircularBuffer(size_type cap) : Base(cap) {}

  CircularBuffer(const CircularBuffer& other)
      : Base(other), size_(other.size_), offset_(other.offset_) {
    for (size_type i = 0; i < size_; ++i)
      new (raw_data() + compute_index(i)) T(other[i]);
  }

  CircularBuffer(CircularBuffer&& other) noexcept
      : Base(std::move(other)), size_(other.size_), offset_(other.offset_) {
    other.size_ = 0;
    other.offset_ = 0;
  }

  CircularBuffer& operator=(const CircularBuffer& other) {
    if (this != &other) {
      if constexpr (Capacity == DYNAMIC_CAPACITY) {
        CircularBuffer tmp(other);
        swap(tmp);
      } else {
        for (size_type i = 0; i < size_; ++i) destroy_at(compute_index(i));
        offset_ = other.offset_;
        size_ = other.size_;
        for (size_type i = 0; i < size_; ++i)
          new (raw_data() + compute_index(i)) T(other[i]);
      }
    }
    return *this;
  }

  CircularBuffer& operator=(CircularBuffer&& other) noexcept {
    if (this != &other) {
      Base::operator=(std::move(other));
      size_ = other.size_;
      offset_ = other.offset_;
      other.size_ = 0;
      other.offset_ = 0;
    }
    return *this;
  }

  ~CircularBuffer() {
    for (size_type i = 0; i < size_; ++i) destroy_at(compute_index(i));
  }

  size_type size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }
  bool full() const noexcept { return size_ == capacity(); }

  T& at(size_type idx) {
    if (idx >= size_) throw std::out_of_range("index out of range");
    return (*this)[idx];
  }

  const T& at(size_type idx) const {
    if (idx >= size_) throw std::out_of_range("index out of range");
    return (*this)[idx];
  }

  T& operator[](size_type idx) {
    assert(idx < size_);
    return *(raw_data() + compute_index(idx));
  }

  const T& operator[](size_type idx) const {
    assert(idx < size_);
    return *(raw_data() + compute_index(idx));
  }

  iterator begin() { return iterator(raw_data(), offset_, capacity()); }

  iterator end() { return iterator(raw_data(), offset_ + size_, capacity()); }

  const_iterator begin() const {
    return const_iterator(raw_data(), offset_, capacity());
  }

  const_iterator end() const {
    return const_iterator(raw_data(), offset_ + size_, capacity());
  }

  const_iterator cbegin() const { return begin(); }

  const_iterator cend() const { return end(); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

  const_reverse_iterator crbegin() const { return rbegin(); }

  const_reverse_iterator crend() const { return rend(); }

  void push_back(const T& value) {
    if (full()) {
      new (raw_data() + compute_index(size_)) T(value);
      destroy_at(compute_index(0));
      increment_offset();
    } else {
      new (raw_data() + compute_index(size_)) T(value);
      ++size_;
    }
  }

  void push_front(const T& value) {
    size_type newOff = (offset_ == 0 ? capacity() : offset_ - 1);
    if (full()) {
      new (raw_data() + newOff) T(value);
      destroy_at(compute_index(size_ - 1));
      offset_ = newOff;
    } else {
      new (raw_data() + newOff) T(value);
      offset_ = newOff;
      ++size_;
    }
  }

  void pop_back() {
    if (empty()) return;
    destroy_at(compute_index(size_ - 1));
    --size_;
  }

  void pop_front() {
    if (empty()) return;
    destroy_at(compute_index(0));
    increment_offset();
    --size_;
  }

  void insert(iterator pos, const T& value);

  void erase(iterator pos);

  void swap(CircularBuffer& other) noexcept {
    Base::swap(other);
    std::swap(size_, other.size_);
    std::swap(offset_, other.offset_);
  }

 private:
  size_type size_ = 0;
  size_type offset_ = 0;

  size_type compute_index(size_type idx) const noexcept {
    return (offset_ + idx) % (capacity() + 1);
  }

  void destroy_at(size_type idx) noexcept { (raw_data() + idx)->~T(); }

  void increment_offset() noexcept {
    offset_ = (offset_ + 1) % (capacity() + 1);
  }

  void decrement_offset() noexcept {
    offset_ = (offset_ == 0 ? capacity() : offset_ - 1);
  }

  template <bool>
  friend class Iterator;
};

template <typename T, std::size_t Capacity>
template <bool IsConst>
class CircularBuffer<T, Capacity>::Iterator
    : private BufferStorage<T, Capacity>::IterBase {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::conditional_t<IsConst, const T, T>;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using pointer = value_type*;

  template <bool>
  friend class Iterator;

  Iterator() = default;

  Iterator(pointer data, difference_type pos, size_type cap)
      : BufferStorage<T, Capacity>::IterBase(cap),
        data_(data),
        pos_(pos),
        cap_(cap) {}

  template <bool OtherIsConst,
            std::enable_if_t<IsConst && !OtherIsConst, int> = 0>
  Iterator(const Iterator<OtherIsConst>& o) noexcept
      : BufferStorage<T, Capacity>::IterBase(o.cap_),
        data_(o.data_),
        pos_(o.pos_),
        cap_(o.cap_) {}

  template <bool OtherIsConst,
            std::enable_if_t<IsConst && !OtherIsConst, int> = 0>
  Iterator& operator=(const Iterator<OtherIsConst>& o) noexcept {
    data_ = o.data_;
    pos_ = o.pos_;
    cap_ = o.cap_;
    return *this;
  }

  reference operator*() const { return *base(); }

  pointer operator->() const { return base(); }

  Iterator& operator++() {
    ++pos_;
    return *this;
  }

  Iterator operator++(int) {
    Iterator tmp = *this;
    ++*this;
    return tmp;
  }

  Iterator& operator--() {
    --pos_;
    return *this;
  }

  Iterator operator--(int) {
    Iterator tmp = *this;
    --*this;
    return tmp;
  }

  Iterator operator+(difference_type n) const {
    return {data_, pos_ + n, cap_};
  }

  Iterator operator-(difference_type n) const {
    return {data_, pos_ - n, cap_};
  }

  difference_type operator-(const Iterator& o) const { return pos_ - o.pos_; }

  Iterator& operator+=(difference_type n) {
    pos_ += n;
    return *this;
  }

  Iterator& operator-=(difference_type n) {
    pos_ -= n;
    return *this;
  }

  template <bool OtherIsConst>
  bool operator==(const Iterator<OtherIsConst>& o) const noexcept {
    return data_ == o.data_ && pos_ == o.pos_;
  }

  template <bool OtherIsConst>
  bool operator!=(const Iterator<OtherIsConst>& o) const noexcept {
    return !(*this == o);
  }

  bool operator<(const Iterator& o) const noexcept { return pos_ < o.pos_; }

  bool operator>(const Iterator& o) const noexcept { return pos_ > o.pos_; }

  bool operator<=(const Iterator& o) const noexcept { return pos_ <= o.pos_; }

  bool operator>=(const Iterator& o) const noexcept { return pos_ >= o.pos_; }

  friend Iterator operator+(difference_type n, const Iterator& it) {
    return it + n;
  }

 protected:
  pointer data_;
  difference_type pos_;
  size_type cap_;

  pointer base() const noexcept {
    auto m = cap_ + 1;
    auto idx = (static_cast<std::size_t>(pos_) % m + m) % m;
    return data_ + idx;
  }
};

template <typename T, std::size_t Capacity>
void CircularBuffer<T, Capacity>::insert(iterator pos, const T& value) {
  if (pos < begin() || pos > end()) throw std::out_of_range("invalid position");
  size_type off = pos - begin();
  if (full()) {
    if (pos == begin()) return;
    destroy_at(compute_index(0));
    increment_offset();
    --size_;
    if (off > 0) --off;
  }
  for (size_type i = size_; i > off; --i) {
    new (raw_data() + compute_index(i)) T(std::move((*this)[i - 1]));
    destroy_at(compute_index(i - 1));
  }
  new (raw_data() + compute_index(off)) T(value);
  ++size_;
}

template <typename T, std::size_t Capacity>
void CircularBuffer<T, Capacity>::erase(iterator pos) {
  if (pos < begin() || pos >= end())
    throw std::out_of_range("invalid position");
  size_type off = pos - begin();
  for (size_type i = off; i + 1 < size_; ++i) {
    (*this)[i] = std::move((*this)[i + 1]);
  }
  pop_back();
}
