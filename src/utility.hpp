#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <atomic>
#include <iostream>
#include <memory>
#include <string>
#include <cstdio>
#include <unordered_map>
#include <functional>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_hash.hpp>

#include "../spdlog/include/spdlog/spdlog.h"
#include "../spdlog/include/spdlog/sinks/rotating_file_sink.h"

using socket_base = boost::asio::socket_base;
using tcp = boost::asio::ip::tcp;
using udp = boost::asio::ip::udp;
using address = boost::asio::ip::address;
using error_code = boost::system::error_code;
using uuid = boost::uuids::uuid;
namespace asio = boost::asio;

namespace LOG = spdlog;
const unsigned int LOG_MAX_SIZE = 1048576 * 200;
const unsigned int LOG_MAX_FLLES = 100;
const auto MAIN_LOG = "mainlog";


#ifdef __SSE2__
#include <emmintrin.h>
inline void spin_loop_pause() noexcept { _mm_pause(); }
// TODO: need verification that the following checks are correct:
#elif defined(_MSC_VER) && _MSC_VER >= 1800 && (defined(_M_X64) || defined(_M_IX86))
#include <intrin.h>
inline void spin_loop_pause() noexcept { _mm_pause(); }
#else
inline void spin_loop_pause() noexcept {}
#endif

#endif