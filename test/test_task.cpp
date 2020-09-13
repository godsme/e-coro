//
// Created by Darwin Yuan on 2020/9/12.
//

#include <catch.hpp>
#include <e-coro/core/task.h>
#include <e-coro/core/sync_wait_task.h>
#include <e-coro/core/when_all_ready.h>
#include <e-coro/core/single_consumer_event.h>

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

   TEST_CASE("awaiting task that completes asynchronously")
   {
      bool reachedBeforeEvent = false;
      bool reachedAfterEvent = false;
      e_coro::single_consumer_event event;
      auto f = [&]() -> e_coro::task<>
      {
         reachedBeforeEvent = true;
         co_await event;
         reachedAfterEvent = true;
      };

      e_coro::sync_wait([&]() -> e_coro::task<> {
          auto t = f();

          CHECK(!reachedBeforeEvent);

          (void)co_await e_coro::when_all_ready(
             [&]() -> e_coro::task<>
             {
                co_await t;
                CHECK(reachedBeforeEvent);
                CHECK(reachedAfterEvent);
             }(),
             [&]() -> e_coro::task<>
             {
                CHECK(reachedBeforeEvent);
                CHECK(!reachedAfterEvent);
                event.set();
                CHECK(reachedAfterEvent);
                co_return;
             }());
       }());
   }
}