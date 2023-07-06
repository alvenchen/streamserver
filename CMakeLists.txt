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
)

add_executable(LiveServer ${SRC})
target_include_directories(LiveServer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol
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
target_include_directories(LiveServer PUBLIC ${OPENSSL_INCLUDE_DIR})

# log
add_subdirectory(${CMAKE_SOURCE_DIR}/spdlog)
target_link_libraries(LiveServer PUBLIC spdlog)
target_include_directories(LiveServer PUBLIC 
        ${CMAKE_BINARY_DIR}/spdlog
)

##############################################################
#for test

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
