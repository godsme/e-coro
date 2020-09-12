cmake_minimum_required(VERSION 3.17)
project(e_coro)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wnon-virtual-dtor \
      -Wcast-align -Wunused -Woverloaded-virtual -Wconversion \
      -Wsign-conversion -Wdouble-promotion -Wformat=2 \
      -Wno-invalid-offsetof -fcoroutines -fno-rtti -fno-exceptions -DDEBUG=1 -DSPDLOG_NO_EXCEPTIONS \
      -Wno-builtin-macro-redefined \
      -D'__FILE__=\"$(subst $(realpath ${CMAKE_SOURCE_DIR})/,,$(abspath $<))\"'")

include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/third-party"
)

#add_library(e_coro include/e-coro/core/task.h include/e-coro/e_coro_ns.h)

add_executable(e_coro_test
        third-party/catch.hpp
        test/catch.cpp test/test_task.cpp include/e-coro/core/sync_wait_task.h include/e-coro/core/awaitable_trait.h)