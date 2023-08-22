#pragma once

namespace fragment {
template <typename T, auto... Field> struct StealMember {
  friend auto steal_impl(T &t) { return std::make_tuple(Field...); }
};
} // namespace fragment

#define FRAGMENT_STEAL_MEMBER_DEFINE(Object, ...)                              \
  namespace fragment {                                                         \
  auto steal_impl(Object &t);                                                  \
  template struct StealMember<Object, __VA_ARGS__>;                            \
  }

#define FRAGMENT_STEAL_MEMBER_METHOD fragment::steal_impl
