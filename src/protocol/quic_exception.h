/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <folly/Optional.h>
#include <folly/Range.h>
#include <folly/Conv.h>
#include "quic_constants.hpp"
#include <stdexcept>
#include <string>

namespace quic {


    struct QuicErrorCode{
        enum class TYPE {
            APP_ERR_CODE,
            LOCAL_ERR_CODE,
            TRANSPOORT_ERR_CODE
        };

        ~QuicErrorCode();
        QuicErrorCode(const QuicErrorCode& other) noexcept;
        QuicErrorCode(QuicErrorCode&& other) noexcept;
        QuicErrorCode& operator=(const QuicErrorCode& other) noexcept;
        QuicErrorCode& operator=(QuicErrorCode&& other) noexcept;
        bool operator==(QuicErrorCode&& other) const;
        QuicErrorCode(ApplicationErrorCode&& in);
        QuicErrorCode(LocalErrorCode&& in);
        QuicErrorCode(TransportErrorCode&& in);

        TYPE type();

        ApplicationErrorCode* asApplicationErrorCode();
        LocalErrorCode* asLocalErrorCode();
        TransportErrorCode* asTransportErrorCode();

    private:
        void destroy() noexcept;

        TYPE _type;
        union{
            ApplicationErrorCode appErr;
            LocalErrorCode localErr;
            TransportErrorCode transportErr;
        };
    };

    

struct QuicError {
    QuicError(QuicErrorCode codeIn, const std::string& messageIn)
        : code(codeIn), message(messageIn) {}
    explicit QuicError(QuicErrorCode codeIn) : code(codeIn) {}
    bool operator==(const QuicError& other) const {
        return code == other.code && message == other.message;
    }

    QuicErrorCode code;
    std::string message;
};

class QuicTransportException : public std::runtime_error {
    public:
    explicit QuicTransportException(const std::string& msg, TransportErrorCode errCode);
    explicit QuicTransportException(const char* msg, TransportErrorCode errCode);
    explicit QuicTransportException(const std::string& msg, TransportErrorCode errCode, FrameType frameType);
    explicit QuicTransportException(const char* msg, TransportErrorCode errCode, FrameType frameType);

    TransportErrorCode errorCode() const noexcept {
        return errCode_;
    }

    folly::Optional<FrameType> frameType() const noexcept {
        return frameType_;
    }

private:
    TransportErrorCode errCode_;
    folly::Optional<FrameType> frameType_;
};

class QuicInternalException : public std::runtime_error {
    public:
    explicit QuicInternalException(const std::string& msg, LocalErrorCode errorCode);
    explicit QuicInternalException(const char* msg, LocalErrorCode errCode);
    explicit QuicInternalException(folly::StringPiece msg, LocalErrorCode errCode);

    LocalErrorCode errorCode() const noexcept {
        return errorCode_;
    }

private:
    LocalErrorCode errorCode_;
};

class QuicApplicationException : public std::runtime_error {
    public:
    explicit QuicApplicationException(const std::string& msg, ApplicationErrorCode errorCode);
    explicit QuicApplicationException(const char* msg, ApplicationErrorCode errorCode);

    ApplicationErrorCode errorCode() const noexcept {
        return errorCode_;
    }

private:
    ApplicationErrorCode errorCode_;
};

/**
 * Convert the error code to a string.
 */
folly::StringPiece toString(LocalErrorCode code);

// TODO: There's some dynamic string construction happening in this (related to
// CryptoError toString). We should eventually figure out a way to avoid the
// copy on return here as well.
std::string toString(TransportErrorCode code);
std::string toString(QuicErrorCode code);
std::string toString(const QuicError& error);

std::string cryptoErrorToString(TransportErrorCode code);
std::vector<TransportErrorCode> getAllTransportErrorCodes();
std::vector<LocalErrorCode> getAllLocalErrorCodes();

inline std::ostream& operator<<(std::ostream& os, const QuicErrorCode& error) {
    os << toString(error);
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const QuicError& error) {
    os << toString(error);
    return os;
}

} // namespace quic
