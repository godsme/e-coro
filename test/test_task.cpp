//
// Created by Darwin Yuan on 2020/9/12.
//

#include <catch.hpp>
#include <e-coro/core/task.h>
#include <e-coro/core/sync_wait_task.h>
#include <e-coro/core/when_all_ready.h>
#include <e-coro/core/single_consumer_event.h>
#include "counted.h"

namespace {

   TEST_CASE("task doesn't start until awaited") {
      bool started = false;
      auto func = [&]() -> e_coro::task<> {
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
      bool reached_before_event = false;
      bool reached_after_event = false;
      e_coro::single_consumer_event event;
      auto f = [&]() -> e_coro::task<> {
         reached_before_event = true;
         co_await event;
         reached_after_event = true;
      };

      e_coro::sync_wait([&]() -> e_coro::task<> {
          auto t = f();

         REQUIRE(!reached_before_event);

          (void)co_await e_coro::when_all_ready(
             [&]() -> e_coro::task<> {
                co_await t;
                REQUIRE(reached_before_event);
                REQUIRE(reached_after_event);
             }(),
             [&]() -> e_coro::task<> {
                REQUIRE(reached_before_event);
                REQUIRE(!reached_after_event);
                event.set();
                REQUIRE(reached_after_event);
                co_return;
             }());
       }());
   }

   TEST_CASE("destroying task that was never awaited destroys captured args") {
      counted::reset_counts();

      auto f = [](counted c) -> e_coro::task<counted> {
         co_return c;
      };

      REQUIRE(counted::active_count() == 0);

      {
         auto t = f(counted{});
         REQUIRE(counted::active_count() == 1);
      }

      REQUIRE(counted::active_count() == 0);
   }

   TEST_CASE("task destructor destroys result") {
      counted::reset_counts();

      auto f = []() -> e_coro::task<counted> {
         co_return counted{};
      };

      {
         auto t = f();
         REQUIRE(counted::active_count() == 0);

         auto& result = e_coro::sync_wait(t);

         REQUIRE(counted::active_count() == 1);
         REQUIRE(result->id == 0);
      }

      REQUIRE(counted::active_count() == 0);
   }

}