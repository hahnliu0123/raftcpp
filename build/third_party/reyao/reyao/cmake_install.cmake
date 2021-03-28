# Install script for directory: /home/wsl/project/raftcpp/third_party/reyao/reyao

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "DEBUG")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}/usr/lib/libreyao.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/lib/libreyao.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}/usr/lib/libreyao.so"
         RPATH "")
  endif()
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/lib/libreyao.so")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/lib" TYPE SHARED_LIBRARY FILES "/home/wsl/project/raftcpp/build/third_party/reyao/lib/libreyao.so")
  if(EXISTS "$ENV{DESTDIR}/usr/lib/libreyao.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/usr/lib/libreyao.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}/usr/lib/libreyao.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/include/reyao/address.h;/usr/local/include/reyao/asynclog.h;/usr/local/include/reyao/bytearray.h;/usr/local/include/reyao/coroutine.h;/usr/local/include/reyao/endian.h;/usr/local/include/reyao/epoller.h;/usr/local/include/reyao/fdmanager.h;/usr/local/include/reyao/fixedbuffer.h;/usr/local/include/reyao/hook.h;/usr/local/include/reyao/log.h;/usr/local/include/reyao/mutex.h;/usr/local/include/reyao/nocopyable.h;/usr/local/include/reyao/scheduler.h;/usr/local/include/reyao/singleton.h;/usr/local/include/reyao/socket.h;/usr/local/include/reyao/socket_stream.h;/usr/local/include/reyao/stackalloc.h;/usr/local/include/reyao/tcp_client.h;/usr/local/include/reyao/tcp_server.h;/usr/local/include/reyao/thread.h;/usr/local/include/reyao/timer.h;/usr/local/include/reyao/util.h;/usr/local/include/reyao/worker.h;/usr/local/include/reyao/workerthread.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include/reyao" TYPE FILE FILES
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/address.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/asynclog.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/bytearray.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/coroutine.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/endian.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/epoller.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/fdmanager.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/fixedbuffer.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/hook.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/log.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/mutex.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/nocopyable.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/scheduler.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/singleton.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/socket.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/socket_stream.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/stackalloc.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/tcp_client.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/tcp_server.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/thread.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/timer.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/util.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/worker.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/workerthread.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/include/reyao/http/http_parser.h;/usr/local/include/reyao/http/http_request.h;/usr/local/include/reyao/http/http_response.h;/usr/local/include/reyao/http/http_server.h;/usr/local/include/reyao/http/http_servlet.h;/usr/local/include/reyao/http/http_session.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include/reyao/http" TYPE FILE FILES
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/http/http_parser.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/http/http_request.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/http/http_response.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/http/http_server.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/http/http_servlet.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/http/http_session.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/usr/local/include/reyao/rpc/codec.h;/usr/local/include/reyao/rpc/rpc_client.h;/usr/local/include/reyao/rpc/rpc_server.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/usr/local/include/reyao/rpc" TYPE FILE FILES
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/rpc/codec.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/rpc/rpc_client.h"
    "/home/wsl/project/raftcpp/third_party/reyao/reyao/rpc/rpc_server.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/wsl/project/raftcpp/build/third_party/reyao/reyao/tests/cmake_install.cmake")

endif()

