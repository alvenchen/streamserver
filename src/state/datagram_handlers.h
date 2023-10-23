/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "../protocol/quic_frame.hpp"
#include "../protocol/quic_constants.hpp"
#include "../state/state_data.h"

namespace quic {

/**
 * Processes a Datagram frame
 */
void handleDatagram(
    QuicConnectionStateBase& conn,
    DatagramFrame& frame,
    TimePoint recvTimePoint);

} // namespace quic
