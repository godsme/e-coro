//
// Created by Darwin Yuan on 2020/9/13.
//

#ifndef E_CORO_SINGLE_CONSUMER_EVENT_H
#define E_CORO_SINGLE_CONSUMER_EVENT_H

#include <e-coro/e_coro_ns.h>
#include <atomic>
#include <coroutine>

E_CORO_NS_BEGIN

struct single_consumer_event {
   single_consumer_event(bool initiallySet = false) noexcept
      : state_(initiallySet ? state::set : state::not_set)
   {}

   auto is_set() const noexcept {
      return state_.load(std::memory_order_acquire) == state::set;
   }

   auto set() {
      const state old_state = state_.exchange(state::set, std::memory_order_acq_rel);
      if (old_state == state::not_set_consumer_waiting) {
         self_.resume();
      }
   }

   auto reset() noexcept {
      state old_state = state::set;
      state_.compare_exchange_strong(old_state, state::not_set, std::memory_order_relaxed);
   }

   auto operator co_await() noexcept {
      struct awaiter {
         awaiter(single_consumer_event& event) : event_(event) {}

         auto await_ready() const noexcept {
            return event_.is_set();
         }

         auto await_suspend(std::coroutine_handle<> self) {
            event_.self_ = self;
            state old_state = state::not_set;
            return event_.state_.compare_exchange_strong(
               old_state,
               state::not_set_consumer_waiting,
               std::memory_order_release,
               std::memory_order_acquire);
         }

         auto await_resume() noexcept {}

      private:
         single_consumer_event& event_;
      };

      return awaiter{ *this };
   }

private:

   enum class state {
      not_set,
      not_set_consumer_waiting,
      set
   };

   std::atomic<state> state_;
   std::coroutine_handle<> self_;
};

E_CORO_NS_END

#endif //E_CORO_SINGLE_CONSUMER_EVENT_H
