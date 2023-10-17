/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "base_qlogger.h"
#include "../protocol/quic_frame.hpp"
#include "../protocol/quic_exception.h"

namespace {
void addQuicSimpleFrameToEvent(
    quic::QLogPacketEvent* event,
    const quic::QuicSimpleFrame& simpleFrame) {
  switch (simpleFrame.type()) {
    case quic::QuicSimpleFrame::TYPE::STOP_SENDING_FRAME: {
      const quic::StopSendingFrame& frame = *simpleFrame.asStopSendingFrame();
      event->frames.push_back(std::make_unique<quic::StopSendingFrameLog>(
          frame.streamId, frame.errorCode));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::PATH_CHALLANGE_FRAME: {
      const quic::PathChallengeFrame& frame =
          *simpleFrame.asPathChallengeFrame();
      event->frames.push_back(
          std::make_unique<quic::PathChallengeFrameLog>(frame.pathData));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::PATH_RESPONSE_FRAME: {
      const quic::PathResponseFrame& frame = *simpleFrame.asPathResponseFrame();
      event->frames.push_back(
          std::make_unique<quic::PathResponseFrameLog>(frame.pathData));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::NEW_CONNECTION_ID_FRAME: {
      const quic::NewConnectionIdFrame& frame =
          *simpleFrame.asNewConnectionIdFrame();
      event->frames.push_back(std::make_unique<quic::NewConnectionIdFrameLog>(
          frame.sequenceNumber, frame.token));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::MAX_STREAMS_FRAME: {
      const quic::MaxStreamsFrame& frame = *simpleFrame.asMaxStreamsFrame();
      event->frames.push_back(std::make_unique<quic::MaxStreamsFrameLog>(
          frame.maxStreams, frame.isForBidirectional));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::RETIRE_CONNECTION_ID_FRAME: {
      const quic::RetireConnectionIdFrame& frame =
          *simpleFrame.asRetireConnectionIdFrame();
      event->frames.push_back(
          std::make_unique<quic::RetireConnectionIdFrameLog>(
              frame.sequenceNumber));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::HANDSHAKE_DONE_FRAME: {
      event->frames.push_back(std::make_unique<quic::HandshakeDoneFrameLog>());
      break;
    }
    case quic::QuicSimpleFrame::TYPE::KNOB_FRAME: {
      const quic::KnobFrame& frame = *simpleFrame.asKnobFrame();
      event->frames.push_back(std::make_unique<quic::KnobFrameLog>(
          frame.knobSpace, frame.id, frame.blob->length()));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::ACK_FREQUENCY_FRAME: {
      const quic::AckFrequencyFrame& frame = *simpleFrame.asAckFrequencyFrame();
      event->frames.push_back(std::make_unique<quic::AckFrequencyFrameLog>(
          frame.sequenceNumber,
          frame.packetTolerance,
          frame.updateMaxAckDelay,
          frame.reorderThreshold));
      break;
    }
    case quic::QuicSimpleFrame::TYPE::NEW_TOKEN_FRAME: {
      const quic::NewTokenFrame& frame = *simpleFrame.asNewTokenFrame();
      auto tokenHexStr = folly::hexlify(frame.token->coalesce());
      event->frames.push_back(
          std::make_unique<quic::NewTokenFrameLog>(tokenHexStr));
      break;
    }
  }
}
} // namespace

namespace quic {

std::unique_ptr<QLogPacketEvent> BaseQLogger::createPacketEvent(
    const RegularQuicPacket& regularPacket,
    uint64_t packetSize) {
  auto event = std::make_unique<QLogPacketEvent>();
  event->refTime = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
  event->packetSize = packetSize;
  event->eventType = QLogEventType::PacketReceived;
  const ShortHeader* shortHeader = regularPacket.header.asShort();
  if (shortHeader) {
    event->packetType = kShortHeaderPacketType.toString();
  } else {
    event->packetType =
        toQlogString(regularPacket.header.asLong()->getHeaderType()).str();
  }
  if (event->packetType != toString(LongHeader::Types::Retry)) {
    // A Retry packet does not include a packet number.
    event->packetNum = regularPacket.header.getPacketSequenceNum();
  }

  uint64_t numPaddingFrames = 0;
  // looping through the packet to store logs created from frames in the packet
  for (const auto& quicFrame : regularPacket.frames) {
    switch (quicFrame.type()) {
      case QuicFrame::TYPE::PADDING_FRAME: {
        numPaddingFrames += quicFrame.asPaddingFrame()->numFrames;
        break;
      }
      case QuicFrame::TYPE::RST_STREAM_FRAME: {
        const auto& frame = *quicFrame.asRstStreamFrame();
        event->frames.push_back(std::make_unique<RstStreamFrameLog>(
            frame.streamId, frame.errorCode, frame.offset));
        break;
      }
      case QuicFrame::TYPE::CONNECTION_CLOSE_FRAME: {
        const auto& frame = *quicFrame.asConnectionCloseFrame();
        event->frames.push_back(std::make_unique<ConnectionCloseFrameLog>(
            frame.errorCode, frame.reasonPhrase, frame.closingFrameType));
        break;
      }
      case QuicFrame::TYPE::MAX_DATA_FRAME: {
        const auto& frame = *quicFrame.asMaxDataFrame();
        event->frames.push_back(
            std::make_unique<MaxDataFrameLog>(frame.maximumData));
        break;
      }
      case QuicFrame::TYPE::MAX_STREAM_DATA_FRAME: {
        const auto& frame = *quicFrame.asMaxStreamDataFrame();
        event->frames.push_back(std::make_unique<MaxStreamDataFrameLog>(
            frame.streamId, frame.maximumData));
        break;
      }
      case QuicFrame::TYPE::DATA_BLOCKED_FRAME: {
        const auto& frame = *quicFrame.asDataBlockedFrame();
        event->frames.push_back(
            std::make_unique<DataBlockedFrameLog>(frame.dataLimit));
        break;
      }
      case QuicFrame::TYPE::STREAM_DATA_BLOCKED_FRAME: {
        const auto& frame = *quicFrame.asStreamDataBlockedFrame();
        event->frames.push_back(std::make_unique<StreamDataBlockedFrameLog>(
            frame.streamId, frame.dataLimit));
        break;
      }
      case QuicFrame::TYPE::STREAMS_BLOCKED_FRAME: {
        const auto& frame = *quicFrame.asStreamsBlockedFrame();
        event->frames.push_back(std::make_unique<StreamsBlockedFrameLog>(
            frame.streamLimit, frame.isForBidirectional));
        break;
      }
      case QuicFrame::TYPE::READ_ACK_FRAME: {
        const auto& frame = *quicFrame.asReadAckFrame();
        event->frames.push_back(std::make_unique<ReadAckFrameLog>(
            frame.ackBlocks,
            frame.ackDelay,
            frame.frameType,
            frame.maybeLatestRecvdPacketTime,
            frame.maybeLatestRecvdPacketNum,
            frame.recvdPacketsTimestampRanges));
        break;
      }
      case QuicFrame::TYPE::READ_STREAM_FRAME: {
        const auto& frame = *quicFrame.asReadStreamFrame();
        event->frames.push_back(std::make_unique<StreamFrameLog>(
            frame.streamId, frame.offset, frame.data->length(), frame.fin));
        break;
      }
      case QuicFrame::TYPE::READ_CRYPTO_FRAME: {
        const auto& frame = *quicFrame.asReadCryptoFrame();
        event->frames.push_back(std::make_unique<CryptoFrameLog>(
            frame.offset, frame.data->length()));
        break;
      }
      case QuicFrame::TYPE::READ_NEW_TOKEN_FRAME: {
        event->frames.push_back(std::make_unique<ReadNewTokenFrameLog>());
        break;
      }
      case QuicFrame::TYPE::PING_FRAME: {
        event->frames.push_back(std::make_unique<quic::PingFrameLog>());
        break;
      }
      case QuicFrame::TYPE::QUIC_SIMPLE_FRAME: {
        const auto& simpleFrame = *quicFrame.asQuicSimpleFrame();
        addQuicSimpleFrameToEvent(event.get(), simpleFrame);
        break;
      }
      case QuicFrame::TYPE::NOOP_FRAME: {
        break;
      }
      case QuicFrame::TYPE::DATAGRAM_FRAME: {
        const auto& frame = *quicFrame.asDatagramFrame();
        event->frames.push_back(
            std::make_unique<quic::DatagramFrameLog>(frame.length));
        break;
      }
      case QuicFrame::TYPE::IMMEDIATE_ACK_FRAME: {
        event->frames.push_back(std::make_unique<quic::ImmediateAckFrameLog>());
        break;
      }
    }
  }
  if (numPaddingFrames > 0) {
    event->frames.push_back(
        std::make_unique<PaddingFrameLog>(numPaddingFrames));
  }
  return event;
}

std::unique_ptr<QLogPacketEvent> BaseQLogger::createPacketEvent(
    const RegularQuicWritePacket& writePacket,
    uint64_t packetSize) {
  auto event = std::make_unique<QLogPacketEvent>();
  event->refTime = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
  event->packetNum = writePacket.header.getPacketSequenceNum();
  event->packetSize = packetSize;
  event->eventType = QLogEventType::PacketSent;
  const ShortHeader* shortHeader = writePacket.header.asShort();
  if (shortHeader) {
    event->packetType = kShortHeaderPacketType.toString();
  } else {
    event->packetType =
        toQlogString(writePacket.header.asLong()->getHeaderType()).str();
  }

  uint64_t numPaddingFrames = 0;
  // looping through the packet to store logs created from frames in the packet
  for (const auto& quicFrame : writePacket.frames) {
    switch (quicFrame.type()) {
      case QuicWriteFrame::TYPE::PADDING_FRAME:
        numPaddingFrames += quicFrame.asPaddingFrame()->numFrames;
        break;
      case QuicWriteFrame::TYPE::RST_STREAM_FRAME: {
        const RstStreamFrame& frame = *quicFrame.asRstStreamFrame();
        event->frames.push_back(std::make_unique<RstStreamFrameLog>(
            frame.streamId, frame.errorCode, frame.offset));
        break;
      }
      case QuicWriteFrame::TYPE::CONNECTION_CLOSE_FRAME: {
        const ConnectionCloseFrame& frame = *quicFrame.asConnectionCloseFrame();
        event->frames.push_back(std::make_unique<ConnectionCloseFrameLog>(
            frame.errorCode, frame.reasonPhrase, frame.closingFrameType));
        break;
      }
      case QuicWriteFrame::TYPE::MAX_DATA_FRAME: {
        const MaxDataFrame& frame = *quicFrame.asMaxDataFrame();
        event->frames.push_back(
            std::make_unique<MaxDataFrameLog>(frame.maximumData));
        break;
      }
      case QuicWriteFrame::TYPE::MAX_STREAM_DATA_FRAME: {
        const MaxStreamDataFrame& frame = *quicFrame.asMaxStreamDataFrame();
        event->frames.push_back(std::make_unique<MaxStreamDataFrameLog>(
            frame.streamId, frame.maximumData));
        break;
      }
      case QuicWriteFrame::TYPE::STREAMS_BLOCKED_FRAME: {
        const StreamsBlockedFrame& frame = *quicFrame.asStreamsBlockedFrame();
        event->frames.push_back(std::make_unique<StreamsBlockedFrameLog>(
            frame.streamLimit, frame.isForBidirectional));
        break;
      }
      case QuicWriteFrame::TYPE::DATA_BLOCKED_FRAME: {
        const DataBlockedFrame& frame = *quicFrame.asDataBlockedFrame();
        event->frames.push_back(
            std::make_unique<DataBlockedFrameLog>(frame.dataLimit));
        break;
      }
      case QuicWriteFrame::TYPE::STREAM_DATA_BLOCKED_FRAME: {
        const StreamDataBlockedFrame& frame =
            *quicFrame.asStreamDataBlockedFrame();
        event->frames.push_back(std::make_unique<StreamDataBlockedFrameLog>(
            frame.streamId, frame.dataLimit));
        break;
      }
      case QuicWriteFrame::TYPE::WRITE_ACK_FRAME: {
        const WriteAckFrame& frame = *quicFrame.asWriteAckFrame();
        event->frames.push_back(std::make_unique<WriteAckFrameLog>(
            frame.ackBlocks,
            frame.ackDelay,
            frame.frameType,
            frame.maybeLatestRecvdPacketTime,
            frame.maybeLatestRecvdPacketNum,
            frame.recvdPacketsTimestampRanges));
        break;
      }
      case QuicWriteFrame::TYPE::WRITE_STREAM_FRAME: {
        const WriteStreamFrame& frame = *quicFrame.asWriteStreamFrame();
        event->frames.push_back(std::make_unique<StreamFrameLog>(
            frame.streamId, frame.offset, frame.len, frame.fin));
        break;
      }
      case QuicWriteFrame::TYPE::WRITE_CRYPTO_FRAME: {
        const WriteCryptoFrame& frame = *quicFrame.asWriteCryptoFrame();
        event->frames.push_back(
            std::make_unique<CryptoFrameLog>(frame.offset, frame.len));
        break;
      }
      case QuicWriteFrame::TYPE::QUIC_SIMPLE_FRAME: {
        const QuicSimpleFrame& simpleFrame = *quicFrame.asQuicSimpleFrame();
        addQuicSimpleFrameToEvent(event.get(), simpleFrame);
        break;
      }
      case QuicWriteFrame::TYPE::NOOP_FRAME: {
        break;
      }
      case QuicWriteFrame::TYPE::DATAGRAM_FRAME: {
        // TODO
        break;
      }
      case QuicWriteFrame::TYPE::IMMEDIATE_ACK_FRAME: {
        event->frames.push_back(std::make_unique<quic::ImmediateAckFrameLog>());
        break;
      }
      case QuicWriteFrame::TYPE::PING_FRAME: {
        event->frames.push_back(std::make_unique<quic::PingFrameLog>());
        break;
      }
    }
  }
  if (numPaddingFrames > 0) {
    event->frames.push_back(
        std::make_unique<PaddingFrameLog>(numPaddingFrames));
  }
  return event;
}

std::unique_ptr<QLogVersionNegotiationEvent> BaseQLogger::createPacketEvent(
    const VersionNegotiationPacket& versionPacket,
    uint64_t packetSize,
    bool isPacketRecvd) {
  auto event = std::make_unique<QLogVersionNegotiationEvent>();
  event->refTime = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
  event->packetSize = packetSize;
  event->eventType =
      isPacketRecvd ? QLogEventType::PacketReceived : QLogEventType::PacketSent;
  event->packetType = kVersionNegotiationPacketType;
  event->versionLog = std::make_unique<VersionNegotiationLog>(
      VersionNegotiationLog(versionPacket.versions));
  return event;
}

std::unique_ptr<QLogRetryEvent> BaseQLogger::createPacketEvent(
    const RetryPacket& retryPacket,
    uint64_t packetSize,
    bool isPacketRecvd) {
  auto event = std::make_unique<QLogRetryEvent>();
  event->refTime = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now().time_since_epoch());
  event->packetSize = packetSize;
  event->tokenSize = retryPacket.header.getToken().size();
  event->eventType =
      isPacketRecvd ? QLogEventType::PacketReceived : QLogEventType::PacketSent;
  event->packetType = toQlogString(retryPacket.header.getHeaderType()).str();
  return event;
}

} // namespace quic
