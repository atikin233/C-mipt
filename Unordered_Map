#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

template <typename T>
struct AlignedBuffer {
  alignas(T) unsigned char storage[sizeof(T)];
  T* pointer() noexcept { return reinterpret_cast<T*>(storage); }
  const T* pointer() const noexcept {
    return reinterpret_cast<const T*>(storage);
  }
  T& value() noexcept { return *pointer(); }
  const T& value() const noexcept { return *pointer(); }
};

template <typename Key, typename T, typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
class UnorderedMap {
 public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using size_type = std::size_t;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using allocator_type = Allocator;

 private:
  struct Node {
    Node* prev_all;
    Node* next_all;
    Node* prev_bucket;
    Node* next_bucket;
    size_type hash_value;
    AlignedBuffer<value_type> storage;
    value_type& kv() noexcept { return storage.value(); }
  };

 public:
  template <bool IsConst>
  class Iterator {
    friend class UnorderedMap;
    Node* ptr;
    explicit Iterator(Node* nodePtr) noexcept : ptr(nodePtr) {}

   public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type =
        std::conditional_t<IsConst, const std::pair<const Key, T>,
                           std::pair<const Key, T>>;
    using difference_type = std::ptrdiff_t;
    using reference = value_type&;
    using pointer = value_type*;

    Iterator() noexcept : ptr(nullptr) {}
    template <bool B = IsConst, typename = std::enable_if_t<B>>
    Iterator(const Iterator<false>& otherIt) noexcept : ptr(otherIt.ptr) {}

    reference operator*() const noexcept { return ptr->kv(); }
    pointer operator->() const noexcept { return std::addressof(ptr->kv()); }

