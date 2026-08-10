add_library(zion_hal_service STATIC HalServiceImpl.cc
) target_include_directories(zion_hal_service PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/../Zion-HAL/src
) target_link_libraries(zion_hal_service PRIVATE zion_registry zion_hal_bridge zion_protos ${ZION_GRPC_LIBRARIES} protobuf::libprotobuf spdlog::spdlog
)
