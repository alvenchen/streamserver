
add_compile_options(-fno-access-control)

add_executable(udp_server_test udp_server_test.cpp)
target_link_libraries(udp_server_test PRIVATE Seastar::seastar)
#add_test(udp_server_test udp_server_test)


find_package(fmt)
add_executable(quic_packet_test quic_packet_num_test.cpp)
message(${CMAKE_SOURCE_DIR})
target_include_directories(quic_packet_test PUBLIC 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/protocol
)
target_link_libraries(quic_packet_test PRIVATE ${CMAKE_SOURCE_DIR}/src/protocol/quic_packet_num.cpp
    fmt::fmt)

#add_test(udp_server_test main)