add_library(zion_config STATIC
    client_config.cpp
    client_id.cpp
)

target_include_directories(zion_config PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(zion_config PRIVATE
    spdlog::spdlog
    tomlplusplus::tomlplusplus
    yaml-cpp::yaml-cpp
)
