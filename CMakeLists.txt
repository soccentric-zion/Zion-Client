cmake_minimum_required(VERSION 3.16)
project(zion-client VERSION 0.1.0 LANGUAGES CXX) set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) include(cmake/CompilerWarnings.cmake)
include(GNUInstallDirs) # ---------------------------------------------------------------------------
# HAL library (sibling directory) — must come before add_subdirectory(src)
# so that the acu_full_test target is available when registry/CMakeLists.txt
# links against it.
# ---------------------------------------------------------------------------
find_package(Threads REQUIRED) # HAL requires Threads::Threads add_subdirectory(${CMAKE_SOURCE_DIR}/../Zion-HAL ${CMAKE_BINARY_DIR}/zion_hal_build EXCLUDE_FROM_ALL
) # ---------------------------------------------------------------------------
# Dependencies
# --------------------------------------------------------------------------- # spdlog
find_package(spdlog REQUIRED) # toml++ (config file parsing)
find_package(tomlplusplus REQUIRED) # yaml-cpp (modules.yaml registry)
find_package(yaml-cpp REQUIRED)
# Ubuntu 20.04 yaml-cpp doesn't export yaml-cpp::yaml-cpp target, create wrapper
if(NOT TARGET yaml-cpp::yaml-cpp) add_library(yaml-cpp-wrapper INTERFACE) target_link_libraries(yaml-cpp-wrapper INTERFACE yaml-cpp) target_include_directories(yaml-cpp-wrapper INTERFACE ${YAML_CPP_INCLUDE_DIR}) add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp-wrapper)
endif # SQLite3
find_package(SQLite3 REQUIRED) # gRPC + Protobuf (CMake config preferred; pkg-config fallback for Ubuntu 22.04)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG QUIET)
if(gRPC_FOUND) message(STATUS "[zion-client] gRPC ${gRPC_VERSION} via CMake config") set(ZION_GRPC_LIBRARIES gRPC::grpc++ gRPC::grpc) get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION) get_filename_component(GRPC_BIN_DIR "${GRPC_CPP_PLUGIN}" DIRECTORY) find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin HINTS "${GRPC_BIN_DIR}"/usr/local/bin/usr/bin DOC "Path to the grpc_cpp_plugin protoc plugin") if(NOT GRPC_CPP_PLUGIN) message(FATAL_ERROR "[zion-client] grpc_cpp_plugin not found") endif find_program(PROTOC_EXECUTABLE protoc HINTS "${GRPC_BIN_DIR}"/usr/local/bin/usr/bin DOC "Path to the protoc compiler") if(NOT PROTOC_EXECUTABLE) message(FATAL_ERROR "[zion-client] protoc not found") endif
else find_package(PkgConfig REQUIRED) pkg_check_modules(GRPCPP REQUIRED grpc++) message(STATUS "[zion-client] gRPC via pkg-config") set(ZION_GRPC_LIBRARIES ${GRPCPP_LIBRARIES}) find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin HINTS/usr/local/bin/usr/bin/usr/lib/grpc/usr/lib/aarch64-linux-gnu/usr/lib/x86_64-linux-gnu DOC "Path to the grpc_cpp_plugin protoc plugin") if(NOT GRPC_CPP_PLUGIN) message(FATAL_ERROR "[zion-client] grpc_cpp_plugin not found. " "Rebuild the Docker image or pass -DGRPC_CPP_PLUGIN=/path/to/grpc_cpp_plugin.") endif find_program(PROTOC_EXECUTABLE protoc HINTS/usr/local/bin/usr/bin DOC "Path to the protoc compiler") if(NOT PROTOC_EXECUTABLE) message(FATAL_ERROR "[zion-client] protoc not found") endif
endif
message(STATUS "[zion-client] grpc_cpp_plugin: ${GRPC_CPP_PLUGIN}") # ---------------------------------------------------------------------------
# Proto generation
# --------------------------------------------------------------------------- set(PROTO_DIR "${CMAKE_SOURCE_DIR}/../Zion-HAL/protos")
set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/proto_gen")
file(MAKE_DIRECTORY ${PROTO_GEN_DIR}) # Compiler warnings
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror") # Enable position independent code for shared libraries
set(CMAKE_POSITION_INDEPENDENT_CODE ON) set(MAINFOLDER ${PROJECT_BINARY_DIR})
set(EXECUTABLE_OUTPUT_PATH "${MAINFOLDER}/bin")
set(LIBRARY_OUTPUT_PATH "${MAINFOLDER}/lib") # Generated sources
set(PROTO_SRCS ${PROTO_GEN_DIR}/common.pb.cc ${PROTO_GEN_DIR}/hal_service.pb.cc ${PROTO_GEN_DIR}/hal_service.grpc.pb.cc ${PROTO_GEN_DIR}/client_service.pb.cc ${PROTO_GEN_DIR}/client_service.grpc.pb.cc
) add_custom_command(OUTPUT ${PROTO_SRCS} COMMAND ${PROTOC_EXECUTABLE} --proto_path=${PROTO_DIR} --cpp_out=${PROTO_GEN_DIR} --grpc_out=${PROTO_GEN_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} ${PROTO_DIR}/common.proto ${PROTO_DIR}/hal_service.proto ${PROTO_DIR}/client_service.proto DEPENDS ${PROTO_DIR}/common.proto ${PROTO_DIR}/hal_service.proto ${PROTO_DIR}/client_service.proto VERBATIM
) add_library(zion_protos STATIC ${PROTO_SRCS})
target_include_directories(zion_protos PUBLIC ${PROTO_GEN_DIR})
target_link_libraries(zion_protos PUBLIC protobuf::libprotobuf ${ZION_GRPC_LIBRARIES}
) # ---------------------------------------------------------------------------
# Sources
# --------------------------------------------------------------------------- add_subdirectory(src) # ---------------------------------------------------------------------------
# Tests
# --------------------------------------------------------------------------- option(BUILD_TESTS "Build unit and integration tests" ON)
if(BUILD_TESTS) enable_testing add_subdirectory(tests)
endif # ---------------------------------------------------------------------------
# Optional sanitizers
# --------------------------------------------------------------------------- option(ENABLE_SANITIZERS "Enable -fsanitize=address,undefined" OFF)
if(ENABLE_SANITIZERS) target_compile_options(zion-client PRIVATE -fsanitize=address,undefined) target_link_options(zion-client PRIVATE -fsanitize=address,undefined)
endif # ---------------------------------------------------------------------------
# Install
# --------------------------------------------------------------------------- install(TARGETS zion-client RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES conf/client.conf.sample DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/zion RENAME client.conf OPTIONAL
)
install(FILES systemd/zion-client.service DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/system OPTIONAL
)
