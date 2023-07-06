
add_compile_options(-fno-access-control)

add_executable(udp_server udp_server.cpp)
target_link_libraries(udp_server PRIVATE Seastar::seastar)
#add_test(udp_server udp_server)
