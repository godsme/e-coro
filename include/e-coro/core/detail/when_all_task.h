//
// Created by Darwin Yuan on 2020/9/13.
//

#ifndef E_CORO_WHEN_ALL_TASK_H
#define E_CORO_WHEN_ALL_TASK_H

#include <e-coro/core/detail/when_all_counter.h>
#include <e-coro/core/awaitable_trait.h>
#include <cstddef>
#include <atomic>
#include <coroutine>
#include <utility>

E_CORO_NS_BEGIN namespace detail {

template<typename C>
struct when_all_ready_awaitable;

template<typename R>
struct when_all_task;

template<typename R>
struct when_all_task_promise final {
   using handle_type = std::coroutine_handle<when_all_task_promise<R>>;

   auto get_return_object() noexcept {
      return handle_type::from_promise(*this);
   }

   auto initial_suspend() noexcept {
      return std::suspend_always{};
   }

   auto final_suspend() noexcept {
      struct completion_notifier {
         bool await_ready() const noexcept { return false; }
         void await_suspend(handle_type self) const noexcept {
            self.promise().counter_->notify_awaitable_completed();
         }
         void await_resume() const noexcept {}
      };
      return completion_notifier{};
   }

   auto yield_value(R&& result) noexcept {
      result_ = std::addressof(result);
      return final_suspend();
   }

   auto start(when_all_counter& counter) noexcept {
      counter_ = &counter;
      handle_type::from_promise(*this).resume();
   }

   auto result() & -> R& {
      return *result_;
   }

   auto result() && -> R&& {
      return std::move(*result_);
   }

private:
   when_all_counter* counter_;
   R result_;
};

template<>
struct when_all_task_promise<void> final {
   using handle_type = std::coroutine_handle<when_all_task_promise<void>>;

   auto get_return_object() noexcept {
      return handle_type::from_promise(*this);
   }

   auto initial_suspend() noexcept {
      return std::suspend_always{};
   }

   auto final_suspend() noexcept {
      struct completion_notifier {
         bool await_ready() const noexcept { return false; }
         void await_suspend(handle_type self) const noexcept {
            self.promise().counter_->notify_awaitable_completed();
         }
         void await_resume() const noexcept {}
      };
      return completion_notifier{};
   }

   void return_void() noexcept {}

   void start(when_all_counter& counter) noexcept {
      counter_ = &counter;
      handle_type::from_promise(*this).resume();
   }

   void result() {}

private:
   when_all_counter* counter_;
};

struct void_value {};

template<typename R>
struct when_all_task final {
   using promise_type = when_all_task_promise<R>;
   using handle_type = typename promise_type::handle_type;

   when_all_task(handle_type self) noexcept
      : self_(self) {}

   when_all_task(when_all_task &&other) noexcept
      : self_(std::exchange(other.self_, handle_type{})) {}

   ~when_all_task() {
      if (self_) self_.destroy();
   }

   when_all_task(const when_all_task &) = delete;
   when_all_task &operator=(const when_all_task &) = delete;

   auto result() & -> decltype(auto) {
      return self_.promise().result();
   }

   auto result() && -> decltype(auto) {
      return std::move(self_.promise()).result();
   }

   auto non_void_result() & -> decltype(auto) {
      if constexpr (std::is_void_v<decltype(this->result())>) {
         this->result();
         return void_value{};
      } else {
         return this->result();
      }
   }

   auto non_void_result() && -> decltype(auto) {
      if constexpr (std::is_void_v<decltype(this->result())>) {
         std::move(*this).result();
         return void_value{};
      } else {
         return std::move(*this).result();
      }
   }

private:
   template<typename TASK_CONTAINER>
   friend class when_all_ready_awaitable;

   void start(when_all_counter& counter) noexcept {
      self_.promise().start(counter);
   }

private:
   handle_type self_;
};

template<void_awaitable T>
auto make_when_all_task(T awaitable) -> when_all_task<await_result_t<T>> {
   co_await static_cast<T&&>(awaitable);
}

template<non_void_awaitable T>
auto make_when_all_task(T awaitable) -> when_all_task<void> {
   co_yield co_await static_cast<T&&>(awaitable);
}

} E_CORO_NS_END

#endif //E_CORO_WHEN_ALL_TASK_H
