//
// Created by Darwin Yuan on 2020/9/12.
//

#ifndef E_CORO_AWAITABLE_TRAIT_H
#define E_CORO_AWAITABLE_TRAIT_H

#include <e-coro/e_coro_ns.h>
#include <coroutine>
#include <memory>
#include <future>

E_CORO_NS_BEGIN

namespace detail {
   template<typename T>
   struct is_coroutine_handle : std::false_type {};

   template<typename T>
   struct is_coroutine_handle<std::coroutine_handle<T>> : std::true_type {};

   template<typename T>
   constexpr bool is_coroutine_handle_v = is_coroutine_handle<T>::value;

   template<typename T>
   concept valid_await_suspend_result =
   std::is_void_v<T> ||
   std::is_same_v<bool, T> ||
   is_coroutine_handle_v<T>;
}

template<typename T>
concept awaiter_concept = requires(T value) {
   { value.await_ready() } -> std::same_as<bool>;
   { value.await_suspend(std::declval<std::coroutine_handle<>>()) } -> detail::valid_await_suspend_result;
   { value.await_resume() };
};

template<typename T>
concept has_co_await = requires(T&& value) {
   static_cast<T&&>(value).operator co_await();
};

template<typename T>
concept has_global_co_await = requires(T&& value) {
   operator co_await(static_cast<T&&>(value));
};

namespace detail {
   template<has_co_await T>
   auto get_awaiter(T&& value) {
      return static_cast<T&&>(value).operator co_await();
   }

   template<has_global_co_await T>
   auto get_awaiter(T&& value) {
      return operator co_await(static_cast<T&&>(value));
   }

   template<awaiter_concept T>
   auto get_awaiter(T&& value) {
      return value;
   }
}

template<typename T>
concept awaitable_concept =
has_co_await<T> || has_global_co_await<T> || awaiter_concept<T>;


template<typename T>
struct awaitable_traits {
   using awaiter_t = decltype(detail::get_awaiter(std::declval<T>()));
   using await_result_t = decltype(std::declval<awaiter_t>().await_resume());
};

template<typename T>
using await_result_t = typename awaitable_traits<T>::await_result_t;

template<typename T>
concept void_awaitable = std::is_void_v<await_result_t<T>>;

template<typename T>
concept non_void_awaitable = !void_awaitable<T>;

E_CORO_NS_END


#endif //E_CORO_AWAITABLE_TRAIT_H
