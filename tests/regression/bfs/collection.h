#pragma once

#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <vector>
#include <array>
#include <iterator>
#include <algorithm>

//#define COLLECTION_ATTRIBUTES __attribute__((noinline))
#define COLLECTION_ATTRIBUTES

namespace collection {

template <int PCode,
          typename Assoc,
          typename Order,
          typename Uniq,
          typename Domain>
class __Collection;

template <typename Assoc, typename Order, typename Uniq, typename Domain>
class Collection;

enum Property {
  PNonAssociative = (0 << 0),
  PAssociative = (1 << 0),

  PUnordered = (0 << 2),
  PIndexed = (1 << 2),
  POrdered = (2 << 2),

  PNonUnique = (0 << 4),
  PUnique = (1 << 4),

  PUnbounded = (0 << 6),
  PBounded = (1 << 6),
  PRange = (2 << 6)
};

template <typename Tk, typename Tv>
class Associative {};

template <typename T>
class NonAssociative {};

template <typename Comp>
class Ordered {};

class Indexed {};

class Unordered {};

class Unique {};

class NonUnique {};

class Unbounded {};

template <typename T, T... Ts>
class Bounded {
public:
  std::array<T, sizeof...(Ts)> values;

  Bounded() {
    init(0, Ts...);
  }

protected:
  void init(int) {}

