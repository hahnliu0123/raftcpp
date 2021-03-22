# all test source file
add_executable(raft_test raft_test.cc config.cc)
target_link_libraries(raft_test raftcpp gtest)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  #项目的可执行文件输出目录