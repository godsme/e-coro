//
// Created by Darwin Yuan on 2020/9/13.
//

#ifndef E_CORO_WHEN_ALL_READY_H
#define E_CORO_WHEN_ALL_READY_H

#include <e-coro/e_coro_ns.h>
#include <e-coro/core/detail/when_all_ready_awaitable.h>
#include <e-coro/core/detail/when_all_task.h>
#include <e-coro/core/awaitable_trait.h>

E_CORO_NS_BEGIN

template<typename... Xs>
[[nodiscard("this is an awaitable")]]
inline auto when_all_ready(Xs&&... xs) {
   using result_t =
      detail::when_all_ready_awaitable<
         std::tuple<
            detail::when_all_task<
               await_result_t<std::decay_t<Xs>>>...>>;
   return result_t{
      std::make_tuple(detail::make_when_all_task(std::forward<Xs>(xs))...)};
}

E_CORO_NS_END

#endif //E_CORO_WHEN_ALL_READY_H
