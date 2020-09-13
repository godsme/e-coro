//
// Created by Darwin Yuan on 2020/9/13.
//

#ifndef E_CORO_WHEN_ALL_COUNTER_H
#define E_CORO_WHEN_ALL_COUNTER_H

#include <e-coro/e_coro_ns.h>
#include <cstddef>
#include <atomic>
#include <coroutine>

E_CORO_NS_BEGIN namespace detail {

struct when_all_counter {
   when_all_counter(std::size_t count) noexcept
      : count_(count + 1)
      , awaiting_(nullptr)
   {}

   auto is_ready() const noexcept -> bool {
      return static_cast<bool>(awaiting_);
   }

   auto try_await(std::coroutine_handle<> awaiting) noexcept -> bool {
      awaiting_ = awaiting;
      return count_.fetch_sub(1, std::memory_order_acq_rel) > 1;
   }

   auto notify_awaitable_completed() noexcept {
      if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
         awaiting_.resume();
      }
   }

protected:
   std::atomic<std::size_t> count_;
   std::coroutine_handle<>  awaiting_;
};

} E_CORO_NS_END

#endif //E_CORO_WHEN_ALL_COUNTER_H
