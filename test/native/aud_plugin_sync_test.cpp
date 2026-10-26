// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// The semaphore of the engine's control thread (aud_plugin_sync.hpp): a
// count that a post raises and a wait takes, with a timeout.

#include "aud_plugin_sync.hpp"

#include <atomic>
#include <chrono>
#include <thread>

#include "aud_plugin_harness.hpp"

using namespace aud_plugin;
using aud_plugin_test::expectEqual;
using aud_plugin_test::group;
using aud_plugin_test::test;

int main() {
  group("waitFor(timeoutNs)", [] {
    test("takes a count that a post raised", [] {
      Semaphore semaphore;
      semaphore.post();
      expectEqual(semaphore.waitFor(0), true);
    });
    test("times out without a count", [] {
      Semaphore semaphore;
      const auto start = std::chrono::steady_clock::now();
      expectEqual(semaphore.waitFor(5'000'000), false);
      expectEqual(std::chrono::steady_clock::now() - start >= std::chrono::milliseconds(4), true);
    });
    test("takes one count per wait", [] {
      Semaphore semaphore;
      semaphore.post();
      semaphore.post();
      expectEqual(semaphore.waitFor(0), true);
      expectEqual(semaphore.waitFor(0), true);
      expectEqual(semaphore.waitFor(0), false);
    });
    test("wakes a waiter when another thread posts", [] {
      Semaphore semaphore;
      std::atomic<bool> woken{false};
      std::thread waiter([&] { woken = semaphore.waitFor(2'000'000'000); });
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      semaphore.post();
      waiter.join();
      expectEqual(woken.load(), true);
    });
  });

  return aud_plugin_test::finish();
}
