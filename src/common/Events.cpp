/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef MVFST_USE_LIBEV

#include "Events.h"

namespace quic {

void QuicEventBase::setBackingEventBase(QuicBackingEventBase* evb) {
  backingEvb_ = evb;
}

QuicBackingEventBase* QuicEventBase::getBackingEventBase() const {
  return backingEvb_;
}

void QuicEventBase::runInLoop(
    QuicEventBaseLoopCallback* callback,
    bool thisIteration) {
  return backingEvb_->runInLoop(callback, thisIteration);
}

void QuicEventBase::runInLoop(std::function<void()> cb, bool thisIteration) {
  return backingEvb_->runInLoop(std::move(cb), thisIteration);
}

void QuicEventBase::runAfterDelay(
    std::function<void()> cb,
    uint32_t milliseconds) {
  return backingEvb_->runAfterDelay(std::move(cb), milliseconds);
}

void QuicEventBase::runInEventBaseThreadAndWait(
    std::function<void()> fn) noexcept {
  return backingEvb_->runInEventBaseThreadAndWait(std::move(fn));
}

bool QuicEventBase::isInEventBaseThread() const {
  return backingEvb_->isInEventBaseThread();
}

bool QuicEventBase::scheduleTimeoutHighRes(
    QuicAsyncTimeout* obj,
    std::chrono::microseconds timeout) {
  return backingEvb_->scheduleTimeoutHighRes(obj, timeout);
}

QuicHHWheelTimer& QuicEventBase::timer() {
  return backingEvb_->timer();
}

bool QuicEventBase::loopOnce(int flags) {
  return backingEvb_->loopOnce(flags);
}

bool QuicEventBase::loop() {
  return backingEvb_->loop();
}

void QuicEventBase::loopForever() {
  return backingEvb_->loopForever();
}

bool QuicEventBase::loopIgnoreKeepAlive() {
  return backingEvb_->loopIgnoreKeepAlive();
}

void QuicEventBase::terminateLoopSoon() {
  return backingEvb_->terminateLoopSoon();
}

} // namespace quic

#endif // !MVFST_USE_LIBEV
