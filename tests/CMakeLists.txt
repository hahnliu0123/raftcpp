# all test source file
file(GLOB test_srcs_temp ${PROJECT_SOURCE_DIR}/tests/*_test.cc)
message(status ${test_srcs_temp})

set(test_srcs "")
foreach(test_src_temp ${test_srcs_temp})
    string(REPLACE "//" "/" test_src ${test_src_temp})
    list(APPEND test_srcs ${test_src})
endforeach(test_src_temp ${test_srcs_temp})


foreach(test_src ${test_srcs}) 
    # get filename
    get_filename_component(test_bare_name ${test_src} NAME)
    string(REPLACE ".cc" "" test_bare_name_without_extension ${test_bare_name})
    string(REPLACE "\"" "" test_name ${test_bare_name_without_extension})
	message("test_name=" "${test_name}" " test_src=" "${test_src}")

    # add_executable(${test_name} EXCLUDE_FROM_ALL ${test_src} ${PROJECT_SOURCE_DIR}/tests/config.cc)
    add_executable(${test_name} ${test_src} ${PROJECT_SOURCE_DIR}/tests/config.cc)

    target_link_libraries(${test_name} raftcpp gtest gmock)

    # set target properties
    # set_target_properties(${test_name}
    #     PROPERTIES
    #     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
    #     COMMAND ${test_name}
    # )

endforeach(test_src ${test_srcs})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)  #项目的可执行文件输出目录