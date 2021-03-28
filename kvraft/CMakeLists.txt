file(GLOB test_srcs_temp ${PROJECT_SOURCE_DIR}/kvraft/.cc)
message(status ${test_srcs_temp})

add_executable(kv_clerk kv_clerk.cc)
target_link_libraries(kv_clerk raftcpp)

add_executable(kv_server kv_server.cc)
target_link_libraries(kv_server raftcpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  #项目的可执行文件输出目录