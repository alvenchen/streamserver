/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "state/quic_stream_function.h"
#include "state/quic_stream_utilities.h"

namespace quic {

void receiveReadStreamFrameSMHandler(
    QuicStreamState& stream,
    ReadStreamFrame&& frame);

void receiveRstStreamSMHandler(
    QuicStreamState& stream,
    const RstStreamFrame& rst);

} // namespace quic