    Iterator& operator++() noexcept {
      ptr = ptr->next_all;
      return *this;
    }
    Iterator operator++(int) noexcept {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    Iterator& operator--() noexcept {
      ptr = ptr->prev_all;
      return *this;
    }
    Iterator operator--(int) noexcept {
      Iterator tmp = *this;
      --*this;
      return tmp;
    }

    bool operator==(const Iterator& otherIt) const noexcept {
      return ptr == otherIt.ptr;
    }
    bool operator!=(const Iterator& otherIt) const noexcept {
      return ptr != otherIt.ptr;
    }
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  UnorderedMap(size_type bucketCount = 1, const Hash& hashFunc = Hash{},
               const KeyEqual& keyEq = KeyEqual{},
               const Allocator& allocParam = Allocator{})
      : buckets(bucketCount, nullptr),
        head(nullptr),
        map_size(0),
        max_load_factor_value(1.0),
        hash_function(hashFunc),
        key_equal_function(keyEq),
        allocator_instance(allocParam) {}

  ~UnorderedMap() {
    clear();
    deallocate_node(head);
  }

  UnorderedMap(const UnorderedMap& otherMap)
      : buckets(otherMap.buckets.size(), nullptr),
        head(nullptr),
        map_size(0),
        max_load_factor_value(otherMap.max_load_factor_value),
        hash_function(otherMap.hash_function),
        key_equal_function(otherMap.key_equal_function),
        allocator_instance(std::allocator_traits<Allocator>::
                               select_on_container_copy_construction(
                                   otherMap.allocator_instance)) {
    for (auto& kvPair : otherMap) {
      insert(kvPair);
    }
  }

  UnorderedMap(UnorderedMap&& sourceMap) noexcept
      : buckets(std::move(sourceMap.buckets)),
        head(sourceMap.head),
        map_size(sourceMap.map_size),
        max_load_factor_value(sourceMap.max_load_factor_value),
        hash_function(std::move(sourceMap.hash_function)),
        key_equal_function(std::move(sourceMap.key_equal_function)),
        allocator_instance(std::move(sourceMap.allocator_instance)) {
    sourceMap.head = nullptr;
    sourceMap.map_size = 0;
  }

  UnorderedMap& operator=(const UnorderedMap& other) {
    if (this != &other) {
      if constexpr (std::allocator_traits<Allocator>::
                        propagate_on_container_copy_assignment::value) {
        clear();
        buckets.clear();
        hash_function = other.hash_function;
        key_equal_function = other.key_equal_function;
        allocator_instance = other.allocator_instance;
        max_load_factor_value = other.max_load_factor_value;
        buckets.assign(other.buckets.size(), nullptr);
        for (auto& kv : other) {
          insert(kv);
        }
      } else {
        if (allocator_instance == other.allocator_instance) {
          clear();
          hash_function = other.hash_function;
          key_equal_function = other.key_equal_function;
          max_load_factor_value = other.max_load_factor_value;
          reserve(other.map_size);
          for (auto& kv : other) {
            insert(kv);
          }
        } else {
          UnorderedMap tmp(other);
          swap(tmp);
        }
      }
    }
    return *this;
  }

  UnorderedMap& operator=(UnorderedMap&& other) noexcept(
      std::allocator_traits<
          Allocator>::propagate_on_container_move_assignment::value ||
      std::allocator_traits<Allocator>::is_always_equal::value) {
    if (this != &other) {
      if constexpr (std::allocator_traits<Allocator>::
                        propagate_on_container_move_assignment::value) {
        clear();
        buckets = std::move(other.buckets);
        head = other.head;
        map_size = other.map_size;
        max_load_factor_value = other.max_load_factor_value;
        hash_function = std::move(other.hash_function);
        key_equal_function = std::move(other.key_equal_function);
        allocator_instance = std::move(other.allocator_instance);

        other.head = nullptr;
        other.map_size = 0;
      } else {
        if (allocator_instance == other.allocator_instance) {
          clear();
          buckets = std::move(other.buckets);
          head = other.head;
          map_size = other.map_size;
          max_load_factor_value = other.max_load_factor_value;
          hash_function = std::move(other.hash_function);
          key_equal_function = std::move(other.key_equal_function);

          other.head = nullptr;
          other.map_size = 0;
        } else {
          UnorderedMap tmp(std::move(other));
          swap(tmp);
        }
      }
    }
    return *this;
  }

  iterator begin() noexcept { return iterator(head->next_all); }
  iterator end() noexcept { return iterator(head); }
  const_iterator begin() const noexcept {
    return const_iterator(head->next_all);
  }
  const_iterator end() const noexcept { return const_iterator(head); }
  const_iterator cbegin() const noexcept { return begin(); }
  const_iterator cend() const noexcept { return end(); }

  bool empty() const noexcept { return map_size == 0; }
  size_type size() const noexcept { return map_size; }
  double load_factor() const noexcept {
    return double(map_size) / buckets.size();
  }
  double max_load_factor() const noexcept { return max_load_factor_value; }
  void max_load_factor(double ml) noexcept { max_load_factor_value = ml; }
  void reserve(size_type n) {
    size_type newBucketCount = std::max<size_type>(
        1, size_type(::ceil(double(n) / max_load_factor_value)));
    rehash(newBucketCount);
  }

  T& at(const Key& key) { return const_cast<T&>(std::as_const(*this).at(key)); }

  const T& at(const Key& key) const {
    Node* foundNode = find_node(key);
    if (!foundNode) throw std::out_of_range("at");
    return foundNode->kv().second;
  }

  T& operator[](const Key& key) {
    auto result = emplace(key, T{});
    return result.first->second;
  }

  iterator find(const Key& key) noexcept {
    Node* foundNode = find_node(key);
    return iterator(foundNode ? foundNode : head);
  }

  const_iterator find(const Key& key) const noexcept {
    Node* foundNode = find_node(key);
    return const_iterator(foundNode ? foundNode : head);
  }

  std::pair<iterator, bool> insert(const value_type& kvPair) {
    return emplace(kvPair);
  }
  std::pair<iterator, bool> insert(value_type&& kvPair) {
    return emplace(std::move(kvPair));
  }
  template <typename Pair>
  std::pair<iterator, bool> insert(Pair&& pairArg) {
    return emplace(std::forward<Pair>(pairArg));
  }
  template <class InputIt>
  void insert(InputIt firstIt, InputIt lastIt) {
    for (; firstIt != lastIt; ++firstIt) {
      insert(*firstIt);
    }
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    if (!head) {
      head = allocate_node();
      head->prev_all = head->next_all = head;
    }
    Node* newNode = allocate_node();
    try {
      std::allocator_traits<Allocator>::construct(allocator_instance,
                                                  newNode->storage.pointer(),
                                                  std::forward<Args>(args)...);
    } catch (...) {
      deallocate_node(newNode);
      throw;
    }
    newNode->hash_value = hash_function(newNode->kv().first);
    size_type bucketIndex = newNode->hash_value % buckets.size();

    for (Node* existingNode = buckets[bucketIndex]; existingNode;
         existingNode = existingNode->next_bucket) {
      if (key_equal_function(existingNode->kv().first, newNode->kv().first)) {
        std::allocator_traits<Allocator>::destroy(allocator_instance,
                                                  newNode->storage.pointer());
        deallocate_node(newNode);
        return {iterator(existingNode), false};
      }
    }

    newNode->next_all = head;
    newNode->prev_all = head->prev_all;
    head->prev_all->next_all = newNode;
    head->prev_all = newNode;

    newNode->next_bucket = buckets[bucketIndex];
    newNode->prev_bucket = nullptr;
    if (buckets[bucketIndex]) buckets[bucketIndex]->prev_bucket = newNode;
    buckets[bucketIndex] = newNode;

    ++map_size;
    if (load_factor() > max_load_factor_value) {
      rehash(buckets.size() * 2);
    }
    return {iterator(newNode), true};
  }

  iterator erase(const_iterator pos) {
    Node* targetNode = pos.ptr;
    if (!targetNode || targetNode == head) return end();
    size_type bucketIndex = targetNode->hash_value % buckets.size();
    if (targetNode->prev_bucket)
      targetNode->prev_bucket->next_bucket = targetNode->next_bucket;
    else
      buckets[bucketIndex] = targetNode->next_bucket;
    if (targetNode->next_bucket)
      targetNode->next_bucket->prev_bucket = targetNode->prev_bucket;
    targetNode->prev_all->next_all = targetNode->next_all;
    targetNode->next_all->prev_all = targetNode->prev_all;
    iterator returnIt(targetNode->next_all);
    std::allocator_traits<Allocator>::destroy(allocator_instance,
                                              targetNode->storage.pointer());
    deallocate_node(targetNode);
    --map_size;
    return returnIt;
  }

  iterator erase(iterator firstIt, iterator lastIt) {
    return erase(const_iterator(firstIt), const_iterator(lastIt));
  }
  iterator erase(const_iterator firstIter, const_iterator lastIter) {
    auto currentIt = firstIter;
    while (currentIt != lastIter) {
      currentIt = erase(currentIt);
    }
    return iterator(lastIter.ptr);
  }

  size_type erase(const Key& key) {
    auto foundIt = find(key);
    if (foundIt == end()) return 0;
    erase(foundIt);
    return 1;
  }

  void clear() noexcept {
    while (!empty()) {
      erase(begin());
    }
  }

  void swap(UnorderedMap& otherMap) noexcept {
    std::swap(buckets, otherMap.buckets);
    std::swap(head, otherMap.head);
    std::swap(map_size, otherMap.map_size);
    std::swap(max_load_factor_value, otherMap.max_load_factor_value);
    std::swap(hash_function, otherMap.hash_function);
    std::swap(key_equal_function, otherMap.key_equal_function);
    if constexpr (std::allocator_traits<
                      Allocator>::propagate_on_container_swap::value) {
      std::swap(allocator_instance, otherMap.allocator_instance);
    }
  }

 private:
  using NodeAlloc =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;

  std::vector<Node*> buckets;
  Node* head;
  size_type map_size;
  double max_load_factor_value;
  Hash hash_function;
  KeyEqual key_equal_function;
  Allocator allocator_instance;

  Node* allocate_node() {
    NodeAlloc nodeAllocator(allocator_instance);
    return std::allocator_traits<NodeAlloc>::allocate(nodeAllocator, 1);
  }
  void deallocate_node(Node* nodePtr) {
    if (!nodePtr) return;
    NodeAlloc nodeAllocator(allocator_instance);
    std::allocator_traits<NodeAlloc>::deallocate(nodeAllocator, nodePtr, 1);
  }

  Node* find_node(const Key& searchKey) const noexcept {
    size_type hashValue = hash_function(searchKey);
    size_type bucketIndex = hashValue % buckets.size();
    for (Node* bucketNode = buckets[bucketIndex]; bucketNode;
         bucketNode = bucketNode->next_bucket) {
      if (key_equal_function(bucketNode->kv().first, searchKey)) {
        return bucketNode;
      }
    }
    return nullptr;
  }

  void rehash(size_type newBucketCount) {
    std::vector<Node*> newBuckets(newBucketCount, nullptr);
    if (head) {
      for (Node* nodeIter = head->next_all; nodeIter != head;
           nodeIter = nodeIter->next_all) {
        size_type bucketIndex = nodeIter->hash_value % newBucketCount;
        nodeIter->next_bucket = newBuckets[bucketIndex];
        nodeIter->prev_bucket = nullptr;
        if (newBuckets[bucketIndex])
          newBuckets[bucketIndex]->prev_bucket = nodeIter;
        newBuckets[bucketIndex] = nodeIter;
      }
    }
    buckets.swap(newBuckets);
  }
};
