/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "../../protocol/quic_constants.hpp"
#include "../state_data.h"

namespace quic {

// Common operations to conduct on QuicStreamState when send reset on it
void resetQuicStream(QuicStreamState& stream, ApplicationErrorCode error);

// Comon operations to conduct on QuicStreamState when receive reset on it
void onResetQuicStream(QuicStreamState& stream, const RstStreamFrame& frame);

bool isAllDataReceived(const QuicStreamState& stream);
} // namespace quic
