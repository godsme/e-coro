//
// Created by Darwin Yuan on 2020/9/12.
//

#include <catch.hpp>
#include <e-coro/core/task.h>
#include <e-coro/core/sync_wait_task.h>
#include <e-coro/core/when_all_ready.h>
#include <e-coro/core/single_consumer_event.h>
#include <e-coro/core/fmap.h>
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
         REQUIRE(result.id == 0);
      }

      REQUIRE(counted::active_count() == 0);
   }

   TEST_CASE("task of reference type")
   {
      int value = 3;
      auto f = [&]() -> e_coro::task<int&> {
         co_return value;
      };

      e_coro::sync_wait([&]() -> e_coro::task<> {
          {
             decltype(auto) result = co_await f();
             static_assert(
                std::is_same<decltype(result), int&>::value,
                "co_await r-value reference of task<int&> should result in an int&");
             REQUIRE(&result == &value);
          }

          {
             auto t = f();
             decltype(auto) result = co_await t;
             static_assert(
                std::is_same<decltype(result), int&>::value,
                "co_await l-value reference of task<int&> should result in an int&");
             REQUIRE(&result == &value);
          }
       }());
   }

   TEST_CASE("passing parameter by value to task coroutine calls move-constructor exactly once") {
      counted::reset_counts();

      auto f = [](counted) -> e_coro::task<> {
         co_return;
      };

      counted c;

      REQUIRE(counted::active_count() == 1);
      REQUIRE(counted::default_construction_count == 1);
      REQUIRE(counted::copy_construction_count == 0);
      REQUIRE(counted::move_construction_count == 0);
      REQUIRE(counted::destruction_count == 0);

      {
         auto t = f(c);

         // Should have called copy-constructor to pass a copy of 'c' into f by value.
         REQUIRE(counted::copy_construction_count == 1);

         // Inside f it should have move-constructed parameter into coroutine frame variable
         // WARN_MESSAGE(counted::move_construction_count == 1,

         // Active counts should be the instance 'c' and the instance captured in coroutine frame of 't'.
         REQUIRE(counted::active_count() == 2);
      }

      REQUIRE(counted::active_count() == 1);
   }

   TEST_CASE("task<void> fmap pipe operator") {
      using e_coro::fmap;

      e_coro::single_consumer_event event;

      auto f = [&]() -> e_coro::task<> {
         co_await event;
         co_return;
      };

      auto t = f() | fmap([] { return 123; });

      e_coro::sync_wait(e_coro::when_all_ready(
         [&]() -> e_coro::task<> {
            CHECK(co_await t == 123);
         }(),
         [&]() -> e_coro::task<> {
            event.set();
            co_return;
         }()));
   }

   TEST_CASE("task<int> fmap pipe operator") {
      using e_coro::task;
      using e_coro::fmap;
      using e_coro::sync_wait;
      using e_coro::make_task;

      auto one = [&]() -> task<int> {
         co_return 1;
      };

      SECTION("r-value fmap / r-value lambda") {
         auto t = one()
                  | fmap([delta = 1](auto i) { return i + delta; });
         REQUIRE(sync_wait(t) == 2);
      }

      SECTION("r-value fmap / l-value lambda") {
         using namespace std::string_literals;

         auto t = [&]
         {
            auto f = [prefix = "pfx"s](int x)
            {
               return prefix + std::to_string(x);
            };

            // Want to make sure that the resulting awaitable has taken
            // a copy of the lambda passed to fmap().
            return one() | fmap(f);
         }();

         REQUIRE(sync_wait(t) == "pfx1");
      }

      SECTION("l-value fmap / r-value lambda") {
         using namespace std::string_literals;

         auto t = [&] {
            auto addprefix = fmap([prefix = "a really really long prefix that prevents small string optimisation"s](int x)
                                  {
                                     return prefix + std::to_string(x);
                                  });

            // Want to make sure that the resulting awaitable has taken
            // a copy of the lambda passed to fmap().
            return one() | addprefix;
         }();

         REQUIRE(sync_wait(t) == "a really really long prefix that prevents small string optimisation1");
      }

      SECTION("l-value fmap / l-value lambda") {
         using namespace std::string_literals;

         task<std::string> t;

         {
            auto lambda = [prefix = "a really really long prefix that prevents small string optimisation"s](int x) {
               return prefix + std::to_string(x);
            };

            auto addprefix = fmap(lambda);

            // Want to make sure that the resulting task has taken
            // a copy of the lambda passed to fmap().
            t = make_task(one() | addprefix);
         }

         REQUIRE(!t.is_ready());

         REQUIRE(sync_wait(t) == "a really really long prefix that prevents small string optimisation1");
      }
   }
}