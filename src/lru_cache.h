/**
Basic templated version of LRU Cache

CPP Idioms:
01. user can use shared_ptr<V> to update the data directly, if want to only grand readonly access,
shared_ptr<const V> is the right type.
02. the semantics of put and emplace is different in this implementation, emplace is more for
updating existing items, that's why we take const& key to void the copy of key here.

*/

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <cassert>
#include <cstddef>
#include <list>
#include <memory>
#include <unordered_map>

template <typename K,
          typename V,
          typename Hash = std::hash<K>,
          typename KeyEqual = std::equal_to<K>>
class LruCache {
public:
  explicit LruCache(std::size_t capacity) : capacity_(capacity) {
    assert(capacity > 0); // LCOV_EXCL_LINE
  }

  LruCache(LruCache &&other) noexcept = default;
  LruCache &operator=(LruCache &&other) noexcept = default;

  LruCache(const LruCache &other) {
    capacity_ = other.capacity_;

    for (auto it = other.list_.crbegin(); it != other.list_.crend(); ++it) {
      put(it->key, *(it->value));
    }
  }

  LruCache &operator=(const LruCache &other) {
    if (this != &other) {
      capacity_ = other.capacity_;

      data_.clear();
      list_.clear();

      for (auto it = other.list_.crbegin(); it != other.list_.crend(); ++it) {
        put(it->key, *(it->value));
      }
    }

    return *this;
  }

  ~LruCache() = default;

  [[nodiscard]] std::size_t capacity() const noexcept {
    return capacity_;
  }

  // If returns a shared_ptr<V>, caller can use *ptr to update the data inside the
  // cache directly, which is not something we want
  [[nodiscard]] std::shared_ptr<const V> get(const K &key) {
    auto iter = data_.find(key);

    if (iter == data_.end()) {
      return nullptr;
    }

    list_.splice(list_.cbegin(), list_, iter->second);
    return iter->second->value;
  }

  void put(K key, V value) {
    auto iter = data_.find(key);

    if (iter == data_.end()) {
      // Insert scenario, need to check capacity first
      if (data_.size() == capacity_) {
        evict();
      }

      list_.emplace_front(key, std::make_shared<V>(std::move(value)));
      data_.emplace(std::move(key), list_.begin());
    } else {
      // update scenario, update the value
      list_.splice(list_.cbegin(), list_, iter->second);
      iter->second->value = std::make_shared<V>(std::move(value));
    }
  }

  // Intentionally set key as const K& to avoid a copy in the cache hit case
  // But in cache miss case, we need 2 copies instead of 1 as in the put
  // (put already copied once when enter that method)
  template <typename... Args>
  void emplace(const K &key, Args &&...args) {
    auto iter = data_.find(key);

    if (iter == data_.end()) {
      // Insert scenario, need to check capacity first
      if (data_.size() == capacity_) {
        evict();
      }

      // We need 2 copies of the key in the internal structure
      // Need to use perfect forward to avoid copy
      // if we use implementation below, compile error for V3_Embrace.PerfectForwardingTest
      // list_.emplace_front(key, std::make_shared<V>(args...));
      list_.emplace_front(key, std::make_shared<V>(std::forward<Args>(args)...));
      data_.emplace(key, list_.begin());
    } else {
      list_.splice(list_.cbegin(), list_, iter->second);
      iter->second->value = std::make_shared<V>(std::forward<Args>(args)...);
    }
  }

private:
  // When we promote a key, we need the pointer to the list_ item
  //    -> data_ map is <key, list::iter>.
  //    -> list_ item should contains the value.
  //
  // When we evict a key, we also need to remove it from data_
  //    -> list_ item should contains the key
  //
  // Now we have 2 copies of key, and we cannot use shared_ptr<key> in hash
  // because shared_ptr use the raw pointer address to calculated hash
  // Then exactly same keys will result in different hash which violate the intention
  struct Entry {
    K key;
    std::shared_ptr<V> value;
    // move a shared_ptr won't increase the internal count
    Entry(K key, std::shared_ptr<V> value) : key(std::move(key)), value(std::move(value)) {
    }
  };

  std::size_t capacity_;
  std::unordered_map<K, typename std::list<Entry>::iterator, Hash, KeyEqual> data_;
  std::list<Entry> list_;

  void evict() {
    data_.erase(list_.back().key);
    list_.pop_back();
  }
};

#endif