  template <typename... Us>
  void init(int i, T first, Us... others) {
    values[i] = first;
    init(i + 1, others...);
  }
};

template <typename T, T first, T last>
class Range {
  static_assert(last - first + 1 > 0, "The size of a range must be positive");
};

//===----------------------------------------------------------------------===//
// Map
//===----------------------------------------------------------------------===//

template <typename Tk, typename Tv>
class __Collection<PAssociative | PUnordered | PNonUnique | PUnbounded,
                   Associative<Tk, Tv>,
                   Unordered,
                   NonUnique,
                   Unbounded> {
public:
  using ContainerType = std::unordered_map<Tk, Tv>;

  size_t size() const {
    return container_.size();
  }

  bool contains(const Tv &value) const {
    return container_.find(value) != container_.end();
  }

  void set(const Tk &key, const Tv &value) {
    container_.insert({ key, value });
  }

  Tv get(const Tk &key) const {
    return container_.at(key);
  }

  typename ContainerType::iterator begin() {
    return container_.begin();
  }

  typename ContainerType::iterator end() {
    return container_.end();
  }

protected:
  ContainerType container_;
};

template <typename Tk, typename Tv>
class Collection<Associative<Tk, Tv>, Unordered, NonUnique, Unbounded>
  : public __Collection<PAssociative | PUnordered | PNonUnique | PUnbounded,
                        Associative<Tk, Tv>,
                        Unordered,
                        NonUnique,
                        Unbounded> {};

//===----------------------------------------------------------------------===//
// Map (iterable in order)
//===----------------------------------------------------------------------===//

template <typename Tk, typename Tv, typename Comp>
class __Collection<PAssociative | POrdered | PNonUnique | PUnbounded,
                   Associative<Tk, Tv>,
                   Ordered<Comp>,
                   NonUnique,
                   Unbounded> {
public:
  using ContainerType = std::map<Tk, Tv, Comp>;

  size_t size() const {
    return container_.size();
  }

  bool contains(const Tv &value) const {
    return container_.find(value) != container_.end();
  }

  void set(const Tk &key, const Tv &value) {
    container_.insert({ key, value });
  }

  Tv get(const Tk &key) const {
    return container_.at(key);
  }

  typename ContainerType::iterator begin() {
    return container_.begin();
  }

  typename ContainerType::iterator end() {
    return container_.end();
  }

protected:
  ContainerType container_;
};

template <typename Tk, typename Tv, typename Comp>
class Collection<Associative<Tk, Tv>, Ordered<Comp>, NonUnique, Unbounded>
  : public __Collection<PAssociative | POrdered | PNonUnique | PUnbounded,
                        Associative<Tk, Tv>,
                        Ordered<Comp>,
                        NonUnique,
                        Unbounded> {};

//===----------------------------------------------------------------------===//
// Bag
//===----------------------------------------------------------------------===//

template <typename T>
class __Collection<PNonAssociative | PUnordered | PNonUnique | PUnbounded,
                   NonAssociative<T>,
                   Unordered,
                   NonUnique,
                   Unbounded> {
public:
  using ContainerType = std::unordered_multiset<T>;

  size_t size() const {
    return container_.size();
  }

  bool empty() const {
    return size() == 0;
  }

  bool contains(const T &value) const {
    return container_.find(value) != container_.end();
  }

  void add(const T &value) {
    container_.insert(value);
  }

  T extract() {
    auto value = *container_.begin();
    container_.erase(container_.begin());
    return value;
  }

  typename ContainerType::iterator begin() {
    return container_.begin();
  }

  typename ContainerType::iterator end() {
    return container_.end();
  }

protected:
  ContainerType container_;
};

template <typename T>
class Collection<NonAssociative<T>, Unordered, NonUnique, Unbounded>
  : public __Collection<PNonAssociative | PUnordered | PNonUnique | PUnbounded,
                        NonAssociative<T>,
                        Unordered,
                        NonUnique,
                        Unbounded> {};

//===----------------------------------------------------------------------===//
// Set
//===----------------------------------------------------------------------===//

template <typename T>
class __Collection<PNonAssociative | PUnordered | PUnique | PUnbounded,
                   NonAssociative<T>,
                   Unordered,
                   Unique,
                   Unbounded> {
public:
  using ContainerType = std::unordered_set<T>;

  size_t size() const {
    return container_.size();
  }

  bool contains(const T &value) const {
    return container_.find(value) != container_.end();
  }

  void remove(const T &value) {
    container_.erase(value);
  }

  void add(const T &value) {
    container_.insert(value);
  }

  typename ContainerType::iterator begin() {
    return container_.begin();
  }

  typename ContainerType::iterator end() {
    return container_.end();
  }

protected:
  ContainerType container_;
};

template <typename T>
class Collection<NonAssociative<T>, Unordered, Unique, Unbounded>
  : public __Collection<PNonAssociative | PUnordered | PUnique | PUnbounded,
                        NonAssociative<T>,
                        Unordered,
                        Unique,
                        Unbounded> {};

//===----------------------------------------------------------------------===//
// Set (iterable in order)
//===----------------------------------------------------------------------===//

template <typename T, typename Comp>
class __Collection<PNonAssociative | POrdered | PUnique | PUnbounded,
                   NonAssociative<T>,
                   Ordered<Comp>,
                   Unique,
                   Unbounded> {
public:
  using ContainerType = std::set<T, Comp>;

  size_t size() const {
    return container_.size();
  }

  bool contains(const T &value) const {
    return container_.find(value) != container_.end();
  }

  void add(const T &value) {
    container_.insert(value);
  }

  typename ContainerType::iterator begin() {
    return container_.begin();
  }

  typename ContainerType::iterator end() {
    return container_.end();
  }

protected:
  ContainerType container_;
};

template <typename T, typename Comp>
class Collection<NonAssociative<T>, Ordered<Comp>, Unique, Unbounded>
  : public __Collection<PNonAssociative | POrdered | PUnique | PUnbounded,
                        NonAssociative<T>,
                        Ordered<Comp>,
                        Unique,
                        Unbounded> {};

//===----------------------------------------------------------------------===//
// Sequence
//===----------------------------------------------------------------------===//

template <typename T>
class __Collection<PNonAssociative | PIndexed | PNonUnique | PUnbounded,
                   NonAssociative<T>,
                   Indexed,
                   NonUnique,
                   Unbounded> {
public:
  using ContainerType = std::vector<T>;

  size_t size() const {
    return container_.size();
  }

  bool empty() const {
    return size() == 0;
  }

  void resize(size_t s) {
    container_.resize(s);
  }

  bool contains(const T &value) const {
    auto pos = std::find(container_.begin(), container_.end(), value);
    return pos != container_.end();
  }

  void appendFront(const T &value) {
    container_.insert(container_.begin(), value);
  }

  void appendBack(const T &value) {
    container_.push_back(value);
  }

  void removeFirst() {
    container_.erase(container_.begin());
  }

  void removeLast() {
    container_.pop_back();
  }

  T &first() const {
    return container_[0];
  }

  T &last() const {
    return container_[container_.size() - 1];
  }

  void insertAt(size_t index, const T &value) {
    auto it = container_.begin();
    std::advance(it, index);
    container_.insert(it, value);
  }

  int64_t indexOf(const T &value) const {
    auto pos = std::find(container_.begin(), container_.end(), value);
    size_t dist = std::distance(container_.begin(), pos);
    if (pos == container_.end()) {
      return -1;
    }
    return std::distance(container_.begin(), pos);
  }

  T &operator[](size_t idx) {
    return container_[idx];
  }

  const T &operator[](size_t idx) const {
    return container_[idx];
  }

  typename ContainerType::iterator begin() {
    return container_.begin();
  }

  typename ContainerType::iterator end() {
    return container_.end();
  }

protected:
  ContainerType container_;
};

template <typename T>
class Collection<NonAssociative<T>, Indexed, NonUnique, Unbounded>
  : public __Collection<PNonAssociative | PIndexed | PNonUnique | PUnbounded,
                        NonAssociative<T>,
                        Indexed,
                        NonUnique,
                        Unbounded> {};

//===----------------------------------------------------------------------===//
// Aliases
//===----------------------------------------------------------------------===//

template <typename Tk, typename Tv, typename Domain = Unbounded>
using Map = Collection<Associative<Tk, Tv>, Unordered, NonUnique, Domain>;

template <typename Tk,
          typename Tv,
          typename Comp = std::less<Tk>,
          typename Domain = Unbounded>
using OrderedMap =
    Collection<Associative<Tk, Tv>, Ordered<Comp>, NonUnique, Domain>;

template <typename T>
using Sequence = Collection<NonAssociative<T>, Indexed, NonUnique, Unbounded>;

template <typename T>
using Set = Collection<NonAssociative<T>, Unordered, Unique, Unbounded>;

template <typename T, typename Comp = std::less<T>>
using OrderedSet =
    Collection<NonAssociative<T>, Ordered<Comp>, Unique, Unbounded>;

template <typename T>
using Bag = Collection<NonAssociative<T>, Unordered, NonUnique, Unbounded>;

} // namespace collection
