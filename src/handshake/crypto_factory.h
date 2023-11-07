/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include "protocol/quic_constants.hpp"
#include "protocol/quic_packet_num_cipher.hpp"
#include "protocol/quic_connection_id.hpp"
#include "protocol/quic_header.hpp"


#include "Aead.hpp"

#include <memory>

namespace quic {

class CryptoFactory {
 public:
  std::unique_ptr<Aead> getClientInitialCipher(
      const ConnectionId& clientDestinationConnId,
      QuicVersion version) const;

  std::unique_ptr<Aead> getServerInitialCipher(
      const ConnectionId& clientDestinationConnId,
      QuicVersion version) const;

  Buf makeServerInitialTrafficSecret(
      const ConnectionId& clientDestinationConnId,
      QuicVersion version) const;
  Buf makeClientInitialTrafficSecret(
      const ConnectionId& clientDestinationConnId,
      QuicVersion version) const;

  /**
   * Makes the header cipher for writing client initial packets.
   */
  std::unique_ptr<PacketNumberCipher> makeClientInitialHeaderCipher(
      const ConnectionId& initialDestinationConnectionId,
      QuicVersion version) const;

  /**
   * Makes the header cipher for writing server initial packets.
   */
  std::unique_ptr<PacketNumberCipher> makeServerInitialHeaderCipher(
      const ConnectionId& initialDestinationConnectionId,
      QuicVersion version) const;

  /**
   * Crypto layer specifc methods.
   */
  virtual Buf makeInitialTrafficSecret(
      folly::StringPiece label,
      const ConnectionId& clientDestinationConnId,
      QuicVersion version) const = 0;

  virtual std::unique_ptr<Aead> makeInitialAead(
      folly::StringPiece label,
      const ConnectionId& clientDestinationConnId,
      QuicVersion version) const = 0;

  virtual std::unique_ptr<PacketNumberCipher> makePacketNumberCipher(
      folly::ByteRange baseSecret) const = 0;

  virtual ~CryptoFactory() = default;
};

} // namespace quic
