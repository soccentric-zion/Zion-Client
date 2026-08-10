add_library(zion_run STATIC
    RunOrchestrator.cc
)

target_include_directories(zion_run PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/../Zion-HAL/src
)

target_link_libraries(zion_run PRIVATE
    zion_hal_bridge
    zion_protos
    spdlog::spdlog
)
