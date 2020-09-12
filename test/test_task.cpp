//
// Created by Darwin Yuan on 2020/9/12.
//

#include <catch.hpp>
#include <e-coro/core/task.h>
#include <e-coro/core/sync_wait_task.h>

namespace {

   TEST_CASE("task doesn't start until awaited") {
      bool started = false;
      auto func = [&]() -> e_coro::task<>
      {
         started = true;
         co_return;
      };

      e_coro::sync_wait([&]() -> e_coro::task<>
       {
          auto t = func();
          REQUIRE(!started);

          co_await t;

          REQUIRE(started);
       }());
   }
}