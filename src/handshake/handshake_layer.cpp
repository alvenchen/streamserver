/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "handshake_layer.hpp"

namespace quic {

EncryptionLevel protectionTypeToEncryptionLevel(ProtectionType type) {
    switch (type) {
    case ProtectionType::Initial:
        return EncryptionLevel::Initial;
    case ProtectionType::Handshake:
        return EncryptionLevel::Handshake;
    case ProtectionType::ZeroRtt:
        return EncryptionLevel::EarlyData;
    case ProtectionType::KeyPhaseZero:
    case ProtectionType::KeyPhaseOne:
        return EncryptionLevel::AppData;
    }
    folly::assume_unreachable();
}

} // namespace quic
