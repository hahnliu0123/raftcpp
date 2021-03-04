# 查找所有源文件和头文件
file(GLOB LIB_SRC ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/src/*.h)

# 用源文件生成静态库
add_library(raftcpp STATIC ${LIB_SRC})
target_link_libraries(raftcpp 
                                reyao 
                                pthread
                                dl
                                ${PROTOBUF_LIBRARIES}
                                ${ZLIB_LIBRARIES})         

# 添加项目用到的所有库
# SET(LIBS 
#     raftcpp
#     reyao
#     pthread
#     dl
#     ${PROTOBUF_LIBRARIES}
#     ${ZLIB_LIBRARIES}
# )

# 安装动态库与头文件
install(TARGETS raftcpp
        ARCHIVE DESTINATION /usr/lib
        LIBRARY DESTINATION /usr/lib)
install(FILES ${HEADERS}
        DESTINATION /usr/local/include/raftcpp)


set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)  #项目生成态库输出目录