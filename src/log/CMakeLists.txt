add_library(zion_log STATIC LogCollector.cc
) target_include_directories(zion_log PRIVATE ${CMAKE_SOURCE_DIR}/src
) target_link_libraries(zion_log PRIVATE zion_protos spdlog::spdlog
)
