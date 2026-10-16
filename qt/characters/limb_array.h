#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <type_traits>

namespace pets::character {
// Bounded walking anatomy. Spare capacity never participates in iteration.
template <class T> class LimbArray {
public:
  LimbArray() = default;
  LimbArray(std::initializer_list<T> values) { assign(values.begin(), values.size()); }
  // The concrete two-element overload also accepts legacy nested braces.
  LimbArray(const std::array<T, 2> &values) { assign(values.begin(), 2); }
  template <std::size_t N, std::enable_if_t<N == 4, int> = 0>
  LimbArray(const std::array<T, N> &values) { assign(values.begin(), N); }
  LimbArray &operator=(std::initializer_list<T> values) {
    assign(values.begin(), values.size()); return *this;
  }
  template <std::size_t N, std::enable_if_t<N == 2 || N == 4, int> = 0>
  LimbArray &operator=(const std::array<T, N> &values) {
    assign(values.begin(), N); return *this;
  }
  void setCount(std::size_t count) {
    checkCount(count);
    for (auto i = count_; i < count; ++i) values_[i] = T{};
    count_ = count;
  }
  std::size_t size() const { return count_; }
  T *begin() { return values_.data(); }
  const T *begin() const { return values_.data(); }
  T *end() { return begin() + count_; }
  const T *end() const { return begin() + count_; }
  T &operator[](std::size_t i) { return at(i); }
  const T &operator[](std::size_t i) const { return at(i); }
  T &at(std::size_t i) {
    if (i >= count_) throw std::out_of_range("inactive limb");
    return values_[i];
  }
  const T &at(std::size_t i) const {
    if (i >= count_) throw std::out_of_range("inactive limb");
    return values_[i];
  }
  void fill(const T &value) { std::fill(begin(), end(), value); }
  template <std::size_t N> std::array<T, N> array() const {
    static_assert(N == 2 || N == 4, "walking anatomy needs two or four limbs");
    if (N != count_) throw std::invalid_argument("limb count mismatch");
    std::array<T, N> result;
    std::copy(begin(), end(), result.begin());
    return result;
  }
private:
  static void checkCount(std::size_t count) {
    if (count != 2 && count != 4)
      throw std::invalid_argument("walking anatomy needs two or four limbs");
  }
  template <class Iterator> void assign(Iterator values, std::size_t count) {
    checkCount(count);
    std::copy_n(values, count, values_.begin());
    count_ = count;
  }
  std::array<T, 4> values_{};
  std::size_t count_ = 2;
};
} // namespace pets::character
