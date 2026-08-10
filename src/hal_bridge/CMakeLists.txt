add_library(zion_hal_bridge STATIC
    ResultSerializer.cc
)

target_include_directories(zion_hal_bridge PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../Zion-HAL/src
)

target_link_libraries(zion_hal_bridge PRIVATE
    zion_protos
    spdlog::spdlog
)
