//
// Created by Darwin Yuan on 2020/9/12.
//

#ifndef E_CORO_TASK_H
#define E_CORO_TASK_H

#include <e-coro/e_coro_ns.h>
#include <coroutine>
#include <concepts>
#include <variant>

E_CORO_NS_BEGIN

template<typename T> struct task;

namespace detail {

   struct task_promise_base {
      friend struct final_awaitable;
      struct final_awaitable {
         auto await_ready() const noexcept { return false; }
         template<typename P> requires std::is_base_of_v<task_promise_base, P>
         auto await_suspend(std::coroutine_handle<P> handle) noexcept {
            return handle.promise().continuation_;
         }
         auto await_resume() noexcept {}
      };

   public:
      auto initial_suspend() noexcept {
         return std::suspend_always{};
      }

      auto final_suspend() noexcept {
         return final_awaitable{};
      }

      auto set_continuation(std::coroutine_handle<> continuation) {
         continuation_ = continuation;
      }
   private:
      std::coroutine_handle<> continuation_;
   };

   template<typename T>
   struct task_promise : task_promise_base {
      template<std::convertible_to<T> V>
      auto return_value(V&& value) noexcept {
         value_ = std::forward<V>(value);
      }

      auto get_return_object() noexcept -> task<T>;

      auto result() & noexcept -> std::optional<T&> {
         if(value_.index() == 1) {
            return std::get<1>(value_);
         } else {
            return std::nullopt;
         }
      }

      auto result() && noexcept -> std::optional<T&&> {
         if(value_.index() == 1) {
            return std::move(std::get<1>(value_));
         } else {
            return std::nullopt;
         }
      }

   private:
      std::variant<std::monostate, T> value_;
   };

   template<>
   struct task_promise<void> : task_promise_base {
      auto return_void() noexcept {}
      auto get_return_object() noexcept -> task<void>;
      auto result() {}
   };
}

template <typename T>
struct [[nodiscard("it will be destroyed automatically otherwise")]] task {
   using promise_type = detail::task_promise<T>;

private:
   using handle_type = std::coroutine_handle<promise_type>;
   struct awaitable_base {
      awaitable_base(handle_type self) noexcept
         : self_{self} {}

      auto await_ready() const noexcept {
         return !self_ || self_.done();
      }

      auto await_suspend(std::coroutine_handle<> caller) noexcept {
         self_.promise().set_continuation(caller);
         return self_;
      }

   protected:
      handle_type self_;
   };

public:
   explicit task(handle_type handle)
      : self_{handle}
   {}

   task(task&& rhs) : self_{std::move(rhs.self_)} {
      rhs.self_ = nullptr;
   }

   task(task const&) = delete;
   task& operator=(task const&) = delete;

   auto operator=(task rhs) -> task& {
      *this = std::move(rhs);
      return *this;
   }

   ~task() {
      if(self_) {
         self_.destroy();
      }
   }

   auto operator co_await() const& noexcept {
      struct awaitable : awaitable_base {
         using awaitable_base::awaitable_base;
         auto await_resume() noexcept -> decltype(auto) {
            return self_.promise().result();
         }
      };

      return awaitable{ self_ };
   }

   auto operator co_await() const&& noexcept {
      struct awaitable : awaitable_base {
         using awaitable_base::awaitable_base;
         auto await_resume() noexcept -> decltype(auto) {
            return std::move(self_.promise()).result();
         }
      };

      return awaitable{ self_ };
   }

private:
   handle_type self_;
};

namespace detail {
   template<typename T>
   inline auto task_promise<T>::get_return_object() noexcept -> task<T> {
      return task<T>{ std::coroutine_handle<task_promise>::from_promise(*this) };
   }

   inline task<void> task_promise<void>::get_return_object() noexcept {
      return task<void>{ std::coroutine_handle<task_promise>::from_promise(*this) };
   }
}

E_CORO_NS_END

#endif //E_CORO_TASK_H
