//
// Created by Darwin Yuan on 2020/9/13.
//

#ifndef E_CORO_WHEN_ALL_READY_AWAITABLE_H
#define E_CORO_WHEN_ALL_READY_AWAITABLE_H

#include <e-coro/e_coro_ns.h>
#include <e-coro/core/detail/when_all_counter.h>
#include <coroutine>
#include <tuple>

E_CORO_NS_BEGIN namespace detail {

template<typename TASK_CONTAINER>
struct when_all_ready_awaitable;

template<typename ... TASKS>
struct when_all_ready_awaitable<std::tuple<TASKS...>> {
   explicit when_all_ready_awaitable(std::tuple<TASKS...>&& tasks)
      : counter_{sizeof...(TASKS)}
      , tasks_{std::move(tasks)}
   {}

   explicit when_all_ready_awaitable(TASKS&&... tasks)
      : when_all_ready_awaitable{std::tuple{std::move(tasks)...}}
   {}

private:
   struct awaiter_base {
      explicit awaiter_base(when_all_ready_awaitable& awaitable) noexcept
         : self_(awaitable)
      {}

      auto await_ready() const noexcept {
         return self_.is_ready();
      }

      // try_await will return true if there are still tasks.
      auto await_suspend(std::coroutine_handle<> awaiting) noexcept -> bool {
         return self_.try_await(awaiting);
      }

      when_all_ready_awaitable& self_;
   };

public:
   auto operator co_await() & noexcept {
      struct awaiter : awaiter_base {
         using awaiter_base::awaiter_base;
         auto await_resume() noexcept -> std::tuple<TASKS...>& {
            return awaiter_base::self_.tasks_;
         }
      };
      return awaiter{ *this };
   }

   auto operator co_await() && noexcept {
      struct awaiter : awaiter_base {
         using awaiter_base::awaiter_base;
         auto await_resume() noexcept -> std::tuple<TASKS...>&& {
            return std::move(awaiter_base::self_.tasks_);
         }
      };
      return awaiter{ *this };
   }

private:
   auto is_ready() const noexcept {
      return counter_.is_ready();
   }

   template<std::size_t... I>
   inline auto start_tasks(std::integer_sequence<std::size_t, I...>) noexcept {
      (std::get<I>(tasks_).start(counter_), ...);
   }

   auto try_await(std::coroutine_handle<> awaiting) noexcept -> bool {
      start_tasks(std::index_sequence_for<TASKS...>{});
      return counter_.try_await(awaiting);
   }

private:
   when_all_counter     counter_;
   std::tuple<TASKS...> tasks_;
};

template<>
struct when_all_ready_awaitable<std::tuple<>> {
   constexpr when_all_ready_awaitable() noexcept {}
   explicit constexpr when_all_ready_awaitable(std::tuple<>) noexcept {}

   // no task, don't suspend.
   constexpr auto await_ready() const noexcept { return true; }
   auto await_suspend(std::coroutine_handle<>) noexcept {}
   auto await_resume() const noexcept { return std::tuple<>{}; }
};

} E_CORO_NS_END

#endif //E_CORO_WHEN_ALL_READY_AWAITABLE_H
