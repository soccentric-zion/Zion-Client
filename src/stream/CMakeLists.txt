add_library(zion_stream STATIC
    StreamClient.cc
)

target_include_directories(zion_stream PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(zion_stream PRIVATE
    zion_protos
    ${ZION_GRPC_LIBRARIES}
    spdlog::spdlog
)
