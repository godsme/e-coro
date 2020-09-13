//
// Created by Darwin Yuan on 2020/9/12.
//

#ifndef E_CORO_TASK_H
#define E_CORO_TASK_H

#include <e-coro/e_coro_ns.h>
#include <coroutine>
#include <concepts>
#include <optional>

E_CORO_NS_BEGIN

template<typename T> struct task;

namespace detail {

   struct task_promise_base {
      friend struct final_awaitable;
      struct final_awaitable {
         auto await_ready() const noexcept { return false; }
         template<typename P> requires std::is_base_of_v<task_promise_base, P>
         auto await_suspend(std::coroutine_handle<P> self) noexcept {
            // i'm done here, return the execution to caller.
            return self.promise().caller_;
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

      auto save_caller(std::coroutine_handle<> caller) noexcept {
         caller_ = caller;
      }

   private:
      std::coroutine_handle<> caller_;
   };

   template<typename T>
   struct task_promise : task_promise_base {
      auto return_value(T&& value) noexcept {
         value_.template emplace<T>(std::forward<T>(value));
      }

      auto get_return_object() noexcept -> task<T>;

      auto result() & noexcept -> T& {
         return *value_;
      }

      auto result() && noexcept -> T&& {
         return std::move(*value_);
      }

   private:
      std::optional<T> value_;
   };

   template<>
   struct task_promise<void> final : task_promise_base {
      auto return_void() noexcept {}
      auto get_return_object() noexcept -> task<void>;
      auto result() noexcept {}
   };

   template<typename T>
   struct task_promise<T&> final : task_promise_base {
      task<T&> get_return_object() noexcept;

      auto return_value(T& value) noexcept {
         m_value = std::addressof(value);
      }

      auto result() noexcept -> T& {
         return *m_value;
      }

   private:
      T* m_value;
   };
}

template <typename T = void>
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
         // When caller awaits me, if I'm not done yet, caller will be suspended.
         // Save my caller so that it will be resumed after I'm done.
         self_.promise().save_caller(caller);
         // i'm gonna be resumed.
         return self_;
      }

   protected:
      handle_type self_;
   };

public:
   explicit task(handle_type handle) noexcept
      : self_{handle}
   {}

   task(task&& rhs) noexcept : self_{std::move(rhs.self_)} {
      rhs.self_ = nullptr;
   }

   task(task const&) noexcept = delete;
   task& operator=(task const&) noexcept = delete;

   auto operator=(task&& rhs) noexcept -> task& {
      *this = std::move(rhs);
      return *this;
   }

   ~task() noexcept {
      if(self_) self_.destroy();
   }

   // caller awaits me, and i'm a lvalue-reference
   auto operator co_await() const& noexcept {
      struct awaitable : awaitable_base {
         using awaitable_base::awaitable_base;
         auto await_resume() noexcept -> decltype(auto) {
            return awaitable_base::self_.promise().result();
         }
      };
      return awaitable{ self_ };
   }

   // caller awaits me, and i'm a rvalue-reference
   auto operator co_await() const&& noexcept {
      struct awaitable : awaitable_base {
         using awaitable_base::awaitable_base;
         auto await_resume() noexcept -> decltype(auto) {
            return std::move(awaitable_base::self_.promise()).result();
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

   inline auto task_promise<void>::get_return_object() noexcept -> task<void> {
      return task<void>{ std::coroutine_handle<task_promise>::from_promise(*this) };
   }

   template<typename T>
   inline auto task_promise<T&>::get_return_object() noexcept -> task<T&> {
      return task<T&>{ std::coroutine_handle<task_promise>::from_promise(*this) };
   }
}

E_CORO_NS_END

#endif //E_CORO_TASK_H
