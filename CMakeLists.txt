cmake_minimum_required (VERSION 3.0)
project (LiveServer)

#set(BUILD_TESTING ON)

add_compile_options(-std=c++14 -Wall -Wextra -Wsign-conversion)

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


# boost
find_package(Boost 1.78.0 COMPONENTS system thread regex REQUIRED)
target_link_libraries(LiveServer PUBLIC ${Boost_LIBRARIES})
target_include_directories(LiveServer PUBLIC ${Boost_INCLUDE_DIR})
message(${Boost_INCLUDE_DIR})


# openssl
find_package(OpenSSL)
message(${OPENSSL_INCLUDE_DIR})
if(OPENSSL_FOUND)
    target_compile_definitions(LiveServer PUBLIC HAVE_OPENSSL)
    target_link_libraries(LiveServer PUBLIC ${OPENSSL_LIBRARIES})
    target_include_directories(LiveServer PUBLIC ${OPENSSL_INCLUDE_DIR})
endif()

# log
message(${CMAKE_SOURCE_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/spdlog)
target_link_libraries(LiveServer PUBLIC spdlog)
target_include_directories(LiveServer PUBLIC 
        ${CMAKE_SOURCE_DIR}/spdlog
        ${CMAKE_SOURCE_DIR}/spdlog/build
)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
