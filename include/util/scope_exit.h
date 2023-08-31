#pragma once

#include <exception>
#include <type_traits>

#define __SCOPE_EXIT_CONCATENATE_IMPL(s1, s2) s1##s2
#define __SCOPE_EXIT_CONCATENATE(s1, s2) __SCOPE_EXIT_CONCATENATE_IMPL(s1, s2)

namespace fragment {
template <typename FUNC> class scoped_lambda {
public:
  scoped_lambda(FUNC &&func)
      : func_(std::forward<FUNC>(func)), cancel_(false) {}

  scoped_lambda(const scoped_lambda &other) = delete;

  scoped_lambda(scoped_lambda &&other)
      : func_(std::forward<FUNC>(other.func_)), cancel_(other.cancel_) {
    other.cancel();
  }

  ~scoped_lambda() {
    if (!cancel_) {
      func_();
    }
  }

  void cancel() { cancel_ = true; }

  void call_now() {
    cancel_ = true;
    func_();
  }

private:
  FUNC func_;
  bool cancel_;
};

/// A scoped lambda that executes the lambda when the object destructs
/// but only if exiting due to an exception (CALL_ON_FAILURE = true) or
/// only if not exiting due to an exception (CALL_ON_FAILURE = false).
template <typename FUNC, bool CALL_ON_FAILURE> class conditional_scoped_lambda {
public:
  conditional_scoped_lambda(FUNC &&func)
      : func_(std::forward<FUNC>(func)),
        uncaught_exception_count_(std::uncaught_exceptions()), cancel_(false) {}

  conditional_scoped_lambda(const conditional_scoped_lambda &other) = delete;

  conditional_scoped_lambda(conditional_scoped_lambda &&other) noexcept(
      std::is_nothrow_move_constructible<FUNC>::value)
      : func_(std::forward<FUNC>(other.func_)),
        uncaught_exception_count_(other.uncaught_exception_count_),
        cancel_(other.cancel_) {
    other.cancel();
  }

  ~conditional_scoped_lambda() noexcept(CALL_ON_FAILURE ||
                                        noexcept(std::declval<FUNC>()())) {
    if (!cancel_ && (is_unwinding_due_to_exception() == CALL_ON_FAILURE)) {
      func_();
    }
  }

  void cancel() noexcept { cancel_ = true; }

private:
  bool is_unwinding_due_to_exception() const noexcept {
    return std::uncaught_exceptions() > uncaught_exception_count_;
  }

  FUNC func_;
  int uncaught_exception_count_;
  bool cancel_;
};

/// Returns an object that calls the provided function when it goes out
/// of scope either normally or due to an uncaught exception unwinding
/// the stack.
///
/// \param func
/// The function to call when the scope exits.
/// The function must be noexcept.
template <typename FUNC> auto on_scope_exit(FUNC &&func) {
  return scoped_lambda<FUNC>{std::forward<FUNC>(func)};
}

/// Returns an object that calls the provided function when it goes out
/// of scope due to an uncaught exception unwinding the stack.
///
/// \param func
/// The function to be called if unwinding due to an exception.
/// The function must be noexcept.
template <typename FUNC> auto on_scope_failure(FUNC &&func) {
  return conditional_scoped_lambda<FUNC, true>{std::forward<FUNC>(func)};
}

/// Returns an object that calls the provided function when it goes out
/// of scope via normal execution (ie. not unwinding due to an exception).
///
/// \param func
/// The function to call if the scope exits normally.
/// The function does not necessarily need to be noexcept.
template <typename FUNC> auto on_scope_success(FUNC &&func) {
  return conditional_scoped_lambda<FUNC, false>{std::forward<FUNC>(func)};
}

namespace detail {
enum class ScopeExit {};

template <typename Func>
inline scoped_lambda<Func> operator+(ScopeExit, Func &&fn) {
  return on_scope_exit(std::forward<Func>(fn));
}
}  // namespace detail
} // namespace fragment

#define ON_SCOPE_EXIT                                   \
  auto __SCOPE_EXIT_CONCATENATE(exitBlock_, __LINE__) = \
      fragment::detail::ScopeExit() + [&]()
