//
// Created by Darwin Yuan on 2020/9/12.
//

#ifndef E_CORO_SYNC_WAIT_TASK_H
#define E_CORO_SYNC_WAIT_TASK_H

#include <e-coro/e_coro_ns.h>
#include <e-coro/core/awaitable_trait.h>
#include <coroutine>
#include <memory>
#include <future>

E_CORO_NS_BEGIN

namespace detail {
   template<typename R>
   struct sync_wait_task;

   using sync_wait_notifier = std::promise<void>;

   template<typename P>
   struct sync_wait_task_promise_base {
      using handle_type = std::coroutine_handle<P>;

      auto initial_suspend() noexcept {
         return std::suspend_always{};
      }

      auto final_suspend() noexcept {
         struct completion_notifier {
            bool await_ready() const noexcept { return false; }
            void await_suspend(handle_type self) const noexcept {
               self.promise().notifier_.set_value();
            }
            void await_resume() noexcept {}
         };
         return completion_notifier{};
      }

   protected:
      sync_wait_notifier notifier_;
   };

   template<typename R>
   struct sync_wait_task_promise final
      : sync_wait_task_promise_base<sync_wait_task_promise<R>> {
      using super = sync_wait_task_promise_base<sync_wait_task_promise<R>>;
      using value_type = std::remove_reference_t<R>;

      auto get_return_object() noexcept {
         return super::handle_type::from_promise(*this);
      }

      auto yield_value(R&& result) noexcept {
         result_ = std::addressof(result);
         return super::final_suspend();
      }

      auto result() noexcept -> value_type&& {
         return static_cast<value_type&&>(*result_);
      }

      auto start(sync_wait_notifier&& notifier) noexcept {
         super::notifier_ = std::move(notifier);
         super::handle_type::from_promise(*this).resume();
      }

   private:
      value_type* result_;
   };

   template<>
   struct sync_wait_task_promise<void> final
      : sync_wait_task_promise_base<sync_wait_task_promise<void>> {
      using super = sync_wait_task_promise_base<sync_wait_task_promise<void>>;

      auto get_return_object() noexcept {
         return super::handle_type::from_promise(*this);
      }

      void return_void() noexcept {}
      auto result() noexcept {}

      auto start(sync_wait_notifier &&notifier) noexcept {
         super::notifier_ = std::move(notifier);
         super::handle_type::from_promise(*this).resume();
      }
   };

   template<typename RESULT>
   struct sync_wait_task final {
      using promise_type = sync_wait_task_promise<RESULT>;
      using handle_type = std::coroutine_handle<promise_type>;

      sync_wait_task(handle_type self) noexcept
         : self_{self} {}

      sync_wait_task(sync_wait_task &&other) noexcept
         : self_(std::exchange(other.self_, handle_type{})) {}

      ~sync_wait_task() noexcept {
         if (self_) self_.destroy();
      }

      sync_wait_task(sync_wait_task const &) noexcept = delete;
      sync_wait_task &operator=(sync_wait_task const &) noexcept = delete;

      void start(sync_wait_notifier &&notifier) noexcept {
         self_.promise().start(std::move(notifier));
      }

      auto result() noexcept -> decltype(auto) {
         return self_.promise().result();
      }

   private:
      handle_type self_;
   };

   template<void_awaitable T>
   auto make_sync_wait_task(T&& awaitable) noexcept -> sync_wait_task<void> {
      co_await std::forward<T>(awaitable);
   }

   template<non_void_awaitable T>
   auto make_sync_wait_task(T&& awaitable) noexcept -> sync_wait_task<await_result_t<T>> {
      co_yield co_await std::forward<T>(awaitable);
   }
}

template<typename AWAITABLE>
auto sync_wait(AWAITABLE&& awaitable) noexcept -> decltype(auto) {
   auto task = detail::make_sync_wait_task(std::forward<AWAITABLE>(awaitable));

   detail::sync_wait_notifier notifier;
   auto future = notifier.get_future();
   task.start(std::move(notifier));
   future.wait();

   return task.result();
}

E_CORO_NS_END

#endif //E_CORO_SYNC_WAIT_TASK_H
