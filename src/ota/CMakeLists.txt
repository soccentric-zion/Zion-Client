add_library(zion_ota STATIC OtaClient.cc
) target_include_directories(zion_ota PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(zion_ota PRIVATE zion_protos ${ZION_GRPC_LIBRARIES} spdlog::spdlog
)
