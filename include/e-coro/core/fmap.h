//
// Created by Darwin Yuan on 2020/9/13.
//

#ifndef E_CORO_FMAP_H
#define E_CORO_FMAP_H

#include <e-coro/core/awaitable_trait.h>
#include <coroutine>
#include <concepts>
#include <functional>

E_CORO_NS_BEGIN

namespace detail {

   template<typename T>
   concept await_resume_return_void = requires(T awaiter) {
      { awaiter.await_resume() } -> std::same_as<void>;
   };

   template<typename F, typename A>
   struct fmap_awaiter {
      using awaiter_t = typename awaitable_traits<A&&>::awaiter_t;

      fmap_awaiter(F&& func, A&& awaitable)
         : func_{static_cast<F&&>(func)}
         , awaiter_{detail::get_awaiter(static_cast<A&&>(awaitable))}
      {}

      auto await_ready() {
         return awaiter_.await_ready();
      }

      template<typename P>
      auto await_suspend(std::coroutine_handle<P> self) -> decltype(auto) {
         return awaiter_.await_suspend(std::move(self));
      }

      auto await_resume() -> decltype(auto)
      requires await_resume_return_void<awaiter_t> {
         awaiter_.await_resume();
         return func_();
      }

      auto await_resume() -> decltype(auto)
      requires (!await_resume_return_void<awaiter_t>) {
         return func_(awaiter_.await_resume());
      }

   private:
      F&& func_;
      awaiter_t awaiter_;
   };

   template<typename A, typename F>
   concept constructible_to = std::constructible_from<F, A>;

   template<typename F, typename A>
   struct fmap_awaitable {
      static_assert(!std::is_lvalue_reference_v<F>);
      static_assert(!std::is_lvalue_reference_v<A>);

      template<
         constructible_to<F> F_ARG,
         constructible_to<A> A_ARG>
      explicit fmap_awaitable(F_ARG&& func, A_ARG&& awaitable)
         : func_(std::forward<F_ARG>(func))
         , awaitable_(std::forward<A_ARG>(awaitable))
      {}

      auto operator co_await() const & {
         return fmap_awaiter<const F&, const A&>(func_, awaitable_);
      }

      auto operator co_await() & {
         return fmap_awaiter<F&, A&>(func_, awaitable_);
      }

      auto operator co_await() && {
         return fmap_awaiter<F&&, A&&>(std::move(func_), std::move(awaitable_));
      }

   private:
      F func_;
      A awaitable_;
   };
}

template<typename F, awaitable_concept A>
inline auto fmap(F&& func, A&& awaitable) {
   using awaiter = detail::fmap_awaitable<std::decay_t<F>, std::decay_t<A>>;
   return awaiter{std::forward<F>(func), std::forward<A>(awaitable)};
}

template<typename F>
struct fmap_transform {
   explicit fmap_transform(F&& f)
      : func_(std::forward<F>(f))
   {}

   F func_;
};

template<typename F>
inline auto fmap(F&& func) {
   return fmap_transform<F>{ std::forward<F>(func) };
}

template<typename T, typename F>
inline auto operator|(T&& value, fmap_transform<F>&& transform) -> decltype(auto) {
   return fmap(std::forward<F>(transform.func_), std::forward<T>(value));
}

template<typename T, typename F>
inline auto operator|(T&& value, const fmap_transform<F>& transform) -> decltype(auto) {
   return fmap(transform.func_, std::forward<T>(value));
}

template<typename T, typename FUNC>
inline auto operator|(T&& value, fmap_transform<FUNC>& transform) -> decltype(auto) {
   return fmap(transform.func_, std::forward<T>(value));
}

E_CORO_NS_END

#endif //E_CORO_FMAP_H
