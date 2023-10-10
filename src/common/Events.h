/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#ifdef MVFST_USE_LIBEV
#include <quic/common/EventsMobile.h>

#include <ev.h>
#else
#include <folly/io/async/EventBase.h>
FOLLY_GNU_DISABLE_WARNING("-Wdeprecated-declarations")
#endif

#include "QuicEventBaseInterface.h"
#include <functional>

namespace quic {

#ifdef MVFST_USE_LIBEV
#else
using QuicEventBaseLoopCallback = folly::EventBase::LoopCallback;
using QuicBackingEventBase = folly::EventBase;
using QuicAsyncTimeout = folly::AsyncTimeout;
using QuicHHWheelTimer = folly::HHWheelTimer;
#endif

class QuicEventBase : public QuicEventBaseInterface<
                          QuicEventBaseLoopCallback,
                          QuicBackingEventBase,
                          QuicAsyncTimeout,
                          QuicHHWheelTimer> {
 public:
  QuicEventBase() = default;
  explicit QuicEventBase(QuicBackingEventBase* evb) : backingEvb_(evb) {}
  ~QuicEventBase() override = default;

  using LoopCallback = QuicEventBaseLoopCallback;

  void setBackingEventBase(QuicBackingEventBase* evb) override;

  [[nodiscard]] QuicBackingEventBase* getBackingEventBase() const override;

  void runInLoop(
      QuicEventBaseLoopCallback* callback,
      bool thisIteration = false) override;

  void runInLoop(std::function<void()> cb, bool thisIteration = false) override;

  void runAfterDelay(std::function<void()> cb, uint32_t milliseconds) override;

  void runInEventBaseThreadAndWait(std::function<void()> fn) noexcept override;

  [[nodiscard]] bool isInEventBaseThread() const override;

  bool scheduleTimeoutHighRes(
      QuicAsyncTimeout* obj,
      std::chrono::microseconds timeout) override;

  QuicHHWheelTimer& timer() override;

  bool loopOnce(int flags = 0) override;

  bool loop() override;

  void loopForever() override;

  bool loopIgnoreKeepAlive() override;

  void terminateLoopSoon() override;

 private:
  QuicBackingEventBase* backingEvb_{nullptr};
};

} // namespace quic
