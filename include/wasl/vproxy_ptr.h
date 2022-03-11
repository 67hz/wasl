#ifndef WASL_VPROXY_PTR_H
#define WASL_VPROXY_PTR_H

#include <wasl/Types.h>

#include <cassert>
#include <functional>
#include <stdexcept>
#include <utility>

#include <iostream>

namespace wasl {

/// \brief Virtual proxy wrapper.
///
/// This gives a type T the ability to defer construction, Two primary use cases
/// are lazy loading T's or containing a
/// non-default-constructible member in a default-constructible class. Pointers
/// can be used for the latter case - this vproxy_ptr wraps such logic into
/// a movable-only RAII handle.
///
/// \note The proxied type is held as a raw pointer. gsl::owner<T> will complain
/// if set to nullptr. Raw pointers will not complain but fail to
/// communicate the ownership of the proxied type. Tradeoffs.
///
/// \note No overriding of unary operator&. This preserves STL container
/// compatibility and prevents referential heartaches.
///
/// \tparam T type to wrap
template <typename T> class vproxy_ptr {
public:
  using value_type = T;
  using pointer = std::add_pointer_t<T>;
  using const_pointer = std::add_const_t<pointer>;
  using reference = std::add_lvalue_reference_t<T>;
  using const_reference = std::add_const<reference>;

  vproxy_ptr() = default;

  /// During proxy construction arguments to this constructor are forwarded into
  //  a closure to be invoked during load().
  template <
      typename... Args,
      std::enable_if_t<std::is_constructible<T, Args...>::value, bool> = true>
  explicit vproxy_ptr(Args... args) {
    init_with_params = lazy_constructor(std::forward<Args...>(args)...);
  }

  /// Initializes proxy given the params previously passed to the constructor or
  /// else via T's default constructor.
  ///
  /// \return a new T*.
  ///
  /// \pre vproxy_ptr was passed required arguments during construction or else
  /// T is default-constructible.
  pointer load() {
    return load_impl(typename std::is_default_constructible<T>());
  }

  /// Construct and return a proxy. If proxy was previously constructed, return
  /// the existing value.
  /// \pre T(args...) must be constructible && proxy has not already been
  /// loaded.
  template <typename... Args>
  typename std::enable_if_t<std::is_constructible<T, Args...>::value, pointer>
  load(Args &&...args) {
    if (proxy == nullptr) {
      proxy = new T(std::forward<Args...>(args)...);
    }
    return get();
  }

  /// TODO create a placeholder type to represent null instead of throwing
  /// to avoid checking on every access.
  constexpr pointer get() const {
    // assert(!!proxy);

    if (!proxy) {
      throw std::runtime_error("dereferencing a null vproxy_ptr");
    }
    return proxy;
  }

  constexpr std::enable_if_t<
      std::is_class<T>::value || std::is_pointer<T>::value, T> *
  operator->() const {
    return get();
  }

  constexpr decltype(auto) operator*() const { return *get(); }

  // implicitly convert to proxy type
  constexpr explicit operator T() const { return get(); }

  constexpr bool operator!() const { return proxy == nullptr; }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  constexpr bool operator==(const vproxy_ptr<U> &rhs) const {
    return proxy == rhs.proxy;
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  constexpr bool operator!=(const vproxy_ptr<U> &rhs) const {
    return proxy != rhs.proxy;
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  inline friend constexpr bool operator==(const vproxy_ptr &lhs, const U *rhs) {
    return lhs.proxy == rhs;
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  inline friend constexpr bool operator==(const U *lhs, const vproxy_ptr &rhs) {
    return lhs == rhs.proxy;
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  inline friend constexpr bool operator!=(const vproxy_ptr &lhs, const U *rhs) {
    return lhs.proxy != rhs;
  }

  template <typename U,
            typename = std::enable_if_t<std::is_convertible<U, T>::value>>
  inline friend constexpr bool operator!=(const U *lhs, const vproxy_ptr &rhs) {
    return lhs != rhs.proxy;
  }

  vproxy_ptr &operator=(vproxy_ptr &&other) noexcept {
    std::swap(proxy, other.proxy);
    std::swap(init_with_params, other.init_with_params);
    other.reset();
    return *this;
  }

  vproxy_ptr(vproxy_ptr &&other) noexcept { *this = std::move(other); }

  ~vproxy_ptr() { reset(); }

private:
  pointer proxy{nullptr};
  std::function<pointer()> init_with_params{nullptr};

  /// Reset underlying proxy
  void reset() {
    if (proxy) {
      delete proxy;
    }

    proxy = nullptr;
    init_with_params = nullptr;
  }

  /// Stores constructor params for deferred loading.
  /// \return factory lambda creating a T*
  template <typename... Args> auto lazy_constructor(Args &&...args) {
    return [args...]() { return new T((args)...); };
  }

  /// Construct a T via lambda closure saved during construction or else
  /// fall back to T's default constructor.
  /// \param true_type T is default-constructible
  /// \pre T is default-constructible
  pointer load_impl(std::true_type) noexcept(noexcept(T())) {
    if (init_with_params == nullptr) {
      proxy = new T();
    } else {
      // create the proxy via the lazy constructor
      proxy = init_with_params();
    }
    return get();
  }

  /// Invoke the deferred constructor and return the new T*.
  /// \param false_type T is non-default-constructible
  /// \pre T is non-default-constructible and arguments to T's constructor were
  /// saved during this proxy's construction.
  pointer load_impl(std::false_type) {
    if (init_with_params != nullptr) {
      /// invoke proxy's constructor
      proxy = init_with_params();
    }

    /// If no params were saved during construction, proxy will be nullptr here
    /// and get() will throw.
    return get();
  }
};

} // namespace wasl

#endif /* WASL_VPROXY_PTR_H */
