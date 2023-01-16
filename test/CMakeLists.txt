
add_compile_options(-fno-access-control)

add_executable(io_test io_test.cpp)
target_link_libraries(io_test LiveServer)
add_test(io_test io_test)
