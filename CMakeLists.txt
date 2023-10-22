#######################################################

cmake_minimum_required (VERSION 3.13)
project (LiveServer VERSION 1.0 LANGUAGES CXX)

set(BUILD_TESTING ON)

add_compile_options(-std=c++20 -Wall -Wextra -Wsign-conversion)

file(GLOB SRC
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/state/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/state/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/state/stream/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/state/stream/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/congestion_control/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/congestion_control/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/observer/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/observer/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/handshake/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/handshake/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/loss/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/loss/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/folly/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/folly/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/folly/*/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/folly/*/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/folly/*/*/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/folly/*/*/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/fizz/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/fizz/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/fizz/*/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/fizz/*/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*/*.hpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*/*.cpp
)

add_executable(LiveServer ${SRC})
target_include_directories(LiveServer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol
    ${CMAKE_CURRENT_SOURCE_DIR}/src/folly
)

#seastar
list(APPEND CMAKE_PREFIX_PATH "/seastar/build/release")
list(APPEND CMAKE_MODULE_PATH "/seastar/cmake")

find_package (Seastar REQUIRED)
find_package (Valgrind REQUIRED)
target_link_libraries(LiveServer PRIVATE Seastar::seastar)

# boost
find_package(Boost COMPONENTS system thread regex REQUIRED)
target_link_libraries(LiveServer PUBLIC ${Boost_LIBRARIES})
target_include_directories(LiveServer PUBLIC ${Boost_INCLUDE_DIR})
message(${Boost_INCLUDE_DIR})


# openssl
set(OPENSSL_ROOT_DIR /usr/local/ssl/*)
find_package (OpenSSL REQUIRED)
message(${OPENSSL_INCLUDE_DIR})
target_compile_definitions(LiveServer PUBLIC HAVE_OPENSSL)
target_link_libraries(LiveServer PUBLIC ${OPENSSL_LIBRARIES})
message(${OPENSSL_LIBRARIES})
target_include_directories(LiveServer PUBLIC ${OPENSSL_INCLUDE_DIR})

# DoubleConversion
list(APPEND CMAKE_PREFIX_PATH "/double-conversion")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(double-conversion REQUIRED)
target_link_libraries(LiveServer PRIVATE /double-conversion/libdouble-conversion.a)
target_include_directories(LiveServer PUBLIC "/double-conversion")


# jemalloc
target_link_libraries(LiveServer PRIVATE /usr/local/lib/libjemalloc.so)
target_include_directories(LiveServer PUBLIC "/usr/local/include")

# sodium
find_package(Sodium REQUIRED)
target_include_directories(LiveServer PUBLIC ${sodium_INCLUDE_DIR})
target_link_libraries(LiveServer PUBLIC sodium)
##############################################################
#for test

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
